"""Read-only PE inventory for native wide-Zeny work; never enables a capability."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


CAPABILITIES = ("wallet", "trade", "mail", "vending", "buying_store", "shop_search")


def inspect_pe(data: bytes) -> dict:
    def unpack(fmt: str, offset: int):
        size = struct.calcsize(fmt)
        if offset < 0 or offset + size > len(data):
            raise ValueError("Truncated PE structure")
        return struct.unpack_from(fmt, data, offset)

    if data[:2] != b"MZ":
        raise ValueError("Missing DOS signature")
    pe, = unpack("<I", 60)
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Missing PE signature")
    machine, count, timestamp, symbols, symbol_count, optional_size, _ = unpack("<HHIIIHH", pe + 4)
    optional = pe + 24
    magic, = unpack("<H", optional)
    if magic not in (0x10B, 0x20B):
        raise ValueError("Unsupported optional-header magic")
    directory_offset = 96 if magic == 0x10B else 112
    if optional_size < directory_offset:
        raise ValueError("Optional header too short")
    unpack(f"<{optional_size}s", optional)
    entry, = unpack("<I", optional + 16)
    directory_count, = unpack("<I", optional + directory_offset - 4)
    directories = []
    for i in range(min(directory_count, 16)):
        if directory_offset + (i + 1) * 8 > optional_size:
            raise ValueError("Data directory exceeds optional header")
        directories.append(unpack("<II", optional + directory_offset + i * 8))
    sections = []
    for i in range(count):
        position = optional + optional_size + i * 40
        name, virtual_size, rva, raw_size, raw_offset, _, _, _, _, flags = unpack("<8sIIIIIIHHI", position)
        if raw_size and raw_offset + raw_size > len(data):
            raise ValueError("Section exceeds file size")
        sections.append({"name": name.rstrip(b"\0").decode("ascii", errors="replace"),
                         "rva": rva, "virtual_size": virtual_size,
                         "raw_size": raw_size, "raw_offset": raw_offset,
                         "executable": bool(flags & 0x20000000)})
    debug_rva, debug_size = directories[6] if len(directories) > 6 else (0, 0)
    strings = [m.group().decode("ascii") for m in re.finditer(rb"[\x20-\x7e]{6,}", data)]
    economic_strings = [s[:200] for s in strings if any(term in s.lower() for term in
                        ("zeny", "vending", "buyingstore", "searchstore", "banking"))]
    return {"sha256": hashlib.sha256(data).hexdigest(), "file_size": len(data),
            "machine": hex(machine), "format": "PE32" if magic == 0x10B else "PE32+",
            "timestamp": timestamp, "entry_rva": hex(entry),
            "entry_section": next((s["name"] for s in sections if
                                   s["rva"] <= entry < s["rva"] + max(s["virtual_size"], s["raw_size"])), None),
            "coff_symbol_pointer": symbols, "coff_symbol_count": symbol_count,
            "debug_directory_rva": debug_rva, "debug_directory_size": debug_size,
            "protection_section_names": [s["name"] for s in sections if
                                         s["name"].lower() in (".themida", ".vmp0", ".vmp1", "upx0", "upx1")],
            "sections": sections, "economic_ascii_strings": economic_strings[:100],
            "pdb_ascii_strings": [s[:200] for s in strings if ".pdb" in s.lower()][:20],
            "wide_zeny": {"enabled": False,
                          "reason": "Static inventory cannot prove native UI, arithmetic, or packet support. Runtime acceptance is required.",
                          "verified_capabilities": [], "required_capabilities": list(CAPABILITIES)}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        report = inspect_pe(args.executable.read_bytes())
    except (OSError, ValueError) as error:
        parser.exit(2, f"Probe failed: {error}\n")
    report["input_name"] = args.executable.name
    result = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.write_text(result, encoding="utf-8")
    else:
        print(result, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
