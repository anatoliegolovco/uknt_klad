// game_entity.h — Structura entității (player/enemy) din КЛАД 1987
// Derivată din entity record layout (ACT_DISPATCH 001436, SPRITE_DRAW 014030)
// Player @ 014420, Enemy1 @ 014430, Enemy2 @ 014440 în address space original
#pragma once
#include "uknc_defs.h"
#include <stdbool.h>

// =============================================================================
// ENTITY (player sau inamic)
// Câmpurile sunt mapate direct pe cuvintele din entity record ASM:
//   word+0 = state, word+2 = tile_ptr, word+4 = col, word+6 = row,
//   word+8 = anim_frame, word+10 = dir
// =============================================================================

typedef struct {
    bool    active;       // EREC_STATE: 010=activ, 0=inactiv
    int     col;          // EREC_X: coloana tile (0–MAP_COLS-1)
    int     row;          // EREC_Y: rândul tile (0–MAP_ROWS-1)
    int     anim_horiz;   // VAR_ENEMYn_ANIM_A: contor frame orizontal (0–7)
    int     anim_vert;    // VAR_ENEMYn_ANIM_B: contor frame vertical (0–4)
    int     tick_ctr;     // VAR_ENEMYn_TICK_CTR: throttle ticks
    int     dir;          // EREC_DIR: direcție curentă (-1=stânga, 1=dreapta)
} Entity;

// =============================================================================
// PLAYER — entitate specială cu mișcare continuă (pixel-level în reimplementare)
// În original: tile-grid based cu sprite workspace BUF_SPRITE_PLAYER (021640)
// =============================================================================

typedef struct {
    Entity  ent;          // date de bază (active, col, row, anim_*)
    float   px, py;       // poziție pixel (continuă, pentru mișcare fluidă)
    float   vy;           // viteză verticală (gravitație)
    bool    on_ladder;    // detectat din tile_at(col,row) == TIDX_LADDER/LADDER2
    bool    dead;         // stare moarte (TSTATE_DEAD)
    int     anim_ctr;     // VAR_PLAYER_ANIM_CTR: throttle animație player
} Player;

// =============================================================================
// GAME STATE — starea globală
// Sursa: VAR_GAME_STATE (017430), VAR_LIVES (017436), VAR_SCORE (017440)
// =============================================================================

// Stările jocului (reimplementare — în original nu exista un enum, ci flags disparate)
typedef enum {
    GS_PLAYING,    // joc activ
    GS_DEAD,       // player mort, așteaptă respawn
    GS_LEVEL_WIN,  // nivel complet (tile_state == TSTATE_LEVEL_WIN)
    GS_GAME_OVER,  // lives == 0
    GS_ALL_WIN,    // toate 10 niveluri completate
} GameState;

// Starea completă a jocului (reimplementare)
typedef struct {
    GameState   state;
    float       state_timer;   // timer pentru tranziții (GS_DEAD, GS_LEVEL_WIN)
    int         score;         // VAR_SCORE; init=0, +SCORE_PER_GOLD la aur
    int         lives;         // VAR_LIVES; init=9 (0o333 în original)
    int         level;         // 0-based (0–9), nivel curent
} GameVars;
