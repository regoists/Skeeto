"""Exercise the published 2.2.3 trigger/files against the current package.

Synthetic game roots only; no manifest resealing of the installed baseline.
"""
from pathlib import Path
import atexit
import hashlib
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "b89926d822f2839b723a78434aaf2ef6b665ba0a"
PAYLOAD_PREFIX = "packages/Skeeto-dev/"


def main():
    scratch = Path(tempfile.mkdtemp(prefix="skeeto-released-update-"))
    atexit.register(lambda: shutil.rmtree(scratch, ignore_errors=True))
    source = subprocess.check_output([
        "git", "show", BASELINE + ":src/skeeto/core/skeeto_workshop.cpp"
    ], cwd=ROOT).decode("utf-8-sig")
    baseline_version = next(line.split("=", 1)[1] for line in subprocess.check_output([
        "git", "show", BASELINE + ":VERSION.txt"
    ], cwd=ROOT).decode("utf-8-sig").splitlines() if line.startswith("semver="))

    def function(name):
        at = source.index(name)
        start = source.rfind("\n", 0, at) + 1
        end = source.index("\n}", start) + 2
        return source[start:end] + "\n"

    code = ('#include <Windows.h>\n#include <bcrypt.h>\n#include <cstring>\n'
            '#include <cstdio>\n#include <cwchar>\n#include <cctype>\n'
            'void Log(const char*,...){}\nstatic constexpr unsigned kSkeetoUpdateProtocol=1;\n'
            '#define SKEETO_VERSION "' + baseline_version + '"\n')
    code += next(line for line in source.splitlines()
                 if line.startswith("static const char kSkeetoInstalledSemver[]")) + "\n"
    for name in ("SkeetoUtf8ToWide(", "SkeetoRootPathW(", "SkeetoIsWorkshopPackageDir(",
                 "SkeetoReadUpdateField(", "SkeetoUpdateEngineHash(", "SkeetoInvokeUpdateEngine("):
        code += function(name)
    code += ('int main(int n,char**a){if(n!=3)return 99;'
             'return (int)SkeetoInvokeUpdateEngine(a[2],a[1],false);}\n')
    (scratch / "trigger.cpp").write_text(code, encoding="utf-8")
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    (scratch / "build.bat").write_text('@echo off\ncall "' + vs +
        '\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
        'cl /nologo /MT /O2 /EHsc /std:c++17 /utf-8 trigger.cpp bcrypt.lib /Fe:trigger.exe\n'
        'exit /b %errorlevel%\n', encoding="utf-8")
    subprocess.run(["cmd", "/c", "build.bat"], cwd=scratch, check=True)
    prefix = PAYLOAD_PREFIX
    names = subprocess.check_output(["git", "ls-tree", "-rz", "--name-only", BASELINE,
                                    "--", prefix], cwd=ROOT).decode("utf-8").split("\0")
    assert prefix + "skeeto.dll" in names and prefix + "bin/wininet.dll" in names
    for case in ("unchanged", "replaced-core", "replaced-proxy", "dll-only", "foreign-core", "foreign-proxy",
                 "foreign-dll-only-core", "foreign-dll-only-proxy"):
        game = scratch / case
        game.mkdir()
        for name in filter(None, names):
            assert name.startswith(prefix)
            if "dll-only" in case and name[len(prefix):] not in ("skeeto.dll", "bin/wininet.dll"):
                continue
            target = game / name[len(prefix):]
            assert target.resolve().is_relative_to(game.resolve())
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(["git", "show", BASELINE + ":" + name], cwd=ROOT))
        (game / "left4dead2").mkdir()
        (game / "left4dead2.exe").write_bytes(b"synthetic game")
        (game / "bin/engine.dll").write_bytes(b"synthetic engine")
        settings = game / "skeeto_setting.ini"
        settings.write_bytes(b"user settings")
        if case.startswith("replaced-"):
            path = game / ("skeeto.dll" if case.endswith("core") else "bin/wininet.dll")
            data = bytearray(path.read_bytes())
            data[0x20] ^= 1  # Reserved DOS-header byte; valid identity, different hash.
            path.write_bytes(data)
        if case.startswith("foreign-"):
            (game / ("skeeto.dll" if case.endswith("core") else "bin/wininet.dll")).write_bytes(b"foreign DLL")
        stage = game / "skeeto_update"
        shutil.copytree(ROOT / "packages/Skeeto-dev", stage)
        version = next(line.split("=", 1)[1] for line in (stage / "VERSION.txt").read_text().splitlines()
                       if line.startswith("semver="))
        assert tuple(map(int, version.split("."))) > tuple(map(int, baseline_version.split("."))), \
            "Test the real published baseline against a newer package"
        # The old DLL has already displayed the staged log before exit.
        (game / "skeeto_ws_changelog_shown.txt").write_text("semver=" + version + "\n")
        def snapshot():
            return {p.relative_to(game).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in game.rglob("*") if p.is_file() and
                    "skeeto_update" not in p.parts and "skeeto_migrate_backup" not in p.parts}
        before = snapshot()
        result = subprocess.run([str(scratch / "trigger.exe"), str(game), str(stage)], timeout=140)
        expected = 0
        assert result.returncode == expected, (case, result.returncode, expected)
        assert snapshot() == before, "Prepare changed installed files"
        assert (stage / "READY").exists() == (expected == 0)
        if expected == 0:
            subprocess.run(["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                str(stage / "Skeeto/Maintain.ps1"), "-Mode", "Apply", "-GameRoot", str(game),
                "-PayloadRoot", str(stage), "-UpdateProtocol", "1", "-NonInteractive"], check=True, timeout=45)
            for relative in ("skeeto.dll", "bin/wininet.dll"):
                assert (game / relative).read_bytes() == (stage / relative).read_bytes()
            assert settings.read_bytes() == b"user settings"
            assert not (stage / "READY").exists()
            assert not (game / "skeeto_migrate_backup/active.json").exists()
            assert not (game / "skeeto_ws_show_changelog.txt").exists(), "Apply queued an already shown log again"
        print(f"PASS published {baseline_version} trigger and original installation -> {version}:", case, flush=True)


if __name__ == "__main__":
    main()
