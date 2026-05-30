// enemy.c — AI inamici, derivat din ENEMY2_TICK / ENEMY3_TICK
#include "enemy.h"

// ENEMY_RESPAWN (012716): MOVB #011,(R5); BIS flags
void enemy_init(Enemy *e, int col, int row) {
    *e = (Enemy){
        .col      = col,
        .row      = row,
        .active   = (col >= 0),  // col < 0 = absent în acest nivel
        .dir      = 1,
        .move_cd  = ENEMY_MOVE_INTERVAL,
        .anim_t   = 0.0f,
        .anim_horiz = 0,
        .anim_vert  = 0,
    };
}

// Gravitație: inamicul cade până la WALL / LADDER / WATER
// Din ENEMY2_MOVE implicit: inamicul nu zboară, se lipește de platforme
static void apply_gravity(Enemy *e, const Map *m) {
    while (e->row + 1 < MAP_ROWS) {
        TileType below = map_at(m, e->col, e->row + 1);
        if (below == T_WALL || below == T_LADDER || below == T_WATER) break;
        e->row++;
    }
}

// ENEMY2_MOVE (006602..007034): greedy chase, horiz-first, vertical pe scară
// Logica din ASM:
//   R0 = col inamic (din tile ptr offset față de BUF_TILE_WORK start)
//   R1 = col player
//   CMP R0,R1: BGT → inamic la dreapta → încearcă stânga
//              BLT → inamic la stânga → încearcă dreapta
//              BEQ → aceeași coloană → încearcă vertical (sus/jos pe scară)
static void do_move(Enemy *e, const Map *m, const Player *p) {
    int pcol, prow;
    // Col/row player din poziția pixel
    pcol = (int)((p->px + TILE_PX * 0.5f) / TILE_PX);
    prow = (int)((p->py + TILE_PX * 0.5f) / TILE_PX);

    int dx = (pcol > e->col) ? 1 : (pcol < e->col) ? -1 : 0;
    int dy = (prow > e->row) ? 1 : (prow < e->row) ? -1 : 0;

    // Mișcare orizontală (prioritate): BIT #100000,(R1) = tile pasabil?
    if (dx != 0) {
        TileType t = map_at(m, e->col + dx, e->row);
        // Inamicul nu intră în WALL sau WATER (similar cu verificarea flag din ASM)
        if (t != T_WALL && t != T_WATER) {
            e->col += dx;
            e->dir  = dx;
            e->anim_horiz = (e->anim_horiz + 1) % ENEMY_ANIM_HORIZ;
        }
    }
    // Mișcare verticală: doar dacă pe scară (BIT #10000 = ladder flag în original)
    else if (dy != 0 && map_ladder(m, e->col, e->row)) {
        TileType t = map_at(m, e->col, e->row + dy);
        if (t != T_WALL) {
            e->row += dy;
            e->anim_vert = (e->anim_vert + 1) % ENEMY_ANIM_VERT;
        }
    }

    apply_gravity(e, m);
}

// ENEMY2_TICK (006552):
//   TST @#14442 → dacă inamic inactiv, return
//   CMP #400, @#TICK_CTR; BNE → INC TICK_CTR, return
//   (la reset counter = 0) → ENEMY2_MOVE
// LEVEL_END_CHECK (001636): CMP @#PLAYER_TILEPTR, @#ENEMY_TILEPTR → dacă egal → moarte
bool enemy_tick(Enemy *e, const Map *m, const Player *p, float dt) {
    if (!e->active) return false;

    // Animație continuă (SPRITE_ANIM_A/C: 8-frame horiz)
    e->anim_t += dt;

    // Throttle mișcare (echivalent CMP #400, @#TICK_CTR)
    e->move_cd -= dt;
    if (e->move_cd <= 0.0f) {
        e->move_cd = ENEMY_MOVE_INTERVAL;
        do_move(e, m, p);
    }

    // LEVEL_END_CHECK (001636): player și inamic pe același tile → moarte player
    int pcol = (int)((p->px + TILE_PX * 0.5f) / TILE_PX);
    int prow = (int)((p->py + TILE_PX * 0.5f) / TILE_PX);
    return (e->col == pcol && e->row == prow);
}
