from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROXY = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
LAYOUT = (ROOT / "src/skeeto/core/skeeto_mainui_layout.h").read_text(encoding="utf-8")
EDITOR = (ROOT / "src/skeeto/core/skeeto_mainui_editor.h").read_text(encoding="utf-8")


def table_body(source: str, marker: str) -> str:
    assert source.count(marker) == 1, f"expected one table declaration: {marker}"
    return source.split(marker, 1)[1].split("\n};", 1)[0]


def initializer_count(body: str) -> int:
    return len(re.findall(r"^\s*\{UiId::", body, re.MULTILINE))


def braced_body(source: str, start: int) -> str:
    opening = source.find("{", start)
    assert opening >= 0, "expected a C++ block"
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1:index]
    raise AssertionError("unterminated C++ block")


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    assert start >= 0, f"missing function: {signature}"
    return braced_body(source, start)


root_items = table_body(PROXY, "static constexpr MenuRootItem kMenuRootItems[] = {")
setup_items = table_body(PROXY, "static constexpr MainUiSetupItem kMainUiSetupItems[] = {")
combat_items = table_body(PROXY, "static constexpr CombatVisualMenuItem kCombatVisualMenuItems[] = {")
nav_items = table_body(PROXY, "static constexpr NavCategoryItem kNavCategoryItems[] = {")
world_items = table_body(PROXY, "static constexpr WorldHudMenuItem kWorldHudMenu[] = {")
sound_items = table_body(PROXY, "static constexpr SoundVolumeMenuItem kSoundVolumeMenuItems[] = {")
hit_feedback_items = table_body(PROXY, "static constexpr HitFeedbackMenuItem kHitFeedbackMenuItems[] = {")
script_items = table_body(PROXY, "static constexpr SkeetoScriptMenuItem kSkeetoScriptMenuItems[] = {")
style_items = table_body(PROXY, "static constexpr MenuStyleItem kMenuStyleItems[] = {")
local_debug_items = table_body(PROXY, "static constexpr LocalDebugMenuItem kLocalDebugMenuItems[] = {")
survivor_glow_items = table_body(PROXY, "static constexpr SurvivorGlowMenuItem kSurvivorGlowMenuItems[] = {")
radar_main_items = table_body(PROXY, "static constexpr RadarMainMenuItem kRadarMainMenuItems[] = {")
sound_aux_items = table_body(PROXY, "static constexpr SoundVolumeAuxMenuItem kSoundVolumeAuxMenuItems[] = {")
career_sections = table_body(PROXY, "static constexpr CareerArchiveSection kCareerArchiveSections[] = {")
convenience_items = table_body(PROXY, "static constexpr ClientConvenienceMenuItem kClientConvenienceMenuItems[] = {")
force_cvar_items = table_body(PROXY, "static constexpr ForceCvarMenuItem kForceCvarMenuItems[] = {")
crash_guard_items = table_body(PROXY, "static constexpr CrashGuardMenuItem kCrashGuardMenuItems[] = {")
overlay_root_items = table_body(PROXY, "static constexpr OverlayHudRootMenuItem kOverlayHudRootMenuItems[] = {")
local_listen_items = table_body(PROXY, "static constexpr LocalListenMenuItem kLocalListenMenuItems[] = {")
last_music_items = table_body(PROXY, "static constexpr LastMusicMenuItem kLastMusicMenuItems[] = {")
safe_music_items = table_body(PROXY, "static constexpr SafeMusicMenuItem kSafeMusicMenuItems[] = {")
si_warning_items = table_body(PROXY, "static constexpr SiWarningMenuItem kSiWarningMenuItems[] = {")
world_names_items = table_body(PROXY, "static constexpr WorldNamesMenuItem kWorldNamesMenuItems[] = {")
chap_stats_items = table_body(PROXY, "static constexpr ChapStatsMenuItem kChapStatsMenuItems[] = {")
chap_stats_size_items = table_body(PROXY, "static constexpr ChapStatsSizeMenuItem kChapStatsSizeMenuItems[] = {")
chap_stats_display_items = table_body(PROXY, "static constexpr ChapStatsDisplayMenuItem kChapStatsDisplayMenuItems[] = {")
chap_stats_score_items = table_body(PROXY, "static constexpr ChapStatsScoreHudMenuItem kChapStatsScoreHudMenuItems[] = {")
overlay_ammo_items = table_body(PROXY, "static constexpr OverlayHudEditMenuItem kOverlayAmmoEditItems[] = {")
overlay_reload_items = table_body(PROXY, "static constexpr OverlayHudEditMenuItem kOverlayReloadEditItems[] = {")
overlay_fire_items = table_body(PROXY, "static constexpr OverlayHudEditMenuItem kOverlayFireEditItems[] = {")

assert initializer_count(root_items) == 24
root_ids = re.findall(r'\{UiId::(\w+),', root_items)
assert root_ids[0] == "FindRootSearch"
assert "FindRootOverview" not in root_ids
assert root_ids[6] == "GameUiRootEntry"
assert root_ids[8:10] == ["FastLoadEntry", "MainBackgroundEntry"]
assert root_ids[10] == "NavRoot", "navigation stays on the second root page"
# Root rendering renumbers an existing prefix; absent prefixes lose the key hint.
english = (ROOT / "src/skeeto/core/skeeto_ui_en.inc").read_text(encoding="utf-8")
for ui_id, chinese in re.findall(r'\{UiId::(\w+), L"([^"\n]*)"', root_items):
    assert re.match(r"\[[1-9]\] ", chinese), f"missing root key prefix: {ui_id} (zh)"
    localized = re.search(r'SKEETO_UI\(' + ui_id + r', L"([^"\n]*)"', english)
    assert localized and re.match(r"\[[1-9]\] ", localized[1]), f"missing root key prefix: {ui_id} (en)"
assert "MenuRootPageCount()" in PROXY and "MenuRootItemAt(g_menuSub, digit)" in PROXY
assert PROXY.count("MenuRootItemAt(") >= 3, "root paint and keyboard input must share the ordered table"
assert "MenuRenumberItem(text, _countof(text), row + 1)" in PROXY

