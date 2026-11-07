# Designing HUD and UI by module

Read only the section for the current task. This page distills experience from shipped modules and real feedback. Function names are search entry points; re-check threads, budgets, defaults, and storage from current source every time. It does not require every module to share fonts, colors, or panels, and it does not authorize inventing a general UI framework.

## 1. Custom in-game HUD

**Goal: understand it in a short combat glance.** Status panels, ammo, memory, TAB, and chapter settle belong here, but small always-on metrics and large settle lists differ in density and timing—do not copy layouts blindly.

- Use screen coordinates and clear anchors; avoid crosshair, weapons, chat, use prompts, and settle UI. Keep labels near values, group related fields, use the component width well; avoid jitter when numbers change. Compact/full "look" and compact/full "information" may be separate settings—do not merge them.
- Keep neutral values neutral; occupancy, remaining, and risk are different semantics. Check units, validity, threshold direction, and source; keep existing warning colors; do not yellow/red every number by habit. Whether hiding a stats page affects scoring is a product rule, not a pure visual switch.
- Wire along existing game/Paint data consumers and Surface paint; check Wanted, preview, and producers. An independent switch must not only hide results while sampling continues; shared sampling may remain when other consumers still need it.
- Position adjust follows existing menus and screen scale. Demo data is bounded and distinct from live data; large panels such as TAB/chapter stats follow existing exceptions and do not join other HUDs' automatic position preview.
- Before changing look, list existing behaviors: crosshair hide, weapon switch, threshold colors, settle sorting, style/info switches, etc. Looking better must not drop those behaviors.
- Large backplate fill alpha at most 110. Forbid whole-panel backgrounds at alpha 180+; thin accent bars may be darker. Text readability comes from type, outline, or shadow—not from painting the panel solid.

**Accept:** Real combat and preview; Chinese/English; extremes/no data; different resolutions; multiple HUDs on; load/spectate/death/map-change/disable-then-reopen. Measure text first, then confirm row/column space—do not estimate width from character counts.

## 2. World HUD

**Goal: explain a target in the world, not a fixed on-screen status bar.** World names, entity cues, and nav arrows check world anchors and projection before discussing text, graphics, and color.

- Confirm the anchor (entity head, ground, route node, etc.), target identity, and lifetime; screen position comes from world projection. Behind camera, screen edge, projection failure, and target gone need clear degradation—do not paint failure coordinates into a screen corner.
- Check distance scale, occlusion, team, visibility, and realism-mode limits against current feature rules separately; do not generalize one name HUD's ignore-occlusion ability to every marker. Distinguish remote-server capability from local limits.
- Multi-target density has a budget; prefer information relevant to the current task; consider near overlaps, same-name targets, and behind-wall/off-screen states. Reduce information, aggregate, or offset when needed—do not solve crowding by shrinking type forever.
- Reuse existing entity snapshots, projection, and visibility data; gate by consumer. Trace, target scans, and text queries each have explicit ceilings/update conditions; do not add a per-frame full-entity scan for decoration.

**Accept:** Turning, near/far changes, occlusion, different FOV/resolution, screen edges, multiple targets at one place, entity destroy/index reuse, spectate, and realism limits. World-position and screen-position editing are not the same capability.

## 3. Independent overlay HUD

**Goal: present timely game state in an independent transparent window when clearer geometry is needed.** Current ammo arcs, reload rings, and post-fire/reload arcs belong here. Overlay is not "zero cost," "zero latency," or automatic support for arbitrary complex graphics.

