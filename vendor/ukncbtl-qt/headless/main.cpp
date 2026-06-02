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
#include "emubase/Memory.h"     // CMemoryController — CPU address-space read/write
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

// --- E2E: bridge collision hypothesis ------------------------------------------
// Player logic state lives in CPU RAM (plane 0):
//   014420 = player state word, 014422 = player cell pointer into BUF_TILE_WORK
//   014550 = BUF_TILE_WORK base, row stride 0100 bytes, 1 word/cell
//   word = (flags<<8 | tile_index); bridge/platform tile index = 8 (017650 "ladder2")
enum { TWORK=014550, PLY_STATE=014420, PLY_PTR=014422, ROWB=0100 };
// CPU address-space access (NOT GetRAMWord, which is a video plane). GetWordView is a
// side-effect-free debugger read; SetWord pokes CPU RAM.
static CMemoryController* MC = nullptr;
static uint16_t rdw(uint16_t a){ int t; return MC->GetWordView(a, false, false, &t); }
static void     wrw(uint16_t a, uint16_t w){ MC->SetWord(a, false, w); }
static int  cell_tile (CMotherboard* b, uint16_t p){ (void)b; return rdw(p) & 0xFF; }
static int  cell_flags(CMotherboard* b, uint16_t p){ (void)b; return (rdw(p)>>8) & 0xFF; }
static int  ptr_row(uint16_t p){ return (int)((p - TWORK) / ROWB); }
static int  ptr_col(uint16_t p){ return (int)(((p - TWORK) % ROWB) / 2); }

static void dump_map(CMotherboard* b, uint16_t plyptr){
    printf("=== tile-work map (tile index per cell; '@'=player, 'B'=bridge tile 8) ===\n");
    for (int r=0; r<16; r++){
        printf("r%02d ", r);
        for (int c=0; c<32; c++){
            uint16_t p = TWORK + r*ROWB + c*2;
            int t = cell_tile(b,p);
            char ch;
            if (p==plyptr) ch='@';
            else if (t==8) ch='B';
            else if (t==0) ch='.';
            else ch = (t<10)?('0'+t):('a'+t-10);
            putchar(ch);
        }
        putchar('\n');
    }
}

