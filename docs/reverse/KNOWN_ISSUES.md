# KNOWN ISSUES — КЛАД 1:1 port

Tracked deviations / open bugs. Each has a clear path to resolution; none blocks the build.

## KI-1 — РУС keyboard mode in the headless emulator
**What:** `vendor/ukncbtl-qt/headless` "glyphs" mode types the missing Cyrillic letters at
the ФОДОС prompt to capture their glyphs, but pressing **АЛФ (0106)** does not switch the
keyboard from ЛАТ to РУС — Latin still echoes (И→I, Ж→V, Ш→[, У→U). Б and 8 do come through.
**Impact:** can't yet capture И й ж з ш у Ч via typing. The font is missing those 6 glyphs.
**Path:** find the correct РУС-mode switch (likely not АЛФ — try ГРАФ 0066 / FIKS 0107 /
a Shift+АЛФ combo, or read how ФОДОС sets the mode). Alternatively capture these letters
from КЛАД's own win/lose screens ("Уровень пройден" → й, "Игра окончена" → И, "нажмите
клавишу" → ж ш у, "Поздравляем" → з) by driving the emulator to those states.

## KI-2 — title credits: Н mis-mapped, Б/8 missing
**What:** the title renders "−иколаев 19 7" / "аранов" — `Н` shows as a dash (wrong index
in `tools/extract_uknc_font.py` GLYPH_MAP for the title source) and `Б`, `8` are absent.
**Impact:** cosmetic, title credit line only. HUD and speed-select are correct.
**Path:** re-dump the title (`extract_uknc_font.py dump 01_title.png`), read the correct
index for `Н`, fix the map; extract `Б`/`8` from the "Баранов"/"1987" credit lines
(group-detection restricted to the centre columns, away from the КЛАД tile-art).

## KI-4 — character sprite decode (tiles 16-31) — IN PROGRESS
**What:** the player/enemy sprite tiles (char 16-31) decoded with the current map rules
come out as scattered dither, not a figure.
**Reference captured:** the headless emulator's `sprite` mode (frame-diff: screenshot at
spawn, move right, screenshot again — the changed pixels isolate the player) gives the real
player figure (`assets/uknc/reference_emu/sprites/player_spawn.png`):
```
....####..##
....####..##
##....##..##
##########..   ← arms out
....####....
....##..##..   ← legs
....##..##..
....##......
```
**Open puzzle:** the figure renders **12 px wide** on screen (tiles are 16 px), so it does
NOT map cleanly onto an 8-px char tile under OR/planes/side-by-side (best brute-force match
only 44/64). The УКНЦ sprite blit (`SPRITE_DRAW` 014030, XOR-based 014772/015072/…) may use
a different width/scale or a different data layout than the map tiles.
**Path:** trace `SPRITE_DRAW` to see how many bytes/scale per sprite row; re-extract a clean
16-px figure from frame B; brute-force tile×decode against it; then fix `gen_gfx_c.py` char
decode + `render.c` CHAR_* slots. Same method that solved the map-tile decode.

**Progress (traced `SPRITE_DRAW` 014030):** the player is drawn as **TWO tiles** side by
side (`JSR TILE_BLIT_REV` twice — "left tile" + right), each 8 px → 16 px sprite (figure
fills ~12). Table @20270 holds per-direction X/Y offsets (not tile indices); the tile index
is the entity record `6(R4)`, set by `SPRITE_HELPERS` 013216. **Decode still open:** char
tile 16 OR-decodes to a figure-like half, but tiles 17-23 OR-decode to a `.#.#.#.` dither
(same symptom the map tiles had before the side-by-side/OR split was found). Next: determine
the sprite plane layout (likely NOT the same as map tiles), then which tile-pair = which
animation frame, then render the player as 2 tiles in `render.c`.

**Traced `TILE_BLIT_REV` 014302 (the sprite blit):** it computes the tile data address as
`index*16 + 017450`, loops `R5=8` rows, and calls the **same `DISP_SCANLINE_WRITE`** (entry
040140) the map tiles use (2 bytes/row). So sprites share the map-tile pixel format → each
sprite tile is 8 px (OR of the two row bytes, like gold), and the player = 2 such tiles =
16 px wide (figure fills ~12). **Remaining:** the figure-to-tile match is still fuzzy (tile
16 OR ≈ a figure half; 17-23 OR ≈ dither), so the open question is *which char tiles are the
standing/walking/climbing player frames* (via `SPRITE_HELPERS` 013216 + the entity record),
not the decode rule. Implementation: char tiles 16-31 → OR(8px) in `gen_gfx_c.py`; render the
player as the correct 2-tile pair in `render.c`.

**Traced `SPRITE_HELPERS` 013216 + table 012410:** the player tile index for state s is
`tile = byte[012410 + (s-0o21)*2 + dir]`; the non-zero entries are tiles **18 and 20**
(0o22/0o24) — climb=18, walk=20 (matches the indices `render.c` already uses). BUT a
brute-force of the captured player figure against every char tile × {OR, plane0, plane1,
side-by-side L/R, plane-OR} tops out at **42/64** — no clean match. Two complications make
the raw tile ≠ the on-screen figure:
  1. `SPRITE_DRAW` composes the sprite from **two tiles at X/Y offsets** (table 020270 holds
     per-direction dx/dy), so the figure is not a simple col-0/col-8 split of one decode.
  2. The blit is **XOR onto the background** (`074437 XOR ...` in the 040140 path, and
     014772/015072), so a captured figure = sprite ⊕ background unless the player stands on
     pure blue (rare in КЛАД).
**Next (deeper):** model the exact `SPRITE_DRAW` offset+XOR composition (or run the two char
tiles through the C23 `uknc_emu` against a blank framebuffer) to reconstruct the true sprite,
then set `render.c` to draw tiles 18/20 (+ their partner) at the right offset. This is a
multi-step modeling task — the architecture is fully traced, the pixel reconstruction isn't.

## Verified OK
- **Level 1 layout** — our maze tile-grid matches the original (`02_gameplay.png`) modulo a
  ~2-row alignment offset in the diff; the structure (borders, ladder columns, platforms)
  coincides. Level data (`level_data.h` from the binary) is faithful.

## KI-3 — font coverage incomplete (43 / ~51 glyphs)
**What:** `()-012345679:АГЗКНПРСУШавгдеиклмнопрстфчыья`. Missing: `Б И й ж з ш у 8`.
**Path:** KI-1 / KI-2 resolve these. The game text falls back to a blank advance for
missing glyphs (no crash).
