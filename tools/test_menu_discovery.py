"""Run production discovery matching and generated menu metadata checks, offline."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
import importlib.util
import re

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = r'''
#include "skeeto_menu_discovery.h"
#include <cassert>
using namespace skmenufind;
static constexpr Entry entries[]={
 {L"镜像反转",L"Mirror",L"反转持枪模型",L"Mirror the held model",L"持枪视角",L"Viewmodel",L"翻转 flip",{121,0,2},false,false},
 {L"镜像反转强度",L"Mirror strength",L"测试参数",L"Parameter",L"持枪视角",L"Viewmodel",L"",{121,0,3},false,false},
 {L"物资雷达",L"Supply radar",L"显示附近物资",L"Show nearby supplies",L"导航与物资",L"Navigation",L"找东西",{100,0,1},false,false},
 {L"雷达透明度",L"Radar opacity",L"调整物资图标",L"Adjust icons",L"物资雷达",L"Supply radar",L"透明 alpha",{100,0,5},false,false},
 {L"镜像音效",L"Mirror audio",L"另一个镜像条目",L"Another entry",L"声音",L"Audio",L"",{70,0,1},false,false}
};
int main(){Index<5> index(entries);
 assert(index.Search(L"镜像反转")&&index.count==2&&index.results[0].index==0);
 assert(index.Search(L"  ViewMODEL  翻转 ")&&index.count==1&&index.results[0].index==0);
 assert(!wcscmp(index.query,L"ViewMODEL 翻转"));
 assert(index.Search(L"SUPPLY radar")&&index.count==2&&index.results[0].index==2);
 assert(index.Search(L"雷达 透明")&&index.count==1&&index.results[0].index==3);
 assert(index.Search(L"根本不存在的功能")&&index.count==0);
 wchar_t tooLong[66]{};for(int i=0;i<65;++i)tooLong[i]=L'a';
 assert(!index.Search(tooLong)&&index.count==0&&!wcscmp(index.query,L"根本不存在的功能"));
 assert(index.Search(L"镜像"));int expected[]={0,1,4};
 for(int i=0;i<3;++i)assert(index.results[i].index==expected[i]);
 for(int n=0;n<10;++n){index.Search(L"镜像");for(int i=0;i<3;++i)assert(index.results[i].index==expected[i]);}
 puts("PASS: exact/contains/alias/description ranking, bilingual AND tokens, stable ties, empty/missing/long query");
}
'''


class DiscoveryMatching(unittest.TestCase):
    def test_production_matching(self):
        with tempfile.TemporaryDirectory(prefix="skeeto-menu-find-") as tmp:
            folder = Path(tmp)
            (folder / "fixture.cpp").write_text(FIXTURE, encoding="utf-8")
            vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
            vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
            bat = folder / "compile.bat"
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX /I"{ROOT / "src/skeeto/core"}" fixture.cpp /Fe:fixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
            result = subprocess.run(["cmd", "/c", str(bat)], cwd=folder, capture_output=True, text=True, errors="replace")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(folder / "fixture.exe")], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class DiscoveryLabels(unittest.TestCase):
    def test_dynamic_counter_does_not_leave_sentence_fragments(self):
        path = ROOT / "tools/generate_menu_discovery.py"
        spec = importlib.util.spec_from_file_location("label_catalog", path)
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        self.assertEqual(generator.clean_label("[3] 重新扫描配置（发现 %d 套）"), "重新扫描配置")
        self.assertEqual(generator.clean_label("[3] Rescan configs (%d found)"), "Rescan configs")
        self.assertEqual(generator.clean_label("[8] 加载提速（Beta）"), "加载提速（Beta）")
        self.assertEqual(generator.clean_label("[4] 数值（1-100）"), "数值（1-100）")


class DiscoveryCatalog(unittest.TestCase):
    def test_catalog_is_generated_from_current_menu_sources(self):
        path = ROOT / "tools/generate_menu_discovery.py"
        spec = importlib.util.spec_from_file_location("menu_catalog", path)
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        rows = generator.build()
        self.assertGreater(len(rows), 300)
        keys = [(r["page"], r["sub"], r["row"]) for r in rows]
        self.assertEqual(len(keys), len(set(keys)))
        labels = {r["id"] for r in rows}
        for required in ("ViewModelMirror", "ViewModelMoveStep", "NativeUiFontSize", "SoundVolumeUi", "NavLabel47", "RadarLabel4", "PluginPlatformEntry"):
            self.assertIn(required, labels)
        self.assertTrue(all(r["zh"] and r["en"] and r["path_zh"] and r["path_en"] for r in rows))
        by_id = {}
        for row in rows:
            by_id.setdefault(row["id"], []).append(row)
        # These are actual consumer positions, including the two shared target
        # filters and the sound mixer's reserved controls (not a flat /9 grid).
        for ident, expected in {
            "ViewModelMirror": {(121, 0, 2)},
            "ViewModelMoveStep": {(121, 1, 1)},
            "NativeUiFontSize": {(131, 0, 5), (132, 0, 4)},
            "SoundVolumeUi": {(70, 1, 7)},
            "NavLabel47": {(86, 0, 5)},
            "NavLabel51": {(86, 0, 6)},
            "NavEdit": {(85, 1, 3)},
            "NavReload": {(86, 0, 4)},
            "NavWorldEdit": {(86, 0, 7)},
            "RadarHomeChanges": {(0, 2, 4)},
            # TabBoard OpenStyle uses [%d] + shared style-page paint; catalog keeps ChapStats style root.
            "PanelStyleEntry": {(83, 0, 0)},
            "RadarLabel4": {(116, 0, 5)},
            "RadarFilterEntry": {(117, 0, 0)},
            "RadarItem1": {(117, 0, 1)},
            "OverlayProgressColor": {(100, 0, 4)},
            "PluginPlatformEntry": {(134, 0, 0)},
        }.items():
            self.assertEqual({(r['page'], r['sub'], r['row']) for r in by_id[ident]}, expected, ident)
        source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        all_source = source + "\n" + "\n".join(p.read_text(encoding="utf-8-sig") for p in (ROOT / 'src/skeeto/core').glob('*.h'))
        pages = {int(v) for v in re.findall(r'constexpr int\s+kMenuPage\w+\s*=\s*(\d+)', all_source)}
        self.assertTrue(all(r['page'] in pages and 0 <= r['row'] <= 9 and 0 <= r['sub'] < 5 for r in rows))
        self.assertFalse(any(r['page'] in (48, 135, 136, 137, 138) for r in rows))
        self.assertFalse(any(re.search('Diagnostic|PrintStatus',r['id']) for r in rows))
        self.assertFalse(any(r['page']==98 and r['row']>0 for r in rows), 'overlay root must not inherit its edit rows')
        self.assertIn('翻转', by_id['ViewModelMirror'][0]['aliases'])
        self.assertIn('枪的位置', by_id['ViewModelRootEntry'][0]['aliases'])
        self.assertTrue(all(r['local'] for r in rows if r['group']=='Local'))
        self.assertTrue(all(r['experimental'] for r in rows if 'Beta' in r['path_zh']))
        result = subprocess.run([os.sys.executable, str(path), "--check"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class DiscoveryCoverage(unittest.TestCase):
    # Rows of table-driven pages that search cannot reach. Leaf HUD pages with
    # dynamically painted labels dominate; the total may shrink but not grow.
    MISSING_ROW_CEILING = 70

    def test_table_rows_stay_searchable(self):
        spec = importlib.util.spec_from_file_location("menu_catalog_cov", ROOT / "tools/generate_menu_discovery.py")
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        pages = {k: int(v) for k, v in re.findall(r'constexpr int\s+(kMenuPage\w+)\s*=\s*(\d+)', source)}
        found = {}
        for r in generator.build():
            if r["row"] > 0:
                found.setdefault(r["page"], set()).add((r["sub"], r["row"]))
        missing = 0
        for table, page_name in generator.TABLES.items():
            body = re.search(r'\b' + table + r'\[\]\s*=\s*\{([\s\S]*?)\n\};', source)
            rows = len(re.findall(r'\{\s*\w+Action::', body.group(1))) if body else 0
            missing += max(0, rows - len(found.get(pages.get(page_name, int(page_name) if str(page_name).isdigit() else -1), ())))
        self.assertLessEqual(missing, self.MISSING_ROW_CEILING, f"{missing} table rows are not searchable")


if __name__ == "__main__":
    unittest.main()
