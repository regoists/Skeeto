"""Guard the optional host's integration and keep callbacks outside render passes."""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
core=ROOT/"src/skeeto/core"
p=(core/"skeeto_proxy.cpp").read_text(encoding="utf-8")
assert "kMenuPagePluginPlatform" in p, "platform root page missing"
assert 'plugin_platform=%d\\n' in p and '"plugin_platform=%d"' in p
assert 'static bool g_optPluginPlatform = false;' in p
assert 'const bool pluginPlatformAtLaunch = LaunchOverrideEnabled(g_pluginPlatformCmdOverride, g_optPluginPlatform);' in p
assert 'CmdLineOverride01(L"-skeeto_plugin_platform")' in p
assert p.index('skplugins::Start(bootRoot,pluginPlatformAtLaunch)') > p.index('SktBootstrapMark(SKT_BOOT_READY)')
assert 'PluginMenuPaintPump(mode);' in p
menu=(core/"skeeto_plugins_menu.h").read_text(encoding="utf-8")
assert 'skplugins::RequestPage' in menu and 'skplugins::RequestAction' in menu
assert "MenuKeyEdge('R'," in p and 'PluginMenuRefresh' in p
render=p[p.index('static void MenuPaintEngine() {'):p.index('static void MenuOnVk(DWORD vk) {')]
assert not re.search(r'\.build_page\s*\(|\.on_action\s*\(|skplugins::PaintPump\s*\(',render)
assert 'PluginMenuPaintPump' not in p[p.index('static void MenuTick() {'):p.index('// Xhair —')]
host=(core/"skeeto_plugins.cpp").read_text(encoding="utf-8")
start=host.split('void Start(',1)[1].split('bool Started()',1)[0]
assert start.index('if(!enabled)return;') < start.index('FindFirstFileW')
assert 'FreeLibrary(' not in host and 'CreateThread(' not in host and 'VirtualQuery(' not in host
print('plugin menu: optional startup snapshot, shared pages/input, Paint ownership and render isolation passed')
