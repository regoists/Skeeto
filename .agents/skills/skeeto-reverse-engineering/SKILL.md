---
name: skeeto-reverse-engineering
description: Use when a Skeeto conclusion must depend on a published Source/L4D2 or Skeeto x86 PE DLL, and source, call chains, and existing records cannot confirm an interface, ConVar read, signature, offset, calling convention, or version difference.
---

# Skeeto binary reverse engineering

Supply only the published-binary evidence that Skeeto source cannot prove. REA, Ghidra, and machine-local tools are governed by the reverse-engineering guide; this Skill covers trigger bounds, read-only analysis order, and how to state evidence.

## Trigger bounds

- **Use when:** Confirming whether a Source/L4D2 x86 DLL actually reads a ConVar; checking interfaces/vtable/call sites/signatures/offsets/calling conventions; or comparing explicitly named Source/L4D2 or Skeeto release DLLs—especially when source exists but the published/build/packaged artifact may differ.
- **Do not use for:** Ordinary source development, menu/settings issues, code review, routine build/deploy, or questions already answered by source and existing evidence. Do not open REA, run doctor, or enumerate a whole binary "just in case."
- Android, browser, firmware, EVM, and other non-Skeeto targets are outside this Skill. Those tasks stay with upstream `$reverse-engineer-anything` under its original trigger scope; the user may also name that Skill explicitly.

## Analysis flow

1. First check Skeeto source, the real call chain, and `docs/agent-guides/reverse-engineering-and-disproven-paths.md`. Continue only if the conclusion still depends on a binary fact.
2. Lock the exact target file. Prefer a user-named file or a precise path already recorded in docs. Do not scan Steam libraries, disks, or unrelated DLLs. Record path, PE architecture, version info, and SHA-256; Skeeto/L4D2 implementation targets must be confirmed x86.
3. Use the connected `rea` MCP; for native DLLs set `provider_id: "ghidra"`. First check `binary_session`: reuse when the target SHA-256 matches; if the current session is bound to another target, do not close or replace it—use a separate CLI session or state the connection limit. When no reusable session exists, `open_binary` on the exact target.
4. After open, run only hypothesis-driven queries: locate with `binary_overview`, `search_strings`, or `search_procedures`; answer with `xrefs`, `analyze_function`, `procedure_assembly`, `procedure_pseudo_code`, or caller/callee queries. Call only tools the current server actually exposes; do not bulk-export the whole program.
5. Separate conclusions into "direct binary observation," "static inference," and "unknown / coverage limit." Addresses and signatures must be reported with file version, architecture, and SHA-256. Decompiled pseudocode is not original source; a static call edge does not prove the game runtime takes that path. An empty query alone cannot prove a feature is absent.
6. Call `close_binary` only when this task opened the session; keep state when reusing an existing session. Before landing an offset, vtable, signature, or ABI, also check the real source call site. Static analysis, build, and live acceptance are reported separately and do not substitute for each other.

## Skeeto RE conclusion format

- **Target:** file, version, x86, SHA-256.
- **Direct observation:** address, symbol/string, xref or call relation, and REA Evidence when available.
- **Inference:** the narrowest conclusion the observation supports.
- **Unknown:** uncovered paths, indirect calls, analyzer limits; next verification when needed.

Example: if the user asks whether `sb_flashlight` is read, first confirm the exact `server.dll` identity, then follow ConVar-object and accessor xrefs for a read path. If only register/construct references appear and no read call site is found, report "no read evidence found on this target version"—do not promote one empty search into an absolute claim for every version or for runtime.

## Tool entry points

- Machine install, environment variables, and CLI paths without MCP are in [`reverse-engineering-and-disproven-paths.md`](../../../docs/agent-guides/reverse-engineering-and-disproven-paths.md) under "Machine-local tools" and "Use REA on demand."
- The full REA toolset stays available. This Skill only constrains when Skeeto tasks enable it and which read-only native analysis tools to prefer; it does not modify the REA install or MCP tool whitelist.