assert initializer_count(setup_items) == 5
assert PROXY.count("kMainUiSetupItems[") >= 3, "main-menu setup paint and input must share the ordered table"
assert "MainUiSetupItemCount() <= kMenuRootItemsPerPage" in PROXY

assert initializer_count(combat_items) == 12
assert "CombatVisualMenuAction::OpenInteractScan" in combat_items
assert "CombatVisualMenuAction::OpenInteractScan, kMenuPageInteractScan" in combat_items
assert "CombatVisualMenuAction::OpenGunFeel, kMenuPageGunFeel" in combat_items
assert "CombatVisualMenuItemAt(g_menuSub, digit)" in PROXY
assert PROXY.count("CombatVisualMenuItemAt(") >= 2, "combat paint and input must share subpage lookup"
assert PROXY.count("kCombatVisualMenuItems[") >= 3, "combat menu paint and input must share the ordered table"
assert "CombatVisualMenuItemsValid()" in PROXY

assert initializer_count(nav_items) == 4
assert PROXY.count("kNavCategoryItems[") >= 3, "navigation root paint and input must share the ordered table"
assert "NavCategoryItemsUnique()" in PROXY

assert initializer_count(world_items) == 24
assert "WorldHudMenuPageCount()" in PROXY and "WorldHudMenuItemAt(g_menuSub, row + 1)" in PROXY
assert "WorldHudMenuItemAt(g_menuSub, digit)" in PROXY, "world HUD paint and input must share row lookup"
assert "WorldHudMenuItemEnabled(item->action)" in PROXY

throw_items = table_body(PROXY, "static constexpr ThrowLandMenuItem kThrowLandMenuItems[] = {")
assert re.findall(r"ThrowLandMenuAction::(\w+)", throw_items) == ["ToggleEnabled", "CycleStyle", "CycleRefreshRate", "CycleShowMode"]
assert "WorldHudMenuAction::OpenPage, kMenuPageThrowLand" in world_items
assert "ThrowLand" not in combat_items, "throw preview belongs only to world HUD"
throw_paint = function_body(PROXY, "static void MenuPaintEngine() {")
throw_keys = function_body(PROXY, "static void MenuProcessKeyImpl(int digit) {")
for action in ("ToggleEnabled", "CycleStyle", "CycleRefreshRate", "CycleShowMode"):
    assert f"case ThrowLandMenuAction::{action}:" in throw_paint
    assert f"case ThrowLandMenuAction::{action}:" in throw_keys
assert "SinglePageMenuItemAt(kThrowLandMenuItems, g_menuSub, digit)" in throw_keys
assert "g_menuSub = ThrowLandParentSubpage();" in throw_keys, "return preserves the world HUD entry page"
root_targets = re.findall(r"MenuRootAction::OpenPage, (kMenuPage\w+)", root_items)
necola_index = root_targets.index("kMenuPageNecolaStartup")
assert root_targets[necola_index-1:necola_index+2] == ["kMenuPageCrashGuard", "kMenuPageNecolaStartup", "kMenuPageLocalListen"]
necola_items = table_body(PROXY, "static constexpr NecolaStartupMenuItem kNecolaStartupMenuItems[] = {")
assert re.findall(r"NecolaStartupMenuAction::(\w+)", necola_items) == ["ToggleAutoload"]
assert "case NecolaStartupMenuAction::ToggleAutoload:" in throw_paint
assert "case NecolaStartupMenuAction::ToggleAutoload:" in throw_keys
assert "SinglePageMenuItemAt(kNecolaStartupMenuItems, g_menuSub, digit)" in throw_keys
assert "const COLORREF yellow = RGB(255, 220, 80);" in throw_paint
assert "static bool g_optNecolaAutoload = true;" in PROXY
assert "g_optNecolaAutoload = true;" in function_body(PROXY, "static void LoadSettings() {")
world_action_body = table_body(PROXY, "enum class WorldHudMenuAction : unsigned char {")
world_actions = re.findall(r"^\s*(\w+),?\s*$", world_action_body, re.MULTILINE)
world_enabled = PROXY.split("static bool WorldHudMenuItemEnabled(", 1)[1].split("\n}", 1)[0]
world_key_start = PROXY.index("if (g_menuPage == kMenuPageWorldHud) {", PROXY.index("static void MenuProcessKeyImpl(int digit)"))
world_key = PROXY[world_key_start:PROXY.index("if (g_menuPage == kMenuPageStaggerHud) {", world_key_start)]
assert world_actions and world_actions[-1] == "ToggleFriendlyFireDamage"
for action in world_actions:
    assert f"WorldHudMenuAction::{action}" in world_enabled or action == "OpenPage"
    assert f"WorldHudMenuAction::{action}" in world_key

sound_entries = re.findall(r"^\s*\{\d+, UiId::SoundVolume", sound_items, re.MULTILINE)
assert len(sound_entries) == 17
sound_indices = list(map(int, re.findall(r"^\s*\{(\d+), UiId::SoundVolume", sound_items, re.MULTILINE)))
assert sound_indices == list(range(17)), "sound volume rows must remain aligned with setting indices"
assert "SoundVolumeMenuItemsValid()" in PROXY
assert "SoundVolumeMenuItemAt(g_menuSub, row)" in PROXY
assert "SoundVolumeMenuItemAt(g_menuSub, digit - 1)" in PROXY, "sound volume paint and input must share row lookup"
assert PROXY.count("SafeMusicTrackIndexAt(") >= 3, "dynamic music list paint and input must share bounded row lookup"

