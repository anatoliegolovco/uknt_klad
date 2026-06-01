# font/ — the real УКНЦ 8×8 font (КЛАД 1987)

Pixel-exact УКНЦ system font, **extracted from what the emulator actually renders** — not a
substitute like DejaVu. This is the font the original КЛАД 1987 (Баранов) shows on screen, now
packaged in every format you'd want, including a **web-servable** webfont.

▶ **Live showcase + downloads:** <https://anatoliegolovco.github.io/uknt_klad/font/>

## Single source of truth

`uknc_font.h` is canonical — the 8×8 glyph matrix the engine renders (`src/render.c`). **Every
other file here is derived from it** by `tools/gen_webfont.py`, so all formats stay consistent
(currently **99 glyphs**: Latin `!()-0-9:` `A–Z` `a–z` + Cyrillic `АБГЗКНПРСУШ` `авгдеиклмнопрстфчыья`).

| File | Format | Use |
|------|--------|-----|
| `uknc_font.h` | C array `UKNC_FONT[]` `{codepoint, uint8 rows[8]}` | **source of truth** — the game renders glyph by glyph |
| `uknc.woff2` | vector OpenType (Brotli) | **web** — `@font-face`, recommended |
| `uknc.woff` | vector OpenType (zlib) | web fallback |
| `uknc.ttf` | vector OpenType | install in the OS / desktop apps |
| `uknc.bdf` | standard BDF bitmap font | classic X11 / bitmap-font tooling |
| `uknc_font.json` | `{char: 8×8 bit matrix}` | tooling / draw on `<canvas>` |
| `uknc_font.png` | labelled atlas | visual reference / verification |

The webfonts are **vector**: each "on" pixel becomes a square (horizontal runs merged into
rectangles), so they stay crisp at any size. The pixel grid in `uknc_font.h` remains the
authority on glyph shapes.

## Use it on the web

```html
<style>
@font-face {
  font-family: "UKNC KLAD";
  src: url("uknc.woff2") format("woff2"),
       url("uknc.woff")  format("woff");
}
.retro { font-family: "UKNC KLAD", monospace; }
</style>
<h1 class="retro">КЛАД 1987 — Баранов</h1>
```

## Why extracted from rendered output (not the ROM)

The text font is **not** a simple linear glyph table in `uknc_rom.bin` (scanned every
base/stride/bit-order — the УКНЦ character generator is effectively a separate/specially stored
ROM not present in that 32 KB image). So the faithful source is the pixels the emulator draws.
Using our **headless УКНЦ emulator fork** (UKNCBTL, Qt-free — see `docs/headless/`),
`tools/extract_uknc_font.py` slices clean text screenshots
(`assets/uknc/reference_emu/headless/`) into 8×8 glyph cells by word-gap detection, verified
glyph-by-glyph (e.g. `ЗАГРУЗКА` → З А Г Р У З К А). Latin glyphs come from
`tools/gen_latin_font.py`; both feed `uknc_font.h`.

## Regenerate

```bash
# 1. (re)extract / rebuild the canonical matrix when the source captures change:
python3 tools/extract_uknc_font.py build   # -> font/uknc_font.h (+ first-pass atlas)
python3 tools/gen_latin_font.py             # merges 8×8 Latin glyphs into uknc_font.h

# 2. package every distributable format FROM uknc_font.h (run after any .h change):
python3 tools/gen_webfont.py                # -> uknc.{ttf,woff2,woff,bdf}, uknc_font.{json,png}
```

These derived files are committed (like the other generated headers in this repo) and CI serves
the webfonts directly to GitHub Pages.
