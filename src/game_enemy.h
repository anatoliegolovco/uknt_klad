// game_enemy.h — Logica inamicilor din КЛАД 1987
// Sursa ASM: ENEMY1/2/3_TICK, SPRITE_ANIM_A/B/C/D, ENEMY_RESPAWN, ENTITY1_RESTORE
#pragma once
#include "uknc_defs.h"
#include "game_entity.h"
#include "game_map.h"

// =============================================================================
// THROTTLE INAMICI
// Sursa: ENEMY2_TICK (006552): CMP #400, @#17376; BNE → INC; else → MOVE
//        ENEMY3_TICK (007462): CMP #400, @#17360; BGE → INC; else → MOVE
// 0o400 = 256 ticks de joc între mișcări.
// Reimplementare: timp real în secunde (0.42s ≈ 256 ticks la ~60fps game loop)
// =============================================================================
#define ENEMY_TICK_INTERVAL  0.42f   // secunde între pași inamic

// =============================================================================
// SPRITE_ANIM_A/B (010102, 010142) — enemy3 horiz + vert
// SPRITE_ANIM_C/D (007202, 007242) — enemy2 horiz + vert
// ASM: "throttle 8-frame" pentru orizontal, "throttle 5-frame" pentru vertical
// =============================================================================
#define ENEMY_ANIM_HORIZ_FRAMES  ANIM_FRAMES_HORIZ   // 8 frame-uri orizontal
#define ENEMY_ANIM_VERT_FRAMES   ANIM_FRAMES_VERT    // 5 frame-uri vertical
#define ENEMY_ANIM_INTERVAL      0.18f               // secunde între frame-uri

// =============================================================================
// ENEMY_RESPAWN (012716)
// ASM:
//   MOVB #11, (R5)               → stare entitate = 011 (activ)
//   BIS #100000, 177776(R5)      → setează flag activ în display
//   BIS #40000, 2(R5)            → setează flag coliziune
//   RTS PC
// Concluzie: resetează inamicul la spawn (stare activă, flags setate).
// =============================================================================

// Inițializare inamic la spawn. Apelat și la LEVEL_COMPLETE pentru Enemy respawn.
static inline void enemy_init(Entity *e, int col, int row) {
    e->active    = (col >= 0);   // col < 0 = absent în nivel
    e->col       = col;
    e->row       = row;
    e->anim_horiz = 0;
    e->anim_vert  = 0;
    e->tick_ctr  = 0;
    e->dir       = 1;
}

// =============================================================================
// ENEMY AI — din ENEMY2_TICK / ENEMY3_TICK (006552 / 007462)
// ASM (logica de chase greedy):
//   1. Calculează offset col inamic față de player (BUF_TILE_WORK addresses)
//   2. Dacă col_inamic < col_player AND tile_dreapta nu e blocat → merge dreapta
//   3. Dacă col_inamic > col_player AND tile_stânga nu e blocat → merge stânga
//   4. Dacă pe scară: poate merge vertical (sus/jos spre player)
//   5. Gravitație: cade dacă tile de jos e gol (nu WALL/LADDER/WATER)
// =============================================================================

// Gravitație inamic: cade până la WALL / LADDER / WATER
static inline void enemy_gravity(Entity *e) {
    while (e->row + 1 < MAP_ROWS) {
        TileType below = map_at(e->col, e->row + 1);
        if (below == T_WALL || below == T_LADDER || below == T_WATER) break;
        e->row++;
    }
}

// Un pas de mișcare inamic (chase greedy, fără apă, cu gravitație)
// ptx, pty = col/row player curent
static inline void enemy_step(Entity *e, int ptx, int pty) {
    int dx = (ptx > e->col) ? 1 : (ptx < e->col) ? -1 : 0;
    int dy = (pty > e->row) ? 1 : (pty < e->row) ? -1 : 0;

    // Mișcare orizontală prioritară (din ASM: verifică horizontal first)
    TileType htile = map_at(e->col + dx, e->row);
    if (dx && htile != T_WALL && htile != T_WATER) {
        e->col += dx;
        e->dir  = dx;
    }
    // Vertical: doar pe scară
    else if (dy && map_at(e->col, e->row) == T_LADDER) {
        TileType vtile = map_at(e->col, e->row + dy);
        if (vtile != T_WALL) e->row += dy;
    }

    enemy_gravity(e);
}

// =============================================================================
// LEVEL_END_CHECK (001636)
// ASM:
//   CMP @#14422, @#14432  → player_tile_ptr == enemy1_tile_ptr?
//   BNE skip
//   JMP @#2040            → GAME_OVER (player killed by enemy)
//   CMP @#14422, @#14442  → player_tile_ptr == enemy2_tile_ptr?
//   BNE skip
//   JMP @#2040
// Concluzie: dacă player și inamic sunt pe același tile → player mort.
// =============================================================================

// Verifică coliziune player-inamic (same tile → moarte)
static inline bool enemy_hits_player(const Entity *e, int ptx, int pty) {
    return e->active && e->col == ptx && e->row == pty;
}
