# Menu and settings rules

When changing menus, options, layout, or ini, read this guide and inspect the current menu page plus its input/save paths.

## Adding a menu page

When adding a whole page (including a settings subpage), follow this order to avoid page-ID collisions and the MSVC C1061 nesting limit in `MenuPaintEngine`:

1. **Page ID:** Assign the current `kMenuPageIdNext` value to the new `kMenuPage*`, then increment `kMenuPageIdNext`. Do not hand-guess a free ID. New symbols must be written into `kMenuPageIdAudit`; `static_assert` and `python tools/test_menu_registry.py` check uniqueness and `Next == max+1`.
2. **Paint:** `MenuPaintEngine` dispatches through three independent callables (MSVC nesting resets per function, avoiding C1061):
   - `menuPaintHitFeedback`: hit-feedback only
   - `menuPaintPagesA`: discovery through panel style, etc. (short `else if` chain; **do not lengthen it further**)
   - `menuPaintPagesB`: same-level independent `if`s (**new pages go here by default**)
   Do not stack more `goto` stopgaps, and do not merge Pages B back into Pages A.
3. **Keys:** In `MenuProcessKeyImpl` use independent `if (g_menuPage == …) { … return; }` blocks; do not turn this into a giant `else if`.
4. **Settings-subpage interaction:** The parent page keeps only a white "【进入设置】"; toggles are the first item on the subpage. **Migratable settings pages must use a small description table** (see `kCampaignOutlineMenuItems` / `kKillFeedMenuItems`: paint iterates + key `ItemAt`/`SinglePageMenuItemAt` + `switch(action)` + `static_assert(count <= 9)`). Do not add more paint/key digit dual-write pages. Only special layouts such as Bot-name editing and career archive may keep dual-write, and they must be registered in `_DUAL_WRITE_EXCEPTIONS` in `tools/test_menu_registry.py` (the whitelist may only shrink).
5. **Finish:** Add new tables to `TABLES` in `tools/generate_menu_discovery.py` (action-only tables without UiId row copy also go into `CATALOG_EXCLUDE`); run `python tools/generate_menu_discovery.py`, `python tools/test_menu_registry.py`, and `cmd /c src\skeeto\build.bat`. The registry checks that tables are registered, settings pages have no unexpected dual-write, and sample tables wire paint/key correctly.

Page IDs are only a runtime and source/discovery contract; they are not written into `skeeto_setting.ini`. Do not renumber existing pages just to "fill gaps."

## Menu layout

- **Unified interaction beats temporary implementation convenience.** For features with a settings subpage, the parent page keeps only one white "【进入设置】" entry and puts toggles as the first subpage item; do not split into a parent toggle plus a parent settings item. New and redesigned features follow this; do not use the current task to rearrange unrelated old pages.
- Color presets default to single-item rotation: each press advances one preset and shows the current preset. Precise RGB adjustment is provided through registered console commands or parameters, not split into multiple presets or three menu RGB items. When the user explicitly asks for another interaction, follow that.
- Numeric settings must be checked against actual consumers and final resources; changing only the saved value is not enough. Cases where a cache falls back to a different font size must not present the requested value as the value that actually took effect. A successful build does not prove live display passed.
- At most 9 items per page, numbered from `[1]`. **When there is a next subpage, the previous subpage must first fill all 9 items** (for example total 14 → `9+5`, not `8+6`). After deletions, backfill earlier-page gaps from the new ordered table; page count and per-page counts are derived from the menu description table or an explicit settings range—do not maintain a handwritten page-count comment.
- Subpage entry copy stays unified: root main-menu options get no enter hint; clickable settings-subpage entries inside a feature page show "【进入设置】"; read-only page entries show "【进入说明】"; English uses the matching localized text. Put the hint on the entry row; do not replace it with an extra status row.
- Two-way toggles use green/red from `col()`; enter-settings, multi-step switches, one-shot actions, and non-clickable rows use white `cText` (locked/unavailable use `cMuted`); rows that should show the real color use that color. Enter-settings or action rows must not stay permanently green.
- Items adjusted with `- / +` must write `- / + 调节` directly on the value row and use the menu's existing yellow/orange adjust color `cAdjust`; footer hints are only supplements and do not replace the on-option control hint.
- Page adjust for `-` / `=` is written only in `MenuAdjustCurrentPage(int dir)`: `dir` is -1/+1, step is `dir * step`, page branches use parallel `if … return`, not an `else if` chain, and do not duplicate positive/negative copies in `MenuOnVk`. Page checks always use `kMenuPage*` constants, never numeric page numbers.
- Shared trailing work across multiple subpages (such as `ClampXxx(); SaveSettings(); ListenMarkDirty();`) is handled only for subpage 0; settings branches on subpages 1+ must save themselves.
- Prefer adding new items to a subpage that still has free slots. When reordering or adding/removing items, prefer changing one ordered description table so paint and digit input read the same table; do not change only the picture, only the key branch, or maintain a parallel order. Recheck the target subpage, return path, and the 9-item-per-page limit; do not force-insert at the top of a full page.
- Engine commands (especially third-party / without `CLIENTCMD_CAN_EXECUTE`) prefer `UnlockConCommand` + `EngClientCmdUnrestricted`. Ordinary `EngClientCmd` may silently drop them—`cl_destroy_ragdolls` already hit this, and `l4n_clear_datacache` is similar.

