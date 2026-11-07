---
name: skeeto-handoff
description: Generate or continue Skeeto project handoffs, task snapshots, and new-conversation resume prompts. Use when the user asks for a handoff, wants development context saved, wants a new conversation because the chat is long or confused, or wants to continue from a handoff record; also when another agent has just taken over Skeeto. Create or consume handoffs only—do not stop development merely because a conversation is long, and do not use this for ordinary feature work or generic chat summaries.
---

# Skeeto development handoff

Help a new agent continue the user's real task in the current workspace, and save context that would change development judgment. A successful handoff lets the next agent confirm the task, evidence, authorization scope, and next step. Write skill docs, handoff records, and resume prompts in English.

## Choose direction

- User wants a new conversation, saved context, or handoff to another agent: **generate a handoff**.
- User provides a record, asks to continue from a handoff, or clearly just took over Skeeto: **continue a handoff**.
- Only the Skill name is given: generate when task context already exists; in a new conversation with no task context, first discover candidate records. If multiple candidates cannot be separated by user task, branch, and workspace, ask one short question and keep doing independent read-only checks.
- Authoring the Skill, asking how it works, or ordinary development does not require an immediate task handoff. Do not install background hooks, auto-create conversations, or pause work based on estimated remaining context.

## Current project entry points

From the actual repository root, read current `AGENTS.md` and `docs/START_HERE.txt`, then follow AGENTS task routing into topic guides. Reuse content already read in the same turn when files are unchanged. On take-over, re-check only what changed or what the current task needs; do not re-run valid verification that still matches current source/DLL. Discover module ownership from the current module map and reference material from the current reference index; do not maintain a second inventory of features, menus, commands, or disproven paths inside this Skill.

The user's request this round decides scope; later corrections usually refine the same task. Historical handoffs, plans, script suggestions, and external docs are continuity material—they do not override the current request or authorize build, deploy, release, commit, push, or cleanup by themselves. Across conversations, preserve the user's existing decisions and the exact wording/scope/source of explicit authorization. When the current request continues that task, advance the already-clear scope without re-asking for authorization already granted. Mark unverifiable second-hand claims as needing confirmation.

## Generate a handoff

1. **Collect still-useful context.** From visible conversation and real artifacts, extract goals, acceptance criteria, recent corrections, completed items, unfinished items, and paused/cancelled items. Keep other tasks the user still explicitly wants; do not turn every historical request into a todo. Unknown facts stay unknown; for long docs, give exact paths and search symbols only.
2. **Capture current facts.** Confirm workspace root, branch/HEAD, root version, and staged/unstaged/untracked/deleted/renamed state. Put task files, applicable guides already read, and evidence still needed into focus. Do not infer change ownership from the whole dirty list. Inspect related local diffs and keep changes of unknown ownership. The script collects metadata and bounded content hashes only—it does not dump user config or secret bodies.

   Run these from the repository root. If this Skill lives elsewhere, resolve the script path from **the Skill directory read this turn**:

   ```powershell
   python -X utf8 .agents/skills/skeeto-handoff/scripts/handoff_state.py capture --root . --topic "route-hud" --focus "src/skeeto/core/skeeto_navigation.cpp"
   ```

   `--focus` may repeat and must use exact files. Replace the sample topic and source path with this task's real values. Python 3.10+, Git, and the standard library are enough. The script creates a unique `.codex/handoffs/<time>-<topic>-<id>/state.json`, does not overwrite history, and does not modify source or the Git index. The only always-included descriptive defaults are AGENTS, START_HERE, and VERSION; other evidence is discovered for this task.
