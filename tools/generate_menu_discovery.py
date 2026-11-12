"""Compile existing numbered-menu metadata into an immutable bilingual catalog.

Runs at build time only. Table order, UI text and direct numbered rows are read
from the current source; no game process, user config or package is inspected.
"""
from pathlib import Path
import argparse
import ast
import json
import re

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "src/skeeto/core"
OUTPUT = CORE / "skeeto_menu_catalog.generated.inc"
LITERAL = r'L"(?:\\.|[^"\\])*"(?:\s*L"(?:\\.|[^"\\])*")*'
UI = re.compile(r'UiText\(UiId::(\w+),\s*(' + LITERAL + r')')
LEX = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')

# Only source-family metadata belongs here. Names, help and item order stay in
# their existing menu tables. Rows without labels resolve their action's painter.
TABLES = {
    "kMenuRootItems": "0", "kNavCategoryItems": "kMenuPageNavigation",
    "kClientBackItemMenuItems": "kMenuPageBackItems",
    "kRadarMainMenuItems": "kMenuPageSupplyRadar",
    "kSoundVolumeMenuItems": "kMenuPageSoundVolume",
    "kSoundVolumeAuxMenuItems": "kMenuPageSoundVolume",
    "kCustomHudMenu": "kMenuPageCustomHudHub",
    "kViewModelMenuItems": "kMenuPageViewModel",
    "kFastLoadMenuItems": "kMenuPageFastLoad",
    "kFastLoadExperimentItems": "kMenuPageFastLoadExperiments",
    "kGameUiMenuItems": "kMenuPageGameUi",
    "kMainUiSetupItems": "kMenuPageMainMenuUi",
    "kEscUiSetupItems": "kMenuPageEscMenuUi",
    "kCombatVisualMenuItems": "kMenuPageCombatVisual",
    "kCampaignOutlineMenuItems": "kMenuPageCampaignOutline",
    "kInteractScanMenuItems": "kMenuPageInteractScan",
    "kViewRollMenuItems": "kMenuPageViewRollSetup",
    "kShotgunReloadMenuItems": "kMenuPageShotgunReload",
    "kXhairTpMenuItems": "kMenuPageXhairThirdPerson",
    "kHoldSemiMenuItems": "kMenuPageHoldSemiSetup",
    "kDissolveMenuItems": "kMenuPageDissolveSetup",
    "kAcidOutlineMenuItems": "kMenuPageAcidOutlineSetup",
    "kWorldHudMenu": "kMenuPageWorldHud",
    "kThrowLandMenuItems": "kMenuPageThrowLand",
    "kWorldTankPredictMenuItems": "kMenuPageWorldTankPredict",
    "kNecolaStartupMenuItems": "kMenuPageNecolaStartup",
    "kHitFeedbackMenuItems": "kMenuPageHitFeedback",
    "kSkeetoScriptMenuItems": "kMenuPageSkeetoScript",
    "kAlarmCarGlowMenuItems": "kMenuPageAlarmCarGlow",
    "kClientPerfMenuItems": "kMenuPageClientPerf",
    "kParryFlashMenuItems": "kMenuPageParryFlash",
    "kSupplyScanMenuItems": "kMenuPageSupplyScan",
    "kSupplyScanHudMenuItems": "kMenuPageSupplyScanHud",
    "kSupplyScanFilterCatItems": "kMenuPageSupplyScanFilter",
    "kMenuStyleItems": "kMenuPageMenuStyle",
    "kLocalDebugMenuItems": "kMenuPageLocalDebugCvars",
    "kSurvivorGlowMenuItems": "kMenuPageSurvivorGlow",
    "kClientConvenienceMenuItems": "kMenuPageClientConvenience",
    "kForceCvarMenuItems": "kMenuPageForceCvar",
    "kCrashGuardMenuItems": "kMenuPageCrashGuard",
    "kClearDecalsMenuItems": "kMenuPageClearDecals",
    "kWModelScaleMenuItems": "kMenuPageWModelScale",
    "kOverlayHudRootMenuItems": "kMenuPageOverlayHud",
    "kOverlayAmmoEditItems": "kMenuPageOverlayAmmo",
    "kOverlayReloadEditItems": "kMenuPageOverlayReload",
    "kOverlayFireEditItems": "kMenuPageOverlayFireGate",
    "kLocalListenMenuItems": "kMenuPageLocalListen",
    "kLocalListenExtraMenuItems": "kMenuPageLocalListen",
    "kLocalBugMenuItems": "kMenuPageLocalBug",
    "kLocalDirectorMenuItems": "kMenuPageLocalDirector",
    "kLastMusicMenuItems": "kMenuPageLastMusic",
    "kSafeMusicMenuItems": "kMenuPageSafeMusic",
    "kSiWarningMenuItems": "kMenuPageSiWarningHud",
    "kWorldNamesMenuItems": "kMenuPageWorldNamesColor",
    "kChapStatsMenuItems": "kMenuPageChapStats",
    "kChapStatsSizeMenuItems": "kMenuPageChapStatsSize",
    "kChapStatsDisplayMenuItems": "kMenuPageChapStatsDisplay",
    "kChapStatsScoreHudMenuItems": "kMenuPageChapStatsScoreHud",
    # WAVE A–C setup / HUD leaves (table-driven)
    "kElimHudMenuItems": "kMenuPageElimHud",
    "kTeamHealthMenuItems": "kMenuPageTeamHealth",
    "kKillFeedMenuItems": "kMenuPageKillFeed",
    "kNetGraphHudMenuItems": "kMenuPageNetGraphHud",
    "kMemoryHudMenuItems": "kMenuPageMemoryHud",
    "kAdrenalineHudMenuItems": "kMenuPageAdrenalineHud",
    "kTongueCutHudMenuItems": "kMenuPageTongueCutHud",
    "kTabBoardMenuItems": "kMenuPageTabBoard",
    "kVoiceHudMenuItems": "kMenuPageVoiceHud",
    "kUpgradeAmmoHudMenuItems": "kMenuPageUpgradeAmmoHud",
    "kBwSelfHudMenuItems": "kMenuPageBwSelfHud",
    "kMateAlertMenuItems": "kMenuPageMateLast",
    "kWitchStartleHudMenuItems": "kMenuPageWitchStartleHud",
    "kLaserRoundNoticeMenuItems": "kMenuPageLaserRoundNotice",
    "kTankSpawnNoticeMenuItems": "kMenuPageTankSpawnNotice",
    "kTankRockNoticeMenuItems": "kMenuPageTankRockNotice",
    "kRouteProgressHudMenuItems": "kMenuPageRouteProgressHud",
    "kDirectorPhaseHudMenuItems": "kMenuPageDirectorPhaseHud",
    "kKillStreakHudMenuItems": "kMenuPageKillStreakHud",
    "kSiPresenceHudMenuItems": "kMenuPageSiPresenceHud",
    "kTankTargetHudMenuItems": "kMenuPageTankTargetHud",
    "kDamageSourceHudMenuItems": "kMenuPageDamageSourceHud",
    "kFriendlyFireHudMenuItems": "kMenuPageFriendlyFireHud",
    "kSiAggroHudMenuItems": "kMenuPageSiAggroHud",
    "kWitchRageHudMenuItems": "kMenuPageWitchRageHud",
    "kBossDmgBoardHudMenuItems": "kMenuPageBossDmgBoardHud",
    "kIgniteHudMenuItems": "kMenuPageIgniteHud",
    "kWitchHpHudMenuItems": "kMenuPageWitchHpHud",
    "kInteractHudMenuItems": "kMenuPageInteractHud",
    "kShoveHudMenuItems": "kMenuPageShoveHud",
    "kWorldClockMenuItems": "kMenuPageWorldClock",
    "kSpeedMenuItems": "kMenuPageSpeed",
    "kRoundTimerMenuItems": "kMenuPageRoundTimer",
    "kMapProgressHudMenuItems": "kMenuPageMapProgressHud",
    "kRescueProgressHudMenuItems": "kMenuPageRescueProgressHud",
    "kFinaleHudMenuItems": "kMenuPageFinaleHud",
    "kItemHudMenuItems": "kMenuPageItemHud",
    "kDirectorHudMenuItems": "kMenuPageDirectorHud",
    "kHideVanillaHudMenuItems": "kMenuPageHideVanillaHud",
    "kExtraAmmoHudMenuItems": "kMenuPageExtraAmmoHud",
    "kControlHudMenuItems": "kMenuPageControlHud",
    "kFireGateHudMenuItems": "kMenuPageFireGateHud",
    "kHazardHudMenuItems": "kMenuPageHazardHud",
    "kEmsHudMenuItems": "kMenuPageEmsHud",
    "kCrosshairHubMenuItems": "kMenuPageCrosshairHub",
    "kHelpDebugMenuItems": "kMenuPageHelpDebug",
    "kLocalCharacterHubMenuItems": "kMenuPageLocalCharacterHub",
    "kStaticCrosshairMenuItems": "kMenuPageStaticCrosshair",
    "kRingCrosshairMenuItems": "kMenuPageRingCrosshair",
    "kTextureCrosshairMenuItems": "kMenuPageTextureCrosshair",
    "kBackgroundEngineMenuItems": "kMenuPageBackgroundEngine",
    "kCfgOverrideMenuItems": "kMenuPageCfgOverride",
    "kClientDmgNumMenuItems": "kMenuPageClientDmgNum",
    "kColorCorrectionMenuItems": "kMenuPageColorCorrection",
    "kDspLockMenuItems": "kMenuPageDspLock",
    "kGunFeelMenuItems": "kMenuPageGunFeel",
    "kInfectedHpMenuItems": "kMenuPageInfectedHp",
    "kSilentSiMenuItems": "kMenuPageSilentSi",
    "kStaggerHudMenuItems": "kMenuPageStaggerHud",
    "kAmmoGfxHudMenuItems": "kMenuPageAmmoGfxHud",
    "kThrowNoticeHudMenuItems": "kMenuPageThrowNoticeHud",
    "kBotNamesMenuItems": "kMenuPageBotNames",
    "kSpotlightVisionMenuItems": "kMenuPageSpotlightVision",
}
# Action-only HUD/setup tables: registered for page wiring, but row labels are
# painted dynamically (no UiId/text members). Skip bilingual catalog rows.
CATALOG_EXCLUDE = frozenset({
    "kElimHudMenuItems", "kTeamHealthMenuItems", "kKillFeedMenuItems",
    "kNetGraphHudMenuItems", "kMemoryHudMenuItems", "kAdrenalineHudMenuItems",
    "kTongueCutHudMenuItems", "kTabBoardMenuItems", "kVoiceHudMenuItems",
    "kUpgradeAmmoHudMenuItems", "kBwSelfHudMenuItems", "kMateAlertMenuItems",
    "kWitchStartleHudMenuItems", "kLaserRoundNoticeMenuItems",
    "kTankSpawnNoticeMenuItems", "kTankRockNoticeMenuItems",
    "kRouteProgressHudMenuItems", "kDirectorPhaseHudMenuItems",
    "kKillStreakHudMenuItems", "kSiPresenceHudMenuItems",
    "kTankTargetHudMenuItems", "kDamageSourceHudMenuItems",
    "kFriendlyFireHudMenuItems", "kSiAggroHudMenuItems",
    "kWitchRageHudMenuItems", "kBossDmgBoardHudMenuItems", "kIgniteHudMenuItems",
    "kWitchHpHudMenuItems", "kInteractHudMenuItems", "kShoveHudMenuItems",
    "kWorldClockMenuItems", "kSpeedMenuItems", "kRoundTimerMenuItems",
    "kMapProgressHudMenuItems", "kRescueProgressHudMenuItems",
    "kFinaleHudMenuItems", "kItemHudMenuItems", "kDirectorHudMenuItems",
    "kHideVanillaHudMenuItems", "kExtraAmmoHudMenuItems",
    "kFireGateHudMenuItems", "kHazardHudMenuItems", "kEmsHudMenuItems",
    "kCrosshairHubMenuItems", "kHelpDebugMenuItems",
    "kLocalCharacterHubMenuItems", "kStaticCrosshairMenuItems",
    "kRingCrosshairMenuItems", "kTextureCrosshairMenuItems",
    "kBackgroundEngineMenuItems", "kCfgOverrideMenuItems",
    "kClientDmgNumMenuItems", "kColorCorrectionMenuItems", "kDspLockMenuItems",
    "kGunFeelMenuItems", "kInfectedHpMenuItems", "kSilentSiMenuItems",
    "kStaggerHudMenuItems", "kWorldTankPredictMenuItems", "kAmmoGfxHudMenuItems", "kThrowNoticeHudMenuItems",
    "kBotNamesMenuItems", "kSpotlightVisionMenuItems",
})
ROOT_GROUPS = {
    "kMenuPageHitFeedback": "Visual", "kMenuPageCrosshairHub": "Combat",
    "kMenuPageCustomHudHub": "Hud", "kMenuPageOverlayHud": "Hud",
    "kMenuPageWorldHud": "Hud", "kMenuPageGameUi": "Interface",
    "kMenuPageSkeetoScript": "Combat", "kMenuPageFastLoad": "System",
    "kMenuPageBackgroundEngine": "Interface", "kMenuPageNavigation": "Navigation",
    "kMenuPageSupplyRadar": "Navigation", "kMenuPageViewModel": "Combat",
    "kMenuPageSoundVolume": "Visual", "kMenuPageCombatVisual": "Combat",
    "kMenuPageClientConvenience": "System", "kMenuPageCrashGuard": "System",
    "kMenuPageClearDecals": "System", "kMenuPageCacheOffload": "System",
    "kMenuPageNecolaStartup": "System", "kMenuPageLocalListen": "Local",
    "kMenuPageCareerArchive": "Hud", "kMenuPageMenuStyle": "Interface",
    "kMenuPageHelpDebug": "System", "kMenuPagePluginPlatform": "System",
}


