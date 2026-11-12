# Release and update rules

Packaging, formal release, the update pipeline, and Git. Day-to-day build and deploy rules are in `AGENTS.md`. Ordinary code changes do not need this guide.

## Directories

| Path | Role |
| --- | --- |
| `packages/Skeeto-dev/` | Installer package master: scripts, in-package docs, DIY, PluginSDK, changelog, command index, and the binaries an ordinary build refreshes |
| Desktop `Skeeto_<version>/` | Player-facing installer, copied from the master by the user |
| `packages/Skeeto/` | Untracked `build.bat` output (`dll` plus a source snapshot). Not a package |
| `release/Skeeto_<ver>/` | Optional mirror of a desktop copy |

Compatibility, changelogs, and migration use versions that were actually distributed. A commit, build, or version bump is not distribution. Desktop-built, distributed, and workshop-uploaded are separate states; record them in the matching checklists. This guide does not keep version ledgers.

## Versioning

`X.Y.Z`: X is a breaking rewrite, Y is a new module or major system, Z is a small feature, fix, or polish. Bump Z unless the user asks for Y or X. Do not bump the version, create archives, or change a desktop folder unless asked.

## Release

On an explicit release or version bump:

1. Update `VERSION.txt` if authorized.
2. Rebuild so `packages/Skeeto-dev/` binaries and generated files match.

This updates `packages/Skeeto-dev/` only.

When editing in-package docs, read the file in `packages/Skeeto-dev/` and edit it there. Do not regenerate a tutorial from Markdown or templates. New in-package docs start in `packages/Skeeto-dev/`. Binaries come from source and `build.bat`.

When asked to compress a prepared desktop installer, pass the version explicitly and read only that version's desktop folder. The script compresses a ready package; it does not sync Skills or docs.

Release tools resolve paths from the repository root (`Path(__file__).resolve().parents[1]` in Python). External tools such as ffmpeg use PATH or environment variables. 

## Review

Review the pending user-visible changes, finish related tests, rebuild, and check package layout, version, and DLL consistency. Reuse review evidence for unchanged, already-reviewed parts. Choose tests by impact; skip update-pipeline matrices when that path did not change. A docs- or Skill-text-only change needs no build or product test.

If the pending changes touch resident Hooks, workers, or cleanup, record a normal `quit` from the main menu, in-map, and after disconnect-back-to-menu, per the performance guide's exit section. State anything left untested.

The cache host is built and embedded in the DLL as RCDATA. The installer does not ship `skeeto_cache_host.exe`. After the feature is enabled, the worker extracts, verifies, and starts the host. Do not delete the user's existing LocalAppData files when cleaning build artifacts.

## Installer layout

- Package root holds only the interactive entry `Setup_安装 维护 卸载.bat`.
- Scripts, docs, and the full DIY set live under `Skeeto/`.
- In-package paths use short English, except names that already-published old pumps read at fixed locations (core, version, marker, updater, and changelog at package root).
- The installer UI is Chinese on Chinese locales and English otherwise (`-Language zh/en` overrides).

Every installer includes the full `Skeeto/DIY/Agent Skills/skeeto-hit-feedback/` tree (keep the original path when maintaining an older package). Repository source for that Skill is `.agents/skills/skeeto-hit-feedback/`; the packaged copy is under `packages/Skeeto-dev/`. If `tools/sync_skeeto_user_skills.py` would overwrite packaged Skill docs with repository copies, merge into the development-package copy first. Packaged Skills must run without a development workspace. Needed validation scripts may ship; optional agent UI metadata need not. User-package Skill references may be folded into `SKILL.md`.

Release checks: package-root `skeeto.dll`, `bin/wininet.dll`, version, the full maintenance manifest, and system x86 WinINet exports.

## Commands and changelog

`packages/Skeeto-dev/Skeeto/Commands.txt` is a long-lived command index, not a changelog. Classify entries as ConCommand, ConVar, or launch option. Do not write version numbers. New entries must match registration code. If this version adds any of those, append them here before the user copies the package.

`packages/Skeeto-dev/Skeeto_更新日志.txt` is the changelog. Keep the existing format and append under 「新增 / 修改 / 修复」. Do not reorder or rewrite old entries. Diff user-visible changes from the last distributed version to this one. Unpublished trial-and-error during feature work is not listed separately. Fixes to an undistributed same-version package still use that last distributed baseline.

## Repository docs and Git

The repository tracks `docs/START_HERE.txt` and `docs/agent-guides/` only. Other `docs/` trees are local. Commit only the tracked docs this change actually touches.

