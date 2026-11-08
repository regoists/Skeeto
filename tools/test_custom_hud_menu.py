from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "skeeto" / "core" / "skeeto_proxy.cpp"
ENGLISH = ROOT / "src" / "skeeto" / "core" / "skeeto_ui_en.inc"
source = SOURCE.read_text(encoding="utf-8")
english = ENGLISH.read_text(encoding="utf-8")

marker = "static constexpr CustomHudMenuItem kCustomHudMenu[] = {"
assert source.count(marker) == 1, "custom HUD menu must have one ordered descriptor table"
start = source.find(marker)
assert start >= 0, "custom HUD menu must have one ordered descriptor table"
end = source.find("\n};", start)
assert end >= 0, "custom HUD descriptor table is not closed"
table = source[start:end]

row_pattern = re.compile(
    r'^\s*\{UiId::(?P<name>\w+), L".*?", UiId::(?P<desc>\w+), L".*?", '
    r'CustomHudMenuAction::(?P<action>\w+), (?P<target>[^,]+), '
    r'(?P<warning>true|false)\},?\s*$',
    re.MULTILINE,
)
rows = list(row_pattern.finditer(table))
assert rows, "descriptor table must contain menu entries"
assert len(rows) == len(table.splitlines()) - 1, "every table row must be a complete descriptor"

# Custom HUD titles and explanatory lines are separate localized resources.
# A title accidentally bound to its description is especially visible in EN,
# where description strings start with indentation to render as a subline.
english_text = {
    match.group("id"): match.group("text")
    for match in re.finditer(
        r'SKEETO_UI\((?P<id>\w+), L"(?P<text>(?:\\.|[^"\\])*)"\)',
        english,
    )
}
for row in rows:
    name_id, desc_id = row.group("name"), row.group("desc")
    assert name_id != desc_id, f"title and description share UiId::{name_id}"
    localized_name = english_text.get(name_id)
    assert localized_name is None or not localized_name.startswith("    "), (
        f"title UiId::{name_id} resolves to an indented description"
    )

actions = [row.group("action") for row in rows]
assert actions[:5] == [
    "ReloadUi",
    "ReloadVideoMode",
    "ToggleHideAllCustomHud",
    "TogglePauseSampling",
    "ToggleHudResolutionScale",
], "reload, hide, pause, and scaling controls must keep their approved order"
assert 'L"    重建 Skeeto 字体和界面贴图缓存，游戏内更改显示模式后字体消失时使用。控制台：skeeto_ui_reload"' in table
assert 'L"    游戏会卡死一段时间然后恢复，可以触发资源重载。控制台：skeeto_ui_videomode_reload"' in table
assert 'L"    临时隐藏SKT的所有自定义HUD，不改变各 HUD 的开关状态，不影响隐藏原版 HUD；控制台指令：skeeto_hud_toggle"' in table
assert 'L"    自定义 HUD、世界 HUD、统计与命中反馈全部暂停，菜单与准星保留。"' in table
assert "SurfWrapDesign(y, cMuted, desc, MenuHintGap())" in source, "custom HUD descriptions should remain gray"
assert "打开 Skeeto 使用说明" not in source and "OpenSkeetoManual" not in source
assert "开启物资雷达后游戏可能会有掉帧现象。" not in source
assert actions.count("TogglePauseSampling") == 1
assert actions.count("ToggleHideAllCustomHud") == 1
assert actions.count("ToggleHudResolutionScale") == 1
assert actions.count("ReloadUi") == 1
assert actions.count("ReloadVideoMode") == 1
team_warning_rows = [row.group("name") for row in rows if row.group("warning") == "true"]
assert team_warning_rows == ["Text_2cdb9a5f"], "the existing Team HUD warning must stay with Team HUD"
assert actions.count("OpenTabBoard") == 1 and actions.count("OpenChapterStats") == 1
assert next(row.group("target").strip() for row in rows if row.group("action") == "OpenTabBoard") == "kMenuPageTabBoard"
assert next(row.group("target").strip() for row in rows if row.group("action") == "OpenChapterStats") == "kMenuPageChapStats"

for row in rows:
    action = row.group("action")
    target = row.group("target").strip()
    if action in {"OpenSettings", "OpenTabBoard", "OpenChapterStats"}:
        assert target != "-1", f"{action} row has no destination"
    else:
        assert target == "-1", f"{action} row must not have a page destination"

assert "case kMenuPageCustomHudHub: return CustomHudMenuPageCount();" in source, (
    "custom HUD page count must derive from descriptor count"
)
assert "static constexpr int kMenuItemsPerPage = 9;" in source
assert "static constexpr int kCustomHudMenuItemsPerPage = kMenuItemsPerPage;" in source
assert "return (CustomHudMenuItemCount() + kMenuItemsPerPage - 1) / kMenuItemsPerPage;" in source
assert "const CustomHudMenuItem& item = kCustomHudMenu[i]" in source, (
    "custom HUD drawing must read the ordered descriptors"
)
key_access = "const CustomHudMenuItem& item = kCustomHudMenu[displayIndex]"
assert key_access in source, (
    "custom HUD key dispatch must use the same ordered descriptors"
)
assert source.index("if (displayIndex < 0 || displayIndex >= CustomHudMenuItemCount()) return;") < source.index(key_access), (
    "key dispatch must reject blank or invalid positions before indexing the table"
)
assert "CustomHudDisplayItem" not in source, "the index remapper must be removed"
assert "UiTextList<52> kNames" not in source and "UiTextList<52> kDesc" not in source
assert "static const int kSub[]" not in source

print(f"custom HUD menu descriptors: {len(rows)}; ordering and shared wiring passed")
