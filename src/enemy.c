// enemy.c — AI inamici, derivat din ENEMY2_TICK / ENEMY3_TICK
#include "enemy.h"

// ENEMY_RESPAWN (012716): MOVB #011,(R5); BIS flags
void enemy_init(Enemy *e, int col, int row) {
    *e = (Enemy){
        .col      = col,
        .row      = row,
        .active   = (col >= 0),  // col < 0 = absent în acest nivel
        .dir      = 1,
        .falling  = false,
        .warmup   = ENEMY_WARMUP,
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
// Flag-uri verticale, exact ca CMAP (013724/013740/013702): #20000 SUS (cur==8 && sus≤8),
// #10000 JOS (jos==8 → pășești pe scara de dedesubt, SAU cur==8 && jos≤6 → cobori în aer).
static bool ecan_up  (const Map *m, int c, int r) { return map_raw(m, c, r) == 8 && map_raw(m, c, r-1) <= 8; }
static bool ecan_down(const Map *m, int c, int r) { return map_raw(m, c, r+1) == 8 || (map_raw(m, c, r) == 8 && map_raw(m, c, r+1) <= 6); }

// ENEMY2_MOVE (006602..007134) — chase greedy MODERAT (nu pathfinding, dar funcțional):
//   - un pas ORIZONTAL spre coloana jucătorului (prioritar);
//   - dacă e BLOCAT orizontal SAU deja pe coloana lui → un pas VERTICAL spre rândul lui,
//     folosind scările (urcă #20000 / COBOARĂ #10000 — poate coborî de pe platformă pe scara
//     de dedesubt, nu doar când e deja pe scară);
//   - apoi cade UN rând dacă nu e grounded (006712). Se mai blochează (greedy), dar acum
//     chiar coboară scările ca să ajungă la tine — nici prea deștept, nici inert.
static void do_move(Enemy *e, const Map *m, const Player *p) {
    // ── cădere ANGAJATĂ: cât cade NU se agață de scări (ca jucătorul) ─────────────
    // Ignoră scara din celula curentă; cade un rând/tick până are PODEA dedesubt (jos>6).
    // (Fără termenul cur==8 din egrounded → nu „revine pe scară" cât e în cădere.)
    if (e->falling) {
        if (e->row + 1 < MAP_ROWS && map_raw(m, e->col, e->row + 1) <= 6)
            e->row++;                       // încă aer/aur dedesubt → continuă căderea
        else
            e->falling = false;             // podea dedesubt → a aterizat
        return;                             // nu urmărește cât cade
    }

    int pcol = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    int prow = (int)((p->py + TILE_H * 0.5f) / TILE_H);

    bool moved = false;
    if (e->col != pcol) {                                  // orizontal spre coloana jucătorului
        int dx = (pcol > e->col) ? 1 : -1;
        if (epass(m, e->col + dx, e->row)) {
            e->col += dx; e->dir = dx;
            e->anim_horiz = (e->anim_horiz + 1) % ENEMY_ANIM_HORIZ;
            moved = true;
        }
    }
    if (!moved && e->row != prow) {                        // aliniat SAU blocat → vertical pe scară
        if (prow > e->row && ecan_down(m, e->col, e->row)) {
            e->row++; e->anim_vert = (e->anim_vert + 1) % ENEMY_ANIM_VERT; moved = true;
        } else if (prow < e->row && ecan_up(m, e->col, e->row)) {
            e->row--; e->anim_vert = (e->anim_vert + 1) % ENEMY_ANIM_VERT; moved = true;
        }
    }

    // Dacă a pășit în gol (nu pe scară, fără podea dedesubt) → începe o cădere angajată.
    if (!moved && map_raw(m, e->col, e->row) != 8 && map_raw(m, e->col, e->row + 1) <= 6) {
        e->falling = true;
        if (e->row + 1 < MAP_ROWS) e->row++;
    }
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

    // Warmup (ENEMY2_TICK 006562): inamicul stă pe loc la începutul nivelului, apoi pornește.
    if (e->warmup > 0.0f) {
        e->warmup -= dt;
    } else {
        // Throttle mișcare (echivalent CMP #400, @#TICK_CTR)
        e->move_cd -= dt;
        if (e->move_cd <= 0.0f) {
            e->move_cd = ENEMY_MOVE_INTERVAL;
            do_move(e, m, p);
        }
    }

    // LEVEL_END_CHECK (001636): player și inamic pe același tile → moarte player
    int pcol = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    int prow = (int)((p->py + TILE_H * 0.5f) / TILE_H);
    return (e->col == pcol && e->row == prow);
}
