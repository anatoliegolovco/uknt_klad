# 01 — Memory map (BK-0010)

All addresses octal. The БК-0010 sees a flat 64 KiB space:

```
000000 - 000777   low core: interrupt/trap vectors, scratch
001000 - 037777   user RAM  <-- the game loads & runs here
040000 - 077777   VIDEO RAM (16 KiB linear framebuffer, 256x256)
100000 - 117777   ROM: MONITOR
120000 - 167777   ROM: BASIC / FOCAL (BK-0010-01)
160000 - 177777   device registers (memory-mapped I/O)
```

## Where КЛАД lives

`KLAD3.BIN` occupies `001000`–`037777` (load `01000`, length `037000`), i.e. it
fills user RAM right up to the video region. Layout, from the disassembly so far:

```
001000            ENTRY: JMP @#4000      ; trampoline to real init
001004 - ~0013xx  dispatch / small routines (level-speed table around 0001302+)
                  e.g. CMP #3,R0 / #4 / #5 ... -> MOV #const,@#1312   (a jump
                  table keyed by a value in R0 writing a "speed/period" word)
004000            real initialisation (entry target)
...               main code, then sprite/level data toward higher addresses
~037777           top of image, just below video RAM at 040000
```

Notable absolute data cells touched early (candidate game-state variables):

- `@#1300`, `@#1302`, `@#1304` — a pointer/counter trio updated together
  (`MOV @#1304,R5` / `ADD #2,@#1304` / `CMP #1302,R5`): looks like a table
  walker (base `01302`, advancing by 2, i.e. a word array).
- `@#1312` — written with `#400`, `#1000`, `#2400` depending on `R0` (3/4/5…):
  a per-level timing/speed constant.
- `@#17430` — written `#10404` near `01116`.

These are *hypotheses* from static reading; confirm by watching them change in
an emulator (dynamic phase).

## Vectors of interest (low core)

| addr | meaning on BK |
|------|---------------|
| `000004` | bus-error / odd-address trap |
| `000060` | keyboard interrupt vector |
| `000070` | keyboard (second) |
| `000100` | timer/line-clock vector |

If the game is interrupt-driven for input or timing, expect it to install
handlers at these vectors during init — a thing to look for at `04000+`.

## The other variants

`KLAD/2/4` load at `0732`, `klad10` at `0720`. Their images begin with a
data/loader preamble (the linear sweep shows repeated `001000` words, which are
data, not `BNE`), so the real entry is *not* the first word — it must be found
dynamically. Their code-vs-data boundaries will differ from KLAD3 and are not
yet mapped.

## Open questions

- Exact code/data split inside KLAD3 (where does code end and the sprite/level
  tables begin?). Needs either dynamic tracing or careful static following of
  all reachable code from `04000`.
- Are sprites stored as raw framebuffer-word bitmaps (likely, given linear
  video) or RLE/compressed?
- Level format and where the level table starts.
