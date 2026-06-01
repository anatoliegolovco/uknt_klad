// sim_play.c — HEADLESS full-game playthrough proof.
// A real player (src/player.c) follows a planned path key→exit while the REAL enemy AI
// (src/enemy.c do_move/enemy_tick) actively chases. Proves a level is winnable WITH the
// adversary running (not just reachable in the abstract). Uses the actual game code.
//
// Build: cc -std=c2x -I src tools/sim_play.c src/map.c src/player.c src/enemy.c src/score.c -lm -o /tmp/sim
// Run:   /tmp/sim [level 1..10]
#include "map.h"
#include "player.h"
#include "enemy.h"
#include "level_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ENEMIES 2

// ── path planning: BFS over the verified movement flags (same as reachability) ──
static int raw(const Map *m, int c, int r) { return map_raw(m, c, r); }
static int settle(const Map *m, int c, int r) {
    while (r < MAP_ROWS - 1 && raw(m,c,r) != 8 && (raw(m,c,r+1) <= 6 || raw(m,c,r+1) >= 13)) r++;
    return r;
}
static bool drowns(const Map *m, int c, int r) { int t=raw(m,c,r); return t==13||t==14; }

// BFS returns path length; fills path[] with (col,row) from start to goal (inclusive).
// `blk` (or NULL) marks cells to avoid (enemy positions + neighbours) — chaser evasion.
static int plan(const Map *m, int sc, int sr, int gc, int gr, int path[][2], int maxp,
                bool blk[MAP_ROWS][MAP_COLS]) {
    static int par[MAP_ROWS][MAP_COLS][2]; static bool seen[MAP_ROWS][MAP_COLS];
    static int qc[MAP_ROWS*MAP_COLS], qr[MAP_ROWS*MAP_COLS];
    memset(seen,0,sizeof seen);
    int head=0,tail=0; sr=settle(m,sc,sr);
    seen[sr][sc]=true; qc[0]=sc; qr[0]=sr; tail=1; par[sr][sc][0]=-1;
    bool found=false;
    while(head<tail){
        int c=qc[head],r=qr[head]; head++;
        if(c==gc && r==gr){ found=true; break; }
        int mv[4][2]; int n=0;
        if(map_can_left (m,c,r)){ mv[n][0]=c-1; mv[n][1]=settle(m,c-1,r); n++; }
        if(map_can_right(m,c,r)){ mv[n][0]=c+1; mv[n][1]=settle(m,c+1,r); n++; }
        if(map_can_up   (m,c,r)){ mv[n][0]=c;   mv[n][1]=r-1;             n++; }
        if(map_can_down (m,c,r)){ mv[n][0]=c;   mv[n][1]=r+1;             n++; }
        for(int i=0;i<n;i++){
            int nc=mv[i][0],nr=mv[i][1];
            if(nr<0||nr>=MAP_ROWS||nc<0||nc>=MAP_COLS) continue;
            if(drowns(m,nc,nr)||seen[nr][nc]) continue;
            if(blk && blk[nr][nc] && !(nc==gc&&nr==gr)) continue;   // avoid enemy zone (but goal ok)
            seen[nr][nc]=true; par[nr][nc][0]=c; par[nr][nc][1]=r;
            qc[tail]=nc; qr[tail]=nr; tail++;
        }
    }
    if(!found) return -1;
    int tmp[MAP_ROWS*MAP_COLS][2]; int len=0;
    int c=gc,r=gr;
    while(c!=-1){ tmp[len][0]=c; tmp[len][1]=r; len++; int pc=par[r][c][0],pr=par[r][c][1]; c=pc; r=pr; if(c==-1)break; }
    if(len>maxp) len=maxp;
    for(int i=0;i<len;i++){ path[i][0]=tmp[len-1-i][0]; path[i][1]=tmp[len-1-i][1]; }
    return len;
}

static void pcell(const Player *p, int *c, int *r){
    *c=(int)((p->px+TILE_W*0.5f)/TILE_W); *r=(int)((p->py+TILE_H*0.5f)/TILE_H);
}

