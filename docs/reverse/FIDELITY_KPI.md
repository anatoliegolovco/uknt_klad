# FIDELITY KPI — КЛАД C23 reimplementation vs real УКНЦ

**Scorecard.** Each element is compared against the **real УКНЦ emulator** (QtUkncBtl
running `KLAD.SAV` from `fodos_games.dsk`). Reference captures live in
`assets/uknc/reference_emu/`.

## Status legend
- ❌ **WRONG** — confirmed different from emulator
- 🔶 **UNVERIFIED** — implemented but not yet compared pixel/behavior to emulator
- ✅ **VALIDATED** — matches emulator (cite the comparison)

**Rule:** never mark ✅ from assumption. Only after a side-by-side with an emulator
capture. The emulator is the single source of truth (not docs, not the BK-0010 build).

---

## Score: 7 / 18 validated (updated 2026-05-31)

**Validated 2026-05-31** (side-by-side `reference_emu/compare/level1_{emulator,mine}.png`):
- ✅ **Level structure** — ladder/platform/wall positions match the emulator exactly.
- Findings: УКНЦ bg index 0 = **blue (0,0,255)**, not black (my palette wrong).
  Ladders render as thin rails (emulator) vs thicker pole (mine) — minor.
  Player/enemy = small men (emulator) vs teal blobs (mine). Lives HUD = 219.
- ASM note: `LEVEL_RENDER_R4` (005002) draws 32 tiles/row (nibble-packed), one 8×8
  tile per cell — there is NO 2-half-tile overlay at map level (corrects TILE_FIDELITY
  agent theory). The rails-per-ladder come from the УКНЦ blit (DISP_COL_BLIT +
  DISP_SCANLINE_WRITE write each tile via video ports 176642/3), not from the map.

---

## A. Tiles (8×8, from tile bank 017450)

| # | Element | Emulator ground truth | Current | Status | Evidence |
|---|---------|----------------------|---------|--------|----------|
| A1 | Wall | white dotted texture on blue | 16×8 1bpp, matches | ✅ | compare/level1_mine_1bpp.png |
| A2 | Ladder | vertical rails + horizontal rungs | 16×8 1bpp → rails+rungs | ✅ | compare/level1_mine_1bpp.png |
| A3 | Gold | treasure-chest shape (white box + coin slots) | 16×8 1bpp, treasure shape | ✅ | compare/maptiles_16x8_1bpp.png |
| A4 | Water | wavy white surface on blue | 16×8 1bpp, wavy surface | ✅ | compare/maptiles_16x8_1bpp.png |
| A5 | Exit | invisible (empty tile) | empty + faint marker | ✅ | compare/maptiles_16x8_1bpp.png |

## B. Sprites / animation

| # | Element | Emulator ground truth | Current | Status | Evidence |
|---|---------|----------------------|---------|--------|----------|
| B1 | Player shape | white humanoid (красный человечек), ~8px | tile-bank overlay (was crocodile/square) | ❌ | player_zoom.png |
| B2 | Enemy shape | green humanoid (зелёный человечек) | enemy walk tiles | ❌ | (to be zoomed) |
| B3 | Player walk anim | (frames to be captured) | 2-frame cycle | 🔶 | — |
| B4 | Player climb anim | (to be captured) | climb tile | 🔶 | — |
| B5 | Enemy anim | 8-frame horiz / 5-frame vert | frame cycle | 🔶 | ANIMATIONS.md |

## C. Mechanics (from ASM, behavior-verified in emulator)

| # | Element | Emulator ground truth | Current | Status | Evidence |
|---|---------|----------------------|---------|--------|----------|
| C1 | Player movement keys | arrows; shoot Q/S | WASD/arrows | 🔶 | UKNC_BUILDS.md intro text |
| C2 | Gravity / fall | falls to floor | implemented | 🔶 | — |
| C3 | Ladder climb | up/down on ladder | implemented | 🔶 | — |
| C4 | Gold collect → score | +N points | +10 | 🔶 | MECHANICS.md |
| C5 | Water death | lose life | implemented | 🔶 | — |
| C6 | Enemy collision | lose life | implemented | 🔶 | — |
| C7 | Level complete | advance | implemented | 🔶 | — |
| C8 | Enemy AI | green-man chase | greedy chase | 🔶 | MECHANICS.md |

## D. HUD / shell

| # | Element | Emulator ground truth | Current | Status | Evidence |
|---|---------|----------------------|---------|--------|----------|
| D1 | Lives display | **219** ("Попытки 219") | "9" | ❌→fixing | level1_full.png |
| D2 | Score label | "Счет N" | "Scor:N" (RO) | ✅ | level1_full.png (label is localization choice; value semantics match) |
| D3 | Title screen | КЛАД / Николаев 1987 / Баранов | none | 🔶 | title.png |
| D4 | Speed select 1–4 | keys 1–4 at start | none | 🔶 | UKNC_BUILDS.md |

---

## How to validate an item (repeatable)
1. Capture the element in the emulator: `tools/shot.py <WID> out.png`, crop+zoom with PIL.
2. Render the same element from the C build (or decode the tile from binary).
3. Side-by-side at equal zoom; compare pixel shape / behavior.
4. Update the row: ❌/🔶/✅ + the evidence filename. Bump the score line.

## Emulator quick-start
```
cd /tmp/squashfs-root
./AppRun "-disk0:<abs>/assets/uknc/fodos_games.dsk" -autostart -boot1
# at ФОДОС prompt:  R KLAD   (xdotool WITHOUT --window; click screen first to focus)
```
