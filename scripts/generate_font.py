#!/usr/bin/env python3
"""
Font Generator for Adafruit GFX / GxEPD2
Renders TrueType/OpenType vector fonts into 1:1 pixel-mapped GFXfont C headers
using FreeType, avoiding boxy / pixelated setTextSize(N) scaling.
"""

import sys
import os
import ctypes

class FT_Vector(ctypes.Structure):
    _fields_ = [('x', ctypes.c_long), ('y', ctypes.c_long)]

class FT_BBox(ctypes.Structure):
    _fields_ = [('xMin', ctypes.c_long), ('yMin', ctypes.c_long), ('xMax', ctypes.c_long), ('yMax', ctypes.c_long)]

class FT_Bitmap(ctypes.Structure):
    _fields_ = [
        ('rows', ctypes.c_uint), ('width', ctypes.c_uint), ('pitch', ctypes.c_int),
        ('buffer', ctypes.POINTER(ctypes.c_ubyte)), ('num_grays', ctypes.c_short),
        ('pixel_mode', ctypes.c_char), ('palette_mode', ctypes.c_char), ('palette', ctypes.c_void_p)
    ]

class FT_Glyph_Metrics(ctypes.Structure):
    _fields_ = [
        ('width', ctypes.c_long), ('height', ctypes.c_long),
        ('horiBearingX', ctypes.c_long), ('horiBearingY', ctypes.c_long), ('horiAdvance', ctypes.c_long),
        ('vertBearingX', ctypes.c_long), ('vertBearingY', ctypes.c_long), ('vertAdvance', ctypes.c_long)
    ]

class FT_GlyphSlotRec(ctypes.Structure):
    pass

class FT_Size_Metrics(ctypes.Structure):
    _fields_ = [
        ('x_ppem', ctypes.c_ushort), ('y_ppem', ctypes.c_ushort),
        ('x_scale', ctypes.c_long), ('y_scale', ctypes.c_long),
        ('ascender', ctypes.c_long), ('descender', ctypes.c_long),
        ('height', ctypes.c_long), ('max_advance', ctypes.c_long),
    ]

class FT_SizeRec(ctypes.Structure):
    _fields_ = [('face', ctypes.c_void_p), ('generic', ctypes.c_void_p * 2), ('metrics', FT_Size_Metrics)]

FT_GlyphSlotRec._fields_ = [
    ('library', ctypes.c_void_p), ('face', ctypes.c_void_p), ('next', ctypes.c_void_p),
    ('glyph_index', ctypes.c_uint), ('generic', ctypes.c_void_p * 2),
    ('metrics', FT_Glyph_Metrics), ('linearHoriAdvance', ctypes.c_long),
    ('linearVertAdvance', ctypes.c_long), ('advance', FT_Vector),
    ('format', ctypes.c_uint), ('bitmap', FT_Bitmap),
    ('bitmap_left', ctypes.c_int), ('bitmap_top', ctypes.c_int),
]

class FT_FaceRec(ctypes.Structure):
    _fields_ = [
        ('num_faces', ctypes.c_long), ('face_index', ctypes.c_long),
        ('face_flags', ctypes.c_long), ('style_flags', ctypes.c_long),
        ('num_glyphs', ctypes.c_long), ('family_name', ctypes.c_char_p),
        ('style_name', ctypes.c_char_p), ('num_fixed_sizes', ctypes.c_int),
        ('available_sizes', ctypes.c_void_p), ('num_charmaps', ctypes.c_int),
        ('charmaps', ctypes.c_void_p), ('generic', ctypes.c_void_p * 2),
        ('bbox', FT_BBox), ('units_per_EM', ctypes.c_ushort),
        ('ascender', ctypes.c_short), ('descender', ctypes.c_short),
        ('height', ctypes.c_short), ('max_advance_width', ctypes.c_short),
        ('max_advance_height', ctypes.c_short), ('underline_position', ctypes.c_short),
        ('underline_thickness', ctypes.c_short), ('glyph', ctypes.POINTER(FT_GlyphSlotRec)),
        ('size', ctypes.POINTER(FT_SizeRec)),
    ]

def generate_font(font_path, pt_size=70, dpi=141, first=0x20, last=0x3A, output_header=None):
    ft = ctypes.CDLL('/usr/lib64/libfreetype.so.6')
    lib = ctypes.c_void_p()
    ft.FT_Init_FreeType(ctypes.byref(lib))
    face = ctypes.POINTER(FT_FaceRec)()
    if ft.FT_New_Face(lib, font_path.encode('utf-8'), 0, ctypes.byref(face)):
        raise RuntimeError(f"Failed to load font: {font_path}")

    base_name = os.path.splitext(os.path.basename(font_path))[0].replace('-', '_')
    font_name = f"{base_name}{pt_size}pt7b"

    ft.FT_Set_Char_Size(face, pt_size << 6, 0, dpi, 0)
    FT_LOAD_TARGET_MONO = (1 & 15) << 16
    FT_RENDER_MODE_MONO = 1

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

    for c in range(first, last + 1):
        err = ft.FT_Load_Char(face, c, FT_LOAD_TARGET_MONO)
        if err:
            continue
        ft.FT_Render_Glyph(face.contents.glyph, FT_RENDER_MODE_MONO)
        slot = face.contents.glyph.contents
        bm = slot.bitmap

        width = bm.width
        height = bm.rows
        x_advance = slot.advance.x >> 6
        x_offset = slot.bitmap_left
        y_offset = 1 - slot.bitmap_top

        glyphs.append({
            'bitmapOffset': bitmap_offset,
            'width': width,
            'height': height,
            'xAdvance': x_advance,
            'xOffset': x_offset,
            'yOffset': y_offset,
            'char': chr(c),
            'code': c
        })

        pitch = bm.pitch
        buf = bm.buffer
        for y in range(height):
            for x in range(width):
                byte_idx = x // 8
                bit_mask = 0x80 >> (x & 7)
                pixel = (buf[y * pitch + byte_idx] & bit_mask) != 0
                enbit(pixel)

        n = (width * height) & 7
        if n:
            for _ in range(8 - n):
                enbit(0)
        bitmap_offset += (width * height + 7) // 8

    if bit_pos != 7:
        bitmap_bytes.append(current_byte)

    if not output_header:
        output_header = f"include/{font_name}.h"

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

        y_advance = face.contents.size.contents.metrics.height >> 6
        f.write(f'const GFXfont {font_name} PROGMEM = {{\n')
        f.write(f'    (uint8_t *){font_name}Bitmaps,\n')
        f.write(f'    (GFXglyph *){font_name}Glyphs,\n')
        f.write(f'    0x{first:02X}, 0x{last:02X}, {y_advance}\n')
        f.write('};\n')

    print(f"Generated {output_header} successfully ({len(glyphs)} glyphs, {len(bitmap_bytes)} bytes)")

if __name__ == '__main__':
    font = sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/gnu-free/FreeSansBold.ttf'
    size = int(sys.argv[2]) if len(sys.argv) > 2 else 70
    generate_font(font, size)
