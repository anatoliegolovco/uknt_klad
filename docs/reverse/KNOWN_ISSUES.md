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

## Verified OK
- **Level 1 layout** — our maze tile-grid matches the original (`02_gameplay.png`) modulo a
  ~2-row alignment offset in the diff; the structure (borders, ladder columns, platforms)
  coincides. Level data (`level_data.h` from the binary) is faithful.

## KI-3 — font coverage incomplete (43 / ~51 glyphs)
**What:** `()-012345679:АГЗКНПРСУШавгдеиклмнопрстфчыья`. Missing: `Б И й ж з ш у 8`.
**Path:** KI-1 / KI-2 resolve these. The game text falls back to a blank advance for
missing glyphs (no crash).
