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

// Inamicul poate păși într-o celulă: tile RAW ≤ 8 (aer/scară/aur/apă-mică/scară2).
// Originalul (ENEMY2_MOVE 006654/006662) testează #100000 (≤9) DAR exclude tile 9
// (CMPB #11,2(R1) → BEQ skip), deci net = ≤8. Inamicul NU calcă pe tile 9 (podea/ușă),
// spre deosebire de jucător (≤9) — de-asta inamicul e mai limitat.
static bool epass(const Map *m, int c, int r) { return map_raw(m, c, r) <= 8; }
// Grounded inamic = #4000: pe scară (raw==8) SAU jos solid (jos>6).
static bool egrounded(const Map *m, int c, int r) {
    return map_raw(m, c, r) == 8 || map_raw(m, c, r + 1) > 6;
}

// ENEMY2_MOVE (006602..007134) — chase GREEDY PRIMITIV (NU pathfinding):
//   - dacă inamicul NU e pe coloana jucătorului → un pas ORIZONTAL spre coloana lui;
//   - DOAR când e pe ACEEAȘI coloană urcă/coboară pe scară spre rândul jucătorului;
//   - apoi, dacă nu e grounded, cade UN SINGUR rând (006712: ADD #100 → 7242).
// => se blochează ușor (nu urcă scări ca să te urmărească decât dacă ești pe coloana lui),
//    cade lent (un rând/tick). Asta îl face „prostuț", ca originalul (nu „prea deștept").
static void do_move(Enemy *e, const Map *m, const Player *p) {
    int pcol = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    int prow = (int)((p->py + TILE_H * 0.5f) / TILE_H);

    if (e->col != pcol) {                                  // coloane diferite → orizontal
        int dx = (pcol > e->col) ? 1 : -1;
        if (epass(m, e->col + dx, e->row)) {
            e->col += dx; e->dir = dx;
            e->anim_horiz = (e->anim_horiz + 1) % ENEMY_ANIM_HORIZ;
        }
    } else if (e->row != prow && map_raw(m, e->col, e->row) == 8) {  // aceeași coloană + pe scară
        int dy = (prow > e->row) ? 1 : -1;
        if (epass(m, e->col, e->row + dy)) {
            e->row += dy;
            e->anim_vert = (e->anim_vert + 1) % ENEMY_ANIM_VERT;
        }
    }

    // Gravitație UN rând/tick (006712), nu instant — coboară lent, ca originalul.
    if (e->row + 1 < MAP_ROWS && !egrounded(m, e->col, e->row))
        e->row++;
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
    int pcol = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    int prow = (int)((p->py + TILE_H * 0.5f) / TILE_H);
    return (e->col == pcol && e->row == prow);
}
