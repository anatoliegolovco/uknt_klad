// map.h — hartă nivel și logică tile
// Sursa ASM: COLLISION_MAP_BUILD (013524), LEVEL_COMPLETE (001034)
#pragma once
#include "klad.h"
#include "level_data.h"

// COLLISION_MAP_BUILD (013524) unpacks la BUF_TILE_WORK (014550):
//   22 rânduri × 32 coloane octeți, fiecare = tile index 0-15.
//   ADD #2576 = buffer size = 22×32×2/2... de fapt 22×32 words = 1408 bytes.
// Noi stocăm același lucru în doi array separați pentru claritate.
typedef struct {
    TileIdx  raw[MAP_ROWS][MAP_COLS];  // index original 0-15 (pentru render)
    TileType typ[MAP_ROWS][MAP_COLS];  // tip logic (pentru coliziune)
    int      gold_count;               // aurul rămas pe nivel
    bool     door_open;                // KI-10: cheia (gold_c) deschide ușa (tile 10)
} Map;

// KI-10: deschide ușa nivelului (apelat la colectarea cheii gold_c). Rutina ASM 12716
// rescrie celulele @17424/@17426 (tile 10 = ușa) făcându-le pasabile.
void map_open_door(Map *m);

// COLLISION_MAP_BUILD (013524): MOVB (R4),(R2); BIC #177760,(R2)+ → low nibble
//                               MOVB (R4)+,(R2); BIC #177417,(R2); ASR×4 → high nibble
// tools/gen_levels_c.py face același lucru și produce LEVEL_TILES[10][22][32].
void map_load(Map *m, int level);

// Acces cu boundary check — în original: boundary = WALL implicit
TileType map_at(const Map *m, int col, int row);
TileIdx  map_raw(const Map *m, int col, int row);

// Predicatele logice folosite în PLAYER_MOVE_STEP și ENEMY_TICK
bool map_solid(const Map *m, int col, int row);   // T_WALL blochează
bool map_ladder(const Map *m, int col, int row);  // T_LADDER permite vertical
bool map_water(const Map *m, int col, int row);   // T_WATER = letal

// FLAG-URI DE MIȘCARE PER-CELULĂ — fidele cu COLLISION_MAP_BUILD (013570).
// Originalul mișcă jucătorul/inamicii STRICT după aceste flag-uri (nu euristici).
// Index tile RAW: ≤8 = nu-zid (aer/scară/aur/apă), 8 = scară climbable (ladder2).
bool map_can_right(const Map *m, int col, int row);  // #1000: tile dreapta ≤ 8
bool map_can_left (const Map *m, int col, int row);  // #400 : tile stânga ≤ 8
bool map_can_up   (const Map *m, int col, int row);  // #20000: curent==8 ȘI sus ≤ 8
bool map_can_down (const Map *m, int col, int row);  // #10000: jos == 8
bool map_grounded (const Map *m, int col, int row);  // #4000: suport (curent==8 sau jos>6)
bool map_drowns   (const Map *m, int col, int row);  // apă letală: în apă sau pe suprafața apei

// PLAYER_STATE_CHECK (012570): CLRB (R3) → tile → EMPTY după colectare aur
void map_clear(Map *m, int col, int row);
