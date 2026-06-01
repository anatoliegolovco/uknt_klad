// uknc_emu.c — headless KM1801VM2 (PDP-11) emulator for tracing КЛАД 1987 logic.
//
// Purpose: run the real target binary assets/uknc/KLAD_1987_Baranov.SAV with TOTAL
// programmatic control — no GUI, no mouse, no keyboard injection. We call any routine,
// read/write any cell or register, and diff memory before/after an action. This is the
// LOGIC ground truth (what the code computes); the Qt УКНЦ emulator stays the VISUAL
// ground truth (pixels). BK-0010 and УКНЦ differ as MACHINES (video, PPU) but share the
// KM1801VM2 instruction set, so for CPU+RAM logic this faithful core is enough.
//
// Instruction set: full PDP-11 integer set + EIS (MUL/DIV/ASH/ASHC) + XOR, all addressing
// modes, byte/word, branch group, SOB, JSR/RTS/JMP/SWAB/MARK, condition codes. I/O page
// (>=0177600) reads 0 / ignores writes; the УКНЦ video/keyboard ports (0176640, 040546)
// sit below that and behave as plain RAM here (harmless for logic).
//
// Build:  cc -std=c23 -O2 -Wall -Wextra tools/uknc_emu.c -o /tmp/uknc_emu
// Use:    /tmp/uknc_emu selftest                 # GAME_INIT must set lives = 0o333
//         /tmp/uknc_emu call <octal_addr> [max]  # call routine, print changed words
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── machine state ─────────────────────────────────────────────────────────────
#define MEMSZ   0200000u          // 64 KiB, octal
#define IOPAGE  0177600u          // >= this → I/O page (reads 0, writes ignored)
#define LOAD    01000u            // program load address (after 512-byte RT-11 header)
#define SENTINEL 04u              // return address we plant so we can detect routine exit

typedef struct {
    uint8_t  mem[MEMSZ];
    uint16_t r[8];
    int N, Z, V, C;
    bool trapped;
    char trapmsg[64];
    // УКНЦ video model (from emubase): 176640=plane address, 176642→plane1, 176643→plane2.
    uint8_t  plane1[MEMSZ], plane2[MEMSZ];
    uint16_t port176640;
    int      keycode;   // injected key (0=none); read via 040546 (bit7=ready, 6-0=code)
} CPU;

// ── memory access ─────────────────────────────────────────────────────────────
// УКНЦ video ports (below the I/O page, so intercept explicitly).
static bool vid_write(CPU *c, uint16_t a, uint8_t v){
    switch(a){
        case 0176640: c->port176640 = (c->port176640 & 0xff00) | v; return true;
        case 0176641: c->port176640 = (c->port176640 & 0x00ff) | (v<<8); return true;
        case 0176642: c->plane1[c->port176640] = v; return true;   // plane 1 data
        case 0176643: c->plane2[c->port176640] = v; return true;   // plane 2 data
    }
    return false;
}
static uint16_t rdw(CPU *c, uint16_t a){ a&=0177776;
    if(a==0176640) return c->port176640;                                  // plane address
    if(a==0176642) return (uint16_t)(c->plane1[c->port176640] | (c->plane2[c->port176640]<<8));
    if(a>=IOPAGE) return 0;
    return (uint16_t)(c->mem[a] | (c->mem[a+1]<<8)); }
static void wrw(CPU *c, uint16_t a, uint16_t v){ a&=0177776; if(a>=IOPAGE) return;
    if(a==0176640){ c->port176640=v; return; }
    if(a==0176642){ c->plane1[c->port176640]=v&0xff; c->plane2[c->port176640]=v>>8; return; }
    c->mem[a]=v&0xff; c->mem[a+1]=v>>8; }
