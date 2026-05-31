# ASM → C mapping — КЛАД 1987 (Баранов), УКНЦ

Tracks every routine in `disassembly/annotated/uknc_klad_1987.asm` (66 `FN_*`) and
whether it has been **read+understood** and **implemented in C** (`src/`).

## Status legend
- ✅ **C** — ported to C (cite file:function)
- 🟡 **PLATFORM** — semantics covered by raylib/host equivalent (render/input/timing),
  not a 1:1 byte port (modern platform replaces УКНЦ hardware)
- ⬜ **TODO** — gameplay-relevant, not yet implemented
- ➖ **N/A** — УКНЦ hardware I/O / housekeeping, not needed on a modern target

Read = the routine has been read and understood (commented in the .asm). Nearly all
are Read ✅; the C column is what matters.

---

## Game logic / flow

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| RESTART | 001000 | ✅ | ✅ C | main.c (cold start → game_init) |
| GAME_OVER_SOFT | 001004 | ✅ | ✅ C | game.c (GS_GAME_OVER restart) |
| PLAYER_DEATH | 001016 | ✅ | ✅ C | game.c (PR_WATER/PR_ENEMY → lose life) |
| BONUS_LIFE | 001024 | ✅ | ✅ C | score.c score_add_life |
| LEVEL_COMPLETE | 001034 | ✅ | ✅ C | score.c score_level_advance + game.c |
| KEY_DIFFICULTY | 001142 | ✅ | ⬜ TODO | speed select 1-4 (sets VAR_SPEED) — no speed system yet |
| DELAY_SPIN | 001306 | ✅ | 🟡 PLATFORM | raylib SetTargetFPS(60) / frame timing |
| GAME_LOOP | 001344 | ✅ | ✅ C | main.c main loop / game_frame |
| GAME_LOOP_MENU | 001354 | ✅ | ⬜ TODO | menu keyboard poll (no menu yet) |
| ACT_DISPATCH | 001436 | ✅ | ✅ C | game.c read_input + game_tick dispatch |
| GAME_TICK | 001602 | ✅ | ✅ C | game.c game_tick (player→enemies→collide) |
| LEVEL_END_CHECK | 001636 | ✅ | ✅ C | enemy.c enemy_hits_player / game.c |

## Title / menu / shell

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| TITLE_SEQ | 002072 | ✅ | ⬜ TODO | title screen (КЛАД / Николаев 1987 / Баранов) |
| TITLE_WAIT | 002264 | ✅ | ⬜ TODO | wait Enter on title |
| DIFF_SELECT | 003234 | ✅ | ⬜ TODO | difficulty/speed select screen |
| GAME_OVER_WAIT | 003274 | ✅ | 🟡 PLATFORM | game.c GS_GAME_OVER (waits action) |
| LIVES_DISPLAY | 003372 | ✅ | ✅ C | render.c render_hud (lives) |
| GAME_LEVEL_LOOP | 003576 | ✅ | ✅ C | game.c game_frame render section |
| HUD_RENDER | 003652 | ✅ | ✅ C | render.c render_hud |
| BONUS_LIFE_ADD | 003746 | ✅ | ✅ C | score.c score_add_life |
| SCORE_ADD | 003764 | ✅ | ✅ C | score.c score_add_gold (+10) |

## Init / numbers

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| GAME_INIT | 004000 | ✅ | ✅ C | game.c game_init + score_init (lives, score) |
| NUM_RENDER | 004210 | ✅ | 🟡 PLATFORM | render.c render_hud via snprintf/DrawText |
| DEATH_TRIGGER | 004640 | ✅ | ✅ C | game.c death handling |
| KBD_GAME_POLL | 004674 | ✅ | 🟡 PLATFORM | game.c read_input (raylib keys) |

## Level render / tile blit  ⚠ DECODE UNDER REVIEW

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| LEVEL_RENDER | 004776 | ✅ | ✅ C | render.c render_map |
| LEVEL_RENDER_R4 | 005002 | ✅ | ✅ C | render.c render_map (per-tile loop) |
| TILE_BLIT_FWD | 005106 | ✅ | 🟡 PLATFORM | render.c draw_slot (tileset texture) |
| HW_INIT | 005754 | ✅ | 🟡 PLATFORM | main.c InitWindow |
| PLAYER_SPRITE_INIT | 006054 | ✅ | ✅ C | render.c render_player |
| LEVEL_RENDER_FULL | 012442 | ✅ | ✅ C | render.c render_map (same target) |
| TILE_BLIT_SUB | 012530 | ✅ | 🟡 PLATFORM | render.c draw_slot |
| COLLISION_MAP_BUILD | 013524 | ✅ | ✅ C(part) | map.c map_load (unpack); **flags #1000/#20000… only partially used** |
| SPRITE_DRAW | 014030 | ✅ | ✅ C | render.c render_player/render_enemy |
| TILE_BLIT_REV | 014302 | ✅ | 🟡 PLATFORM | render.c draw_slot |

