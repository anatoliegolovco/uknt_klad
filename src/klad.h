// klad.h — tipuri și constante comune, derivate din uknc_klad_1987.asm
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── dimensiuni fixe, din COLLISION_MAP_BUILD (013524) și stride 0o540 ────────
// ADD #0o540 (=352) per nivel confirmă 22×16 bytes = 22×32 tiles
enum {
    MAP_COLS  = 32,
    MAP_ROWS  = 22,
    TILE_PX   = 8,    // pixeli per tile
    VW        = 256,  // lățime render target = MAP_COLS × TILE_PX
    VH        = 192,  // înălțime render target = (MAP_ROWS × TILE_PX) + 16 HUD
};

// ── tile indices 0-15, din DAT_TILE_* symbols (017450–020030) ────────────────
typedef uint8_t TileIdx;
enum {
    TIDX_AIR     =  0,  // 017450: background, never blitted
    TIDX_LADDER  =  1,  // 017470: scară tip 1
    TIDX_EXIT    =  2,  // 017510: ieșire nivel (pixels = air, collision = exit)
    TIDX_GOLD_A  =  4,  // 017550: aur → SCORE_ADD (003764)
    TIDX_GOLD_B  =  5,  // 017570: aur → BONUS_LIFE_ADD (003746)
    TIDX_GOLD_C  =  6,  // 017610: aur → LEVEL_COMPLETE (001034)
    TIDX_WATER   =  7,  // 017630: apă, letal
    TIDX_LADDER2 =  8,  // 017650: scară tip 2 (pixels identici cu tip 1)
    TIDX_WATER2  = 14,  // 020010: apă animată, letal
};

// ── tipuri logice tile, derivate din orig_to_tile în PLAYER_STATE_CHECK ──────
typedef enum {
    T_EMPTY  = 0,
    T_WALL,
    T_LADDER,
    T_WATER,
    T_GOLD,
    T_EXIT,
} TileType;

static inline TileType tidx_to_type(TileIdx i) {
    switch (i) {
        case 1: case 8:              return T_LADDER;
        case 2:                      return T_EXIT;
        case 4: case 5: case 6:     return T_GOLD;
        case 7: case 14:            return T_WATER;
        case 9: case 10: case 11:
        case 12: case 13:           return T_WALL;
        default:                    return T_EMPTY;
    }
}

// ── stări joc (reimplementare — originalul folosea flags disparate) ───────────
typedef enum {
    GS_PLAYING,
    GS_DEAD,
    GS_LEVEL_WIN,
    GS_GAME_OVER,
    GS_ALL_WIN,
} GameState;

// ── input abstract (independent de platformă) ─────────────────────────────────
typedef struct {
    bool left, right, up, down, action;
} Input;