- First verify the `HudOverlay*` / `HudLayer*` sample → publish → present chain: game/Paint only samples and publishes bounded snapshots; the UI thread only paints snapshots; do not read entities or call Source on UI/worker threads. Keep snapshot expiry and invalidation behavior.
- Design outlines, stroke width, and antialiasing for output pixels; check small icons after downscale. Bullet icons need a clear tip/case and correct orientation; decorative ticks, sparkles, and pointed cursors only when they help reading. Distinguish progress, base track, and cursor; readable on both bright and dark backgrounds.
- Arc angle, center, radius, crosshair-relative compass direction, and window position are different parameters. "Put it above/below/left/right of the crosshair" must not only rotate art at the old offset; state who is the center, the offset anchor, and reset semantics; verify edges and that the cursor tracks angle consistently.
- Keep independent switches and settings; do not depend on the original Surface HUD switch. Existing behavior such as hiding all Skeeto crosshairs during reload must check every crosshair consumer and restore after end/fail/off; multiple hide sources must not clear each other early.
- Reuse existing small-window, DIB/GDI+ resources and UI scheduling; redraw when snapshot content or size changes. Check transparent edges, premultiplied alpha, window bounds, and color; avoid clear-then-show flicker each frame. Clearer curves must not buy large windows, unbounded paths, or per-frame resource creation.
- Evaluate Paint publish rate, UI update rate, and game FPS separately; confirm throttle, dedupe, and latency from the current implementation—do not hard-code permanent budgets. On window hide, focus loss, load, resize, Paint stop, and unload, leave no top-level window or stale frame; preview comes only from the matching settings subpage.

**Accept:** Sudden progress jumps, weapon switches, four compass directions, fire/reload switches independently, color, scale, windowed/fullscreen/unfocused, together with static crosshairs, load and disconnect. Report clarity, flicker, sample latency, and frame time separately; do not infer total cost drop from fewer Surface calls.

## 4. Custom main-menu UI

**Goal: an editable, extensible, consistent mouse UI on top of stock capability.** It is an independent module; using Surface paint does not make it an in-game custom HUD and does not mean it should look like the numbered settings menu.

### Layout and interaction

- Build information hierarchy around game modes, settings, servers, personal data, and Skeeto tools. Common paths need few clicks; parameterized capability needs understandable entry points; users must not bounce to stock UI for basic tasks. Keep layouts editable; do not freeze default-preset coordinates as a universal standard.
- Group actions by purpose; keep similar parameterized actions adjacent (ConVar set, toggle, cycle, slider); distinguish "open a management page" from "run one operation now"; parameterized actions state format, limits, and examples. Server rows must not be mistaken for home-page rows; parent/child ownership is understandable; deleting a parent handles full descendants.
- Click, hover, drag, keyboard focus, back, and audio stay consistent. Reuse currently verified main-menu audio; do not borrow in-game numbered-menu sounds. Measure/scale fonts by resolution with bounded caches; long server text, English labels, and footers need clip or wrap and must not exceed background/hit bounds.
- Background opacity and unified/follow-button colors are appearance parameters—**they do not provide interaction isolation**. At any opacity (especially 0%), submenu layers must not show overlapping text or hit-test the layer underneath; paint order and hit registration stay in sync. Custom submenus and native mode flyouts may use different layouts; do not force every root item to show/hide together.
- Decorative text, marks, version lines, and hints may be hidden, adjusted, or deleted; existing user schemes keep their content and are not rebuilt because defaults changed. Separators limited by fonts use verified characters; editor input or a green compile does not prove the game font can draw a glyph.

### State, native collaboration, and save