int main(int argc, char**argv){
    int lvl = argc>1 ? atoi(argv[1])-1 : 0;
    Map m; map_load(&m, lvl);
    Player pl; player_init(&pl, LEVEL_SPAWNS[lvl][0][0], LEVEL_SPAWNS[lvl][0][1]);
    Enemy en[MAX_ENEMIES];
    for(int i=0;i<MAX_ENEMIES;i++) enemy_init(&en[i], LEVEL_SPAWNS[lvl][i+1][0], LEVEL_SPAWNS[lvl][i+1][1]);

    // goals: key (gold_c=tile6) if present, then exit (tile2)
    int kc=-1,kr=-1, xc=-1,xr=-1;
    for(int r=0;r<MAP_ROWS;r++)for(int c=0;c<MAP_COLS;c++){
        if(m.raw[r][c]==6){kc=c;kr=r;} if(m.raw[r][c]==2){xc=c;xr=r;}
    }
    printf("=== SIM level %d: spawn(%d,%d) key(%d,%d) exit(%d,%d) | enemies: ",
           lvl+1, LEVEL_SPAWNS[lvl][0][0], LEVEL_SPAWNS[lvl][0][1], kc,kr, xc,xr);
    for(int i=0;i<MAX_ENEMIES;i++) if(en[i].active) printf("(%d,%d) ", en[i].col,en[i].row);
    printf("===\n");

    static int path[MAP_ROWS*MAP_COLS][2]; int plen=0;
    int gc = (kc>=0)?kc:xc, gr = (kc>=0)?kr:xr;   // current goal (key first, else exit)
    bool gotkey = (kc<0);
    bool dashing = false;                          // exit-phase commit flag (lure→dash)
    int pc,pr; pcell(&pl,&pc,&pr);

    const float dt=1.0f/60.0f; int maxframes=60*120; // 120s
    int noprogress=0, bestdist=1<<30;
    for(int f=0; f<maxframes; f++){
        pcell(&pl,&pc,&pr);
        // mark enemy cells + neighbours as blocked (chaser evasion)
        static bool blk[MAP_ROWS][MAP_COLS]; memset(blk,0,sizeof blk);
        for(int i=0;i<MAX_ENEMIES;i++){
            if(!en[i].active) continue;
            for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++){
                int c=en[i].col+dc, r=en[i].row+dr;
                if(c>=0&&c<MAP_COLS&&r>=0&&r<MAP_ROWS) blk[r][c]=true;
            }
        }
        // RE-PLAN every frame to the goal, avoiding the enemy zone; fall back w/o avoidance.
        plen = plan(&m,pc,pr,gc,gr,path,MAP_ROWS*MAP_COLS, blk);
        if(plen<0) plen = plan(&m,pc,pr,gc,gr,path,MAP_ROWS*MAP_COLS, NULL);

        // nearest-enemy distance (Manhattan)
        int edist=1<<30, eci=-1;
        for(int i=0;i<MAX_ENEMIES;i++){ if(!en[i].active)continue;
            int d=abs(en[i].col-pc)+abs(en[i].row-pr); if(d<edist){edist=d; eci=i;} }

        // GOAL selection: in the exit phase, if a chaser still guards the exit, LURE it away
        // (head to the bottom-left spawn) so it leaves; once the exit is clear, DASH (player is
        // 2–4x faster, so it beats the chaser back). Otherwise pursue the real goal.
        if(gotkey){
            int eExit=1<<30;
            for(int i=0;i<MAX_ENEMIES;i++){ if(!en[i].active)continue;
                int d=abs(en[i].col-xc)+abs(en[i].row-xr); if(d<eExit)eExit=d; }
            // dash as soon as the chaser is OFF the exit's approach (>=4 cells away) and commit
            // (player is 2-4x faster → beats it back). Re-lure only if it gets right onto the exit.
            if(!dashing && eExit>=4) dashing=true;
            if(dashing && eExit<=1)  dashing=false;      // chaser back ON the exit → re-lure
            if(dashing){ gc=xc; gr=xr; }                                                            // dash
            else { gc=LEVEL_SPAWNS[lvl][0][0]; gr=settle(&m,gc,LEVEL_SPAWNS[lvl][0][1]); }          // lure
        }

        // adjacent reachable cells (via the verified flags + settle)
        int adj[4][2]; int an=0;
        if(map_can_left (&m,pc,pr)){adj[an][0]=pc-1;adj[an][1]=settle(&m,pc-1,pr);an++;}
        if(map_can_right(&m,pc,pr)){adj[an][0]=pc+1;adj[an][1]=settle(&m,pc+1,pr);an++;}
        if(map_can_up   (&m,pc,pr)){adj[an][0]=pc;  adj[an][1]=pr-1;             an++;}
        if(map_can_down (&m,pc,pr)){adj[an][0]=pc;  adj[an][1]=pr+1;             an++;}

        int tc=pc, tr=pr;
        if(edist<=2 && eci>=0){
            // FLEE: pick the adjacent cell that maximises distance from the chaser (juke).
            int best=-1, bestd=-1;
            for(int i=0;i<an;i++){
                if(drowns(&m,adj[i][0],adj[i][1])) continue;
                int d=abs(adj[i][0]-en[eci].col)+abs(adj[i][1]-en[eci].row);
                if(d>bestd){bestd=d; best=i;}
            }
            if(best>=0){ tc=adj[best][0]; tr=adj[best][1]; }
        } else if(plen>=2){
            tc=path[1][0]; tr=path[1][1];        // pursue goal
        }
        Input in={0};
        if      (tc<pc) in.left=true;
        else if (tc>pc) in.right=true;
        else if (tr<pr) in.up=true;
        else if (tr>pr) in.down=true;

        PlayerResult res = player_update(&pl,&m,in,dt);
        if(res==PR_GOLD||res==PR_BONUS){
            int cc,cr; pcell(&pl,&cc,&cr); map_clear(&m,cc,cr);
        } else if(res==PR_LEVEL_WIN){           // key → open door, switch goal to exit
            int cc,cr; pcell(&pl,&cc,&cr); map_clear(&m,cc,cr); map_open_door(&m);
            gotkey=true; gc=xc; gr=xr; bestdist=1<<30;
            printf("  [f%4d] KEY collected at (%d,%d) → door open, now heading to exit\n",f,cc,cr);
        } else if(res==PR_EXIT){
            printf("  [f%4d] reached EXIT (%d,%d)\n",f,pc,pr);
            printf("RESULT: \xe2\x9c\x85 WIN — level %d passed with adversary AI active (%d frames, %.1fs)\n",
                   lvl+1, f, f*dt);
            return 0;
        } else if(res==PR_WATER){ printf("RESULT: FAIL — drowned at (%d,%d) f%d\n",pc,pr,f); return 1; }

        for(int i=0;i<MAX_ENEMIES;i++){
            if(enemy_tick(&en[i],&m,&pl,dt)){
                printf("RESULT: FAIL — caught by enemy %d at (%d,%d) f%d (goal %s)\n",
                       i,pc,pr,f, gotkey?"exit":"key"); return 1;
            }
        }
        if(getenv("TRACE") && f%180==0){
            int eExit=1<<30; for(int i=0;i<MAX_ENEMIES;i++){ if(en[i].active){int d=abs(en[i].col-xc)+abs(en[i].row-xr); if(d<eExit)eExit=d;} }
            fprintf(stderr,"  f%4d p(%2d,%2d) e0(%2d,%2d) dash=%d eExit=%d plen=%d %s\n",
                    f,pc,pr, en[0].col,en[0].row, dashing, eExit, plen, gotkey?"EXIT":"KEY");
        }
        (void)noprogress; (void)bestdist;   // luring makes plen oscillate; rely on maxframes
    }
    printf("RESULT: FAIL — timeout (%ds), goal %s\n", maxframes/60, gotkey?"exit":"key");
    return 1;
}
