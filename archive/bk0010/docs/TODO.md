# Reverse-engineering TODO

## Why some steps are "do locally"

This repo is developed partly in a **headless cloud container** with no display.
The static pipeline (download, extract, disassemble, document) runs there fine.
The **dynamic** pipeline needs a GUI emulator and a screen, so it must be done
on your local Ubuntu desktop. Those steps are marked 🖥️ below.

## Static (can run anywhere)
- [x] Download + verify blobs (`tools/fetch_original.sh`)
- [x] Identify container/platform (`00_overview.md`)
- [x] Linear-sweep disassembly (`tools/disasm.sh`)
- [x] First memory map + I/O notes
- [ ] Recursive-descent follow of all code reachable from KLAD3 `04000`
      (improve `pdp11dis.py` with a code/data worklist to cut linear-sweep noise)
- [ ] Locate sprite tables and level tables statically

## Dynamic 🖥️ (local Ubuntu desktop)
- [ ] Build a BK-0010 emulator with debugger. Options:
      - **UKNCBTL** (`github.com/nzeemin/ukncbtl`) — primarily УКНЦ; the Qt
        build needs Qt5. There is also a BK emulator family.
      - **bkbtl** (`github.com/nzeemin/bkbtl`) — the БК-0010 sibling, the right
        fit for these binaries.
- [ ] Load `KLAD3.BIN` (BIN loader / tape image) and confirm it boots & plays.
- [ ] Set the colour palette and verify the look.
- [ ] Breakpoints + traces, saved as text into `disassembly/traces/`:
      - after boot/init settles
      - on read of `@#177662` (keyboard) → map controls
      - on writes into `040000+` (sprite blit) → find sprite routine & data
      - on the water-death transition → find collision logic
- [ ] Feed trace findings back into `03_boot_init.md` … `09_sound.md`.

## Tooling wishlist
- [ ] `rt11dsk` build (for if/when we get a real `.dsk`, e.g. the 40-in-1 image)
- [ ] Optional: SLEIGH module for PDP-11 to use Ghidra's decompiler
- [ ] Sprite extractor: dump `040000`-format bitmaps to PNG for reference
