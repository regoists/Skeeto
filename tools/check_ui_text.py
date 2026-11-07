"""Validate the compiled catalog against its actual C++ call sites."""
import re
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WIDE = r'L"(?:\\.|[^"\\])*"(?:\s*L"(?:\\.|[^"\\])*")*'
FORMAT = re.compile(r'%(?:%|[-+ #0]*?(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:I64|I32|hh|ll|[hljztLw])?[diuoxXfFeEgGaAcCsSpn])')
# Deliberately blank labels. NavItem0 is the navigation item with no text.
BLANK_LABELS = frozenset({'NavItem0'})
TOKEN = re.compile(WIDE + r'|(?:u8)?"(?:\\.|[^"\\])*"|//[^\n]*|/\*[\s\S]*?\*/')

def format_args(text):
    args = []
    for match in FORMAT.finditer(text):
        spec = match.group()
        if spec == '%%': continue
        # "90% notice" and "(% of screen)" are prose, not printf conversions.
        if (match.start() > 0 and text[match.start() - 1].isdigit()) or spec[1].isspace(): continue
        args.extend(['width'] * spec.count('*'))
        args.append(re.sub(r'^%[-+ #0]*[\d*]*(?:\.[\d*]+)?', '', spec))
    return args

def read_catalog(path):
    src = path.read_text(encoding='utf-8-sig')
    src = TOKEN.sub(lambda m: '' if m.group().startswith(('//', '/*')) else m.group(), src)
    rows = re.findall(r'SKEETO_UI\((\w+),\s*('+WIDE+r')\)', src)
    result = {}
    for key, value in rows:
        if key in result: raise ValueError('duplicate token: '+key)
        if value == 'L""' and key not in BLANK_LABELS: raise ValueError('empty translation: '+key)
        result[key] = value
    return result

def uitext_calls(src):
    calls, start = [], 0
    while True:
        at = src.find('UiText(', start)
        if at < 0: break
        i, depth = at + len('UiText('), 1
        while i < len(src) and depth:
            if src.startswith(('L"', '"', "L'", "'"), i):
                quote = '"' if src[i:i+2] in ('L"', '"') or src[i] == '"' else "'"
                i += 2 if src[i] == 'L' else 1
                while i < len(src) and src[i] != quote:
                    i += 2 if src[i] == '\\' else 1
                i += 1
                continue
            depth += src[i] == '('
            depth -= src[i] == ')'
            i += 1
        calls.append((at, i))
        start = i
    return calls

def validate_source(src, catalog, allowed):
    src = TOKEN.sub(lambda m: ' ' * len(m.group()) if m.group().startswith(('//', '/*')) else m.group(), src)
    errors, used = [], set()
    localized = []
    for begin, end in uitext_calls(src):
        body = src[begin:end]
        ids = re.findall(r'UiId::(\w+)', body)
        strings = list(re.finditer(WIDE, body))
        for key in ids:
            if key == 'Count': continue
            used.add(key)
            if key not in catalog:
                errors.append('missing token: '+key)
        if len(ids) == len(strings):
            for key, match in zip(ids, strings):
                localized.append((begin + match.start(), begin + match.end()))
                if key != 'Count' and key in catalog and format_args(match.group()) != format_args(catalog[key]):
                    errors.append('format mismatch: '+key)
        else:
            for match in strings:
                localized.append((begin + match.start(), begin + match.end()))
    # Table rows written as UiId, fallback even when not wrapped in UiText yet.
    for match in re.finditer(r'UiId::(\w+),\s*('+WIDE+r')', src):
        key, chinese = match.groups()
        localized.append(match.span(2))
        if key == 'Count': continue
        used.add(key)
        if key not in catalog:
            errors.append('missing token: '+key)
        elif format_args(chinese) != format_args(catalog[key]):
            errors.append('format mismatch: '+key)
    localized_literals = {src[a:b] for a, b in localized}
    for match in TOKEN.finditer(src):
        raw = match.group()
        if not raw.startswith('L"') or not re.search(r'[\u3400-\u9fff]', raw): continue
        if any(a <= match.start() < b for a, b in localized) or raw in localized_literals: continue
        if raw not in allowed:
            errors.append(f'unlocalized line {src.count(chr(10), 0, match.start()) + 1}: {raw}')
    return list(dict.fromkeys(errors)), used

def check():
    core = ROOT / 'src/skeeto/core'
    catalog = read_catalog(core / 'skeeto_ui_en.inc')
    exclusions = json.loads((ROOT / 'tools/ui_text_exclusions.json').read_text(encoding='utf-8'))
    errors = []
    used = set()
    for path in [*core.glob('*.cpp'), *core.glob('*.h')]:
        text = path.read_text(encoding='utf-8-sig')
        found, refs = validate_source(text, catalog, exclusions.get(path.name, {}))
        errors.extend(f'{path.name}: {e}' for e in found)
        used.update(refs)
    # Subdirectories reference the same catalog. Count every UiId so a token
    # used outside the top-level files is not reported unused.
    for path in [*core.rglob('*.cpp'), *core.rglob('*.h')]:
        used.update(re.findall(r'UiId::(\w+)', path.read_text(encoding='utf-8-sig')))
    for key in catalog.keys() - used:
        errors.append('unused token: '+key)
    if errors: raise ValueError('\n'.join(errors))
    print(f'UI catalog: {len(catalog)} entries, all used; format arguments match.')

if __name__ == '__main__':
    check()
