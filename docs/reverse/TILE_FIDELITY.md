# TILE_FIDELITY — Ladders & Collectibles shape audit

**Date:** 2026-05-31
**Scope:** Compare the SHAPE of ladders (scări) and gold collectibles (aur) between the
original game (BK-0010 emulator capture + УКНЦ-extracted tiles) and the C reimplementation.
**Method:** byte-level decode of the tile bank, pixel analysis of emulator screenshots,
and reconstruction of how tiles look when tiled. **Observation only — no source modified.**

Authoritative byte values come from `docs/reverse/GFX_MAP.md` (confirmed from disassembly)
and from re-decoding `assets/uknc/tiles/*.png` and
`assets/uknc/KLAD_1987_Baranov.SAV`.

Inputs analysed: `/tmp/emu_lvl.png`, `/tmp/emu_right.png`, `/tmp/emu_left.png`,
`/tmp/my_tiles.png`, `/tmp/my_field.png`, the extracted 64×64 tile PNGs, and
`assets/uknc/levels/level_01.png`.

---

## TL;DR

| Element | Original (emulator) | Reimplementation | Match? |
|---------|---------------------|------------------|--------|
| **Ladder** | Two solid vertical **side rails** with horizontal **rungs** between them; ~2 tiles wide | One solid **center bar / pole** with periodic widening bumps; 1 tile wide | ❌ NO |
| **Gold** | (not isolable in this B&W capture) — УКНЦ bytes say full yellow textured block | Full yellow "space-invader" block, identical to the extracted bytes | ⚠ matches the *extraction*, but reads as a wall-brick, not a coin |

The reimplementation **faithfully reproduces the extracted tile bytes**. The divergence
from the *visual original* comes from how those bytes are (a) **mapped to screen pixels**
and (b) **how many tiles wide each structure is drawn**. The original draws ladders/walls
**2 half-plane tiles wide**; the reimplementation draws every map tile **1 tile wide**.

---

## Ladders

### 1. The actual tile bytes (from the binary)

Tile 1 (`ladder`) and tile 8 (`ladder2`) are byte-identical. Pixel plane = colour plane:

```
3C 3C FF FF 3C 3C 3C 3C
```

Decoded as 8 horizontal pixels per byte (MSB-first; identical for LSB-first because each
byte is symmetric), white where bit=1:

```
..####..   3C
..####..   3C
########   FF
########   FF
..####..   3C
..####..   3C
..####..   3C
..####..   3C
```

This is a **centered cross / plus**: a 4-px-wide central block with two full-width rungs.
The extracted PNGs `tile_01_ladder.png` / `tile_08_ladder2.png` confirm this exactly, and
`src/gfx_data.h` `TILE_GFX[1]`/`TILE_GFX[8]` reproduce it byte-for-byte.

### 2. What the original ACTUALLY renders (emulator)

In `/tmp/emu_lvl.png` (and the crops `/tmp/emu_left_lower.png`, `/tmp/emu_ladder_zoom.png`)
every ladder is unmistakably a **classic ladder**: two solid vertical white rails on the
**outer edges** with a black channel in the middle that is crossed by **horizontal rungs**
at a regular pitch. Pixel measurement of one ladder column:

- column white-fraction profile: left rail (~10 px solid) · middle (~10 px, white only on
  rung rows ≈ 27 %) · right rail (~10 px solid) → **rails at the edges, gap in the middle**.
- rung pitch ≈ 16 px = exactly one logical tile; rung ≈ 3–4 logical rows tall.

Reconstructed to an 8×8 logical tile from the emulator (`/tmp/emu_ladder_zoom.png`):

```
###..###   rails both sides, gap in middle
########   rung
########   rung
###..###   rails, middle gap
##...###
##...###
##...###
##...###
```

This is the **horizontal inverse** of the extracted cross: WHITE at the edges / BLACK in
the middle, versus the extracted tile's BLACK edges / WHITE middle. Each emulator ladder is
~32 px = **2 logical tiles wide**.

### 3. What the reimplementation renders

`src/render.c::render_map()` draws every non-air map tile once, **1 tile (8 px) wide**, at
`col*TILE_PX`. Stacking the `3C 3C FF FF 3C…` cross vertically (`/tmp/recon_ladder_1wide.png`,
confirmed in the live game crop `/tmp/myfield_ladder.png`) yields a single **solid white
vertical pole** with small bumps where the `FF FF` rungs stick out — NOT a ladder. The center
4-px column is continuous, so it fuses into one bar. See `/tmp/cmp_ladders.png`.

### 4. Discrepancy & root cause