def masked(text):
    return LEX.sub(lambda m: "".join("\n" if c == "\n" else " " for c in m[0]), text)


def end_pair(text, start, opening="{", closing="}"):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == opening:
            depth += 1
        elif text[i] == closing:
            depth -= 1
            if not depth:
                return i + 1
    raise ValueError("Unclosed source construct")


def body(text, marker):
    match = re.search(marker, text, re.M)
    if not match:
        raise ValueError("Missing menu source: " + marker)
    clean = masked(text)
    start = clean.index("{", match.start())
    return text[start + 1:end_pair(clean, start) - 1]


def wide(text):
    return "".join(ast.literal_eval(t[1:]) for t in re.findall(r'L"(?:\\.|[^"\\])*"', text))


def clean_label(text):
    text = re.sub(r'^\s*\[(?:\d+|%d)\]\s*', '', text)
    text = re.sub(r'【[^】]*】', '', text)
    text = re.sub(r'\[[^\]]*%[^\]]*\]', '', text)
    text = re.sub(r'\[(?:Open settings|Open help)\]', '', text, flags=re.I)
    text = re.sub(r'\s+v%[\w]+.*$', '', text)
    # Drop a whole dynamic numeric counter, keeping static qualifiers (Beta/ranges).
    text = re.sub(r'[（(][^（）()]*%(?:[-+ #0\d.*]*)(?:I64|ll|[hlw])?[diuoxXfFeEgGaA][^（）()]*[）)]', '', text)
    text = re.sub(r'%(?:[-+ #0\d.*]*)(?:I64|ll|[hlw])?[diuoxXfFeEgGaAcCsSpn%]', '', text)
    text = re.split(r'[（(]\s*[-+]|\s+(?:`?-`?\s*/\s*`?[+=]|-/\+|\s*Shift|\s*Ctrl)', text)[0]
    text = text.replace('【进入设置】', '').replace('【进入说明】', '')
    return re.sub(r'\s+', ' ', text).strip(' :：|/-%')


