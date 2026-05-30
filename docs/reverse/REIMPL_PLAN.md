# Plan reimplementare КЛАД 1987 Баранов — C23 + raylib

**Principiu:** fiecare funcție C se scrie DUPĂ ce citim ASM-ul corespunzător.
Nicio valoare nu e inventată. Sursa = `disassembly/annotated/uknc_klad_1987.asm`.

**Strat grafic** — deja rezolvat pixel-perfect din binar (`gfx_data.h`).
**Strat logic** — se construiește acum, funcție cu funcție.

---

## Faza 0 — Fundație (headers + date)

- [x] `src/gfx_data.h` — TILE_GFX[16][8][8] + SPRITE_GFX[208][8][8] din binar
- [x] `src/levels_data.h` — LEVEL_TILES[10][22][32] + LEVEL_SPAWNS[10][3][2]
- [x] `src/uknc_defs.h` — constante din symbol table (tile idx, states, layout)
- [x] `src/game_entity.h` — Entity, Player, GameVars structs din entity record
- [x] `src/game_map.h` — TileType, map_load(), map_at(), map_collect_gold()

---

## Faza 1 — Logica jucătorului

Sursa ASM: PLAYER_STATE_CHECK (012570), PLAYER_MOVE_STEP (012740),
           ANIM_THROTTLE_PLAYER (007432), WATER_COLLISION (006462)

- [x] `src/game_player.h` — interfața publică
- [x] player_state_check() — din PLAYER_STATE_CHECK (012570):
      detectează aur(4), bonus(5), ieșire(6), moarte(9/15)
- [x] player_move() — din PLAYER_MOVE_STEP (012740)
- [x] player_on_ladder() — din detecția scară
- [x] water_collision() — din WATER_COLLISION (006462)
- [x] player_anim_tick() — din ANIM_THROTTLE_PLAYER (007432)

---

## Faza 2 — Logica inamicilor

Sursa ASM: ENEMY1/2/3_TICK (007376, 006552, 007462),
           SPRITE_ANIM_A/B/C/D (010102, 010142, 007202, 007242),
           ENEMY_RESPAWN (012716), ENTITY1_RESTORE (007306)

- [x] `src/game_enemy.h` — interfața publică
- [x] enemy_tick() — din ENEMY_TICK (greedy chase)
- [x] enemy_gravity() — cade până la WALL/LADDER/WATER
- [x] enemy_anim_tick() — 8-frame horiz + 5-frame vert
- [x] enemy_respawn() — din ENEMY_RESPAWN (012716)
- [x] enemy_collision() — din LEVEL_END_CHECK (001636)

---

## Faza 3 — Scor, vieți, nivel

Sursa ASM: SCORE_ADD (003764), BONUS_LIFE_ADD (003746),
           LEVEL_COMPLETE (001034), LEVEL_RESET (010206),
           GAME_INIT (004000), HUD_RENDER (003652)

- [ ] `src/game_score.h` — interfața publică
- [ ] score_add() — din SCORE_ADD: ADD #12 (octal) = +10 decimal la VAR_SCORE
- [ ] bonus_life_add() — din BONUS_LIFE_ADD: VAR_LIVES++
- [ ] level_complete() — din LEVEL_COMPLETE (001034): advance pointer, check wrap
- [ ] level_reset() — din LEVEL_RESET (010206): moarte → reload nivel, lives--
- [ ] game_init() — din GAME_INIT (004000): LIVES=0333, SCORE=0, nivel=0

---

## Faza 4 — Rendering

Sursa ASM: LEVEL_RENDER (004776), SPRITE_DRAW (014030),
           TILE_BLIT_FWD (005106), TILE_BLIT_REV (014302),
           TBL_ANIM_FRAMES (020270)

- [ ] `src/game_render.h` — interfața publică
- [ ] render_map() — din LEVEL_RENDER: pentru fiecare tile non-zero din raw_tile,
      desenează TILE_GFX[raw_tile[r][c]] la poziția (c*8, r*8)
- [ ] render_sprite() — din SPRITE_DRAW: 2 frame-uri consecutive = 16×8 sprite
- [ ] render_hud() — din HUD_RENDER: scor + vieți + nivel
- [ ] Sprite frame mapping — din TBL_ANIM_FRAMES + SPRITE_HELPERS (013216):
      player walk=frames 0-7, climb=8-11, death=12-15, enemy=16+

---

## Faza 5 — Game loop principal

Sursa ASM: GAME_TICK (001602), GAME_LOOP (001344),
           ACT_DISPATCH (001436), GAME_LEVEL_LOOP (003576)

- [ ] `src/game_loop.h` — interfața publică
- [ ] game_tick() — din GAME_TICK (001602):
      DELAY_SPIN → PLAYER_STATE_CHECK → ENEMY1_TICK → ENEMY2_TICK →
      ENEMY3_TICK → ENTITY1_RESTORE → WATER_COLLISION → LEVEL_END_CHECK
- [ ] input_read() — din KBD_GAME_POLL (004674): mapare taste → acțiuni
- [ ] Rewire game.c să folosească toate modulele noi

---

## Faza 6 — Verificare

- [ ] Testează toate 10 niveluri: player spawn, enemy spawn, gold count
- [ ] Testează colectare aur → scor +10
- [ ] Testează moarte apă → lives--
- [ ] Testează coliziune inamic → lives--
- [ ] Testează completare nivel (tile TIDX_EXIT) → nivel următor
- [ ] Testează game over (lives == 0)
- [ ] Testează all win (nivel 10 completat)
- [ ] Web build (emscripten) verificat

---

## Status curent

Faza 0: ✅ completă
Faza 1: ✅ completă
Faza 2: ✅ completă
Fazele 3-6: ⏳ planificate (implementate parțial în game.c)