# Formatting uses fixed printf arguments, so these descriptors must not drift
# away from the row actions paired with those arguments in the painter.
format_sensitive_tables = [
    ("MenuStyleMenuItemsValid", "kMenuStyleItems", "MenuStyleAction", [
        "SetLanguage", "CycleSize", "CycleAlignX", "CycleAlignY", "CycleBackgroundAlpha",
        "CycleBackground", "CycleTextColor", "CycleTitleColor", "CycleFont",
        "ToggleForceBind", "ToggleOutline", "ToggleKbListSection",
    ]),
    ("HitFeedbackMenuItemsValid", "kHitFeedbackMenuItems", "HitFeedbackMenuAction", [
        "ToggleParticlePlatform", "ToggleSound", "ToggleIcon", "CycleStreakWindow",
        "CycleCommonStyle", "CycleSpecialStyle", "ToggleSpecialVisual", "ToggleSpecialSound",
        "ToggleSpecialKill", "CycleHitMode", "CycleBossHitTrigger", "ToggleBloodSplat",
    ]),
    ("SkeetoScriptMenuItemsValid", "kSkeetoScriptMenuItems", "SkeetoScriptMenuAction", [
        "ToggleLowHealthCue", "ToggleHealFade", "ToggleEmptyMagSound", "ToggleLowAmmoSound",
        "ToggleShellImpact", "ToggleClankReload", "OpenHoldSemi", "OpenShotgunReload",
        "OpenViewRoll", "OpenDissolve", "OpenAcidOutline", "OpenTongueCut",
        "OpenLastMusic", "OpenTpsAim",
    ]),
]
for validator, table, action_type, expected_actions in format_sensitive_tables:
    assert f"static_assert({validator}()" in PROXY, f"{table} needs a compile-time action-order check"
    valid_body = function_body(PROXY, f"static constexpr bool {validator}()")
    for index, action in enumerate(expected_actions):
        assert f"{table}[{index}].action == {action_type}::{action}" in valid_body, (
            f"{table}[{index}] must remain {action} to match its formatter arguments"
        )

survivor_valid = function_body(PROXY, "static constexpr bool SurvivorGlowMenuItemsValid()")
assert "kSurvivorGlowMenuItems[9].action != SurvivorGlowMenuAction::SelectBrightness" in survivor_valid
assert "kSurvivorGlowMenuItems[9].valueIndex != 0" in survivor_valid
assert "kSurvivorGlowMenuItems[10].action != SurvivorGlowMenuAction::SelectBlur" in survivor_valid
assert "kSurvivorGlowMenuItems[10].valueIndex != 1" in survivor_valid

# Keep legacy click feedback for unused digits on the radar's short final page.
menu_key_start = PROXY.index("static void MenuProcessKeyImpl(int digit) {")
radar_key_start = PROXY.index("if (RadarMenuPage(g_menuPage)) {", menu_key_start)
radar_key_body = braced_body(PROXY, radar_key_start)
assert re.search(
    r"if \(!item\)\s*\{\s*EngClientCmd\(\"play buttons/button14\"\);\s*return;\s*\}",
    radar_key_body,
), "unused digits on the last radar page must keep their button feedback"

# Derive visible selections and auxiliary adjustment steps from the same rows.
sound_adjust_body = function_body(PROXY, "static void MenuAdjustSoundVolume(int delta) {")
assert "SoundVolumeSelectionVisible(g_menuSub, g_soundVolumeMenuSel)" in sound_adjust_body
aux_adjust_body = function_body(PROXY, "static void MenuAdjustAudioMixerAux(int delta) {")
assert "SoundVolumeAuxItemForSelection(g_soundVolumeAuxSel)" in aux_adjust_body
assert "g_soundVolumeAuxSel == 3" not in aux_adjust_body
assert "g_soundVolumeAuxSel == 2" not in aux_adjust_body
assert PROXY.count("SoundVolumeAuxSelectedStep(g_soundVolumeAuxSel)") >= 1

# Every action-based renderer has a fallback arm, preventing an unhandled row
# from reusing the previous row's text in MenuPaintEngine.
paint_start = PROXY.index("static void MenuPaintEngine() {")
menu_paint = braced_body(PROXY, paint_start)
action_switches = list(re.finditer(r"switch\s*\(([^)]*action)[^)]*\)\s*\{", menu_paint))
assert action_switches, "expected action-based menu renderers"
for switch in action_switches:
    body = braced_body(menu_paint, switch.start())
    if 'CampaignOutlineMenuAction::' in body:
        for action in ('Car','Witch','TankBody','CarColor','WitchColor','TankBodyColor','Range'):
            assert f'case CampaignOutlineMenuAction::{action}:' in body
        assert 'const wchar_t* value = L""' in menu_paint, 'fresh text state for each outline row'
    else:
        assert re.search(r"\bdefault\s*:", body), f"renderer lacks a fallback: {switch.group(1)}"


# Hit feedback had been the renderer/input fallthrough for every otherwise
# unhandled page. Keep its rows explicit and use the same table for both paths.
assert initializer_count(hit_feedback_items) == 16
assert "HitFeedbackMenuPageCount()" in PROXY
assert "HitFeedbackMenuLabel(0, 1" in PROXY and "HitFeedbackMenuLabel(1, 4" in PROXY
assert "HitFeedbackMenuLabel(1, 6" in PROXY
assert "HitFeedbackMenuLabel(1, 7" in PROXY
assert "HitFeedbackMenuItemAt(g_menuSub, digit)" in PROXY
assert "if (g_menuPage != kMenuPageHitFeedback) return false;" in PROXY
assert "if (g_menuSub == 1)" in PROXY.split("menuPaintHitFeedback = [&]() -> bool", 1)[1][:400]
assert "if (g_menuPage != kMenuPageHitFeedback) return;" in PROXY

