# Module, performance, and code-structure rules

When adding/changing modules, hot paths, checks, fallbacks, or lifecycle, read the shared boundaries first, then locate related sections from the module map. Read concrete budgets and historical verification via links as needed; source and live measurement beat records. Do not split a module first merely because a file is large.

## Performance and stability

- Accept optimizations over the full same-condition target interval, including setup, hit/miss, maintenance, sync, IO, and cleanup; separate first load from warm cache. Diagnostics must be off or their disturbance quantified; moving cost until after map enter does not count as gain. When the difference does not exceed test noise, record it as no detected net gain and withdraw or simplify mechanisms with no gain.
- Paint/FSN/CreateMove/Event/recv scans, Trace, loops, queue consumption, and resource creation have budgets and ceilings. Reuse already-valid data; non-per-frame needs update by event or throttle; do not sacrifice interaction precision. Without evidence, do not add threads, locks, caches, or state machines for tiny gains.
- Source/L4D2 internal interfaces are called only on known-safe game/Paint threads; workers do not call engine, cvar, command, or entity APIs. Event/recv only publish light state / bounded queues and do not Trace, create particles, or do disk IO directly.
- `IN_KeyEvent` does not repeatedly reinstall VMTs and does not call `SaveSettings` / `ClientCmd`; when the original function is invalid, it must not swallow all keys. `HudVguiSearch` obeys budget/depth; fonts/textures and similar are reused by lifecycle—changing release policy first checks engine ownership and cache references.
- Hot paths must not add blocking work such as Steam, disk, MessageBox, or saving settings. Existing features that must use engine commands keep their verified paths; new command calls state throttle, thread, and reset clearly, and do not tear down normal behavior for a literal rule.

## Memory readability checks (VirtualQuery red line)

- `VirtualQuery` can reach about 100–200µs in large continuously committed regions; P0 was once triggered by stacking roughly 430 per-field queries. Hot paths must not use it directly.
- Game memory reads use `core/skeeto_memread.h` uniformly: entity fields via `EntRead` / `EntReadBlock` / `EntWrite` / `EntLive` / `EntFieldOk`, ordinary pointers via `Probe` / `Copy` / `CopyCStr` / `SpanProbe`. `PtrCommitted`, `EntReadable`, `IsExec`, and `IfaceAlive` are already centralized; this only lowers access-fault risk and does not prove identity, liveness, type, or ABI. Entities are still validated by handle/serial, userid, or existing generation.
- `Executable` still uses forward per-page cached page lookup; `ReadableRange` is only for non-high-frequency paths that truly need whole-span judgment. Cold queries prefer counting `skmem::Query(p, &mbi, (uintptr_t)_ReturnAddress())`. Address-space scan/crash paths that truly cannot go through skmem must register direct queries in `ALLOWED` of `tools/check_memory_probes.py` by file+function with reason and call ceiling—not whole-file exemptions.
- For code changes involving memory access, Hooks, or query registration, run `python tools/check_memory_probes.py`. Diagnostic `skeeto_memprobe` shows page-query counts, protected-read faults, and top call sites; RVAs are matched against the same-origin MAP. Readable prechecks cannot replace use-site protection and cannot eliminate invalidation between check and use.

## Basis, cost, and bounds for defensive checks

For added/changed checks, verify four things; no separate proof file is required:

1. **Failure scenario:** Which reachable boundary, external input, or supported contract does it correspond to? An incident need not have happened first, but "might be useful later" alone is not enough.
2. **Who already guarantees it:** Within the same object and same valid lifetime, has an upper layer already guaranteed it? After cross-callback, async, or generation change, re-judge—do not mechanically check only once.
3. **Frequency × cost:** Does it include syscalls, allocation, locks, formatting, or state growth? Simple compares are usually cheap; SEH normal-read and fault costs differ—use counters/profiler for magnitude.
4. **Behavior after failure:** Does it stop safely so callers can recognize failure? Do not return fake defaults/fake success, and do not treat user-off or explicit empty config as errors.

