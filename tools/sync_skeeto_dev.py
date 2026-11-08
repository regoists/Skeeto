"""Maintain the development package without replacing edited documents."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PREVIEW = ROOT / "packages/Skeeto-dev"
SUPPORT = "Skeeto"
MANIFEST = Path(SUPPORT) / "Manifest.json"
DOCUMENT_SUFFIXES = {".md", ".txt", ".rst"}
GENERATED_TEXT = {"VERSION.txt", "SKEETO_WORKSHOP.txt"}
DIY_FILES = (
    "Skeeto/DIY/Background_主菜单背景.txt", "Skeeto/DIY/Menu_主菜单UI.txt",
    "Skeeto/DIY/ESCMenu_ESC菜单UI.txt",
    "Skeeto/DIY/Plugins_插件平台.txt",
    "Skeeto/DIY/Crosshair_贴图准星.txt", "Skeeto/DIY/HitFeedback_命中反馈.txt",
    "Skeeto/DIY/Agent Skills/README.txt", "Skeeto/DIY/Agent Skills/skeeto-hit-feedback/SKILL.md",
    "Skeeto/DIY/Agent Skills/skeeto-hit-feedback/scripts/validate_feedback.py",
    *("Skeeto/DIY/Examples 命中反馈json示例/" + name + ".json" for name in (
        "skeeto_ci_my_random_sfx", "skeeto_ci_my_sequence", "skeeto_ci_my_style",
        "skeeto_ff_my_style", "skeeto_fx_my_style", "skeeto_si_my_simple",
        "skeeto_si_my_streak", "skeeto_si_my_world")),
)
PLUGIN_SDK_FILES = (
    "Skeeto/PluginSDK/README.md",
    "Skeeto/PluginSDK/include/skeeto_plugin_api.h",
    "Skeeto/PluginSDK/examples/HelloMenu/plugin.cpp",
    "Skeeto/PluginSDK/examples/HelloMenu/build.bat",
)
PAYLOAD_FILES = (
    "skeeto.dll", "bin/wininet.dll", "VERSION.txt", "SKEETO_WORKSHOP.txt",
    "skeeto_apply_update.bat", "Setup_安装 维护 卸载.bat", "Skeeto/Maintain.ps1",
    "Skeeto/README.txt", "Skeeto/Commands.txt", "Skeeto_更新日志.txt", *DIY_FILES, *PLUGIN_SDK_FILES,
)


def file_role(relative: str) -> str:
    if relative == "skeeto.dll":
        return "core"
    if relative == "bin/wininet.dll":
        return "proxy"
    if relative in GENERATED_TEXT:
        return "marker"
    if relative.startswith(("Skeeto/DIY/", "Skeeto/PluginSDK/")) or Path(relative).suffix.lower() in DOCUMENT_SUFFIXES:
        return "document"
    return "tool"


def remove_retired_plugin_tree(root: Path, *, preview: bool = False) -> None:
    retired = root / "bin/neko/plugins/skeeto.dll"
    if retired.is_file():
        if preview:
            check_destination(retired)
        retired.unlink()
    for relative in ("bin/neko/plugins", "bin/neko"):
        path = root / relative
        if path.is_dir() and not any(path.iterdir()):
            if preview:
                check_destination(path)
            path.rmdir()


def validate_package_paths(folder: Path) -> None:
    metadata = json.loads((folder / MANIFEST).read_text(encoding="utf-8-sig"))
    names = [entry["relative"].replace("\\", "/") for entry in metadata["files"]]
    if len(names) != len(set(names)) or set(names) != set(PAYLOAD_FILES):
        raise ValueError("Package manifest does not match the fixed package layout")
    if (folder / "bin/neko").exists():
        raise ValueError("Package still contains bin/neko; plugin track was removed")
    actual = {p.relative_to(folder).as_posix() for p in folder.rglob("*") if p.is_file()}
    if actual != set(PAYLOAD_FILES) | {MANIFEST.as_posix()}:
        raise ValueError(f"Package has missing or unexpected files: {actual ^ (set(PAYLOAD_FILES) | {MANIFEST.as_posix()})}")
    for entry in metadata["files"]:
        path = folder / entry["relative"].replace("\\", "/")
        if not path.resolve().is_relative_to(folder.resolve()):
            raise ValueError(f"Package path escapes its directory: {path}")
        if entry["role"] != "document" and hashlib.sha256(path.read_bytes()).hexdigest() != entry["hash"]:
            raise ValueError(f"Package hash mismatch: {path}")
    # This ties the compiled updater's exact path to the manifest. A layout
    # rename without rebuilding the core must fail before deployment/release.
    core = (folder / "skeeto.dll").read_bytes()
    import pefile
    image = pefile.PE(data=core)
    names = {e.name for e in image.DIRECTORY_ENTRY_EXPORT.symbols}
    image.close()
    if b"GetL4NPluginInstance" in names:
        raise ValueError("Core still exports the removed L4N plugin factory")
    if b"SkeetoBootstrapQuery" not in names or b"necola_export" not in names:
        raise ValueError("Core lacks the Steam loading identity")
    for relative in ("Skeeto/Maintain.ps1",):
        if relative.replace("/", "\\").encode("utf-8") + b"\0" not in core:
            raise ValueError(f"Core was not built for this package path: {relative}")
    marker = (folder / "SKEETO_WORKSHOP.txt").read_text(encoding="ascii")
    engine_hash = hashlib.sha256((folder / "Skeeto/Maintain.ps1").read_bytes()).hexdigest()
    if (folder / "Skeeto/Maintain.ps1").stat().st_size > 1024 * 1024 or len(marker.splitlines()) > 32:
        raise ValueError("Update envelope exceeds protocol-1 bounds")
    if "update_protocol=1\n" not in marker or "engine_sha256=" + engine_hash + "\n" not in marker:
        raise ValueError("Update envelope does not match its engine")
    if metadata.get("product") != "Skeeto" or metadata.get("update_protocol") != 1:
        raise ValueError("Manifest does not match update protocol 1")
    if metadata.get("requires_manual_migrate") is not False:
        raise ValueError("Current Steam payload must support automatic upgrades; do not publish a manual-only update")
    for entry in metadata["files"]:
        if entry.get("role") != file_role(entry["relative"].replace("\\", "/")):
            raise ValueError("Manifest file role does not match package layout")
    setup = (folder / "Setup_安装 维护 卸载.bat").read_text(encoding="utf-8-sig")
    if '-File "%~dp0Skeeto\\Maintain.ps1"' not in setup:
        raise ValueError("Setup entry does not address the packaged maintenance engine")


def check_destination(target: Path) -> None:
    # The standalone sync reads a disk manifest. Reject paths or links that
    # would write outside the fixed preview directory.
    if not target.resolve().is_relative_to(PREVIEW):
        raise ValueError(f"Preview destination escapes its directory: {target}")


def copy_file(source: Path, target: Path, *, preserve: bool = False) -> None:
    check_destination(target)
    if preserve and target.exists():
        return
    target.parent.mkdir(parents=True, exist_ok=True)
    # Replacing a complete file also works when a reader maps the old file;
    # truncating wininet.dll in place failed with EINVAL during ordinary builds.
    temporary = target.with_name(target.name + ".skt-copy.tmp")
    created = False
    try:
        with temporary.open("xb") as output:
            created = True
            with source.open("rb") as input_file:
                shutil.copyfileobj(input_file, output)
        temporary.replace(target)
    finally:
        if created and temporary.exists():
            temporary.unlink()


def sync_dev_package(payload: Path) -> None:
    remove_retired_plugin_tree(PREVIEW, preview=True)
    metadata = json.loads((payload / MANIFEST).read_text(encoding="utf-8-sig"))
    for entry in metadata["files"]:
        relative = Path(entry["relative"].replace("\\", "/"))
        source, target = payload / relative, PREVIEW / relative
        if not source.resolve().is_relative_to(payload.resolve()):
            raise ValueError(f"Payload entry escapes its directory: {relative}")
        copy_file(source, target, preserve=relative.suffix.lower() in DOCUMENT_SUFFIXES
                  and relative.as_posix() not in GENERATED_TEXT)
    # Packaged references are merged into SKILL.md. Only the standalone
    # validator remains separate; optional agent UI metadata is not required.
    skill = ROOT / ".agents/skills/skeeto-hit-feedback"
    skill_target = PREVIEW / SUPPORT / "DIY/Agent Skills" / skill.name
    packaged_skill = skill_target / "SKILL.md"
    check_destination(packaged_skill)
    if not packaged_skill.exists():
        text = skill_text(skill)
        packaged_skill.parent.mkdir(parents=True, exist_ok=True)
        packaged_skill.write_text(text, encoding="utf-8")
    copy_file(skill / "scripts/validate_feedback.py", skill_target / "scripts/validate_feedback.py")

    # Documents register only their paths/roles; retained edits need no hashes.
    for entry in metadata["files"]:
        target = PREVIEW / entry["relative"].replace("\\", "/")
        if entry["role"] == "document":
            entry.pop("hash", None)
        else:
            entry["hash"] = hashlib.sha256(target.read_bytes()).hexdigest()
    manifest_target = PREVIEW / MANIFEST
    check_destination(manifest_target)
    # Truncating a mapped manifest in place returns EINVAL, same as wininet.dll.
    temporary = manifest_target.with_name(manifest_target.name + ".skt-copy.tmp")
    temporary.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(manifest_target)
    validate_package_paths(PREVIEW)
    print(f"Development package (not a release): {PREVIEW}")


def skill_text(skill: Path) -> str:
    text = (skill / "SKILL.md").read_text(encoding="utf-8-sig")
    for filename, anchor in (("asset-workflow.md", "asset-workflow"),
                             ("schema-and-layout.md", "schema-and-layout")):
        text = text.replace("references/" + filename, "#" + anchor)
        text += '\n\n<a id="' + anchor + '"></a>\n\n'
        text += (skill / "references" / filename).read_text(encoding="utf-8-sig")
    return text


if __name__ == "__main__":
    sync_dev_package(ROOT / "tools/local/bootstrap/migration-package")
