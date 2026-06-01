// player.c — logica jucătorului, derivată instrucțiune cu instrucțiune din ASM
#include "player.h"

// ── helpers ──────────────────────────────────────────────────────────────────

// Coloana și rândul tile central al playerului
static void center_tile(const Player *p, int *col, int *row) {
    *col = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    *row = (int)((p->py + TILE_H * 0.5f) / TILE_H);
}

// ── API ───────────────────────────────────────────────────────────────────────

// GAME_INIT (004000) → spawnul playerului la coord din entity record
void player_init(Player *p, int spawn_col, int spawn_row) {
    *p = (Player){
        .px         = (float)(spawn_col * TILE_W),
        .py         = (float)(spawn_row * TILE_H),
        .vy         = 0.0f,
        .on_ladder  = false,
        .dead       = false,
        .facing     = 1,           // implicit cu fața la dreapta
        .anim       = PA_STAND,
        .anim_ctr   = 0,
        .anim_frame = 0,
    };
}

// PLAYER_MOVE_STEP (012740) + PLAYER_STATE_CHECK (012570)
// Returnează PR_* pentru ca game_tick() să reacționeze.
PlayerResult player_update(Player *p, const Map *m, Input in, float dt) {
    // Celula curentă + "pe scară" = celulă RAW ladder2 (tile 8) — ca originalul.
    int col, row;
    center_tile(p, &col, &row);
    p->on_ladder = (map_raw(m, col, row) == 8);

    // ── mișcare STRICT după flag-urile COLLISION_MAP_BUILD (013570) ───────────
    // Prioritate: cățărat (scară) → mers pe suport → gravitație. Aliniere la grilă
    // (px la coloană când urci/cazi, py la rînd când mergi) ca să nu se blocheze
    // între celule (modelul nostru e continuu, originalul e pe celule).
    bool climbed = false;
    if (in.up && map_can_up(m, col, row)) {          // #20000: urcă scara
        p->px = (float)(col * TILE_W); p->py -= PLAYER_SPEED * dt; p->vy = 0.0f; climbed = true;
    } else if (in.down && map_can_down(m, col, row)) { // #10000: coboară scara
        p->px = (float)(col * TILE_W); p->py += PLAYER_SPEED * dt; p->vy = 0.0f; climbed = true;
    }
    if (!climbed) {
        bool supported = p->on_ladder || map_grounded(m, col, row);   // #4000
        if (supported) {
            p->vy = 0.0f;
            if (!p->on_ladder) p->py = (float)(row * TILE_H);          // stă pe platformă
            if      (in.right && map_can_right(m, col, row)) p->px += PLAYER_SPEED * dt; // #1000
            else if (in.left  && map_can_left (m, col, row)) p->px -= PLAYER_SPEED * dt; // #400
        } else {
            // gravitație: cade pe coloană până ajunge pe suport/scară
            p->vy += PLAYER_GRAVITY * dt; if (p->vy > 300.0f) p->vy = 300.0f;
            p->px = (float)(col * TILE_W); p->py += p->vy * dt;
        }
    }

    // Clamp la limitele nivelului
    if (p->px < 0.0f) p->px = 0.0f;
    if (p->px > (MAP_COLS - 1) * (float)TILE_W) p->px = (MAP_COLS - 1) * (float)TILE_W;
    if (p->py < 0.0f)                           { p->py = 0.0f;  p->vy = 0.0f; }
    if (p->py > (MAP_ROWS - 1) * (float)TILE_H) p->py = (MAP_ROWS - 1) * (float)TILE_H;

    // ── direcție + stare animație (SPRITE_HELPERS 013216) ────────────────────
    // ASM: dacă entity[+0o16]==0o12 (pe scară) → state 0o21 (climb), altfel 0o23 (walk).
    if (in.right) p->facing = 1;
    else if (in.left) p->facing = -1;

    bool moving_h = in.left || in.right;
    bool moving_v = p->on_ladder && (in.up || in.down);
    if (p->on_ladder && (moving_v || moving_h))  p->anim = PA_CLIMB;  // state 0o21
    else if (moving_h)                           p->anim = PA_WALK;   // state 0o23
    else                                         p->anim = PA_STAND;

    // ── animație (ANIM_THROTTLE_PLAYER 007432) ───────────────────────────────
    // CMP #3, @#PLAYER_ANIM_CTR: la al 4-lea tick → schimbă frame, resetează
    bool moving = moving_h || moving_v;
    if (moving) {
        p->anim_ctr++;
        if (p->anim_ctr >= PLAYER_ANIM_STATES) {
            p->anim_ctr   = 0;
            p->anim_frame = (p->anim_frame + 1) % PLAYER_ANIM_STATES;
        }
    }

    // ── PLAYER_STATE_CHECK (012570) ──────────────────────────────────────────
    // MOV @#14422, R3 → tile la poziția playerului (recalculat după mișcare)
    center_tile(p, &col, &row);
    TileIdx idx = map_raw(m, col, row);

    // CMPB #4,(R3): tile = GOLD_A → SCORE_ADD
    if (idx == TIDX_GOLD_A) return PR_GOLD;
    // CMPB #5,(R3): tile = GOLD_B → BONUS_LIFE_ADD
    if (idx == TIDX_GOLD_B) return PR_BONUS;
    // CMPB #6,(R3): tile = GOLD_C → LEVEL_COMPLETE
    if (idx == TIDX_GOLD_C) return PR_LEVEL_WIN;

    // WATER_COLLISION (006462) + flag #2000 (ACT_DISPATCH 001450 → drown → state 0o15):
    // mori dacă ești ÎN apă sau stai pe SUPRAFAȚA apei (jos e apă, nu pe scară). Asta acoperă
    // și "căderea în apă": deasupra apei adânci (13/14) nu ești grounded → cazi → te îneci.
    if (map_drowns(m, col, row)) return PR_WATER;
    // EXIT: tile = exit → nivel complet
    TileType typ = map_at(m, col, row);
    if (typ == T_EXIT)  return PR_EXIT;

    return PR_NONE;
}
