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
// Pasabil pentru JUCĂTOR = tile ≤ 9, SAU ușa (tile 10) deschisă (KI-10).
// CMAP_FLAGS (013570) calculează DOUĂ seturi de flag-uri orizontale:
//   #1000/#400   : vecin ≤ 8  (folosit de INAMICI — nu pot trece tile 9)
//   #100000/#40000: vecin ≤ 9  (folosit de JUCĂTOR — calcă pe tile 9 = podea/ușă deschisă)
// Deschiderea ușii (ENEMY_RESPAWN 012716, la luarea cheii) face MOVB #11=9 în celula ușii
// + setează #100000/#40000 pe vecini → ușa (tile 10) devine tile 9 = pasabilă pentru jucător.
// SPRITE_HELPERS (013346) confirmă: jucătorul testează #101000 (#1000|#100000) / #40400.
static bool pass(const Map *m, int c, int r) {       // JUCĂTOR: ≤9
    int t = raw_w(m, c, r);
    return t <= 9 || (t == 10 && m->door_open);
}
// Per-cell movement flags — EXACT from COLLISION_MAP_BUILD (013570). RAW tile index:
// ≤9 = pasabil jucător (aer/scară/aur/apă/podea-9), 8 = scară climbable, 10 = ușă.
bool map_can_right(const Map *m, int c, int r) { return pass(m, c+1, r); }              // #100000
bool map_can_left (const Map *m, int c, int r) { return pass(m, c-1, r); }              // #40000
// #20000: cur==8 && above≤8. PLUS (KI-13 design B): poți urca ÎN ieșire (tile 2 = ușa de sus)
// din celula de dedesubt, chiar fără scară — „urci în dreapta pe scară în sus și ieși". Ieșirea
// e unică pe nivel, deci nu produce victorii false. Face toate 10 nivele completabile.
bool map_can_up   (const Map *m, int c, int r) {
    if (raw_w(m, c, r-1) == TIDX_EXIT) return true;          // urcă în ieșire
    return raw_w(m, c, r) == 8 && raw_w(m, c, r-1) <= 8;
}
// #10000 DOWN (CMAP 013724/013740): jos==8 (pășești pe scara de dedesubt) SAU cur==8 && jos≤6
// (cobori de pe scară în aer). Lipsea al doilea caz → scări care se termină peste gol blocau.
bool map_can_down (const Map *m, int c, int r) {
    return raw_w(m, c, r+1) == 8 || (raw_w(m, c, r) == 8 && raw_w(m, c, r+1) <= 6);
}
// #4000 grounded: cur==8 SAU jos>6 — DAR CMAP (014012) ȘTERGE #4000 dacă jos≥13 (apă
// adâncă). Verificat pe bufferul real УКНЦ: (10..17,20) deasupra tile-13 au GND=0, H2O=1 →
// NU ești grounded deasupra apei adânci → cazi ÎN ea. (cols 2..9 deasupra tile-11 → GND=1.)
bool map_grounded (const Map *m, int c, int r) {
    if (raw_w(m, c, r+1) >= 13) return false;            // apă adâncă jos → cazi prin (CMAP clears #4000)
    return raw_w(m, c, r) == 8 || raw_w(m, c, r+1) > 6;
}

// Apă LETALĂ = tile 13/14 (apă adâncă). ACT_DISPATCH (001500): CMPB #15,@2(R4) → dacă tile-ul
// celulei CURENTE == 0o15 (13) → moarte (drown). Tile 7 = apă MICĂ, pasabilă, NU letală.
// Cazi în apa adâncă fiindcă deasupra ei nu ești grounded (vezi map_grounded), apoi cur==13/14.
static bool is_deep_water(int t) { return t == 13 || t == 14; }
bool map_drowns(const Map *m, int c, int r) { return is_deep_water(raw_w(m, c, r)); }

void map_open_door(Map *m) { m->door_open = true; }

// PLAYER_STATE_CHECK (012570): CLRB (R3) — șterge tile după colectare
void map_clear(Map *m, int col, int row) {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return;
    m->raw[row][col] = TIDX_AIR;
    m->typ[row][col] = T_EMPTY;
}
