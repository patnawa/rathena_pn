#!/usr/bin/env python3
"""Patch the two private Aquila AI rows without versioning the private database."""
import argparse
from pathlib import Path


def patched_database(data: bytes) -> bytes:
    lines = data.splitlines(keepends=True)
    found = {b"21531": 0, b"21588": 0}
    for i, line in enumerate(lines):
        fields = line.rstrip(b"\r\n").split(b",")
        if len(fields) < 7 or fields[0] not in found or fields[3] != b"716":
            continue
        found[fields[0]] += 1
        if fields[1:6] != [b"Aquila@NPC_MAXPAIN", b"attack", b"716", b"5", b"2500"]:
            raise ValueError("Unexpected Aquila Max Pain configuration; review before applying")
        if fields[6] not in (b"0", b"2000"):
            raise ValueError("Unexpected Aquila cast time; refusing to overwrite a custom value")
        fields[6] = b"2000"
        newline = line[len(line.rstrip(b"\r\n")):]
        lines[i] = b",".join(fields) + newline
    if any(count != 1 for count in found.values()):
        raise ValueError(f"Expected exactly one Max Pain row per Aquila form: {found}")
    return b"".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--check", action="store_true", help="Fail if either cast is not already 2000 ms")
    args = parser.parse_args()
    path = args.root / "db/import/mob_skill_db.txt"
    before = path.read_bytes()
    after = patched_database(before)
    if args.check and after != before:
        raise SystemExit("FAIL: Aquila still has an instant AI Max Pain cast")
    if not args.check and after != before:
        path.write_bytes(after)
    print("PASS: both Aquila AI Max Pain casts are 2000 ms; scripted warning timing is unchanged")


if __name__ == "__main__":
    main()