`docs/START_HERE.txt` is an AI handoff note, not a changelog. Read or update it only for an explicit handoff. Local `docs/reference/` is a material index, not a source of constraints.

Repository Skills live in `.agents/skills/`. An authorized commit should include the Skills, assets, scripts, and packaging files this task changed. Check that directory explicitly; `git commit -a` is not enough. Leave unrelated dirty work alone. Do not commit caches, user configs, `.codex/handoffs/`, `src/skeeto/build.local.bat`, machine-absolute paths, or accidental `archive/` / unrelated large `assets/` files.

Do not commit or push unless the user asked, or the user asked for a release and the flow above succeeded. Before claiming either is done, check `git status`. Feature-branch release commits merge into the designated release branch before push; do not push unfinished work to `main`.

Before an authorized commit or formal release, review affected behavior with `.agents/skills/skeeto-code-review/SKILL.md`. For memory access, Hooks, or query registration, run `python tools/check_memory_probes.py`. Prefer a reviewer who did not implement the change. Pure-doc commits do not need product tests.

## Game root

Startup needs only root `skeeto.dll` and `bin/wininet.dll`. Install and auto-update write those two DLLs into the game root, plus `Skeeto_更新日志.txt` when the menu opens it.

Workshop packages and update staging keep `VERSION.txt`, `SKEETO_WORKSHOP.txt`, `skeeto_apply_update.bat`, and the maintenance engine at package root. Published clients read the two short files before the consent dialog returns, without unpacking first. Auto-install does not run the bat; it runs the new engine from staging. Those files are not installed into the game root.

Do not create install receipts, readme folders, empty `skeeto_migrate_backup`, or unexplained files in the game root. After the game exits, install and update silently remove leftovers from the fixed list. Do not run that cleanup on the consent-dialog return path. In-progress `skeeto_update` staging is not an installed file; after success, do not spill it into the game root.

## Auto-update

Read this section when the change involves Workshop/VPK detection, VERSION/READY/markers, the updater, post-exit update, the load entry, or update-package layout.

- Do not put blocking work in Paint, FSN, CreateMove, or Event. Workers must not call engine APIs, cvars, commands, or entity interfaces. Hot paths only read already-prepared light state.
- Completion means target files are valid, version and marker values are legal, and the required DLLs are usable. READY is a protocol/semver/manifestHash receipt written by the new-package engine. Apply re-validates the critical payload and receipt. Creating a process is not proof the helper has taken over; confirm with StartedEvent and watch the helper exit. On failure, keep a retryable path; do not treat it as user refusal.
- The DLL discovers candidates, compares versions, collects consent, and validates the stable envelope. protocol=1 Prepare/Apply is implemented by `Skeeto/Maintain.ps1` from the new package in staging, so old DLLs do not have to understand future layouts. Do not rename this public entry in a way that breaks old triggers.
- Same-version candidates do not normally prompt. An existing READY or active transaction may recover or retry at the same version after recovery clears `active.json`. Lower versions are refused. Auxiliary-doc edits, damage, or locks do not block critical-file updates. Uninstall recognizes known core and proxy files and does not depend on a full-package registry. Recovery data keeps only fixed slots for unfinished transactions and is cleared after commit or recovery; ownership logs are deleted last. Uninstall does not accumulate history.
- Update regression must use the previous formal-release commit's real payload and trigger code. Do not fake an old version by rewriting the current artifact's version and rewrapping the manifest. Auto-update replaces prescribed targets from the new-package manifest only. It does not use old core, proxy, or marker identity as admission, and it does not run old DLLs to identify them. After a successful update with no pending recovery, same-version does not prompt again. A same-version changelog is shown once; Apply does not recreate that marker. Test entry: `tools/test_skeeto_released_update.py`.
- Protocol, updater, READY/marker, or cross-version changes also run `tools/test_skeeto_package_layout.py` and `tools/test_skeeto_update_reliability.py`. Load-entry changes run `tools/test_skeeto_l4n_bootstrap.py`.
- If an already-published DLL cannot enter the new flow, say so and give a manual path.
- Old-pump light probe reads package-root `SKEETO_WORKSHOP.txt` and `VERSION.txt`. Full probe reads root `skeeto.dll` and root `skeeto_apply_update.bat` (at least 400 bytes). The fixed log list reads root `Skeeto_更新日志.txt`. Keep these at package root. The new pump checks the stable update envelope; the shipped engine validates the critical payload and writes READY; the exit helper uses the in-stage maintenance engine. Auxiliary docs neither block the core nor create historical backups.
