// player.c — logica jucătorului, derivată instrucțiune cu instrucțiune din ASM
#include "player.h"

// ── helpers ──────────────────────────────────────────────────────────────────

// Coloana și rândul tile central al playerului
static void center_tile(const Player *p, int *col, int *row) {
    *col = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    *row = (int)((p->py + TILE_H * 0.5f) / TILE_H);
}

// "Pe scară" = celula centrală e scară, SAU ești într-un zid (platformă) prin care
// scara trece — adică ai scară și deasupra ȘI dedesubt în coloană (passthrough).
// Așa nu cazi din modul scară când urci prin platforme, dar nu urci prin tavan în aer.
static bool on_ladder(const Player *p, const Map *m) {
    int cx = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    int cy = (int)((p->py + TILE_H * 0.5f) / TILE_H);
    if (map_ladder(m, cx, cy)) return true;
    if (map_at(m, cx, cy) == T_WALL) {
        bool up = false, dn = false;
        for (int r = cy - 1; r >= 0; r--) {
            TileType t = map_at(m, cx, r);
            if (t == T_LADDER) { up = true; break; }
            if (t != T_WALL) break;
        }
        for (int r = cy + 1; r < MAP_ROWS; r++) {
            TileType t = map_at(m, cx, r);
            if (t == T_LADDER) { dn = true; break; }
            if (t != T_WALL) break;
        }
        return up && dn;   // prins între scară sus și jos → încă pe scară
    }
    return false;
}

// CMAP_FLAGS (013570): scările trec PRIN platforme — dar doar dacă scara continuă
// dincolo de platformă. Permite mișcarea verticală în celula țintă dacă:
//   - e scară, sau goală/pasabilă (nu zid), sau
//   - e zid DAR celula imediat următoare în aceeași direcție e scară (climb-through).
// dir < 0 = sus, dir > 0 = jos.
static bool can_climb_into(const Map *m, int cx, int cell_row, int dir) {
    TileType t = map_at(m, cx, cell_row);
    if (t == T_LADDER) return true;
    if (t == T_WALL) {
        // traversează MAI MULTE platforme consecutive dacă scara continuă dincolo
        for (int r = cell_row + dir; r >= 0 && r < MAP_ROWS; r += dir) {
            TileType tr = map_at(m, cx, r);
            if (tr == T_LADDER) return true;   // scara continuă → treci prin platforme
            if (tr != T_WALL)   break;          // gol/apă → nu mai e scară
        }
        return false;
    }
    return dir > 0;   // gol/aur/exit: permite jos (cobori de pe scară), blochează sus (nu pluti)
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
    p->on_ladder = on_ladder(p, m);

    // ── mișcare verticală: scară vs gravitație ───────────────────────────────
    // PLAYER_MOVE_STEP: dacă tile curent = LADDER → control vertical direct
    if (p->on_ladder) {
        p->vy = 0.0f;
        int dir = in.down - in.up;            // +1 jos, -1 sus
        if (dir != 0) {
            float ny  = p->py + (float)dir * PLAYER_SPEED * dt;
            int   cx  = (int)((p->px + TILE_W * 0.5f) / TILE_W);
            // rândul muchiei de avans în direcția mișcării
            int   row = (int)(((dir > 0) ? ny + TILE_H - 1 : ny) / TILE_H);
            if (can_climb_into(m, cx, row, dir)) p->py = ny;
        }
    } else {
        // Gravitație: ADD la viteza verticală, oprire la WALL de jos
        p->vy += PLAYER_GRAVITY * dt;
        float ny  = p->py + p->vy * dt;
        int   ftx = (int)((p->px + TILE_W * 0.5f) / TILE_W);
        int   fty = (int)((ny + TILE_H) / TILE_H);
        if (map_solid(m, ftx, fty)) {
            p->vy = 0.0f;
            ny    = (float)(fty * TILE_H) - (float)TILE_H;
        }
        p->py = ny;
    }
    // ── mișcare orizontală (DUPĂ verticală) ──────────────────────────────────
    // КЛАД e pe celule: dacă vrei să mergi lateral de pe scară, aliniază la rînd
    // ca să nu rămîi blocat între două rînduri (col-coliziunea folosea rîndul greșit).
    if ((in.left || in.right) && p->on_ladder && !(in.up || in.down)) {
        float snapped = (float)((int)((p->py + TILE_H * 0.5f) / TILE_H) * TILE_H);
        // aliniază doar dacă rîndul țintă e liber (altfel rămîi pe scară)
        int   cx = (int)((p->px + TILE_W * 0.5f) / TILE_W);
        int   sr = (int)(snapped / TILE_H);
        if (!map_solid(m, cx, sr)) p->py = snapped;
    }
    float nx  = p->px + (float)(in.right - in.left) * PLAYER_SPEED * dt;
    int   ckx = (int)((nx + (in.right ? TILE_W - 1 : 0)) / TILE_W);
    int   midy = (int)((p->py + TILE_H * 0.5f) / TILE_H);
    if (!map_solid(m, ckx, midy)) p->px = nx;

    // Clamp la limitele nivelului
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
    // MOV @#14422, R3 → tile la poziția playerului
    int col, row;
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
