# Decision log

ADR-style. Newest decisions at the bottom. Each entry: context → decision →
consequences.

---

## D1 — Use a from-scratch PDP-11 disassembler instead of radare2/Ghidra
**Context.** The task suggested radare2/rizin/Ghidra. The Ubuntu radare2 package
(5.5.0) ships **no PDP-11 plugin** (its arches are generic 4/8/16/32-bit only),
and building rizin/a SLEIGH module in an ephemeral container is heavy.
**Decision.** Write `tools/pdp11dis.py`: a self-contained PDP-11 decoder (octal,
all 8 addressing modes, PC-relative/immediate/absolute resolution, branches,
SOB/JSR/RTS). Validated against known canonical encodings.
**Consequences.** No external dependency; full control over annotation and
target resolution; reusable and educational. Linear-sweep only (mis-decodes
data as code) — acceptable; recursive-descent is a future improvement.

## D2 — Source the blob from archive.pdp-11.org.ru, not r-games.net
**Context.** `r-games.net` (task's primary source) does not resolve from the
build environment; `archive.pdp-11.org.ru` does.
**Decision.** Fetch the five `klad*.rar` from the pdp-11.org.ru archive; record
SHA-256 sums in `tools/fetch_original.sh`.
**Consequences.** Reproducible download; integrity verifiable.

## D3 — Treat the studied build as BK-0010, not УКНЦ
**Context.** The task assumed the УКНЦ "Crocodile" build (planar video, GRB
palette, RT-11 `.dsk`). The archived binaries are raw BK `.BIN` images that
write `#040000` (BK linear framebuffer), `#0177716`, `#0177662`, `#0177664`
(BK system/keyboard/scroll) — a BK-0010 signature.
**Decision.** Proceed on the BK-0010 build (it is what exists, and is simpler:
linear framebuffer vs планар). Gameplay is identical across ports.
**Consequences.** Mechanics study is unaffected. If the specific УКНЦ/GRB build
is wanted, it likely lives in the unfound 800 KiB "40-in-1" RT-11 image.

## D4 — Reimplementation stack: C23 + raylib → WebAssembly
**Context.** User requirement: playable in browser and on phone.
**Decision.** C23 (`-std=c2x`) with raylib, one source building native (dev) and
web (`emcc`, ASYNCIFY, `emscripten_set_main_loop`). Touch + keyboard input.
**Consequences.** Single codebase, first-class web target, retro look via a
fixed 256×192 render target upscaled nearest-neighbour.

## D5 — Romanian as the default UI language
**Context.** User request: UI strings in Romanian.
**Decision.** `src/i18n.h` with `LANG_RO` default and `LANG_EN` fallback; lookup
via `T(StrId)`. ASCII (no diacritics) until a custom font ships.
**Consequences.** Localisation centralised; diacritics restored in one place
later.

## D6 — Keep the original blob AND its full disassembly local-only
**Context.** A complete disassembly is a mechanical transform (derivative) of a
copyrighted binary; committing it also breaks clean-room hygiene (implementers
must not read it).
**Decision.** `.gitignore` excludes `assets/original/`, `disassembly/raw/`, and
extracted data. Commit only original code, tools, and our own prose notes.
**Consequences.** Public repo stays clean; everything regenerates locally from
the blob via `tools/`.

## D7 — Do NOT ship the original level/asset data; author original levels
**Context.** The user stated an intent to commercialise and asked to commit the
extracted original levels to git. The project's own opening research states that
*Rise Out From Dungeons* (ASCII/Kadokawa) is under copyright until ~2053 and
КЛАД (Crocodile/Baranov) is likewise protected, and that only original code,
art, **levels**, and name are "legally safe."
**Decision.** **Decline** to commit the original copyrighted level data. The
extracted blocks remain a **local, git-ignored study reference** (see
`legal.md`). The shipped game uses **original levels** ("inspired by"), or
licensed content if a licence is obtained.
**Consequences.** Keeps the project shippable/sellable without distributing
third-party copyrighted assets and without exposing the author to infringement
claims/takedowns. Authoring original levels is now a roadmap item. This decision
stands regardless of build target and is independent of the engine choice.
**Reversal condition.** A written licence from the rights holder(s), or the
works entering the public domain.
