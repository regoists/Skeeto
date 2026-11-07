# Skeeto HUD/UI finished references and source entry points

This index finds verifiable Skeeto finished pieces and the current implementation. It does not make any design proposal or research report the standard answer for HUDs. The user's current request always wins.

## Design-reference priority

1. **This round's explicit requirements and target images from the user.** Direct basis for product and visual judgment this round; when the engine constrains implementation, state the difference and offer a feasible presentation.
2. **Previously designed and shipped Skeeto HUD pieces plus user feedback on them.** From real implementations, available screenshots, and concrete feedback, extract useful hierarchy, layout detail, color semantics, brand recognition, and interaction. Positive comments help continuity; explicit negatives are pitfalls. Extract principles before migrating—do not copy coordinates, sizes, or whole styles.
3. **Other Skeeto HUDs still in use.** Use them to understand existing fonts, color, borders, spacing, and interaction habits. Their goals and density may differ; code existence alone does not mean the user accepted them or that they should be copied.
4. **Proposals, learning reports, Git history, and external mods.** Not default design references, and not required to always exist. Look them up only when the task needs a concrete technical fact, background for a past tradeoff, or the user explicitly asks to compare them. Confirm document status and evidence first, then return to current source and user feedback.

If neither context nor the workspace preserved concrete feedback, do not invent which style was once approved. Check current effects of previously shipped HUDs for Skeeto continuity, and mark aesthetic choices as this round's proposal for real-screen verification—do not promote temporary doc proposals into design standards.

## Current implementation entry points (distinguish modules first)

- Module ownership, HUD call entries, and stable search clues follow `docs/agent-guides/module-performance-and-structure.md`; source of truth is `src/skeeto/`. Search the current map and source for the concrete feature first, then verify the full chain through Paint calls, producers, Wanted gating, menus, and caches.
- **Custom in-game HUD:** `core/skeeto_hud.cpp` and matching headers, navigation submodules; locate TAB, chapter stats, memory, etc. from the module map. Follow producers, Wanted, Paint, and screen settings—do not copy the main-menu editor.
- **World HUD:** From `WorldNotice*`, `TeamWorldNames*`, `NativeWorldNamesWanted`, `TeamWorldNamesWanted`, and world-nav consumers, check world anchors, projection, distance, and visibility. They often share `skeeto_hud.cpp` with screen HUDs; file ownership alone does not decide the module.
- **Independent overlay HUD:** `HudLayer*` in `core/skeeto_proxy.cpp`, `HudOverlay*` in `core/skeeto_hud.cpp`, shared declarations in `skeeto_host.h`. Follow Paint snapshot → UI-thread present → window hide/destroy. Do not explain independent-window effects with Surface coordinates, fonts, or batching.
- **Main-menu UI:** `MainUi*` in proxy and `skeeto_mainui_layout/editor/widgets/config/hub/native/services`. Inspect layout/params/parenting, paint/hit-test, config apply/save, native entry, and file/network services separately; sharing Surface still does not reuse in-game HUD schemes.
- **ESC-menu UI:** Independent `EscUi*` module, separate from `MainUi*`. Use the module map for ESC layout/editor/native bridge and config, then the ESC section in module-profiles; check in-map state, native pause layer, focus, and input release. The home page has some user evidence; remaining acceptance scope follows related records.
- **Shared style and resources:** Trace real consumers to `skeeto_style`, Surface fonts/measure, or independent-window GDI+/DIB. Share only what applies; brand continuity does not require world markers, mouse main menus, and combat panels to share one style.
- Menu numbering, position/switch persistence, localization, and off-state behavior follow matching topics in `AGENTS.md`.

## Reusable checks distilled from past feedback

These checks help evaluate a screen. They do not mean any existing style is approved, and they do not require new HUDs to reuse a particular look.

- Allocate space from the component's real width/height; avoid crushing everything to one side; keep related labels and values close; keep numeric alignment stable.
- Rows and blocks inside one HUD need coherent hierarchy and visual language. Whitespace, lines, underlays, or small brand marks can relate them; every HUD need not use the same card, border, or layout.
- Keep established, user-accepted color semantics such as threshold colors for existing metrics; keep neutral measurements neutral. Add state color for new metrics only when threshold meaning is clear, and also encode state with text or shape.
- For dense UIs, sort primary/secondary and reading order before choosing horizontal layout, grouping, or paging. Do not crush current content just to reuse another page's format.
- Code review cannot replace looking at the picture. Inspect final results at real resolutions, text, data, and common HUD combinations. Turn concrete problems from negative feedback into reusable checks; do not keep rejected old looks as exemplars.

Technical reports are secondary evidence only for concrete mechanism questions. Discovery and evidence rules for reference projects follow `AGENTS.md` and `docs/reference/Skeeto开发信息参考指南.md`. When related reports are missing, return to current source, game resources, or reproducible live tests—do not cite assumed paths.

## Reusable judgments from past issues

- Transparent flyout submenus stacked on the home page: the issue is front/back paint and hit isolation, not palette. Covering mistakes with an opaque background is not correct layering—test full transparency.
- Reload glyphs losing bullet recognition, or native crosshair-hide behavior lost: keep functional meaning and silhouette first, then denser decoration. Moving Surface to overlay does not authorize dropping state coupling.
- "Up/down/left/right" only rotating an arc at the old offset: first state center, radius, crosshair anchor, and position offset; distinguish compass direction from rotation angle. Verify extreme positions, not only the default right side.
- Main-menu actions that look similar but are scattered, with unclear parameters: action grouping and parameter hints are interaction design; keep similar items together and give parameterized ops full syntax plus workable examples.
- IME, character display, and painted text are different capabilities: native Unicode input windows open manually when needed; whether the game font supports a symbol still needs a real-screen check.
- Improving default presets must not overwrite user edits; keys/behaviors discarded in development are not a permanent compatibility burden. Formal-release user-data protection follows project rules.

These conclusions come from concrete feedback. Do not freeze one temporary implementation, screenshot size, or resource budget as the answer for every later UI. When future modules change, maintain the matching branch and entry points—do not copy the whole module map or depend on machine-local temporary plan files.