script_item_count = initializer_count(script_items)
assert script_item_count == 25
assert "SkeetoScriptMenuPageCount()" in PROXY
assert "SkeetoScriptMenuLabel(0, 1" in PROXY and "SkeetoScriptMenuLabel(1, 6" in PROXY
assert "SkeetoScriptMenuLabel(1, 7" in PROXY
assert "SkeetoScriptMenuItemAt(g_menuSub, digit)" in PROXY
assert "SkeetoScriptMenuAction::OpenCampaignCompletion, kMenuPageRouteProgressHud" in script_items
assert "item->action == SkeetoScriptMenuAction::OpenCampaignCompletion" in PROXY
assert "SkeetoScriptMenuAction::OpenBackItems, kMenuPageBackItems" in script_items
assert "SkeetoScriptMenuAction::OpenAlarmCarGlow, kMenuPageAlarmCarGlow" in script_items
assert "SkeetoScriptMenuAction::OpenClientPerf, kMenuPageClientPerf" in script_items
assert "item->action == SkeetoScriptMenuAction::OpenBackItems" in PROXY
assert "SkeetoScriptMenuAction::OpenParry, kMenuPageParryFlash" in script_items
assert "OpenGunFeel" not in script_items
assert "SkeetoScriptMenuAction::ToggleIncapRoll" in script_items
assert "item->action == SkeetoScriptMenuAction::OpenParry" in PROXY
assert "ParryFlashMenuItemAt(g_menuSub, digit)" in PROXY

assert initializer_count(style_items) == 12
assert "MenuStylePageCount()" in PROXY
assert "MenuStyleMenuLabel(0, 1" in PROXY and "MenuStyleMenuLabel(1, 3" in PROXY
assert "MenuStyleItemAt(g_menuSub, digit)" in PROXY

assert initializer_count(local_debug_items) == 12
assert "LocalDebugMenuPageCount()" in PROXY
assert "LocalDebugMenuItemAt(g_menuSub, row)" in PROXY
assert "LocalDebugMenuItemAt(g_menuSub, digit)" in PROXY

assert initializer_count(survivor_glow_items) == 18
assert "SurvivorGlowMenuPageCount()" in PROXY
assert "SurvivorGlowMenuItemAt(g_menuSub, digit)" in PROXY
assert "SurvivorGlowMenuCharacterPage(g_menuSub)" in PROXY, "paint, adjustment, and page hints must use the survivor page descriptor"
paint_start = PROXY.index("static void MenuPaintEngine()")
# Pages B uses sibling ifs (not else-if) after the Pages A ladder.
survivor_paint_start = PROXY.index("if (g_menuPage == kMenuPageSurvivorGlow)", paint_start)
survivor_paint_end = PROXY.index("if (g_menuPage == kMenuPageWorldClock)", survivor_paint_start)
survivor_paint = PROXY[survivor_paint_start:survivor_paint_end]
assert "const int pageCount = SurvivorGlowMenuPageCount();" in survivor_paint
local_debug_paint_start = PROXY.index("} else if (g_menuPage == kMenuPageLocalDebugCvars)", paint_start)
local_debug_paint_end = PROXY.index("} else if (g_menuPage == kMenuPageLocalCharacterHub)", local_debug_paint_start)
local_debug_paint = PROXY[local_debug_paint_start:local_debug_paint_end]
assert "const int pageCount = LocalDebugMenuPageCount();" in local_debug_paint
assert "SurvivorGlowMenuPageCount()" not in local_debug_paint

# The remaining multi-page menus keep their page count and row routing tied to
# the same ordered descriptors/ranges used by paint and input.
radar_main_actions = re.findall(r"RadarMenuAction::(\w+)", radar_main_items)
assert len(radar_main_actions) == 20, "radar main page must describe 8+1, 7+2, and 2 rows"
radar_main_rows = [
    (int(setting), action)
    for setting, action in re.findall(r"\{(-?\d+), RadarMenuAction::(\w+)\}", radar_main_items)
]
assert radar_main_rows == (
    [(i, "None") for i in range(8)]
    + [(-1, "OpenItemFilter")]
    + [(i, "None") for i in range(8, 15)]
    + [(-1, "OpenLayout"), (-1, "ResetLayout"), (-1, "ResetAll"), (-1, "PrintStatus")]
), f"radar main-page ordering or action targets changed: {radar_main_rows}"
assert "RadarMainMenuPageCount()" in PROXY
assert "RadarMainMenuItemAt(g_menuSub, row)" in PROXY
assert "RadarMainMenuItemAt(g_menuSub, digit - 1)" in PROXY
radar_validator = function_body(PROXY, "static constexpr bool RadarMainMenuItemsValid()")
for action_index in (8, 16, 17, 18, 19):
    assert f"kRadarMainMenuItems[{action_index}].settingIndex != -1" in radar_validator, (
        f"radar action row {action_index + 1} must keep the -1 setting sentinel"
    )
assert "MenuPageCountForItems(RadarMenuItemCount(false))" in PROXY
assert "MenuPageCountForItems(RadarMenuItemCount(true))" in PROXY

assert "SoundVolumeMenuPageCount()" in PROXY and "SoundVolumeContentIndex(" in PROXY
assert "kSoundVolumePages" not in PROXY
assert len(re.findall(r"^\s*\{SoundVolumeAuxAction::", sound_aux_items, re.MULTILINE)) == 5
sound_aux_rows = re.findall(
    r"^\s*\{SoundVolumeAuxAction::(\w+), (-?\d+),", sound_aux_items, re.MULTILINE
)
assert sound_aux_rows == [
    ("OpenDspLock", "-1"),
    ("AdjustHitFeedback", "0"),
    ("AdjustBackground", "1"),
    ("AdjustSafeMusic", "2"),
    ("AdjustLastMusic", "3"),
], "sound auxiliary order or selection indexes changed"
assert "SoundVolumeAuxMenuItemAt(g_menuSub, row)" in PROXY
assert "SoundVolumeAuxMenuItemAt(g_menuSub, digit - 1)" in PROXY
assert "SoundVolumeAuxSelectionVisible(g_menuSub, g_soundVolumeAuxSel)" in PROXY

assert "CareerArchiveSectionCount()" in PROXY
assert len(re.findall(r"CareerArchiveSection::", career_sections)) == 4
assert re.findall(r"CareerArchiveSection::(\w+)", career_sections) == [
    "Overview", "Team", "Weapons", "SurvivorsAndModes"
]
assert "CareerArchiveSectionAt(g_menuSub)" in PROXY
assert "NavMenuPageCount(page)" in PROXY
assert "kNavSuppliesFirstSetting" in PROXY and "kNavSuppliesEndSetting" in PROXY
assert "kNavWeaponsFirstSetting" in PROXY and "kNavWeaponsEndSetting" in PROXY
sub_count_start = PROXY.index("static int MenuPageSubCount(int page) {")
sub_count_end = PROXY.index("\n}\n", sub_count_start)
sub_count_body = PROXY[sub_count_start:sub_count_end]
assert not re.search(r"case .*?: return 1\b", sub_count_body), "one-page menus should use the default count"
assert not re.search(r"case .*?: return [234]\s*;", sub_count_body), "multi-page counts should be derived from descriptors or ranges"

