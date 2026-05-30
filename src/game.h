// game.h — public game API, platform-independent. C23.
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

// Fixed low-res render target (echoes the BK framebuffer feel). 8x8 tiles.
enum { VW = 256, VH = 192, TILE = 8, COLS = VW / TILE, ROWS = VH / TILE,
       LEVEL_ROWS = 22 };   // original УКНЦ level height; LEVEL_COLS = COLS

// Abstract input actions — keyboard and touch both map onto these.
typedef struct {
    bool left, right, up, down, action;
} Input;

// One-time setup (after the GL context exists).
void game_init(void);

// Advance one frame: read input, update simulation, draw.
// dt = seconds since last frame.
void game_frame(float dt);

// Release resources (native only; the browser tears the page down).
void game_shutdown(void);

#endif // GAME_H