def page_visible(fragment, position, page, constants):
    """Reject rows inside another page's nested positive/negative branch."""
    clean = masked(fragment)
    for match in re.finditer(r'\bif\s*\(', clean):
        start = clean.index('(', match.start()); end = end_pair(clean,start,'(',')')
        condition = clean[start+1:end-1].strip()
        compare = re.fullmatch(r'g_menuPage\s*(==|!=)\s*(kMenuPage\w+|\d+)',condition)
        if not compare: continue
        op,value = compare.groups(); value=constants[value] if value in constants else int(value)
        at=end
        while at<len(clean) and clean[at].isspace():at+=1
        finish=end_pair(clean,at) if clean[at]=='{' else clean.find(';',at)+1
        visible=(page==value) if op=='==' else (page!=value)
        if at<=position<finish and not visible:return False
        tail=finish
        while tail<len(clean) and clean[tail].isspace():tail+=1
        if clean.startswith('else',tail):
            branch=tail+4
            while branch<len(clean) and clean[branch].isspace():branch+=1
            if branch<len(clean) and clean[branch]=='{' and branch<=position<end_pair(clean,branch) and visible:return False
    return True


def fields(text):
    clean = masked(text)
    result, start, depth = [], 0, 0
    for i, c in enumerate(clean):
        if c in '({[':
            depth += 1
        elif c in ')}]':
            depth -= 1
        elif c == ',' and depth == 0:
            result.append(text[start:i].strip()); start = i + 1
    if text[start:].strip():
        result.append(text[start:].strip())
    return result