assert re.findall(r"ClientConvenienceMenuAction::(\w+)", convenience_items) == [
    "OpenBotNames", "ToggleIdleFlashlight", "ToggleIdleViewRestore", "ToggleIdleWeaponRestore",
    "ToggleAutoClearDatacache", "ClearDatacacheNow", "OpenCfgOverride", "OpenSilentSi", "ToggleIdlePersonRestore",
    "OpenWModelScale",
]
assert PROXY.count("ClientConvenienceMenuItemAt(g_menuSub, digit)") >= 2
assert "ClientConvenienceMenuItemCount() == 10" in PROXY

assert re.findall(r"ForceCvarMenuAction::(\w+)", force_cvar_items) == [
    "ToggleDirectorHud", "ToggleFog", "ToggleFarZ", "ToggleFlashlight", "ResetFlashlightBrightness",
    "ToggleNoModelDecals", "OpenColorCorrection", "ToggleWeather", "ToggleFullbright", "ToggleNoGroundCorpses",
]
assert PROXY.count("ForceCvarMenuItemAt(g_menuSub, digit)") >= 2
assert "subpage * kMenuItemsPerPage + digit - 1" in PROXY

assert re.findall(r"CrashGuardMenuAction::(\w+)", crash_guard_items) == [
    "CycleCacheLimit", "CycleIndexBuffer", "OpenCacheOffload", "ToggleNoModelDecals",
    "ToggleAutoClearDatacache", "ClearDatacacheNow", "OpenClearDecals",
]
assert PROXY.count("CrashGuardMenuItemAt(g_menuSub, digit)") >= 2
assert "CrashGuardMenuItemCount() == 7" in PROXY
assert "CrashGuardMenuItemCount() <= kMenuItemsPerPage" in PROXY
assert "kMenuPageClearDecals" in PROXY
assert "ClearDecalsMenuItemAt(g_menuSub, digit)" in PROXY

assert re.findall(r"OverlayHudRootAction::(Open\w+)", overlay_root_items) == [
    "OpenAmmoArc", "OpenReloadRing", "OpenFireGate"
]
assert PROXY.count("OverlayHudRootMenuItemAt(g_menuSub, digit)") >= 2
assert "g_menuPage = item->targetPage;" in PROXY
assert "case kMenuPageOverlayHud: return MenuPageCountForItems(OverlayHudRootMenuItemCount());" in PROXY
assert re.findall(r"OverlayHudEditAction::(\w+)", overlay_ammo_items) == [
    "ToggleEnabled", "AdjustSize", "Reset", "CycleColor", "ToggleAmmoText", "CycleAmmoSide"
]
assert re.findall(r"OverlayHudEditAction::(\w+)", overlay_reload_items) == [
    "ToggleEnabled", "AdjustSize", "Reset", "CycleColor", "ToggleReloadHideXhair"
]
assert re.findall(r"OverlayHudEditAction::(\w+)", overlay_fire_items) == [
    "ToggleEnabled", "AdjustSize", "Reset", "CycleFireSide", "ToggleFireReload", "ToggleFireRecovery"
]
assert PROXY.count("OverlayHudEditMenuItemAt(g_menuPage, digit)") >= 2
overlay_paint_start = PROXY.index("} else if (g_menuPage == kMenuPageOverlayHud || OverlayHudEditPage(g_menuPage))", PROXY.index("static void MenuPaintEngine()"))
overlay_paint_end = PROXY.index("} else if (g_menuPage == kMenuPageSoundVolume)", overlay_paint_start)
overlay_paint = PROXY[overlay_paint_start:overlay_paint_end]
assert "OverlayHudEditItemsForPage(g_menuPage, itemCount)" in overlay_paint
assert "OverlayHudEditMenuItemAt(g_menuPage, digit)" in overlay_paint
overlay_key_start = PROXY.index("if (g_menuPage == kMenuPageOverlayHud || OverlayHudEditPage(g_menuPage))", PROXY.index("static void MenuProcessKeyImpl(int digit)"))
overlay_key_end = PROXY.index("if (g_menuPage == kMenuPageSoundVolume)", overlay_key_start)
overlay_key = PROXY[overlay_key_start:overlay_key_end]
assert "OverlayHudEditMenuItemAt(g_menuPage, digit)" in overlay_key
for action in re.findall(r"OverlayHudEditAction::(\w+)", overlay_ammo_items + overlay_reload_items + overlay_fire_items):
    assert f"case OverlayHudEditAction::{action}:" in overlay_paint
    assert f"case OverlayHudEditAction::{action}:" in overlay_key

