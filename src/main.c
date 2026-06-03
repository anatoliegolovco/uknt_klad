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
    // Keep the canvas DRAWING BUFFER matched to its CSS layout box (clientWidth/Height —
    // ignores the portrait rotation transform), and then fire a 'resize' event so raylib's
    // own emscripten resize handler re-runs: it updates the screen size AND the GL viewport.
    // (SetWindowSize alone left the viewport stale → render_present drew into the bottom-left
    // 768×576 corner of a larger buffer; a manual window resize fixed it because it triggers
    // exactly this handler. We trigger it ourselves on load / orientation / size change.)
    // Guard on the buffer being out of sync so we don't dispatch every frame.
    int stale = emscripten_run_script_int(
        "(function(){var c=document.getElementById('canvas');"
        "return (c.width!=c.clientWidth||c.height!=c.clientHeight)?1:0;})()");
    if (stale) {
        emscripten_run_script(
            "var c=document.getElementById('canvas');"
            "c.width=c.clientWidth; c.height=c.clientHeight;"
            "window.dispatchEvent(new Event('resize'));");
    }
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
