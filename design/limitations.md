# Limitations & constraints

Honest record of what is *not* solved or *cannot* be done in the current setup.

## Environment

- **Headless cloud container.** Development happens partly in an ephemeral
  container with **no display**. Static work (download, extract, disassemble,
  document, build-check) runs fine. **GUI work cannot.**
- **No GPU/window.** The C23/raylib game compiles here (syntax-verified under
  `-std=c2x` against the raylib API) but cannot *run* — no display, and raylib
  isn't packaged in this container's apt repos. Native run and the WASM build
  are done on the local Ubuntu desktop.
- **Ephemeral storage.** Anything not committed/pushed is lost when the
  container is reclaimed. The original blob is intentionally never committed, so
  it must be re-fetched locally via `tools/fetch_original.sh`.

## Reverse engineering

- **Dynamic analysis is blocked here.** Confirming boot flow, input mapping,
  sprite blit, collision, and especially the **level data format** requires a
  BK-0010 emulator with a debugger (`bkbtl`). This is a local-desktop task; the
  exact procedure is in `docs/reverse/08_level_data.md` and
  `docs/reverse/TODO.md`.
- **Level format not cracked.** We located a strong 20-entry / 352-byte table at
  `010406` (stride `0540`, corroborated by `ADD #540,@#1300` in init), but its
  blocks render as structured graphics, not obvious maze layouts, and carry too
  many distinct byte values for a 1-byte tilemap. It is **probably a sprite/font
  bank or encoded data**, not the level table. Unconfirmed.
- **Linear-sweep disassembly only.** `pdp11dis.py` does not yet follow control
  flow, so data regions appear as bogus instructions. A recursive-descent mode
  is a future improvement.
- **Five variants, not fully distinguished.** `KLAD/2/3/4` and `klad10` differ;
  only `KLAD3` has a clean `JMP @#4000` entry. The others begin with a data/
  loader preamble whose true entry needs dynamic analysis. Which one is the
  "canonical" Crocodile vs Baranov build is not yet established.

## Platform

- **Studied build is BK-0010, not УКНЦ.** The УКНЦ "GRB palette" build was not
  found on a reachable mirror (`r-games.net` does not resolve here). Mechanics
  are identical, so this does not block the reimplementation.

## Legal / commercial

- **Original assets are off-limits for shipping.** See `legal.md` and decision
  D7. The original levels/graphics are copyrighted; they are used only as local
  reference. This bounds what can be in the sellable product to original work.
