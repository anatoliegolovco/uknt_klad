#!/usr/bin/env python3
"""Package the extracted УКНЦ font into every distributable format.

Single source of truth: font/uknc_font.h — the canonical 8x8 glyph matrix used
by the engine (src/render.c), produced by the extraction pipeline
(tools/extract_uknc_font.py + the Latin glyphs from tools/gen_latin_font.py).
Everything else in font/ is *derived* from it by this script, so all formats
stay consistent.

Emits, into font/:
  uknc.woff2 / uknc.woff / uknc.ttf   web-servable vector font (pixel→squares)
  uknc.bdf                            standard BDF bitmap font
  uknc_font.json                      {char: 8x8 bit matrix}  (tooling / canvas)
  uknc_font.png                       labelled atlas (visual reference)

Run:  python3 tools/gen_webfont.py
"""
import json
import re
import pathlib

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

ROOT = pathlib.Path(__file__).resolve().parent.parent
HEADER = ROOT / "font" / "uknc_font.h"
OUT = ROOT / "font"

EM = 1024          # units per em
PX = EM // 8       # 128 units per pixel (8x8 grid)
ASCENT = EM        # all 8 rows sit above the baseline
ADVANCE = EM       # monospace 8px cell

GLYPH_RE = re.compile(r"\{0x([0-9a-fA-F]+),\s*\{([^}]*)\}\}")


def parse_glyphs():
    """-> dict {codepoint: [8 row bytes]} from uknc_font.h (the source of truth)."""
    glyphs = {}
    for m in GLYPH_RE.finditer(HEADER.read_text()):
        cp = int(m.group(1), 16)
        rows = [int(b, 16) for b in m.group(2).split(",")]
        if len(rows) == 8:
            glyphs[cp] = rows
    glyphs.setdefault(0x20, [0] * 8)   # ensure blank space exists
    return glyphs


# bit order matches src/render.c: col 0 = bit 7 (MSB) = leftmost pixel
def pixel(rows, r, c):
    return (rows[r] >> (7 - c)) & 1


# ── vector OpenType (TTF / WOFF2 / WOFF) ─────────────────────────────────────
def draw_glyph(rows):
    """TrueType glyph: merge per-row pixel runs into clockwise rectangles."""
    pen = TTGlyphPen(None)
    for r in range(8):
        c = 0
        while c < 8:
            if not pixel(rows, r, c):
                c += 1
                continue
            start = c
            while c < 8 and pixel(rows, r, c):
                c += 1
            x0, x1 = start * PX, c * PX
            y1, y0 = (8 - r) * PX, (8 - r - 1) * PX
            pen.moveTo((x0, y0)); pen.lineTo((x0, y1))
            pen.lineTo((x1, y1)); pen.lineTo((x1, y0))
            pen.closePath()
    return pen.glyph()


def build_otf(glyphs):
    cps = sorted(glyphs)
    names = {cp: f"uni{cp:04X}" for cp in cps}
    fb = FontBuilder(EM, isTTF=True)
    fb.setupGlyphOrder([".notdef"] + [names[cp] for cp in cps])
    fb.setupCharacterMap({cp: names[cp] for cp in cps})
    glyf = {".notdef": TTGlyphPen(None).glyph()}
    metrics = {".notdef": (ADVANCE, 0)}
    for cp in cps:
        glyf[names[cp]] = draw_glyph(glyphs[cp])
        metrics[names[cp]] = (ADVANCE, 0)
    fb.setupGlyf(glyf)
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=ASCENT, descent=0)
    fb.setupNameTable({
        "familyName": "UKNC KLAD", "styleName": "Regular",
        "fullName": "UKNC KLAD Regular", "psName": "UKNCKLAD-Regular",
        "version": "1.0",
        "manufacturer": "Reverse-engineered from КЛАД 1987 (Баранов), УКНЦ МС-0511",
    })
    fb.setupOS2(sTypoAscender=ASCENT, sTypoDescender=0,
                usWinAscent=ASCENT, usWinDescent=0)
    fb.setupPost()
    fb.save(str(OUT / "uknc.ttf"))
    fb.font.flavor = "woff2"; fb.save(str(OUT / "uknc.woff2"))
    fb.font.flavor = "woff";  fb.save(str(OUT / "uknc.woff"))


# ── BDF ──────────────────────────────────────────────────────────────────────
def build_bdf(glyphs):
    cps = sorted(glyphs)
    out = [
        "STARTFONT 2.1",
        "FONT -uknc-klad-medium-r-normal--8-80-75-75-c-80-iso10646-1",
        "SIZE 8 75 75",
        "FONTBOUNDINGBOX 8 8 0 0",
        "STARTPROPERTIES 2", "FONT_ASCENT 8", "FONT_DESCENT 0", "ENDPROPERTIES",
        f"CHARS {len(cps)}",
    ]
    for cp in cps:
        out += [f"STARTCHAR U+{cp:04X}", f"ENCODING {cp}",
                "SWIDTH 1000 0", "DWIDTH 8 0", "BBX 8 8 0 0", "BITMAP"]
        out += [f"{b:02X}" for b in glyphs[cp]]
        out.append("ENDCHAR")
    out.append("ENDFONT")
    (OUT / "uknc.bdf").write_text("\n".join(out) + "\n")


# ── JSON ─────────────────────────────────────────────────────────────────────
def build_json(glyphs):
    data = {chr(cp): [[pixel(rows, r, c) for c in range(8)] for r in range(8)]
            for cp, rows in sorted(glyphs.items())}
    (OUT / "uknc_font.json").write_text(
        json.dumps(data, ensure_ascii=False, indent=0))


# ── PNG atlas ────────────────────────────────────────────────────────────────
def build_png(glyphs):
    from PIL import Image, ImageDraw
    cps = sorted(glyphs)
    cols, scale, cell, pad, lab = 12, 4, None, 4, 8
    gpx = 8 * scale
    cell = gpx + pad * 2 + lab
    rows = (len(cps) + cols - 1) // cols
    BLUE, WHITE, DIM = (0, 0, 170), (255, 255, 255), (150, 150, 170)
    img = Image.new("RGB", (cols * cell, rows * cell), BLUE)
    d = ImageDraw.Draw(img)
    for i, cp in enumerate(cps):
        ox = (i % cols) * cell + pad
        oy = (i // cols) * cell + pad
        for r in range(8):
            for c in range(8):
                if pixel(glyphs[cp], r, c):
                    d.rectangle([ox + c * scale, oy + r * scale,
                                 ox + (c + 1) * scale - 1, oy + (r + 1) * scale - 1],
                                fill=WHITE)
        d.text((ox, oy + gpx), f"{cp:04X}", fill=DIM)
    img.save(OUT / "uknc_font.png")


def main():
    glyphs = parse_glyphs()
    build_otf(glyphs)
    build_bdf(glyphs)
    build_json(glyphs)
    build_png(glyphs)
    print(f"packaged {len(glyphs)} glyphs -> "
          "uknc.{ttf,woff2,woff,bdf}, uknc_font.{json,png}")


if __name__ == "__main__":
    main()
