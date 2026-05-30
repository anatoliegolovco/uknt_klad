// map.c — implementare hartă nivel
// Sursa ASM: COLLISION_MAP_BUILD (013524)
#include "map.h"

// COLLISION_MAP_BUILD (013524):
//   MOV #14550, R2  → start BUF_TILE_WORK
//   ADD #2576, R3   → end   BUF_TILE_WORK
//   MOVB (R4),(R2); BIC #177760,(R2)+  → low nibble = col*2+0
//   MOVB (R4)+,(R2); BIC #177417; ASR×4 → high nibble >> 4 = col*2+1
// tools/gen_levels_c.py reproduce aceeași logică → LEVEL_TILES[level][row][col]
void map_load(Map *m, int level) {
    m->gold_count = 0;
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            TileIdx idx  = LEVEL_TILES[level][r][c];
            TileType typ = tidx_to_type(idx);
            m->raw[r][c] = idx;
            m->typ[r][c] = typ;
            if (typ == T_GOLD) m->gold_count++;
        }
    }
}

// Boundary: tot ce e în afara hărții = T_WALL (jucătorul nu iese din nivel)
TileType map_at(const Map *m, int col, int row) {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return T_WALL;
    return m->typ[row][col];
}

TileIdx map_raw(const Map *m, int col, int row) {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return TIDX_AIR;
    return m->raw[row][col];
}

bool map_solid(const Map *m, int col, int row)  { return map_at(m, col, row) == T_WALL;   }
bool map_ladder(const Map *m, int col, int row) { return map_at(m, col, row) == T_LADDER; }
bool map_water(const Map *m, int col, int row)  { return map_at(m, col, row) == T_WATER;  }

// PLAYER_STATE_CHECK (012570): CLRB (R3) — șterge tile după colectare
void map_clear(Map *m, int col, int row) {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return;
    m->raw[row][col] = TIDX_AIR;
    m->typ[row][col] = T_EMPTY;
}