- Keep necessary boundaries: Hook/vtable/interface acquisition and slots, index/handle-to-entity and generation, ini/file/ConVar string/network input, cross-thread, objects after map change/disconnect/unload, and memory modified by third parties.
- Delete layer/field probes proven redundant; cheap extra cold-path checks may be noted, but do not delete protection recklessly for neatness. When failure paths truly need observation, reuse existing state or fault counters; ordinary null checks, no-target, off state, and side-effect-free pure functions do not require new counters/logs.
- When a symptom comes from old design, state root cause, change scope, and normal behavior, then fix at the root within authorization; out-of-scope refactors need separate confirmation. For performance issues, locate with counters/profiler first, then decide delete, move, cache, or keep—do not stack state by feel.

## Game small-update hardening and feature-equivalence verification

- Write clearly "entry → failure condition → dangerous call/write → impact". Without a required dependency, do not publish ready or start consumers; missing optional dependencies do not expand into whole-module disable; write-permission failure must not fall back to bare writes.
- First fix the normal path: parameters/returns, field strings, call order, native upstream, coexistence chain, permissions, settings and save timing. Prefer changing only clear failure branches; when helper return semantics change, check all callers. Evaluate existing doubts item by item; do not rewrite normal bindings or globally lock based on speculated future ABI.
- Against the clear pre-fix content, choose source, extracted production-function differential fixtures, binary, or same-condition live evidence by risk; check integers/decimals, boundaries, already-saved-on state, and evidenced coexistence paths. Compare state and side effects; do not only write asserts shaped like the new implementation, and do not count an entire dirty workspace diff as this change.
- Inject this change's matching missing-interface, write-denied, and similar conditions; check no wrong call/write, no fake ready/fake success, and effective cleanup. For multi-step writes, separately check partial results after later failure and page-permission restore; without rollback, do not call it an atomic transaction.
- Binding and admission live on init/enable/existing lifecycle edges; do not add per-frame hashes, page queries, scans, unknown-slot trial calls, or baseless version whitelists. Runtime that cannot keep user settings and intent must not pass tests by resetting config, and must not early-return around still-valid cleanup.
- Update the current conclusions of the original review report; separately list reproduced/fixed items, intentionally retained regression risks, and live gaps. Source equivalence, offline tests, build, game feature/quit, and performance data are stated separately; do not claim safety against arbitrary future updates.

## P0 retrospective: general lessons

- Shared helpers must be read for implementation to spot hidden syscalls, locks, allocation, and IO; judge call volume and per-call cost. When runtime attribution is truly needed, reuse diagnostics—do not force instrumentation on every helper.
- Cost may grow with memory region, entity count, chapter count, session length, and resource accumulation. Caches/queues/lists/handles/maps/threads state ceilings and cleanup edges; related performance acceptance covers continuous map changes, long sessions, or max state—not only just-after-launch.
- Mitigations such as turning HUD off or resetting display may only change call volume or memory layout; "cold path / default off / rarely triggered" needs matching counters or runtime evidence. Without evidence, record as a clue, not as root cause fixed; default-off also does not prove cheap when on.
- For hard-to-reproduce issues, first make the next reproduction attributable: bounded counters, rate-limited logs, or existing profiler; diagnostics themselves count toward cost. New infrastructure checks actual callers and valid offline tests; effectiveness conclusions that involve game behavior need matching runtime evidence.
- Check hidden cost by change: game-thread locks/waits, exception-retry storms, per-frame allocation/resource creation, state that only grows, disk and logs, unbounded scans, and unexpected DLLs. Paint must not block waiting for workers; try-lock failure must not busy-retry. Time/deadline covering high bits and wrap: see `tools/test_memread.cpp`.
- When hot paths, shared layers, or resident resources are involved, delivery briefly states thread, frequency/workload, growth ceiling, and cleanup; report measured cost/gain only with measurement, otherwise state evidence bounds—do not invent nanosecond numbers.