## Player

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| WATER_COLLISION | 006462 | ✅ | ✅ C | player.c player_update → PR_WATER |
| ANIM_THROTTLE_PLAYER | 007432 | ✅ | ✅ C | player.c anim throttle |
| PLAYER_STATE_CHECK | 012570 | ✅ | ✅ C | player.c player_update (gold/bonus/exit/death) |
| PLAYER_MOVE_STEP | 012740 | ✅ | ✅ C(adapted) | player.c player_update (continuous, not tile-step) |

## Enemies

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| ENTITY_HANDLER | 006204 | ✅ | ⬜ TODO | **SOUND engine** (toggle @REG_SYSREG bit7) — no audio yet |
| ENTITY_STATE_INIT | 006444 | ✅ | ✅ C | enemy.c enemy_init (reset counters) |
| ENEMY2_TICK | 006552 | ✅ | ✅ C | enemy.c enemy_tick |
| ENEMY2_MOVE_UP | 007136 | ✅ | ✅ C(part) | enemy.c do_move (vertical) |
| SPRITE_ANIM_C | 007202 | ✅ | ✅ C | enemy.c anim (8-frame horiz) |
| SPRITE_ANIM_D | 007242 | ✅ | ✅ C | enemy.c anim (5-frame vert) |
| ENTITY1_RESTORE | 007306 | ✅ | 🟡 PLATFORM | erase/restore handled by full redraw |
| ENEMY1_TICK | 007376 | ✅ | ✅ C | enemy.c enemy_tick (enemy 0) |
| ENEMY3_TICK | 007462 | ✅ | ✅ C | enemy.c enemy_tick (enemy 1) |
| ENEMY3_MOVE_UP | 010036 | ✅ | ✅ C(part) | enemy.c do_move |
| SPRITE_ANIM_A | 010102 | ✅ | ✅ C | enemy.c anim |
| SPRITE_ANIM_B | 010142 | ✅ | ✅ C | enemy.c anim |
| LEVEL_RESET | 010206 | ✅ | ✅ C | game.c load_level (on death) |
| ENEMY_RESPAWN | 012716 | ✅ | ✅ C | enemy.c enemy_init |
| SPRITE_HELPERS | 013216 | ✅ | ✅ C(part) | render.c char-tile selection (states 0o21-24) |

## Sound  ⬜ entire subsystem TODO

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| ENTITY_HANDLER (sound engine) | 006204 | ✅ | ⬜ TODO | speaker toggle; TBL_SOUND_A/B note tables |
| SOUND_WRAPPER_A/B | 002050/2060 | ✅ | ⬜ TODO | note sequence triggers |

## УКНЦ video / keyboard I/O  ➖ replaced by raylib

| ASM routine | addr | Read | C status | Where / note |
|-------------|------|------|----------|--------------|
| VSYNC_WAIT | 012326 | ✅ | ➖ N/A | raylib vsync |
| DISP_SCANLINE_WRITE | 040060 | ✅ | ➖ N/A | port 176642/3 writes — **but defines the tile pixel format (decode)** |
| KBD_READ | 040660 | ✅ | 🟡 PLATFORM | raylib IsKeyDown |
| KBD_POLL | 041020 | ✅ | 🟡 PLATFORM | raylib |
| DISP_COL_BLIT | 041040 | ✅ | ➖ N/A | address arithmetic for the port |
| EMT_TEXT | 041100 | ✅ | 🟡 PLATFORM | DrawText |
| DISP_SETUP | 041140 | ✅ | ➖ N/A | display init |
| DISP_MODE_CHECK | 041200 | ✅ | ➖ N/A | colour-mode check |
| VSYNC_LOOP | 041400 | ✅ | ➖ N/A | software vsync |
| DISP_PIXEL_PORT | 041460 | ✅ | ➖ N/A | port 176642 |
| DISP_LINE_ADV | 041500 | ✅ | ➖ N/A | line advance |
| DISP_ROW_WRITE | 041600 | ✅ | ➖ N/A | row register |
| DISP_SYNC_BIT | 041620 | ✅ | ➖ N/A | sync toggle |
| DISP_WRITE_COL | 041740 | ✅ | ➖ N/A | column write entry |

---

## Summary

| Category | Count | Done |
|----------|-------|------|
| Gameplay logic (player/enemy/score/map/collide) | ~28 | ✅ mostly C |
| Render (level/sprite/HUD) | ~10 | ✅ C via raylib (tile **decode under review**) |
| Title / menu / speed-select | 5 | ⬜ TODO |
| Sound | ~3 | ⬜ TODO (whole subsystem) |
| УКНЦ hardware I/O | ~13 | ➖ N/A (raylib) |

**Implemented & playable:** movement, gravity, ladder collision, gold/score, lives,
water death, enemy AI + collision, level load, level transition, render.

**Not yet (gameplay):** title screen, difficulty/speed select (1-4), **sound**.

**Open correctness items** (see FIDELITY_KPI.md): tile pixel **decode** (gold/ladder
shape — 16×8 vs 8×8 under investigation), collision-flag fidelity in
COLLISION_MAP_BUILD (only the WALL/LADDER subset is used; the full #1000/#400/#20000/
#10000 directional flag set is approximated), exact sprite frames, lives value (219 vs 5).
