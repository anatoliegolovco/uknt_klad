# 02 — I/O registers (BK-0010)

Memory-mapped device registers live at `0177xxx`. These are the ones КЛАД is
expected to touch; entries marked **seen** were found as explicit constants in
the disassembly fingerprint (see `00_overview.md`).

| register | name | purpose | status |
|----------|------|---------|--------|
| `0177660` | keyboard status | bit 7 = key ready; interrupt enable | expected |
| `0177662` | keyboard data | ASCII/KOI code of last key (read clears) | **seen** (KLAD3) |
| `0177664` | scroll register | screen vertical scroll / display start | **seen** (KLAD3) |
| `0177706` | timer count | programmable interval timer | expected |
| `0177710` | timer setting | timer reload value | expected |
| `0177712` | timer control | timer mode/enable | expected |
| `0177714` | parallel port | printer / external I/O | **seen** (KLAD3) |
| `0177716` | system register | RAM/ROM page, tape, INIT; key system control | **seen** (KLAD3) |
| `0177130` | floppy (if fitted) | disk controller | unlikely (tape game) |

## How to read these in the disassembly

The idiom is "load the register address as an immediate, then dereference":

```
012737 000400 001312   MOV #400,@#1312    ; absolute store to a RAM variable
013705 001304          MOV @#1304,R5       ; absolute load
```

A genuine I/O access looks like `MOV something,@#177662` or
`MOVB @#177662,Rn`. Grep the raw disassembly:

```bash
grep -E '@#1776|@#1777|#01776|#01777' disassembly/raw/KLAD3.asm
```

## Sound

The БК-0010 makes sound by toggling a bit in the **system register
`0177716`** (the tape-output / speaker bit) in a timed loop — there is no
dedicated sound chip. If КЛАД has sound, expect tight loops writing alternating
values to `@#177716`. To be confirmed (`09_sound.md`).

## Keyboard

БК keyboard delivers KOI-style codes via `0177662`, gated by `0177660` bit 7.
Arrow keys / movement keys map to specific codes; the game's input routine
(future `05_input.md`) will compare the value read from `@#177662` against a set
of constants. Those constants, once found, give us the original control scheme
to mirror (or remap) in the reimplementation.
