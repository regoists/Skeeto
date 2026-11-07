# Skeeto handoff checks

Read selectively for this task; save only facts that would change take-over judgment. Current AGENTS and topic guides decide the rules; this page helps spot missing handoff facts.

## Common facts

- Which user goal is currently being satisfied? How do recent corrections affect the original task? What is done, cancelled, or explicitly deferred? Do not let the latest status question overwrite the whole goal.
- Locate with path plus symbol/entry, not only line numbers in a huge file; line numbers are only a then-current hint. After source moves, re-check along the current call chain.
- Git dirty does not equal this round's changes. Record staged, unstaged, untracked, deleted, and renamed; mark ownership basis; keep unknowns.
- Under the same HEAD, branch, and status, file content can still change. State hashes and index blobs detect differences; unrecorded/over-budget parts stay unknown.
- While the record is being generated, are other agents/users/builds still editing files? Before/after Git checks only find observable changes; multi-file capture is not a locked atomic snapshot. Compare again after finishing the handoff; the next agent also compares again.
- A machine-local snapshot preserves working-state evidence; it is not a backup of uncommitted source. Across checkouts or machines, state whether needed code and assets actually exist. Do not auto cherry-pick, apply patches, restore, delete, or overwrite.

## New features, HUD, Hooks, or entities

Only record what the current change involves:

- Where data is produced/consumed; main callers; actual thread; frequency/budget; re-entrancy; who releases resources.
- How switches and Wanted wire; missing-key defaults; module off state; reopen; map/round/disconnect/exit handling.
- Console/menu/ini entry symbols; whether English resources exist; which master switch it depends on; whether it adds ongoing scan/Trace/IO.
- Live-test map, player/bot counts, server type, idle takeover, resolution, and related conditions. Without performance data, do not state optimization gains as settled fact.
- Binary ABI, signature, or vtable conclusions need target module, bitness, version/hash, uniqueness/call-site evidence, and degradation. SDK or third-party code is only a clue.

## Triage and failed directions

- Separate user observation, logs, source inference, and proven causation. Cannot-reproduce does not mean absent; one user report does not prove every environment.
- Keep the minimal operation order and change points; for example "close window → open and close another panel → reopen restores" is better for re-check than "fixed transparency."
- For abandoned experiments, state actual effect, off-state impact, performance/stability cost, and whether it was removed from source/DLL; do not let a new agent treat a withdrawn design as current behavior.
- Failed directions attach then-current target/version and evidence. When the current topic guide already records them, link and summarize instead of pasting the full text into every handoff.
- Do this test and historical tests use the same source/DLL? One old green build cannot endorse later dirty source.

## Build, deploy, installer package, and Git

- Follow `docs/agent-guides/release-and-updates.md`. A handoff itself does not trigger build, overwrite game DLLs, bump version, update packages, or commit/push. Preserve full scope and execution state for authorization already granted; a new conversation handles it under the current request.
- Record the actual build command and exit result. Read source-of-truth tree, generated mirror, and version number from current project entry points; do not bake their temporary state into the Skill.
- Separately record `runtime/`, `packages/Skeeto-dev/`, build outputs under `packages/Skeeto/`, game root, exported desktop `Skeeto_*` package, and `release/` state. An ordinary build or a release/version bump that updates `packages/Skeeto-dev/` does not prove a desktop installer folder exists. `packages/Skeeto-dev/` is the package master; desktop `Skeeto_*` is a user copy of that master; `packages/Skeeto/` is a build.bat output location (dll + source snapshot), not a player or master pack.
- When game-held copy is skipped, record "build succeeded, target not replaced"—do not vaguely say deployed. Do not end the user's game or poll-wait just to finish a handoff.
- Matching DLL file hashes only prove that file's content. Whether the game reloaded, whether Necola/independent tracks were live-tested, and runtime effect need separate evidence.
- For release tasks, hand off unfinished installer notes/changelog/command index/user Skill items and their sources of truth. Do not forget release docs because only source is remembered, and do not auto-copy this development Skill into player packages.
- Commit, merge, and push get separate real identifiers and check scope. Keep the uncommitted-change list; do not write "clean" in a handoff to hide other work.

## Docs, attachments, and long-term maintenance

- `docs/START_HERE.txt` keeps stable entry points; AGENTS and the current module map/guides decide task routing. This Skill only defines handoff method; module growth does not require editing a static feature table here.
- Keep status, version, and evidence bounds at the top of plans or reports; "for reference only" is not a user implementation command. When implementation is explicitly requested, continue the authorized goal.
- `reference/`, `archive/`, machine-local tools, and temporary screenshots may not travel with Git. Record key facts and conditions; attachments need the same machine or a completed transfer to be readable. Do not leave only a temporary path the next agent cannot open.
- Exact drafts that are not yet on disk but must continue may be saved as attachments in this record and marked "draft / not executed." For existing source, give symbols and paths only—do not copy whole modules.
- On missing Python/Git, Git command failure, unknown schema, over-budget, or file-read failure, use the manual path and state the incomplete scope. Empty output or an exception is not established fact.
- When multiple handoffs exist, locate by task and workspace; with a clearly named older record, do not substitute another task's newest record. Each record continues independently; older records are for tracing only. Do not auto-delete history or maintain an ever-growing "all historical tasks" list.

## Done criteria for a new agent taking over

The agent can accurately say what the user wants, where things stand, which conclusions have which evidence, which files must be protected, and the next concrete action inside current authorization. Take-over feedback only states one or two differences that affect work, then proceeds. Do not restate the whole project to the user, do not redo valid existing verification, and do not treat unevidenced historical claims as acceptance.
