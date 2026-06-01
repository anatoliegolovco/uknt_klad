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

**✅ Video model implemented in `uknc_emu` (the authoritative renderer).** From `emubase`:
port 176640 = plane address, 176642→plane1, 176643→plane2, pixel = 3-bit plane combo.
`uknc_emu blit <tile>` now executes the REAL `TILE_BLIT_REV` against this model and reads
plane1. Results (authoritative — the actual game code rendering):
- ladder (1) → `..####....####..` = **2 rails** ✓
- gold (4)  → `######....######` = **2 blocks** (NOT a solid chest!) — so map tiles are
  unambiguously **side-by-side 16px**. The solid chest the player sees is the GOLD drawn as
  a **sprite** (2 tiles + offsets), not as a map tile. (Our OR gold happens to match the
  sprite look, so it stays.)
- player tiles 18/20 (from table 012410) → still render as sparse **dither** via plain
  `TILE_BLIT_REV`, so the clean captured figure is NOT a plain blit of those tiles → the
  player sprite must use a different composition (XOR path 040220, or different tiles).
**Open:** use `uknc_emu` to execute the full `SPRITE_DRAW` (set up the player entity record)
and read plane1 → the true player sprite, frame by frame.

**✅ RESOLVED understanding (executed SPRITE_DRAW in uknc_emu).** Drove the real game to
gameplay inside the C23 emulator and read the player from plane1 at its entity position
(player entity @014420, `[6]`=plane pos). The raw plane1 player is **dither** (`#.#.#.#.`) —
NOT a clean figure. The clean figure only appears after the УКНЦ **display transform**
(3-plane combine + per-line scale + palette in `Emulator_PrepareScreenRGB32`). So the
faithful player sprite is the **DISPLAYED output** (captured from the Qt/headless emulator,
`reference_emu/sprites/player_spawn.png`), not the raw char-tile/plane bytes — those are
dither by design. Conclusion: bake the captured displayed sprite into `render.c` (like the
font, extracted from rendered output). Walk/climb frames: capture the same way at those poses.

**Walk/climb animation (captured via the `sprite` mode, frame-diff at multiple phases):**
- **Walk** — the player figure is the SAME across walk frames (captured at several phases) →
  КЛАД uses a single figure for stand/walk (no visible walk animation). The single
  `PLAYER_ART` already baked is faithful. ✓
- **Climb** — there IS a distinct climbing pose, but the player is ON a ladder, so the
  capture = sprite ⊕ ladder rungs (garbled). A clean climb frame needs subtracting the ladder
  pattern or the display-transform path. Minor refinement; current code uses the walk figure
  while climbing (acceptable). Frames saved: `/tmp/spr_{rest,w1..w3,c1,c2}.ppm`.

## Verified OK
- **Level 1 layout** — our maze tile-grid matches the original (`02_gameplay.png`) modulo a
  ~2-row alignment offset in the diff; the structure (borders, ladder columns, platforms)
  coincides. Level data (`level_data.h` from the binary) is faithful.

## KI-3 — font coverage incomplete (43 / ~51 glyphs)
**What:** `()-012345679:АГЗКНПРСУШавгдеиклмнопрстфчыья`. Missing: `Б И й ж з ш у 8`.
**Path:** KI-1 / KI-2 resolve these. The game text falls back to a blank advance for
missing glyphs (no crash).

## KI-5 — player not animated like the original
**What:** in our impl the player figure doesn't animate the way the original does. (Earlier
frame-diff captures of walk looked static, but the user observes the original animates.)
**Impact:** visual fidelity. **Path:** re-capture player frames at finer anim phases
(ANIM_THROTTLE_PLAYER 007432 changes frame every 4 ticks) via the headless `sprite` mode;
the original may cycle 2 leg-position frames. Investigate later.

