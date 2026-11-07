# Design sources and maintenance

This Skill is written for Skeeto's real handoff boundaries. It does not copy external scripts or depend on their frameworks. The materials below are method references only—not project rules—and do not need to be loaded or fetched online during ordinary handoffs.

Surveyed 2026-10-01 from fixed sources:

- [lifang336/codex-session-handoff: SKILL.md](https://github.com/lifang336/codex-session-handoff/blob/67fbdbb35f4d0ffc37d1f71e3ee455d326861d65/SKILL.md): borrow generate/continue flows, on-disk state, and change checks on resume. Skeeto uses its own project entry points, does not create a second SESSION/TODO/DECISIONS rule set, and does not treat time-age as truth.
- [jumpifequal/handoff-skill: SKILL.md](https://github.com/jumpifequal/handoff-skill/blob/0e6cb31a66ede5c4791e8fa077e23bee002bb698/SKILL.md): borrow short summaries, decision rationale, failure reasons, and authorization bounds for input material. Skeeto uses existing code references, keeps undropped drafts only when needed, does not stuff whole source/raw chat into a handoff, and does not install automatic lifecycle hooks.
- [Codex local Skill discovery rules](https://learn.chatgpt.com/docs/build-skills): project-level `.agents/skills/` can travel with the repository. Keep one project source of truth; avoid a long-drift personal-directory duplicate of the same Skill name.

## Why this layering

- Current AGENTS, topic guides, and source decide project rules and real behavior; the Skill decides handoff method; each concrete `handoff.md` stores the task at that time; `state.json` helps detect workspace drift. Module growth mainly updates the existing module map—do not revise the Skill feature-by-feature.
- The state script captures only this repository's metadata and bounded file fingerprints. It can detect same-status-but-different-content, a different staged version, rename/delete, and common concurrent changes. It does not collect config bodies or run suggested commands from a handoff.
- New records use time plus a random id so old records are not overwritten; history is not auto-cleaned. Each record is a self-contained handoff and does not accumulate context through long chains. Script listings show a bounded candidate set; content hashes have an explicit budget.
- The data schema is independent of Skeeto product version. When an old schema cannot be parsed, fall back to manual checks—do not forge a match. Product version bumps or new features do not require schema changes.

## What to verify when maintaining

1. The entry Skill still triggers only generate/continue handoff, and still supports natural-language handoff requests; ordinary development is not interrupted.
2. Referenced project entry points still exist and still own that responsibility. After path changes, discover replacements from current AGENTS; do not keep stale duplicate rules.
3. In a temporary Git repository, verify Unicode/space paths, dirty content changes, staging-area changes, delete/rename/untracked, ignored evidence, migrated roots, budget exhaustion, and unknown schemas. Do not use real product source as the test subject.
4. Scenario-check that a new agent respects a current investigation-only request, recognizes evidence bounds on old "already fixed" reports, protects others' dirty changes, and continues within confirmable scope instead of re-asking for authorization.
5. Revise the Skill only from real omissions. Do not turn one crash, one function name, one version, one machine path, or one withdrawn design into a permanent rule.
