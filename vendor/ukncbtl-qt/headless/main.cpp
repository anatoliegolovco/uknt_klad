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
#include <vector>
#include <algorithm>
#include <utility>

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
    for (int r=0; r<22; r++){
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

// --- E2E: level-1 chest reachability (column-locked fall, verified KI-14 physics) ----
// Movement model (matches the verified mechanics): walls (tile>=9) block & support;
// ladders (1,8) are climbable & passable; fall is STRAIGHT DOWN (no mid-fall steering);
// gold (4,5,6) is collected on contact (incl. passing through during a fall). Water (7)
// is treated as PASSABLE here (generous — gives chests the best chance to be reachable).
enum { COLS=32, ROWS=22 };
static int g_tile[ROWS][COLS];
static bool g_rest[ROWS][COLS], g_seen[ROWS][COLS], g_passed[ROWS][COLS];
static bool solid(int t){ return t>=9; }                  // wall / brick / border
static bool ladder(int t){ return t==1 || t==8; }
static bool passable(int t){ return !solid(t); }          // air, ladder, gold, exit, water
static bool rest_at(int r,int c){
    if (r>=ROWS-1) return true;                            // bottom floor
    int below=g_tile[r+1][c];
    return ladder(g_tile[r][c]) || solid(below) || ladder(below);
}
static int fall_to(int r,int c){                          // straight down until a rest cell
    while (r<ROWS-1 && !rest_at(r,c)){ g_passed[r][c]=true; r++; }
    g_passed[r][c]=true; return r;
}
static void reach_bfs(CMotherboard* b){
    for (int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++){
        g_tile[r][c]=cell_tile(b, TWORK+r*ROWB+c*2);
        g_rest[r][c]=g_seen[r][c]=g_passed[r][c]=false;
    }
    uint16_t sp=rdw(PLY_PTR); int sr=ptr_row(sp), sc=ptr_col(sp);
    sr=fall_to(sr,sc);                                     // spawn settles onto ground
    // BFS over rest cells
    static int qr[ROWS*COLS], qc[ROWS*COLS]; int head=0,tail=0;
    auto push=[&](int r,int c){ if(r>=0&&r<ROWS&&c>=0&&c<COLS&&!g_seen[r][c]){ g_seen[r][c]=true; g_passed[r][c]=true; qr[tail]=r; qc[tail]=c; tail++; } };
    push(sr,sc);
    while(head<tail){
        int r=qr[head],c=qc[head]; head++;
        // walk left / right, then fall
        for(int dc=-1; dc<=1; dc+=2){ int nc=c+dc;
            if(nc>=0&&nc<COLS && passable(g_tile[r][nc])){ g_passed[r][nc]=true; push(fall_to(r,nc),nc); } }
        // climb up
        if((ladder(g_tile[r][c])||(r>0&&ladder(g_tile[r-1][c]))) && r>0 && passable(g_tile[r-1][c])) push(r-1,c);
        // climb down
        if(r+1<ROWS && (ladder(g_tile[r+1][c])) && passable(g_tile[r+1][c])) push(r+1,c);
    }
    printf("=== level-1 reachability (spawn r%d c%d) ===\n", sr, sc);
    int goldTot=0, goldReach=0;
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++){
        int t=g_tile[r][c]; if(t==4||t==5||t==6){ goldTot++;
            bool got=g_passed[r][c];
            if(got) goldReach++;
            printf("  GOLD tile%d at r%d c%d : %s\n", t, r, c, got?"REACHABLE":"*** UNREACHABLE ***");
        }
    }
    printf("  -> %d/%d gold reachable\n", goldReach, goldTot);
}

