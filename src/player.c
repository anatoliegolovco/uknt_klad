// player.c — logica jucătorului, derivată instrucțiune cu instrucțiune din ASM
#include "player.h"

// ── helpers ──────────────────────────────────────────────────────────────────

// Coloana și rândul tile central al playerului
static void center_tile(const Player *p, int *col, int *row) {
    *col = (int)((p->px + TILE_PX * 0.5f) / TILE_PX);
    *row = (int)((p->py + TILE_PX * 0.5f) / TILE_PX);
}

// CMAP_FLAGS (013570): flag #20000 = tile scară cu tavan solid deasupra.
// Asta înseamnă că playerul poate urca PRIN platforme (wall tiles) pe coloana
// de scară. Fix: verificăm scara în 3 rânduri consecutive în jurul playerului,
// inclusiv rândul de deasupra (pentru "climb-through-platform").
static bool on_ladder(const Player *p, const Map *m) {
    int cx  = (int)((p->px + TILE_PX * 0.5f) / TILE_PX);
    int ry1 = (int)(p->py / TILE_PX);
    int ry2 = (int)((p->py + TILE_PX - 1) / TILE_PX);
    // Rândul de deasupra: dacă playerul apasă Up și există scară deasupra,
    // continuă să urce chiar dacă tile-ul curent e platformă (wall)
    return map_ladder(m, cx, ry1)   ||
           map_ladder(m, cx, ry2)   ||
           map_ladder(m, cx, ry1-1) ||  // scară imediat deasupra → climb-through
           map_ladder(m, cx, ry2+1);    // scară imediat dedesubt → intrare scară
}

// ── API ───────────────────────────────────────────────────────────────────────

// GAME_INIT (004000) → spawnul playerului la coord din entity record
void player_init(Player *p, int spawn_col, int spawn_row) {
    *p = (Player){
        .px         = (float)(spawn_col * TILE_PX),
        .py         = (float)(spawn_row * TILE_PX),
        .vy         = 0.0f,
        .on_ladder  = false,
        .dead       = false,
        .anim_ctr   = 0,
        .anim_frame = 0,
    };
}

// PLAYER_MOVE_STEP (012740) + PLAYER_STATE_CHECK (012570)
// Returnează PR_* pentru ca game_tick() să reacționeze.
PlayerResult player_update(Player *p, const Map *m, Input in, float dt) {
    // ── mișcare orizontală ──────────────────────────────────────────────────
    // PLAYER_MOVE_STEP: verifică dacă tile-ul din direcția de mers e solid
    float nx  = p->px + (float)(in.right - in.left) * PLAYER_SPEED * dt;
    int   ckx = (int)((nx + (in.right ? TILE_PX - 1 : 0)) / TILE_PX);
    int   midy = (int)((p->py + TILE_PX * 0.5f) / TILE_PX);
    if (!map_solid(m, ckx, midy)) p->px = nx;

    // ── mișcare verticală: scară vs gravitație ───────────────────────────────
    // PLAYER_MOVE_STEP: dacă tile curent = LADDER → control vertical direct
    p->on_ladder = on_ladder(p, m);
    if (p->on_ladder) {
        p->vy  = 0.0f;
        p->py += (float)(in.down - in.up) * PLAYER_SPEED * dt;
    } else {
        // Gravitație: ADD la viteza verticală, oprire la WALL de jos
        p->vy += PLAYER_GRAVITY * dt;
        float ny  = p->py + p->vy * dt;
        int   ftx = (int)((p->px + TILE_PX * 0.5f) / TILE_PX);
        int   fty = (int)((ny + TILE_PX) / TILE_PX);
        if (map_solid(m, ftx, fty)) {
            p->vy = 0.0f;
            ny    = (float)(fty * TILE_PX) - (float)TILE_PX;
        }
        p->py = ny;
    }
    // Clamp la limitele nivelului
    if (p->py < 0.0f)                           { p->py = 0.0f;  p->vy = 0.0f; }
    if (p->py > (MAP_ROWS - 1) * (float)TILE_PX) p->py = (MAP_ROWS - 1) * (float)TILE_PX;

    // ── animație (ANIM_THROTTLE_PLAYER 007432) ───────────────────────────────
    // CMP #3, @#PLAYER_ANIM_CTR: la al 4-lea tick → schimbă frame, resetează
    bool moving = in.left || in.right || (p->on_ladder && (in.up || in.down));
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
