#!/usr/bin/env python3
"""Extract the REAL УКНЦ 8x8 font from emulator-rendered text (pixel-exact, faithful).

The text font is NOT in a simple linear table in uknc_rom.bin (the УКНЦ char generator
is effectively separate / specially stored). So we recover the exact glyphs from what
the headless emulator actually renders: rich clean text screenshots. Method: per text
row band, detect word groups (gaps), slice each into 8-px glyph cells, emit 8x8 bitmaps.

This pass dumps ALL detected glyphs into a numbered atlas so a human/LLM can read off the
character for each cell and fill GLYPH_MAP; a second pass writes font/ in all formats.

Usage: python3 tools/extract_uknc_font.py dump   # -> /tmp/glyph_dump.png + glyphs.json
"""
import json, sys
from PIL import Image, ImageDraw

SRC = 'assets/uknc/reference_emu/headless/00_boot_menu.png'

def load(src=None):
    img = Image.open(src or SRC).convert('RGB'); W, H = img.size; px = img.load()
    def white(x, y): return 0 <= x < W and 0 <= y < H and px[x, y][0] > 140 and px[x, y][1] > 140
    return W, H, white

def row_bands(W, H, white):
    has = [any(white(x, y) for x in range(W)) for y in range(H)]
    bands = []; y = 0
    while y < H:
        if has[y]:
            y0 = y
            while y < H and has[y]: y += 1
            if y - y0 >= 6: bands.append((y0, y0 + 8))   # 8-px glyph height
        else: y += 1
    return bands

def col_groups(white, x_range, y0, y1, gap=3):
    """Find runs of columns that contain white, separated by >= gap blank columns."""
    has = [any(white(x, y) for y in range(y0, y1)) for x in range(*x_range)]
    groups = []; x = 0; N = len(has)
    while x < N:
        if has[x]:
            x0 = x
            blank = 0
            while x < N and (has[x] or blank < gap):
                if has[x]: blank = 0
                else: blank += 1
                x += 1
            x1 = x - blank
            groups.append((x_range[0] + x0, x_range[0] + x1))
        else: x += 1
    return groups

def glyph_at(white, x0, y0, w=8, h=8):
    return [[1 if white(x0 + c, y0 + r) else 0 for c in range(w)] for r in range(h)]

