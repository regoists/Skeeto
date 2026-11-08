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
"""PNG killicons -> SpriteCard particle materials (single frame, no mksheet).

Reads the three game folders under assets/ (三角洲 / 战场1 / 战场2042),
writes VTF+VMT under assets/skeeto_killfeed/materials/skeeto/pt/{id}/.
Does NOT deploy to the game install.
"""
from __future__ import annotations

import struct
import subprocess
import sys
import tempfile
from pathlib import Path
import os
import shutil

import numpy as np
from PIL import Image

GAME = _game_root()
BIN = GAME / "bin"
VTEX = BIN / "vtex.exe"
ASSETS = ROOT / "assets"
OUT_ROOT = ASSETS / "skeeto_killfeed" / "materials" / "skeeto" / "pt"
SRC_STAGE = ASSETS / "skeeto_killfeed" / "_src" / "killicon_pt"

# power-of-two canvas for particles (512 is sharp enough; lighter than 1024)
SIZE = 512

# Detect style from PNG filenames (folder names may be locale-garbled in tools).
def detect_style(folder: Path) -> str | None:
    names = " ".join(p.name.lower() for p in folder.glob("*.png"))
    if "battlefield1" in names:
        return "bf1"
    if "scrolling" in names or "2042" in names:
        return "bf2042"
    if "df_" in names or "killicon_df" in names:
        return "deltaforce"
    if "killicon_kill.png" in names and "headshot" in names:
        # 三角洲 uses killicon_kill.png + killicon_df_headshot.png
        return "deltaforce"
    return None


def find_styles() -> list[tuple[str, Path]]:
    found: list[tuple[str, Path]] = []
    for d in ASSETS.iterdir():
        if not d.is_dir() or d.name == "skeeto_killfeed":
            continue
        sid = detect_style(d)
        if sid:
            found.append((sid, d))
    order = {"bf1": 0, "bf2042": 1, "deltaforce": 2}
    found.sort(key=lambda x: order.get(x[0], 99))
    return found


def classify_png(path: Path) -> str | None:
    n = path.name.lower()
    if "headshot" in n:
        return "headshot"
    if "kill" in n:
        return "kill"
    return None


