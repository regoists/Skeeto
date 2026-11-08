---
name: skeeto-code-review
description: Use for Skeeto local code review, cumulative pre-release review, cross-module performance/crash/lifecycle/feature-isolation investigation, and authorized pre-commit defensive self-checks.
---

# Skeeto code review

Adds project risk and scope context. It does not replace an existing review tool, invent a mandatory report format, force multi-agent review, or require repeated review passes. Rules follow current `AGENTS.md` and the matching guides. Behavior is checked against current source, call chains, and matching live evidence.

## Scope and evidence

- **Pre-release:** Confirm the previous really released version's commit/payload and distribution record. Cover commits after that baseline plus this round's staged, unstaged, related untracked, deleted, and renamed files. Building a desktop package or bumping a version does not prove distribution. Do not mechanically use `HEAD~1` or a merge-base. If a reliable baseline is missing, review what can be determined, report the gap, and ask only when the ambiguity is necessary.
- **Local change:** Follow the user-named feature/files/commits and this round's content into callers, shared state, and cleanup. Other dirty work is not auto-included; committed work from this round must not be skipped.
- **Whole project:** Build a coverage list from the current module map; record read, unread, and unverified batches. Local symptoms expand along dependencies; do not claim the first batch covers everything.
- At start, record baseline, HEAD, and file/change sources; when concurrent edits exist, identify content as needed. Before delivery, only re-review new or changed parts and reuse evidence that still matches the final content. An unchanged HEAD does not prove an unchanged working tree. When the baseline is not an ancestor, compare by the real relationship; do not guess full coverage.
- Review is read-only by default; do not auto-switch branches, stash, edit, build/deploy, or commit. When the user has authorized a fix/release, continue the necessary work. Design fixes inside authorization do not need re-confirmation; out-of-scope refactors are reported separately.

## Follow the call chain

Check "entry → thread/frequency → data production → consumption → invalidation/cleanup" for items actually involved. Do not mechanically expand into unrelated matrices.

- **Hot paths:** Scans, Trace, loops, sorts, queues, resource creation, and retries have ceilings. Read shared-helper implementations; check syscalls, locks, allocation, IO, and expensive log arguments while off. Caches need correct fill gating, lifetime, ceiling, and cleanup. Default-off does not prove cheap when on.
- **Defense:** Use the four questions in the performance guide section "Basis, cost, and bounds for defensive checks": failure scenario, existing guarantees, frequency × cost, and failure behavior. Keep real boundaries; change repeated expensive checks; note cheap cold-path redundancy. Ordinary no-target/off branches do not require new diagnostics; abnormal failures prefer existing fault counters. For memory access, Hooks, or query registration, run `python tools/check_memory_probes.py`.
- **Crash and ABI:** Check lengths/indexes, entity identity/generation, Hook original functions and re-entrancy, missing dependencies, and partial init. Signatures/vtable/offset/patches follow the reverse-engineering guide against the target x86 module, call contract, and failure degradation. Readable/executable or an SDK declaration does not prove ABI. Safety hardening also verifies normal-path equivalence and no wrong call/write on failure; do not rewrite a normal coexistence chain while hardening.
- **Threads and lifecycle:** Engine interfaces without thread-safety proof stay on known game/Paint threads; Event/recv enter bounded queues. Check ownership, cross-thread publish, cancel, release order, enable/disable/reopen, map change/disconnect, player lifetime, and process exit. Old snapshots/restores must not write back into new objects or unloaded modules.
- **Feature isolation:** From consumers, reverse-check producers, Wanted, and async work. Hiding paint alone does not prove scans/sounds/commands stopped. New active features default off; missing-key defaults and user settings must be correct. Off allows bounded cleanup and shared work still required by other consumers; do not clear shared needs incorrectly.
- **Fallback semantics:** Distinguish user-off, explicit empty config, missing resources, and execution failure. Fallback must not bypass an explicit channel switch or resource source. Existing contracts such as same-package inheritance or send-path degradation are confirmed along the call chain; do not delete them just because the name contains "fallback."
- **Client and wiring:** Remote-server data/event coverage cannot be inferred from a server plugin. Check related modes, human/bot, spectate/idle, spawn, and entity reuse. Settings/menus check paint/input, page count/return, save/load, defaults, and localization. Resources check GAME/VPK/loose files, missing assets, and MOD coexistence. HUD visuals may additionally use `$skeeto-hud-design` when needed.

## Verification and delivery

- Prefer tests that exercise production implementations or extract real functions. Doubles substitute only external boundaries such as the engine; they do not rewrite the decision under test. When a fixture's compile/deps are broken, fix the fixture—do not change correct product behavior to satisfy an old assert. Asserts cover the original defect, normal behavior, and side effects. Returning false does not prove there was no wrong call/write.
- For performance symptoms, measure before judging a fix. Report static bounds, offline tests, build/deploy, live behavior, and performance data separately; state untested gaps. Do not treat a green compile as proof of no crash, and do not promise zero cost or safety in every environment.
- Findings give concrete location, reachable conditions, impact, and evidence, ordered by real severity. Separate this-round regressions, pre-existing defects, design suggestions, and speculation. In-scope design defects do not automatically block every task; fix suggestions must keep necessary protection and normal behavior.
- Before commit/release, run the defensive self-check above on this round's code, briefly state substantive conclusions, and do not invent per-null-check scorecards or extra proof files. Prefer a reviewer who did not implement the change when practical; multi-agent review is not required. At triage wrap-up, remove temporary diagnostics with no consumers; keep necessary abnormal observation and offline regressions.
- Versions, packages, player docs, and user Skills follow `docs/agent-guides/release-and-updates.md`. Do not run unrelated update matrices when the update path was not changed. A review tool's default diff is not this round's scope; fill gaps and state actual coverage.

This Skill maintains review method and easy-to-miss risks only. Concrete rules, the module map, and release flow stay in their sources of truth—do not copy them into long tables or version ledgers here.