static uint8_t rdb(CPU *c, uint16_t a){
    if(a==0176640) return c->port176640&0xff;
    if(a==0176641) return c->port176640>>8;
    if(a==0176642) return c->plane1[c->port176640];
    if(a==0176643) return c->plane2[c->port176640];
    if(a==0040546) return c->keycode ? (uint8_t)(0200 | (c->keycode & 0177)) : 0;  // kbd status
    if(a==0176674) return 0200;   // display status: bit7=ready (unstick the vsync poll)
    return a>=IOPAGE ? 0 : c->mem[a]; }
static void wrb(CPU *c, uint16_t a, uint8_t v){ if(vid_write(c,a,v)) return; if(a<IOPAGE) c->mem[a]=v; }
static uint16_t fetch(CPU *c){ uint16_t w=rdw(c,c->r[7]); c->r[7]+=2; return w; }

static void trap(CPU *c, const char *what, uint16_t op, uint16_t pc){
    c->trapped=true; snprintf(c->trapmsg,sizeof c->trapmsg,"%s %06o @ %06o",what,op,pc); }

// ── operand decode (effective address) ────────────────────────────────────────
// is_reg=true → operand is register `reg`; else operand is memory at `addr`.
typedef struct { bool is_reg; int reg; uint16_t addr; } Opnd;

static Opnd ea(CPU *c, int mode, int reg, bool byte){
    int d = (byte && reg<6) ? 1 : 2;            // autoinc/dec step (reg 6,7 always 2)
    Opnd o = {0};
    switch(mode){
        case 0: o.is_reg=true; o.reg=reg; return o;
        case 1: o.addr=c->r[reg]; return o;
        case 2: o.addr=c->r[reg]; c->r[reg]+=(reg>=6)?2:d; return o;          // (Rn)+
        case 3: o.addr=rdw(c,c->r[reg]); c->r[reg]+=2; return o;              // @(Rn)+
        case 4: c->r[reg]-=(reg>=6)?2:d; o.addr=c->r[reg]; return o;          // -(Rn)
        case 5: c->r[reg]-=2; o.addr=rdw(c,c->r[reg]); return o;              // @-(Rn)
        case 6: { uint16_t x=fetch(c); o.addr=c->r[reg]+x; return o; }        // X(Rn)
        default:{ uint16_t x=fetch(c); o.addr=rdw(c,(uint16_t)(c->r[reg]+x)); return o; } // @X(Rn)
    }
}
static uint16_t ld(CPU *c, Opnd o, bool byte){
    if(o.is_reg) return c->r[o.reg] & (byte?0xff:0xffff);
    return byte ? rdb(c,o.addr) : rdw(c,o.addr);
}
static void st(CPU *c, Opnd o, uint16_t v, bool byte){
    if(o.is_reg){ if(byte) c->r[o.reg]=(c->r[o.reg]&0xff00)|(v&0xff); else c->r[o.reg]=v; }
    else { if(byte) wrb(c,o.addr,v&0xff); else wrw(c,o.addr,v); }
}
static void setnz(CPU *c, uint16_t v, bool byte){
    uint16_t m=byte?0x80:0x8000, k=byte?0xff:0xffff;
    c->Z=((v&k)==0); c->N=((v&m)!=0);
}