def main(src=None):
    W, H, white = load(src)
    bands = row_bands(W, H, white)
    glyphs = []   # list of (band_i, group_i, cell_i, 8x8 bitmap)
    for bi, (y0, y1) in enumerate(bands):
        for gi, (gx0, gx1) in enumerate(col_groups(white, (0, W), y0, y1)):
            wpx = gx1 - gx0
            n = max(1, round(wpx / 8))
            cw = wpx / n
            for ci in range(n):
                cx = gx0 + int(round(ci * cw))
                glyphs.append((bi, gi, ci, glyph_at(white, cx, y0)))
    # numbered atlas
    S = 4; cell = 8 * S + 14
    cols = 16; rows = (len(glyphs) + cols - 1) // cols
    im = Image.new('RGB', (cols * cell, rows * cell), (10, 10, 20)); dr = ImageDraw.Draw(im)
    for i, (bi, gi, ci, g) in enumerate(glyphs):
        ox = (i % cols) * cell; oy = (i // cols) * cell
        dr.text((ox, oy), str(i), fill=(180, 180, 80))
        for r in range(8):
            for c in range(8):
                if g[r][c]:
                    for dy in range(S):
                        for dx in range(S): im.putpixel((ox + c*S+dx, oy + 12 + r*S+dy), (255,255,255))
    im.save('/tmp/glyph_dump.png')
    json.dump([g for *_ , g in glyphs], open('/tmp/glyphs.json', 'w'))
    print(f"{len(glyphs)} glyphs -> /tmp/glyph_dump.png  (bands={len(bands)})")

REF = 'assets/uknc/reference_emu/headless/'
# Per-source index -> character (read off each `dump <src>` atlas).
SOURCES = {
    REF+'00_boot_menu.png': {
        0:'З', 1:'А', 2:'Г', 3:'Р', 4:'У', 6:'К', 8:'1', 9:'-', 10:'д', 11:'и',
        12:'с', 13:'к', 14:'(', 17:'3', 18:')', 19:':', 20:'0', 21:'2', 24:'а',
        27:'е', 28:'т', 30:'П', 47:'ь', 50:'ы', 52:'С', 54:'5', 56:'м', 58:'г',
        59:'н', 62:'о', 63:'ф', 66:'6', 70:'л', 72:'д', 75:'7', 82:'р', 84:'в', 88:'е',
    },
    REF+'02_gameplay.png': {           # HUD line: Счет 0 Попытки 2 1 9
        1:'ч', 7:'п', 14:'9',
    },
    REF+'01_title.png': {              # Поставка: Н-Шангская СШ
        11:'Ш', 18:'я',                # (9 was the dash, NOT Н — Н comes from credits)
    },
    REF+'title_credits.png': {         # Николаев 1987 / Баранов (centre crop, no side art)
        0:'Н', 10:'8', 12:'Б',
    },
}

def extract_src(src):
    W, H, white = load(src)
    glyphs = []
    for y0, y1 in row_bands(W, H, white):
        for gx0, gx1 in col_groups(white, (0, W), y0, y1):
            wpx = gx1 - gx0; n = max(1, round(wpx / 8)); cw = wpx / n
            for ci in range(n):
                cx = gx0 + int(round(ci * cw))
                glyphs.append(glyph_at(white, cx, y0))
    return glyphs

# Glyphs extracted at an EXACT (src, x, y) — for chars the word-detector mis-slices
# (next to maze art). (src, x_top_left, y_top_left, char).
MANUAL = [
    ('00_boot_menu.png', 17, 90, '4'),     # from "4 - стык С2"
]

def build():
    chars = {}                       # char -> 8x8 bitmap (first clean occurrence)
    for src, gmap in SOURCES.items():
        glyphs = extract_src(src)
        for idx, ch in gmap.items():
            if idx < len(glyphs) and ch not in chars:
                chars[ch] = glyphs[idx]
    for src, x, y, ch in MANUAL:
        _, _, white = load(REF + src)
        if ch not in chars:
            chars[ch] = glyph_at(white, x, y)
    order = sorted(chars, key=lambda c: ord(c))
    # labelled verification atlas
    S = 5; cell = 8*S + 16; cols = 12; rows = (len(order)+cols-1)//cols
    im = Image.new('RGB', (cols*cell, rows*cell), (10,10,20)); dr = ImageDraw.Draw(im)
    for i, ch in enumerate(order):
        ox=(i%cols)*cell; oy=(i//cols)*cell
        dr.text((ox,oy), ch, fill=(255,230,120))
        for r in range(8):
            for c in range(8):
                if chars[ch][r][c]:
                    for dy in range(S):
                        for dx in range(S): im.putpixel((ox+c*S+dx, oy+12+r*S+dy),(255,255,255))
    im.save('font/uknc_font.png')
    # JSON
    json.dump({ch: chars[ch] for ch in order}, open('font/uknc_font.json','w'),
              ensure_ascii=False, indent=0)
    # C header: array of {codepoint, 8 bytes (MSB=left)}
    with open('font/uknc_font.h','w') as f:
        f.write('// Auto-generated by tools/extract_uknc_font.py — do not edit.\n')
        f.write('// Real УКНЦ 8x8 font, extracted pixel-exact from emulator-rendered text.\n')
        f.write('#pragma once\n#include <stdint.h>\n\n')
        f.write('typedef struct { uint32_t cp; uint8_t rows[8]; } UkncGlyph;\n')
        f.write(f'static const int UKNC_FONT_COUNT = {len(order)};\n')
        f.write('static const UkncGlyph UKNC_FONT[] = {\n')
        for ch in order:
            bs = []
            for r in range(8):
                b = 0
                for c in range(8):
                    if chars[ch][r][c]: b |= (1 << (7-c))
                bs.append(b)
            row = ','.join(f'0x{b:02x}' for b in bs)
            f.write(f'  {{0x{ord(ch):04x}, {{{row}}}}},  // {ch}\n')
        f.write('};\n')
    # BDF — standard bitmap font (web: convert to WOFF/TTF via fonttools/bdf2ttf)
    with open('font/uknc.bdf','w') as f:
        f.write('STARTFONT 2.1\nFONT -uknc-klad-medium-r-normal--8-80-75-75-c-80-iso10646-1\n')
        f.write('SIZE 8 75 75\nFONTBOUNDINGBOX 8 8 0 0\n')
        f.write('STARTPROPERTIES 2\nFONT_ASCENT 8\nFONT_DESCENT 0\nENDPROPERTIES\n')
        f.write(f'CHARS {len(order)}\n')
        for ch in order:
            f.write(f'STARTCHAR U+{ord(ch):04X}\nENCODING {ord(ch)}\n')
            f.write('SWIDTH 1000 0\nDWIDTH 8 0\nBBX 8 8 0 0\nBITMAP\n')
            for r in range(8):
                b = 0
                for c in range(8):
                    if chars[ch][r][c]: b |= (1 << (7-c))
                f.write(f'{b:02X}\n')
            f.write('ENDCHAR\n')
        f.write('ENDFONT\n')
    print(f"font/: {len(order)} glyphs -> uknc_font.{{png,json,h}} + uknc.bdf")
    print("chars:", ''.join(order))
    # Apendăm blocul latin (A-Z a-z ! :) în aceeași matrice 8x8 → română/ASCII fără font de rezervă.
    import gen_latin_font; gen_latin_font.main()

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'build': build()
    elif len(sys.argv) > 2 and sys.argv[1] == 'dump': main(sys.argv[2])
    else: main()
