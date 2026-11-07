# handoff.md content template

Trim optional sections for the task; keep the structure so key context has a clear place. Keep the opening to about 15 lines and the body usually to 100–180 lines; put detailed evidence for complex investigations in dedicated records. Do not delete critical conditions just to meet a line count. Replace every bracketed placeholder—never deliver an empty template.

```markdown
# Skeeto handoff: [current task or work theme]

Purpose: development continuity record, not a new user command; on take-over, check the current request and workspace.
Recorded at: [timestamp with timezone]; state snapshot: state.json; prior: [optional relative path to a previous record, for tracing only]

Goal: [result the user wants, and whether current scope is analysis/implementation/review/release/etc.]
Status: [where things stand; if the task is done, say clearly there is no pending development]
First step: [one concrete action and the evidence/result it should produce; matches Next steps below]

## User intent, corrections, and authorization

- Original goal and acceptance: [behavior, performance, stability, default-off / target servers, and other real requirements]
- Latest corrections: [meaningful exact copy, feature dependencies, placement/key rules; which older requirements were replaced]
- Explicit authorization: [user's exact words or a verifiable message pointer, applicable task and actions; done / not done]
- Current bounds and pending questions: [analyze-only / no version bump / etc.; say none if there are no pending questions]

## Current state and change ownership

- Repo-relative entry points: [AGENTS, START_HERE, related topics]
- Branch, HEAD, version: [from current files/Git; not bound to a machine path]
- This round's changes: [file → role → done/undone → ownership basis]
- Pre-existing or others' edits: [protect explicitly; mark unknown ownership as unknown—do not claim or revert]
- Staged, unstaged, untracked, deleted/renamed: [point at the state detail list; body only highlights what affects later work]
- In-progress work: [unfinished functions/config, on-disk patches/drafts, pending queues/debug state]

## Decisions and needed understanding

1. [What was decided; user decision / implementation judgment / assumption; why; conditions; source symbols or record evidence]
2. [Key call chain, data sources, threads, and lifecycle; only what this task will use]
3. [Rejected / deleted / failed directions → observed failure and evidence → reopen conditions]

## Verification and evidence bounds

| Layer | Result and time | Object / conditions / evidence | Still unproven |
| --- | --- | --- | --- |
| Source check | [actual result] | [symbols, paths, related diffs] | [runtime unknowns] |
| Build | [pass/fail/not run] | [command, version, source state, original output location] | [does not replace live tests] |
| Deploy | [ok / skipped while held / not run] | [exact targets, DLL identity, hashes and compare scope] | [whether actually loaded] |
| Game live test | [local live / user feedback / unverified] | [repro steps, map/server/mode/background format, related conditions] | [frequency and environment bounds] |
| Installer package / Git | [separate actual states] | [package path, logs, commit/push ids; only with real evidence] | [still unfinished items] |

Layers not run still say "not run"—do not delete rows so a reader thinks they passed. Cancelled or failed plans are not current passing verification. User "looks normal" only proves that feedback instance; it does not auto-prove root cause or that every condition is fixed.

## Continue work

| Order | Current pending / waiting / cancelled | Next concrete action | Done criteria / dependencies |
| --- | --- | --- | --- |
| 1 | [current main task] | [local read/dev/check that can start now; exact symbols or commands] | [acceptance or needed input] |
| 2 | [other tasks the user still explicitly wants] | [independent action] | [conditions] |

When there is no unfinished work, keep only that statement—do not invent a todo table. Commands are action descriptions for the next agent to review under current authorization; generating the handoff does not run them.

## Machine-local dependencies and portability

- [Game root, desktop package, reference/archive, screenshots/logs that exist only on this machine; note machine path and purpose]
- [Summaries of irreplaceable evidence, target version/hash, live conditions; still understandable if the original is unavailable]
- [Whether the record applies only to this workspace; what WIP, assets, or analysis attachments a cross-machine handoff still needs]
- [Real state of current processes/background tasks; the record does not mean a new session owns old tool sessions or may kill processes]
```

Do not invent long-running commands, unknown branch state, missing machine attachments, or user.ini contents. Do not put tokens, passwords, cookies, unsanitized connection strings, or user config bodies in the output. Keep only failures and historical decisions that still affect continuation.

Example `resume-prompt.txt` (replace with real paths and goals):

```text
Use $skeeto-handoff to take over Skeeto. Read .codex/handoffs/<this-dir>/handoff.md, first check current project rules and the workspace, then continue: [current task and current authorization scope]. Existing decisions and verification bounds are in that record.
```

In the final reply, provide a machine absolute file link so the user can open it directly. When pasting into a new conversation in the same workspace, the relative path is enough. Across machines, transfer the record folder first, then change paths in the prompt to the target machine's real locations.
