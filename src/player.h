// player.h — stare și logică jucător
// Sursa ASM: PLAYER_STATE_CHECK (012570), PLAYER_MOVE_STEP (012740),
//            ANIM_THROTTLE_PLAYER (007432), PLAYER_DEATH (001016)
#pragma once
#include "klad.h"
#include "map.h"

// ── constante din ASM ────────────────────────────────────────────────────────

// GAME_INIT (004000): MOV #0333, @#LIVES — 0o333 = 219, afișat ca "9"
// Reimplementare folosește direct 9 (reprezentare umană echivalentă)
#define PLAYER_LIVES_INIT  9

// SCORE_ADD (003764): ADD #012, @#SCORE — octal 012 = decimal 10
#define SCORE_PER_GOLD     10

// ANIM_THROTTLE_PLAYER (007432): CMP #3, @#PLAYER_ANIM_CTR → 4 stări (0,1,2,3)
// La fiecare 4 frame-uri de joc se schimbă frame-ul de animație
#define PLAYER_ANIM_STATES 4

// Viteza playerului — din PLAYER_MOVE_STEP: mișcare tile cu tile în original
// Reimplementare: pixel-level pentru fluiditate pe display modern
#define PLAYER_SPEED   64.0f   // px/s orizontal
#define PLAYER_GRAVITY 320.0f  // px/s² gravitație

// ── rezultatele verificării stării tile (PLAYER_STATE_CHECK 012570) ───────────
// CMPB #4,(R3) → gold A; CMPB #5 → gold B/bonus; CMPB #6 → level complete
typedef enum {
    PR_NONE,
    PR_GOLD,       // TIDX_GOLD_A (4): CLRB + SCORE_ADD
    PR_BONUS,      // TIDX_GOLD_B (5): CLRB + BONUS_LIFE_ADD
    PR_LEVEL_WIN,  // TIDX_GOLD_C (6): LEVEL_COMPLETE
    PR_EXIT,       // TIDX_EXIT   (2): ieșire nivel
    PR_WATER,      // apă adâncă (13,14): PLAYER_DEATH (tile 7 = apă mică, pasabilă, ne-letală)
    PR_ENEMY,      // coliziune inamic: LEVEL_END_CHECK (001636)
} PlayerResult;

// ── stare animație, din SPRITE_HELPERS (013216) ───────────────────────────────
// ASM: state ∈ {0o21..0o24}. 0o21 = climb (cînd entity[+0o16]==0o12 = pe scară),
//      0o23 = walk (altfel). Direcția (stînga/dreapta) vine din action code.
// Transpunem în enum-ul de mai jos.
typedef enum {
    PA_STAND,   // nemișcat
    PA_WALK,    // mers orizontal (state 0o23)
    PA_CLIMB,   // urcat scară (state 0o21)
} PlayerAnim;

// ── structura playerului ──────────────────────────────────────────────────────
typedef struct {
    float      px, py;     // poziție pixel (continuă)
    float      vy;         // viteză verticală (gravitație)
    bool       on_ladder;  // detectat din tile curent
    bool       dead;       // flag moarte curentă
    int        facing;     // direcție: -1 stînga, +1 dreapta (pentru flip sprite)
    PlayerAnim anim;       // starea de animație (SPRITE_HELPERS state)
    int        anim_ctr;   // ANIM_THROTTLE_PLAYER: contor 0..PLAYER_ANIM_STATES-1
    int        anim_frame; // frame curent în ciclul de mers (0,1)
} Player;

// ── API ───────────────────────────────────────────────────────────────────────
void         player_init(Player *p, int spawn_col, int spawn_row);
PlayerResult player_update(Player *p, const Map *m, Input in, float dt);
