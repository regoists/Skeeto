"""Supply radar wiring checks; no game, DLL build or user settings are touched."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from check_ui_text import WIDE, format_args

ROOT = Path(__file__).resolve().parents[1]
PROXY = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
ENGLISH = (ROOT / "src/skeeto/core/skeeto_ui_en.inc").read_text(encoding="utf-8-sig")


class SupplyRadarIntegration(unittest.TestCase):
    def assertIn(self, member, container, msg=None):
        self.assertTrue(member in container, msg or f"missing wiring: {member}")

    def assertRegex(self, text, regex, msg=None):
        self.assertTrue(re.search(regex, text) is not None, msg or f"missing wiring pattern: {regex}")

    def test_default_off(self):
        self.assertIn("bool g_optSupplyRadar = false;", PROXY)
        self.assertIn("skeeto_nav::RadarResetDefaults();", PROXY)

    def test_all_settings_save_and_load(self):
        self.assertRegex(PROXY, r"for \(int i = 0; i < skeeto_nav::kRadarSettingCount; \+\+i\)\s*fprintf\(f,.*RadarKey\(i\).*RadarGet\(i\)")
        self.assertIn('"supply_radar_", 13', PROXY)
        self.assertIn("skeeto_nav::RadarSet(i, v);", PROXY)

    def test_independent_wanted(self):
        self.assertIn("skeeto_nav::PresentationActive() || g_optSupplyRadar ||", PROXY)
        self.assertIn("skeeto_nav::Active() || g_optSupplyRadar)", PROXY)

    def test_hud_visibility_gate(self):
        self.assertIn("customHudVisible && g_optSupplyRadar", PROXY)
        self.assertRegex(PROXY, r"skeeto_nav::Paint\([^;]+,\s*customHudVisible && g_optSupplyRadar\);")

    def test_menu_paging_and_root(self):
        self.assertIn("static_assert(MenuRootPageCount() == 3", PROXY)
        self.assertIn("case kMenuPageSupplyRadar: return RadarMainMenuPageCount();", PROXY)
        self.assertIn("RadarRootEntry", PROXY)
        self.assertIn("RadarFilterEntry", PROXY)

    def test_filter_and_layout_paths(self):
        self.assertIn("RadarMenuItemLabel", PROXY)
        self.assertIn("skeeto_nav::RadarItemSupported", PROXY)
        self.assertIn("skeeto_nav::RadarBeginEdit", PROXY)
        self.assertIn("RadarMenuItemEnabled", PROXY)

    def test_input_and_cancel_paths(self):
        self.assertIn("RadarMenuAdjust(dir)", PROXY)
        self.assertIn("RadarMenuAdjust(+1)", PROXY)
        self.assertIn("skeeto_nav::RadarEditKey", PROXY)
        self.assertIn("skeeto_nav::RadarEndEdit()", PROXY)
        self.assertNotIn("skeeto_nav::RadarEndEdit(false)", PROXY)
        self.assertNotIn("skeeto_nav::RadarEndEdit(true)", PROXY)

    def test_menu_edit_is_deferred_until_next_paint(self):
        start = PROXY.index("static void RadarMenuBeginEdit(unsigned label)")
        end = PROXY.index("static int MenuPageSubCount", start)
        begin = PROXY[start:end]
        self.assertIn("g_radarEditRequest.store((int)label, std::memory_order_release);", begin)
        self.assertNotIn("skeeto_nav::RadarBeginEdit(label)", begin)
        self.assertIn("!BackgroundConsoleOpen()", PROXY)
        self.assertIn("skeeto_nav::RadarBeginEdit((unsigned)radarRequest);", PROXY)

    def test_console_and_localization(self):
        self.assertIn('RegisterEngineCommand("skeeto_supply_radar"', PROXY)
        self.assertIn('RegisterEngineCommand("skeeto_supply_radar_edit"', PROXY)
        ids = set(re.findall(r"UiId::(Radar\w+)", PROXY))
        self.assertGreater(len(ids), 15)
        for name in ids:
            self.assertIn(f"SKEETO_UI({name},", ENGLISH)

    def test_new_translation_format_arguments(self):
        catalog = dict(re.findall(r"SKEETO_UI\((Radar\w+),\s*(" + WIDE + r")\)", ENGLISH))
        calls = re.findall(r"(?:UiText\(UiId::|\{UiId::)(Radar\w+),\s*(" + WIDE + r")", PROXY)
        self.assertGreater(len(calls), 30)
        for name, chinese in calls:
            self.assertIn(name, catalog)
            self.assertEqual(format_args(chinese), format_args(catalog[name]), name)

    def test_real_menu_mapping_and_enabled_layouts(self):
        """Compile the actual pure menu helpers, with a fixed engine-free API stub."""
        compiler = shutil.which("clang++") or shutil.which("g++")
        self.assertIsNotNone(compiler, "a C++ compiler is required for menu mapping checks")
        names = ["RadarMenuItemEnabled", "RadarMenuItemCount", "RadarMenuItemLabel", "RadarMenuSettingIndex", "RadarMenuBeginEdit"]
        functions = []
        for name in names:
            start = re.search(r"^static [^\n]+ " + name + r"\(", PROXY, re.M).start()
            opening = PROXY.index("{", start)
            depth, end = 1, opening + 1
            while depth:
                depth += (PROXY[end] == "{") - (PROXY[end] == "}")
                end += 1
            functions.append(PROXY[start:end])
        constants = "static constexpr int kMenuItemsPerPage = 9;\nstatic constexpr int kMenuRootItemsPerPage = kMenuItemsPerPage;\n"
        constants += "\n".join(f"[[maybe_unused]] static constexpr int {n} = {v};" for n, v in dict(re.findall(r"constexpr int (kMenuPage\w+) = (\d+);", PROXY + (ROOT / "src/skeeto/core/skeeto_host.h").read_text(encoding="utf-8-sig"))).items())

        def span(start, end):
            first = PROXY.index(start)
            return PROXY[first:PROXY.index(end, first) + len(end)]

        table_decls = "\n".join([
            span("static constexpr int MenuPageCountForItems(int itemCount) {", "\n}\n"),
            span("enum class MenuRootAction", "bool resetSubpage;\n};"),
            span("static constexpr MenuRootItem kMenuRootItems[] = {", "&kMenuRootItems[index] : nullptr;\n}"),
            "static_assert(MenuRootItemsValid(), \"root menu\");",
            span("enum class RadarMenuAction", "&kRadarMainMenuItems[index] : nullptr;\n}"),
        ])
        root_start = PROXY.index("\tif (g_menuPage == kMenuPageRoot) {", PROXY.index("static void MenuProcessKeyImpl(int digit)"))
        opening = PROXY.index("{", root_start)
        depth, end = 1, opening + 1
        while depth:
            depth += (PROXY[end] == "{") - (PROXY[end] == "}")
            end += 1
        root_input = PROXY[root_start:end]
        ui_ids = sorted(set(re.findall(r"UiId::(\w+)", root_input + table_decls)))
        root_stub = "\nenum class UiId {" + ",".join(ui_ids) + "};\n" + r'''
int g_menuPage=0, g_menuSub=0, g_soundVolumeMenuSel=0, docsOpened=0;
bool g_menuVisible=false;
void MenuForceClose() {g_menuVisible=false;}
const wchar_t* UiText(UiId, const wchar_t* text) {return text;}
void EngClientCmd(const char*) {}
void MenuToggle() {}
bool MenuDiscoveryRequest(bool) {return true;}
void CareerArchiveRefresh() {}
void SkeetoOpenRootDoc(const char*,const wchar_t*) {++docsOpened;}
''' + table_decls + r'''
void HomeInput(int digit) {
''' + root_input + "\n}\n"
        save = re.search(r"for \(int i = 0; i < skeeto_nav::kRadarSettingCount; \+\+i\)\s*fprintf\(f,[^;]+;", PROXY).group()
        load_start = PROXY.index('if (!_strnicmp(line, "supply_radar_", 13))')
        opening = PROXY.index("{", load_start)
        depth, end = 1, opening + 1
        while depth:
            depth += (PROXY[end] == "{") - (PROXY[end] == "}")
            end += 1
        persistence = "\nvoid SaveRadar(FILE* f) {" + save + "}\nvoid LoadRadar(FILE* f) {char line[128]; while(fgets(line,sizeof(line),f)) {int v=0;" + PROXY[load_start:end] + "}}\n"
        stub = r'''
#include <cassert>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <strings.h>
#define _strnicmp strncasecmp
bool g_optSupplyRadar=false;
std::atomic_int g_radarEditRequest{-1};
namespace skeeto_nav {
constexpr int kRadarLabelCount=35, kRadarItemBase=15, kRadarLayoutBase=50, kRadarSettingCount=190;
int settings[190];
int radarBeginCalls=0;
void RadarBeginEdit(unsigned) {++radarBeginCalls;}
bool RadarItemSupported(unsigned label) { return label>=1&&label<=34&&label!=10&&label!=22&&label!=23&&label!=30&&label!=31; }
int RadarGet(int index) { return index>=0&&index<190 ? settings[index] : 0; }
void RadarSet(int index,int value) { settings[index]=value; }
bool RadarEnabled() {return settings[0]!=0;}
const char* RadarKey(int index) {static char key[64]; std::snprintf(key,sizeof(key),"supply_radar_test_%d",index);return key;}
}
'''
        main = r'''
int main() {
    using namespace skeeto_nav;
    for (int& setting : settings) setting = 1;
    static_assert(kRadarLayoutBase==kRadarItemBase+kRadarLabelCount,"layout block follows item block");
    assert(RadarMenuItemCount(false)==34);
    assert(RadarMenuItemCount(true)==29);
    g_optSupplyRadar=true;g_menuVisible=true;RadarMenuBeginEdit(1);
    assert(!g_menuVisible && g_radarEditRequest.load()==1 && radarBeginCalls==0);
    g_radarEditRequest.store(-1);RadarMenuBeginEdit(10);assert(g_radarEditRequest.load()==-1);
    g_radarEditRequest.store(-1);RadarMenuBeginEdit(34);assert(g_radarEditRequest.load()==34);
    g_radarEditRequest.store(-1);
    int last=0;
    for (int slot=0;slot<34;++slot) {
        int label=RadarMenuItemLabel(slot/9,slot%9,false);
        assert(label==slot+1);
        int index=RadarMenuSettingIndex(kMenuPageSupplyRadarItems,slot/9,slot%9);
        assert(index==(RadarItemSupported(label)?kRadarItemBase+label:-1));
    }
    for (int slot=0;slot<29;++slot) {
        int label=RadarMenuItemLabel(slot/9,slot%9,true);
        assert(label>last && RadarItemSupported(label)); last=label;
    }
    assert(last==34);
    assert(RadarMenuItemLabel(3,2,true)==-1);
    assert(RadarMenuItemLabel(3,7,false)==-1);
    for (int row=0;row<9;++row) {
        assert(RadarMenuSettingIndex(kMenuPageSupplyRadar,0,row)==(row<8?row:-1));
        assert(RadarMenuSettingIndex(kMenuPageSupplyRadar,1,row)==(row<7?8+row:-1));
        assert(RadarMenuSettingIndex(kMenuPageSupplyRadar,2,row)==-1);
    }
    assert(RadarMenuSettingIndex(kMenuPageSupplyRadarLayout,0,0)==-1);
    // Category switches (12 medical, 13 throwables, 14 weapons) gate their labels only.
    settings[12]=0; assert(RadarMenuItemCount(true)==25);
    settings[13]=0; assert(RadarMenuItemCount(true)==22);
    settings[14]=0; assert(RadarMenuItemCount(true)==5); // ammo, laser, gas can, fireworks, safehouse
    settings[kRadarItemBase+9]=0; assert(RadarMenuItemCount(true)==4);
    settings[kRadarItemBase+34]=0; assert(RadarMenuItemCount(true)==3);
    settings[kRadarItemBase+8]=0; settings[kRadarItemBase+32]=0; settings[kRadarItemBase+33]=0;
    assert(RadarMenuItemCount(true)==0);
    assert(RadarMenuItemLabel(0,0,true)==-1);
    const int expected[]={kMenuPageBackgroundEngine,kMenuPageNavigation,kMenuPageSupplyRadar,kMenuPageViewModel,kMenuPageSoundVolume,kMenuPageCombatVisual,kMenuPageClientConvenience,kMenuPageCrashGuard,kMenuPageNecolaStartup};
    for(int digit=1;digit<=9;++digit) {
        g_menuPage=0; g_menuSub=1; HomeInput(digit);
        assert(g_menuPage==expected[digit-1]);
    }
    assert(docsOpened==0);
    g_menuPage=0; g_menuSub=2; HomeInput(1); assert(g_menuPage==kMenuPageLocalListen);
    g_menuPage=0; g_menuSub=2; HomeInput(2); assert(g_menuPage==kMenuPageCareerArchive);
    g_menuPage=0; g_menuSub=2; HomeInput(3); assert(g_menuPage==kMenuPageMenuStyle);
    g_menuPage=0; g_menuSub=2; HomeInput(4); assert(docsOpened==1 && g_menuPage==0);
    g_menuPage=0; g_menuSub=0; HomeInput(9); assert(g_menuPage==kMenuPageFastLoad);
    FILE* saved=std::tmpfile(); assert(saved);
    for(int i=0;i<kRadarSettingCount;++i) settings[i]=i*17+1;
    SaveRadar(saved); std::rewind(saved); std::memset(settings,0,sizeof(settings));
    LoadRadar(saved); std::fclose(saved);
    for(int i=0;i<kRadarSettingCount;++i) assert(settings[i]==i*17+1);
    assert(g_optSupplyRadar);
    std::puts("PASS actual menu mappings, all supported labels, category gating and enabled-only layouts");
}
'''
        with tempfile.TemporaryDirectory(prefix="skeeto-radar-menu-") as directory:
            path = Path(directory)
            source, binary = path / "menu.cpp", path / "menu.exe"
            source.write_text(stub + constants + root_stub + persistence + "\n" + "\n".join(functions) + main, encoding="utf-8")
            built = subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
            run = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
