// game.h — structura și API-ul jocului
// Sursa ASM: GAME_INIT (004000), GAME_TICK (001602), GAME_LOOP (001344)
#pragma once
#include "klad.h"
#include "map.h"
#include "player.h"
#include "enemy.h"
#include "score.h"
#include "render.h"

#define MAX_ENEMIES 2   // jocul original: enemy1 @ 014430, enemy2 @ 014440

typedef struct {
    Map      map;
    Player   player;
    Enemy    enemies[MAX_ENEMIES];
    Score    score;
    Renderer renderer;
    GameState state;
    float    state_timer;
} Game;

// GAME_INIT (004000): init complet de la zero
void game_init(Game *g);

// GAME_TICK (001602) + render: un cadru complet
void game_frame(Game *g, float dt);

// Eliberare resurse (native; WASM = tear-down automat)
void game_shutdown(Game *g);
