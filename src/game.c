// game.c — game loop principal
// Sursa ASM: GAME_INIT (004000), GAME_TICK (001602), GAME_LOOP (001344),
//            ACT_DISPATCH (001436), KBD_GAME_POLL (004674)
#include "game.h"
#include "level_data.h"
#include <string.h>
#include <stdlib.h>

static void start_game(Game *g);   // fwd

// ── input (KBD_GAME_POLL 004674) ─────────────────────────────────────────────
// Originalul: polling non-blocking de taste УКНЦ, coduri mapate via KEY_CODE_TBL
// Reimplementare: raylib keyboard + touch (identic pe Linux și WASM)
static Input read_input(void) {
    Input in = {0};
    in.left   = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.right  = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in.up     = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in.down   = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    in.action = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);
    if (GetTouchPointCount() > 0) {
        Vector2 t = GetTouchPosition(0);
        float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
        if      (t.x < w / 3)       in.left  = true;
        else if (t.x > 2.0f * w/3)  in.right = true;
        if      (t.y < h / 4)       in.up    = true;
        else if (t.y > 3.0f * h/4)  in.down  = true;
    }
    return in;
}

// KI-15 (design B balance): în binar unii inamici apar FIX pe celula de ieșire și o
// camp-uiesc de la start (originalul nu folosea ieșirea, deci nu conta). Pentru mecanica
// noastră cheie→ușă→ieșire îi mutăm pe poziții NEUTRALE (departe de ieșire/cheie/spawn,
// calculate cu BFS). Restul inamicilor rămân ca în binar. Vezi docs/reverse/AI_PASSABILITY.md.
static bool enemy_spawn_override(int lvl, int idx, int *col, int *row) {
    switch (lvl) {                                   // lvl 0-indexat; idx 0=enemy1, 1=enemy2
        case 5: if (idx==0){ *col=28; *row=20; return true; } break;   // L6: e1 era (8,0) lângă exit(7,0)
        case 7: if (idx==0){ *col=29; *row=20; return true; }          // L8: e1 era (15,0) PE exit
                if (idx==1){ *col=22; *row=10; return true; } break;   // L8: e2 era (16,0) lângă exit
        case 8: if (idx==0){ *col=27; *row=1;  return true; } break;   // L9: e1 era (9,8) PE exit
    }
    return false;
}

// ── load nivel ───────────────────────────────────────────────────────────────
// LEVEL_COMPLETE (001034) + COLLISION_MAP_BUILD (013524)
static void load_level(Game *g) {
    int lvl = g->score.level;
    map_load(&g->map, lvl);

    // Spawn player din LEVEL_SPAWNS (derivat din entity records ASM)
    player_init(&g->player,
                LEVEL_SPAWNS[lvl][0][0],
                LEVEL_SPAWNS[lvl][0][1]);

    // Spawn inamici (entity records la 014430, 014440), cu override anti-camp pe ieșire
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int ec = LEVEL_SPAWNS[lvl][i+1][0], er = LEVEL_SPAWNS[lvl][i+1][1];
        enemy_spawn_override(lvl, i, &ec, &er);
        enemy_init(&g->enemies[i], ec, er);
    }
}

// ── GAME_INIT (004000) ────────────────────────────────────────────────────────
// Fluxul original: HW_INIT → TITLE_SEQ (titlu) → DIFF_SELECT (viteză) → joc.
void game_init(Game *g) {
    render_init(&g->renderer);
    score_init(&g->score);
    g->speed       = 2;          // viteză implicită (DIFF_SELECT, 1-4)
    g->state       = GS_TITLE;   // pornim pe ecranul de titlu, ca originalul
    g->state_timer = 0.0f;
    load_level(g);               // pregătim nivelul 1 (afișat după alegerea vitezei)

    // hook de test (capturi side-by-side): KLAD_START=play|speed sare la ecranul cerut
    const char *start = getenv("KLAD_START");
    if (start) {
        if      (!strcmp(start, "play"))  start_game(g);
        else if (!strcmp(start, "speed")) g->state = GS_SPEED_SELECT;
    }
}

// Start efectiv al jocului după DIFF_SELECT: GAME_INIT (lives 0o333, scor 0) + nivel 1.
static void start_game(Game *g) {
    score_init(&g->score);       // MOV #0333,@#LIVES ; CLR @#SCORE
    g->score.level = 0;
    load_level(g);
    g->state = GS_PLAYING;
}