// ── one instruction ───────────────────────────────────────────────────────────
static void step(CPU *c){
    uint16_t pc=c->r[7];
    uint16_t op=fetch(c);
    uint16_t b=op&0177400;

    // branch group (offset is signed byte × 2)
    int off=(int8_t)(op&0xff);
    #define BR(cond) do{ if(cond) c->r[7]+=off*2; return; }while(0)
    switch(b){
        case 0000400: BR(true);
        case 0001000: BR(c->Z==0);
        case 0001400: BR(c->Z==1);
        case 0002000: BR(c->N==c->V);
        case 0002400: BR(c->N!=c->V);
        case 0003000: BR(c->Z==0 && c->N==c->V);
        case 0003400: BR(c->Z==1 || c->N!=c->V);
        case 0100000: BR(c->N==0);
        case 0100400: BR(c->N==1);
        case 0101000: BR(c->C==0 && c->Z==0);
        case 0101400: BR(c->C==1 || c->Z==1);
        case 0102000: BR(c->V==0);
        case 0102400: BR(c->V==1);
        case 0103000: BR(c->C==0);
        case 0103400: BR(c->C==1);
    }

    // SOB Rn,offset
    if((op&0177000)==0077000){
        int reg=(op>>6)&7; c->r[reg]--;
        if(c->r[reg]) c->r[7]-=(op&077)*2;
        return;
    }
    // JSR reg,dst
    if((op&0177000)==0004000){
        int reg=(op>>6)&7; Opnd o=ea(c,(op>>3)&7,op&7,false);
        c->r[6]-=2; wrw(c,c->r[6],c->r[reg]);
        c->r[reg]=c->r[7]; c->r[7]=o.addr; return;
    }
    // RTS reg
    if((op&0177770)==0000200){
        int reg=op&7; c->r[7]=c->r[reg]; c->r[reg]=rdw(c,c->r[6]); c->r[6]+=2; return;
    }
    // JMP dst
    if((op&0177700)==0000100){
        Opnd o=ea(c,(op>>3)&7,op&7,false); c->r[7]=o.addr; return;
    }
    // SWAB dst
    if((op&0177700)==0000300){
        Opnd o=ea(c,(op>>3)&7,op&7,false); uint16_t v=ld(c,o,false);
        uint16_t r=(uint16_t)((v<<8)|(v>>8)); st(c,o,r,false);
        setnz(c,r&0xff,true); c->V=0; c->C=0; return;
    }
    // MARK n  (0064nn) — used by some return sequences
    if((op&0177700)==0006400){
        c->r[6]=c->r[7]+(op&077)*2; c->r[7]=c->r[5]; c->r[5]=rdw(c,c->r[6]); c->r[6]+=2; return;
    }

    // no-operand / condition-code ops
    if(op==0){ trap(c,"HALT",op,pc); return; }
    if(op==1||op==2||op==3||op==4||op==5) return;          // WAIT/RTI/BPT/IOT/RESET → nop
    if(op>=0104000 && op<=0104777){                        // EMT/TRAP (ФОДОС syscall)
        if(op==0104006) c->r[0]=c->keycode;                // EMT 6 = char input → injected key
        return;
    }
    if(op>=0240 && op<=0277){                               // condition-code set/clear
        int bits=op&017, setf=(op&020)!=0;
        if(bits&1) c->C=setf;
        if(bits&2) c->V=setf;
        if(bits&4) c->Z=setf;
        if(bits&8) c->N=setf;
        return;
    }

    // single-operand group (word 0050xx-0067xx; byte +0100000)
    uint16_t key=op&0007700; bool bflag=(op&0100000)!=0; uint16_t top=op&0170000;
    if(top==0 || top==0100000){
        const char *nm=NULL;
        switch(key){
            case 0005000: nm="CLR"; break; case 0005100: nm="COM"; break;
            case 0005200: nm="INC"; break; case 0005300: nm="DEC"; break;
            case 0005400: nm="NEG"; break; case 0005500: nm="ADC"; break;
            case 0005600: nm="SBC"; break; case 0005700: nm="TST"; break;
            case 0006000: nm="ROR"; break; case 0006100: nm="ROL"; break;
            case 0006200: nm="ASR"; break; case 0006300: nm="ASL"; break;
            case 0006700: nm="SXT"; break;
        }
        if(nm){
            bool byte = bflag && key!=0006700;        // SXT has no byte form
            Opnd o=ea(c,(op>>3)&7,op&7,byte); uint16_t v=ld(c,o,byte);
            uint16_t mask=byte?0xff:0xffff, m=byte?0x80:0x8000;
            if(!strcmp(nm,"CLR")){ st(c,o,0,byte); c->Z=1;c->N=0;c->V=0;c->C=0; return; }
            if(!strcmp(nm,"TST")){ setnz(c,v,byte); c->V=0;c->C=0; return; }
            if(!strcmp(nm,"INC")){ uint16_t r=(v+1)&mask; c->V=((v&mask)==(byte?0x7f:0x7fff)); setnz(c,r,byte); st(c,o,r,byte); return; }
            if(!strcmp(nm,"DEC")){ uint16_t r=(v-1)&mask; c->V=((v&mask)==m); setnz(c,r,byte); st(c,o,r,byte); return; }
            if(!strcmp(nm,"NEG")){ uint16_t r=(-v)&mask; setnz(c,r,byte); c->C=(r!=0); c->V=(r==m); st(c,o,r,byte); return; }
            if(!strcmp(nm,"COM")){ uint16_t r=(~v)&mask; setnz(c,r,byte); c->C=1;c->V=0; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ROL")){ int cc=(v&m)!=0; uint16_t r=((v<<1)|c->C)&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ROR")){ int cc=v&1; uint16_t r=((v>>1)|(c->C?m:0))&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ASL")){ int cc=(v&m)!=0; uint16_t r=(v<<1)&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ASR")){ int cc=v&1; uint16_t r=((v>>1)|(v&m))&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"SXT")){ uint16_t r=c->N?mask:0; c->Z=!c->N; st(c,o,r,false); return; }
            if(!strcmp(nm,"ADC")){ int a=c->C; uint16_t r=(v+a)&mask; c->C=(v==mask&&a); c->V=((v&mask)==(byte?0x7f:0x7fff)&&a); setnz(c,r,byte); st(c,o,r,byte); return; }
            if(!strcmp(nm,"SBC")){ int a=c->C; uint16_t r=(v-a)&mask; c->C=(v==0&&a); c->V=((v&mask)==m&&a); setnz(c,r,byte); st(c,o,r,byte); return; }
        }
    }

    // EIS + XOR group (op bits 15-9 = 0070xx..0074xx)
    if((op&0177000)==0070000){ // MUL Rn,src
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int32_t a=(int16_t)c->r[reg], bb=(int16_t)ld(c,s,false); int32_t p=a*bb;
        c->r[reg]=(p>>16)&0xffff; c->r[reg|1]=p&0xffff;
        c->N=(p<0); c->Z=(p==0); c->V=0; c->C=(p< -32768 || p>32767); return;
    }
    if((op&0177000)==0071000){ // DIV Rn,src
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int32_t dv=(int16_t)ld(c,s,false);
        int32_t num=(int32_t)((c->r[reg]<<16)|c->r[reg|1]);
        if(dv==0){ c->V=1; c->C=1; return; }
        int32_t q=num/dv, rem=num%dv;
        if(q>32767||q< -32768){ c->V=1; return; }
        c->r[reg]=q&0xffff; c->r[reg|1]=rem&0xffff; c->N=(q<0); c->Z=(q==0); c->V=0;c->C=0; return;
    }
    if((op&0177000)==0072000){ // ASH Rn,src (shift R by signed count)
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int sh=ld(c,s,false)&077; if(sh&040) sh-=64;
        int32_t v=(int16_t)c->r[reg], r;
        if(sh>=0) r=v<<sh; else r=v>>(-sh);
        c->r[reg]=r&0xffff; setnz(c,c->r[reg],false); c->V=0; return;
    }
    if((op&0177000)==0073000){ // ASHC Rn,src (shift R:R|1 by signed count)
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int sh=ld(c,s,false)&077; if(sh&040) sh-=64;
        int64_t v=(int32_t)((c->r[reg]<<16)|c->r[reg|1]), r;
        if(sh>=0) r=v<<sh; else r=v>>(-sh);
        c->r[reg]=(r>>16)&0xffff; c->r[reg|1]=r&0xffff;
        c->N=(r<0); c->Z=(r==0); c->V=0; return;
    }
    if((op&0177000)==0074000){ // XOR Rn,dst
        int reg=(op>>6)&7; Opnd d=ea(c,(op>>3)&7,op&7,false);
        uint16_t r=ld(c,d,false)^c->r[reg]; st(c,d,r,false); setnz(c,r,false); c->V=0; return;
    }

    // double-operand
    int dop=op>>12;
    if((dop>=1&&dop<=6) || (dop>=011&&dop<=016)){
        bool byte=dop>=010; int o=dop&7;
        Opnd se=ea(c,(op>>9)&7,(op>>6)&7,byte); uint16_t s=ld(c,se,byte);
        uint16_t mask=byte?0xff:0xffff, m=byte?0x80:0x8000;
        if(o==1){ // MOV/MOVB
            if(byte){ Opnd de=ea(c,(op>>3)&7,op&7,true);
                if(de.is_reg) c->r[de.reg]=(s&0x80)?(0xff00|s):(s&0xff); // MOVB to reg sign-extends
                else wrb(c,de.addr,s);
                setnz(c,s,true); c->V=0; return; }
            Opnd de=ea(c,(op>>3)&7,op&7,false); st(c,de,s,false); setnz(c,s,false); c->V=0; return;
        }
        Opnd de=ea(c,(op>>3)&7,op&7,byte); uint16_t d=ld(c,de,byte); uint16_t r;
        switch(o){
            case 2: r=(s-d)&mask; setnz(c,r,byte); c->C=((s&mask)<(d&mask)); c->V=(((s^d)&(s^r)&m)!=0); return; // CMP
            case 3: r=d&s; setnz(c,r,byte); c->V=0; return;                                                     // BIT
            case 4: r=d&(~s&mask); st(c,de,r,byte); setnz(c,r,byte); c->V=0; return;                            // BIC
            case 5: r=d|s; st(c,de,r,byte); setnz(c,r,byte); c->V=0; return;                                    // BIS
            case 6: if(byte){ r=(d-s)&mask; c->C=((d&mask)<(s&mask)); c->V=(((d^s)&(d^r)&m)!=0); }              // SUB(byte)
                    else { uint32_t sum=d+s; r=sum&mask; c->C=(sum>0xffff); c->V=((~(d^s)&(d^r)&m)!=0); }       // ADD(word)
                    setnz(c,r,byte); st(c,de,r,byte); return;
        }
    }
    trap(c,"illegal op",op,pc);
}

