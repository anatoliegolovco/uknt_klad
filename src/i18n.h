// i18n.h — UI string localisation. C23. Default language: Romanian.
//
// Add a key to enum StrId, then a row to STRINGS[] with one entry per Lang.
// Keep entries in the same order. Romanian (LANG_RO) is the default.
#ifndef I18N_H
#define I18N_H

typedef enum { LANG_RO = 0, LANG_EN, LANG_COUNT } Lang;

typedef enum {
    STR_TITLE = 0,
    STR_PRESS_START,
    STR_SCORE,
    STR_LEVEL,
    STR_LIVES,
    STR_GAME_OVER,
    STR_LEVEL_CLEAR,
    STR_DROWNED,
    STR_PAUSED,
    STR_ID_COUNT
} StrId;

// [StrId][Lang]. Romanian first (default), English second.
static const char *const STRINGS[STR_ID_COUNT][LANG_COUNT] = {
    [STR_TITLE]        = { "COMOARA",            "TREASURE" },
    [STR_PRESS_START]  = { "Apasa START",        "Press START" },
    [STR_SCORE]        = { "Scor",               "Score" },
    [STR_LEVEL]        = { "Nivel",              "Level" },
    [STR_LIVES]        = { "Vieti",              "Lives" },
    [STR_GAME_OVER]    = { "Joc terminat",       "Game over" },
    [STR_LEVEL_CLEAR]  = { "Nivel terminat!",    "Level clear!" },
    [STR_DROWNED]      = { "Te-ai inecat!",      "You drowned!" },
    [STR_PAUSED]       = { "Pauza",              "Paused" },
};

static Lang g_lang = LANG_RO;

static inline void i18n_set(Lang l) { if (l < LANG_COUNT) g_lang = l; }

// Localised lookup. ASCII-only on purpose: raylib's default font has no
// diacritics, so we use plain forms (Vieti, Pauza). Swap in a custom font with
// ă/î/ș/ț later and restore the diacritics here.
static inline const char *T(StrId id) {
    if (id < 0 || id >= STR_ID_COUNT) return "?";
    const char *s = STRINGS[id][g_lang];
    return s ? s : STRINGS[id][LANG_RO];
}

#endif // I18N_H