assert re.findall(r"LastMusicMenuAction::(\w+)", last_music_items) == [
    "ToggleEnabled", "CycleMode", "AdjustVolume", "CycleVolumeStep", "RescanTrack",
    "ToggleAutoStop", "ToggleNotice", "SelectNoticeSize", "SelectNoticePosition",
]
assert "kLastMusicMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kLastMusicMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageLastMusic: return MenuPageCountForItems(SinglePageMenuItemCount(kLastMusicMenuItems));" in PROXY
assert re.findall(r"SafeMusicMenuAction::(\w+)", safe_music_items) == [
    "ToggleEnabled", "ToggleRandom", "OpenTrackList", "AdjustVolume", "CycleVolumeStep", "RescanTracks", "Preview", "Stop",
]
assert "kSafeMusicMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kSafeMusicMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageSafeMusic: return MenuPageCountForItems(SinglePageMenuItemCount(kSafeMusicMenuItems));" in PROXY
assert re.findall(r"SiWarningMenuAction::(\w+)", si_warning_items) == [
    "ToggleSpawn", "ToggleProximity", "CycleStyle", "AdjustSize", "ResetPosition",
]
assert "kSiWarningMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kSiWarningMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageSiWarningHud: return MenuPageCountForItems(SinglePageMenuItemCount(kSiWarningMenuItems));" in PROXY
assert re.findall(r"WorldNamesMenuAction::(\w+)", world_names_items) == [
    "ToggleVisible", "CycleColor", "AdjustSize", "ResetStyle",
]
assert "kWorldNamesMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kWorldNamesMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageWorldNamesColor: return MenuPageCountForItems(SinglePageMenuItemCount(kWorldNamesMenuItems));" in PROXY
assert re.findall(r"ChapStatsMenuAction::(\w+)", chap_stats_items) == [
    "ToggleEnabled", "ToggleScore", "CycleLayout", "OpenScoreHud", "OpenStyle", "OpenSize", "OpenDisplay", "CyclePageDuration",
]
assert "kChapStatsMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kChapStatsMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageChapStats: return MenuPageCountForItems(SinglePageMenuItemCount(kChapStatsMenuItems));" in PROXY
assert re.findall(r"ChapStatsSizeMenuAction::(\w+)", chap_stats_size_items) == ["AdjustScale", "ToggleResolutionScale"]
assert "kChapStatsSizeMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kChapStatsSizeMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageChapStatsSize: return MenuPageCountForItems(SinglePageMenuItemCount(kChapStatsSizeMenuItems));" in PROXY
assert re.findall(r"ChapStatsDisplayMenuAction::(\w+)", chap_stats_display_items) == [
    "CycleRows", "CycleView", "ToggleOverview", "ToggleKills", "ToggleDamage",
    "ToggleSpecialInfected", "ToggleTeam", "ToggleSpecial",
]
assert "kChapStatsDisplayMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kChapStatsDisplayMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageChapStatsDisplay: return MenuPageCountForItems(SinglePageMenuItemCount(kChapStatsDisplayMenuItems));" in PROXY
assert re.findall(r"ChapStatsScoreHudMenuAction::(\w+)", chap_stats_score_items) == ["ToggleEnabled", "AdjustScale", "ResetPosition"]
assert "kChapStatsScoreHudMenuItems[i].action" in PROXY and "SinglePageMenuItemAt(kChapStatsScoreHudMenuItems, g_menuSub, digit)" in PROXY
assert "case kMenuPageChapStatsScoreHud: return MenuPageCountForItems(SinglePageMenuItemCount(kChapStatsScoreHudMenuItems));" in PROXY

assert re.findall(r"LocalListenMenuAction::(\w+)", local_listen_items) == [
    "CycleTick", "CycleNextbotRate", "ToggleAllowZeroLerp", "CycleLerp",
    "ToggleSoloIdle", "ToggleNoIdleDelay", "CycleAirAcceleration",
    "OpenCharacterSettings", "OpenDebugCvars",
]
assert "LocalListenMenuItemCount() == 14" in PROXY
assert PROXY.count("LocalListenMenuItemAt(g_menuSub, digit)") >= 2
listen_key_start = PROXY.index("if (g_menuPage == kMenuPageLocalListen) {", PROXY.index("static void MenuProcessKeyImpl(int digit)"))
listen_key_end = PROXY.index("if (g_menuPage == kMenuPageLocalCharacterHub) {", listen_key_start)
listen_key = PROXY[listen_key_start:listen_key_end]
assert listen_key.index("if (digit == 0)") < listen_key.index("if (!ListenDetectHost())") < listen_key.index("LocalListenMenuItemAt(g_menuSub, digit)")
for action in re.findall(r"LocalListenMenuAction::(\w+)", local_listen_items):
    assert f"case LocalListenMenuAction::{action}:" in listen_key
listen_paint_start = PROXY.index("} else if (g_menuPage == kMenuPageLocalListen)", paint_start)
listen_paint_end = PROXY.index("} else if (g_menuPage == kMenuPageLocalDebugCvars)", listen_paint_start)
listen_paint = PROXY[listen_paint_start:listen_paint_end]
assert "LocalListenMenuItemAt(g_menuSub, digit)" in listen_paint
# Item loop must finish before the multi-page footer; do not pin absolute indent.
_item_guard = listen_paint.rfind("LocalListenMenuItemAt(g_menuSub, digit)")
assert _item_guard >= 0
assert re.search(r"if\s*\(\s*subN\s*>\s*1\s*\)", listen_paint[_item_guard:]), (
	"local listen item guard must close before page footer handling"
)

# Main-menu action numbers are serialized in current.hud. Keep the current set
# stable and make the editor list the validator's source of truth.
action_body = table_body(EDITOR, "static constexpr MainUiActionDef kMainUiActions[]={")
action_ids = list(map(int, re.findall(r"^\s*\{(\d+),\{UiId::", action_body, re.MULTILINE)))
allowed_ids = (
    {0, 1, 2, 3, 4, 5, 8, 11, 12, 27, 928, 929}
    | set(range(100, 109))
    | (set(range(200, 221)) - {215})
    | {900, 901, 902, 903}
    | (set(range(904, 926)) - {908, 909, 923})
    | set(range(930, 938))
)
assert len(action_ids) == len(set(action_ids)), "duplicate persisted main-menu action id"
assert set(action_ids) == allowed_ids, "serialized action ids changed; review current.hud compatibility"
assert "static bool MainUiLayoutActionValid(int action) { return MainUiActionKnown(action); }" in LAYOUT
assert "for (const auto& item : kMainUiActions) if (item.action == action) return true;" in EDITOR
assert "static_assert(MainUiActionCatalogUnique()" in EDITOR

print(
    "menu registry wiring passed: root=24, main-menu setup=5, combat=12, "
    f"navigation=4, world HUD={initializer_count(world_items)}, sound volume=17, serialized actions={len(action_ids)}"
)

