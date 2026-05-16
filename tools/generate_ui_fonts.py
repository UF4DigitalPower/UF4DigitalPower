from __future__ import annotations

import ast
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FONT_DIR = ROOT / "UI" / "Font"
OUTPUT_PATH = ROOT / "UI" / "ui_font_data.c"
TEMP_DIR = ROOT / "tmp_font_extract"


def extract_array_bytes(path: Path, symbol: str) -> bytes:
    text = path.read_text(encoding="utf-8", errors="ignore")
    match = re.search(r"const\s+uint8_t\s+" + re.escape(symbol) + r"\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if match is None:
        raise RuntimeError(f"array {symbol!r} not found in {path}")
    return bytes(ast.literal_eval("[" + match.group(1) + "]"))


def emit_bytes(name: str, data: list[int]) -> str:
    lines = [f"static const uint8_t {name}[] = {{"]
    for i in range(0, len(data), 16):
        chunk = ", ".join(f"{value}" for value in data[i:i + 16])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def emit_glyphs(name: str, glyphs: list[tuple[int, int, int]]) -> str:
    lines = [f"static const ui_bitmap_glyph_t {name}[] = {{"]
    for offset, width, advance in glyphs:
        lines.append(f"    {{{offset}, {width}, {advance}}},")
    lines.append("};")
    return "\n".join(lines)


def raster_font(font_path: Path, size: int, first_char: int, last_char: int) -> tuple[int, list[tuple[int, int, int]], list[int]]:
    font = ImageFont.truetype(str(font_path), size=size)
    boxes = [font.getbbox(chr(code)) for code in range(first_char, last_char + 1)]
    top_min = min(box[1] for box in boxes)
    bottom_max = max(box[3] for box in boxes)
    height = bottom_max - top_min

    glyphs: list[tuple[int, int, int]] = []
    bitmap: list[int] = []

    for code in range(first_char, last_char + 1):
        ch = chr(code)
        left, _top, right, _bottom = font.getbbox(ch)
        width = max(right - left, 0)
        advance = int(round(font.getlength(ch)))
        if advance <= 0:
            advance = width
        offset = len(bitmap)

        if width > 0:
            image = Image.new("L", (width, height), 0)
            draw = ImageDraw.Draw(image)
            draw.text((-left, -top_min), ch, fill=255, font=font)
            bitmap.extend(1 if pixel >= 96 else 0 for pixel in image.getdata())

        glyphs.append((offset, width, advance))

    return height, glyphs, bitmap


def main() -> None:
    TEMP_DIR.mkdir(exist_ok=True)

    extracted = {
        "Bender": TEMP_DIR / "Bender.otf",
        "blender": TEMP_DIR / "blender.otf",
    }
    extracted["Bender"].write_bytes(extract_array_bytes(FONT_DIR / "bender.c", "Bender"))
    extracted["blender"].write_bytes(extract_array_bytes(FONT_DIR / "blender.c", "blender"))

    configs = [
        ("small", extracted["Bender"], 12, 32, 90),
        ("title", extracted["blender"], 20, 32, 90),
        ("value", extracted["blender"], 36, 45, 57),
    ]

    parts = [
        "/**",
        " * @file    : ui_font_data.c",
        " * @brief   : 由 tools/generate_ui_fonts.py 从 UI/Font 中的字体资源自动生成的位图字模。",
        " */",
        "",
        '#include "ui_font.h"',
        "",
    ]

    font_defs: list[str] = []

    for name, font_path, size, first_char, last_char in configs:
        height, glyphs, bitmap = raster_font(font_path, size, first_char, last_char)
        glyph_name = f"g_ui_font_{name}_glyphs"
        bitmap_name = f"g_ui_font_{name}_bitmap"
        font_name = f"g_ui_font_{name}"

        parts.append(emit_glyphs(glyph_name, glyphs))
        parts.append("")
        parts.append(emit_bytes(bitmap_name, bitmap))
        parts.append("")
        font_defs.append(
            f"const ui_bitmap_font_t {font_name} = {{{first_char}, {last_char}, {height}, {glyph_name}, {bitmap_name}}};"
        )

    parts.extend(font_defs)
    parts.append("")

    OUTPUT_PATH.write_text("\n".join(parts), encoding="utf-8")
    print(f"generated: {OUTPUT_PATH}")


if __name__ == "__main__":
    main()

