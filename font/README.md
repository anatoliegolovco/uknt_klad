# font/ — the real УКНЦ 8×8 font (КЛАД 1987)

Pixel-exact УКНЦ system font, **extracted from what the emulator actually renders** — not a
substitute like DejaVu. This is the font the original КЛАД shows on screen.

## Why extracted from rendered output (not the ROM)

The text font is **not** stored as a simple linear glyph table in `uknc_rom.bin` (scanned
every base/stride/bit-order — the УКНЦ character generator is effectively a separate/specially
stored ROM not present in that 32 KB image). So the faithful source is the pixels the headless
emulator draws. `tools/extract_uknc_font.py` slices clean text screenshots
(`assets/uknc/reference_emu/headless/`) into 8×8 glyph cells by word-gap detection, verified
glyph-by-glyph. Proven correct on `ЗАГРУЗКА` → З А Г Р У З К А.

## Files / formats

| File | Format | Use |
|------|--------|-----|
| `uknc_font.h` | C array `UKNC_FONT[]` `{codepoint, uint8 rows[8]}` | the game (`src/render.c`) — render text glyph by glyph |
| `uknc_font.png` | labelled atlas | visual reference / verification |
| `uknc_font.json` | `{char: 8×8 bitmap}` | tooling / web canvas |
| `uknc.bdf` | standard BDF bitmap font | **web**: convert to WOFF/TTF (`fonttools`, `bdf2ttf`, or an online converter) for CSS `@font-face` |

Web usage: either `uknc.bdf` → WOFF2 for `@font-face`, or draw `uknc_font.json` bitmaps directly
on a `<canvas>` (each glyph is an 8×8 1-bit matrix).

## Coverage

Current: **36 glyphs** from the boot menu — `()-0123567:` `АГЗКПРСУ` `авгдеиклмнорстфыь`.

To complete the set the game needs (`ч п Н Б И 4 8 9 й ж ш я з` …), extract from more rendered
sources — the КЛАД title (`Николаев 1987` / `Баранов`), the HUD (`Счет` / `Попытки`), or drive
the headless emulator to type the missing characters — then add their indices to `GLYPH_MAP`.

## Regenerate

```bash
python3 tools/extract_uknc_font.py dump    # -> /tmp/glyph_dump.png (numbered; read off chars)
python3 tools/extract_uknc_font.py build   # -> font/uknc_font.{h,png,json} + uknc.bdf
```
