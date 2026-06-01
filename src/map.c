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
    m->door_open  = false;          // KI-10: ușa închisă la început (cheia o deschide)
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

// Raw tile index with out-of-bounds = wall (COLLISION_MAP_BUILD compares neighbour tiles;
// the playfield border is walls, so OOB must read as a wall, not air).
static int raw_w(const Map *m, int c, int r) {
    if (c < 0 || c >= MAP_COLS || r < 0 || r >= MAP_ROWS) return 11; // wall
    return map_raw(m, c, r);
}
// Pasabil = non-wall (≤8), SAU ușa (tile 10) când e deschisă (KI-10: cheia o deschide).
static bool pass(const Map *m, int c, int r) {
    int t = raw_w(m, c, r);
    return t <= 8 || (t == 10 && m->door_open);
}
// Per-cell movement flags — EXACT from COLLISION_MAP_BUILD (013570). RAW tile index:
// ≤8 = non-wall (air/ladder/gold/water), 8 = climbable ladder (ladder2), 10 = ușă.
bool map_can_right(const Map *m, int c, int r) { return pass(m, c+1, r); }              // #1000
bool map_can_left (const Map *m, int c, int r) { return pass(m, c-1, r); }              // #400
bool map_can_up   (const Map *m, int c, int r) { return raw_w(m, c, r) == 8 && pass(m, c, r-1); } // #20000
bool map_can_down (const Map *m, int c, int r) { return raw_w(m, c, r+1) == 8; }        // #10000
bool map_grounded (const Map *m, int c, int r) { return raw_w(m, c, r) == 8 || raw_w(m, c, r+1) > 6; } // #4000

void map_open_door(Map *m) { m->door_open = true; }

// PLAYER_STATE_CHECK (012570): CLRB (R3) — șterge tile după colectare
void map_clear(Map *m, int col, int row) {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return;
    m->raw[row][col] = TIDX_AIR;
    m->typ[row][col] = T_EMPTY;
}
