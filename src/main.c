// main.c — platform glue + main loop. The ONLY file that differs by target.
// C23. Native: blocking loop. Web: browser-driven via emscripten.
#include "raylib.h"
#include "game.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// raylib upscales our fixed VWxVH target to this window, nearest-neighbour.
enum { SCALE = 3, WIN_W = VW * SCALE, WIN_H = VH * SCALE };

static void frame(void) {
    game_frame(GetFrameTime());
}

int main(void) {
    InitWindow(WIN_W, WIN_H, "Klad-Reimagined (working title)");
    InitAudioDevice();
    game_init();

#if defined(PLATFORM_WEB)
    // Browser owns the event loop; never block. fps=0 -> use requestAnimationFrame.
    emscripten_set_main_loop(frame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) frame();
    game_shutdown();
    CloseAudioDevice();
    CloseWindow();
#endif
    return 0;
}
