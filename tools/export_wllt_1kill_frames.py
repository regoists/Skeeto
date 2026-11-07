import subprocess
from pathlib import Path
import os
import shutil
from PIL import Image

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
ff = str(_ffmpeg())
vid = ROOT / "assets" / "wllt_1kill.mp4"
out_dir = ROOT / "assets" / "wllt_1kill_frames_256"
tmp_dir = out_dir / "_tmp_png"

out_dir.mkdir(parents=True, exist_ok=True)
if tmp_dir.exists():
    for p in tmp_dir.glob("*"):
        p.unlink()
else:
    tmp_dir.mkdir()

for p in out_dir.glob("*.tga"):
    p.unlink()

# Probe duration
probe = subprocess.run(
    [ff, "-hide_banner", "-i", str(vid)],
    capture_output=True,
    text=True,
)
# duration is on stderr for ffmpeg -i
import re

m = re.search(r"Duration:\s*(\d+):(\d+):(\d+\.\d+)", probe.stderr)
if not m:
    raise SystemExit("cannot parse duration:\n" + probe.stderr)
duration = int(m.group(1)) * 3600 + int(m.group(2)) * 60 + float(m.group(3))

n = 30
times = [(i + 0.5) * duration / n for i in range(n)]

# Black -> alpha, then force rgba; lanczos to 256
vf = "scale=256:256:flags=lanczos,colorkey=0x000000:0.12:0.15,format=rgba"

print(f"duration={duration:.3f}s, exporting {n} frames -> {out_dir}")
for i, t in enumerate(times):
    png = tmp_dir / f"frame_{i:02d}.png"
    cmd = [
        ff,
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        "-ss",
        f"{t:.6f}",
        "-i",
        str(vid),
        "-frames:v",
        "1",
        "-vf",
        vf,
        str(png),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 or not png.exists():
        raise SystemExit(f"ffmpeg failed at frame {i} t={t}: {r.stderr}")

    im = Image.open(png).convert("RGBA")
    pixels = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r_, g_, b_, a = pixels[x, y]
            if a == 0:
                continue
            if r_ <= 12 and g_ <= 12 and b_ <= 12:
                pixels[x, y] = (0, 0, 0, 0)

    tga = out_dir / f"frame_{i:02d}.tga"
    im.save(tga, format="TGA")
    print(f"  [{i + 1:02d}/{n}] t={t:.3f}s -> {tga.name} {im.size} RGBA")

for p in tmp_dir.glob("*"):
    p.unlink()
tmp_dir.rmdir()

files = sorted(out_dir.glob("frame_*.tga"))
print(f"done: {len(files)} TGA files")
print(f"sizes: {files[0].name}={files[0].stat().st_size}, {files[-1].name}={files[-1].stat().st_size}")