## Mechanical contracts

After changing menus, menu copy, or the search catalog, the following three must pass. Debt numbers may only go down; lower the test ceilings with them—do not raise ceilings just to pass.

- `python tools/test_menu_registry.py`
- `python tools/test_menu_discovery.py`
- `python tools/check_ui_text.py`

Behaviors they lock:

- Settings-page paint and digit keys read the same description table. Dual-write remains only in `_DUAL_WRITE_EXCEPTIONS` in `tools/test_menu_registry.py`; the list only shrinks.
- Handwritten fixed rows for `digit == 1` through `digit == 9` in `MenuProcessKeyImpl` stay under the current ceiling; new fixed rows go into description tables.
- `g_menuPage` is compared only with `kMenuPage*`. `MenuOnVk` `-` / `=` only call `MenuAdjustCurrentPage`. Page branches in `MenuAdjustCurrentPage` stay parallel `if … return` and must not fall back to a top-level `else` chain.
- The search catalog is generated by `tools/generate_menu_discovery.py`; do not hand-edit generated files. Unfindable rows among registered description tables must not exceed the current ceiling in `test_menu_discovery.py`. Only tables whose structs and paint branches both lack stable `UiText` row copy go into `CATALOG_EXCLUDE`. Rows painted as `[%d]` with a runtime index are not recognized by the number scan; those tables must stay in the table loop for inclusion—controlled-HUD background-color rows are an example. Do not add another feature overview.
- Player-visible Chinese goes through `UiText(UiId::…, L"…")` or `UiTextEntry`; English lives in `skeeto_ui_en.inc`; formatters match on both sides. Body text like `90%` is not a format specifier. Intentional blanks are allowed only for `NavItem0`. Catalog entries with no `UiId::` reference are deleted; do not leave empty translations. Unused and unlocalized checks in `python tools/check_ui_text.py` follow real call sites, including `UiText` arguments, table rows with `UiId::` plus wide strings, and literals already used as fallback copy in the same file.

## Menu description tables and paint/input sync

- Root menu and migrated feature menus use static ordered description tables for row order, actions, target pages, and needed settings indexes. `MenuPaintEngine` and `MenuProcessKeyImpl` must obtain rows through the same table or the same `ItemAt` lookup so paint order, digit-key targets, and page counts are not written three ways. State values still come from the original settings variables; description tables do not copy settings state.
- Page count is computed from description-table item count or from an explicit settings-index range. Menu subpage-count dispatch only calls the matching count function; do not re-handwrite multi-page counts. Special layouts such as fixed control rows on the volume page or action rows on the radar page use named constants, with action-row sentinel indexes and similar conventions in `static_assert`.
- Description-table order is display order and digit-key order. Action targets, cross-subpage state, and format parameters that depend on a specific position should use compile-time asserts; formatting may also select by action directly so `%s` / `%d` and argument types do not mismatch after position changes. `UiId` and Chinese fallback copy must keep correct format placeholders and localization counterparts.
- Paint action branches that produce row text/color must cover registered actions; `default` should clear the current row buffer and use the unavailable color so previous-row text cannot linger. Hint-only branches may choose not to show hints for unknown actions. Input branches that hit an empty row or unknown action should exit safely without triggering another page's settings. Known actions must keep their existing save timing, key-click sound, target page, `g_menuSub` / `g_menuPagePrev` reset, and `MenuMarkDirty` behavior.
- `current.hud` saves numeric action IDs for the main-menu UI. They are persisted values, not menu row numbers; when adding or adjusting editor options, keep IDs unique and preserve the currently valid ID set—paint-order changes must not renumber those IDs.
- Offline menu-registry checks: `python tools/test_menu_registry.py` and `python tools/test_custom_hud_menu.py`. They check tables, page counts, paint/input wiring, and main-menu serialization IDs in source; passing does not prove in-game text, color, paging, key feedback, and settings I/O were live-tested. Related changes are verified under this round's scope and current live authorization; if not live-tested, state the gap and do not start the game on your own.

