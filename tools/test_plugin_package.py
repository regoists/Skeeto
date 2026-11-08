"""Plugin SDK distribution contract; real package engine, no game launch."""
from pathlib import Path
import json
import re
import shutil
import subprocess
import tempfile
import sync_skeeto_dev as package

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_SDK = {
    'Skeeto/PluginSDK/README.md',
    'Skeeto/PluginSDK/include/skeeto_plugin_api.h',
    'Skeeto/PluginSDK/examples/HelloMenu/plugin.cpp',
    'Skeeto/PluginSDK/examples/HelloMenu/build.bat',
}
GUIDE = 'Skeeto/DIY/Plugins_插件平台.txt'


def main():
    assert set(getattr(package, 'PLUGIN_SDK_FILES', ())) == EXPECTED_SDK, 'Plugin SDK is absent from the package contract'
    assert EXPECTED_SDK | {GUIDE} <= set(package.PAYLOAD_FILES)
    for relative in EXPECTED_SDK | {GUIDE}:
        assert package.file_role(relative) == 'document', relative
    script = (ROOT/'tools/bootstrap/lifecycle/skeeto_lifecycle.ps1').read_text(encoding='utf-8-sig')
    entries = script.split('$script:PayloadFiles = @(', 1)[1].split('$script:Allowed +=', 1)[0]
    engine_paths = {s.replace('\\', '/') for s in re.findall(r"'([^']+)'", entries)}
    assert engine_paths == set(package.PAYLOAD_FILES), 'Engine/package file lists differ'
    preview = ROOT/'packages/Skeeto-dev'
    package.validate_package_paths(preview)
    manifest = json.loads((preview/package.MANIFEST).read_text(encoding='utf-8-sig'))
    by_path = {entry['relative'].replace('\\', '/'): entry for entry in manifest['files']}
    for relative in EXPECTED_SDK | {GUIDE}:
        assert by_path[relative] == {'relative': relative.replace('/', '\\'), 'role': 'document'}
        assert (preview/relative).is_file()
    assert not any(p.suffix.lower()=='.dll' for p in (preview/'Skeeto/PluginSDK').rglob('*'))
    assert not (preview/'bin/skeeto/plugins').exists(), 'Example DLLs must not autoload'
    for relative in EXPECTED_SDK:
        suffix = relative.removeprefix('Skeeto/PluginSDK/')
        if Path(suffix).suffix.lower() not in package.DOCUMENT_SUFFIXES:
            assert (preview/relative).read_bytes() == (ROOT/'src/skeeto/plugin_sdk'/suffix).read_bytes()
    with tempfile.TemporaryDirectory(prefix='skeeto-plugin-package-') as directory:
        scratch = Path(directory).resolve()
        copy = scratch/'payload'
        shutil.copytree(preview, copy)
        sdk = copy/'Skeeto/PluginSDK'
        # Build SDK from the package rather than the developer source tree.
        result = subprocess.run(['cmd', '/c', str(sdk/'examples/HelloMenu/build.bat')], capture_output=True)
        assert result.returncode == 0, (result.stdout+result.stderr).decode(errors='replace')
        assert (sdk/'examples/HelloMenu/out/HelloMenu.dll').is_file()
        # Missing/corrupted optional SDK material cannot reject a core update.
        (sdk/'include/skeeto_plugin_api.h').unlink()
        (sdk/'examples/HelloMenu/plugin.cpp').write_bytes(b'edited optional example')
        result = subprocess.run(['powershell.exe', '-NoLogo', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File', str(copy/'Skeeto/Maintain.ps1'), '-Mode', 'Validate', '-PayloadRoot', str(copy)], capture_output=True, timeout=30)
        assert result.returncode == 0, (result.stdout+result.stderr).decode(errors='replace')
        # Future ordinary builds retain edited prose, but refresh SDK code/ABI.
        saved_preview = package.PREVIEW
        package.PREVIEW = scratch/'sync-preview'
        try:
            package.sync_dev_package(preview)
            readme = package.PREVIEW/'Skeeto/PluginSDK/README.md'
            header = package.PREVIEW/'Skeeto/PluginSDK/include/skeeto_plugin_api.h'
            readme.write_bytes(b'user-edited SDK prose\r\n')
            header.write_bytes(b'outdated SDK header')
            package.sync_dev_package(preview)
            assert readme.read_bytes() == b'user-edited SDK prose\r\n'
            assert header.read_bytes() == (preview/'Skeeto/PluginSDK/include/skeeto_plugin_api.h').read_bytes()
        finally:
            package.PREVIEW = saved_preview
    print('PASS: complete SDK/tutorial, matching PS5.1 file list, optional document roles, no autoload DLLs, independent sample build, missing/edited SDK accepted by real engine, edited prose retained while SDK code refreshed')


if __name__ == '__main__':
    main()
