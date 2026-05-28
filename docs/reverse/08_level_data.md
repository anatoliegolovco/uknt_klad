# 08 — Level / graphics data (КЛАД, KLAD3.BIN)

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
tools/fetch_original.sh           # blob -> assets/original/ (git-ignored)
python3 tools/extract_data.py assets/original/ex_klad3/KLAD3.BIN \
        --montage /tmp/klad3_blocks.png
# re-render at another width, e.g. 176px:
python3 tools/extract_data.py assets/original/ex_klad3/KLAD3.BIN --width 176 \
        --montage /tmp/w176.png
```

Output (`.bin` + `.png` per block) lands in `assets/original/extracted/`, which
is **git-ignored** — these are derivatives of the copyrighted binary and stay
local (see the note in `00_overview.md` and the project's clean-room stance).

## Clean-room caveat on shipping extracted levels

You asked to extract the original levels. Extracting them as **local study
references** is fine. *Shipping* the original layouts in the public build is a
separate decision: level designs carry their own copyright, independent of the
code. Recommendation: use extracted layouts as reference to author our own
("inspired by") levels, or decide explicitly to include them. Tracked as an
open question, not assumed.
