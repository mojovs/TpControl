#!/usr/bin/env python3
"""Convert img/font.txt glyph data to a C typFNT_GB12 array.

The input format is expected to contain lines like:
    0x04,0x06,...,"孵",0

Usage:
    python Tools/font_txt_to_c_array.py img/font.txt
    python Tools/font_txt_to_c_array.py img/font.txt -o img/tfont12.c
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

GLYPH_RE = re.compile(r'^(?P<data>(?:0x[0-9A-Fa-f]{1,2}\s*,\s*)+)"(?P<char>.)"\s*,\s*(?P<index>\d+)\s*,?\s*$')
HEX_RE = re.compile(r"0x[0-9A-Fa-f]{1,2}")


def parse_font_txt(path: Path) -> list[tuple[str, list[str], int]]:
    """Parse glyph rows from font.txt."""
    glyphs: list[tuple[str, list[str], int]] = []

    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = line.strip()
        if not line or not line.startswith("0x"):
            continue

        match = GLYPH_RE.match(line)
        if match is None:
            raise ValueError(f"line {line_no}: invalid glyph line: {line}")

        data = [value.upper().replace("X", "x") for value in HEX_RE.findall(match.group("data"))]
        if len(data) != 24:
            raise ValueError(f"line {line_no}: expected 24 bytes, got {len(data)}")

        glyphs.append((match.group("char"), data, int(match.group("index"))))

    return glyphs


def format_c_array(glyphs: list[tuple[str, list[str], int]], array_name: str) -> str:
    """Format glyphs as a typFNT_GB12 C array, one glyph per line."""
    lines = [f"const typFNT_GB12 {array_name}[] = {{"]

    for char, data, _index in glyphs:
        lines.append(f'    "{char}",{",".join(data)},')

    lines.append("};")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert font.txt to a typFNT_GB12 C array")
    parser.add_argument("input", type=Path, help="input font.txt path")
    parser.add_argument("-o", "--output", type=Path, help="output file path; default: print to stdout")
    parser.add_argument("--array-name", default="tfont12", help="C array name, default: tfont12")
    args = parser.parse_args()

    glyphs = parse_font_txt(args.input)
    output = format_c_array(glyphs, args.array_name)

    if args.output is None:
        print(output, end="")
    else:
        args.output.write_text(output, encoding="utf-8", newline="\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