static void run_bridge_test(CMotherboard* b){
    boot_klad(b);
    MC = b->GetCPUMemoryController();
    printf("sanity: 001000=%06o 004000=%06o lives@017436=%06o (expect game code + 0o333=219)\n",
           rdw(001000), rdw(004000), rdw(017436));
    uint16_t p0 = rdw(PLY_PTR);
    printf("after boot: state=%06o playerPtr=%06o  row=%d col=%d  tile@player=%d\n",
           rdw(PLY_STATE), p0, ptr_row(p0), ptr_col(p0), cell_tile(b,p0));
    dump_map(b, p0);

    // Validate the observable: how does playerPtr move under RIGHT, then idle?
    printf("=== move RIGHT (log ptr every 4 frames) ===\n");
    b->KeyboardEvent(K_RIGHT, true);
    for (int i=0;i<8;i++){ run_frames(b,4); uint16_t p=rdw(PLY_PTR);
        printf("  f%02d ptr=%06o row=%d col=%d tile=%d\n", i*4, p, ptr_row(p), ptr_col(p), cell_tile(b,p)); }
    b->KeyboardEvent(K_RIGHT, false); run_frames(b,6);

    // --- controlled drop test: place the player N air-rows above a TARGET tile and
    //     let gravity act. Does it LAND on top (row stops at target-1) or pass THROUGH?
    auto drop_test = [&](int target, const char* name){
        // find a cell (r,c) of TARGET tile with >=3 air rows directly above it
        int br=-1, bc=-1, air=0;
        for (int c=2; c<30 && br<0; c++)
          for (int r=5; r<15; r++){
            if (cell_tile(b, TWORK+r*ROWB+c*2) != target) continue;
            int a=0; for (int k=1;k<=4 && r-k>=0;k++){ if (cell_tile(b,TWORK+(r-k)*ROWB+c*2)==0) a++; else break; }
            if (a>=3){ br=r; bc=c; air=a; break; }
          }
        if (br<0){ printf("[%s] no drop site found (TARGET tile %d with >=3 air above)\n", name, target); return; }
        int startRow = br - air;
        printf("\n=== DROP onto tile %d (%s): target cell r%d c%d, start r%d (gap %d) ===\n",
               target, name, br, bc, startRow, air);
        wrw(PLY_STATE, 000010);                                   // idle/alive
        wrw(PLY_PTR,  (uint16_t)(TWORK + startRow*ROWB + bc*2));   // teleport above target
        run_frames(b, 3);
        int prev=-1, stable=0;
        for (int i=0;i<24;i++){
            run_frames(b,3);
            uint16_t p=rdw(PLY_PTR); int row=ptr_row(p);
            printf("  f%02d row=%2d col=%2d tile@=%2d  tileBelow=%2d\n",
                   i*3, row, ptr_col(p), cell_tile(b,p), cell_tile(b,(uint16_t)(p+ROWB)));
            if (row==prev){ if(++stable>=3) break; } else stable=0;
            prev=row;
        }
        int finalRow = ptr_row(rdw(PLY_PTR));
        printf("  -> RESULT: bridge/target row=%d, player stopped at row=%d : %s\n",
               br, finalRow,
               finalRow < br ? "LANDED ON TOP (collision held)"
                             : "FELL THROUGH / past target (no collision)");
    };
    drop_test(8,  "tile8 = ladder/bridge (pod)");
    drop_test(12, "tile12 = brick platform");

    // The strongest case for the hypothesis: a SUSPENDED bridge = tile 8 with AIR below
    // it (nothing holding it up). List candidates, then drop onto the best one.
    printf("\n=== suspended-bridge candidates (tile8 with air directly below) ===\n");
    int sbr=-1, sbc=-1, sair=0;
    for (int c=2;c<30;c++) for(int r=4;r<15;r++){
        if (cell_tile(b,TWORK+r*ROWB+c*2)==8 && cell_tile(b,TWORK+(r+1)*ROWB+c*2)==0){
            int a=0; for(int k=1;k<=4 && r-k>=0;k++){ if(cell_tile(b,TWORK+(r-k)*ROWB+c*2)==0) a++; else break; }
            printf("  r%d c%d : air-above=%d, below=%d\n", r, c, a, cell_tile(b,TWORK+(r+1)*ROWB+c*2));
            if (a>sair){ sair=a; sbr=r; sbc=c; }
        }
    }
    if (sbr>=0 && sair>=2){
        int startRow=sbr-sair;
        printf("=== DROP onto SUSPENDED bridge r%d c%d (air below), start r%d ===\n", sbr, sbc, startRow);
        wrw(PLY_STATE,000010);
        wrw(PLY_PTR,(uint16_t)(TWORK + startRow*ROWB + sbc*2));
        run_frames(b,3);
        int prev=-1,stable=0;
        for(int i=0;i<24;i++){ run_frames(b,3); uint16_t p=rdw(PLY_PTR); int row=ptr_row(p);
            printf("  f%02d row=%2d tile@=%2d tileBelow=%2d\n", i*3,row,cell_tile(b,p),cell_tile(b,(uint16_t)(p+ROWB)));
            if(row==prev){ if(++stable>=3) break;} else stable=0; prev=row; }
        int fr=ptr_row(rdw(PLY_PTR));
        printf("  -> RESULT: suspended bridge row=%d, stopped at row=%d : %s\n", sbr, fr,
               fr<sbr ? "LANDED ON TOP" : (fr==sbr ? "STOPPED IN BRIDGE CELL" : "FELL THROUGH"));
    } else printf("  (no suspended tile8 with >=2 air above in level 1)\n");

    // --- stand test: place player directly ON TOP of a bridge tile (8), no input ---
    {
        int br=-1,bc=-1;
        for (int c=2;c<30&&br<0;c++) for(int r=4;r<15;r++)
            if (cell_tile(b,TWORK+r*ROWB+c*2)==8 && cell_tile(b,TWORK+(r-1)*ROWB+c*2)==0){ br=r;bc=c;break; }
        if (br>=0){
            printf("\n=== STAND on tile8 bridge: cell directly above r%d c%d, idle 40 frames ===\n", br-1, bc);
            wrw(PLY_STATE,000010);
            wrw(PLY_PTR,(uint16_t)(TWORK + (br-1)*ROWB + bc*2));
            int r0=ptr_row(rdw(PLY_PTR));
            run_frames(b,40);
            int r1=ptr_row(rdw(PLY_PTR));
            printf("  start row=%d after 40 frames row=%d : %s\n", r0, r1,
                   r1==r0 ? "SUPPORTED (no gravity while on bridge)" : "fell");
        }
    }
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
    } else if (!strcmp(arg3, "bridge")) {   // E2E: bridge collision hypothesis
        run_bridge_test(board);
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
