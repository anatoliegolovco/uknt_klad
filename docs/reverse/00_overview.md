# 00 — Overview & binary identification

## What we have

The pdp-11.org.ru game archive (`archive.pdp-11.org.ru/tmp/`) yields **five**
КЛАД variants, each a RAR holding a single BK-style raw `.BIN` memory image.
(`r-games.net`, the task's primary source, does not resolve from our network;
this archive is the working fallback.)

| file | size | header load | header len | leading bytes | classification |
|------|-----:|------------:|-----------:|---------------|----------------|
| `KLAD.BIN`    | 10433 | `0732`  | `024275` (10429) | data preamble | BK-0010, entry TBD |
| `KLAD2.BIN`   | 11086 | `0732`  | `025512` (11082) | data preamble | BK-0010, entry TBD |
| `KLAD3.BIN`   | 15876 | `01000` | `037000` (15872) | `JMP @#4000` | **BK-0010, clean entry** |
| `KLAD4.BIN`   | 10267 | `0732`  | `024027` (10263) | data preamble | BK-0010, entry TBD |
| `klad10.bin`  | 11527 | `0720`  | `026403` (11523) | data preamble | BK-0010, entry TBD |

All five validate as BK raw images: `header_len + 4 == file_size` exactly.
SHA-256 sums are recorded in `tools/fetch_original.sh`.

## Platform: BK-0010, not УКНЦ

The task assumed the УКНЦ "Crocodile Software" build (planar video, GRB palette,
distributed in an RT-11 `.dsk`). **These binaries are the БК-0010 build
instead.** Operand-level fingerprinting (only deliberate `#imm`/`@#abs`
constants, which filters data noise) shows:

- `KLAD3.BIN`: `MOV #040000` (BK screen base) ×5, plus `#0177700`, `#0177716`
  (BK system register), `#0177662` (BK keyboard data), `#0177664` (BK scroll),
  `#0177714`. 15 distinct BK-register hits.
- All variants pour constants into the `040000`–`077777` range — the БК linear
  framebuffer. УКНЦ does not have a CPU-visible framebuffer there (its video is
  planar, behind the peripheral processor).

**Implication:** for understanding *mechanics* — the only thing the clean-room
reimplementation needs — the BK-0010 build is equivalent to and simpler than
the УКНЦ build. We proceed with it. If the specific УКНЦ/GRB build is wanted
later, it most plausibly lives in the 800 KiB "40-in-1" RT-11 floppy image,
which we have not yet found on a reachable mirror.

The five variants are likely different versions / level sets / authorship lines
(there are both a 1987 Баранов КЛАД and the Crocodile port). Distinguishing them
precisely needs dynamic analysis in an emulator (next section).

## Reference target

Until told otherwise we treat **`KLAD3.BIN`** as the primary subject: it is the
largest (most complete), loads at the canonical БК `01000`, and has a clean
`JMP @#4000` entry trampoline that disassembles into coherent init code. The
others begin with a data/loader preamble whose true entry point we will confirm
dynamically.

## Methodology & tooling

We could not use radare2 here (the Ubuntu package ships no PDP-11 plugin), so we
wrote **`tools/pdp11dis.py`** — a from-scratch, validated PDP-11 disassembler
(octal output, full addressing-mode + PC-relative target resolution). It is the
canonical tool for this repo; `disassembly/raw/*.asm` is its linear-sweep
output. Linear sweep necessarily mis-decodes data regions as code; the
`disassembly/annotated/` tree will hold hand-corrected, symbolised routines as
we work through them.

### The intended pipeline

1. **Recon (done):** identify container, load address, platform — this file.
2. **Static sweep (done):** `tools/disasm.sh` → `disassembly/raw/`.
3. **Dynamic analysis (needs a GUI host):** run in **UKNCBTL** / a BK emulator
   with a debugger; breakpoint boot, keyboard read, sprite blit, water-death;
   capture traces. **This step cannot run in the headless cloud container** —
   it is to be done on the local Ubuntu desktop. See `docs/reverse/TODO.md`.
4. **Annotate:** promote understood routines into `disassembly/annotated/` and
   write up `03_boot_init.md` … `09_sound.md`.
5. **Reimplement clean-room:** `src/` (C23 → WebAssembly).

## Status of the task's checklist

- [x] Find a working source for the blob (archive.pdp-11.org.ru)
- [x] Download + integrity (SHA-256 recorded) into `assets/original/` (tracked)
- [x] Identify format (BK raw `.BIN`, not `.dsk`) and platform (BK-0010)
- [x] Toolchain: disassembler (custom), extractor scripts, unrar/7z
- [ ] Confirm the game runs in an emulator — **GUI, do locally** (see TODO.md)
- [x] Extract individual files from the archives
- [x] Begin disassembly of the main binary
- [x] First documentation iteration (this set)
- [x] Git repo structure