## Exit and module unload

- Distinguish owned threads/handles, game Hook slots, and engine/entity objects. `g_run`, DLL still mapped, and non-null pointers do not prove the target module is valid.
- On disable/map change/disconnect, at existing game/Paint safe points restore still-valid owned changes and clear state; do not delete runtime cleanup while fixing exit. Process termination leaves address-space reclaim to the system; do not risk late writes into game modules.
- After a worker exits because the module disappeared, a window was destroyed, or an interface invalidated, only handle resources still owned; atomic pointer writes likewise require the target to be valid and ownership to hold. A core pinned until process end does not add logic for a nonexistent hot-unload; if hot-unload is truly required, first stop callbacks, restore Hooks, and finish concurrent teardown.
- The `DLL_PROCESS_DETACH` terminate branch does not wait for threads, walk entities, call Source, or write modules back; `lpReserved==nullptr` also does not guarantee other modules are valid. Do not mask timing issues with per-frame module probes, whole-block prechecks, global exception swallowing, or closing L4N dialogs.
- Waits on owned UI/input threads must be wakeable by exit and must not depend on later Paint; window/Hook/drawing resources are released by their owning thread. Timeout does not mean the thread has ended; do not destroy shared GDI+ early. Synthetic keys record ownership only after actual successful send, and handle exit races during send.
- When Hooks/workers/exit cleanup are involved, under current live authorization accept normal quit from main menu, in-map, and after disconnect-back-to-menu, plus related enable/disable/map-change and Hook not-fired/mounted/restored states; use the actual Necola/L4N coexistence environment. Force-kill, green build, and no dialog do not replace exit evidence; when forbidden or unable to live-test, list gaps and do not start the game on your own.
- Historical basis: `docs/troubleshooting/投掷提示、红点局与团灭计数_2026-10-15.md`; the 2.1.0 core once late-restored RecvProp after client unload and caused access faults; later addresses must be rematched to the actual DLL.

## Module isolation and new-feature wiring

- New player-facing active features complete the applicable items: switch and missing-key default, config read/write, menu/command entry, demand gating, enable/disable and map-change/disconnect/exit cleanup; paint and input stay in sync. Shared wiring is usually in proxy; state stays in the owning module. Do not force every item to add `g_opt*`, a menu, or INI. Internal helpers, SDK capabilities, and fixes to existing features wire by actual consumers.
- The master feature defaults off; internal parameters may have reasonable defaults, applied only for missing keys, while keeping existing user config. In a stable off state there is no dedicated picture/sound/engine impact, scan, Trace, load, or state update; edges allow necessary bounded restore/save/cancel; shared work is kept for other consumers' needs.
- Reuse existing `Wanted()` and clean by source; must not mis-clear shared caches, leave replayable stale tasks, or put one-shot cleanup into every frame. Elim still reads `g_optElim` and does not invent another aggregate.
- Driver-style modules that truly must run before the early return in `kPaintInGamePanels` may follow `ListenPump`/`XhairTick`/`SpotlightVisionNeedsTick`: a few O(1) gates, state edge cleanup; periodic reads are throttled, bounded, and self-terminating, with reasons at call sites.
- Fallback is judged by feature contract: user off, explicit empty config, missing resource, and execution failure are handled separately and do not cross the user's explicit channel or source choice. Behaviors with real purpose such as same-package inheritance or send-path degradation must be confirmed along the call chain and kept.

## Hot paths and module map

This table is the single maintained place for file responsibilities and search entry points. First look up source by file/prefix and the search-entry symbols in the third column. Keep this table in sync when adding/moving/merging modules; README and handoff keep only navigation. Locations such as `core/` below are relative to `src/skeeto/`.

