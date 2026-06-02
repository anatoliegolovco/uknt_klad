// main.c — entry point: Linux native + WebAssembly
// Singurul fișier cu #ifdef PLATFORM_WEB.
// Sursa ASM: FN_HW_INIT (005754) → init hardware + JMP TITLE_SEQ
//            FN_GAME_LOOP (001344) → entry per-frame
#include "raylib.h"
#include "klad.h"
#include "game.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
static Game g_game;
static void web_frame(void) {
    // raylib fixes the web framebuffer at init and never updates it, so any later display
    // change (window resize, mobile URL-bar show/hide, orientation flip, our portrait
    // rotation) leaves a STALE buffer → the scene renders into the wrong-shaped buffer and
    // looks squashed / clipped / off-centre. Re-sync raylib's size to the canvas's live CSS
    // box every frame so render_present always centres into the size actually on screen.
    // Use the canvas LAYOUT box (clientWidth/Height), which ignores CSS transforms — in
    // portrait we rotate the canvas 90°, and the scene must render into the pre-rotation
    // box (e.g. 844×390) and then be rotated, NOT into the post-transform 390×844 box.
    int w = emscripten_run_script_int("document.getElementById('canvas').clientWidth");
    int h = emscripten_run_script_int("document.getElementById('canvas').clientHeight");
    if (w > 0 && h > 0 && (w != GetScreenWidth() || h != GetScreenHeight()))
        SetWindowSize(w, h);
    game_frame(&g_game, GetFrameTime());
}
#endif

int main(void) {
    // FN_HW_INIT (005754): init hardware, apoi TITLE_SEQ
    // Echivalent: inițializăm fereastra și contextul grafic
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(768, 576, "КЛАД (1987 Баранов)");
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
