"""Build the minimal Steam payload and refresh the complete user package preview."""
import hashlib
import json
import shutil
from pathlib import Path
import pefile
from sync_skeeto_dev import (
    sync_dev_package, DIY_FILES, PLUGIN_SDK_FILES, PAYLOAD_FILES, validate_package_paths, file_role,
    remove_retired_plugin_tree,
)

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tools/local/bootstrap/migration-package"
SOURCE = ROOT / "tools/bootstrap/lifecycle"
PREVIEW = ROOT / "packages/Skeeto-dev"
SUPPORT = "Skeeto"
DOCUMENTS = (SUPPORT + "/README.txt", SUPPORT + "/Commands.txt", "Skeeto_更新日志.txt")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / SUPPORT).mkdir(exist_ok=True)
    (OUT / "bin").mkdir(exist_ok=True)
    for source, relative in [(ROOT / "runtime/dll/skeeto.dll", "skeeto.dll"),
                             (ROOT / "runtime/bootstrap/wininet.dll", "bin/wininet.dll")]:
        image = pefile.PE(str(source))
        if image.FILE_HEADER.Machine != 0x14C:
            raise RuntimeError(f"Not x86: {source}")
        image.close()
        (OUT / relative).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, OUT / relative)
    remove_retired_plugin_tree(OUT)
    for name in DOCUMENTS:
        # Edited preview originals are authoritative; ordinary builds never
        # regenerate the user's guide, command reference or release history.
        shutil.copyfile(PREVIEW / name, OUT / name)
    for name in DIY_FILES:
        target = OUT / name
        target.parent.mkdir(parents=True, exist_ok=True)
        source = (ROOT / ".agents/skills/skeeto-hit-feedback/scripts/validate_feedback.py"
                  if name.endswith("/scripts/validate_feedback.py") else PREVIEW / name)
        shutil.copyfile(source, target)
    for name in PLUGIN_SDK_FILES:
        relative = Path(name).relative_to("Skeeto/PluginSDK")
        target = OUT / name
        target.parent.mkdir(parents=True, exist_ok=True)
        # Keep edited preview prose; SDK code/ABI files follow their source.
        preview_document = PREVIEW / name
        source = (preview_document if relative.suffix.lower() in {".md", ".txt"} and preview_document.exists()
                  else ROOT / "src/skeeto/plugin_sdk" / relative)
        shutil.copyfile(source, target)
    shutil.copyfile(ROOT / "VERSION.txt", OUT / "VERSION.txt")
    engine = (SOURCE / "skeeto_lifecycle.ps1").read_text(encoding="utf-8-sig")
    import_line = "    . (Join-Path $PSScriptRoot 'skeeto_pe.ps1')"
    assert engine.count(import_line) == 1
    engine = engine.replace(import_line, (SOURCE / "skeeto_pe.ps1").read_text(encoding="utf-8-sig"))
    (OUT / SUPPORT / "Maintain.ps1").write_bytes(
        engine.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8-sig"))
    for name in ("Setup_安装 维护 卸载.bat", "skeeto_apply_update.bat"):
        text = (SOURCE / name).read_text(encoding="utf-8-sig")
        (OUT / name).write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8"))
    engine_hash = hashlib.sha256((OUT / SUPPORT / "Maintain.ps1").read_bytes()).hexdigest()
    (OUT / "SKEETO_WORKSHOP.txt").write_text(
        "SkeetoWorkshopPackage=1\nformat=zip-as-vpk\nworkshop_id=3787239937\ninstall_layout=2\n"
        "update_protocol=1\nengine_sha256=" + engine_hash + "\n", encoding="ascii")
    semver = next(line.split("=", 1)[1].strip() for line in
                  (OUT / "VERSION.txt").read_text(encoding="utf-8-sig").splitlines() if line.startswith("semver="))
    names = PAYLOAD_FILES
    names = [name.replace("/", "\\") for name in names]
    metadata = {"schema": 1, "product": "Skeeto", "update_protocol": 1,
                "requires_manual_migrate": False, "install_layout": 2, "semver": semver,
                "files": []}
    for name in names:
        entry = {"relative": name, "role": file_role(name.replace("\\", "/"))}
        if entry["role"] != "document":
            entry["hash"] = hashlib.sha256((OUT / name.replace("\\", "/")).read_bytes()).hexdigest()
        metadata["files"].append(entry)
    (OUT / SUPPORT / "Manifest.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    validate_package_paths(OUT)
    sync_dev_package(OUT)
    print(f"Development payload: {OUT}")


if __name__ == "__main__":
    main()
