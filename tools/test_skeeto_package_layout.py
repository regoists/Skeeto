"""Offline regression checks for the current desktop compressor; no desktop writes."""
from __future__ import annotations

import atexit
import hashlib
import importlib.util
import os
from pathlib import Path
import tempfile
import re
import shutil
import subprocess
import sys
import zipfile
import json
import time


def refresh_manifest(folder: Path, version: str | None = None) -> None:
    path = folder / "Skeeto/Manifest.json"
    metadata = json.loads(path.read_text(encoding="utf-8-sig"))
    if version:
        text = (folder / "VERSION.txt").read_text(encoding="utf-8-sig")
        (folder / "VERSION.txt").write_text(re.sub(r"(?m)^semver=.*$", "semver=" + version, text), encoding="utf-8")
        metadata["semver"] = version
    marker = folder / "SKEETO_WORKSHOP.txt"
    marker_text = marker.read_text(encoding="ascii")
    engine_hash = hashlib.sha256((folder / "Skeeto/Maintain.ps1").read_bytes()).hexdigest()
    marker.write_text(re.sub(r"(?m)^engine_sha256=.*$", "engine_sha256=" + engine_hash, marker_text), encoding="ascii")
    for entry in metadata["files"]:
        if entry["role"] == "document":
            entry.pop("hash", None)
        else:
            entry["hash"] = hashlib.sha256((folder / entry["relative"].replace("\\", "/")).read_bytes()).hexdigest()
    path.write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8")


