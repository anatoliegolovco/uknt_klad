// enemy.h — AI inamici
// Sursa ASM: ENEMY2_TICK (006552), ENEMY3_TICK (007462),
//            SPRITE_ANIM_A/B/C/D, ENEMY_RESPAWN (012716),
//            LEVEL_END_CHECK (001636)
#pragma once
#include "klad.h"
#include "map.h"
#include "player.h"

// ENEMY2_TICK (006552): CMP #0o400, @#TICK_CTR; BNE → INC; → MOVE
// 0o400 = 256 ticks de joc între mișcări.
// La УКНЦ ~8MHz cu DELAY_SPIN=1000 → ~133 ticks/s → 256/133 ≈ 1.9s/mișcare
#define ENEMY_MOVE_INTERVAL  0.5f   // secunde (apropriat pentru display modern)

// SPRITE_ANIM_A/B (010102/010142): "8-frame horiz, 5-frame vert"
#define ENEMY_ANIM_HORIZ  8
#define ENEMY_ANIM_VERT   5
#define ENEMY_ANIM_DT     0.12f  // secunde/frame animație

typedef struct {
    int   col, row;      // poziție tile curentă
    bool  active;        // EREC_STATE: 0=inactiv, 010=activ
    int   dir;           // direcție orizontală: -1 stânga, +1 dreapta
    bool  falling;       // în cădere liberă (nu se agață de scări cât cade — ca jucătorul)
    float move_cd;       // cooldown până la mișcare (TICK_CTR throttle)
    float anim_t;        // timer animație continuă
    int   anim_horiz;    // frame orizontal curent (0-7)
    int   anim_vert;     // frame vertical curent (0-4)
} Enemy;

// ENEMY_RESPAWN (012716): MOVB #011, (R5) → stare activă
void enemy_init(Enemy *e, int col, int row);

// ENEMY2_TICK / ENEMY3_TICK: throttle + ENEMY_MOVE
// Returnează true dacă a lovit playerul (LEVEL_END_CHECK 001636)
bool enemy_tick(Enemy *e, const Map *m, const Player *p, float dt);