// ── harness ───────────────────────────────────────────────────────────────────
static CPU C;

static int load_sav(const char *path){
    FILE *f=fopen(path,"rb"); if(!f){ perror(path); return -1; }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,512,SEEK_SET);   // skip RT-11 header
    long body=n-512; if(body<=0||LOAD+body>MEMSZ){ fclose(f); return -1; }
    size_t got=fread(C.mem+LOAD,1,body,f); fclose(f);
    if((long)got!=body) return -1;
    C.r[6]=0160000;
    return (int)body;
}

// Call a subroutine: plant SENTINEL return addr, run until it RTSes back (or HALT/limit).
static long call(uint16_t addr, long maxins){
    C.trapped=false;
    C.r[6]-=2; wrw(&C,C.r[6],SENTINEL);
    C.r[7]=addr;
    long n=0;
    while(n<maxins){
        if(C.r[7]==SENTINEL) return n;
        step(&C); n++;
        if(C.trapped) return -n;       // negative = trapped (still ran |n| instr)
    }
    return n;                          // hit limit
}

// Dump plane 1 as a 640x288 PGM (80 bytes/scanline, stride 0o120, MSB=left pixel).
static void dump_plane(CPU *c, const char *path, uint16_t base){
    const int W=640, H=288;
    FILE *f=fopen(path,"wb"); if(!f) return;
    fprintf(f,"P5\n%d %d\n255\n",W,H);
    for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        uint16_t addr=(uint16_t)(base + y*0120 + (x>>3));
        uint8_t b=c->plane1[addr];
        uint8_t px=((b>>(7-(x&7)))&1)?255:0;
        fwrite(&px,1,1,f);
    }
    fclose(f);
    printf("plane1 -> %s (640x288 from base %06o)\n", path, base);
}