def check_transactions(root: Path, scratch: Path, source: Path, current_version: str, next_version: str) -> None:
    # Only synthetic roots; never target the actual game or end a process.
    def fixture(name):
        target = scratch / name
        shutil.copytree(source, target)
        (target / "left4dead2").mkdir()
        (target / "left4dead2.exe").write_bytes(b"offline fixture")
        (target / "bin/engine.dll").write_bytes(b"offline fixture")
        (target / "skeeto_setting.ini").write_bytes(b"user-owned settings")
        refresh_manifest(target, current_version)
        return target

    def invoke(target, mode, *extra, good=True):
        engine = target / ("skeeto_update/Skeeto/Maintain.ps1" if mode in {"Apply", "Prepare", "Uninstall"} else "Skeeto/Maintain.ps1")
        result = subprocess.run(["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass",
                                 "-File", str(engine), "-Mode", mode, "-GameRoot", str(target),
                                 "-NonInteractive", *extra], capture_output=True)
        assert (result.returncode == 0) == good, repr(result.stdout + result.stderr)

    installed = fixture("layout2-installed")
    original = (installed / "skeeto.dll").read_bytes()
    stage = installed / "skeeto_update"
    shutil.copytree(source, stage)
    # Distinct valid PE bytes prove both loaded artifacts are replaced after exit.
    for relative in ("skeeto.dll", "bin/wininet.dll"):
        binary = bytearray((stage / relative).read_bytes())
        binary[0x20] ^= 1  # Reserved DOS header byte, never executable code.
        (stage / relative).write_bytes(binary)
    refresh_manifest(stage, next_version)
    invoke(installed, "Prepare", "-PayloadRoot", str(stage), "-UpdateProtocol", "1")
    invoke(installed, "Apply")
    assert not (stage / "READY").exists()
    assert (installed / "skeeto.dll").read_bytes() == (stage / "skeeto.dll").read_bytes() != original
    assert (installed / "bin/wininet.dll").read_bytes() == (stage / "bin/wininet.dll").read_bytes()
    assert not (installed / "bin/neko/plugins/skeeto.dll").exists()
    assert not (installed / "Skeeto/Manifest.json").exists()
    assert not (installed / "VERSION.txt").exists()
    assert not (installed / "SKEETO_WORKSHOP.txt").exists()
    assert not (installed / "skeeto_apply_update.bat").exists()
    assert (installed / "Skeeto_更新日志.txt").read_bytes() == (stage / "Skeeto_更新日志.txt").read_bytes()
    assert (stage / "VERSION.txt").is_file() and (stage / "Skeeto/Maintain.ps1").is_file()
    assert (installed / "skeeto_setting.ini").read_bytes() == b"user-owned settings"
    assert not list((installed / "skeeto_migrate_backup").glob("*/transaction.json"))
    assert not list((installed / "skeeto_migrate_backup").glob("*/original-*.bin"))
    neighbor = installed / "bin/neko/plugins/other-plugin.dll"
    neighbor.parent.mkdir(parents=True, exist_ok=True)
    neighbor.write_bytes(b"foreign plugin")
    invoke(installed, "Uninstall")
    assert not (installed / "skeeto.dll").exists() and not (installed / "bin/wininet.dll").exists()
    assert not (installed / "bin/neko/plugins/skeeto.dll").exists()
    assert neighbor.read_bytes() == b"foreign plugin"
    assert not (installed / "Setup_安装 维护 卸载.bat").exists() and not (installed / "Skeeto/Maintain.ps1").exists()
    assert not (installed / "Skeeto_更新日志.txt").exists()
    assert (installed / "skeeto_setting.ini").read_bytes() == b"user-owned settings"
    print("PASS: layout2 installed core updates to the staged package and uninstalls; no completed backups; settings preserved.")

    edited = fixture("edited-document")
    stage = edited / "skeeto_update"
    shutil.copytree(source, stage)
    refresh_manifest(stage, next_version)
    invoke(edited, "Prepare", "-PayloadRoot", str(stage), "-UpdateProtocol", "1")
    tutorial = edited / "Skeeto/DIY/HitFeedback_命中反馈.txt"
    tutorial.write_bytes(b"user-authored tutorial")
    invoke(edited, "Apply")
    assert not tutorial.exists()
    assert (edited / "Skeeto_更新日志.txt").read_bytes() == (stage / "Skeeto_更新日志.txt").read_bytes()
    assert (stage / tutorial.relative_to(edited)).is_file()
    assert not (stage / "READY").exists()
    backups = list((edited / "skeeto_migrate_backup").glob("*/original-*.bin"))
    assert not any(p.read_bytes() == b"user-authored tutorial" for p in backups)
    print("PASS: edited DIY document is refreshed without backups or blocking the update.")

    changed = fixture("changed-after-prepare")
    stage = changed / "skeeto_update"
    shutil.copytree(source, stage)
    refresh_manifest(stage, next_version)
    invoke(changed, "Prepare", "-PayloadRoot", str(stage), "-UpdateProtocol", "1")
    path = stage / "Skeeto/Manifest.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    data["note"] = "changed after preparation"
    path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
    invoke(changed, "Apply", good=False)
    assert (stage / "READY").exists() and (changed / "skeeto.dll").read_bytes() == original
    print("PASS: valid package metadata changed after Prepare cannot consume an older READY receipt.")


