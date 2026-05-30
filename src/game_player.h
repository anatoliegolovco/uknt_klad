// game_player.h — Logica jucătorului din КЛАД 1987
// Fiecare funcție are referință exactă la rutina ASM sursă.
#pragma once
#include "uknc_defs.h"
#include "game_entity.h"
#include "game_map.h"
#include <stdbool.h>

// =============================================================================
// ANIM_THROTTLE_PLAYER (007432)
// ASM:
//   CMP #3, @#17374    → dacă counter >= 3: apelează SPRITE_DRAW, resetează la 0
//   BGE 7454           → altfel: incrementează counter
//   INC @#17374
// Concluzie: sprite-ul playerului se actualizează la fiecare 4 ticks de joc.
// =============================================================================
#define PLAYER_ANIM_TICKS  4   // ticks până la schimbarea frame-ului de animație

// Frame-uri sprite player din SPRITE_GFX (estimare din ANIMATIONS.md):
//   walk: frames 0-7 (0-3 = dreapta, 4-7 = stânga sau alternativă)
//   climb: frame 8 sau 9
//   death: frame 12-15
// Slot-urile din tileset (definite în game.c la TILE_SLOT_PLW etc.):
#define PLAYER_WALK_FRAMES  4   // 4 frame-uri walk cycle
#define PLAYER_CLIMB_FRAME  8   // SPRITE_GFX[8] = climbing
#define PLAYER_DEATH_FRAME  12  // SPRITE_GFX[12] = death

// =============================================================================
// PLAYER_STATE_CHECK (012570)
// ASM (secvența completă):
//   MOV @#14422, R3           → R3 = ptr tile player în BUF_TILE_WORK
//   CMPB #011, (R3)           → tile_state == 9? (entity active = enemy on same tile)
//   BNE skip                  → nu → salt
//   MOVB #017, (R3)           → setează tile_state = 15 (dead)
//   JMP @#4640                → trigger DEATH_TRIGGER
//   BIC #4000, 177700(R3)     → clear collision flag în display
//   JSR PC, @#12326           → VSYNC_WAIT
//   CMPB #4, (R3)             → tile_state == 4? (gold A)
//   CLRB (R3)                 → șterge tile
//   JSR PC, @#3764            → SCORE_ADD (+10)
//   CMPB #5, (R3)             → tile_state == 5? (gold B = bonus life)
//   CLRB (R3)
//   JSR PC, @#3746            → BONUS_LIFE_ADD
//   CMPB #6, (R3)             → tile_state == 6? (gold C = LEVEL_COMPLETE)
//   MOVB #020, (R3)
//   MOV @#17424, R5; JSR @#12716 → ENEMY_RESPAWN(enemy1)
//   MOV @#17426, R5; JSR @#12716 → ENEMY_RESPAWN(enemy2)
//   JSR PC, @#2060            → SOUND_WRAPPER_A
//   RTS PC
// =============================================================================

typedef enum {
    PSR_NONE,           // nicio interacțiune
    PSR_COLLECT_GOLD,   // tile_state == 4: +10 scor, tile → EMPTY
    PSR_BONUS_LIFE,     // tile_state == 5: +1 viață, tile → EMPTY
    PSR_LEVEL_WIN,      // tile_state == 6: nivel complet
    PSR_ENEMY_HIT,      // tile_state == 9 (TSTATE_ENTITY_ACTIVE): moarte
    PSR_WATER_DEATH,    // player pe tile apă (T_WATER): moarte
    PSR_EXIT_REACHED,   // tile_state == 2 (T_EXIT): ieșire
} PlayerStateResult;

// Verifică starea tile-ului curent al playerului și returnează acțiunea necesară.
// Input: col, row = poziția tile curentă a playerului; map_tile = harta logică
static inline PlayerStateResult player_state_check(int col, int row) {
    TileType t = map_at(col, row);
    TileIdx  idx = (col >= 0 && col < MAP_COLS && row >= 0 && row < MAP_ROWS)
                   ? raw_tile[row][col] : TIDX_AIR;

    // CMPB #4, (R3) → tile_state == 4: gold A → SCORE_ADD
    if (idx == TIDX_GOLD_A)   return PSR_COLLECT_GOLD;
    // CMPB #5, (R3) → tile_state == 5: gold B → BONUS_LIFE_ADD
    if (idx == TIDX_GOLD_B)   return PSR_BONUS_LIFE;
    // CMPB #6, (R3) → tile_state == 6: gold C → LEVEL_COMPLETE
    if (idx == TIDX_GOLD_C)   return PSR_LEVEL_WIN;
    // tile T_EXIT → nivel complet (tile 2 în hartă)
    if (t == T_EXIT)           return PSR_EXIT_REACHED;
    // tile T_WATER → moarte (WATER_COLLISION / DEATH_TRIGGER)
    if (t == T_WATER)          return PSR_WATER_DEATH;

    return PSR_NONE;
}

// =============================================================================
// PLAYER MOVEMENT — din PLAYER_MOVE_STEP (012740)
// ASM descrie mișcarea bazată pe tile grid cu sprite blitting.
// Reimplementare: mișcare continuă pixel-level (mai fluidă pe display modern).
//
// Reguli derivate din ASM:
//   1. Gravitație: playerul cade dacă nu e pe scară și nu există WALL sub el
//   2. Scară: playerul se poate mișca vertical NUMAI dacă tile-ul curent e LADDER
//   3. Coliziune orizontală: nu poate intra în WALL
//   4. Coliziune verticală jos: se oprește pe WALL
// =============================================================================

#define PLAYER_SPEED_PX   64.0f   // pixeli/secundă (viteza orizontală)
#define PLAYER_GRAV_PX   320.0f   // pixeli/secundă² (gravitație)

// Detectează dacă playerul e pe scară (col central, oricare dintre rândul de sus/jos)
static inline bool player_on_ladder(float px, float py) {
    int cx  = (int)((px + TILE_W * 0.5f) / TILE_W);
    int ry1 = (int)(py / TILE_H);
    int ry2 = (int)((py + TILE_H - 1) / TILE_H);
    return map_at(cx, ry1) == T_LADDER || map_at(cx, ry2) == T_LADDER;
}

// Tile central al playerului (pentru detecția interacțiunilor)
static inline void player_center_tile(float px, float py, int *col, int *row) {
    *col = (int)((px + TILE_W * 0.5f) / TILE_W);
    *row = (int)((py + TILE_H * 0.5f) / TILE_H);
}