- **Discrepancy:** ladders look like a solid vertical pole/spine instead of a rail-framed
  ladder. The visual is essentially color-inverted (white where the original is black, in the
  middle vs edges) and half the intended width.
- **Root cause (two compounding factors):**
  1. **Half-plane / planar pixel mapping not applied.** УКНЦ video is planar via ports
     `@#176640/176642` (see `CLAUDE.md` D3), and the level map stores **2 tiles per byte**
     (`docs/reverse/MECHANICS.md` line 107: "22×16 bytes, 2 tile-uri/byte"). The extractor
     and `gfx_data.h` treat each tile's 8 bytes as a plain left-to-right MSB-first bitmap.
     On real УКНЦ the 8 bits do not land on 8 contiguous screen pixels; the `3C` center block
     decodes to **edge rails** once the planar mapping is honoured, which is exactly what the
     emulator shows. Tell-tale evidence that the bank is half-plane data: walls `wall_b`/`wall_c`
     (`TILE_GFX[11]`/`[12]`) carry pixels **only in even columns** (`{3,0,3,0,…}`) and `wall_a`
     **only in odd columns** (`{0,3,0,3,…}`) — i.e. the tiles are interleaved halves meant to
     be combined, never a straight bitmap.
  2. **1-wide blit instead of 2-wide.** Even ignoring the planar mapping, the original draws
     each structural element across **2 tiles**; the reimplementation blits 1 tile wide
     (`render_map`), so the cross can never resolve into rails. Reconstructing a 2-wide pairing
     (`/tmp/recon_ladder_2wide.png`, `/tmp/recon_ladder_pair.png`) already pushes white toward
     the edges and opens a central channel — much closer to the emulator.

### 5. Recommended fix (ladders)

- **Preferred:** decode the УКНЦ tile bank through the **planar/half-plane pixel mapping**
  (combine the paired half-plane tiles and apply the УКНЦ bit→column layout) so a single
  logical tile already reads as the emulator's `###..###` rail pattern. Re-run
  `tools/extract_uknc_gfx.py` / `tools/gen_gfx_c.py` after fixing the bit layout, then the
  existing 1-wide `render_map` blit would be correct.
- **Quick visual approximation (if the planar decode is deferred):** hand-author the ladder
  tile in `TILE_GFX[1]`/`[8]` as rails-at-edges + rungs, i.e. invert the cross to:
  ```
  3,3,0,0,0,0,3,3
  3,3,0,0,0,0,3,3
  3,3,3,3,3,3,3,3
  3,3,3,3,3,3,3,3
  3,3,0,0,0,0,3,3
  3,3,0,0,0,0,3,3
  3,3,0,0,0,0,3,3
  3,3,0,0,0,0,3,3
  ```
  This makes a single 8-px tile already read as a ladder when stacked, without touching the
  blit width.

---

## Collectibles (gold / aur)

### 1. The actual tile bytes (from the binary)

Tiles 4, 5, 6 (`gold_a/b/c`) are byte-identical (the three "animation frames" are the same
in this build). Pixel plane = all zeros; colour plane:

```
FC 3F A8 2A FC 3F FC 3F
```

Decoded (MSB-first), yellow where bit=1:

```
YYYYYY..   FC
..YYYYYY   3F
Y.Y.Y...   A8
..Y.Y.Y.   2A
YYYYYY..   FC
..YYYYYY   3F
YYYYYY..   FC
..YYYYYY   3F
```

This is a **full 8×8 yellow textured block** ("space-invader" look). The extracted PNGs
`tile_04/05/06` and `src/gfx_data.h` `TILE_GFX[4..6]` reproduce it exactly.

**Critical equivalence:** these gold bytes `FC 3F A8 2A FC 3F FC 3F` are **identical to the
wall tile** `wall_a` (tile 9, `GFX_MAP.md`). Gold and wall share the same pattern; the only
difference is gold uses the **colour plane** (→ yellow) while the wall uses **both planes**
(→ white). So in the current decode, **gold = a yellow brick of wall texture**. Tiled 2×2 it
forms a seamless yellow wall fill (`/tmp/recon_gold_2x2.png`, `/tmp/cmp_gold.png`).

### 2. What the original renders

The provided emulator capture is monochrome (BK) and no gold piece could be isolated from
the wall hatching in it (gold and wall share the same bit pattern, so in B&W they are
indistinguishable — itself a hint that the decode is wrong). `level_01.png` is a **schematic
legend render** (solid 16×16 colour blocks: yellow=gold, blue=other, white=ladder), NOT a
pixel-accurate tile render, so it cannot be used to judge tile shape — its small "dots" were
just downscaling artefacts; at tile resolution each gold cell is a solid colour block.

### 3. What the reimplementation renders

