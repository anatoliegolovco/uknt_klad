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
    // Match the canvas DRAWING BUFFER to its CSS layout box (clientWidth/Height — ignores the
    // portrait rotation transform) and hand that size to render_present, which sets the GL
    // viewport/projection itself. (raylib's own web viewport stays at the init size and
    // SetWindowSize / synthetic 'resize' don't fix it, so the scene rendered into the
    // bottom-left corner. Driving the viewport from render_present fixes it deterministically
    // on every size + orientation.)
    int w = emscripten_run_script_int("document.getElementById('canvas').clientWidth");
    int h = emscripten_run_script_int("document.getElementById('canvas').clientHeight");
    if (w > 0 && h > 0) {
        render_web_w = w; render_web_h = h;
        emscripten_run_script(
            "var c=document.getElementById('canvas');"
            "if(c.width!=c.clientWidth||c.height!=c.clientHeight){c.width=c.clientWidth;c.height=c.clientHeight;}");
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
