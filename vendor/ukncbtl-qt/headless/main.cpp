// headless/main.cpp — drive the FULL УКНЦ machine (UKNCBTL emubase core) with NO Qt,
// NO GUI, NO mouse/keyboard injection through X. Total programmatic control: load ROM +
// disk, run frames, inject key scancodes, read planar video RAM, dump the screen.
//
// This is the VISUAL ground truth, scriptable — complementary to tools/uknc_emu.c (our
// own C23 LOGIC emulator). See ../FORK.md for provenance (LGPL-3.0 fork of ukncbtl-qt).
//
// Build:  make -C vendor/ukncbtl-qt/headless
// Use:    ./uknc_headless <uknc_rom.bin> [disk.dsk] [frames]
#include "stdafx.h"          // pulls stdint/stdio/stdlib + the Qt-free Common.h shim
#include "emubase/Board.h"
#include "emubase/Processor.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// --- external symbols the emubase core references, stubbed for headless ---------
void DebugLog(const char*) {}
void DebugLogFormat(const char*, ...) {}
bool AssertFailedLine(const char* file, int line) {
    fprintf(stderr, "ASSERT failed %s:%d\n", file, line); return true;
}
// emubase disasm/debug helper (normally in the GUI's Common.cpp): 6-digit octal.
void PrintOctalValue(char* buffer, uint16_t value) {
    for (int p = 5; p >= 0; p--) { buffer[p] = char('0' + (value & 7)); value >>= 3; }
    buffer[6] = 0;
}

// --- screen rendering (screen.cpp, lifted from upstream) -------------------------
CMotherboard* g_pBoard = nullptr;       // the renderer reads the screen via this global
bool g_okEmulatorInitialized = false;
extern void Emulator_PrepareScreenRGB32(void* pImageBits, const quint32* colors);
extern const quint32 ScreenView_StandardRGBColors[];
#define SCR_W 640
#define SCR_H 288