`my_tiles.png` shows tiles 4/5/6 as the full yellow space-invader block — a 1-to-1 match with
the extracted bytes. In `my_field.png` gold appears as chunky yellow blobs the size of a full
tile.

### 4. Discrepancy & root cause

- **Discrepancy:** the collectible reads as a **full-tile yellow brick** that is visually the
  same shape as a wall (only recoloured), rather than a small distinct coin/treasure symbol.
- **Root cause:** the **same planar/half-plane decode problem** as the ladder. Gold sharing
  byte-for-byte the wall pattern is the strongest evidence: under a correct УКНЦ planar mapping
  these bytes are a *half* of a two-tile graphic, so a single decoded tile is meaningless on
  its own. The plain MSB-first bitmap interpretation turns that half-plane data into a generic
  textured block. The reimplementation is faithful to the (mis-)extraction, so the error is
  upstream in `tools/extract_uknc_gfx.py`, not in `render.c`.

### 5. Recommended fix (gold)

- Re-decode the tile bank with the correct УКНЦ planar/half-plane mapping (same fix as the
  ladder) and combine the paired half-plane tiles. Verify the result against a **colour
  emulator** capture (UKNCBTL) where gold is actually distinguishable from walls — a colour
  reference is required because the supplied BK capture is monochrome.
- As a stopgap, hand-author a small centered coin glyph for `TILE_GFX[4..6]` (e.g. a 4–6 px
  yellow disc on black) so the collectible is visually distinct from walls until the planar
  decode lands.

---

## Comparison images (saved to /tmp)

- `/tmp/cmp_ladders.png` — emulator ladder vs reimpl ladder vs gfx cross (1-wide).
- `/tmp/emu_ladder_zoom.png` — tight zoom of a real ladder (rails + rungs).
- `/tmp/emu_left_lower.png`, `/tmp/emu_right_upper.png`, `/tmp/emu_leftinterior.png` — emulator
  maze regions showing ladder structure clearly.
- `/tmp/myfield_ladder.png` — reimplementation ladder (pole artefact) from the live game.
- `/tmp/recon_ladder_1wide.png` / `_2wide.png` / `_pair.png` — gfx cross tiled, showing the
  center-bar (1-wide) vs edge-rails (2-wide) behaviour.
- `/tmp/cmp_gold.png`, `/tmp/recon_gold_2x2.png` — gold tile and its wall-like 2×2 tiling.

## Bottom line

Both elements are **byte-accurate to the УКНЦ extraction but not to the original's on-screen
appearance.** The single shared root cause is that the УКНЦ tile bank is **planar / half-plane
data drawn 2 tiles wide**, and both the extractor (`tools/extract_uknc_gfx.py`) and the 1-wide
blit in `src/render.c::render_map()` treat each tile as an independent straight bitmap. Fix the
planar decode (and/or draw structural tiles 2-wide) and the ladder will gain its side rails and
gold will stop looking like a recoloured wall.

---

## RESOLVED 2026-05-31 — tiles are 16×8 1bpp (not 8×8 2bpp)

Validated against the real УКНЦ emulator (not the mono BK capture used above).

**Root cause:** the УКНЦ tile is **16 pixels wide × 8 tall, 1 bit-per-pixel** (2-colour).
Evidence:
1. `DISP_SCANLINE_WRITE` (040060) consumes **2 bytes per row** (bytes 2r and 2r+1 →
   the 16-bit pixel word for row r), 8 rows = 16 bytes. NOT 8 pixel-plane + 8 colour-plane.
2. The emulator playfield contains **exactly two colours**: blue (0,0,255) bg + white fg.
   No yellow/green — the game is 2-colour. The prior 4-colour 2bpp decode invented colours
   that never appear on screen.
3. Decoding tile 1 (ladder, bytes `3C 3C FF FF …`) as 16×8 1bpp →
   `..####....####..` rows + `################` rung rows = **two rails + rungs**, exactly
   the emulator ladder. The 8×8 2bpp decode gave a single centred band (wrong).

**Consequence for the reimplementation:**
- Tiles are 16w×8h, 1bpp. Palette is 2 colours: index 0 = background, 1 = foreground.
- Playfield = 32 tiles × 16px = 512px wide, 22 rows × 8px = 176px tall.
- `tools/gen_gfx_c.py` and `extract_uknc_gfx.py` decode must change to 1bpp 16×8.
- The 4-colour `UKNC_PAL` (black/green/yellow/white) is wrong; use bg/fg (+ mono toggle).
- "красный/зелёный человечек" (red/green men) only differ in colour in УКНЦ RGB mode;
  in КЛАД's 2-colour mode they are white shapes distinguished by form/position.
