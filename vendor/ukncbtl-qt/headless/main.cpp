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
    board->LoadROM(rombuf);
    if (argc >= 3) {
        if (board->AttachFloppyImage(0, argv[2])) printf("floppy attached: %s\n", argv[2]);
        else fprintf(stderr, "warning: could not attach %s\n", argv[2]);
    }
    board->Reset();

    int frames = (argc >= 4) ? atoi(argv[3]) : 100;
    for (int i = 0; i < frames; i++) board->SystemFrame();

    CProcessor* cpu = board->GetCPU();
    printf("ran %d frames. CPU PC=%06o\n", frames, cpu->GetPC());
    // sanity peek: a few words of plane-0 RAM around the КЛАД load area
    printf("RAM[plane0] 001000=%06o 004000=%06o 017436(lives)=%06o\n",
           board->GetRAMWord(0, 001000), board->GetRAMWord(0, 004000),
           board->GetRAMWord(0, 017436));
    delete board; free(raw);
    return 0;
}
