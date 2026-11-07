#!/usr/bin/env python3
"""Read-only static checks for a Skeeto hit-feedback asset root.

Usage: python validate_feedback.py <asset-root> [--dependency-root <asset-root> ...]
This cannot validate actual particle names, Source material rendering or gameplay.
"""

import argparse
import json
import re
import struct
import sys
from pathlib import Path


CHANNELS = {"ci", "si", "ff", "fx"}
EVENT_KEYS = {"hit", "kill", "headshot", "melee", "streak_default"}
EVENT_FIELDS = {
    "priority", "overlay", "overlay_sequence", "overlay_step_ms", "overlay_ms",
    "duration", "overlay_duration", "particle", "particles", "sound", "sounds", "world",
}
TOP_FIELDS = {"id", "name", "channel", "sort", "note", "streak"}
PARTICLE_WARNING = "Particle effect names and load manifests cannot be verified from JSON alone; check the PCF and observe in-game"


def no_duplicate_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("Duplicate JSON key: " + key)
        result[key] = value
    return result


def utf8_length(value):
    return len(value.encode("utf-8"))


class Validator:
    def __init__(self, roots):
        self.roots = [root.resolve() for root in roots]
        self.errors = []
        self.warnings = []
        self.ids = {}
        self.materials_checked = set()
        self.particle_names = set()

    def error(self, where, message):
        self.errors.append(f"{where}: {message}")

    def warn(self, where, message):
        self.warnings.append(f"{where}: {message}")

    def relative(self, value, where, maximum, prefix=""):
        if not isinstance(value, str) or not value:
            self.error(where, "Non-empty string required")
            return None
        if utf8_length(value) > maximum:
            self.error(where, f"UTF-8 path exceeds {maximum} bytes; Skeeto will truncate")
        normalized = value.replace("\\", "/")
        if normalized != value:
            self.warn(where, "Prefer / as the path separator")
        parts = normalized.split("/")
        if normalized.startswith("/") or ":" in parts[0] or any(p in ("", ".", "..") for p in parts):
            self.error(where, "Path must not be absolute and must not contain empty segments, . or ..")
            return None
        if not re.fullmatch(r"[A-Za-z0-9_./-]+", normalized):
            self.error(where, "Asset paths allow only ASCII letters, digits, /, _, -, . to avoid console parse or cross-machine encoding issues")
            return None
        return prefix + normalized

    def find(self, relative):
        for root in self.roots:
            path = root.joinpath(*relative.split("/"))
            if path.is_file():
                return path
        return None

    def material(self, name, where):
        path = self.relative(name, where, 95)
        if path is None:
            return
        if path.startswith("materials/") or path.lower().endswith((".vmt", ".vtf")):
            self.error(where, "overlay must be a material path without materials/ or an extension")
            return
        vmt_rel = "materials/" + path + ".vmt"
        vmt_path = self.find(vmt_rel)
        if not vmt_path:
            self.error(where, "VMT not found: " + vmt_rel)
            return
        if vmt_rel in self.materials_checked:
            return
        self.materials_checked.add(vmt_rel)
        try:
            text = vmt_path.read_text(encoding="utf-8-sig", errors="replace")
        except OSError as exc:
            self.error(vmt_rel, f"Cannot read VMT: {exc}")
            return
        match = re.search(r'(?im)^\s*"?\$basetexture"?\s+"?([^\s"\r\n]+)', text)
        if not match:
            self.warn(vmt_rel, "No $basetexture found; material may use another texture path—check manually")
            return
        texture = self.relative(match.group(1), vmt_rel + " $basetexture", 95)
        if texture is None:
            return
        if texture.startswith("materials/") or texture.lower().endswith(".vtf"):
            self.error(vmt_rel, "$basetexture must not contain materials/ or .vtf")
            return
        vtf_rel = "materials/" + texture + ".vtf"
        vtf_path = self.find(vtf_rel)
        if not vtf_path:
            self.error(vmt_rel, "VTF not found: " + vtf_rel)
            return
        self.vtf(vtf_path, vtf_rel, text)

    def vtf(self, path, where, vmt_text):
        try:
            with path.open("rb") as file:
                header = file.read(96)
            file_bytes = path.stat().st_size
        except OSError as exc:
            self.error(where, f"Cannot read VTF: {exc}")
            return
        if len(header) < 64 or header[:4] != b"VTF\x00":
            self.error(where, "Not a valid VTF header")
            return
        header_size = struct.unpack_from("<I", header, 12)[0]
        width, height = struct.unpack_from("<HH", header, 16)
        flags = struct.unpack_from("<I", header, 20)[0]
        frames = struct.unpack_from("<H", header, 24)[0]
        image_format = struct.unpack_from("<I", header, 52)[0]
        mip_count = header[56]
        if not width or not height or not frames or not mip_count or header_size > file_bytes:
            self.error(where, "Abnormal size, frame count, mip count, or header length")
            return
        if image_format in (13, 15):
            block = 8 if image_format == 13 else 16
            frame_bytes = ((width + 3) // 4) * ((height + 3) // 4) * block
        elif image_format in (0, 12):
            frame_bytes = width * height * 4
        else:
            self.warn(where, f"Image format id {image_format} has no payload-length check")
            frame_bytes = 0
        if frame_bytes and file_bytes < header_size + frame_bytes * frames:
            self.error(where, "File is too small for all frames at the highest resolution")
        if width * height >= 1920 * 1080:
            self.warn(where, f"Canvas {width}x{height} is large; for a small icon, check empty margins and frame count")
        if "$basetexturetransform" in vmt_text.lower() and flags & 0xC != 0xC:
            self.warn(where, "VMT uses a texture transform, but the VTF does not set Clamp S/T")

    def string(self, value, where, maximum, required=False):
        if value is None or value == "":
            if required:
                self.error(where, "Must not be empty")
            return None
        if not isinstance(value, str):
            self.error(where, "String required")
            return None
        if utf8_length(value) > maximum:
            self.error(where, f"UTF-8 content exceeds {maximum} bytes; Skeeto will truncate")
        return value

    def integer(self, value, where, low=None, high=None):
        if isinstance(value, bool) or not isinstance(value, int):
            self.error(where, "Integer required")
            return
        if low is not None and value < low or high is not None and value > high:
            self.warn(where, f"Value {value} is outside suggested/implementation range {low}..{high}")

    def strings_array(self, value, where, count_limit, byte_limit, kind):
        if not isinstance(value, list):
            self.error(where, "String array required")
            return
        if len(value) > count_limit:
            self.error(where, f"Exceeds Skeeto limit of {count_limit} items; extras are not read")
        if not value:
            self.warn(where, "Empty array produces no effect")
        for index, item in enumerate(value):
            item_where = f"{where}[{index}]"
            if kind == "material":
                self.material(item, item_where)
            elif kind == "sound":
                self.sound(item, item_where)
            else:
                name = self.string(item, item_where, byte_limit, required=True)
                if name:
                    self.particle_names.add(name)

    def sound(self, value, where):
        path = self.relative(value, where, 95)
        if path is None:
            return
        if path.startswith("sound/"):
            self.error(where, "sound paths are relative to sound/ and must not include a sound/ prefix")
            return
        if Path(path).suffix.lower() not in (".mp3", ".wav"):
            self.warn(where, "Audio should usually use an explicit .mp3 or .wav extension")
        if not self.find("sound/" + path):
            self.error(where, "Audio not found: sound/" + path)

    def event(self, value, where):
        if not isinstance(value, dict):
            self.error(where, "Event must be a JSON object")
            return
        unknown = set(value) - EVENT_FIELDS
        for key in sorted(unknown):
            self.warn(where + "." + key, "Current Skeeto does not parse this event field")
        if "priority" in value:
            self.integer(value["priority"], where + ".priority")
        if "overlay" in value and value["overlay"] != "":
            self.material(value["overlay"], where + ".overlay")
        if "overlay_sequence" in value:
            self.strings_array(value["overlay_sequence"], where + ".overlay_sequence", 30, 95, "material")
            if value.get("overlay"):
                self.warn(where, "When overlay_sequence is present, a single overlay is overridden")
        if "overlay_step_ms" in value:
            self.integer(value["overlay_step_ms"], where + ".overlay_step_ms", 0, 1000)
        duration_keys = [key for key in ("overlay_ms", "duration", "overlay_duration") if key in value]
        if len(duration_keys) > 1:
            self.warn(where, "Multiple aliases for the same display duration; keep only one")
        for key in duration_keys:
            self.integer(value[key], where + "." + key, 0, 5000)
        if "particle" in value and value["particle"] != "":
            name = self.string(value["particle"], where + ".particle", 63, required=True)
            if name:
                self.particle_names.add(name)
        if "particles" in value:
            self.strings_array(value["particles"], where + ".particles", 8, 63, "particle")
        if "sound" in value and value["sound"] != "":
            self.sound(value["sound"], where + ".sound")
        if "sounds" in value:
            self.strings_array(value["sounds"], where + ".sounds", 8, 95, "sound")
        if "world" in value and not isinstance(value["world"], (bool, int)):
            self.error(where + ".world", "true/false or an integer required")
        if "overlay_sequence" in value and isinstance(value["overlay_sequence"], list):
            count = len(value["overlay_sequence"])
            step = value.get("overlay_step_ms", 40) or 40
            if count and isinstance(step, int) and not isinstance(step, bool) and count * max(16, step) > 5000:
                self.warn(where, "Sequence interval will be shortened by Skeeto to fit within 5 seconds")

    def style(self, path, dependency=False):
        try:
            data = json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=no_duplicate_keys)
        except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as exc:
            self.error(str(path), f"JSON parse failed: {exc}")
            return
        if not isinstance(data, dict):
            self.error(str(path), "Top level must be an object")
            return
        style_id = self.string(data.get("id"), str(path) + " id", 47, required=True)
        channel = self.string(data.get("channel"), str(path) + " channel", 7, required=True)
        if channel and channel not in CHANNELS:
            self.error(str(path), "channel must be ci/si/ff/fx")
        if style_id:
            if style_id in self.ids:
                message = f"Style ID duplicates {self.ids[style_id]}; later load will override"
                (self.warn if dependency else self.error)(str(path), message)
            else:
                self.ids[style_id] = path
            if not re.fullmatch(r"[A-Za-z0-9_-]+", style_id):
                self.warn(str(path), "Prefer ASCII letters, digits, _, or - for IDs")
        if "name" in data:
            self.string(data["name"], str(path) + " name", 63)
        if "sort" in data:
            self.integer(data["sort"], str(path) + " sort")
        streak = data.get("streak")
        if streak is not None:
            if not isinstance(streak, dict):
                self.error(str(path), "streak must be an object")
            else:
                for key in streak:
                    if key not in ("enabled", "wrap", "priority"):
                        self.warn(str(path) + " streak." + key, "Current Skeeto does not parse this field")
                if "enabled" in streak and not isinstance(streak["enabled"], (bool, int)):
                    self.error(str(path), "streak.enabled needs a boolean or integer")
                if "wrap" in streak:
                    self.integer(streak["wrap"], str(path) + " streak.wrap", 0, 10)
                if "priority" in streak:
                    self.integer(streak["priority"], str(path) + " streak.priority")
        named_count = 0
        for key, value in data.items():
            if key in TOP_FIELDS:
                continue
            if key in EVENT_KEYS or re.fullmatch(r"streak_(?:[1-9]|10)", key):
                self.event(value, str(path) + " " + key)
            elif key.startswith("streak_"):
                self.warn(str(path) + " " + key, "Skeeto only recognizes streak_1..streak_10 and streak_default")
            elif isinstance(value, dict):
                named_count += 1
                if channel != "fx":
                    self.warn(str(path) + " " + key, "Custom named events are usually only called from existing fx-channel triggers")
                if utf8_length(key) > 39:
                    self.error(str(path), "Named-event key exceeds 39 bytes")
                self.event(value, str(path) + " " + key)
            else:
                self.warn(str(path) + " " + key, "Unknown top-level field; current Skeeto does not parse it")
        if named_count > 24:
            self.error(str(path), "More than 24 named events; extras are not read")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("asset_root", type=Path, help="Directory containing skeeto/, materials/, and sound/")
    parser.add_argument("--dependency-root", action="append", type=Path, default=[],
                        help="Repeatable existing asset pack or game root for dependency lookup")
    parser.add_argument("--style-limit", type=int, choices=(64, 96, 128), default=64,
                        help="Check against Skeeto catalog capacity tier (default 64)")
    args = parser.parse_args()
    root = args.asset_root.resolve()
    if not root.is_dir():
        parser.error("asset_root does not exist or is not a directory")
    validator = Validator([root] + args.dependency_root)
    styles = sorted((root / "skeeto").glob("skeeto_*.json"))
    if not styles:
        validator.error(str(root), "No skeeto/skeeto_*.json found")
    if len(styles) > args.style_limit:
        validator.error(str(root), f"This pack has more styles than selected Skeeto capacity {args.style_limit}; use --style-limit 64, 96, or 128")
    for path in styles:
        validator.style(path)
    style_paths = {item.resolve() for item in styles}
    for dep in args.dependency_root:
        for path in sorted((dep / "skeeto").glob("skeeto_*.json")):
            if path.resolve() in style_paths:
                continue
            try:
                data = json.loads(path.read_text(encoding="utf-8-sig"))
            except (OSError, UnicodeError, json.JSONDecodeError):
                validator.warn(str(path), "Dependency style JSON unreadable; cannot check ID collisions")
                continue
            style_id = data.get("id") if isinstance(data, dict) else None
            if isinstance(style_id, str) and style_id:
                if style_id in validator.ids:
                    validator.warn(str(path), f"Style ID duplicates {validator.ids[style_id]}; later load will override")
                else:
                    validator.ids[style_id] = path
    if len(validator.ids) > args.style_limit:
        validator.warn(str(root), f"After merging dependencies, style count exceeds capacity {args.style_limit}; confirm the in-game capacity tier and that all styles loaded")
    if validator.particle_names:
        validator.warn(str(root), f"References {len(validator.particle_names)} particle effect name(s); {PARTICLE_WARNING}")
    for item in validator.errors:
        print("ERROR", item)
    for item in validator.warnings:
        print("WARN ", item)
    print(f"Checked {len(styles)} pack style(s): {len(validator.errors)} error(s), {len(validator.warnings)} warning(s)")
    return 1 if validator.errors else 0


if __name__ == "__main__":
    sys.exit(main())
