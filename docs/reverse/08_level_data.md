# 08 — Level / graphics data (КЛАД, KLAD3.BIN)

## ✅ SOLVED (blit-routine trace) — format fully recovered

Tracing the tile renderer cracked the whole thing. The blit subroutine at
`005106` copies **8 words** (one 8×8 tile, 2bpp) from source `(R2)+` into the
screen at `46000(R1)`, advancing R1 by `#100` (one scanline) per row. The caller
at `005010` reads a map byte, isolates each **nibble as a tile index** (`BIC
#177760` → low nibble; `BIC #177417` → high nibble), and forms the source as
**`#017450 + index*16`**.

Recovered format:

| thing            | value |
|------------------|-------|
| tile bank        | octal **`017450`**, 16 tiles, **16 bytes each = 8×8 @ 2bpp** |
| pixel order      | row = 1 little-endian word; pixel *p* = bits `(2p,2p+1)`, LSB = left |
| map → tile       | nibble index (low nibble = left cell, high nibble = right cell) |
| level table      | octal **`010406`** — 20 records × 10 words; field 0 steps `0540` |
| level maps       | `022100, 022640, …` one **352-byte** block each = **32×22 tiles** packed 2/byte |
| playfield base   | screen `046000` (= `040000` + 6 tile-rows) |

`tools/extract_tiles.py` dumps the 16 tiles and all 20 levels (raw `.bin` +
rendered `.png`) to `assets/original/extracted/klad3/`. Level 0 renders as a
coherent maze (border, ladders, water pools, treasure, guards) — confirming the
decode. **No emulator was needed**; the earlier caveat below is superseded for
KLAD3.

**Caveats / open items**
- KLAD3 is the **Баранов** line; its tiles differ from the **Crocodile 1991**
  screenshot (green/white blocks, not Crocodile's white diamond-mesh).
- KLAD/KLAD2/KLAD4 use the **same blit engine** (`MOV (R2)+,46000(R1)`) but have
  **no `0540` level table** — they store levels differently; their tile banks
  aren't pinned yet (the disassembler misframes their index math).
- Exact **palette** (which of black/cyan/green/white/yellow each 2-bit value
  maps to) depends on the runtime palette register; shapes are exact, hues are
  a reasonable guess in `extract_tiles.py:PALETTE`.

---

## (historical) earlier static-analysis notes

Goal: locate the original level data so it can be extracted (your request).
Status: **structural location found; semantics not yet confirmed.** Honest
state below — confirming what the bytes *mean* needs the emulator.

## The strongest structured table: 20 records @ `010406`

Scanning for regular structure turned up one clear table:

- A record array starting at **`010406`**, **20 entries**, **10 words (20
  bytes) per record**.
- The **first field of each record steps by exactly `0540` (octal) = 352
  decimal**: `022100, 022640, 023400, … 037140` — i.e. pointers into a packed
  data region `022100`–`037700`, one **352-byte block per entry**.
- This stride is corroborated by the init code at `01050`:
  `ADD #540,@#1300` — the program itself advances a pointer by `0540`. So
  `0540`-sized blocks are real, program-defined units, not a coincidence of our
  scan.

That `022100 .. 037700` region (≈3.5 KiB) sits in the upper part of the image,
exactly where bulk asset data is expected.

## What the 352-byte blocks look like

`tools/extract_data.py` dumps each block and renders it 1-bpp (BK convention:
set bit = pixel, LSB first). Rendered at 256 px wide (= 32 bytes → 11 rows) the
20 blocks are **clearly structured graphics** with stable left/right borders —
but they do **not** read as 20 distinct maze layouts at any width tried (256,
176, 128, 88, 64). Also each block has ~40 distinct byte values, **too many for
a 1-byte-per-tile map**.

Conclusions (ranked):

1. **Most likely:** the `010406` table is a **sprite / font / UI-glyph bank**,
   not the level table. КЛАД draws its maze from tiles; a glyph/sprite bank of
   fixed-size cells fits "regular table of bitmap blocks" well.
2. **Possible:** level data *is* here but **encoded** (RLE or tile-indexed
   against the bank above), so raw 1-bpp rendering won't reveal it.
3. The true **level table** may be a different structure — candidates from the
   pointer-table scan include the dense regions around `021630` (84 in-image
   words) and `010320`/`010340` (sequences of `MOV …,014420/014430/014440`,
   suggesting three parallel buffers/streams).

## Confirmed: stored bitmaps DO decode directly (credits screen) ✅

Direct 1-bpp rendering (LSB-first) of `KLAD3.BIN` at the screen width (256 px /
32 bytes) reads cleanly for **pre-composed bitmap regions**:

- A **credits screen at octal `013740`** renders as legible text + a stylised
  "КЛАД" logo: *"ул. Шевченко д.32/108, г.Николаев, тел. 37*82*52, Баранов Д."*
  This **identifies KLAD3 as the Баранов (Николаев) authorship line** and proves
  the decoder/bit-order are correct.

So the extraction method is sound — for **stored bitmaps**. What it does *not*
recover is the **gameplay tile set**, because those are not stored as a plain
bitmap bank:

- 8×8 de-interleaved contact sheets of the whole image are code-noise; no clean
  tile bank stands out (tried 8×8, 16×16, multiple offsets).
- The `010406` 352-byte blocks render as *structured but encoded* data (~40
  distinct byte values/block — too many for 1-byte-per-tile).
- Disassembly shows drawing routed through a **screen-address table** (the run
  of `040000`-range words at `020300`–`021100` is such a table) plus an encoded
  source — not a straight sprite blit. Wall textures (the diamond mesh) are most
  plausibly a **procedural fill pattern**, so there is no stored "wall sprite".

**Bottom line:** pixel-exact tiles require seeing them *rendered* and tracing the
bytes back (emulator VRAM dump, below), or fully reversing the encoded
draw routine. Until then the re-imagining reproduces tiles from screen
reference (see `src/game.c` tile bank), which already matches the original look.

## How to confirm (emulator-assisted — do locally) 🖥️

Static analysis cannot tell sprite-bank from level-bank with certainty. The
decisive experiment:

1. Run `KLAD3.BIN` in **bkbtl** (BK-0010 emulator), reach level 1.
2. Dump video RAM (`040000`–`077777`) — that's the rendered screen.
3. Find which source bytes produced it: breakpoint writes into `040000+`, note
   the source address the blit reads from. That address range *is* the level/
   sprite data; compare to `022100…` and `014xxx…`.
4. Single-step the routine at the blit source to learn the encoding
   (raw copy? RLE? tile indices into the `010406` bank?).
5. Feed the confirmed format back here and into `tools/extract_data.py`
   (`--first/--stride/--width`, or a new decoder mode).

## Reproduce the extraction

```bash
tools/fetch_original.sh           # blob -> assets/original/ (tracked)
python3 tools/extract_data.py assets/original/ex_klad3/KLAD3.BIN \
        --montage /tmp/klad3_blocks.png
# re-render at another width, e.g. 176px:
python3 tools/extract_data.py assets/original/ex_klad3/KLAD3.BIN --width 176 \
        --montage /tmp/w176.png
```

Output (`.bin` + `.png` per block) lands in `assets/original/extracted/`, which
is **tracked** in this personal, non-commercial repo (see `design/legal.md`).

## Note

The extracted data is kept for personal study and to inform the re-imagining's
own level design. See `design/legal.md`.
