# FIDELITY KPI — КЛАД C23 reimplementation vs real УКНЦ

**Scorecard.** Each element is compared against the **real УКНЦ emulator** (QtUkncBtl
running `KLAD.SAV` from `fodos_games.dsk`). Reference captures live in
`assets/original/extracted/uknc/reference_emu/`.

## Status legend
- ❌ **WRONG** — confirmed different from emulator
- 🔶 **UNVERIFIED** — implemented but not yet compared pixel/behavior to emulator
- ✅ **VALIDATED** — matches emulator (cite the comparison)

**Rule:** never mark ✅ from assumption. Only after a side-by-side with an emulator
capture. The emulator is the single source of truth (not docs, not the BK-0010 build).

---

## Score: 1 / 18 validated (updated 2026-05-31)

---

## A. Tiles (8×8, from tile bank 017450)

| # | Element | Emulator ground truth | Current | Status | Evidence |
|---|---------|----------------------|---------|--------|----------|
| A1 | Wall | white dotted/dashed texture on background | TILE_GFX[9/11/12/13] 1-wide | 🔶 | level1_full.png |
| A2 | Ladder | vertical rails + horizontal rungs | TILE_GFX[1/8] cross, 1-wide → solid pole | ❌ | player_zoom.png, TILE_FIDELITY.md |
| A3 | Gold | (to be zoomed in emulator) | TILE_GFX[4] yellow block (== wall bytes) | ❌ | TILE_FIDELITY.md |
| A4 | Water | (to be zoomed) animated | TILE_GFX[7] green band | 🔶 | — |
| A5 | Exit | (to be zoomed) | faint outline only | 🔶 | — |

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
./AppRun "-disk0:<abs>/assets/original/extracted/uknc/fodos_games.dsk" -autostart -boot1
# at ФОДОС prompt:  R KLAD   (xdotool WITHOUT --window; click screen first to focus)
```
