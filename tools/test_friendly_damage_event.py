"""Exercise the production direct-friendly-damage and Tank target filters."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

from test_chapstats_pages import ROOT, block


PRELUDE = r'''
#include <cassert>
#include "skeeto_damage_events.h"
int EntIndexFromHandle(int h) {
    if (h == 0 || h == -1) return 0;
    const int idx = h & 0xFFF;
    if (idx <= 0 || idx >= 2048) return 0;
    return idx;
}
'''

CASES = r'''
int main() {
    assert(FeedbackFriendlyDamageEventAllowed(1, 2, 2, 2, 1, 11));
    assert(!FeedbackFriendlyDamageEventAllowed(1, 2, 1, 2, 1, 11)); // self damage
    assert(!FeedbackFriendlyDamageEventAllowed(1, 2, 2, 3, 1, 11)); // infected victim
    assert(!FeedbackFriendlyDamageEventAllowed(1, 2, 2, 2, 3, 11)); // other attacker
    assert(!FeedbackFriendlyDamageEventAllowed(1, 3, 2, 2, 1, 11)); // local not survivor
    assert(!FeedbackFriendlyDamageEventAllowed(1, 2, 2, 2, 1, 0));  // no health damage
    assert(!FeedbackFriendlyDamageEventAllowed(0, 2, 2, 2, 0, 11));

    assert(TankTargetHudTargetIndex(0) == 0);
    assert(TankTargetHudTargetIndex(-1) == 0);
    assert(TankTargetHudTargetIndex(64) == 64);
    assert(TankTargetHudTargetIndex(0x1000 | 7) == 7); // EHANDLE serial bits
    assert(TankTargetHudTargetIndex(65) == 0);         // not a player slot
    assert(TankTargetHudTargetIndex(2048) == 0);
    return 0;
}
'''


class DirectFriendlyDamage(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        proxy = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        hud = (ROOT / "src/skeeto/core/skeeto_hud.cpp").read_text(encoding="utf-8-sig")
        try:
            cls.ff_helper = block(proxy, r"^static bool FeedbackFriendlyDamageEventAllowed\(")
            cls.tank_helper = block(hud, r"^static int TankTargetHudTargetIndex\(")
        except (AssertionError, AttributeError, ValueError):
            cls.ff_helper = cls.tank_helper = ""
        if not cls.ff_helper or not cls.tank_helper:
            return

        cls.temp = tempfile.TemporaryDirectory(prefix="skeeto-direct-ff-")
        out = Path(cls.temp.name)
        cpp = out / "test.cpp"
        cpp.write_text(PRELUDE + cls.ff_helper + cls.tank_helper + CASES, encoding="utf-8")
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        vs = subprocess.check_output([
            str(vswhere), "-latest", "-products", "*", "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"
        ], text=True).strip()
        cls.exe = out / "test.exe"
        bat = out / "compile.bat"
        bat.write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
            f'cl /nologo /EHsc /std:c++17 /W3 /I"{ROOT / "src/skeeto/core"}" /Fe"{cls.exe}" /Fo"{out / "test.obj"}" "{cpp}"\n'
            'exit /b %errorlevel%\n', encoding="utf-8"
        )
        result = subprocess.run(["cmd", "/c", str(bat)], capture_output=True, text=True, errors="replace")
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "temp"):
            cls.temp.cleanup()

    def test_direct_friendly_damage_and_tank_player_handle_filters(self):
        self.assertTrue(self.ff_helper, "production direct-friendly-damage filter is missing")
        self.assertTrue(self.tank_helper, "production Tank player-handle filter is missing")
        result = subprocess.run([str(self.exe)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

if __name__ == "__main__":
    unittest.main()