// --- E2E: can the two LEFT chests (c4 r15, c4 r17) actually be collected? -----------
// Uses REAL physics: teleport the player to a plausible approach cell, inject real key
// presses, and check whether the gold tile clears (collected) and the player survives.
static void run_chest_test(CMotherboard* b){
    boot_klad(b); MC = b->GetCPUMemoryController();
    auto cell=[&](int r,int c){ return (uint16_t)(TWORK + r*ROWB + c*2); };
    printf("lives=%d  gold@(r15,c4)=%d  gold@(r17,c4)=%d\n",
           rdw(017436), cell_tile(b,cell(15,4)), cell_tile(b,cell(17,4)));

    // TEST A (natural): stand on the c4 ladder at r10, step RIGHT off it into c5, and let
    // the real fall carry the player down c5 (through shallow water @r13). Where do we land?
    printf("\n=== A: on c4 ladder (r10), step RIGHT into c5, natural fall ===\n");
    wrw(PLY_STATE,000010); wrw(PLY_PTR, cell(10,4)); run_frames(b,4);
    b->KeyboardEvent(K_RIGHT,true);
    for(int i=0;i<14;i++){ run_frames(b,4); uint16_t p=rdw(PLY_PTR);
        printf("  f%02d r%d c%d tile@=%d state=%06o lives=%d\n",
               i*4, ptr_row(p), ptr_col(p), cell_tile(b,p), rdw(PLY_STATE), rdw(017436)); }
    b->KeyboardEvent(K_RIGHT,false); run_frames(b,4);

    // TEST B: from the landing spot r15 c5, walk LEFT toward the chest at c4
    printf("\n=== B: from r15 c5, hold LEFT (reach chest c4?) ===\n");
    wrw(PLY_STATE,000010); wrw(PLY_PTR, cell(15,5)); run_frames(b,3);
    b->KeyboardEvent(K_LEFT,true);
    for(int i=0;i<10;i++){ run_frames(b,4); uint16_t p=rdw(PLY_PTR);
        printf("  f%02d r%d c%d  gold@c4r15=%d  score=%d\n",
               i*4, ptr_row(p), ptr_col(p), cell_tile(b,cell(15,4)), rdw(017434)); }
    b->KeyboardEvent(K_LEFT,false);
    printf("  -> gold@(r15,c4) now = %d  (0 = COLLECTED)\n", cell_tile(b,cell(15,4)));

    // TEST C: lower chest — from r17 c5, walk LEFT toward c4 r17
    printf("\n=== C: from r17 c5, hold LEFT (reach chest c4 r17?) ===\n");
    wrw(PLY_STATE,000010); wrw(PLY_PTR, cell(17,5)); run_frames(b,3);
    b->KeyboardEvent(K_LEFT,true);
    for(int i=0;i<8;i++){ run_frames(b,4); uint16_t p=rdw(PLY_PTR);
        printf("  f%02d r%d c%d  gold@c4r17=%d\n", i*4, ptr_row(p), ptr_col(p), cell_tile(b,cell(17,4))); }
    b->KeyboardEvent(K_LEFT,false);
    printf("  -> gold@(r17,c4) now = %d  (0 = COLLECTED)\n", cell_tile(b,cell(17,4)));
}

// --- E2E animation: capture the left-chest collection route as a frame sequence -----
static void run_chest_gif(CMotherboard* b){
    boot_klad(b); MC = b->GetCPUMemoryController();
    auto cell=[&](int r,int c){ return (uint16_t)(TWORK + r*ROWB + c*2); };
    char path[64]; int f=0;
    auto snap=[&](){ snprintf(path,sizeof path,"/tmp/cg_%03d.ppm", f++); shoot(path); };

    // Start on the col-4 ladder (a normal level feature), then play the route naturally.
    wrw(PLY_STATE,000010); wrw(PLY_PTR, cell(7,4)); run_frames(b,4); snap(); snap();
    // step RIGHT off the ladder into col 5, fall straight down (through shallow water) to row 15
    b->KeyboardEvent(K_RIGHT,true);
    for(int i=0;i<26;i++){ run_frames(b,2); snap(); if(ptr_row(rdw(PLY_PTR))>=15) break; }
    b->KeyboardEvent(K_RIGHT,false); run_frames(b,2); snap();
    // walk LEFT onto the chest at col 4 → collected
    b->KeyboardEvent(K_LEFT,true);
    for(int i=0;i<14;i++){ run_frames(b,2); snap(); if(cell_tile(b,cell(15,4))==0 && ptr_col(rdw(PLY_PTR))<=4) break; }
    b->KeyboardEvent(K_LEFT,false);
    for(int i=0;i<4;i++){ run_frames(b,3); snap(); }
    printf("captured %d frames; gold@(15,4) now=%d (0=collected)\n", f, cell_tile(b,cell(15,4)));
}

