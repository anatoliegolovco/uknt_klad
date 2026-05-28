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

## D7 — Track the original КЛАД material in this (personal, non-commercial) repo
**Context.** This was briefly thought to be a commercial project (a typo: "u
planific" read as commercial; the user meant "**nu** planific sa fac bani" — *not*
making money). It is a **personal, non-commercial hobby** preservation/study
project of ~35-year-old Soviet abandonware, not intended for wide distribution.
The opening research itself notes "disassembly pentru studiu personal = uz
acceptabil în practică."
**Earlier (commercial) stance, now superseded.** While the goal looked
commercial, we declined to commit the copyrighted original levels — selling a
product embedding third-party level designs is infringement and exposes the
author to takedowns/claims. That reasoning was correct *for a commercial
product*.
**Decision (current context).** For personal, non-commercial study/preservation,
**track the original material** (binaries, extracted data, disassembly) in the
repo, as the user requested. This is the accepted-practice case for personal RE
of abandonware.
**Consequences.** The repo now contains the original `.BIN` images, extracted
data blocks, and raw disassembly. `.gitignore` only excludes build/tooling
artifacts.
**Boundary that still holds.** This rests on the project staying
**non-commercial and personal**. If it ever turns into a product sold or widely
distributed, the copyright analysis in `legal.md` reapplies: ship only original/
licensed assets. Note copyright still legally exists (Russia has copyright law;
*Rise Out* is Japanese/Kadokawa) — the "Soviet/no private property" point is
rhetorical, not a legal basis; what makes this fine is the personal,
non-commercial nature, not the works' origin.
