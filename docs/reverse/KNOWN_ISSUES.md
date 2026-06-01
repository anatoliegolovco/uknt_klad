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

## KI-4 — character sprite decode (tiles 16-31) likely wrong
**What:** the player/enemy sprite tiles (char 16-31) decoded with the current map rules
(structural=side-by-side, gold=OR) come out as scattered dither (`..#.#.#.`), not a human
figure. Tile 16 has body/legs-like blocks but the rest looks scrambled.
**Impact:** player/enemy may not render as the faithful figure.
**Path:** same investigation as the tile decode (TILE_FIDELITY.md) but for sprites — compare
against the player figure in `02_gameplay.png` (top-left at spawn) and find the right rule
(likely OR like gold, or 8×8). Verify in `tools/gen_gfx_c.py` + `render.c` CHAR_* slots.

## Verified OK
- **Level 1 layout** — our maze tile-grid matches the original (`02_gameplay.png`) modulo a
  ~2-row alignment offset in the diff; the structure (borders, ladder columns, platforms)
  coincides. Level data (`level_data.h` from the binary) is faithful.

## KI-3 — font coverage incomplete (43 / ~51 glyphs)
**What:** `()-012345679:АГЗКНПРСУШавгдеиклмнопрстфчыья`. Missing: `Б И й ж з ш у 8`.
**Path:** KI-1 / KI-2 resolve these. The game text falls back to a blank advance for
missing glyphs (no crash).