def check_update_protocol(root: Path, scratch: Path, current_version: str, next_version: str) -> None:
    r=root; scratch=scratch/'protocol-native'; scratch.mkdir()
    preview=r/'packages/Skeeto-dev';source=(r/'src/skeeto/core/skeeto_workshop.cpp').read_text(encoding='utf-8-sig')
    def function(name):
     at=source.index(name);start=source.rfind('\n',0,at)+1;end=source.index('\n}',start)+2
     return source[start:end]+'\n'
    code='#include <Windows.h>\n#include <bcrypt.h>\n#include <cstring>\n#include <cstdio>\n#include <cwchar>\n#include <cctype>\nvoid Log(const char*,...){}\nstatic constexpr unsigned kSkeetoUpdateProtocol=1;\nstatic const char kSkeetoInstalledSemver[]="SkeetoInstalledSemver=0.0.0";\n'
    for name in ['SkeetoUtf8ToWide(', 'SkeetoRootPathW(', 'SkeetoIsWorkshopPackageDir(', 'SkeetoReadUpdateField(', 'SkeetoUpdateEngineHash(', 'SkeetoInvokeUpdateEngine(']:code+=function(name)
    code+='int main(int n,char**a){if(n!=4)return 99;bool apply=!strcmp(a[3],"apply");DWORD c=SkeetoInvokeUpdateEngine(a[2],a[1],apply);if(apply && !c)Sleep(4000);return static_cast<int>(c);}\n'
    (scratch/'trigger.cpp').write_text(code,encoding='utf-8')
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe';vs=Path(subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip())
    (scratch/'build.bat').write_text('@echo off\nchcp 65001 >nul\ncall "'+str(vs/'VC/Auxiliary/Build/vcvarsall.bat')+'" x86 >nul\ncl /nologo /MT /O2 /EHsc /std:c++17 /utf-8 /Fe:trigger.exe trigger.cpp bcrypt.lib\n',encoding='utf-8')
    subprocess.run(['cmd','/d','/c',str(scratch/'build.bat')],cwd=scratch,check=True)
    actor=scratch/'trigger.exe'
    def fixture(name):
     root=scratch/name;shutil.copytree(preview,root);(root/'left4dead2').mkdir();(root/'left4dead2.exe').write_bytes(b'fixture');(root/'bin/engine.dll').write_bytes(b'fixture');(root/'skeeto_setting.ini').write_bytes(b'user settings');refresh_manifest(root,current_version);return root
    def hashes(root):return {p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in root.rglob('*') if p.is_file() and 'skeeto_update' not in p.parts and 'skeeto_migrate_backup' not in p.parts}
    def make_stage(root,version=next_version):
     stage=root/'skeeto_update';shutil.copytree(preview,stage);refresh_manifest(stage,version);return stage
    def prepare(root,stage,expected):
     before=hashes(root);q=subprocess.run([str(actor),str(root),str(stage),'prepare']);assert q.returncode==expected,(q.returncode,expected);assert hashes(root)==before,'Prepare mutated installed files'
    root=fixture('future-layout');stage=make_stage(root)
    # One frozen OLD trigger binary is used against a NEW package engine and layout.
    engine=stage/'Skeeto/Maintain.ps1';text=engine.read_text(encoding='utf-8-sig');text=text.replace('$script:PackageLayout = 2','$script:PackageLayout = 3').replace("'Skeeto\\README.txt'","'SkeetoNext\\Manual.txt'");engine.write_bytes(text.replace('\r\n','\n').replace('\n','\r\n').encode('utf-8-sig'))
    (stage/'SkeetoNext').mkdir();shutil.move(str(stage/'Skeeto/README.txt'),str(stage/'SkeetoNext/Manual.txt'))
    m=stage/'Skeeto/Manifest.json';meta=json.loads(m.read_text(encoding='utf-8'));meta['install_layout']=3
    next(e for e in meta['files'] if e['relative']=='Skeeto\\README.txt')['relative']='SkeetoNext\\Manual.txt';m.write_text(json.dumps(meta,ensure_ascii=False),encoding='utf-8');marker=stage/'SKEETO_WORKSHOP.txt';marker.write_text(marker.read_text(encoding='ascii').replace('install_layout=2','install_layout=3'),encoding='ascii');refresh_manifest(stage)
    prepare(root,stage,0);assert (stage/'READY').exists()
    before=hashes(root);child=subprocess.Popen([str(actor),str(root),str(stage),'apply']);time.sleep(2);assert child.poll() is None and hashes(root)==before,'Applied before accepted process exit';assert child.wait(timeout=20)==0
    deadline=time.monotonic()+20
    while ((stage/'READY').exists() or (root/'Skeeto/Manifest.json').exists()) and time.monotonic()<deadline:time.sleep(.2)
    assert not (stage/'READY').exists()
    assert not (root/'SkeetoNext/Manual.txt').exists()
    assert (root/'Skeeto/README.txt').is_file()
    assert (root/'skeeto.dll').read_bytes()==(stage/'skeeto.dll').read_bytes()
    assert not (root/'Skeeto/Manifest.json').exists()
    print('PASS: frozen old x86 trigger prepares a changed package with the new engine, waits for the exact process, and applies only the runtime DLLs.')
    root=fixture('manual');stage=make_stage(root);m=stage/'Skeeto/Manifest.json';meta=json.loads(m.read_text(encoding='utf-8'));meta['requires_manual_migrate']=True;m.write_text(json.dumps(meta,ensure_ascii=False),encoding='utf-8');prepare(root,stage,2);assert not (stage/'READY').exists();print('PASS: manual migration returns 2 without READY or installed changes.')
    root=fixture('bad-envelope');stage=make_stage(root);engine=stage/'Skeeto/Maintain.ps1';engine.write_bytes(engine.read_bytes()+b'\r\n# Changed after download');prepare(root,stage,1);assert not (stage/'READY').exists();print('PASS: changed engine is rejected before execution.')
    root=fixture('bad-payload');stage=make_stage(root);(stage/'skeeto.dll').write_bytes(b'broken core');prepare(root,stage,1);assert not (stage/'READY').exists();print('PASS: new engine rejects bad payload without installed changes.')
    root=fixture('new-protocol');stage=make_stage(root);marker=stage/'SKEETO_WORKSHOP.txt';marker.write_text(marker.read_text(encoding='ascii').replace('update_protocol=1','update_protocol=2'),encoding='ascii');prepare(root,stage,2);assert not (stage/'READY').exists();print('PASS: incompatible protocol returns manual migration.')


