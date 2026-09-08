#!/usr/bin/env python3
"""
Font Generator for Adafruit GFX / GxEPD2
Renders TrueType/OpenType vector fonts into 1:1 pixel-mapped GFXfont C headers
using Pillow (PIL), avoiding boxy / pixelated setTextSize(N) scaling.
"""

import sys
import os
from PIL import Image, ImageFont, ImageDraw

def generate_font(font_path, pt_size=70, dpi=141, first=0x20, last=0x3A, output_header=None):
    if not os.path.exists(font_path):
        raise FileNotFoundError(f"Font file not found: {font_path}")

    # Calculate pixel size: pt * dpi / 72 (matches Adafruit fontconvert)
    font_size_px = int(round(pt_size * dpi / 72.0))
    font = ImageFont.truetype(font_path, font_size_px)

    base_name = os.path.splitext(os.path.basename(font_path))[0].replace('-', '_')
    font_name = f"{base_name}{pt_size}pt7b"

    bitmap_bytes = []
    current_byte = 0
    bit_pos = 7

    def enbit(val):
        nonlocal current_byte, bit_pos, bitmap_bytes
        if val:
            current_byte |= (1 << bit_pos)
        bit_pos -= 1
        if bit_pos < 0:
            bitmap_bytes.append(current_byte)
            current_byte = 0
            bit_pos = 7

    glyphs = []
    bitmap_offset = 0

    margin = font_size_px * 2
    base_y = font_size_px * 2
    canvas_size = font_size_px * 4

    for c in range(first, last + 1):
        char = chr(c)
        canvas = Image.new('1', (canvas_size, canvas_size), 0)
        draw = ImageDraw.Draw(canvas)
        draw.text((margin, base_y), char, font=font, fill=1, anchor='ls')
        box = canvas.getbbox()
        adv = int(round(font.getlength(char)))

        if box is None:
            # Empty / whitespace glyph (e.g. space)
            glyphs.append({
                'bitmapOffset': bitmap_offset,
                'width': 0,
                'height': 0,
                'xAdvance': adv,
                'xOffset': 0,
                'yOffset': 1,
                'char': char,
                'code': c
            })
        else:
            cropped = canvas.crop(box)
            width, height = cropped.size
            x_offset = box[0] - margin
            # Adafruit GFX convention: yOffset = 1 - g->top = (box[1] - base_y) + 1
            y_offset = (box[1] - base_y) + 1

            glyphs.append({
                'bitmapOffset': bitmap_offset,
                'width': width,
                'height': height,
                'xAdvance': adv,
                'xOffset': x_offset,
                'yOffset': y_offset,
                'char': char,
                'code': c
            })

            # Stream bits row-by-row into bit-packed array
            for y in range(height):
                for x in range(width):
                    pixel = cropped.getpixel((x, y))
                    enbit(pixel)

            # Pad to next byte boundary at end of glyph
            n = (width * height) & 7
            if n:
                for _ in range(8 - n):
                    enbit(0)

            bitmap_offset += (width * height + 7) // 8

    if not output_header:
        output_header = f"include/{font_name}.h"

    # Ensure target directory exists
    os.makedirs(os.path.dirname(os.path.abspath(output_header)), exist_ok=True)

    with open(output_header, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <Adafruit_GFX.h>\n\n')
        f.write(f'// Native High-Resolution TrueType Font at 1:1 Pixel Mapping (No 3x3 Block Scaling)\n')
        f.write(f'// Generated from {os.path.basename(font_path)} at {pt_size}pt ({dpi} DPI) for ASCII 0x{first:02X} to 0x{last:02X}\n\n')

        f.write(f'const uint8_t {font_name}Bitmaps[] PROGMEM = {{\n')
        for i in range(0, len(bitmap_bytes), 12):
            chunk = bitmap_bytes[i:i+12]
            hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
            f.write(f'    {hex_str},\n' if i + 12 < len(bitmap_bytes) else f'    {hex_str}\n')
        f.write('};\n\n')

        f.write(f'const GFXglyph {font_name}Glyphs[] PROGMEM = {{\n')
        for idx, g in enumerate(glyphs):
            c_desc = f"'{g['char']}'" if g['char'] != '\\' else "'\\\\'"
            comma = ',' if idx < len(glyphs) - 1 else ''
            f.write(f"    {{ {g['bitmapOffset']:5d}, {g['width']:3d}, {g['height']:3d}, {g['xAdvance']:3d}, {g['xOffset']:3d}, {g['yOffset']:4d} }}{comma} // 0x{g['code']:02X} {c_desc}\n")
        f.write('};\n\n')

        y_advance = int(round(font_size_px * 1.15))
        f.write(f'const GFXfont {font_name} PROGMEM = {{\n')
        f.write(f'    (uint8_t *){font_name}Bitmaps,\n')
        f.write(f'    (GFXglyph *){font_name}Glyphs,\n')
        f.write(f'    0x{first:02X}, 0x{last:02X}, {y_advance}\n')
        f.write('};\n')

    print(f"Generated {output_header} successfully ({len(glyphs)} glyphs, {len(bitmap_bytes)} bytes)")

if __name__ == '__main__':
    font = sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/gnu-free/FreeSansBold.ttf'
    size = int(sys.argv[2]) if len(sys.argv) > 2 else 70
    out = sys.argv[3] if len(sys.argv) > 3 else None
    generate_font(font, size, output_header=out)
