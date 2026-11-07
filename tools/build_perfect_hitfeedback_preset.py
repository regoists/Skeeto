from __future__ import annotations

import io
import json
import shutil
import struct
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "reference" / "AAAAA击杀反馈最终完美版AAAAA"
HIT_SOURCE = SOURCE / "击杀反馈2-声音版本2(推荐)"
SKULL_SOURCE = SOURCE / "击杀反馈3-骷髅图标（必须)"
if not SKULL_SOURCE.exists():
    SKULL_SOURCE = SOURCE / "击杀反馈3-骷髅图标（必须）"

OUTPUT = ROOT / "assets" / "完美版击杀反馈_Skeeto预设"
MATERIAL_ROOT = OUTPUT / "materials" / "skeeto" / "perfect_hitfeedback"
SOUND_ROOT = OUTPUT / "sound" / "skeeto" / "perfect_hitfeedback"
JSON_ROOT = OUTPUT / "skeeto"

REFERENCE_W = 1920
REFERENCE_H = 1080
HIT_SIZE = 128
COMPOSITE_W = 256
COMPOSITE_H = 512
COMPOSITE_LEFT = (REFERENCE_W - COMPOSITE_W) // 2
COMPOSITE_TOP = (REFERENCE_H - COMPOSITE_H) // 2