## KI-6 — enemy chase AI not working visibly
**What:** `enemy.c do_move` has greedy chase code (toward player, horiz-first, vertical on
ladders, from ENEMY2_MOVE 006602), but the adversary doesn't appear to chase in-game.
**Impact:** gameplay. **Path:** verify enemy_init spawns + enemy_tick runs each frame; check
speed/throttle; compare to ENEMY1/2/3_TICK. Investigate.

## KI-7 — enemy sprite identical to the player
**What:** `render_enemy` now draws PLAYER_ART, so enemies look exactly like the player. The
original enemies were **hatched/different** (distinct texture).
**Impact:** can't tell player from enemy. **Path:** capture the enemy figure via frame-diff
(the enemy moves on its own — diff two frames isolates it), bake a separate ENEMY_ART.

## KI-8 — player can't reach level goals (gets stuck) — FOUND BY REACHABILITY E2E
**What:** `tools/reachability.c` (BFS over the movement graph, same collision as map.c) shows
the player reaches only the bottom rows + a few ladder stubs in EVERY level; the exit is
unreachable (0/1) in all 10 levels and most gold is unreachable. The player gets stuck.
**Overlay (level 1):** climbs e.g. the col-9 ladder to its top (row 17) but is walled in
left/right there — ladders dead-end instead of connecting to platforms.
**Likely causes (to investigate):**
  1. Movement too strict — the climb-up rule (`player.c can_climb_into`, up only into a
     T_LADDER cell) stops at a ladder top and can't step onto the platform above. (Loosening
     it risks the old "climb through ceiling" bug — delicate.)
  2. Level-data vertical offset — the earlier tile-grid diff vs the original showed a ~2-row
     offset; if real, ladders wouldn't line up with platforms.
**Ground truth needed (KI/T8):** extract the real per-level reachability from uknc_emu
(COLLISION_MAP_BUILD flags) — if the real game reaches everything but we don't, it's our
movement/data. This is the concrete next step before changing the climb rule.

### KI-8 ground-truth result (uknc_emu) — CONCLUSIVE: our movement is the bug
Extracted the REAL collision flags from uknc_emu (BUF_TILE_WORK 014550 after
COLLISION_MAP_BUILD) and BFS'd them from the real player cell (014422 → cell 642 = col2,
row20). With the documented direction bits (right=0o1000, left=0o400, up=0o20000,
down=0o10000) the REAL game reaches **363/704 cells, exit 1/1, gold 6/8** — i.e. it
navigates to the exit and gold. Our impl reaches ~53 cells, exit 0/1.
**Root cause:** the original moves by **per-cell direction flags** (R/L/U/D computed by
COLLISION_MAP_BUILD); our `map.c`/`player.c` use heuristics (map_solid + climb-only-into-
ladder) that are far too strict. **Faithful fix:** reimplement COLLISION_MAP_BUILD's flag
computation (or bake the per-level flag maps extracted from uknc_emu) and make player
movement obey the flags — then the player can traverse like the original. This is the fix
for KI-8 (the player getting stuck) and supersedes the delicate ad-hoc climb rule.

### KI-8 FIX applied — player movement now uses the faithful collision flags
`map.c` now exposes `map_can_right/left/up/down/grounded` computed EXACTLY like
COLLISION_MAP_BUILD (013570): right/left = neighbour raw tile ≤ 8; up = current==8 (ladder2)
&& above ≤ 8; down = below==8; grounded = current==8 || below>6. `player.c player_update`
was rewritten to move STRICTLY by these flags (climb → walk-on-support → gravity), with
grid snapping. Result: the player navigates (reachability: level 1 212 cells incl. exit
1/1, was 53/exit-0; levels 4 & 10 also reach the exit). E2E collision invariants still 10/10.
**Still open:** levels 2,3,5,6,7,8,9 show the EXIT unreachable — but the КЛАД win condition
is collecting gold (gold_c=tile6 → LEVEL_COMPLETE), not touching the exit tile; and those
levels' DATA may be mis-extracted. Next: validate per-level reachability vs uknc_emu ground
truth for all 10 (T8 done only for level 1) + use gold_c as the completability metric.
