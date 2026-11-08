from pathlib import Path
import tempfile
import unittest
from check_ui_text import format_args, read_catalog, validate_source


class CatalogChecks(unittest.TestCase):
    def test_format_types_and_dynamic_width(self):
        self.assertEqual(format_args('L"%s %hs %I64u %.*ls %%"'),
                         ['s', 'hs', 'I64u', 'width', 'ls'])

    def test_rejects_pointer_integer_mismatch(self):
        errors, _ = validate_source('UiText(UiId::Label, L"名称 %s")', {'Label': 'L"Name %d"'}, {})
        self.assertTrue(any('format mismatch' in e for e in errors))

    def test_rejects_untranslated_visible_text(self):
        errors, _ = validate_source('draw(L"返回");', {}, {})
        self.assertTrue(any('unlocalized' in e for e in errors))

    def test_accepts_static_choices_and_ignores_comments(self):
        errors, used = validate_source('// L"注释"\n{UiId::Label, L"返回"}', {'Label': 'L"Back"'}, {})
        self.assertEqual(errors, [])
        self.assertEqual(used, {'Label'})

    def test_prose_percent_and_urls_stay_intact(self):
        self.assertEqual(format_args('L"90% notice, 0% to 100%, (% of screen)"'), [])
        self.assertEqual(format_args('L"version %s"'), ['s'])
        with tempfile.NamedTemporaryFile('w', encoding='utf-8', suffix='.inc', delete=False) as catalog:
            catalog.write('SKEETO_UI(Hint, L"see https://example.test/a")\n')
            path = catalog.name
        self.assertEqual(read_catalog(Path(path)), {'Hint': 'L"see https://example.test/a"'})

    def test_missing_entry_is_rejected(self):
        errors, _ = validate_source('UiText(UiId::Absent, L"返回")', {}, {})
        self.assertTrue(any('missing' in e for e in errors))


if __name__ == '__main__':
    unittest.main()
