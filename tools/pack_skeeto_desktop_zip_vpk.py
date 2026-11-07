# -*- coding: utf-8 -*-
"""Pack desktop Skeeto_x.y.z folder -> Skeeto_x.y.z.zip + .vpk (same content, on Desktop)."""
from __future__ import annotations

import hashlib
import shutil
import sys
import zipfile
from pathlib import Path
import os
import re
import subprocess
from sync_skeeto_dev import validate_package_paths

DESKTOP = Path(os.environ.get("SKEETO_DESKTOP_OUT", str(Path.home() / "Desktop")))


def validate_folder(folder: Path, semver: str) -> None:
    """Read-only validation of a prepared Steam-layout package before compression."""
    if not re.fullmatch(r"\d+\.\d+\.\d+", semver):
        raise SystemExit(f"invalid version: {semver}")
    required = (
        "Setup_安装 维护 卸载.bat", "skeeto.dll", "VERSION.txt", "SKEETO_WORKSHOP.txt",
        "skeeto_apply_update.bat", "Skeeto_更新日志.txt", "bin/wininet.dll",
        "Skeeto/Manifest.json",
    )
    for name in required:
        if not (folder / name).is_file():
            raise SystemExit(f"missing Steam-layout file: {folder / name}")
    version = (folder / "VERSION.txt").read_text(encoding="utf-8-sig")
    if not re.search(rf"(?m)^semver={re.escape(semver)}\r?$", version):
        raise SystemExit(f"package version does not match {semver}")
    for name in ("necola.dll", "left4dead2_skeeto.exe", "skeeto.ini", "start_skeeto.bat",
                 "start_skeeto_with_update.bat", "start_necola_with_update.bat"):
        if (folder / name).exists():
            raise SystemExit(f"legacy startup component in Steam package: {name}")
    try:
        validate_package_paths(folder)
    except ValueError as error:
        raise SystemExit(str(error)) from error
    # Use the repository validator, not executable input from the package.
    validator = Path(__file__).resolve().parent / "bootstrap/lifecycle/skeeto_lifecycle.ps1"
    subprocess.run([
        "powershell.exe", "-NoLogo", "-NoProfile", "-NonInteractive",
        "-ExecutionPolicy", "Bypass", "-File", str(validator), "-Mode", "Validate",
        "-PayloadRoot", str(folder.resolve()),
    ], check=True)


def pack_version(semver: str) -> None:
    if not re.fullmatch(r"\d+\.\d+\.\d+", semver):
        raise SystemExit(f"invalid version: {semver}")
    folder = DESKTOP / f"Skeeto_{semver}"
    if not folder.is_dir():
        raise SystemExit(f"missing folder {folder}")
    validate_folder(folder, semver)
    out_zip = DESKTOP / f"Skeeto_{semver}.zip"
    out_vpk = DESKTOP / f"Skeeto_{semver}.vpk"
    if out_zip.exists():
        out_zip.unlink()
    if out_vpk.exists():
        out_vpk.unlink()
    with zipfile.ZipFile(out_zip, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for p in folder.rglob("*"):
            if p.is_file():
                zf.write(p, p.relative_to(folder).as_posix())
    shutil.copy2(out_zip, out_vpk)
    with zipfile.ZipFile(out_vpk) as z:
        zm = {n: hashlib.sha1(z.read(n)).hexdigest() for n in z.namelist() if not n.endswith("/")}
    dm = {
        p.relative_to(folder).as_posix(): hashlib.sha1(p.read_bytes()).hexdigest()
        for p in folder.rglob("*")
        if p.is_file()
    }
    if zm != dm:
        raise SystemExit(f"content mismatch for {semver}")
    print("OK", out_zip.name, out_vpk.name, "mb", round(out_vpk.stat().st_size / 1024 / 1024, 2))


if __name__ == "__main__":
    vers = sys.argv[1:]
    if not vers:
        raise SystemExit("usage: pack_skeeto_desktop_zip_vpk.py X.Y.Z [X.Y.Z ...]; prepare and review the package folder first")
    for v in vers:
        pack_version(v)
