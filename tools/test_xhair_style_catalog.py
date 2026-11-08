import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROXY = ROOT / "src/skeeto/core/skeeto_proxy.cpp"


class XhairStyleCatalogChecks(unittest.TestCase):
    def test_x_variants_are_appended_with_factory_tunes_and_draw_cases(self):
        source = PROXY.read_text(encoding="utf-8-sig")
        count_match = re.search(r"kXhairStyleCount\s*=\s*(\d+)", source)
        self.assertIsNotNone(count_match)
        count = int(count_match.group(1))

        names_match = re.search(
            r"kXhairStyleNames\s*=\s*\{\{(.*?)\}\};", source, re.S
        )
        self.assertIsNotNone(names_match)
        names = re.findall(r"\{UiId::(\w+),\s*L\"[^\"]+\"\}", names_match.group(1))
        self.assertEqual(count, len(names))
        self.assertEqual(
            names[-4:],
            ["XhairStyleX", "XhairStyleXDot", "XhairStyleXRing", "XhairStyleXRingDot"],
        )

        factory_match = re.search(
            r"kXhairFactory\[kXhairStyleCount\]\s*=\s*\{(.*?)\n\};", source, re.S
        )
        self.assertIsNotNone(factory_match)
        tunes = re.findall(r"^\s*\{\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+\s*\}", factory_match.group(1), re.M)
        self.assertEqual(count, len(tunes))

        draw_match = re.search(
            r"static void XhairDrawAll\(.*?\n\}(?=\n\nstatic int XhairStyleSig)",
            source,
            re.S,
        )
        self.assertIsNotNone(draw_match)
        draw = draw_match.group(0)
        self.assertIn("auto xCross = [&]()", draw)
        cases = {
            int(number): body
            for number, body in re.findall(
                r"case\s+(\d+):\s*//[^\n]*([\s\S]*?)(?=\n\s*case\s+\d+:|\n\s*default:)",
                draw,
            )
        }
        self.assertTrue(all(i in cases for i in range(16, 20)))
        for i in range(16, 20):
            self.assertIn("xCross();", cases[i])
        self.assertIn("maybeDot();", cases[17])
        self.assertIn("XhairCircleAA", cases[18])
        self.assertIn("XhairCircleAA", cases[19])
        self.assertIn("maybeDot();", cases[19])


if __name__ == "__main__":
    unittest.main()