// Write a 640x288 ARGB framebuffer as a binary PPM (P6, RGB).
static void write_ppm(const char* path, const uint32_t* argb) {
    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
    for (int i = 0; i < SCR_W * SCR_H; i++) {
        uint32_t c = argb[i];
        uint8_t rgb[3] = { (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("screenshot -> %s (%dx%d)\n", path, SCR_W, SCR_H);
}

static void shoot(const char* path){
    static uint32_t img[SCR_W * SCR_H];
    memset(img, 0, sizeof img);
    Emulator_PrepareScreenRGB32(img, ScreenView_StandardRGBColors);
    write_ppm(path, img);
}

// --- key injection (УКНЦ scancodes, octal) --------------------------------------
enum { K_1=0030, K_R=0074, K_K=0052, K_L=0056, K_A=0072, K_D=0057,
       K_SPACE=0113, K_ENTER=0153, K_ALF=0106 };
// Cyrillic letter scancodes (phonetic УКНЦ keyboard) for missing-glyph capture.
enum { K_cB=0076, K_cI=0073, K_cJ=0027, K_cZH=0137, K_cZ=0157, K_cSH=0036,
       K_cU=0051, K_cCH=0110, K_c8=0145 /*numpad 8*/ };
static void run_frames(CMotherboard* b, int n){ for (int i=0;i<n;i++) b->SystemFrame(); }
static void key(CMotherboard* b, uint8_t scan){          // press + release one key
    b->KeyboardEvent(scan, true);  run_frames(b, 5);
    b->KeyboardEvent(scan, false); run_frames(b, 5);
}
// Scripted boot of КЛАД: boot menu → disk → ФОДОС → "R KLAD" → start. Returns after load.
static void boot_klad(CMotherboard* b){
    run_frames(b, 150);                 // wait for ЗАГРУЗКА (boot) menu
    key(b, K_1); key(b, K_ENTER);       // 1 = disk, Enter → boot ФОДОС
    run_frames(b, 500);                 // ФОДОС loads to its prompt
    const uint8_t cmd[] = { K_R, K_SPACE, K_K, K_L, K_A, K_D };  // "R KLAD"
    for (uint8_t k : cmd) key(b, k);
    key(b, K_ENTER);                    // run KLAD.SAV
    run_frames(b, 900);                 // КЛАД loads + title screen appears
    key(b, K_ENTER);                    // dismiss title
    run_frames(b, 200);
    key(b, K_1);                        // speed select 1 → start the game
    run_frames(b, 600);                 // into the maze
}

static void shoot(const char* path);   // fwd

// Capture the player sprite by frame-diff: screenshot at spawn, move right, screenshot
// again. The pixels that change isolate the player figure from the static maze.
enum { K_RIGHT=0133, K_LEFT=0116, K_UP=0154, K_DOWN=0134 };
static void boot_sprite(CMotherboard* b){
    boot_klad(b);                       // into the maze at spawn
    shoot("/tmp/spr_rest.ppm");         // standing/at rest
    // walk right, capturing several animation phases (player moves between shots)
    b->KeyboardEvent(K_RIGHT, true);
    run_frames(b, 14); shoot("/tmp/spr_w1.ppm");
    run_frames(b, 12); shoot("/tmp/spr_w2.ppm");
    run_frames(b, 12); shoot("/tmp/spr_w3.ppm");
    b->KeyboardEvent(K_RIGHT, false);
    run_frames(b, 10);
    // try to climb: move up (if on/at a ladder)
    b->KeyboardEvent(K_UP, true);
    run_frames(b, 20); shoot("/tmp/spr_c1.ppm");
    run_frames(b, 16); shoot("/tmp/spr_c2.ppm");
    b->KeyboardEvent(K_UP, false);
    run_frames(b, 8);
    shoot("/tmp/spr_after.ppm");        // reference for diffing the climb shots
}

// Boot to the ФОДОС prompt (РУС mode) and type the missing Cyrillic letters with spaces,
// so they echo on screen and can be sliced into glyphs. Б И Й Ж З Ш У Ч 8.
static void boot_glyphs(CMotherboard* b){
    run_frames(b, 150);
    key(b, K_1); key(b, K_ENTER);       // boot from disk
    run_frames(b, 500);                 // ФОДОС prompt
    key(b, K_ALF);                      // toggle keyboard to РУС (Cyrillic) — was ЛАТ
    run_frames(b, 20);
    const uint8_t seq[] = { K_cB, K_SPACE, K_cI, K_SPACE, K_cJ, K_SPACE, K_cZH, K_SPACE,
                            K_cZ, K_SPACE, K_cSH, K_SPACE, K_cU, K_SPACE, K_cCH };
    for (uint8_t k : seq) key(b, k);
    run_frames(b, 60);
}

static uint8_t* load_file(const char* path, long* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return nullptr; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*)malloc(n);
    if (fread(buf, 1, n, f) != (size_t)n) { fclose(f); free(buf); return nullptr; }
    fclose(f); if (out_len) *out_len = n; return buf;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <uknc_rom.bin(32KB)> [disk.dsk] [frames]\n", argv[0]);
        return 2;
    }
    long romlen = 0;
    uint8_t* raw = load_file(argv[1], &romlen);
    if (!raw || romlen < 32256) { fprintf(stderr, "ROM must be >= 32256 bytes (got %ld)\n", romlen); return 1; }
    uint8_t rombuf[32768];                       // LoadROM wants a 32 KB buffer; ROM is 32256, zero-pad
    memset(rombuf, 0, sizeof rombuf);
    memcpy(rombuf, raw, romlen < 32768 ? romlen : 32768);

    CProcessor::Init();                          // build the static opcode dispatch table (once)
    CMotherboard* board = new CMotherboard();
    g_pBoard = board; g_okEmulatorInitialized = true;   // for the screen renderer
    board->LoadROM(rombuf);
    if (argc >= 3) {
        if (board->AttachFloppyImage(0, argv[2])) printf("floppy attached: %s\n", argv[2]);
        else fprintf(stderr, "warning: could not attach %s\n", argv[2]);
    }
    board->Reset();

    const char* arg3 = (argc >= 4) ? argv[3] : "100";
    const char* shot = (argc >= 5) ? argv[4] : nullptr;
    int frames = 0;
    if (!strcmp(arg3, "klad")) {        // scripted: boot the game via key injection
        boot_klad(board);
        printf("booted КЛАD (scripted key injection)\n");
    } else if (!strcmp(arg3, "glyphs")) {   // type missing Cyrillic letters at ФОДОС prompt
        boot_glyphs(board);
        printf("typed missing glyphs at ФОДОС prompt\n");
    } else if (!strcmp(arg3, "sprite")) {   // capture player sprite via frame-diff
        boot_sprite(board);
        printf("captured spr_a + spr_b for sprite diff\n");
    } else {
        frames = atoi(arg3);
        for (int i = 0; i < frames; i++) board->SystemFrame();
    }

    CProcessor* cpu = board->GetCPU();
    printf("ran %d frames. CPU PC=%06o\n", frames, cpu->GetPC());
    printf("RAM[plane0] 001000=%06o 004000=%06o 017436(lives)=%06o\n",
           board->GetRAMWord(0, 001000), board->GetRAMWord(0, 004000),
           board->GetRAMWord(0, 017436));
    if (shot) {
        static uint32_t img[SCR_W * SCR_H];
        memset(img, 0, sizeof img);
        Emulator_PrepareScreenRGB32(img, ScreenView_StandardRGBColors);
        write_ppm(shot, img);
    }
    delete board; free(raw);
    return 0;
}
