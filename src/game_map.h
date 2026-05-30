// game_map.h — Harta de nivel și logica tile-urilor din КЛАД 1987
// Sursa: COLLISION_MAP_BUILD (013524), PLAYER_STATE_CHECK (012570),
//        WATER_COLLISION (006462), LEVEL_COMPLETE (001034)
#pragma once
#include "uknc_defs.h"
#include "levels_data.h"
#include <string.h>

// =============================================================================
// HARTA DE NIVEL
// BUF_TILE_WORK (014550) în original: 22×32 cuvinte cu tile index + entity flags
// Aici: doi array separați pentru claritate
// =============================================================================

// Tile index original (0-15) — pentru rendering pixel-perfect
static uint8_t raw_tile[MAP_ROWS][MAP_COLS];

// Tile logic type — pentru coliziune și interacțiune
typedef enum {
    T_EMPTY  = 0,   // aer / transparent
    T_WALL,         // solid — blochează mișcarea
    T_LADDER,       // scară — permite urcare/coborâre verticală
    T_WATER,        // apă — letal la contact (WATER_COLLISION)
    T_GOLD,         // aur — colectabil (+10 scor, tile → T_EMPTY după colectare)
    T_EXIT,         // ieșire nivel — activată când player ajunge here
} TileType;

static TileType  map_tile[MAP_ROWS][MAP_COLS];

// =============================================================================
// CONVERSIE tile index → tile type
// Sursa: PLAYER_STATE_CHECK (012570) detects states 4,5,6 (gold), 9 (enemy),
//        WATER_COLLISION (006462) detects states 13,14 (water),
//        LEVEL_END_CHECK (001636) compares tile pointers.
//
// Mapare confirmată din distribuția nivelului 1 + annotated ASM:
//   0       → T_EMPTY  (aer)
//   1, 8    → T_LADDER (scară)
//   2       → T_EXIT   (ieșire)
//   4, 5, 6 → T_GOLD   (aur A/B/C)
//   7, 14   → T_WATER  (apă)
//   9–13    → T_WALL   (pereți)
// =============================================================================

static inline TileType tile_type_of(TileIdx idx) {
    switch (idx) {
        case TIDX_LADDER:
        case TIDX_LADDER2:  return T_LADDER;
        case TIDX_EXIT:     return T_EXIT;
        case TIDX_GOLD_A:
        case TIDX_GOLD_B:
        case TIDX_GOLD_C:   return T_GOLD;
        case TIDX_WATER:
        case TIDX_WATER2:   return T_WATER;
        case 9: case 10: case 11: case 12: case 13: return T_WALL;
        default:            return T_EMPTY;
    }
}

// =============================================================================
// ACCES tile cu boundary check
// La bord exterior: T_WALL (nu poate ieși din nivel)
// =============================================================================

static inline TileType map_at(int col, int row) {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return T_WALL;
    return map_tile[row][col];
}

static inline int map_solid(int col, int row) {
    return map_at(col, row) == T_WALL;
}

// =============================================================================
// ÎNCĂRCARE NIVEL
// Sursa: LEVEL_COMPLETE (001034): ADD #540, @#CUR_MAP_ADDR (stride=352=MAP_STRIDE)
// Sursa: COLLISION_MAP_BUILD (013524): unpack nibble pairs → BUF_TILE_WORK
// =============================================================================

static int gold_count;  // numărul de tile-uri de aur rămase (urmărit la colectare)

static inline void map_load(int level_idx) {
    gold_count = 0;
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            TileIdx idx = LEVEL_TILES[level_idx][r][c];
            raw_tile[r][c] = idx;
            TileType t = tile_type_of(idx);
            map_tile[r][c] = t;
            if (t == T_GOLD) gold_count++;
        }
    }
}

// Colectare aur: șterge tile-ul din hartă, actualizează contorul
// Sursa: PLAYER_STATE_CHECK (012570): CMPB #4,(R3) → CLRB (R3) → SCORE_ADD
static inline void map_collect_gold(int col, int row) {
    map_tile[row][col] = T_EMPTY;
    raw_tile[row][col] = TIDX_AIR;
    if (gold_count > 0) gold_count--;
}