def read_dxt5_vtf(path: Path) -> list[Image.Image]:
    data = path.read_bytes()
    if data[:4] != b"VTF\0":
        raise ValueError(f"not a VTF: {path}")
    header_size = struct.unpack_from("<I", data, 12)[0]
    width, height = struct.unpack_from("<HH", data, 16)
    frames = struct.unpack_from("<H", data, 24)[0]
    image_format = struct.unpack_from("<I", data, 52)[0]
    mip_count = data[56]
    if image_format != 15 or mip_count != 1:
        raise ValueError(f"expected single-mip DXT5 VTF: {path}")
    frame_bytes = ((width + 3) // 4) * ((height + 3) // 4) * 16
    result = []
    for index in range(frames):
        begin = header_size + index * frame_bytes
        block = data[begin : begin + frame_bytes]
        if len(block) != frame_bytes:
            raise ValueError(f"truncated VTF frame: {path} #{index}")
        result.append(Image.frombytes("RGBA", (width, height), block, "bcn", (3,)))
    return result


def write_dxt5_vtf(path: Path, image: Image.Image) -> None:
    image = image.convert("RGBA")
    width, height = image.size
    dds = io.BytesIO()
    image.save(dds, format="DDS", pixel_format="DXT5")
    payload = dds.getvalue()[128:]
    expected = ((width + 3) // 4) * ((height + 3) // 4) * 16
    if len(payload) != expected:
        raise ValueError(f"unexpected DXT5 payload size: {path}")

    header = bytearray(80)
    header[:4] = b"VTF\0"
    struct.pack_into("<II", header, 4, 7, 2)
    struct.pack_into("<I", header, 12, 80)
    struct.pack_into("<HH", header, 16, width, height)
    struct.pack_into("<I", header, 20, 0x0000230C)  # clamp S/T, no mip/LOD, alpha
    struct.pack_into("<HH", header, 24, 1, 0)
    struct.pack_into("<3f", header, 32, 0.0, 0.0, 0.0)
    struct.pack_into("<f", header, 48, 1.0)
    struct.pack_into("<I", header, 52, 15)  # DXT5
    header[56] = 1
    struct.pack_into("<I", header, 57, 0xFFFFFFFF)
    header[61] = 0
    header[62] = 0
    struct.pack_into("<H", header, 63, 1)
    path.write_bytes(header + payload)


def write_material(path: Path, texture: str, scale_x: float, scale_y: float) -> None:
    text = (
        '"UnlitGeneric"\n'
        "{\n"
        f'    "$basetexture" "{texture}"\n'
        '    "$translucent" "1"\n'
        '    "$ignorez" "1"\n'
        '    "$nolod" "1"\n'
        '    "$nomip" "1"\n'
        '    "$no_fullbright" "1"\n'
        f'    "$basetexturetransform" "center .5 .5 scale {scale_x:g} {scale_y:g} rotate 0 translate 0 0"\n'
        "}\n"
    )
    path.write_text(text, encoding="utf-8", newline="\n")


def alpha_composite_at(canvas: Image.Image, image: Image.Image, x: int, y: int) -> None:
    canvas.alpha_composite(image, (x, y))


def make_hit_frames(kind: str) -> list[Image.Image]:
    frames = read_dxt5_vtf(HIT_SOURCE / "materials" / "hitmarker" / f"{kind}.vtf")
    return [frame.resize((HIT_SIZE, HIT_SIZE), Image.Resampling.LANCZOS) for frame in frames]


def make_skull_frames(kind: str) -> list[Image.Image]:
    source_root = SKULL_SOURCE / "materials" / "vgui" / "killicon_cod19"
    names = [f"{kind}_scale{index:02d}" for index in range(1, 8)] + [kind]
    result = []
    for name in names:
        frame = read_dxt5_vtf(source_root / f"{name}.vtf")[0]
        # Match the original full-screen overlay at the 1920x1080 reference size.
        frame = frame.resize((REFERENCE_W, REFERENCE_H), Image.Resampling.BILINEAR)
        result.append(
            frame.crop(
                (
                    COMPOSITE_LEFT,
                    COMPOSITE_TOP,
                    COMPOSITE_LEFT + COMPOSITE_W,
                    COMPOSITE_TOP + COMPOSITE_H,
                )
            )
        )
    return result


def save_sequence(name: str, frames: list[Image.Image], scale_x: float, scale_y: float) -> list[str]:
    sequence_dir = MATERIAL_ROOT / name
    sequence_dir.mkdir(parents=True, exist_ok=True)
    paths = []
    for index, frame in enumerate(frames):
        stem = f"frame_{index:02d}"
        write_dxt5_vtf(sequence_dir / f"{stem}.vtf", frame)
        material_name = f"skeeto/perfect_hitfeedback/{name}/{stem}"
        write_material(sequence_dir / f"{stem}.vmt", material_name, scale_x, scale_y)
        paths.append(material_name)
    return paths


def compose_kill(hit_kind: str, skull_kind: str) -> list[Image.Image]:
    hit_frames = make_hit_frames(hit_kind)
    skull_frames = make_skull_frames(skull_kind)
    frames = []
    for index, hit_frame in enumerate(hit_frames):
        canvas = Image.new("RGBA", (COMPOSITE_W, COMPOSITE_H), (0, 0, 0, 0))
        skull_index = min(int((index * 1000 / 31) / 38), len(skull_frames) - 1)
        alpha_composite_at(canvas, skull_frames[skull_index], 0, 0)
        alpha_composite_at(
            canvas,
            hit_frame,
            (COMPOSITE_W - HIT_SIZE) // 2,
            (COMPOSITE_H - HIT_SIZE) // 2,
        )
        frames.append(canvas)
    # The sequence remains on this skull-only frame until overlay_ms clears it.
    final_frame = Image.new("RGBA", (COMPOSITE_W, COMPOSITE_H), (0, 0, 0, 0))
    alpha_composite_at(final_frame, skull_frames[-1], 0, 0)
    frames.append(final_frame)
    return frames


def style_json(channel: str, display_name: str, hit: list[str], kill: list[str], headshot: list[str]) -> dict:
    return {
        "id": f"{channel}_perfect_hitfeedback",
        "name": display_name,
        "channel": channel,
        "sort": 210,
        "streak": {"enabled": False, "wrap": 0, "priority": 10},
        "hit": {
            "priority": 0,
            "overlay_sequence": hit,
            "overlay_step_ms": 32,
            "overlay_ms": 340,
            "particle": "",
            "sound": "skeeto/perfect_hitfeedback/hit.mp3",
        },
        "kill": {
            "priority": 10,
            "overlay_sequence": kill,
            "overlay_step_ms": 32,
            "overlay_ms": 1080,
            "particle": "",
            "sound": "skeeto/perfect_hitfeedback/kill.mp3",
        },
        "headshot": {
            "priority": 100,
            "overlay_sequence": headshot,
            "overlay_step_ms": 32,
            "overlay_ms": 1080,
            "particle": "",
            "sound": "skeeto/perfect_hitfeedback/headshot.mp3",
        },
    }


def main() -> None:
    MATERIAL_ROOT.mkdir(parents=True, exist_ok=True)
    SOUND_ROOT.mkdir(parents=True, exist_ok=True)
    JSON_ROOT.mkdir(parents=True, exist_ok=True)

    hit_frames = make_hit_frames("hit")
    hit_paths = save_sequence(
        "hit", hit_frames, REFERENCE_W / HIT_SIZE, REFERENCE_H / HIT_SIZE
    )
    kill_paths = save_sequence(
        "kill", compose_kill("kill", "kill_normal"),
        REFERENCE_W / COMPOSITE_W, REFERENCE_H / COMPOSITE_H,
    )
    headshot_paths = save_sequence(
        "headshot", compose_kill("headshot", "kill_headshot"),
        REFERENCE_W / COMPOSITE_W, REFERENCE_H / COMPOSITE_H,
    )

    sound_source = HIT_SOURCE / "sound" / "hitmarker"
    for name in ("hit", "hit_head", "kill", "headshot"):
        shutil.copy2(sound_source / f"{name}.mp3", SOUND_ROOT / f"{name}.mp3")

    configs = (
        ("ci", "普感  完美版击杀反馈"),
        ("si", "特感  完美版击杀反馈"),
    )
    for channel, display_name in configs:
        data = style_json(channel, display_name, hit_paths, kill_paths, headshot_paths)
        path = JSON_ROOT / f"skeeto_{channel}_perfect_hitfeedback.json"
        path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    readme = (
        "Skeeto 完美版击杀反馈还原预设\n\n"
        "将本文件夹内的 materials、sound、skeeto 三个目录复制到 left4dead2。\n"
        "进游戏后分别在普感素材包、特感素材包中选择“完美版击杀反馈”。\n\n"
        "素材来源：击杀反馈2-声音版本2(推荐)及骷髅图标必要素材。\n"
        "该版本不安装 VScript，不覆盖 resource/ui/hud/itempickup.res。\n"
        "普通命中使用原动态素材的逐帧还原；击杀和爆头击杀已预合成中央标记与骷髅动画。\n"
    )
    (OUTPUT / "使用说明.txt").write_text(readme, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