def check_legacy_bridge(root: Path, scratch: Path, source: Path, current_version: str, next_version: str) -> None:
    for alias in (False, True):
        target = scratch / ("legacy-necola" if alias else "legacy-standalone")
        (target / "bin").mkdir(parents=True)
        (target / "left4dead2").mkdir()
        for relative in ("left4dead2.exe", "bin/engine.dll", "left4dead2_skeeto.exe"):
            (target / relative).write_bytes(b"legacy fixture")
        shutil.copyfile(source / "skeeto.dll", target / "skeeto.dll")
        installed = bytearray((target / "skeeto.dll").read_bytes())
        match = re.search(br"SkeetoInstalledSemver=\d+\.\d+\.\d+", installed)
        assert match, "installed core missing SkeetoInstalledSemver marker"
        field = match.end() - match.start()
        replacement = b"SkeetoInstalledSemver=1.7.4"
        assert len(replacement) <= field, (replacement, field)
        if len(replacement) < field:
            replacement = replacement + (b"\0" * (field - len(replacement)))
        installed[match.start():match.end()] = replacement
        (target / "skeeto.dll").write_bytes(installed)
        (target / "VERSION.txt").write_text("Skeeto 1.7.4\nsemver=1.7.4\n", encoding="ascii")
        if alias:
            shutil.copyfile(target / "skeeto.dll", target / "necola.dll")
        stage = target / "skeeto_update"
        shutil.copytree(source, stage)
        refresh_manifest(stage, current_version)
        (stage / "READY").write_text("1\n", encoding="ascii")
        engine = stage / "Skeeto/Maintain.ps1"
        command = ["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass",
                   "-File", str(engine), "-GameRoot", str(target), "-PayloadRoot", str(stage), "-NonInteractive"]
        result = subprocess.run(command + ["-Mode", "Apply"], capture_output=True)
        assert result.returncode == 0, repr(result.stdout + result.stderr)
        assert not (target / "bin/wininet.dll").exists() and (target / "left4dead2_skeeto.exe").exists()
        assert not (target / "Skeeto/Manifest.json").exists()
        refresh_manifest(stage, next_version)
        result = subprocess.run(command + ["-Mode", "Prepare", "-UpdateProtocol", "1"], capture_output=True)
        assert result.returncode == 2 and not (stage / "READY").exists()
        result = subprocess.run(command + ["-Mode", "Migrate"], capture_output=True)
        assert result.returncode == 0, repr(result.stdout + result.stderr)
        assert (target / "bin/wininet.dll").exists()
        assert not (target / "left4dead2_skeeto.exe").exists()
        assert not (target / "Skeeto/Manifest.json").exists()
        assert not (target / "VERSION.txt").exists()
    print("PASS: legacy bridge does not install the proxy or a game-root manifest; automatic Prepare requests manual migration; explicit Migrate installs only the runtime DLLs.")


