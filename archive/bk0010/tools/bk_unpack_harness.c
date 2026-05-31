/* Headless harness: run a BK .BIN through the real bk-emulator CPU core
 * (no SDL/ROM/devices) far enough for its self-unpacker to run, then dump
 * all 64K of RAM.  Memory is flat RAM for the whole space so every unpack
 * write succeeds. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"

static d_word M[32768];          /* flat 64K, word-addressed */
unsigned short last_branch;
flag_t key_pressed;
unsigned short tty_scroll;
unsigned scr_dirty;

#define IDX(a) (((a) & 0177776) >> 1)

int lc_word(c_addr addr, d_word *word){ *word = M[IDX(addr)]; return OK; }
int sc_word(c_addr addr, d_word word){ M[IDX(addr)] = word; return OK; }

int ll_byte(pdp_regs *p, d_word baddr, d_byte *byte){
    d_word w = M[IDX(baddr)];
    *byte = (baddr & 1) ? (w >> 8) & 0377 : w & 0377;
    return OK;
}
int sl_byte(pdp_regs *p, d_word laddr, d_byte byte){
    int i = IDX(laddr);
    if (laddr & 1) M[i] = (M[i] & 0x00ff) | (byte << 8);
    else           M[i] = (M[i] & 0xff00) | byte;
    return OK;
}
/* devices / extras possibly referenced by the core: harmless stubs */
int service(d_word v){ return OK; }
int scr_write(int a, c_addr b, d_word c){ return OK; }
int q_reset(void){ return OK; }
int rti(pdp_regs *p){ return OK; }
int rtt(pdp_regs *p){ return OK; }
flag_t bkmodel=0;
flag_t io_stop_happened=0;
int pending_interrupts=0;

int main(int argc, char **argv){
    const char *path = argv[1];
    unsigned entry = argc>2 ? strtoul(argv[2],0,8) : 01000;
    unsigned long maxins = argc>3 ? strtoul(argv[3],0,10) : 5000000UL;
    FILE *f = fopen(path,"rb"); if(!f){perror("open");return 1;}
    unsigned char hdr[4]; fread(hdr,1,4,f);
    unsigned load = hdr[0]|(hdr[1]<<8), len = hdr[2]|(hdr[3]<<8);
    unsigned char *body = malloc(len); fread(body,1,len,f); fclose(f);
    for(unsigned i=0;i<len;i++){               /* load body byte-wise */
        unsigned a=load+i; int idx=IDX(a);
        if(a&1) M[idx]=(M[idx]&0x00ff)|(body[i]<<8);
        else    M[idx]=(M[idx]&0xff00)|body[i];
    }
    pdp_regs P; memset(&P,0,sizeof P);
    P.regs[PC]=entry; P.regs[SP]=0160000; P.psw=0;
    unsigned long n=0; int result;
    unsigned oldpc=0;
    for(;n<maxins;n++){
        oldpc=P.regs[PC];
        if(lc_word(P.regs[PC], &P.ir)!=OK){ printf("fetch fail @%06o\n",P.regs[PC]); break; }
        P.regs[PC]=(P.regs[PC]+2)&0xffff;
        result=(itab[P.ir>>6].func)(&P);
        if(result!=OK){
            if(result==CPU_HALT){ printf("HALT @%06o after %lu\n",oldpc,n); break; }
            if(result==ODD_ADDRESS){ printf("ODD_ADDRESS ir=%06o @%06o after %lu\n",P.ir,oldpc,n); break; }
            if(result==CPU_ILLEGAL){ printf("ILLEGAL ir=%06o @%06o after %lu\n",P.ir,oldpc,n); break; }
            /* EMT/TRAP/IOT/WAIT/BPT/RTT: skip MONITOR call, keep running */
        }
    }
    if(n>=maxins) printf("limit %lu reached, PC=%06o\n",maxins,P.regs[PC]);
    printf("regs:"); for(int i=0;i<8;i++) printf(" R%d=%06o",i,P.regs[i]); printf("\n");
    /* dump 64K */
    FILE *o=fopen("/tmp/emu2_dump.bin","wb");
    for(int i=0;i<32768;i++){ unsigned char lo=M[i]&0xff,hi=M[i]>>8; fputc(lo,o); fputc(hi,o);}
    fclose(o); printf("dumped /tmp/emu2_dump.bin\n");
    return 0;
}