def case_segment(fragment):
    """Stop before the case's terminal or its enclosing switch's closing brace."""
    clean=masked(fragment);depth=0
    for at,c in enumerate(clean):
        if c=='}':
            if depth==0:return fragment[:at]
            depth-=1
        elif c=='{':depth+=1
        elif depth==0 and re.match(r'(?:break|return)\s*;',clean[at:]):return fragment[:at]
    return fragment


def array_rows(source, name):
    value = body(source, r'\b' + name + r'\[\]\s*=\s*\{')
    clean = masked(value)
    rows, at = [], 0
    while at < len(clean):
        start = clean.find('{', at)
        if start < 0:
            break
        end = end_pair(clean, start)
        rows.append(fields(value[start + 1:end - 1])); at = end
    return rows


def page_branches(fragment, constants):
    clean = masked(fragment); depth, depths = 0, []
    for c in clean:
        depths.append(depth)
        if c == '{': depth += 1
        elif c == '}': depth -= 1
    found = []
    for match in re.finditer(r'\bif\s*\(', clean):
        start = clean.index('(', match.start()); end = end_pair(clean, start, '(', ')')
        condition = clean[start + 1:end - 1]
        pages = re.findall(r'g_menuPage\s*==\s*(kMenuPage\w+|\d+)', condition)
        at = end
        while at < len(clean) and clean[at].isspace(): at += 1
        if pages and at < len(clean) and clean[at] == '{':
            finish = end_pair(clean, at)
            values = [constants[p] if p in constants else int(p) for p in pages]
            found.append((depths[match.start()], values, condition, fragment[at + 1:finish - 1]))
    minimum = min((r[0] for r in found), default=0)
    return [r[1:] for r in found if r[0] == minimum]