// --- Drive the REAL game by injecting movement keys (no teleport) ------------------
// argv[4] = move script: space-separated "<DIR><frames>" tokens, DIR in L/R/U/D/N(idle).
// e.g. "R30 U60 L8". Captures /tmp/cg_NNN.ppm every few frames and logs the player
// cell (row,col) after each token so the route can be tuned iteratively.
static void run_play(CMotherboard* b, const char* script){
    boot_klad(b); MC = b->GetCPUMemoryController();
    run_frames(b,12);                                   // let the spawn settle
    int f=0; char path[64];
    auto snap=[&](){ snprintf(path,sizeof path,"/tmp/cg_%03d.ppm", f++); shoot(path); };
    printf("spawn r%d c%d\n", ptr_row(rdw(PLY_PTR)), ptr_col(rdw(PLY_PTR)));
    snap();
    for (const char* p=script; *p; ){
        while(*p==' ') p++;
        if(!*p) break;
        char dir=*p++; int n=atoi(p); while(*p && *p!=' ') p++;
        uint8_t key = dir=='R'?K_RIGHT : dir=='L'?K_LEFT : dir=='U'?K_UP : dir=='D'?K_DOWN : 0;
        if(key) b->KeyboardEvent(key,true);
        for(int i=0;i<n;i++){ run_frames(b,2); if(i%2==0) snap(); }
        if(key) b->KeyboardEvent(key,false);
        run_frames(b,3); snap();
        uint16_t pp=rdw(PLY_PTR);
        printf("  %c%-3d -> r%d c%d tile@=%d  gold(15,4)=%d gold(17,4)=%d\n",
               dir, n, ptr_row(pp), ptr_col(pp), cell_tile(b,pp),
               cell_tile(b,(uint16_t)(TWORK+15*ROWB+4*2)), cell_tile(b,(uint16_t)(TWORK+17*ROWB+4*2)));
    }
    printf("done: %d frames, final r%d c%d\n", f, ptr_row(rdw(PLY_PTR)), ptr_col(rdw(PLY_PTR)));
}

