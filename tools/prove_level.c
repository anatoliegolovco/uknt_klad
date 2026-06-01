// prove_level.c — RIGOROUS proof a level is winnable WITH the adversary AI.
// Turn-based state-space BFS over (player cell, enemy cell, enemy-falling, got-key).
//   - Player abilities = the exact verified flags (map_can_left/right/up/down + settle) + STAY,
//     using the door-open map once the key is taken (design B).
//   - Enemy step = enemy.c do_move replicated at cell level (greedy chase + commit-fall).
//   - Player gets 2 moves per 1 enemy move (CONSERVATIVE: the real player is 2-4x faster, so a
//     win here implies a win in the real continuous game). Collision at any point = pruned.
// If BFS reaches the exit (after the key) → prints the winning move sequence = PROOF.
//
// Build: cc -std=c2x -I src tools/prove_level.c src/map.c src/score.c -lm -o /tmp/prove
// Run:   /tmp/prove [level 1..10]
#include "map.h"
#include "level_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NC MAP_COLS
#define NR MAP_ROWS
#define CELLS (NC*NR)
#define CELL(c,r) ((r)*NC+(c))
#define COL(x) ((x)%NC)
#define ROW(x) ((x)/NC)

static Map mClosed, mOpen;   // door closed / open
static int raw(const Map*m,int c,int r){ return map_raw(m,c,r); }
static int settle(const Map*m,int c,int r){
    while(r<NR-1 && raw(m,c,r)!=8 && (raw(m,c,r+1)<=6||raw(m,c,r+1)>=13)) r++;
    return r;
}
static bool drowns(int c,int r){ int t=raw(&mClosed,c,r); return t==13||t==14; }

// enemy step — EXACT replica of src/enemy.c do_move (cell level). Uses base map (enemies can't
// pass tile 9/10 anyway since epass = ≤8). Returns packed (cell<<1 | falling).
static int estep(int ec,int er,int ef,int pc,int pr){
    const Map*m=&mClosed;
    if(ef){
        if(er+1<NR && raw(m,ec,er+1)<=6) er++;
        else ef=0;
        return (CELL(ec,er)<<1)|ef;
    }
    bool moved=false;
    if(ec!=pc){ int dx=pc>ec?1:-1; if(raw(m,ec+dx,er)<=8){ec+=dx;moved=true;} }
    if(!moved && er!=pr){
        if(pr>er && (raw(m,ec,er+1)==8 || (raw(m,ec,er)==8&&raw(m,ec,er+1)<=6))){er++;moved=true;}
        else if(pr<er && raw(m,ec,er)==8 && raw(m,ec,er-1)<=8){er--;moved=true;}
    }
    if(!moved && raw(m,ec,er)!=8 && raw(m,ec,er+1)<=6){ ef=1; if(er+1<NR) er++; }
    return (CELL(ec,er)<<1)|ef;
}

// player candidate moves from (pc,pr) given door state; fills dst cells, returns count.
// actions: 0=stay 1=L 2=R 3=U 4=D
static int pmoves(int pc,int pr,bool door,int dst[5],char act[5]){
    const Map*m = door?&mOpen:&mClosed;
    int n=0;
    dst[n]=CELL(pc,pr); act[n]='.'; n++;                                   // stay
    if(map_can_left (m,pc,pr)){int r=settle(m,pc-1,pr); dst[n]=CELL(pc-1,r);act[n]='L';n++;}
    if(map_can_right(m,pc,pr)){int r=settle(m,pc+1,pr); dst[n]=CELL(pc+1,r);act[n]='R';n++;}
    if(map_can_up   (m,pc,pr)){dst[n]=CELL(pc,pr-1);act[n]='U';n++;}
    if(map_can_down (m,pc,pr)){dst[n]=CELL(pc,pr+1);act[n]='D';n++;}
    return n;
}

// state index: pcell * ecell * ef(2) * key(2) * sub(g_nsub)
// sub 0..ratio-1 = player moves; sub==ratio = enemy move → player moves `ratio`× per enemy move.
#define NEF 2
#define NKEY 2
static int g_nsub = 3;     // = ratio+1 (set from argv)
static long sidx(int pc,int ec,int ef,int key,int sub){
    return (((( (long)pc*CELLS + ec)*NEF + ef)*NKEY + key)*g_nsub + sub);
}