def prepare_rgba(src: Path, size: int = SIZE) -> Image.Image:
    im = Image.open(src).convert("RGBA")
    # Keep square; high-quality downsample for particle use.
    if im.size != (size, size):
        im = im.resize((size, size), Image.Resampling.LANCZOS)
    arr = np.array(im)
    # Zero RGB where alpha==0 (cleaner VTF / no fringe on translucent)
    a = arr[:, :, 3:4]
    arr[:, :, :3] = (arr[:, :, :3].astype(np.uint16) * a // 255).astype(np.uint8)
    return Image.fromarray(arr, "RGBA")


def write_tga32(path: Path, im: Image.Image) -> None:
    """Uncompressed 32-bit TGA (BGRA) for vtex."""
    rgba = np.array(im.convert("RGBA"))
    h, w = rgba.shape[:2]
    bgra = rgba[:, :, [2, 1, 0, 3]].tobytes()
    header = struct.pack(
        "<BBBHHBHHHHBB",
        0,  # id length
        0,  # no palette
        2,  # uncompressed true-color
        0,
        0,
        0,
        0,
        0,  # x/y origin
        w,
        h,
        32,  # bpp
        0x28,  # alpha 8 + top-left origin
    )
    path.write_bytes(header + bgra)


def write_vtex_txt(path: Path) -> None:
    # clamps + nolod; keep mips so scale-down particles stay clean
    path.write_text(
        '"clamps" "1"\n'
        '"clampt" "1"\n'
        '"nolod" "1"\n',
        encoding="ascii",
    )


def write_vmt(path: Path, basetexture: str) -> None:
    # Match existing skeeto particle / effects SpriteCard conventions.
    text = (
        "spritecard\n"
        "{\n"
        f'\t$basetexture "{basetexture}"\n'
        "\t$depthblend 0\n"
        "\t$blendframes 0\n"
        "\t$translucent 1\n"
        "\t$ignorez 1\n"
        "\t$vertexcolor 1\n"
        "\t$vertexalpha 1\n"
        "}\n"
    )
    path.write_text(text, encoding="ascii")


def run_vtex(stem: str, work: Path, mat_src_rel: Path) -> Path:
    """Compile TGA via game materialsrc -> materials, return VTF path."""
    mat_src = GAME / "left4dead2" / "materialsrc" / mat_src_rel
    mat_out = GAME / "left4dead2" / "materials" / mat_src_rel
    mat_src.mkdir(parents=True, exist_ok=True)
    mat_out.mkdir(parents=True, exist_ok=True)

    (mat_src / f"{stem}.tga").write_bytes((work / f"{stem}.tga").read_bytes())
    (mat_src / f"{stem}.txt").write_text((work / f"{stem}.txt").read_text(encoding="ascii"), encoding="ascii")

    for old in mat_out.glob(f"{stem}*"):
        if old.suffix.lower() in {".vtf", ".vmt"}:
            old.unlink()

    r = subprocess.run(
        [str(VTEX), "-nopause", "-game", str(GAME / "left4dead2"), f"{stem}.txt"],
        cwd=str(mat_src),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    vtf = mat_out / f"{stem}.vtf"
    if not vtf.exists():
        raise RuntimeError(f"vtex failed for {stem}:\nSTDOUT:\n{r.stdout}\nSTDERR:\n{r.stderr}")
    pwl = mat_out / f"{stem}.pwl.vtf"
    if pwl.exists():
        pwl.unlink()
    return vtf


def process_one(style: str, kind: str, src: Path) -> None:
    out_dir = OUT_ROOT / style
    out_dir.mkdir(parents=True, exist_ok=True)
    stage = SRC_STAGE / style
    stage.mkdir(parents=True, exist_ok=True)

    im = prepare_rgba(src, SIZE)
    # Keep a clean PNG preview + TGA for recompile
    png_path = stage / f"{kind}.png"
    tga_path = stage / f"{kind}.tga"
    txt_path = stage / f"{kind}.txt"
    im.save(png_path)
    write_tga32(tga_path, im)
    write_vtex_txt(txt_path)

    rel = Path("skeeto") / "pt" / style
    with tempfile.TemporaryDirectory(prefix="vtex_pt_") as tmp:
        tmp_p = Path(tmp)
        (tmp_p / f"{kind}.tga").write_bytes(tga_path.read_bytes())
        (tmp_p / f"{kind}.txt").write_text(txt_path.read_text(encoding="ascii"), encoding="ascii")
        vtf_game = run_vtex(kind, tmp_p, rel)

    vtf_dst = out_dir / f"{kind}.vtf"
    vtf_dst.write_bytes(vtf_game.read_bytes())
    basetexture = f"skeeto/pt/{style}/{kind}"
    write_vmt(out_dir / f"{kind}.vmt", basetexture)

    # vtex must write under game materials/; remove leftovers (user manages deploy).
    try:
        vtf_game.unlink(missing_ok=True)
        for leftover in (GAME / "left4dead2" / "materials" / rel).glob(f"{kind}*"):
            if leftover.suffix.lower() in {".vtf", ".vmt"}:
                leftover.unlink(missing_ok=True)
    except OSError:
        pass

    print(f"OK {basetexture}  vtf={vtf_dst.stat().st_size}  from={src.name}")


def main() -> int:
    if not VTEX.exists():
        print("vtex.exe missing:", VTEX, file=sys.stderr)
        return 1

    styles = find_styles()
    if not styles:
        print("No source folders found under assets/", file=sys.stderr)
        return 1

    print("Styles:", ", ".join(f"{s} <- {p.name}" for s, p in styles))
    n = 0
    for style, folder in styles:
        for png in sorted(folder.glob("*.png")):
            kind = classify_png(png)
            if not kind:
                print("skip", png.name)
                continue
            process_one(style, kind, png)
            n += 1
    print(f"DONE {n} materials -> {OUT_ROOT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