// ===================================================================================
// LEVEL SOLVER: BFS to the tile-6 "key" (= LEVEL_COMPLETE), executed by real key
// injection, capturing frames. Loops level→level as each completes (CUR_MAP_ADDR 001300).
enum { CUR_MAP_ADDR=001300 };
static int   sv_t[ROWS][COLS];
static bool  sv_dw(int t){ return t==13||t==14; }              // deep water (lethal)
static bool  sv_wall(int t){ return t>=9 && !sv_dw(t); }       // wall/brick/border (stand/block)
static bool  sv_ladder(int t){ return t==1||t==8; }
static bool  sv_pass(int t){ return t<9; }                     // air/ladder/gold/exit/shallow
static bool  sv_rest(int r,int c){
    if(sv_ladder(sv_t[r][c])) return true;
    if(r>=ROWS-1) return false;
    int bb=sv_t[r+1][c];
    if(sv_dw(bb)) return false;                                // above deep water → fall in
    return sv_wall(bb) || sv_ladder(bb);
}
static int sv_fall(int r,int c){                               // straight down; -1 if drowns
    while(r<ROWS-1 && !sv_rest(r,c)){ if(sv_dw(sv_t[r][c])) return -1; r++; }
    return sv_dw(sv_t[r][c]) ? -1 : r;
}
static void sv_load(CMotherboard* b){
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) sv_t[r][c]=cell_tile(b,(uint16_t)(TWORK+r*ROWB+c*2));
}
// BFS → move string (L/R/U/D) from (sr,sc) to the tile-6 goal; "" if none.
static const char* sv_plan(int sr,int sc,int& gr,int& gc){
    static char par[ROWS][COLS], moves[2048]; static int pR[ROWS][COLS],pC[ROWS][COLS];
    static bool seen[ROWS][COLS];
    gr=gc=-1;
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++){ seen[r][c]=false; if(sv_t[r][c]==6){gr=r;gc=c;} }
    if(gr<0) return "";
    static int qr[ROWS*COLS],qc[ROWS*COLS]; int h=0,t=0;
    sr=sv_fall(sr,sc); if(sr<0) return "";
    seen[sr][sc]=true; qr[t]=sr;qc[t]=sc;t++;
    auto add=[&](int r,int c,int fr,int fc,char m){ if(r<0||r>=ROWS||c<0||c>=COLS||seen[r][c])return;
        seen[r][c]=true; par[r][c]=m; pR[r][c]=fr; pC[r][c]=fc; qr[t]=r;qc[t]=c;t++; };
    while(h<t){
        int r=qr[h],c=qc[h];h++;
        if(r==gr&&c==gc) break;
        for(int dc=-1;dc<=1;dc+=2){ int nc=c+dc; if(nc<0||nc>=COLS) continue;
            if(sv_pass(sv_t[r][nc])){ int nr=sv_fall(r,nc); if(nr>=0) add(nr,nc,r,c, dc<0?'L':'R'); } }
        if(r>0 && (sv_ladder(sv_t[r][c])||sv_ladder(sv_t[r-1][c])) && sv_pass(sv_t[r-1][c])) add(r-1,c,r,c,'U');
        if(r+1<ROWS && sv_ladder(sv_t[r+1][c]) && sv_pass(sv_t[r+1][c])) add(r+1,c,r,c,'D');
    }
    if(!seen[gr][gc]) return "";
    int rr=gr,cc=gc,n=0; static char tmp[2048];
    while(!(rr==sr&&cc==sc)){ tmp[n++]=par[rr][cc]; int a=pR[rr][cc],bb=pC[rr][cc]; rr=a;cc=bb; }
    for(int i=0;i<n;i++) moves[i]=tmp[n-1-i]; moves[n]=0;
    return moves;
}
// ---- emulator-guided BFS: uses the REAL emulator (save-states) as the physics+enemy
//      oracle. Snapshot each reached cell; from it try L/R/U/D (real injection, run to
//      settle); restore for the next try. Records the winning (key,chunks) path, then
//      replays it from the level start to capture frames. Deterministic (snapshots fix
//      enemy positions), so the replay reproduces the win.
static const uint8_t DIRK[4] = { K_LEFT, K_RIGHT, K_UP, K_DOWN };
static const char    DIRC[4] = { 'L','R','U','D' };
struct SNode{ int r,c,par; char mv; int pch,ich; uint8_t* img; };

