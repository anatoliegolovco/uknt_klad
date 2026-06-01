// player.c — logica jucătorului, derivată instrucțiune cu instrucțiune din ASM
#include "player.h"

// ── helpers ──────────────────────────────────────────────────────────────────

// Coloana și rândul tile central al playerului
static void center_tile(const Player *p, int *col, int *row) {
    *col = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    *row = (int)((p->py + TILE_H * 0.5f) / TILE_H);
}

// "Pe scară" = celula centrală e scară (regula strictă din ASM: PLAYER_STATE_CHECK
// lucrează pe tile-ul curent). Fără passthrough — segmentele separate de platforme
// rămîn separate (nu urci prin poduri/platforme).
static bool on_ladder(const Player *p, const Map *m) {
    int cx = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    int cy = (int)((p->py + TILE_H * 0.5f) / TILE_H);
    return map_ladder(m, cx, cy);
}

// CMAP_FLAGS (013570): scările trec PRIN platforme — dar doar dacă scara continuă
// dincolo de platformă. Permite mișcarea verticală în celula țintă dacă:
//   - e scară, sau goală/pasabilă (nu zid), sau
//   - e zid DAR celula imediat următoare în aceeași direcție e scară (climb-through).
// dir < 0 = sus, dir > 0 = jos.
// Mișcarea verticală pe scară, din ASM dar fără oscilație (modelul nostru e continuu):
//   SUS  (dir<0): doar într-o celulă SCARĂ → te oprești curat la vârful scării
//                 (nu urci în aer ca să cazi înapoi; nu treci prin platforme).
//   JOS  (dir>0): în orice celulă care NU e zid (scară/aer/aur/ieșire) → cobori
//                 segmentul și pășești jos; zidul (podea/platformă) te oprește.
static bool can_climb_into(const Map *m, int cx, int cell_row, int dir) {
    if (dir < 0) return map_at(m, cx, cell_row) == T_LADDER;   // sus: doar pe scară
    return map_at(m, cx, cell_row) != T_WALL;                  // jos: orice non-zid
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

    TileType typ = map_at(m, col, row);
    // WATER_COLLISION (006462): tile = water → PLAYER_DEATH
    if (typ == T_WATER) return PR_WATER;
    // EXIT: tile = exit → nivel complet
    if (typ == T_EXIT)  return PR_EXIT;

    return PR_NONE;
}