## Deleting an option

When deleting a user settings option, finish all six places:

1. Check old-key migration paths in `LoadSettings`; save rewrites the currently supported settings keys uniformly, so old keys need no cleanup list.
2. Delete the menu item.
3. Delete the key branch.
4. Fix page copy that still references it; do not describe deleted or unimplemented features.
5. Clean matching state: snapshots, handles, timestamps, armed flags, etc.
6. Search source and menu copy to confirm identifiers and key names have no live references (except removed sections), then build.

Do not edit unrelated historical docs just for "no leftovers."

## Search

- The first root-menu item is search; the built-in catalog is generated at build time from current menu description tables and numbered rows. After changing menus or localization, run `python tools/generate_menu_discovery.py`; ordinary builds also generate it. Do not hand-edit generated files. New description tables must be registered with the generator; do not treat display numbers as persistent feature identity.
- Feature name, description, original path, and common nicknames may all match. Third-party plugin internal menus, dynamic asset names, and internal diagnostic commands are not cataloged. Do not add another parallel "feature overview" entry or category browser.
- Search lists need name, purpose, and path lines, with an independent list budget of at most five entries per page reduced by currently available height; also leave `[1] 修改关键词`. This reading-list exception does not change the ordinary settings-page rule of filling nine items. Choosing an entry only opens the settings page and shows a locate hint; it does not execute the original row. Return reuses the existing stack and keeps the query and page number.
- Text input uses a caller-exclusive ticket. Search only cancels and consumes its own requests; when the menu closes or leaves its page, drop its own pending input; while temporarily parked, keep the window transaction so Paint can consume the completed result. Queries are computed only on confirm; the current-page text cache updates only with list, page, language, and font changes; do not add new files or settings keys.
- Offline checks: `python tools/test_menu_discovery.py`, `python tools/test_menu_discovery_navigation.py`, `python tools/test_menu_registry.py`, `cmd /c src\skeeto\test_mainui_services.bat`. These do not replace live focus, IME, and readability acceptance.

## Settings and localization

- Ordinary persistent settings use the single file `skeeto_setting.ini`. Custom main-menu UI is an independent module and, as specified by the user, saves layout, appearance, and module switches under the game root at `left4dead2/skeeto/mainmenu/current.hud`; shareable schemes include only the main menu and are not mixed with in-game HUD config. `SaveSettings` / `LoadSettings` key names stay consistent; keep settings across old-settings upgrades, and apply feature-defined defaults only for missing keys. Installer packages must not ship a user ini.
- On save, write only current-version valid keys and user comments; unrecognized assignment lines are not written back. Whether old versions and old configs are compatible follows the "prefer the new version" principle in `AGENTS.md`; do not default to adding migration, aliases, or extra fallbacks; keys, layouts, formats, and behaviors discarded during unpublished development iteration are removed directly. Replace via a same-directory temp file; on read or write failure keep the original file; do not create settings backups—only delete the leftover `skeeto_setting.ini.before-key-cleanup.bak`.
- Menu delay and deadline compares use `(LONG)(now - deadline)` to handle `GetTickCount` wrap; do not compare magnitudes directly.
- Prefer placing high-risk menu options into a suitable free slot so the whole page does not renumber without need.
- When editing multi-line `fprintf` format strings in `SaveSettings`, formatters and argument order must match one-to-one. Adding a `%d` or similar formatter requires inserting the matching argument in the same argument list; `C4477`/`C4473` warnings are failures and require a zero-warning build.
- Menu and HUD copy follow the English-localization requirements in root `AGENTS.md`; intentionally shared English uses existing explicit fallback text and does not scatter isolated hardcoding at paint sites.
- In-game Surface fonts are known to render `±` as `?`; do not use that symbol in menu or minimap copy. Before adding special symbols, confirm display in the real game font, and check numbering, titles, and English fallbacks together.
- Avoid unnecessary secondary labels and descriptions. Let the layout do the explaining. Option names, grouping, on-row value text, and existing entry-row marks such as "【进入设置】" or `- / + 调节` should carry the meaning. Do not add a hint that restates a clear switch (for example, an auto-save toggle must not also say that enabling it will save automatically). Do not put a caution paragraph under every delete or risky control when the name already states the action; write a short warning only when the consequence is irreversible or otherwise easy to miss. A settings page must not become a manual with a paragraph under every item.
- When a hint is warranted, keep it professional and short. Explain only the non-obvious player-visible result after enabling, or a required file-placement path (and whether loose files and mounted VPKs are supported). Do not write ConVars, console commands, offsets, function names, or implementation process. Do not use "本地" to mean "only happens on this client"—players will read that as local-server-only; write "本地服" only when the feature truly works only on a local server.