// Run from addr for up to maxins instructions (no sentinel; the game loops).
static long run_n(uint16_t addr, long maxins){
    C.r[7]=addr; C.trapped=false; long n=0;
    while(n<maxins){ step(&C); n++; if(C.trapped) return -n; }
    return n;
}

int main(int argc, char **argv){
    const char *sav="assets/uknc/KLAD_1987_Baranov.SAV";
    const char *cmd = argc>1 ? argv[1] : "selftest";

    if(!strcmp(cmd,"selftest")){
        if(load_sav(sav)<0) return 1;
        printf("lives @017436 before GAME_INIT: %03o\n", rdw(&C,017436));
        long n=call(04000, 5000000);
        printf("GAME_INIT -> %s after %ld instr%s\n",
               C.trapped?"TRAP":(n>=0?"returned/limit":"?"), n<0?-n:n,
               C.trapped?C.trapmsg:"");
        if(C.trapped) printf("  (trap detail: %s — expected: video-setup EIS, not logic)\n",C.trapmsg);
        uint16_t lives=rdw(&C,017436);
        printf("lives @017436 after GAME_INIT: %03o (%d)\n", lives, lives);
        bool ok=(lives==0333);
        printf("SELF-TEST %s — expected 0o333 (219)\n", ok?"PASS ✅":"FAIL ❌");
        return ok?0:1;
    }

    if(!strcmp(cmd,"blit") && argc>=3){  // call TILE_BLIT_REV for one tile → read real pixels
        if(load_sav(sav)<0) return 1;
        int tile=(int)strtol(argv[2],NULL,8);
        C.mem[0100]=(uint8_t)tile;       // tile index byte
        C.r[2]=0100;                     // R2 -> tile index
        C.r[1]=0;                        // R1 = plane position (base added inside)
        long n=call(014302, 200000);     // TILE_BLIT_REV
        printf("blit tile %o: %s after %ld instr%s\n", tile,
               C.trapped?"TRAP":"done", n<0?-n:n, C.trapped?C.trapmsg:"");
        // dump a small region around the blit (base 0o106210, first rows)
        const int W=16, H=8;
        printf("rendered pixels (plane1, 16x8 at base):\n");
        for(int y=0;y<H;y++){ printf("  ");
            for(int x=0;x<W;x++){
                uint16_t addr=(uint16_t)(0106210 + y*0120 + (x>>3));
                printf("%c", ((C.plane1[addr]>>(7-(x&7)))&1)?'#':'.');
            }
            printf("\n");
        }
        return 0;
    }

    if(!strcmp(cmd,"game")){             // drive title→speed→gameplay, dump plane1 with player
        if(load_sav(sav)<0) return 1;
        C.r[7]=01000; C.trapped=false;
        long lim=0;
        #define STEP_N(n) do{ for(long i=0;i<(n)&&!C.trapped;i++){ step(&C); lim++; } }while(0)
        C.keycode=015; STEP_N(2000000);     // boot → title, advance on Enter (0o15)
        C.keycode=061; STEP_N(800000);      // speed select '1' → start game (level+player)
        C.keycode=0;   STEP_N(500000);      // no input → player at rest, frame rendered
        long nz=0; int lo=-1,hi=-1;
        for(int a=0;a<MEMSZ;a++) if(C.plane1[a]){ nz++; if(lo<0)lo=a; hi=a; }
        printf("drove %ld instr, PC=%06o, plane1 nz=%ld range %06o..%06o%s\n",
               lim, C.r[7], nz, lo<0?0:lo, hi<0?0:hi, C.trapped?C.trapmsg:"");
        // player entity @014420: [0]=type [2]=tile-cell ptr/X [6]=screen(plane) pos
        printf("player entity @014420: ");
        for(int w=0; w<=016; w+=2) printf("[%o]=%06o ", w, rdw(&C,(uint16_t)(014420+w)));
        printf("\n");
        uint16_t spos = rdw(&C, 014426);     // entity[6] = R1 = blit plane position
        uint16_t pbase = (uint16_t)(spos + 0106210);
        printf("player rendered at plane base %06o (entity[6]=%06o + 0106210):\n", pbase, spos);
        for(int y=0;y<8;y++){ printf("  ");
            for(int x=0;x<24;x++){
                uint16_t addr=(uint16_t)(pbase + y*0120 + (x>>3));
                printf("%c", ((C.plane1[addr]>>(7-(x&7)))&1)?'#':'.');
            }
            printf("\n");
        }
        dump_plane(&C, "/tmp/plane1_game.pgm", lo<0?0106210:(uint16_t)lo);
        // dump the collision work buffer (BUF_TILE_WORK 014550, 22*32 words) — the REAL
        // movement graph (per-cell direction flags built by COLLISION_MAP_BUILD).
        FILE *cf=fopen("/tmp/cmap.bin","wb");
        if(cf){ for(int i=0;i<22*32;i++){ uint16_t w=rdw(&C,(uint16_t)(014550+i*2));
                    fputc(w&0xff,cf); fputc(w>>8,cf); } fclose(cf);
                printf("collision buffer @014550 -> /tmp/cmap.bin (704 words)\n"); }
        uint16_t pp=rdw(&C,014422);   // VAR_PLAYER_TILE_PTR → player cell in the buffer
        int pidx=((int)pp-014550)/2;
        printf("player tile ptr @014422=%06o -> cell index %d (row %d, col %d if /32)\n",
               pp, pidx, pidx/32, pidx%32);
        // DOOR cells: 12716 (key-collect) rewrites cells @17424 / @17426.
        for (int k=0; k<2; k++) {
            uint16_t dp = rdw(&C, (uint16_t)(017424 + k*2));
            int di = ((int)dp - 014550)/2;
            uint16_t cellw = rdw(&C, dp);
            printf("door ptr @%o=%06o -> cell idx %d (row %d, col %d) tile=%d flags=%06o\n",
                   017424+k*2, dp, di, di/32, di%32, cellw&0xff, cellw&0177400);
        }
        return 0;
    }

    if(!strcmp(cmd,"video")){            // run real render code, dump plane1 to verify model
        if(load_sav(sav)<0) return 1;
        long n=run_n(01000, argc>=3?strtol(argv[2],NULL,10):3000000);
        printf("ran %ld instr from RESTART (%s)%s  final PC=%06o\n", n<0?-n:n,
               C.trapped?"TRAP":"limit", C.trapped?C.trapmsg:"", C.r[7]);
        // scan whole plane1 for content
        long nz=0; int lo=-1, hi=-1;
        for(int a=0;a<MEMSZ;a++) if(C.plane1[a]){ nz++; if(lo<0)lo=a; hi=a; }
        printf("plane1 non-zero bytes: %ld  range %06o..%06o\n", nz, lo<0?0:lo, hi<0?0:hi);
        uint16_t base = argc>=4 ? (uint16_t)strtol(argv[3],NULL,8) : (lo<0?0106210:(uint16_t)lo);
        dump_plane(&C, "/tmp/plane1.pgm", base);
        return 0;
    }

    if(!strcmp(cmd,"call") && argc>=3){
        if(load_sav(sav)<0) return 1;
        uint16_t addr=(uint16_t)strtol(argv[2],NULL,8);
        long maxins = argc>=4 ? strtol(argv[3],NULL,10) : 5000000;
        static uint8_t before[MEMSZ]; memcpy(before,C.mem,MEMSZ);
        long n=call(addr,maxins);
        printf("call %06o -> %s after %ld instr%s\n", addr,
               C.trapped?"TRAP":"done", n<0?-n:n, C.trapped?C.trapmsg:"");
        int changes=0;
        for(uint16_t a=0; a<IOPAGE; a+=2){
            uint16_t o=before[a]|(before[a+1]<<8), nw=rdw(&C,a);
            if(o!=nw){ if(changes<60) printf("  %06o: %06o -> %06o\n",a,o,nw); changes++; }
        }
        printf("%d word(s) changed%s\n", changes, changes>60?" (first 60 shown)":"");
        return 0;
    }

    if(!strcmp(cmd,"psc")){   // probe PLAYER_STATE_CHECK (012570): what does each tile DO?
        if(load_sav(sav)<0) return 1;
        C.r[7]=01000; C.trapped=false;
        #define STEPN(n) do{ for(long i=0;i<(n)&&!C.trapped;i++){ step(&C); } }while(0)
        C.keycode=015; STEPN(2000000);   // → title
        C.keycode=061; STEPN(800000);    // speed '1' → start level 1
        C.keycode=0;   STEPN(500000);    // settle
        printf("== baseline: CUR_MAP@1300=%06o LEVEL_TBL@1304=%06o GAMESTATE@17430=%06o "
               "lives@17436=%03o playerstate@14420=%06o\n",
               rdw(&C,01300), rdw(&C,01304), rdw(&C,017430), rdw(&C,017436), rdw(&C,014420));
        uint16_t scratch = 014550 + (10*32+10)*2;   // interior cell row10 col10
        for(int tile=0; tile<=14; tile++){
            // snapshot
            uint16_t m0=rdw(&C,01300), t0=rdw(&C,01304), st0=rdw(&C,014420);
            uint16_t d1=rdw(&C,017424), d2=rdw(&C,017426);
            // place tile in scratch cell, point player tile ptr at it
            C.mem[scratch]=(uint8_t)tile; C.mem[scratch+1]=0;
            wrw(&C,014422,scratch);
            C.r[7]=0; long n=call(012570, 300000);   // PLAYER_STATE_CHECK
            uint16_t m1=rdw(&C,01300), t1=rdw(&C,01304), st1=rdw(&C,014420);
            uint16_t cellnow=C.mem[scratch];
            uint16_t nd1=rdw(&C,017424), nd2=rdw(&C,017426);
            printf("tile %2d (0o%02o): %s | cell %2d->%2d | pstate %06o->%06o | "
                   "MAP %s LVLTBL %s | door1@%06o %s door2@%06o %s\n",
                   tile, tile, C.trapped?"TRAP":"ok", tile, cellnow, st0, st1,
                   m0==m1?"=":"CHANGED!", t0==t1?"=":"CHANGED!",
                   d1, d1!=nd1?"chg":"-", d2, d2!=nd2?"chg":"-");
            (void)n; (void)st0;
            C.trapped=false;
        }
        return 0;
    }

    if(!strcmp(cmd,"advance")){   // does key 0o55 advance the level? (CUR_MAP 1300 / spawn ptr 17430)
        if(load_sav(sav)<0) return 1;
        C.r[7]=01000; C.trapped=false;
        #define STP(n) do{ for(long i=0;i<(n)&&!C.trapped;i++){ step(&C); } }while(0)
        C.keycode=015; STP(2000000);   // → title
        C.keycode=061; STP(800000);    // speed '1' → level 1
        C.keycode=0;   STP(500000);    // settle
        printf("before: CUR_MAP@1300=%06o spawnptr@17430=%06o player_ptr@14422=%06o\n",
               rdw(&C,01300), rdw(&C,017430), rdw(&C,014422));
        C.keycode=055; STP(1500000);   // inject key 0o55 (the suspected advance key)
        C.keycode=0;   STP(500000);
        printf("after 0o55: CUR_MAP@1300=%06o spawnptr@17430=%06o player_ptr@14422=%06o%s\n",
               rdw(&C,01300), rdw(&C,017430), rdw(&C,014422), C.trapped?C.trapmsg:"");
        return 0;
    }

    fprintf(stderr,"usage: %s selftest|game|video|blit <t>|call <addr>|psc|advance\n", argv[0]);
    return 2;
}