assert "kNavDisplaySettings[] = {0, 50, 46, 45, 1, 2, 3, 4, 5, 6, 7, -1}" in PROXY
assert "g_menuPage == kMenuPageNavDisplay && g_menuSub == 1 && digit == 3" in PROXY
assert "g_menuPage == kMenuPageNavRoute && digit == 4" in PROXY
assert "g_menuPage == kMenuPageNavRoute && digit == 6" in PROXY
assert 'MapHudMissionPump((g_optMapHud || g_optTabBoard || g_optChapStats || EscUiWantsMapInfo())' in PROXY

# Full kMenuPage* audit: uniqueness, complete coverage, and kMenuPageIdNext = max+1.
_PAGE_ID_RE = re.compile(r"(?:static\s+)?constexpr\s+int\s+(kMenuPage\w+)\s*=\s*(\d+)")
_PAGE_ID_SKIP = {"kMenuPageIdNext", "kMenuItemsPerPage"}


def _load_menu_page_ids() -> dict[str, int]:
    ids: dict[str, int] = {}
    for rel in (
        "src/skeeto/core/skeeto_proxy.cpp",
        "src/skeeto/core/skeeto_hud.h",
        "src/skeeto/core/skeeto_host.h",
    ):
        src = (ROOT / rel).read_text(encoding="utf-8")
        for match in _PAGE_ID_RE.finditer(src):
            name, value = match.group(1), int(match.group(2))
            if name in _PAGE_ID_SKIP or name.startswith("kMenuPageId"):
                continue
            assert name not in ids or ids[name] == value, f"conflicting {name}"
            ids[name] = value
    return ids


_menu_page_ids = _load_menu_page_ids()
assert len(_menu_page_ids) == len(set(_menu_page_ids.values())), "duplicate kMenuPage* value"
_audit_body = PROXY.split("kMenuPageIdAudit[] = {", 1)[1].split("};", 1)[0]
_audit_names = set(re.findall(r"kMenuPage\w+", _audit_body))
assert _audit_names == set(_menu_page_ids), (
    f"kMenuPageIdAudit mismatch missing={sorted(set(_menu_page_ids) - _audit_names)} "
    f"extra={sorted(_audit_names - set(_menu_page_ids))}"
)
_next = int(re.search(r"kMenuPageIdNext\s*=\s*(\d+)", PROXY).group(1))
assert _next == max(_menu_page_ids.values()) + 1, "kMenuPageIdNext must be max page id + 1"
assert "MenuPageIdAuditMax" in PROXY
assert "auto menuPaintHitFeedback = [&]() -> bool" in PROXY
assert "auto menuPaintPagesA = [&]()" in PROXY
assert "auto menuPaintPagesB = [&]()" in PROXY
assert "if (!menuPaintHitFeedback())" in PROXY
assert "Pages B: ViewRoll + ThrowNotice through AcidOutline as sibling ifs" in PROXY
assert "goto menu_paint_after_pages" not in PROXY
assert "kViewRollMenuItems[]" in PROXY
assert "ViewRollMenuItemAt(digit)" in PROXY
for _table, _at in (
    ("kShotgunReloadMenuItems[]", "ShotgunReloadMenuItemAt(digit)"),
    ("kXhairTpMenuItems[]", "XhairTpMenuItemAt(digit)"),
    ("kHoldSemiMenuItems[]", "HoldSemiMenuItemAt(digit)"),
    ("kDissolveMenuItems[]", "DissolveMenuItemAt(digit)"),
    ("kAcidOutlineMenuItems[]", "AcidOutlineMenuItemAt(digit)"),
):
    assert _table in PROXY, f"missing {_table}"
    assert _at in PROXY, f"missing {_at}"
assert "} else if (g_menuPage == kMenuPageRouteProgressHud)" not in PROXY
print(
    f"menu page ids: {len(_menu_page_ids)} unique, next={_next}, "
    "paint callables HitFeedback/PagesA/PagesB present; setup tables=5"
)

# ---------------------------------------------------------------------------
# Dual-write contract: settings pages must be table-driven; exceptions only.
# ---------------------------------------------------------------------------

def _load_discovery_tables() -> dict[str, str]:
    gen = (ROOT / "tools/generate_menu_discovery.py").read_text(encoding="utf-8")
    body = re.search(r"TABLES = \{([\s\S]*?)\n\}", gen)
    assert body, "generate_menu_discovery.py TABLES missing"
    return dict(re.findall(r'"(k\w+)":\s*("(?:kMenuPage\w+|\d+)")', body.group(1)))


def _enumerate_menu_tables(source: str) -> set[str]:
    names = set(re.findall(r"static constexpr \w+ (k\w+)\[\]", source))
    return {
        name
        for name in names
        if name.endswith(
            (
                "MenuItems",
                "EditItems",
                "ExperimentItems",
                "SetupItems",
                "RootItems",
                "CategoryItems",
                "StyleItems",
                "Menu",
            )
        )
    }


_discovery_tables = _load_discovery_tables()
_menu_tables = _enumerate_menu_tables(PROXY)
_missing_discovery = sorted(_menu_tables - set(_discovery_tables))
assert not _missing_discovery, (
    "menu tables missing from generate_menu_discovery.py TABLES: "
    + ", ".join(_missing_discovery)
)

# Permanent dual-write / special-layout exceptions (must not grow without review).
_DUAL_WRITE_EXCEPTIONS = frozenset(
    {
        "kMenuPageBotNameEdit",  # text editor flow
        "kMenuPageCareerArchive",  # archive browser, not 1–9 settings rows
    }
)

_val_to_page: dict[int, str] = {}
for _name, _value in _menu_page_ids.items():
    _val_to_page.setdefault(_value, _name)


def _resolve_page_token(token: str) -> str:
    if token.isdigit():
        return _val_to_page.get(int(token), f"PAGE_{token}")
    return token


