#!/usr/bin/env python3
"""Convert JPG icons to RGB565 C image arrays for the TFT driver.

Default usage from an icon directory:
    python ../jpg_to_tft_c_array.py

Usage from the project root:
    python img/jpg_to_tft_c_array.py img/icon_24x24
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover - depends on local environment
    Image = None  # type: ignore[assignment]


C_IDENTIFIER_RE = re.compile(r"[^0-9A-Za-z_]")


@dataclass(frozen=True)
class ImageArrayInfo:
    """Generated C array metadata."""

    c_file: Path
    array_name: str
    size: int
    width: int
    height: int


def make_c_identifier(name: str) -> str:
    """Convert a file stem to a safe C identifier suffix."""
    identifier = C_IDENTIFIER_RE.sub("_", name)
    identifier = re.sub(r"_+", "_", identifier).strip("_")
    if not identifier:
        identifier = "image"
    if identifier[0].isdigit():
        identifier = f"_{identifier}"
    return identifier


def make_include_guard(header_name: str) -> str:
    """Convert a header filename to a C include guard."""
    guard = C_IDENTIFIER_RE.sub("_", header_name.upper())
    guard = re.sub(r"_+", "_", guard).strip("_")
    if not guard:
        guard = "IMAGE_ARRAYS_H"
    if guard[0].isdigit():
        guard = f"_{guard}"
    return guard


def rgb_to_rgb565_le(red: int, green: int, blue: int) -> tuple[int, int]:
    """Convert RGB888 to RGB565 and return low byte first, high byte second."""
    rgb565 = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)
    return rgb565 & 0xFF, (rgb565 >> 8) & 0xFF


def image_to_rgb565_bytes(path: Path) -> tuple[int, int, list[int]]:
    """Read a JPG image and convert pixels to RGB565 little-endian bytes."""
    if Image is None:
        raise RuntimeError("Pillow is not installed. Run: python -m pip install pillow")

    with Image.open(path) as image:
        rgb_image = image.convert("RGB")
        width, height = rgb_image.size
        data: list[int] = []

        for red, green, blue in rgb_image.getdata():
            low_byte, high_byte = rgb_to_rgb565_le(red, green, blue)
            data.extend((low_byte, high_byte))

    return width, height, data


def format_c_array(array_name: str, width: int, height: int, data: list[int]) -> str:
    """Format RGB565 image bytes as one C array."""
    metadata = f"0X00,0X10,0X{width & 0xFF:02X},0X{width >> 8:02X},0X{height & 0xFF:02X},0X{height >> 8:02X},0X01,0X1B"
    lines = [f"const unsigned char {array_name}[{len(data)}] = {{ /* {metadata}, */"]

    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("".join(f"0X{value:02X}," for value in chunk))

    lines.append("};")
    return "\n".join(lines) + "\n"


def format_header(array_infos: list[ImageArrayInfo], header_name: str) -> str:
    """Format extern declarations for all generated image arrays."""
    guard = make_include_guard(header_name)
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
    ]

    for info in array_infos:
        lines.append(f"/* {info.c_file.name}: {info.width}x{info.height}, {info.size} bytes */")
        lines.append(f"extern const unsigned char {info.array_name}[{info.size}];")
        lines.append("")

    lines.extend(
        [
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
            f"#endif /* {guard} */",
        ]
    )
    return "\n".join(lines) + "\n"


def convert_one(input_path: Path, output_dir: Path, prefix: str) -> ImageArrayInfo:
    """Convert one JPG file to one C file."""
    width, height, data = image_to_rgb565_bytes(input_path)
    size_suffix = f"_{width}x{height}"
    array_name = f"{prefix}{make_c_identifier(input_path.stem + size_suffix)}"
    output_path = output_dir / f"{input_path.stem}{size_suffix}.c"
    output_path.write_text(format_c_array(array_name, width, height, data), encoding="utf-8", newline="\n")
    return ImageArrayInfo(c_file=output_path, array_name=array_name, size=len(data), width=width, height=height)


def write_header(output_dir: Path, header_name: str, array_infos: list[ImageArrayInfo]) -> Path:
    """Write a header with extern declarations for generated arrays."""
    header_path = output_dir / header_name
    header_path.write_text(format_header(array_infos, header_name), encoding="utf-8", newline="\n")
    return header_path


def find_jpg_files(input_dir: Path) -> list[Path]:
    """Find JPG/JPEG files directly under input_dir."""
    files = [*input_dir.glob("*.jpg"), *input_dir.glob("*.jpeg"), *input_dir.glob("*.JPG"), *input_dir.glob("*.JPEG")]
    return sorted(set(files), key=lambda path: path.name.lower())


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert JPG files to RGB565 C arrays for the TFT driver")
    parser.add_argument("input_dir", nargs="?", type=Path, default=Path.cwd(), help="directory containing JPG files; default: current directory")
    parser.add_argument("-o", "--output-dir", type=Path, help="directory for generated .c/.h files; default: input directory")
    parser.add_argument("--prefix", default="gImage_", help="C array name prefix, default: gImage_")
    parser.add_argument("--header", help="generated header filename; default: output directory name + .h")
    parser.add_argument("--no-header", action="store_true", help="do not generate the header file")
    args = parser.parse_args()

    input_dir = args.input_dir.resolve()
    output_dir = (args.output_dir or input_dir).resolve()

    if not input_dir.is_dir():
        raise NotADirectoryError(f"input directory does not exist: {input_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)

    jpg_files = find_jpg_files(input_dir)
    if not jpg_files:
        print(f"no JPG files found in {input_dir}", file=sys.stderr)
        return 1

    array_infos: list[ImageArrayInfo] = []
    for jpg_file in jpg_files:
        info = convert_one(jpg_file, output_dir, args.prefix)
        array_infos.append(info)
        print(f"converted {jpg_file.name} -> {info.c_file.name}")

    if not args.no_header:
        header_name = args.header or f"{output_dir.name}.h"
        header_path = write_header(output_dir, header_name, array_infos)
        print(f"generated header -> {header_path.name}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