// ── GAME_TICK (001602) ────────────────────────────────────────────────────────
// Secvența exactă din ASM:
//   JSR DELAY_SPIN
//   JSR PLAYER_STATE_CHECK  ← player_update()
//   JSR ENEMY1_TICK         ← enemy_tick() × MAX_ENEMIES
//   JSR ENEMY2_TICK
//   JSR ENEMY3_TICK
//   JSR ENTITY1_RESTORE
//   JSR WATER_COLLISION     ← inclus în player_update()
//   CMP @#PLAYER_TILE, @#ENEMY1_TILE → level end check
//   CMP @#PLAYER_TILE, @#ENEMY2_TILE → level end check
static void game_tick(Game *g, Input in, float dt) {
    // PLAYER_STATE_CHECK + PLAYER_MOVE_STEP + WATER_COLLISION
    PlayerResult pr = player_update(&g->player, &g->map, in, dt);

    switch (pr) {
        case PR_GOLD:
            map_clear(&g->map,
                (int)((g->player.px + TILE_W*0.5f) / TILE_W),
                (int)((g->player.py + TILE_H*0.5f) / TILE_H));
            score_add_gold(&g->score);
            break;
        case PR_BONUS:
            map_clear(&g->map,
                (int)((g->player.px + TILE_W*0.5f) / TILE_W),
                (int)((g->player.py + TILE_H*0.5f) / TILE_H));
            score_add_life(&g->score);
            break;
        case PR_LEVEL_WIN:   // KI-10: gold_c = CHEIA → DESCHIDE UȘA (NU avansează nivelul!)
            map_clear(&g->map,
                (int)((g->player.px + TILE_W*0.5f) / TILE_W),
                (int)((g->player.py + TILE_H*0.5f) / TILE_H));
            map_open_door(&g->map);     // ușa (tile 10) devine pasabilă; sunet (TODO audio)
            score_add_gold(&g->score);  // cheia dă și puncte
            break;                      // continuă jocul — exit-ul (după ușă) termină nivelul
        case PR_EXIT:        // tile 2 = IEȘIREA (atinsă după ușă + scara din dreapta) → final
            if (score_level_advance(&g->score))
                g->state = GS_ALL_WIN;
            else {
                g->state       = GS_LEVEL_WIN;
                g->state_timer = 1.2f;
            }
            return;
        case PR_WATER:
        case PR_ENEMY:
            g->player.dead = true;
            if (score_lose_life(&g->score)) g->state = GS_GAME_OVER;
            else { g->state = GS_DEAD; g->state_timer = 1.5f; }
            return;
        case PR_NONE:
            break;
    }

    // ENEMY1_TICK / ENEMY2_TICK / ENEMY3_TICK + LEVEL_END_CHECK (001636)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemy_tick(&g->enemies[i], &g->map, &g->player, dt)) {
            // CMP @#PLAYER_TILE, @#ENEMY_TILE → egal → GAME_OVER sequence
            g->player.dead = true;
            if (score_lose_life(&g->score)) g->state = GS_GAME_OVER;
            else { g->state = GS_DEAD; g->state_timer = 1.5f; }
            return;
        }
    }
}

// ── game_frame — entry point per-cadru (FN_GAME_LOOP 001344) ─────────────────
void game_frame(Game *g, float dt) {
    if (dt > 0.1f) dt = 0.1f;  // cap la 100ms (fereastră în background)

    Input in = read_input();

    // Comutare paletă color/mono (tasta M) — emulator УКНЦ vs monitor monocrom
    if (IsKeyPressed(KEY_M)) render_toggle_mono();

    // State machine joc
    switch (g->state) {
        case GS_TITLE:
            // TITLE_WAIT (002264): orice tastă → ecranul de alegere a vitezei
            if (in.action) g->state = GS_SPEED_SELECT;
            break;

        case GS_SPEED_SELECT:
            // KEY_DIFFICULTY (001142): tastele 1-4 aleg viteza, apoi pornește jocul
            if      (IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) { g->speed = 1; start_game(g); }
            else if (IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) { g->speed = 2; start_game(g); }
            else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) { g->speed = 3; start_game(g); }
            else if (IsKeyPressed(KEY_FOUR)  || IsKeyPressed(KEY_KP_4)) { g->speed = 4; start_game(g); }
            break;

        case GS_PLAYING:
            game_tick(g, in, dt);
            break;

        case GS_DEAD:
        case GS_LEVEL_WIN:
            g->state_timer -= dt;
            if (g->state_timer <= 0.0f) {
                // LEVEL_RESET (010206): reload nivel, păstrează scor + vieți
                if (g->state == GS_LEVEL_WIN) {
                    // scorul a fost deja avansat în game_tick()
                }
                load_level(g);
                g->state = GS_PLAYING;
            }
            break;

        case GS_GAME_OVER:
        case GS_ALL_WIN:
            // GAME_OVER_WAIT (003274): orice tastă → înapoi la ecranul de titlu
            if (in.action) g->state = GS_TITLE;
            break;
    }

    // ── render ───────────────────────────────────────────────────────────────
    BeginTextureMode(g->renderer.target);
    ClearBackground(render_bg());

    if (g->state == GS_TITLE) {
        render_title(&g->renderer);
    } else if (g->state == GS_SPEED_SELECT) {
        render_speed_select(&g->renderer, g->speed);
    } else {
        render_map(&g->renderer, &g->map);
        for (int i = 0; i < MAX_ENEMIES; i++)
            render_enemy(&g->renderer, &g->enemies[i]);
        render_player(&g->renderer, &g->player, g->state, g->state_timer);
        render_hud(&g->renderer, &g->score);

        // Overlay mesaje (centrat în zona de joc, sub HUD)
        int mx = VW/2, my = PLAYFIELD_Y + MAP_ROWS*TILE_H/2;
        Color cw = render_fg();
        if (g->state == GS_LEVEL_WIN)
            render_text(&g->renderer, "Уровень пройден", mx-58, my-6, 12, cw);
        if (g->state == GS_GAME_OVER) {
            render_text(&g->renderer, "Игра окончена", mx-52, my-10, 12, cw);
            render_text(&g->renderer, "нажмите клавишу", mx-56, my+6, 10, cw);
        }
        if (g->state == GS_ALL_WIN) {
            render_text(&g->renderer, "Поздравляем!", mx-48, my-10, 12, cw);
            render_text(&g->renderer, "нажмите клавишу", mx-56, my+6, 10, cw);
        }
    }

    EndTextureMode();
    render_present(&g->renderer);
}

void game_shutdown(Game *g) {
    render_shutdown(&g->renderer);
}
