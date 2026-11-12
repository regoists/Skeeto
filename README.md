# Skeeto

<p align="center">
  <img src="LOGO.png" alt="Skeeto" width="360">
</p>

Skeeto is a Left 4 Dead 2 client enhancement platform that adds a large set of functional extensions and improvements.

Copyright (C) 2026 regoists

This program is free software under the [GNU General Public License version 3](LICENSE). You may redistribute and modify it under the terms of GPL-3.0.

Skeeto is an independent project and has no affiliation with or official partnership with Valve.

The name "Skeeto" is reserved for the official version of this repository. Modified versions based on this project must comply with GPL-3.0 when distributed, but must not use the Skeeto name or marks to impersonate the official build.

## Build

Requires the MSVC x86 toolchain from Visual Studio or Build Tools. From the repository root run:

```bat
src\skeeto\build.bat
```

Outputs:

- `runtime/dll/skeeto.dll`: core
- `runtime/bootstrap/wininet.dll`: Steam load entry

Before making changes, read [AGENTS.md](AGENTS.md) and follow its task index into `docs/agent-guides/`. Handoff navigation is in [docs/START_HERE.txt](docs/START_HERE.txt).

## Directory

| Path | Purpose |
| --- | --- |
| `src/skeeto/` | Source of truth for source code |
| `src/skeeto/core/skeeto_proxy.cpp` | Main dispatch, menu, settings, event wiring, and not-yet-split implementation |
| `src/skeeto/build.bat` | MSVC x86 build entry |
| `runtime/dll/` | Local build output (skeeto.dll only) |
| `runtime/bootstrap/` | Steam load entry wininet.dll |
| `packages/Skeeto-dev/` | Installer package master |
| `assets/` | Assets and preset packs |
| `tools/` | Build, test, and packaging scripts |
| `docs/START_HERE.txt` | Handoff navigation |
| `docs/agent-guides/` | Topic guides |
| `LICENSE` | GNU GPL-3.0 |

## Acknowledgments

Thanks to the [Necola](https://github.com/Punizione/Necola) project and its developer Punizione.
Skeeto was inspired by Necola at the start of the project, and its developer provided substantial help during development.