int main(int argc,char**argv){
    int lvl = argc>1?atoi(argv[1])-1:0;
    int ratio = argc>2?atoi(argv[2]):2;  if(ratio<1)ratio=1;  // player moves per enemy move
    g_nsub = ratio+1;
    long NSTATES = (long)CELLS*CELLS*NEF*NKEY*g_nsub;
    map_load(&mClosed,lvl);
    map_load(&mOpen,lvl); map_open_door(&mOpen);

    int sc=LEVEL_SPAWNS[lvl][0][0], sr=settle(&mClosed,LEVEL_SPAWNS[lvl][0][0],LEVEL_SPAWNS[lvl][0][1]);
    int ec0=LEVEL_SPAWNS[lvl][1][0], er0=LEVEL_SPAWNS[lvl][1][1];
    if(ec0<0){ printf("level %d has no enemy1 — trivially see reachability tool\n",lvl+1); }
    int kc=-1,kr=-1,xc=-1,xr=-1;
    for(int r=0;r<NR;r++)for(int c=0;c<NC;c++){ if(mClosed.raw[r][c]==6){kc=c;kr=r;} if(mClosed.raw[r][c]==2){xc=c;xr=r;} }
    bool needkey = (kc>=0);
    printf("=== PROVE level %d: spawn(%d,%d) key%s exit(%d,%d) enemy1(%d,%d) | player %dx speed\n",
           lvl+1, sc,sr, needkey?"(present)":"(none)", xc,xr, ec0,er0, ratio);

    // BFS
    unsigned char *vis = calloc(NSTATES,1);
    int *par = malloc(sizeof(int)*NSTATES);
    char *pact = malloc(NSTATES);
    if(!vis||!par||!pact){ printf("alloc fail\n"); return 2; }
    long *q = malloc(sizeof(long)*NSTATES);
    long head=0,tail=0;

    int ef0 = 0;
    int key0 = needkey?0:1;
    long s0 = sidx(CELL(sc,sr), ec0<0?0:CELL(ec0,er0), ef0, key0, 0);
    vis[s0]=1; par[s0]=-1; pact[s0]=0; q[tail++]=s0;
    long winstate=-1; long expanded=0; long keystates=0;

    while(head<tail){
        long s=q[head++]; expanded++;
        // decode
        long t=s; int sub=t%g_nsub; t/=g_nsub; int key=t%NKEY; t/=NKEY; int ef=t%NEF; t/=NEF;
        int ec=t%CELLS; t/=CELLS; int pc_=t;
        int pcol=COL(pc_),prow=ROW(pc_), ecol=COL(ec),erow=ROW(ec);

        if(sub < g_nsub-1){
            // PLAYER MOVE (sub 0..ratio-1 → `ratio` player moves before the enemy moves)
            int dst[5]; char act[5]; int n=pmoves(pcol,prow,key, dst,act);
            int nsub = sub+1;                          // advance sub; reaches g_nsub-1 = enemy turn
            for(int i=0;i<n;i++){
                int npc=dst[i]; int ncol=COL(npc),nrow=ROW(npc);
                if(drowns(ncol,nrow)) continue;
                if(npc==ec) continue;                 // would step onto enemy = death
                int nkey=key;
                if(key==0 && needkey && ncol==kc && nrow==kr) nkey=1;   // got key → door opens
                if((nkey==1) && ncol==xc && nrow==xr){                  // WIN: on exit + key
                    long ws=sidx(npc,ec,ef,nkey,nsub);
                    if(!vis[ws]){vis[ws]=1;par[ws]=(int)s;pact[ws]=act[i];}
                    winstate=ws; goto done;
                }
                long ns=sidx(npc,ec,ef,nkey,nsub);
                if(!vis[ns]){ vis[ns]=1; par[ns]=(int)s; pact[ns]=act[i]; q[tail++]=ns; if(nkey&&!key)keystates++; }
            }
        } else {
            // ENEMY MOVE (sub 2 → back to sub 0).
            int packed = (ec0<0)? ((ec<<1)|ef) : estep(ecol,erow,ef, pcol,prow);
            int nec=packed>>1, nef=packed&1;
            if(nec==pc_){ continue; }                 // enemy steps onto player = death → prune
            long ns=sidx(pc_,nec,nef,key,0);
            if(!vis[ns]){ vis[ns]=1; par[ns]=(int)s; pact[ns]=0; q[tail++]=ns; }
        }
    }
done:
    if(winstate>=0){
        // reconstruct player actions
        char seq[100000]; int sl=0;
        long s=winstate;
        while(s>=0 && par[s]>=0){ if(pact[s]) seq[sl++]=pact[s]; s=par[s]; }
        printf("RESULT: \xe2\x9c\x85 WIN PROVEN — a winning play exists vs the live enemy AI.\n");
        printf("  player moves (reversed→forward), %d steps:\n  ", sl);
        for(int i=sl-1;i>=0;i--) putchar(seq[i]);
        printf("\n  (expanded %ld states)\n", expanded);
        return 0;
    }
    printf("RESULT: \xe2\x9d\x8c NO WIN at %dx (expanded %ld states; key-collected states=%ld). %s\n",
           ratio, expanded, keystates,
           keystates==0 ? "Player never even reaches the key safely → enemy guards the key path."
                        : "Player gets the key but can't survive to the exit.");
    return 1;
}
