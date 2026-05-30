# Plan implementare C23 — КЛАД (УКНЦ 1987 Баранов)

**Regulă:** Citesc ASM, înțeleg, scriu C23. Fără ghicit. Fără întrebări.
**Sursa:** `disassembly/annotated/uknc_klad_1987.asm`
**Target:** Linux native (`cc -std=c23`) + WebAssembly (`emcc`)

---

## Structura fișierelor

```
src/
  Makefile          build: native + wasm
  main.c            entry point: Linux (main) + WASM (emscripten_set_main_loop)
  klad.h            tipuri publice + API între module
  map.c / map.h     tile data, collision map, level load
  player.c/.h       mișcare player, state machine, coliziuni
  enemy.c/.h        AI inamici, throttle, animație
  score.c/.h        scor, vieți, level complete
  render.c/.h       tile blit, sprite draw, HUD
  game.c/.h         game loop principal, init, dispatch
  gfx_data.h        pixel data extras din binar (tool existent)
  level_data.h      hărți nivele extrase din binar (tool existent)
```

---

## Faza 0 — Infrastructură build

- [ ] `src/Makefile` — native (`cc -std=c23 -Wall`) + web (`emcc -std=c23`)
- [ ] `src/main.c` — entry point: `main()` pentru Linux, `emscripten_set_main_loop` pentru WASM
- [ ] `src/klad.h` — tipuri de bază: `TileIdx`, `TileType`, `Vec2i`, `GameState` enum
- [ ] regenerat `src/gfx_data.h` — din `tools/gen_gfx_c.py` (pixel data din binar)
- [ ] regenerat `src/level_data.h` — din `tools/gen_levels_c.py` (hărți + spawn)

## Faza 1 — Date și hartă (MAP)

Sursa ASM: `COLLISION_MAP_BUILD (013524)`, `LEVEL_COMPLETE (001034)`, `LEVEL_RESET (010206)`

- [ ] `src/map.h` — `TileIdx` constants (0-15), `TileType` enum, `Map` struct
- [ ] `map_load(int level)` — din `COLLISION_MAP_BUILD`: unpack nibble pairs → tile array
- [ ] `map_tile_at(Map*, int col, int row)` — boundary-checked access
- [ ] `map_collect(Map*, int col, int row)` — din `PLAYER_STATE_CHECK`: CLRB (R3)
- [ ] `map_is_solid(Map*, int col, int row)` — wall detection
- [ ] `map_is_ladder(Map*, int col, int row)` — ladder detection

## Faza 2 — Player

Sursa ASM: `PLAYER_STATE_CHECK (012570)`, `PLAYER_MOVE_STEP (012740)`,
           `ANIM_THROTTLE_PLAYER (007432)`, `WATER_COLLISION (006462)`,
           `PLAYER_DEATH (001016)`, `DEATH_TRIGGER (004640)`

- [ ] `src/player.h` — `Player` struct, `PlayerResult` enum
- [ ] `player_init(Player*, spawn_col, spawn_row)` — din `GAME_INIT (004000)`
- [ ] `player_move(Player*, Map*, Input, float dt)` — din `PLAYER_MOVE_STEP (012740)`
- [ ] `player_on_ladder(Player*, Map*)` — detectare scară
- [ ] `player_state_check(Player*, Map*)` → `PlayerResult` — din `PLAYER_STATE_CHECK (012570)`
- [ ] `player_anim_tick(Player*, float dt)` — din `ANIM_THROTTLE_PLAYER (007432)`: 4 ticks/frame

## Faza 3 — Enemy AI

Sursa ASM: `ENEMY2_TICK (006552)`, `ENEMY3_TICK (007462)`,
           `SPRITE_ANIM_A/B/C/D (010102, 010142, 007202, 007242)`,
           `ENEMY_RESPAWN (012716)`, `ENTITY1_RESTORE (007306)`,
           `LEVEL_END_CHECK (001636)`