_key_impl = PROXY[PROXY.index("static void MenuProcessKeyImpl(int digit)") :]
_dual_pages: set[str] = set()
_tabled_pages: set[str] = set()
_top_page_ifs = list(
    re.finditer(r"(?m)^\tif \(g_menuPage == (kMenuPage\w+|\d+)\) \{", _key_impl)
)
for _index, _match in enumerate(_top_page_ifs):
    _end = (
        _top_page_ifs[_index + 1].start()
        if _index + 1 < len(_top_page_ifs)
        else len(_key_impl)
    )
    _chunk = _key_impl[_match.start() : _end]
    _page = _resolve_page_token(_match.group(1))
    _has_digit = bool(re.search(r"digit == [1-9]", _chunk))
    _has_item = bool(
        re.search(
            r"MenuItemAt\(|SinglePageMenuItemAt\(|PanelStyleMenuDigit\(", _chunk
        )
    )
    if _has_item:
        _tabled_pages.add(_page)
    elif _has_digit:
        _dual_pages.add(_page)

for _match in re.finditer(
    r"(?m)^\tif \((g_menuPage == (?:kMenuPage\w+|\d+)(?: \|\| g_menuPage == (?:kMenuPage\w+|\d+))+)\) \{",
    _key_impl,
):
    _chunk = _key_impl[_match.start() : _match.start() + 8000]
    _has_digit = bool(re.search(r"digit == [1-9]", _chunk))
    _has_item = bool(
        re.search(
            r"MenuItemAt\(|SinglePageMenuItemAt\(|PanelStyleMenuDigit\(", _chunk
        )
    )
    for _tok in re.findall(r"g_menuPage == (kMenuPage\w+|\d+)", _match.group(1)):
        _page = _resolve_page_token(_tok)
        if _has_item:
            _tabled_pages.add(_page)
            _dual_pages.discard(_page)
        elif _has_digit:
            _dual_pages.add(_page)

_unexpected_dual = sorted(_dual_pages - _DUAL_WRITE_EXCEPTIONS)
assert not _unexpected_dual, (
    "settings pages still use digit dual-write (migrate or add to "
    "_DUAL_WRITE_EXCEPTIONS after review): " + ", ".join(_unexpected_dual)
)
_stale_exceptions = sorted(_DUAL_WRITE_EXCEPTIONS - _dual_pages)
assert not _stale_exceptions, (
    "dual-write exceptions no longer dual — shrink _DUAL_WRITE_EXCEPTIONS: "
    + ", ".join(_stale_exceptions)
)

# Action symmetry for dedicated setup tables (paint + key both switch on Action::).
_SETUP_ACTION_TABLES = (
    ("ViewRollMenuAction", "kViewRollMenuItems"),
    ("ShotgunReloadMenuAction", "kShotgunReloadMenuItems"),
    ("XhairTpMenuAction", "kXhairTpMenuItems"),
    ("HoldSemiMenuAction", "kHoldSemiMenuItems"),
    ("DissolveMenuAction", "kDissolveMenuItems"),
    ("AcidOutlineMenuAction", "kAcidOutlineMenuItems"),
    ("KillFeedMenuAction", "kKillFeedMenuItems"),
    ("TabBoardMenuAction", "kTabBoardMenuItems"),
    ("TongueCutHudMenuAction", "kTongueCutHudMenuItems"),
    ("WModelScaleMenuAction", "kWModelScaleMenuItems"),
)
_paint_engine = function_body(PROXY, "static void MenuPaintEngine() {")
_key_body = function_body(PROXY, "static void MenuProcessKeyImpl(int digit) {")
for _action_enum, _table_name in _SETUP_ACTION_TABLES:
    _decl = re.search(rf"static constexpr \w+ {_table_name}\[\] = \{{", PROXY)
    assert _decl, f"missing {_table_name}"
    _tbody = table_body(PROXY, _decl.group(0))
    _actions = re.findall(rf"{_action_enum}::(\w+)", _tbody)
    assert _actions, f"no actions in {_table_name}"
    assert _table_name in _paint_engine, f"paint must iterate {_table_name}"
    assert _table_name in _key_body or f"{_table_name[:-1]}At" in _key_body or (
        "SinglePageMenuItemAt(" + _table_name
    ) in _key_body or any(
        f"case {_action_enum}::{_action}:" in _key_body for _action in _actions
    ), f"key must use {_table_name}"
    for _action in _actions:
        _token = f"{_action_enum}::{_action}"
        assert f"case {_token}:" in _key_body, f"key missing case {_token}:"

print(
    f"menu dual-write contract: tables={len(_menu_tables)}, "
    f"discovery={len(_discovery_tables)}, "
    f"tabled_pages={len(_tabled_pages)}, "
    f"dual_exceptions={sorted(_DUAL_WRITE_EXCEPTIONS)}"
)

# Page ids are named; adjustment is one direction-parameterised function.
assert not re.search(r"g_menuPage\s*(?:==|!=)\s*\d", PROXY), "g_menuPage must be compared with kMenuPage* constants"
_vk_body = function_body(PROXY, "static void MenuOnVk(DWORD vk) {")
assert _vk_body.count("MenuAdjustCurrentPage(-1)") == 1 and _vk_body.count("MenuAdjustCurrentPage(1)") == 1
assert "MenuAdjustSfxVol" not in _vk_body and "MenuBumpHudScale" not in _vk_body, \
    "-/= adjustment belongs in MenuAdjustCurrentPage(dir)"

# Remaining literal digit rows live only on dynamic/paged pages (sound-volume main rows, nav routes, bot-name edit,
# career archive). The number may shrink; new fixed rows belong in tables.
_key_impl_start = PROXY.index("static void MenuProcessKeyImpl(int digit)")
_key_impl_text = PROXY[_key_impl_start:PROXY.index("static void MenuProcessKey(", _key_impl_start)]
_literal_digit_rows = len(re.findall(r"\bdigit == [1-9]\b", _key_impl_text))
assert _literal_digit_rows <= 8, f"new fixed rows must use tables ({_literal_digit_rows} literal digit tests)"

assert not re.search(r"(?m)^\telse", function_body(PROXY, "static void MenuAdjustCurrentPage(int dir) {")), \
    "MenuAdjustCurrentPage uses sibling early-return ifs, not an else-if chain"
