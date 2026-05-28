# Roadmap

Ordered next steps. ⬜ = todo, 🖥️ = needs a local GUI desktop (can't run in the
headless container).

## Reverse engineering (study)
- 🖥️ Run `KLAD3.BIN` in `bkbtl`, reach level 1, dump video RAM (`040000`).
- 🖥️ Breakpoint blits into `040000+`; find the source data address & the
  encoding (raw / RLE / tile-index into the `010406` bank).
- ⬜ Feed the confirmed format into `tools/extract_data.py` (decoder mode).
- ⬜ Add recursive-descent to `pdp11dis.py` to cut linear-sweep noise.
- ⬜ Map input codes (`@#177662`) → control scheme; write `05_input.md`.
- ⬜ Find collision/water-death logic; write `07_collision.md`.

## Reimplementation (the sellable product — original content only)
- ⬜ `src/level.*`: tile-map model + loader (our own level files).
- ⬜ **Author original levels** "inspired by" the originals (replaces shipping
  the copyrighted layouts — see `legal.md`). Optional procedural generation.
- ⬜ `src/entity.*`: guardians (patrol AI), player polish, gravity tuning.
- ⬜ Scoring, lives, title / level-clear / game-over screens (i18n-driven).
- ⬜ `src/audio.*`: SFX (raylib audio).
- ⬜ Original tile art under `assets/new/` (8×8, own palette).
- ⬜ Custom font with Romanian diacritics; restore ă/î/ș/ț in `i18n.h`.
- ⬜ Verify the WASM build on desktop + mobile browsers; tune the touch D-pad.

## Commercial readiness (if monetising)
- ⬜ Confirm **all** shipped assets are original or licensed (audit).
- ⬜ Finalise game name + branding (currently working title).
- ⬜ Choose distribution (itch.io / web host / app wrapper) and licence terms.
- ⬜ Add credits/inspiration acknowledgement in-game and in README.

## Infrastructure
- ⬜ CI: build native + WASM on push; run the `pdp11dis.py` self-tests.
- ⬜ `rt11dsk` build for if/when a real `.dsk` (e.g. the 40-in-1 image) appears.