def subpages(fragment, position, base_condition):
    constraints = [base_condition]
    clean = masked(fragment)
    for match in re.finditer(r'\bif\s*\(', clean):
        start = clean.index('(', match.start()); end = end_pair(clean, start, '(', ')')
        condition = clean[start + 1:end - 1]
        if 'g_menuSub' not in condition:
            continue
        at = end
        while at < len(clean) and clean[at].isspace(): at += 1
        finish = end_pair(clean, at) if at < len(clean) and clean[at] == '{' else clean.find(';', at) + 1
        if at <= position < finish:
            constraints.append(condition)
        tail = finish
        while tail < len(clean) and clean[tail].isspace(): tail += 1
        if clean.startswith('else', tail):
            branch = tail + 4
            while branch < len(clean) and clean[branch].isspace(): branch += 1
            if branch < len(clean) and clean[branch] == '{':
                other_end = end_pair(clean, branch)
                if branch <= position < other_end and re.fullmatch(r'\s*\(?g_menuSub\s*(?:==|!=)\s*\d+\)?\s*', condition):
                    constraints.append('!' + condition)
    allowed = set(range(8)); restricted = False
    for condition in constraints:
        matches = list(re.finditer(r'g_menuSub\s*(==|!=)\s*(\d+)', condition))
        if not matches:
            continue
        # Menu subpage guards use equality. OR branches are a union; outer
        # unrelated runtime flags do not make a static row another subpage.
        selected = set()
        for match in matches:
            op, value = match.groups(); value = int(value)
            selected |= ({value} if op == '==' else set(range(8)) - {value})
        if condition.startswith('!'): selected = set(range(8)) - selected
        allowed &= selected; restricted = True
    return sorted(allowed) if restricted else [0]


