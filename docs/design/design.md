# Design — the reimplementation

Clean-room re-imagining of КЛАД's mechanics. **No original code/art/levels.**

## Target & stack (per your requirement)

- **Language: C23.** Modern C (`-std=c2x`/`c23`), `<stdbool.h>` built in,
  designated initialisers, `constexpr`, `[[attributes]]`, `static_assert`.
- **Primary target: WebAssembly**, so it runs in any browser — desktop **and
  phone**. Built via **Emscripten** (`emcc`), output `index.html` + `.wasm`.
- **Engine: raylib.** A tiny C game library that is genuinely first-class for
  WebAssembly (official `PLATFORM_WEB`/emscripten target), gives us
  input + 2D drawing + audio with almost no boilerplate, and keeps a retro feel.
  raylib also builds natively (desktop) from the *same* source, so we can
  develop/debug fast locally and ship the WASM build.
- **Same source, two builds:** native (`gcc`/`clang` + raylib) for quick
  iteration, web (`emcc` + raylib for web) for the real deliverable. The only
  platform-specific bit is the main loop (see below).

### The one WASM gotcha — the main loop

The browser owns the event loop; you cannot block in a `while(!quit)` loop.
raylib handles this with `emscripten_set_main_loop`. We isolate it:

```c
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(frame, 0, 1);   // browser drives frame()
#else
    while (!WindowShouldClose()) frame();    // native drives it
#endif
```

Everything else (`update`, `render`, input) is identical across targets.

## Phone playability

- Touch: raylib exposes `GetTouchPosition`/`GetGestureDetected`; we draw an
  on-screen D-pad + action button for phones, keyboard for desktop.
- Responsive canvas: the HTML shell scales the canvas to viewport while keeping
  the integer-pixel retro look (nearest-neighbour upscale of a fixed-size
  render target).

## Aesthetic

- Fixed low-res render target (e.g. **256×192** or **256×256**, echoing the БК
  framebuffer), nearest-neighbour upscaled. 8×8 tiles.
- Our own palette — *inspired by* the БК/УКНЦ GRB look but an original choice.
- Original tile art under `assets/new/` (none copied from the ROM).

## Mechanics to reproduce (from study, not code)

The rules are simple and uncopyrightable as *ideas*; we implement them fresh:

- Tile maze; player climbs/walks/falls under gravity.
- Collect all treasure to clear a level.
- Guardians patrol; contact kills.
- **Water = instant death.**
- Destructible/diggable blocks gate progress.
- Per-level speed/timing (cf. the `@#1312` constant we spotted in the original).

Exact numbers (speeds, level layouts) are **not** copied — we author our own and
tune for feel.

## Module sketch (`src/`)

```
src/
  main.c        platform glue + main loop (the #if above)
  game.h/.c     game state machine (title/play/dead/clear), update()
  level.h/.c    tile map model + our own level data (assets/new/levels/)
  render.h/.c   fixed render target, tile + sprite drawing, upscale
  input.h/.c    unified keyboard + touch -> abstract actions
  entity.h/.c   player + guardians, gravity, collision
  audio.h/.c    SFX (raylib audio)
  web/shell.html  custom Emscripten HTML shell (responsive canvas)
```

## Localisation

UI text is localised via `src/i18n.h` — **Romanian is the default language**
(`LANG_RO`), with English as a fallback column. Add a `StrId` key + a row in
`STRINGS[]` to add a string. Strings are currently ASCII (no diacritics)
because raylib's built-in font lacks `ă/î/ș/ț`; once we ship a custom font we
restore the diacritics in one place (`i18n.h`).

## Level data

Per your request we extract levels from the original (see
`docs/reverse/08_level_data.md`). The format isn't fully cracked yet (needs
emulator correlation); `src/level.*` will load our level model, and once the
original encoding is confirmed, a converter will translate extracted data into
it. Note the clean-room caveat there about shipping original layouts publicly.

## Open design decisions (need your call)

1. **Game name.** Working title only right now. Candidates: *Dungeon Climber*,
   *Tomb Runner*, *Klad: Reforged*, or your own.
2. **Render resolution / aspect** for the fixed target.
3. **Level source:** hand-authored levels vs procedural generation (or both).

These are tracked as questions back to you, not assumed.
