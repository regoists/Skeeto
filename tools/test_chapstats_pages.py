"""Compile real chapter-stat scoring/page/sort code with deterministic records.

Engine drawing is deliberately excluded. This does not validate game behavior.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def block(source, pattern):
    match = re.search(pattern, source, re.MULTILINE)
    if not match:
        raise ValueError(f"Missing production block: {pattern}")
    opening = source.index("{", match.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


FIXTURES = r'''
static void Reset() {
    g_optChapStatsScore = g_optChapStatsOverview = g_optChapStatsKills = true;
    g_optChapStatsSi = g_optChapStatsTeam = g_optChapStatsSpecial = true;
    g_optChapStatsDmg = false;
}
static ChapStatsRec Record() {
    ChapStatsRec r{};
    r.st[kCsCi] = 10; r.hsKill[kCsCi] = 2;
    r.st[kCsSmoker] = 2; r.hsKill[kCsSmoker] = 1;
    r.st[kCsTank] = r.st[kCsWitch] = 1;
    r.st[kCsRevive] = r.st[kCsPinned] = r.st[kCsMeleeSi] = 1;
    return r;
}
static bool Check(int got, int want, const char* label) {
    if (got == want) return true;
    printf("%s: got %d, expected %d\n", label, got, want);
    return false;
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const int test = atoi(argv[1]); Reset(); bool ok = true;
    const ChapStatsRec r = Record();
    if (test == 0) {
        // 30.5 non-SI kills + 9 SI + 10 team + 2 special; no duplicate SI.
        ok &= Check(ChapStatsScoreTotal(r), 5150, "all enabled total");
        ok &= Check(ChapStatsPageScoreContribution(r, kCsKindKills), 3050, "kills page");
        ok &= Check(ChapStatsPageScoreContribution(r, kCsKindSi), 900, "SI page");
        ok &= Check(ChapStatsPageScoreContribution(r, kCsKindTeam), 1000, "team page");
        ok &= Check(ChapStatsPageScoreContribution(r, kCsKindSpec), 200, "special page");
    } else if (test == 1) {
        g_optChapStatsKills = false;
        ok &= Check(ChapStatsScoreTotal(r), 2100, "hidden kills");
        g_optChapStatsKills = true; g_optChapStatsSi = false;
        ok &= Check(ChapStatsScoreTotal(r), 4250, "hidden SI");
        g_optChapStatsSi = true; g_optChapStatsTeam = false;
        ok &= Check(ChapStatsScoreTotal(r), 4150, "hidden team");
        g_optChapStatsTeam = true; g_optChapStatsSpecial = false;
        ok &= Check(ChapStatsScoreTotal(r), 4950, "hidden special");
        g_optChapStatsOverview = false;
        ok &= Check(ChapStatsScoreTotal(r), 4950, "overview visibility does not disable scoring");
    } else if (test == 2) {
        for (int mask = 0; mask < 32; ++mask) {
            g_optChapStatsOverview = (mask & 1) != 0;
            g_optChapStatsKills = (mask & 2) != 0;
            g_optChapStatsSi = (mask & 4) != 0;
            g_optChapStatsTeam = (mask & 8) != 0;
            g_optChapStatsSpecial = (mask & 16) != 0;
            const int pageKinds[] = {kCsKindScore, kCsKindKills, kCsKindSi, kCsKindTeam, kCsKindSpec};
            int expected[5]{}, n = 0;
            for (int k = 0; k < 5; ++k) if (mask & (1 << k)) expected[n++] = pageKinds[k];
            ok &= Check(ChapStatsPageCount(), n, "page count");
            const wchar_t* labels[5]{};
            ok &= Check(ChapStatsFillTabLabels(labels, 5), n, "tab count");
            for (int i = 0; i < n; ++i)
                ok &= Check(ChapStatsPageKind(i), expected[i], "page kind order");
            const int wantTotal = ((mask & 2) ? 3050 : 0) + ((mask & 4) ? 900 : 0)
                + ((mask & 8) ? 1000 : 0) + ((mask & 16) ? 200 : 0);
            ok &= Check(ChapStatsScoreTotal(r), wantTotal, "category total across all masks");
            const wchar_t* limited[1] = {nullptr};
            ok &= Check(ChapStatsFillTabLabels(limited, 1), n > 0 ? 1 : 0, "bounded tab write");
        }
        Reset(); g_optChapStatsScore = false;
        ok &= Check(ChapStatsPageCount(), 4, "legacy scoring disabled");
        ok &= Check(ChapStatsPageKind(0), kCsKindKills, "no overview without scoring");
    } else if (test == 3) {
        ChapStatsSet S{}; S.n = 2;
        S.rec[0].userid = 1; S.rec[0].st[kCsCi] = 100;
        S.rec[1].userid = 2; S.rec[1].st[kCsSmoker] = 10;
        const int total = 2;
        int idx[32] = {0, 1}; int page = 0;
        Sort(S, idx, total, page);
        ok &= Check(idx[0], 0, "overview leader");
        idx[0] = 0; idx[1] = 1; page = 2;
        Sort(S, idx, total, page);
        ok &= Check(idx[0], 1, "SI leader differs from total leader");
        S.rec[0] = {}; S.rec[1] = {};
        S.rec[0].userid = 1; S.rec[1].userid = 2;
        S.rec[0].st[kCsDeath] = 1; S.rec[1].st[kCsPinned] = 1;
        idx[0] = 0; idx[1] = 1; page = 3;
        Sort(S, idx, total, page);
        ok &= Check(idx[0], 1, "negative page scores descend");
        S.rec[0] = {}; S.rec[1] = {};
        S.rec[0].userid = 1; S.rec[0].bot = true; S.rec[1].userid = 2;
        idx[0] = 0; idx[1] = 1;
        Sort(S, idx, total, page);
        ok &= Check(idx[0], 1, "equal score humans first");
        S.rec[0].bot = false; idx[0] = 1; idx[1] = 0;
        Sort(S, idx, total, page);
        ok &= Check(idx[0], 0, "equal score userid order");
    } else if (test == 4) {
        ChapStatsRec special{};
        special.st[kCsMeleeSi] = 2;
        special.st[kCsMeleeSkeet] = special.st[kCsChargerLevel] = 1;
        ok &= Check(ChapStatsScoreTotal(special), 800, "no melee double bonus");
        g_csScore.hsMul = 150; g_csScore.ci = 3;
        ChapStatsRec custom{}; custom.st[kCsCi] = custom.hsKill[kCsCi] = 1;
        ok &= Check(ChapStatsScoreTotal(custom), 450, "custom weights preserved");
        g_optChapStatsKills = false;
        ok &= Check(ChapStatsScoreTotal(custom), 0, "disabled custom score");
    } else if (test == 5) {
        ChapStatsRec ff{};
        ff.st[kCsFfDamage] = 1;
        ok &= Check(ChapStatsPageScoreContribution(ff, kCsKindTeam), -50, "one FF damage at 0.5 mul is half a point");
        ff.st[kCsFfDamage] = 2;
        ok &= Check(ChapStatsPageScoreContribution(ff, kCsKindTeam), -100, "two FF damage points cost one score point");
        ff.st[kCsFfDamage] = 5;
        ok &= Check(ChapStatsPageScoreContribution(ff, kCsKindTeam), -250, "odd FF damage keeps fractional penalty");
        ok &= Check(ChapStatsScoreTotal(ff), -250, "overview includes FF penalty while team page is enabled");
        g_csScore.ffDmgMul = 25;
        ok &= Check(ChapStatsPageScoreContribution(ff, kCsKindTeam), -125, "custom 0.25 mul uses hundredths");
        g_csScore.ffDmgMul = 50;
        g_optChapStatsTeam = false;
        ok &= Check(ChapStatsScoreTotal(ff), 0, "hidden team page leaves FF penalty out of score");
    } else if (test == 6) {
        ChapStatsRec ff{};
        ff.st[kCsFfDamage] = 12;
        ff.st[kCsFfDamageTaken] = 500;
        ok &= Check(ChapStatsPageScoreContribution(ff, kCsKindTeam), -600,
            "received friendly fire must not change the attacker's damage penalty");
        ff.st[kCsFfDamage] = 0;
        ok &= Check(ChapStatsScoreTotal(ff), 0,
            "received friendly fire alone must not contribute to score");
    } else if (test == 7) {
        g_optChapStatsDmg = true;
        ChapStatsRec damage{};
        damage.st[kCsTankDmg] = 879; damage.st[kCsWitchDmg] = 250;
        ok &= Check(ChapStatsDmgScoreCents(damage), 1200, "damage page flooring and weights");
        ok &= Check(ChapStatsPageScoreContribution(damage, kCsKindDmg), 1200, "damage page contribution");
        ok &= Check(ChapStatsScoreTotal(damage), 1200, "damage adds to total");
        ok &= Check(ChapStatsPageCount(), 6, "damage page visibility");
        ok &= Check(ChapStatsPageKind(2), kCsKindDmg, "damage page order");
        damage.st[kCsTankDmg] = -100; damage.st[kCsWitchDmg] = -100;
        ok &= Check(ChapStatsDmgScoreCents(damage), 0, "negative damage is clamped");
    } else return 2;
    return ok ? 0 : 1;
}
'''


class ChapterPages(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        chunks = ["#include <windows.h>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>",
                  '#include "skeeto_ui_text.h"', 'static constexpr int kCsRecMax = 32;',
                  '#include "skeeto_damage_events.h"',
                  'static bool g_optChapStatsDmg = false, g_optChapStatsScore = true, g_optChapStatsOverview = true, '
                  'g_optChapStatsKills = true, g_optChapStatsSi = true, '
                  'g_optChapStatsTeam = true, g_optChapStatsSpecial = false;']
        for pattern in [r"^enum \{\s*\n\s*kCsCi", r"^enum \{\s*\n\s*kCsKindScore",
                        r"^struct ChapStatsRec \{", r"^struct ChapStatsSet \{",
                        r"^struct ChapStatsScoreW \{", r"^struct ChapStatsScoreParts \{"]:
            chunks.append(block(source, pattern) + ";")
        chunks.append(re.search(r"static constexpr ChapStatsScoreW kCsScoreDefault = \{.*?\};", source, re.S).group())
        chunks.append("static ChapStatsScoreW g_csScore = kCsScoreDefault; static constexpr int kCsPagesMax = 7;")
        for name in ["ChapStatsKillBasePts", "ChapStatsKills", "ChapStatsSiKills", "ChapStatsDmgScoreCents", "ChapStatsScoreOf",
                     "ChapStatsScoreTotal", "ChapStatsPageScoreContribution", "ChapStatsPageCount",
                     "ChapStatsPageKind", "ChapStatsFillTabLabels", "ChapStatsScoreMetric"]:
            chunks.append(block(source, rf"^static [^;\n]*\b{name}\([^;]*?\)\s*\{{"))
        sort = block(source, r"^\tfor \(int i = 1; i < total; \+\+i\) \{ // stable insertion sort")
        chunks.append("static void Sort(const ChapStatsSet& S, int* idx, int total, int page) {\n" + sort + "\n}")
        chunks.append(FIXTURES)
        cls.temp = tempfile.TemporaryDirectory(prefix="skeeto-chapstats-")
        out = Path(cls.temp.name)
        cpp = out / "test.cpp"; cpp.write_text("\n".join(chunks), encoding="utf-8")
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                                      "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property",
                                      "installationPath"], text=True).strip()
        cls.exe = out / "test.exe"
        bat = out / "compile.bat"
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                       f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /I"{ROOT / "src/skeeto/core"}" '
                       f'/Fe"{cls.exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n', encoding="utf-8")
        result = subprocess.run(["cmd", "/c", str(bat)], capture_output=True, text=True, errors="replace")
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_case(self, case):
        result = subprocess.run([str(self.exe), str(case)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_scores_are_owned_by_one_page(self): self.run_case(0)
    def test_hidden_categories_leave_the_total(self): self.run_case(1)
    def test_all_visibility_combinations(self): self.run_case(2)
    def test_actual_paint_sort_uses_current_page(self): self.run_case(3)
    def test_custom_weights_and_special_deduplication(self): self.run_case(4)
    def test_friendly_fire_damage_penalty_is_cumulative(self): self.run_case(5)
    def test_received_friendly_fire_is_recorded_without_score(self): self.run_case(6)
    def test_damage_score_is_included_on_its_own_page(self): self.run_case(7)


if __name__ == "__main__":
    unittest.main()