- [ ] `src/enemy.h` — `Enemy` struct
- [ ] `enemy_init(Enemy*, col, row)` — din `ENEMY_RESPAWN (012716)`
- [ ] `enemy_tick(Enemy*, Map*, Player*, float dt)` — throttle 256 ticks → move
- [ ] `enemy_step(Enemy*, Map*, Player*)` — greedy chase: horiz-first, vertical pe scară
- [ ] `enemy_gravity(Enemy*, Map*)` — cade până la WALL/LADDER/WATER
- [ ] `enemy_hits_player(Enemy*, Player*)` — din `LEVEL_END_CHECK (001636)`
- [ ] `enemy_anim_tick(Enemy*, float dt)` — 8-frame horiz, 5-frame vert

## Faza 4 — Scor, vieți, nivel

Sursa ASM: `SCORE_ADD (003764)`, `BONUS_LIFE_ADD (003746)`,
           `LEVEL_COMPLETE (001034)`, `LEVEL_RESET (010206)`,
           `GAME_INIT (004000)`, `HUD_RENDER (003652)`, `NUM_RENDER (004210)`

- [ ] `src/score.h` — `Score` struct (score, lives, level)
- [ ] `score_add(Score*)` — din `SCORE_ADD`: ADD #012 octal = +10 decimal
- [ ] `bonus_life(Score*)` — din `BONUS_LIFE_ADD (003746)`
- [ ] `level_advance(Score*)` — din `LEVEL_COMPLETE (001034)`: stride +352 bytes
- [ ] `game_over_check(Score*)` — lives == 0
- [ ] `all_clear_check(Score*)` — level > 9

## Faza 5 — Rendering

Sursa ASM: `LEVEL_RENDER (004776)`, `SPRITE_DRAW (014030)`,
           `TILE_BLIT_FWD (005106)`, `TILE_BLIT_REV (014302)`,
           `HUD_RENDER (003652)`, `TBL_ANIM_FRAMES (020270)`

- [ ] `src/render.h` — `Renderer` struct (tileset texture, render target)
- [ ] `render_init(Renderer*)` — build tileset din `gfx_data.h`, init render texture
- [ ] `render_map(Renderer*, Map*)` — din `LEVEL_RENDER`: fiecare tile non-zero → blit
- [ ] `render_player(Renderer*, Player*, GameState)` — din `SPRITE_DRAW (014030)`
- [ ] `render_enemy(Renderer*, Enemy*)` — sprite frame din animație
- [ ] `render_hud(Renderer*, Score*)` — din `HUD_RENDER (003652)`: scor + nivel + vieți
- [ ] `render_frame(Renderer*)` — upscale nearest-neighbor la fereastră

## Faza 6 — Game loop

Sursa ASM: `GAME_INIT (004000)`, `GAME_TICK (001602)`, `GAME_LOOP (001344)`,
           `ACT_DISPATCH (001436)`, `KBD_GAME_POLL (004674)`

- [ ] `src/game.h` — `Game` struct (conține Map, Player, Enemy[], Score, Renderer)
- [ ] `game_init(Game*)` — din `GAME_INIT`: LIVES=9, SCORE=0, nivel=0
- [ ] `game_input(Game*)` — din `KBD_GAME_POLL`: mapare taste → Input
- [ ] `game_tick(Game*, float dt)` — din `GAME_TICK (001602)`:
      PLAYER_STATE_CHECK → ENEMY_TICK × 3 → WATER_COLLISION → LEVEL_END_CHECK
- [ ] `game_frame(Game*, float dt)` — tick + render (apelat din main loop)

## Faza 7 — Verificare

- [ ] nivel 1: player spawn corect, enemy spawn corect, gold count corect
- [ ] colectare aur → scor +10
- [ ] moarte apă → lives--
- [ ] coliziune inamic → lives--
- [ ] ieșire nivel → nivel următor
- [ ] game over (lives == 0) → restart
- [ ] all win (nivel 10) → ecran final
- [ ] toate 10 niveluri parcurse
- [ ] build WASM compilează fără erori
- [ ] rulează în browser (Firefox/Chrome)

---

## Status

- Faza 0: ⏳
- Faza 1: ⏳
- Faza 2: ⏳
- Faza 3: ⏳
- Faza 4: ⏳
- Faza 5: ⏳
- Faza 6: ⏳
- Faza 7: ⏳