- Check `MainUiBeforePaint`, page paint/hit-test, `MainUiActionAllowed`, and native entry points. Own input only in the current main-menu context; console, server panel, create-lobby, modal, and Steam UI must remain operable and must not be covered or click-through by Skeeto. When a custom underlay must stay, keep disabling its input; do not hide every stock layer to fix one occlusion.
- Share only verified background layers/resources with the background engine; switches and decode duties stay separate. Viewing background hides both menus but Esc restores; focus loss stops input and focus regain re-seeds edges. On exit, stop interaction and keep needed picture—do not block exit to keep a frame; on native identity/layer verify failure, restore stock safely.
- Normal use, edit draft, text input, action picking, and file dialogs each have clear state. Edit mode does not run button actions; hotkeys do not interrupt typing/capture or silently drop drafts via combinations such as Shift+Tab. Fixed keys and game binds are different mechanisms; configure from the current user request—do not infer defaults from old development behavior.
- Main-menu layout, button actions, and appearance use independent files and must not write into in-game HUD schemes. Valid personal layouts are not overwritten by default-preset updates; import validates first and restores on failure; which machine-local preferences do not travel with a scheme import is confirmed from current implementation. Unpublished formats get no extra historical migration.
- File, Shell, UDP, and record writes stay out of Paint; bounded tasks and snapshots reuse current services. Blocking file/text dialogs must not stall the shared worker. Visible pages request queries; network failure shows unknown rather than inventing offline; a successful status query is not proof of game connect capability.
- After entering a map, restore native panel ownership and stop main-menu-only sampling, input, and queries; independent record features or already-accepted save work run by their own switches and lifecycle—do not mislabel them as continuous main-menu paint. Enumerate real leftover work instead of claiming "completely zero cost."

**Accept:** Main-menu launch, opacity 0/100, Chinese/English/low resolution, nested submenus, back/delete/hide, final slider values, edit/cancel/import errors, native dialogs, console open/close, background on/off, focus loss, exit, and enter-map. Data/file tests and a green build do not prove in-game click, occlusion, or look-and-feel acceptance.

**Tutorials and delivery:** Explain parameter locations, allowed values, and copyable examples in player language; parameterless actions need only a short note. Existing installer tutorials/notes may be edited only from the desktop-package original after backup, following document-protection rules in `AGENTS.md` / `docs/agent-guides/release-and-updates.md`. Updating this Skill does not authorize regenerating or sync-overwriting them.

## 5. ESC-menu UI (independent first cut; editor live acceptance pending)

An independent `EscUi*` module already exists, defaults off, and stores config under `left4dead2/skeeto/escmenu/`; it does not reuse main-menu layout or disconnect gating. File ownership, budgets, formats, and concrete features follow the [module map](../../../../docs/agent-guides/module-performance-and-structure.md#hot-paths-and-module-map) and matching implementation records. The user has confirmed the home page, audio round-trip, and continue-game; the newer editor and other lifecycle edges still have live gaps—see `docs/feature-plans/SKT 游戏内ESC菜单自定义 - 可行性研究与实施方案.md` sections 13–23. Edit actions end the current frame and rebuild controls next frame; after paging or deleting items, do not keep using old control indexes. Session variables follow the shared skt_cvar contract—do not rewrite them for UI design convenience.

- State whether the menu appears in local, remote online, loading, dead, or spectate play; a remote game does not auto-pause because the menu opened. Design separately which data keeps updating and which input must be released.
- "Back to game" is highest priority; game settings, server/vote, and disconnect actions depend on current connection state and permissions. Do not copy the main menu's "must disconnect" execution gate or call unverified same-named native entries directly.
- First verify native pause panels, chat, console, Steam, mouse capture, input throughput, and panel ownership; open/close/enter-map/disconnect/unload must restore so the character does not move unexpectedly and the mouse is not still locked after close.
- Choose a paint path from real layering, focus, and lifecycle evidence; do not reuse main-menu Hooks/state merely because the main menu uses Surface, and do not assert that an existing transparent window can replace the pause menu.
- Layout, schemes, defaults, and data ownership are decided during development; do not pre-reuse main-menu `current.hud` or in-game HUD schemes. Verified base controls and resources may be shared; state and action context stay independent.

**Accept:** Separately build local/online, combat/spectate/load, Esc double-tap, modal return, focus/input release, and module-off scenes. File and state checks do not replace live interaction; report interfaces, save, input, and stability from evidence actually obtained.

Read-only widgets reuse existing snapshots; do not invent values for data the server did not sync. Edit geometry, selection, and guides follow current layout capability; temporary selections do not enter user schemes. HUD control buttons read real switch state; long labels clip while keeping the state suffix.