// ONE-CELL move from the current state: hold key only until the player enters a new cell
// (pch chunks), then release and let any fall settle (ich chunks). 2 frames/chunk. This
// keeps walks to a single column so the search branches correctly. Sets died/adv.
static void sv_runmove(CMotherboard* b, uint8_t key, uint16_t map0, int lives0,
                       int& pch, int& ich, bool& died, bool& adv){
    died=adv=false; pch=ich=0;
    uint16_t start=rdw(PLY_PTR);
    b->KeyboardEvent(key,true);
    for(int i=0;i<24;i++){ run_frames(b,2); pch++;
        if((int)rdw(017436)<lives0){ died=true; break; }
        if(rdw(CUR_MAP_ADDR)!=map0){ adv=true; break; }
        if(rdw(PLY_PTR)!=start) break;
    }
    b->KeyboardEvent(key,false);
    if(died||adv) return;
    int last=-1, stuck=0;
    for(int i=0;i<30;i++){ run_frames(b,2); ich++;
        if((int)rdw(017436)<lives0){ died=true; break; }
        if(rdw(CUR_MAP_ADDR)!=map0){ adv=true; break; }
        int p=rdw(PLY_PTR); if(p==last){ if(++stuck>=4) break; } else stuck=0; last=p;
    }
}
static void run_solve(CMotherboard* b){
    const int IMG = UKNCIMAGE_SIZE;
    boot_klad(b); MC=b->GetCPUMemoryController();
    uint8_t* start = (uint8_t*)malloc(IMG); b->SaveToImage(start);   // level-1 start
    for(int lvl=1; lvl<=10; lvl++){
        uint16_t map0=rdw(CUR_MAP_ADDR); int lives0=rdw(017436);
        // locate the tile-6 goal cell
        int gr=-1,gc=-1; for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) if(cell_tile(b,(uint16_t)(TWORK+r*ROWB+c*2))==6){gr=r;gc=c;}
        printf("=== LEVEL %d: spawn r%d c%d, key(tile6) r%d c%d ===\n",
               lvl, ptr_row(rdw(PLY_PTR)), ptr_col(rdw(PLY_PTR)), gr, gc);
        // BFS over cells using the emulator
        std::vector<SNode> nd; static bool seen[ROWS][COLS]; memset(seen,0,sizeof seen);
        uint8_t* s0=(uint8_t*)malloc(IMG); memcpy(s0,start,IMG);
        nd.push_back({ptr_row(rdw(PLY_PTR)),ptr_col(rdw(PLY_PTR)),-1,0,0,0,s0});
        seen[nd[0].r][nd[0].c]=true;
        int found=-1; char foundMv=0; int foundP=0,foundI=0;
        for(int h=0; h<(int)nd.size() && found<0 && (int)nd.size()<800; h++){
            for(int d=0; d<4 && found<0; d++){
                b->LoadFromImage(nd[h].img);
                bool died,adv; int pch,ich; sv_runmove(b,DIRK[d],map0,lives0,pch,ich,died,adv);
                if(adv){ found=h; foundMv=DIRC[d]; foundP=pch; foundI=ich; break; }   // tile-6 collected
                if(died) continue;
                int r=ptr_row(rdw(PLY_PTR)), c=ptr_col(rdw(PLY_PTR));
                if(r<0||r>=ROWS||c<0||c>=COLS||seen[r][c]) continue;
                seen[r][c]=true;
                uint8_t* img=(uint8_t*)malloc(IMG); b->SaveToImage(img);
                nd.push_back({r,c,h,DIRC[d],pch,ich,img});
            }
        }
        if(found<0){ int minr=99,upper=0; for(auto& n:nd){ if(n.r<minr)minr=n.r; if(n.r<=9)upper++; }
            printf("  no route found (explored %d cells; min row reached=%d, cells in rows0-9=%d) — stop.\n",
                   (int)nd.size(), minr, upper);
            for(auto& n:nd) free(n.img); break; }
        // reconstruct (move, pch, ich) path: parents up to root, then the winning move
        struct Step{ char m; int p,i; };
        std::vector<Step> seq; seq.push_back({foundMv,foundP,foundI});
        for(int i=found; i>0; i=nd[i].par) seq.push_back({nd[i].mv,nd[i].pch,nd[i].ich});
        std::reverse(seq.begin(),seq.end());
        // replay from level start, capturing frames (deterministic → reproduces the win)
        b->LoadFromImage(start);
        int f=0; char path[80];
        auto snap=[&](){ snprintf(path,sizeof path,"/tmp/lvl%d_%03d.ppm", lvl, f++); shoot(path); };
        snap();
        for(auto& st : seq){
            uint8_t key = st.m=='L'?K_LEFT:st.m=='R'?K_RIGHT:st.m=='U'?K_UP:K_DOWN;
            b->KeyboardEvent(key,true);
            for(int i=0;i<st.p;i++){ run_frames(b,2); if(i%2==0) snap(); }
            b->KeyboardEvent(key,false);
            for(int i=0;i<st.i;i++){ run_frames(b,2); if(i%2==0) snap(); }
        }
        for(int i=0;i<8;i++){ run_frames(b,3); snap(); }
        bool done = rdw(CUR_MAP_ADDR)!=map0;
        printf("  level %d: %s — %d moves, %d frames captured\n", lvl, done?"SOLVED":"replay mismatch", (int)seq.size(), f);
        for(auto& n:nd) free(n.img);
        if(!done) break;
        b->SaveToImage(start);   // emulator is now at the next level's start → search from here
    }
    free(start);
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
    } else if (!strcmp(arg3, "solve")) {
        run_solve(board);
    } else if (!strcmp(arg3, "play")) {
        run_play(board, (argc>=5)?argv[4]:"");
    } else if (!strcmp(arg3, "chestgif")) {
        run_chest_gif(board);
    } else if (!strcmp(arg3, "chest")) {
        run_chest_test(board);
    } else if (!strcmp(arg3, "reach")) {    // E2E: level-1 chest reachability
        boot_klad(board); MC = board->GetCPUMemoryController();
        dump_map(board, rdw(PLY_PTR));
        reach_bfs(board);
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
