# Platform architecture: PDP-11 family (УКНЦ, БК-0010)

A short primer on the hardware these КЛАД binaries target, written for someone
with a solid technical background but no prior PDP-11 experience.

## The CPU: KM1801VM2 — a PDP-11 clone

Both the УКНЦ and the БК use Soviet **К1801ВМ** processors that are
instruction-set compatible with DEC's **PDP-11**. So the machine code in these
binaries *is* PDP-11 code. Key traits, because they will surprise you:

- **Everything is octal.** Addresses, opcodes, and constants are conventionally
  written base-8. `01000` = 512, `0177700` = 65472. Our disassembler prints
  octal throughout to match the original toolchain (MACRO-11).
- **16-bit words, little-endian.** 64 KiB address space (`0`..`0177777`).
- **8 general registers** `R0`..`R7`. `R6` = **SP** (stack pointer),
  `R7` = **PC** (program counter). The PC being a normal register is what makes
  PC-relative and immediate addressing fall out of the same 8 addressing modes.
- **8 addressing modes** per operand (mode 0..7), encoded as a 6-bit field
  `mmm rrr`. With `rrr = 7` (PC) the modes become: `#imm` (2), `@#abs` (3),
  `relative` (6), `@relative` (7). This is why constants and absolute addresses
  appear as a *second word* following the instruction.
- **Memory-mapped I/O.** The top of the address space (`0160000`..`0177777`) is
  device registers, not RAM. Writing there pokes hardware. See
  [`reverse/02_io_registers.md`](reverse/02_io_registers.md).

If you read one external reference, read the MACRO-11 Language Reference (linked
in the task notes) — the addressing-mode table is the crux.

## Two related machines — and which one КЛАД here targets

| | БК-0010(-01) | УКНЦ / МС-0511 |
|---|---|---|
| CPU | one К1801ВМ1 | **two** К1801ВМ2 (CPU + PPU) |
| RAM | 32 KiB user | 64 KiB CPU + 32 KiB PPU + video |
| Video | **linear framebuffer at `040000`–`077777`** (256×256) | **planar**, driven by the peripheral processor (PPU) |
| Screen control | scroll register `0177664` | PPU channel regs (`0177010`/`0177026`…) |
| Keyboard | `0177660` status / `0177662` data | via the PPU |

**Finding for this repo:** the binaries in the pdp-11.org.ru archive are the
**БК-0010** build of КЛАД, not the УКНЦ one. Evidence (see
[`reverse/00_overview.md`](reverse/00_overview.md)): they load at low addresses
(`01000` etc.) and write constants like `#040000` (BK screen base), `#0177716`
(BK system register), `#0177662`/`#0177664` (BK keyboard/scroll) — a signature
that does **not** match УКНЦ planar video.

Why this is *good* for us: the БК's video is a plain linear 1-bit (or 4-colour)
framebuffer, which is far easier to read sprite and level data out of than the
УКНЦ's planar memory driven through a second CPU. The **gameplay is identical**
between the two ports, and gameplay is all the clean-room reimplementation
needs. If we later want the specific УКНЦ "GRB palette" build, that most likely
lives in the 800 KiB "40-in-1" RT-11 floppy image, which we have not located on
a reachable mirror yet.

## БК-0010 video, in one paragraph

The screen is `040000`–`077777` (16 KiB), 256×256 pixels. In black-and-white
mode each bit is one pixel; in colour mode each pixel is 2 bits → 4 colours from
a fixed palette, so a byte holds 4 pixels and a 16-bit word holds 8. A row is 64
bytes (`0100` octal). Sprite blitting is therefore "OR/MOV words into the
framebuffer at `040000 + row*0100 + col`". This linear model is exactly what we
emulate conceptually in the C23 reimplementation's tile renderer.

## The `.BIN` container

These are **not** RT-11 `.dsk` floppy images. Each file is a raw BK memory
image with a tiny 4-byte header:

```
offset 0: load address (1 word, little-endian, octal)
offset 2: length in bytes (1 word)
offset 4: <length> bytes of payload, copied verbatim to the load address
```

Execution starts at the load address. (`tools/pdp11dis.py` parses this header
by default; pass `--raw --org` to disassemble headerless dumps.)
