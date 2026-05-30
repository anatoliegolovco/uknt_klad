// main.c — entry point: Linux native + WebAssembly
// Singurul fișier cu #ifdef PLATFORM_WEB.
// Sursa ASM: FN_HW_INIT (005754) → init hardware + JMP TITLE_SEQ
//            FN_GAME_LOOP (001344) → entry per-frame
#include "raylib.h"
#include "klad.h"
#include "game.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
static Game g_game;
static void web_frame(void) { game_frame(&g_game, GetFrameTime()); }
#endif

int main(void) {
    // FN_HW_INIT (005754): init hardware, apoi TITLE_SEQ
    // Echivalent: inițializăm fereastra și contextul grafic
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(VW * 3, VH * 3, "КЛАД (1987 Баранов)");
    SetTargetFPS(60);

    Game game;
    game_init(&game);

#ifdef PLATFORM_WEB
    // FN_GAME_LOOP (001344): JMP @#KBD_GAME_POLL — main per-frame entry
    g_game = game;
    emscripten_set_main_loop(web_frame, 0, 1);
#else
    // Bucla nativă — FN_GAME_LOOP echivalent
    while (!WindowShouldClose())
        game_frame(&game, GetFrameTime());
#endif

    game_shutdown(&game);
    CloseWindow();
    return 0;
}