def check_root_clutter(repo: Path, scratch: Path) -> None:
    engine = repo / "tools/bootstrap/lifecycle/skeeto_lifecycle.ps1"

    def game(name: str) -> Path:
        target = scratch / name
        (target / "bin").mkdir(parents=True)
        (target / "left4dead2").mkdir()
        (target / "left4dead2.exe").write_bytes(b"offline fixture")
        (target / "bin/engine.dll").write_bytes(b"offline fixture")
        return target

    def clean(target: Path) -> None:
        result = subprocess.run(
            ["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass",
             "-File", str(engine), "-Mode", "Clean", "-GameRoot", str(target), "-NonInteractive"],
            capture_output=True)
        assert result.returncode == 0, repr(result.stdout + result.stderr)

    root = game("clutter")
    (root / "skeeto.dll").write_bytes(b"core")
    (root / "bin/wininet.dll").write_bytes(b"proxy")
    (root / "skeeto_setting.ini").write_bytes(b"user-owned settings")
    (root / "necola.dll").write_bytes(b"real-necola")
    (root / "kpatch.ini").write_text("cmdline=-novid\n", encoding="ascii")
    (root / "skeeto.ini").write_text("cmdline=-old\n", encoding="ascii")
    (root / "VERSION.txt").write_text("Skeeto 2.0.1\nsemver=2.0.1\n", encoding="ascii")
    (root / "SKEETO_WORKSHOP.txt").write_text("SkeetoWorkshopPackage=1\n", encoding="ascii")
    (root / "skeeto_install.txt").write_text("Skeeto installation\n", encoding="ascii")
    (root / "skeeto_setting.ini.before-key-cleanup.bak").write_bytes(b"old-settings")
    (root / "skeeto_apply_update.bat").write_text("@echo off\n", encoding="ascii")
    (root / "Skeeto_更新日志.txt").write_text("log\n", encoding="utf-8")
    (root / "Skeeto").mkdir()
    (root / "Skeeto/README.txt").write_text("readme\n", encoding="ascii")
    (root / "Skeeto/user-note.txt").write_text("keep\n", encoding="ascii")
    (root / "skeeto_migrate_backup/current").mkdir(parents=True)
    (root / "skeeto_migrate_backup/current/transaction.json").write_text("{}\n", encoding="ascii")
    (root / "skeeto_ws_changelog_shown.txt").write_text("semver=2.0.1\n", encoding="ascii")
    (root / "Skeeto_使用说明.txt").write_text("old readme\n", encoding="utf-8")
    (root / "skeeto_update").mkdir()
    (root / "skeeto_update/VERSION.txt").write_text("Skeeto 9\n", encoding="ascii")
    (root / "skeeto_update/Skeeto").mkdir()
    (root / "skeeto_update/Skeeto/Manifest.json").write_text("{}\n", encoding="ascii")
    (root / "left4dead2/skeeto").mkdir()
    (root / "left4dead2/skeeto/custom.txt").write_text("asset\n", encoding="ascii")
    (root / "keep-me.txt").write_text("foreign\n", encoding="ascii")
    clean(root)
    assert (root / "skeeto.dll").read_bytes() == b"core"
    assert (root / "bin/wininet.dll").read_bytes() == b"proxy"
    assert (root / "skeeto_setting.ini").read_bytes() == b"user-owned settings"
    assert (root / "necola.dll").read_bytes() == b"real-necola"
    assert (root / "kpatch.ini").read_text(encoding="ascii") == "cmdline=-novid\n"
    assert (root / "left4dead2/skeeto/custom.txt").read_text(encoding="ascii") == "asset\n"
    assert (root / "keep-me.txt").read_text(encoding="ascii") == "foreign\n"
    assert (root / "Skeeto/user-note.txt").read_text(encoding="ascii") == "keep\n"
    assert (root / "Skeeto_更新日志.txt").read_text(encoding="utf-8") == "log\n"
    for gone in (
        "VERSION.txt", "SKEETO_WORKSHOP.txt", "skeeto_install.txt", "skeeto.ini",
        "skeeto_setting.ini.before-key-cleanup.bak", "skeeto_apply_update.bat",
        "skeeto_ws_changelog_shown.txt", "Skeeto_使用说明.txt",
        "Skeeto/README.txt", "skeeto_migrate_backup", "skeeto_update"):
        assert not (root / gone).exists(), gone

    foreign = game("foreign-version")
    (foreign / "VERSION.txt").write_text("OtherMod 1\n", encoding="ascii")
    clean(foreign)
    assert (foreign / "VERSION.txt").read_text(encoding="ascii") == "OtherMod 1\n"

    pending = game("pending-stage")
    (pending / "skeeto_update").mkdir()
    (pending / "skeeto_update/READY").write_text("{}\n", encoding="ascii")
    (pending / "skeeto_update/VERSION.txt").write_text("Skeeto 9\n", encoding="ascii")
    clean(pending)
    assert (pending / "skeeto_update/READY").is_file()
    assert (pending / "skeeto_update/VERSION.txt").is_file()

    active = game("active-recovery")
    (active / "skeeto_migrate_backup").mkdir()
    (active / "skeeto_migrate_backup/active.json").write_text("{", encoding="ascii")
    clean(active)
    assert (active / "skeeto_migrate_backup/active.json").is_file()
    print("PASS: game-root cleanup removes confirmed leftovers and keeps runtime, settings, assets and other platforms.")


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    source = root / "packages/Skeeto-dev"
    version = re.search(r"(?m)^semver=(\d+\.\d+\.\d+)\r?$",
                        (source / "VERSION.txt").read_text(encoding="utf-8-sig")).group(1)
    major, minor, patch = (int(part) for part in version.split("."))
    next_version = f"{major}.{minor}.{patch + 1}"
    scratch = Path(tempfile.mkdtemp(prefix="skeeto-package-layout-"))
    atexit.register(lambda: shutil.rmtree(scratch, ignore_errors=True))
    check_root_clutter(root, scratch)
    spec = importlib.util.spec_from_file_location("compressor", root / "tools/pack_skeeto_desktop_zip_vpk.py")
    compressor = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(compressor)
    compressor.DESKTOP = scratch
    folder = scratch / f"Skeeto_{version}"
    shutil.copytree(source, folder)
    refresh_manifest(folder)
    document_manifest = (folder / "Skeeto/Manifest.json").read_bytes()
    metadata = json.loads(document_manifest)
    assert all(set(e) == {"relative", "role"} for e in metadata["files"] if e["role"] == "document")
    user_doc = folder / "Skeeto/DIY/Agent Skills/skeeto-hit-feedback/SKILL.md"
    user_doc.parent.mkdir(parents=True, exist_ok=True)
    user_doc.write_bytes(b"User-authored package documentation: preserve exactly.\r\n")
    assert (folder / "Skeeto/Manifest.json").read_bytes() == document_manifest
    original = {p.relative_to(folder).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in folder.rglob("*") if p.is_file()}
    compressor.pack_version(version)
    after = {p.relative_to(folder).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
             for p in folder.rglob("*") if p.is_file()}
    assert original == after, "Compressor modified prepared files"
    archive = scratch / f"Skeeto_{version}.zip"
    assert archive.read_bytes() == (scratch / f"Skeeto_{version}.vpk").read_bytes()
    with zipfile.ZipFile(archive) as packed:
        assert "skeeto.dll" in packed.namelist() and "bin/wininet.dll" in packed.namelist()
        assert "SKEETO_WORKSHOP.txt" in packed.namelist()
        assert {n: hashlib.sha256(packed.read(n)).hexdigest() for n in packed.namelist()} == original
    print("PASS: document entries contain only path/role; edited package documents compress without regenerating manifest or changing user contents.")

    def reject(label: str, action) -> None:
        try:
            action()
        except (SystemExit, subprocess.CalledProcessError):
            print("PASS: rejects " + label)
        else:
            raise AssertionError("Accepted " + label)

    reject("version/path input", lambda: compressor.pack_version("../1.4.11"))
    reject("version mismatch", lambda: compressor.validate_folder(folder, "99.99.99"))
    legacy = scratch / "legacy"
    legacy.mkdir()
    (legacy / "skeeto.dll").write_bytes(b"old layout fixture")
    reject("old layout", lambda: compressor.validate_folder(legacy, version))
    (folder / "necola.dll").write_bytes(b"legacy alias fixture")
    reject("legacy alias mixed into current package", lambda: compressor.validate_folder(folder, version))
    shutil.move(str(folder / "necola.dll"), str(scratch / "retired-alias-fixture.dll"))
    unexpected = folder / "Skeeto/debug.log"
    unexpected.write_text("not a package file")
    reject("unlisted package file", lambda: compressor.validate_folder(folder, version))
    shutil.move(str(unexpected), str(scratch / "retired-debug.log"))
    core = folder / "skeeto.dll"
    compiled = core.read_bytes()
    core.write_bytes(compiled.replace(b"Skeeto\\Maintain.ps1", b"Skeeto\\Obsolete.ps1"))
    refresh_manifest(folder)
    reject("core compiled for another layout", lambda: compressor.validate_folder(folder, version))
    core.write_bytes(compiled)
    refresh_manifest(folder)
    metadata_path = folder / "Skeeto/Manifest.json"
    metadata = json.loads(metadata_path.read_text(encoding="utf-8-sig"))
    metadata["requires_manual_migrate"] = True
    metadata_path.write_text(json.dumps(metadata, ensure_ascii=False), encoding="utf-8")
    reject("current release payload disabling automatic upgrades", lambda: compressor.validate_folder(folder, version))
    metadata["requires_manual_migrate"] = False
    metadata_path.write_text(json.dumps(metadata, ensure_ascii=False), encoding="utf-8")
    refresh_manifest(folder)
    setup = folder / "Setup_安装 维护 卸载.bat"
    setup_bytes = setup.read_bytes()
    setup.write_bytes(setup_bytes.replace(b"%~dp0Skeeto\\Maintain.ps1", b"%~dp0Maintain.ps1"))
    refresh_manifest(folder)
    reject("setup points to an absent sibling engine", lambda: compressor.validate_folder(folder, version))
    setup.write_bytes(setup_bytes)
    refresh_manifest(folder)
    (folder / "skeeto_apply_update.bat").write_bytes(b"corrupt updater fixture")
    reject("manifest/content mismatch", lambda: compressor.validate_folder(folder, version))
    no_args = subprocess.run([sys.executable, root / "tools/pack_skeeto_desktop_zip_vpk.py"],
                             env={**os.environ, "SKEETO_DESKTOP_OUT": str(scratch)}, capture_output=True)
    assert no_args.returncode != 0 and b"usage:" in no_args.stderr
    print("PASS: no implicit old version when arguments are absent.")
    check_transactions(root, scratch, source, version, next_version)
    check_update_protocol(root, scratch, version, next_version)
    check_legacy_bridge(root, scratch, source, version, next_version)


if __name__ == "__main__":
    main()