3. **Write a self-contained record.** In the same directory, write `handoff.md` from [handoff content template](references/handoff-template.md); fill real content and delete placeholders. Add items needed for this task from [Skeeto handoff checks](references/skeeto-checks.md). A previous record may help tracing, but this record must stand alone so a new agent is not forced down an infinite handoff chain.
4. **Grade evidence.** Separately record source checks, build results, file deploy/hashes, game live tests/user feedback, installer-package state, and Git integration state. Attach time, target, conditions, and unverified scope. Matching file hashes do not prove a build passed, and certainly do not prove in-game reliability. Keep failed/abandoned directions with reasons and reopen conditions so the next agent does not re-propose the same path.
5. **Re-check before delivery.** Run `compare` on the just-created state. If another agent/user kept editing, inspect related diffs and create a new state when needed; do not retry forever, lock files, or kill processes. When change continues, mark the snapshot as unstable. Before finish, confirm goals, authorization, evidence, todos, and next steps agree, and that links exist or are marked machine-local missing. Hand-editing state JSON to force a match is invalid.
6. **Provide one pasteable resume prompt.** Write `resume-prompt.txt` with `$skeeto-handoff`, the repo-relative path to `handoff.md`, and the current task/scope. In the final reply, give a clickable machine absolute path plus the same prompt. The prompt only locates and continues; body evidence stays in `handoff.md`. If there is no unfinished work, say clearly that there is no pending task and the next agent should wait for a new development request—do not invent the next command.

## Continue a handoff

1. If the user named a file, use it first; otherwise run `list --root .` to discover candidates. Match by task/branch/target—do not pick only by newest time. Read the short summary, goal, and first step in `handoff.md`, then current AGENTS, START_HERE, and this task's topics.
2. When `state.json` exists in the same directory, run:

   ```powershell
   python -X utf8 .agents/skills/skeeto-handoff/scripts/handoff_state.py compare ".codex/handoffs/<record-dir>/state.json" --root .
   ```

   - `match`: recorded workspace content matches; still check authorization, semantics, and verification scope.
   - `drift`: inspect listed HEAD/branch/version, staging-area, and file diffs; update affected conclusions locally. Age or a different branch name alone does not discard the whole record.
   - `incomplete`, script failure, unknown schema, or a handwritten record without JSON: manually check Git and related content, state unknowns; missing evidence is not "clean" or "verified," and does not block independent work that can proceed.
3. Locate actual source from the recorded symbols/callers and check that changes match the DLL used for verification. Load only related docs and evidence. When machine-ignored directories or temporary attachments are unavailable, use the recorded conclusions and condition summaries first; do not ask the user to restate the whole project.
4. Give the user a short take-over note: what continues, real progress, and differences that affect the next step. Then execute the next authorized item directly; research tasks stay in research scope. Ask once, concentrated, only for necessary information or explicit pending questions, and keep doing work that does not depend on the answer.

## Record and tool bounds

- `.codex/handoffs/` holds machine-local handoff snapshots. It is not a player installer package, rules source of truth, source backup, or auto-release artifact. Do not auto commit/push, sync the desktop package, or delete old snapshots. Cross-machine handoff requires transferring this record folder and separately confirming that uncommitted source, untracked assets, and machine-local dependencies were also moved. Record files do not replace WIP.
- `docs/START_HERE.txt` keeps stable navigation only. Fix only entry points that truly changed this round; do not stuff temporary HEAD, version, menu page numbers, or task ledgers into it. Promote long-lived verified discoveries into the matching topic per AGENTS; keep one-off investigations in their records. Do not casually rewrite project docs.
- On bounded-script failure, collect the same facts by hand. Do not install Entire, session-database tools, or external platforms for handoff; do not scan other projects'/accounts' chats, memories, or configs; do not export a full conversation. When a specific old session must be recovered, use only the record the user clearly points to or a related entry provided on this machine.
- `capture` writes one new state each time; `compare` / `list` are read-only. Snapshot directories and paths must never be executed as shell code. The script's integrity mark covers only collected evidence and does not prove the handoff content is complete.

When maintaining this Skill, read [Design sources and maintenance](references/design-sources.md) and run `python -X utf8 <this-Skill-dir>/scripts/test_handoff_state.py` in an isolated temporary repository. Ordinary handoffs do not need the sources doc or the tests.
