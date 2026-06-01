// i18n.h — tabel de șiruri. RU = fidel originalului (implicit), RO = versiunea română.
// Comutare la runtime cu tasta L (ecranul de titlu). Vezi decizia D5.
#pragma once

typedef enum { LANG_RU = 0, LANG_RO = 1, LANG_COUNT } Lang;

typedef enum {
    STR_SCORE,        // eticheta scor (HUD)
    STR_LIVES,        // eticheta vieți/încercări (HUD)
    STR_SPEED,        // titlu ecran viteză
    STR_SPEED_HINT,   // hint rapid/lent
    STR_LEVEL_CLEAR,  // nivel terminat
    STR_GAME_OVER,    // joc terminat
    STR_PRESS_KEY,    // apasă o tastă
    STR_CONGRATS,     // felicitări (toate nivelele)
    STR_LANG_HINT,    // hint comutare limbă (titlu)
    STR_COUNT
} StrId;

// RU = text rusesc fidel (ca în binarul УКНЦ). RO = română (ASCII-fold, fără diacritice, ca să
// se randeze cu fontul de rezervă raylib — fontul УКНЦ are doar chirilice).
static const char *const STRINGS[LANG_COUNT][STR_COUNT] = {
    [LANG_RU] = {
        [STR_SCORE]       = "Счет",
        [STR_LIVES]       = "Попытки",
        [STR_SPEED]       = "Скорость",
        [STR_SPEED_HINT]  = "1 - быстро   4 - медленно",
        [STR_LEVEL_CLEAR] = "Уровень пройден",
        [STR_GAME_OVER]   = "Игра окончена",
        [STR_PRESS_KEY]   = "нажмите клавишу",
        [STR_CONGRATS]    = "Поздравляем!",
        [STR_LANG_HINT]   = "L - Limba: Rus",
    },
    [LANG_RO] = {
        [STR_SCORE]       = "Scor",
        [STR_LIVES]       = "Incercari",
        [STR_SPEED]       = "Viteza",
        [STR_SPEED_HINT]  = "1 - rapid   4 - lent",
        [STR_LEVEL_CLEAR] = "Nivel trecut",
        [STR_GAME_OVER]   = "Joc terminat",
        [STR_PRESS_KEY]   = "apasa o tasta",
        [STR_CONGRATS]    = "Felicitari!",
        [STR_LANG_HINT]   = "L - Limba: Rom",
    },
};

extern Lang g_lang;                       // definit în render.c
#define T(id) (STRINGS[g_lang][id])