def build():
    source = (CORE / 'skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    english = {m[1]: wide(m[2]) for m in re.finditer(r'SKEETO_UI\((\w+),\s*(' + LITERAL + r')\)', (CORE / 'skeeto_ui_en.inc').read_text(encoding='utf-8-sig'))}
    constant_source = source + '\n' + '\n'.join(p.read_text(encoding='utf-8-sig') for p in sorted(CORE.glob('*.h')))
    constants = {m[1]: int(m[2]) for m in re.finditer(r'(?:static\s+)?constexpr int\s+(kMenuPage\w+)\s*=\s*(\d+)', constant_source)
                 if not m[1].startswith('kMenuPageId')}
    def number(value):
        value = value.strip()
        if value in constants: return constants[value]
        if re.fullmatch(r'-?\d+', value): return int(value)
        raise ValueError('Nonconstant menu page: ' + value)
    paint = body(source, r'^static void MenuPaintEngine\(\)\s*\{')
    key = body(source, r'^static void MenuProcessKeyImpl\(int digit\)\s*\{')
    branches = page_branches(paint, constants)
    key_branches = page_branches(key, constants)
    def numbered_target(page, sub, digit):
        targets=set()
        for pages,condition,part in key_branches:
            if page not in pages:continue
            clean=masked(part)
            for match in re.finditer(r'\bif\s*\(\s*digit\s*==\s*'+str(digit)+r'\s*\)',clean):
                at=match.end()
                while at<len(clean) and clean[at].isspace():at+=1
                end=end_pair(clean,at) if clean[at]=='{' else clean.find(';',at)+1
                if sub not in subpages(part,match.start(),condition) or not page_visible(part,match.start(),page,constants):continue
                values=set(re.findall(r'g_menuPage\s*=\s*(kMenuPage\w+|\d+)\s*;',part[at:end]))
                if len(values)==1:targets.add(number(values.pop()))
        return targets.pop() if len(targets)==1 else -1
    page_info, graph, raw = {}, {}, []
    for pages, condition, part in branches:
        calls = list(UI.finditer(part))
        heading = next(((m[1], clean_label(wide(m[2]))) for m in calls if not wide(m[2]).lstrip().startswith('[')), ('Count', ''))
        help_text = next(((m[1], wide(m[2]).strip()) for m in calls if len(wide(m[2]).strip()) > 12 and not wide(m[2]).lstrip().startswith('[') and '%' not in wide(m[2]) and m[1] != heading[0] and not re.search(r'源代码|授权|上一页|下一页',wide(m[2]))), ('Count', ''))
        for page in pages:
            page_info.setdefault(page, {'id': heading[0], 'zh': heading[1], 'en': clean_label(english.get(heading[0], heading[1])), 'help': help_text})
    def add(page, sub, row, ident, zh, hint=None, action='', target=-1, primary=False):
        if not zh or ident == 'Count' or 'Discovery' in ident:
            return
        if page not in page_info:
            page_info[page] = {'id': ident, 'zh': clean_label(zh), 'en': clean_label(english.get(ident,zh)), 'help': ('Count','')}
        name = clean_label(zh)
        if not name:
            return
        en = clean_label(english.get(ident, zh))
        if target >= 0:
            graph.setdefault(page, []).append((target, name, en))
            if target not in page_info or not page_info[target]['zh']:
                page_info[target] = {'id': ident, 'zh': name, 'en': en, 'help': ('Count','')}
        raw.append({'page': page, 'sub': sub, 'row': row, 'id': ident, 'label_zh':name,'label_en':en,'hint':hint,'action':action,'target':target,'primary':primary})
    declarations = {m[2]:m[1] for m in re.finditer(r'static\s+(?:constexpr|const)\s+(\w+)\s+(k\w+)\[\]\s*=\s*\{', source)}
    unmapped={name for name,kind in declarations.items() if kind.endswith('MenuItem') and name not in TABLES}
    if unmapped:raise ValueError('Register new numbered-menu tables: '+', '.join(sorted(unmapped)))
    actions = {}
    action_targets = {}
    key_cases = list(re.finditer(r'\bcase\s+(\w+::\w+)\s*:', key))
    for n, match in enumerate(key_cases):
        segment=case_segment(key[match.end():key_cases[n+1].start() if n+1<len(key_cases) else len(key)])
        # Only named navigation actions are eligible. This never interprets a
        # toggle, reset, preview or editor action as something safe to execute.
        if not re.search(r'::(?:Open|Select).*',match[1]):continue
        targets=set(re.findall(r'g_menuPage\s*=\s*(kMenuPage\w+|\d+)\s*;',segment))
        if len(targets)==1:action_targets[match[1]]=number(targets.pop())
    # A table with action-only rows still gets its text from the real painter.
    cases = list(re.finditer(r'\bcase\s+(\w+::\w+)\s*:', source))
    for n, match in enumerate(cases):
        segment = source[match.end():cases[n+1].start() if n+1<len(cases) else len(source)]
        candidates = list(UI.finditer(segment))
        label = next((m for m in candidates if re.match(r'\[(?:[1-9]|%d)\]', wide(m[2]))), None)
        if label is not None: actions.setdefault(match[1], (label[1], wide(label[2])))
        else:
            assigned=re.search(r'label\s*=\s*UiText\(UiId::(\w+),\s*('+LITERAL+r')\)',segment)
            if assigned:actions.setdefault(match[1],(assigned[1],wide(assigned[2])))
            elif re.search(r'L"\[[1-9]\] %s %s"',segment) and candidates:
                actions.setdefault(match[1],(candidates[0][1],wide(candidates[0][2])))
    # This action uses the painter's default label before its switch.
    default_label=re.search(r'label\s*=\s*UiText\(UiId::(RadarFilterEntry),\s*('+LITERAL+r')\)',source)
    if not default_label:raise ValueError('Radar filter default label moved; update its catalog adapter')
    actions['RadarMenuAction::OpenItemFilter']=(default_label[1],wide(default_label[2]))
    for table, page_name in TABLES.items():
        if table not in declarations:
            raise ValueError('Missing registered table: ' + table)
        if table in CATALOG_EXCLUDE:
            continue
        page = number(page_name)
        definition = body(source, r'^struct\s+'+declarations[table]+r'\s*\{')
        member_names = [re.findall(r'(\w+)\s*$', item.strip())[0] for item in definition.split(';') if item.strip()]
        rows = array_rows(source, table)
        for index, values in enumerate(rows):
            item = dict(zip(member_names, values))
            action = item.get('action','')
            if action.endswith('::PrintStatus'):continue
            title_pair = None
            for a,b in zip(member_names,member_names[1:]):
                if item.get(a,'').startswith('UiId::') and item.get(b,'').startswith('L"'):
                    title_pair = (item[a][6:],wide(item[b])); break
            if table == 'kRadarMainMenuItems' and int(item['settingIndex']) >= 0:
                labels = array_rows(source,'kRadarUiLabels'); label = labels[int(item['settingIndex'])]
                title_pair=(label[0][6:],wide(label[1]))
            if title_pair is None: title_pair = actions.get(action)
            if title_pair is None:
                if action.endswith('::PrintStatus'):continue  # Internal diagnostic action.
                raise ValueError('Missing action label: '+table+' '+action)
            ident,zh = title_pair
            if table == 'kOverlayReloadEditItems' and action.endswith('::CycleColor'):
                label = next(m for m in UI.finditer(source) if m[1]=='OverlayProgressColor')
                ident,zh=label[1],wide(label[2])
            if table == 'kMenuRootItems' and 'OpenSearch' in action: continue
            hint = None
            for a,b in zip(member_names,member_names[1:]):
                if ('hint' in a.lower() or 'desc' in a.lower()) and item.get(a,'').startswith('UiId::') and item.get(b,'').startswith('L"'):
                    hint=(item[a][6:],wide(item[b]));break
            target = item.get('targetPage',item.get('page','-1'))
            try: target = number(target)
            except ValueError: target = -1
            if target < 0: target=action_targets.get(action,-1)
            sub,row = divmod(index,9);row += 1
            if table == 'kSoundVolumeMenuItems':
                if index < 7: sub,row=0,index+3
                else:
                    rest=index-7
                    sub,row=1+rest//9,rest%9+1
            if table == 'kSoundVolumeAuxMenuItems':
                content=len(array_rows(source,'kSoundVolumeMenuItems'))+index
                if content < 7: sub,row=0,content+3
                else:
                    rest=content-7
                    sub,row=1+rest//9,rest%9+1
            # Extra listen rows continue after the first full page of main items.
            if table == 'kLocalListenExtraMenuItems':
                content=len(array_rows(source,'kLocalListenMenuItems'))+index
                sub,row=divmod(content,9);row += 1
            add(page,sub,row,ident,zh,hint,action,target,True)
    # Native chat and console share one target-filtered source table.
    native = (CORE/'skeeto_nativeui_menu.h').read_text(encoding='utf-8-sig')
    for target,page_name in enumerate(('kMenuPageChatUi','kMenuPageConsoleUi')):
        index=0
        for item in array_rows(native,'kNativeUiMenuItems'):
            if int(item[3]) & (1<<target):
                sub,row=divmod(index,9);add(number(page_name),sub,row+1,item[0][6:],wide(item[1]),primary=True);index+=1
    # Navigation setting IDs are the same indices used by drawing and input.
    nav_labels=array_rows(source,'kNavUiLabels')
    display=re.search(r'kNavDisplaySettings\[\]\s*=\s*\{([^}]+)',source)[1]
    nav = [('kMenuPageNavDisplay',i//9,i%9+1,int(value.strip())) for i,value in enumerate(display.split(','))]
    nav += [('kMenuPageNavRoute',0,row,index) for row,index in ((1,8),(2,9),(3,44),(5,47),(6,51))]
    ranges={m[1]:int(m[2]) for m in re.finditer(r'constexpr int (kNav\w+Setting)\s*=\s*(\d+)',source)}
    for prefix in ('Supplies','Weapons'):
        begin=ranges['kNav'+prefix+'FirstSetting'];end=ranges['kNav'+prefix+'EndSetting']
        nav += [('kMenuPageNav'+prefix,(i-begin)//9,(i-begin)%9+1,i) for i in range(begin,end)]
    for page_name,sub,row,index in nav:
        if index>=0:
            item=nav_labels[index];add(number(page_name),sub,row,item[0][6:],wide(item[1]),primary=True)
    # These three non-setting rows use compound page/sub/row guards, unlike the
    # static page renderer. Read the guard and displayed key together rather
    # than spreading their text over every page in the shared renderer.
    for ident in ('NavEdit','NavReload','NavWorldEdit'):
        match=next(m for m in UI.finditer(paint) if m[1]==ident)
        guards=list(re.finditer(r'if\s*\((g_menuPage\s*==\s*kMenuPageNav\w+[^)]*)\)',masked(paint[:match.start()])))
        condition=guards[-1][1]
        page=number(re.search(r'g_menuPage\s*==\s*(\w+)',condition)[1])
        sub_match=re.search(r'g_menuSub\s*==\s*(\d+)',condition)
        sub=int(sub_match[1]) if sub_match else 0
        digit=int(re.match(r'\[(\d+)\]',wide(match[2]))[1])
        if not re.search(r'row\s*==\s*'+str(digit-1)+r'\b',condition):raise ValueError('Navigation action key/guard mismatch: '+ident)
        add(page,sub,digit,ident,wide(match[2]),primary=True)
    radar_source=(CORE/'skeeto_navigation/supply_radar/runtime.h').read_text(encoding='utf-8-sig')
    # The real UiTextList is ordered by item label.
    radar_names=re.findall(r'\{UiId::(RadarItem\w+),\s*('+LITERAL+r')\}',body(radar_source,r'^static const WCHAR\* ItemName[^\n]*\{'))
    if len(radar_names)!=39:raise ValueError('Radar item labels changed; review filter adapter')
    for label,(ident,text) in enumerate(radar_names[1:],1):
        sub,row=divmod(label-1,9)
        add(number('kMenuPageSupplyRadarItems'),sub,row+1,ident,wide(text),primary=True)
    occupied={(r['page'],r['sub'],r['row']) for r in raw}
    for pages,condition,part in branches:
        if 'OverlayHudEditPage(g_menuPage)' in condition:continue # Fully described by the three edit tables.
        for match in UI.finditer(part):
            zh=wide(match[2]);digit=re.match(r'^\s*\[([1-9])\]',zh)
            number_key=int(digit[1]) if digit else None
            if number_key is None and zh.startswith('[%d]'):
                # A literal swprintf key is a static row too (TAB panel style).
                # Dynamic row+1 values are already supplied by registered tables.
                argument=re.match(r'\s*\)\s*,\s*([1-9])\s*[,)]',part[match.end():])
                if argument:number_key=int(argument[1])
            if number_key is None: continue
            for page in pages:
                if page in {constants.get(k) for k in ('kMenuPageSafeMusicList','kMenuPageNavigation','kMenuPageNavDisplay','kMenuPageNavRoute','kMenuPageNavSupplies','kMenuPageNavWeapons')} or not page_visible(part,match.start(),page,constants):continue
                for sub in subpages(part,match.start(),condition):
                    slot=(page,sub,number_key)
                    if slot not in occupied:
                        target=numbered_target(*slot)
                        add(*slot,match[1],zh,target=target);occupied.add(slot)
    for pages,_,part in page_branches(key,constants):
        for value in re.findall(r'g_menuPage\s*=\s*(kMenuPage\w+|\d+)\s*;',part):
            target=number(value)
            if target>0 and target in page_info:
                for page in pages:
                    if page!=target:graph.setdefault(page,[]).append((target,page_info[target]['zh'],page_info[target]['en']))
    paths={0:([],[], 'System')};pending=[0]
    root_groups={number(k):v for k,v in ROOT_GROUPS.items() if k in constants}
    while pending:
        parent=pending.pop(0)
        for target,zh,en in graph.get(parent,[]):
            if target in paths:continue
            old=paths[parent];group=root_groups.get(target,old[2])
            paths[target]=(old[0]+[zh],old[1]+[en],group);pending.append(target)
    summaries={}
    for index,values in enumerate(array_rows((CORE/'skeeto_menu_discovery_descriptions.h').read_text(encoding='utf-8-sig'),'kMenuDiscoverySummaries')):
        match=re.search(r'UiId::(\w+),\s*('+LITERAL+r')',values[1])
        summaries[number(values[0])]=(match[1],wide(match[2]),index)
    result=[];seen=set()
    for r in raw:
        if r['page']==constants.get('kMenuPageBotNameEdit') or r['target']==constants.get('kMenuPageBotNameEdit'):continue  # Needs a chosen character.
        if re.search(r'Diagnostic|Diagnostics|PrintStatus',r['id']):continue
        page=r['page'];info=page_info[page]
        path=paths.get(page,([info['zh']],[info['en']],root_groups.get(page,'System')))
        context=info['zh'];context_en=info['en']
        name,en=r.pop('label_zh'),r.pop('label_en');label=name
        if label in ('总开关','启用','自定义样式') and context and context not in name:
            name=context+' / '+name;en=context_en+' / '+en
        hint=r.pop('hint');summary_page=r['target'] if r['target']>=0 else page
        summary=summaries.get(summary_page);description_ref=None
        if not hint and summary:
            hint=summary[:2];description_ref=f'kMenuDiscoverySummaries[{summary[2]}].text.chinese'
        if not hint:hint=page_info.get(summary_page,info)['help']
        description=clean_label(hint[1]) if hint[1] else context or name
        description_en=clean_label(english.get(hint[0],hint[1])) if hint[1] else context_en or en
        group=path[2] if page else root_groups.get(r['target'],'System')
        aliases=[]
        if '镜像' in name:aliases+=['翻转','mirror','flip']
        if '持枪' in name+context and ('位置' in name or '视角' in name):aliases+=['枪的位置','枪模','viewmodel']
        if '准星' in name:aliases+=['crosshair']
        if '雷达' in name:aliases+=['radar','找东西']
        if '命中反馈' in name:aliases+=['hitmarker','hit feedback']
        target_page=r['page'];target_sub=r['sub'];target_row=r['row']
        if r['target']>=0:
            target_page,target_sub,target_row=r['target'],0,0
            path=paths.get(target_page,(path[0]+[label],path[1]+[r['label_en'] if 'label_en' in r else en],group))
        identity=(target_page,target_sub,target_row,r['id'])
        if identity in seen:continue
        seen.add(identity)
        result.append({'id':r['id'],'zh':name,'en':en,'description_zh':description,'description_en':description_en,
                       'path_zh':' > '.join(path[0]) or 'Skeeto','path_en':' > '.join(path[1]) or 'Skeeto',
                       'aliases':' '.join(aliases),'page':target_page,'sub':target_sub,'row':target_row,'group':group,
                       'description_ref':description_ref,
                       'local':group=='Local','experimental':bool(re.search(r'实验|试验|Beta',name+' '.join(path[0]))) or page==constants.get('kMenuPageFastLoadExperiments')})
    return result


def render(rows):
    def literal(s):return 'L'+json.dumps(s,ensure_ascii=False)
    output=['// Generated by tools/generate_menu_discovery.py. Do not edit by hand.',
            '// Names/help/coordinates come from current menu source. No runtime source scan.',
            'static constexpr skmenufind::Entry kMenuDiscoveryEntries[] = {']
    for r in rows:
        strings=[r[k] for k in ('zh','en','description_zh','description_en','path_zh','path_en','aliases')]
        rendered=list(map(literal,strings))
        if r['description_ref']:rendered[2]=r['description_ref']
        fields=', '.join(rendered)
        flags=', '.join('true' if r[k] else 'false' for k in ('local','experimental'))
        output.append(f'    {{{fields}, {{{r["page"]}, {r["sub"]}, {r["row"]}}}, {flags}}}, // {r["id"]}')
    output.append('};\n');return '\n'.join(output)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--report',action='store_true');args=parser.parse_args()
    rows=build();text=render(rows)
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8')!=text:raise SystemExit('Menu discovery catalog is stale; run tools/generate_menu_discovery.py')
    elif not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8')!=text:OUTPUT.write_text(text,encoding='utf-8',newline='\n')
    print(f'Menu catalog: {len(rows)} entries, {len({r["page"] for r in rows})} destination pages')
    if args.report:
        for r in rows:print(r['page'],r['sub'],r['row'],r['id'],r['zh'])


if __name__=='__main__':main()