| Responsibility | Source location (relative to `src/skeeto/`) | Search entry / on-demand record |
| --- | --- | --- |
| Numbered menu search | `core/skeeto_menu_discovery.h` (bounded match), `core/skeeto_menu_discovery_host.h` (navigation, text request, and current-page paint), `core/skeeto_menu_discovery_descriptions.h` (purpose metadata when existing description is missing), `core/skeeto_menu_catalog.generated.inc` (compile catalog); generator entry `tools/generate_menu_discovery.py` | `MenuDiscovery*`; compute on confirmed query; Paint only draws current-page cache; return context reuses `MenuReturnFrame` |
| First-person left/right mirror toggle | `core/skeeto_proxy.cpp` | `ViewModelMirror*` |
| Viewmodel position, angles, pivot, and scale | `core/skeeto_proxy.cpp` | `skviewmodel::*` / `ViewModelScale*` |
| VPK map-pack and file-filter experiments | `core/skeeto_vpk_experiments.h`; host wiring in `core/skeeto_fast_load.h`, `core/skeeto_proxy.cpp` | `skvpkexp::Controller` |
| Load-time texture warmup experiment | `core/skeeto_texture_warmup.h`; `TextureWarmup` wrapper in `core/skeeto_fast_load.h`; menu in `core/skeeto_proxy.cpp` | `fast_load_less_texture_warmup` |
| Dispatch, Hook, menu, settings, ConVar, event routing, input, crosshair, Listen, and ClientUx | `core/skeeto_proxy.cpp` | `Hooked_EnginePaint` |
| Throwable trajectory and landing preview | `core/skeeto_proxy.cpp` | `ClientUxPaintThrowLand` |
| Damage-event classification strategy | `core/skeeto_damage_events.h` | `skeeto_damage::*` |
| Third-person aim correction (Skeetoscript) | `core/skeeto_proxy.cpp`, `core/skeeto_tps_aim_aac.h` | `TpsAim*` |
| Hold-to-fire for semi-autos (Skeetoscript) | `core/skeeto_proxy.cpp` | `ClientUxHoldSemiPump` |
| Anti-crash patch integration | `core/skeeto_proxy.cpp` | `CrashGuard*` |
| Cache offload and reclaim | `core/skeeto_cache_offload.cpp/.h`, `core/skeeto_cache_protocol.h`, `cache_host/skeeto_cache_host.cpp` | `skeeto_cache::StartWorker / Pump / RequestReclaim / StopWorker` |
| Teammate and world HUD | `core/skeeto_hud.cpp`, `core/skeeto_hud.h` | `TeamHud*` |
| Elimination-stat persistence | `core/skeeto_elim_persist.cpp`, `core/skeeto_elim_persist.h` | `ElimPersist*` |
| Navigation, minimap, routes, chapter distance estimate, and supplies | `core/skeeto_navigation.cpp`, `core/skeeto_navigation.h`, `core/skeeto_navigation/` | `skeeto_nav::Pump` |
| Laser-round notice | `core/skeeto_proxy.cpp` | `LaserRound*` |
| Teammate throwable notice | `core/skeeto_proxy.cpp` | `ThrowNoticeRecv/ThrowNoticeHudPump/ThrowNoticeConfigure` |
| Load optimization (experimental) | `core/skeeto_fast_load.h`, `core/skeeto_staticprop_groups.h`; menu/commands in `core/skeeto_proxy.cpp` | `skload::Configure/SetLoading/SplitPath/NextCandidate/Observe/Detach` |
| Supply radar | `core/skeeto_navigation/supply_radar/`, `core/skeeto_navigation.cpp/.h` | `supplyradar::*` |
| Main-menu background engine, independent saferoom music, and rain love | `core/skeeto_background.cpp`, `core/skeeto_background.h` | `SkeetoSafeMusicResourcePump` |
| Skeeto plugin platform (optional) | `core/skeeto_plugins.cpp/.h` (load/ABI/bounded snapshot), `core/skeeto_plugins_menu.h` (independent menu adapter), `plugin_sdk/` (public C headers and HelloMenu sample); proxy wiring only | `skplugins::*` |
| Native chat box and console style | `core/skeeto_nativeui_config.h` (pure config/bounds), `core/skeeto_nativeui_settings.h` (settings snapshot), `core/skeeto_nativeui_runtime.h` (native control bind/restore), `core/skeeto_nativeui_menu.h` (shared menu order), `core/skeeto_nativeui_commands.h` (precise settings commands); proxy wiring only | `NativeUi*` |
| Custom in-game ESC menu UI (multi-level flyout; live acceptance pending) | `core/skeeto_proxy.cpp` (settings / Paint / native bridge / worker tasks), `core/skeeto_escui_layout.h` (128 elements / parent tree / format), `core/skeeto_escui_io.h` (bounded I/O), `core/skeeto_escui_editor.h` (paint / input / draft), `core/skeeto_escui_geometry.h` (pure layout multi-select / align / snap), `core/skeeto_escui_native.h` (home paint and hit takeover / native active window / native vote), `core/skeeto_escui_extras.h` (read-only info widgets / preset interaction), `core/skeeto_escui_presets.h` (8-slot ConVar value docs / worker storage), `core/skeeto_menu_params.h` (pure slider param rules shared with main menu) | `EscUi*` |
| Custom main-menu UI experimental build | `core/skeeto_proxy.cpp` (Paint / native wiring), `core/skeeto_mainui_layout.h` (bounded model / serialization), `core/skeeto_mainui_editor.h` (home / editor) | `MainUi*` |
| Custom home services and main-menu schemes | `core/skeeto_mainui_hub.h` (Paint pages and scheme apply), `core/skeeto_mainui_config.h` (independent main-menu config capture / validate / save gating), `core/skeeto_mainui_services.h/.cpp` (fixed snapshots/tasks, Win32 files and Shell, UDP), `core/skeeto_mainui_battle_days.h` (fixed 7 calendar-day totals, atomically saved with history.bin and details), `core/skeeto_mainui_report.h` (browser battle report generated on worker only when explicitly viewed; at most 10 detail reports; trends accumulate independently by day; Steam career stats are read once on the click action's Paint thread via CareerArchiveRefresh and copied into the task—worker does not call Steam) | `MainUiHub*` |
| Spotlight night vision | `core/skeeto_spotlight.cpp`, `core/skeeto_spotlight.h` | `SpotlightVision*` |
| Independent overlay HUD | `core/skeeto_proxy.cpp`, `core/skeeto_hud.cpp` | `HudOverlay*` |
| Menu / HUD shared style | `core/skeeto_style.cpp`, `core/skeeto_style.h` | `PanelStyle*` |
| Career archive, workshop entry | `core/skeeto_career.*`, `core/skeeto_workshop.*` | `CareerArchive*` |
| Memory reads and readability checks | `core/skeeto_memread.h` (`skmem::`, header-only) | `tools/check_memory_probes.py` |
| Shared declarations and light infrastructure | `core/skeeto_host.h`, `core/skeeto_runtime.h`, `core/skeeto_filesystem.h`, `core/skeeto_ui_text.h`, `core/skeeto_ui_en.inc` | `UiText` |
| Steam load entry and coexistence | `src/skeeto/bootstrap/wininet_proxy.cpp`, `core/skeeto_bootstrap_{abi,runtime,common,pe}.h` | `skeeto.dll` |
| Install and transactional maintenance | `tools/bootstrap/lifecycle/`, `tools/package_bootstrap_migration.py` | `skeeto_lifecycle.ps1` |
| Build and sync | `src/skeeto/build.bat`, `src/skeeto/bootstrap/build_bootstrap.bat` | `runtime/dll` |
| Hit feedback and workshop asset catalog | `core/skeeto_proxy.cpp`, `core/skeeto_style.cpp/.h` | `Feedback*` / `AssembleSiFx` / `DlcCatalogRecovery*` |
| Map chapter and names | `core/skeeto_hud.cpp`, `core/skeeto_mission.h` | `MapHudMission*` / `MapHudChapterCounts` |
| Campaign danger outline, laser red-dot, and interactable-item scan | `core/skeeto_proxy.cpp` | `CampaignOutline*` / `InteractScan*` |
| SkeetoScript extra effects and attachment hide | `core/skeeto_script_extras.h`, `core/skeeto_proxy.cpp` | `ClientUxBackItems*` / sounds and short flash / `SupplyScan*` |
| Damage numbers, friendly-fire HUD, and chapter stats | `core/skeeto_proxy.cpp`, `core/skeeto_hud.cpp` | `ClientUxPaintDmgNums` / `DrainDamageEventQueue` |
| Idle person restore | `core/skeeto_proxy.cpp` | `IdlePersonRestore*` |
| Jockey skeet | `core/skeeto_proxy.cpp` | `VictimWasAirborne` |
| Weapon magazine capacity | `core/skeeto_proxy.cpp` | `WeaponClipCapacity` / `WepClipCapResolve` |

## Code layering

- Use the module map above as the sole file-responsibility index. New features first reuse the nearest complete module boundary; only split a new module when state, cache, callers, and lifecycle can move together and there is real independent maintenance value.
- `skeeto_proxy.cpp` continues to own shared dispatch and user wiring; menu, settings, and event routing must not be split into fragments that are hard to cross-check merely because the file is large.
- `skeeto_hud.cpp` keeps the TeamHudSlot nest intact; do not move menu pages, ini, crosshair, Listen, or Spotlight into it.
- `skeeto_host.h`: existing shared kernel declarations for proxy; definitions stay in proxy. Do not pile new modules, HUD, crosshair tables, debug variables, or engine interfaces into it.
- Independent modules follow existing responsibilities in the map above; do not split `.cpp` files only to reduce line count or by function-name prefix zero-cut.
- `skeeto_spotlight.cpp` keeps the SpotlightVision nest intact; `ClampedOffset` that depends on `EngTrace` stays in proxy. Spotlight headers export only nest APIs—no `Log`, `SaveSettings`, `g_paint`, or host interfaces.
- Do not zero-cut Paint, menu, `HandleEvent`, Settings, Xhair, TeamHud, Listen, or Spotlight by function-name prefix; if a move is truly needed, move state, cache, callers, and lifecycle together and verify with a build.
- Do not expand the `Extract scar` region of `skeeto_host.h` (Ev/GetIface/VGet/ThrowNotice/LocalPlay/`kXhair*`/`g_dbg*`); avoid referencing it when possible.
- The nest-local prototype area at the top of `skeeto_hud.cpp` holds only necessary internal forward declarations; do not copy them into a public header API.
- Comments state only constraints such as thread, lifecycle, Wanted, and forbidden zones; do not restate the next line. New file headers follow the 3–4 line career/elim banner style.

## Logging

- `Log()` is off by default; when off it does not format, touch FILE, or create `skeeto.log`. New logs are rate-limited and not written per-frame/per-event without budget; expensive arguments stay behind the same guard as the log.
- When introducing temporary diagnostics, state the problem and deletion condition; after the fix and targeted verification, remove the command, request, consume pump, no-consumer dedicated counters, and current command docs together. Keep offline regressions and historical troubleshooting records; long-lived general profiler/fault counters/product state need ongoing purpose and do no dedicated sampling when off.
- `skeeto.log` only appends and has no rotation; when handling long logs, extract only the relevant range; move into ignored `archive/` only when archival is needed and authorized this round. `skeeto-debug.ndjson` currently has no writer; `L4D2-Necola.log` belongs to Necola.
- When tracked docs depend on machine-local `archive/` or `reference/`, keep conclusions, readings, conditions, and necessary hashes sufficient to continue work, and mark that originals are machine-local only—do not leave only inaccessible paths.
