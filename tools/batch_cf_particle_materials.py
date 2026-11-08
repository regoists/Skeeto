ROOT = Path(__file__).resolve().parents[1]
def _game_root():
    env = os.environ.get('SKEETO_GAME_ROOT', '').strip()
    if env: return Path(env)
    raise SystemExit(
        'SKEETO_GAME_ROOT is not set. Set it to your Left 4 Dead 2 install '
        '(shared scripts must not hardcode a machine path). '
        'Laptop example was D:\\Game\\steamapps\\common\\Left 4 Dead 2'
    )
def _ffmpeg():
    env = os.environ.get('SKEETO_FFMPEG', '').strip()
    if env and Path(env).exists(): return Path(env)
    w = shutil.which('ffmpeg')
    if w: return Path(w)
    for c in [ROOT/'tools'/'ffmpeg'/'ffmpeg.exe', ROOT/'tools'/'ffmpeg'/'ffmpeg-master-latest-win64-gpl-shared'/'bin'/'ffmpeg.exe']:
        if c.exists(): return c
    raise FileNotFoundError('ffmpeg not found; set SKEETO_FFMPEG or PATH')
"""Batch: CF green-screen mp4 -> RGBA frames -> mksheet -> vtex -> vmt.

Clips: cf_1kill..cf_10kill, cf_headshot, cf_knifed
Materials: particles/hstm/hstm_cf_*

Differences vs wllt pipeline:
- Chroma key on green (~0x00D600), not black background
- Same 0.75 horizontal pre-squash for 16:9 screen-space particles
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
import os
import shutil

from PIL import Image

FF_CANDIDATES = [
    _ffmpeg(),
    _ffmpeg(),
]
ASSETS = ROOT / "assets"
GAME = _game_root()
BIN = GAME / "bin"
MAT_SRC = GAME / "left4dead2" / "materialsrc" / "particles" / "hstm"
MAT_OUT = GAME / "left4dead2" / "materials" / "particles" / "hstm"
PKG = Path(
    str(ROOT / "archive" / "legacy_hstm_package")
    r"\loose_into_left4dead2\materials\particles\hstm"
)

BASE_DURATION = 1.18
BASE_FRAMES = 30
FRAME_SIZE = 256
MAX_FRAMES_AT_256 = 64
TARGET_LONG_FRAMES = 60

# 16:9 vs engine 4:3 screen-space stretch compensation
ASPECT_COMPENSATE_X = 0.75

# Measured CF green screen corner ~ (0, 214, 0)
GREEN_KEY = (0, 214, 0)
# Similarity / blend for ffmpeg chromakey (0..1)
CHROMA_SIMILARITY = 0.12
CHROMA_BLEND = 0.18

CLIPS = [
    "1kill",
    "2kill",
    "3kill",
    "4kill",
    "5kill",
    "6kill",
    "7kill",
    "8kill",
    "9kill",
    "10kill",
    "headshot",
    "knifed",
]


def find_ffmpeg() -> Path:
    for p in FF_CANDIDATES:
        if p.exists():
            return p
    raise FileNotFoundError("ffmpeg not found")


FF = find_ffmpeg()


def frame_count_for_duration(duration: float) -> int:
    n = int(round(BASE_FRAMES * (duration / BASE_DURATION)))
    n = max(BASE_FRAMES, n)
    if n > MAX_FRAMES_AT_256:
        n = min(TARGET_LONG_FRAMES, MAX_FRAMES_AT_256)
    return n


def probe_duration(vid: Path) -> float:
    r = subprocess.run([str(FF), "-hide_banner", "-i", str(vid)], capture_output=True, text=True)
    m = re.search(r"Duration:\s*(\d+):(\d+):(\d+\.\d+)", r.stderr)
    if not m:
        raise RuntimeError(f"duration parse failed for {vid}\n{r.stderr}")
    return int(m.group(1)) * 3600 + int(m.group(2)) * 60 + float(m.group(3))


def squash_artwork_horizontal(im: Image.Image, sx: float = ASPECT_COMPENSATE_X) -> Image.Image:
    """Keep canvas WxH; shrink artwork width only; transparent L/R padding."""
    if sx >= 0.999:
        return im
    w, h = im.size
    new_w = max(1, int(round(w * sx)))
    if new_w == w:
        return im
    scaled = im.resize((new_w, h), Image.Resampling.LANCZOS)
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    x0 = (w - new_w) // 2
    out.paste(scaled, (x0, 0), scaled)
    return out


def refine_green_alpha(im: Image.Image) -> Image.Image:
    """Post-pass: kill residual green + mild despill on semi-transparent edges."""
    px = im.load()
    w, h = im.size
    kr, kg, kb = GREEN_KEY
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            # Near-key green (or dark leftover from keyed areas)
            if g >= 90 and g >= r + 40 and g >= b + 40:
                px[x, y] = (0, 0, 0, 0)
                continue
            if r <= 10 and g <= 10 and b <= 10:
                px[x, y] = (0, 0, 0, 0)
                continue
            # Despill: pull green toward luma of R/B on fringe
            if g > r and g > b and (g - max(r, b)) > 12:
                g2 = max(r, b)
                px[x, y] = (r, g2, b, a)
    return im


def export_frames(vid: Path, out_dir: Path, n_frames: int, size: int) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    tmp = out_dir / "_tmp_png"
    if tmp.exists():
        for p in tmp.glob("*"):
            p.unlink()
    else:
        tmp.mkdir()
    for p in out_dir.glob("frame_*.tga"):
        p.unlink()

    duration = probe_duration(vid)
    times = [(i + 0.5) * duration / n_frames for i in range(n_frames)]
    # chromakey: green screen; then force rgba
    key_hex = f"0x{GREEN_KEY[0]:02X}{GREEN_KEY[1]:02X}{GREEN_KEY[2]:02X}"
    vf = (
        f"scale={size}:{size}:flags=lanczos,"
        f"chromakey={key_hex}:{CHROMA_SIMILARITY}:{CHROMA_BLEND},"
        f"format=rgba"
    )
    print(f"  export {n_frames} frames @ {size}x{size} from {vid.name} ({duration:.3f}s)")
    print(f"  chromakey {key_hex} sim={CHROMA_SIMILARITY} blend={CHROMA_BLEND}")
    print(f"  aspect compensate X={ASPECT_COMPENSATE_X} (canvas stays {size}x{size})")

    for i, t in enumerate(times):
        png = tmp / f"frame_{i:02d}.png"
        cmd = [
            str(FF), "-y", "-hide_banner", "-loglevel", "error",
            "-ss", f"{t:.6f}", "-i", str(vid),
            "-frames:v", "1", "-vf", vf, str(png),
        ]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0 or not png.exists():
            raise RuntimeError(f"ffmpeg fail frame {i}: {r.stderr}")

        im = Image.open(png).convert("RGBA")
        im = refine_green_alpha(im)
        im = squash_artwork_horizontal(im, ASPECT_COMPENSATE_X)
        im.save(out_dir / f"frame_{i:02d}.tga", format="TGA")

    for p in tmp.glob("*"):
        p.unlink()
    tmp.rmdir()


def write_mks(out_dir: Path, name: str, n_frames: int) -> Path:
    lines = ["sequence 0"]
    for i in range(n_frames):
        lines.append(f"frame frame_{i:02d}.tga 1")
    mks = out_dir / f"{name}.mks"
    mks.write_text("\n".join(lines) + "\n", encoding="ascii")
    return mks


def run_mksheet(out_dir: Path, name: str) -> None:
    for ext in (".sht", ".tga"):
        p = out_dir / f"{name}{ext}"
        if p.exists():
            p.unlink()

    mksheet = BIN / "mksheet.exe"
    cmd = [str(mksheet), f"{name}.mks", f"{name}.sht", f"{name}.tga"]
    print(f"  mksheet {name}")
    r = subprocess.run(cmd, cwd=str(out_dir), capture_output=True, text=True)
    out = (r.stdout or "") + (r.stderr or "")
    if "Packing error" in out or "failed to pack" in out:
        raise RuntimeError(f"mksheet packing failed:\n{out}")
    if not (out_dir / f"{name}.sht").exists() or not (out_dir / f"{name}.tga").exists():
        raise RuntimeError(f"mksheet failed:\n{out}")
    print(f"  sheet TGA bytes={(out_dir / f'{name}.tga').stat().st_size}")


def run_vtex(name: str, out_dir: Path) -> Path:
    MAT_SRC.mkdir(parents=True, exist_ok=True)
    MAT_OUT.mkdir(parents=True, exist_ok=True)
    for ext in (".sht", ".tga"):
        src = out_dir / f"{name}{ext}"
        (MAT_SRC / f"{name}{ext}").write_bytes(src.read_bytes())

    for old in MAT_OUT.glob(f"{name}*.vtf"):
        old.unlink()

    vtex = BIN / "vtex.exe"
    print(f"  vtex {name}")
    r = subprocess.run(
        [str(vtex), "-nopause", "-game", str(GAME / "left4dead2"), f"{name}.sht"],
        cwd=str(MAT_SRC),
        capture_output=True,
        text=True,
    )
    vtf = MAT_OUT / f"{name}.vtf"
    if not vtf.exists():
        raise RuntimeError(f"vtex failed, no VTF:\n{r.stdout}\n{r.stderr}")
    pwl = MAT_OUT / f"{name}.pwl.vtf"
    if pwl.exists():
        pwl.unlink()
    return vtf


def write_vmt(name: str) -> Path:
    text = (
        "spritecard\n"
        "{\n"
        f'\t$basetexture "particles/hstm/{name}"\n'
        "\t$depthblend 0\n"
        "\t$blendframes 0\n"
        "\t$translucent 1\n"
        "\t$ignorez 1\n"
        "}\n"
    )
    vmt = MAT_OUT / f"{name}.vmt"
    vmt.write_text(text, encoding="ascii")
    return vmt


def copy_to_packs(name: str) -> None:
    PKG.mkdir(parents=True, exist_ok=True)
    for ext in (".vtf", ".vmt"):
        src = MAT_OUT / f"{name}{ext}"
        (PKG / f"{name}{ext}").write_bytes(src.read_bytes())


def process_clip(clip: str) -> None:
    name = f"hstm_cf_{clip}"
    vid = ASSETS / f"cf_{clip}.mp4"
    out_dir = ASSETS / f"cf_{clip}_frames"
    if not vid.exists():
        raise FileNotFoundError(vid)

    duration = probe_duration(vid)
    n_frames = frame_count_for_duration(duration)

    print(f"=== {name} ===")
    print(f"  duration={duration:.3f}s -> {n_frames} frames @ {FRAME_SIZE} (cap={MAX_FRAMES_AT_256})")

    export_frames(vid, out_dir, n_frames, FRAME_SIZE)
    write_mks(out_dir, name, n_frames)
    run_mksheet(out_dir, name)

    vtf = run_vtex(name, out_dir)
    vmt = write_vmt(name)
    copy_to_packs(name)
    print(f"  OK material: particles/hstm/{name}")
    print(f"      VTF={vtf.stat().st_size} frames={n_frames} size={FRAME_SIZE} VMT={vmt}")


def main() -> None:
    clips = sys.argv[1:] or list(CLIPS)
    # allow numeric shorthand: 1 -> 1kill
    resolved = []
    for c in clips:
        if c.isdigit():
            resolved.append(f"{c}kill")
        else:
            resolved.append(c)
    for clip in resolved:
        process_clip(clip)
    print("ALL DONE")
    for clip in resolved:
        name = f"hstm_cf_{clip}"
        vtf = MAT_OUT / f"{name}.vtf"
        print(f"  {name}: vtf={vtf.exists()} ({vtf.stat().st_size if vtf.exists() else 0})")


if __name__ == "__main__":
    main()
