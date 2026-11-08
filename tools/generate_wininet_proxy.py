"""Generate x86 export thunks from the audited local system DLL (build tool only)."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import pefile


def generate(system, output):
    image = pefile.PE(str(system))
    if image.FILE_HEADER.Machine != 0x14c:
        raise ValueError("Expected system x86 WinINet")
    symbols = sorted(image.DIRECTORY_ENTRY_EXPORT.symbols, key=lambda s: s.ordinal)
    output.mkdir(parents=True, exist_ok=True)
    asm = [".686p", ".xmm", ".model flat", "EXTERN _SktProxyTargets:DWORD",
           "EXTERN _SktProxyResolve:PROC", ".code"]
    defs = ["LIBRARY wininet", "EXPORTS"]
    catalog = []
    maps = []
    for index, symbol in enumerate(symbols):
        section = image.get_section_by_rva(symbol.address)
        if not symbol.forwarder and (not section or not section.Characteristics & 0x20000000):
            raise ValueError(f"Data export is unsupported: ordinal {symbol.ordinal}")
        name = symbol.name.decode("ascii") if symbol.name else None
        if name and not re.fullmatch(r"[A-Za-z0-9_?@$]+", name):
            raise ValueError(f"Unsupported DEF export spelling: {name}")
        thunk = f"_SktProxyThunk{index}"
        # The cold path preserves all integer, x87 and SSE arguments. The real
        # stdcall/cdecl function remains responsible for stack cleanup.
        asm += [f"PUBLIC {thunk}", f"{thunk} PROC", f"  cmp DWORD PTR [_SktProxyTargets+{index * 4}],0",
                f"  je cold{index}", f"  jmp DWORD PTR [_SktProxyTargets+{index * 4}]", f"cold{index}:",
                "  pushfd", "  pushad", "  mov ebp,esp", "  sub esp,528", "  and esp,-16",
                "  fxsave [esp]", f"  push {index}", "  call _SktProxyResolve", "  add esp,4",
                "  fxrstor [esp]", "  mov esp,ebp", "  popad", "  popfd",
                f"  jmp DWORD PTR [_SktProxyTargets+{index * 4}]", f"{thunk} ENDP"]
        internal = f"SktProxyThunk{index}" # LINK adds the x86 C decoration here.
        defs.append(f'  {name}={internal} @{symbol.ordinal}' if name else f"  SktOrdinal{symbol.ordinal}={internal} @{symbol.ordinal} NONAME")
        if name in {"DllCanUnloadNow", "DllGetClassObject", "DllInstall", "DllRegisterServer", "DllUnregisterServer"}:
            defs[-1] += " PRIVATE"
        maps.append(f'  {{{json.dumps(name) if name else "nullptr"}, {symbol.ordinal}}},')
        catalog.append({"name": name, "ordinal": symbol.ordinal})
    asm += ["END"]
    (output / "wininet_thunks.asm").write_text("\n".join(asm) + "\n", encoding="ascii")
    (output / "wininet.def").write_text("\n".join(defs) + "\n", encoding="ascii")
    (output / "wininet_exports.inc").write_text(
        f"constexpr unsigned SKT_PROXY_EXPORT_COUNT = {len(symbols)};\n"
        "static const SktProxyExport sktProxyExports[] = {\n" + "\n".join(maps) + "\n};\n", encoding="ascii")
    (output / "wininet_catalog.json").write_text(json.dumps({
        "machine": "x86", "sha256": hashlib.sha256(system.read_bytes()).hexdigest(),
        "exports": catalog}, indent=2) + "\n", encoding="utf-8")
    print(f"Generated {len(symbols)} x86 function exports")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--system", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    generate(args.system, args.output)
