import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WORLD_DRAW = ROOT / "src/skeeto/core/skeeto_navigation/world_draw.h"


class WorldRouteArrowColor(unittest.TestCase):
    def test_route_card_direction_glyph_uses_configured_arrow_color(self):
        source = WORLD_DRAW.read_text(encoding="utf-8-sig")
        self.assertIn(
            "const unsigned arrowColor=WorldArrowColor(settings.worldArrowColor);",
            source,
        )

        start = source.index("float cx=float(x+21)")
        end = source.index("Text(x+40,y+9,message,color,worldFont)", start)
        glyph = source[start:end]
        self.assertIn("WorldLine(a,tip,arrowColor,230,1)", glyph)
        self.assertIn("WorldLine(b,tip,arrowColor,230,1)", glyph)


if __name__ == "__main__":
    unittest.main()
