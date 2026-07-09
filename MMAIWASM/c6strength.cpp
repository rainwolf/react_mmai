// Connect6 v3 strength harness (native, not wasm). Self-play v3-ON (engine A)
// vs baseline v3-OFF (engine B), color-balanced, varied seeds, at several search
// levels. The two engines differ ONLY by c6ForceEnabled, so any A-advantage is
// exactly the v3 forcing terminal's contribution. Also reports how often the v3
// RECURSION fired (c6ForceHits) — the honest "is it marginal?" signal.
//
// Build:  clang++ -O2 -std=c++11 c6strength.cpp Ai.cpp CPoint.cpp -o c6strength
// Run:    ./c6strength [gamesPerColorPerLevel]   (from the MMAIWASM dir)
#define protected public
#include "Ai.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// Tunable deterministic clock: Move() seeds srand((unsigned)time(NULL)); the
// harness bumps g_fakeTime per ply so rand-based tie-breaks vary across games
// (otherwise every game from the empty board would be identical).
static long g_fakeTime = 777;
extern "C" time_t time(time_t *t) { if (t) *t = (time_t)g_fakeTime; return (time_t)g_fakeTime; }

static const int AX[4][2] = { {-1,-1},{0,-1},{1,-1},{-1,0} };
static int c6owner(int idx) { return (idx%4==0 || idx%4==3) ? 1 : 2; }   // Connect6 cadence
// diagnostics: does the engine ever CLAIM a win (bscr>=10000) in these games?
static long g_maxBscr = -1000000, g_winClaims = 0;

// 6+-in-a-row through (x,y) for player P.
static bool sixThrough(const int b[19][19], int x, int y, int P) {
    for (int a=0;a<4;a++){ int dx=AX[a][0],dy=AX[a][1],run=1;
        for (int s=1;s<6;s++){int cx=x+s*dx,cy=y+s*dy; if(cx<0||cx>=19||cy<0||cy>=19||b[cx][cy]!=P)break; run++;}
        for (int s=1;s<6;s++){int cx=x-s*dx,cy=y-s*dy; if(cx<0||cx>=19||cy<0||cy>=19||b[cx][cy]!=P)break; run++;}
        if (run>=6) return true; }
    return false;
}

// deterministic LCG for random openings (per-game seed)
static unsigned long lcgS = 1;
static int lcg(int n){ lcgS = lcgS*6364136223846793005UL + 1442695040888963407UL; return (int)((lcgS>>33)%n); }

// One game: e1 owns player 1, e2 owns player 2. Returns 1/2 winner or 0 draw.
// openN random legal stones (following the 2-stone cadence) are placed first so
// games diverge into varied tactical positions instead of one deterministic
// line from the empty board.
static int playGame(CAi &e1, CAi &e2, long seed, long &h1, long &h2, int &plies, int maxPlies, int openN) {
    int b[19][19]; for(int x=0;x<19;x++)for(int y=0;y<19;y++)b[x][y]=0;
    int moves[400], count=0, winner=0;
    e1.c6ForceHits=0; e2.c6ForceHits=0;
    lcgS = (unsigned long)(seed*2654435761UL + 12345);
    for (int i=0; i<openN && count<maxPlies; i++) {          // random opening
        int owner = c6owner(count), x, y, tries=0;
        do { x=3+lcg(13); y=3+lcg(13); } while (b[x][y]!=0 && ++tries<50);
        if (b[x][y]!=0) break;
        b[x][y]=owner; moves[count++]=x+y*19;
        if (sixThrough(b,x,y,owner)) { winner=owner; break; }  // unlikely in opening
    }
    while (winner==0 && count < maxPlies) {
        int owner = c6owner(count);
        CAi &e = (owner==1)? e1 : e2;
        g_fakeTime = seed*1000003L + count;
        int packed = e.getMove(moves, count);
        if (e.bscr > g_maxBscr) g_maxBscr = e.bscr;
        if (e.bscr >= 10000) g_winClaims++;              // engine believes it has a win
        int m1 = packed/362, m2 = packed%362;
        if (m1<0||m1>360||b[m1%19][m1/19]!=0) break;         // illegal -> stop (draw)
        b[m1%19][m1/19]=owner; moves[count++]=m1;
        if (sixThrough(b, m1%19, m1/19, owner)) { winner=owner; break; }
        if (m2 != 361) {
            if (m2<0||m2>360||b[m2%19][m2/19]!=0) break;
            b[m2%19][m2/19]=owner; moves[count++]=m2;
            if (sixThrough(b, m2%19, m2/19, owner)) { winner=owner; break; }
        }
    }
    h1 += e1.c6ForceHits; h2 += e2.c6ForceHits; plies = count;
    return winner;
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL);                      // unbuffered: per-level progress visible live
    if (chdir("MMAIWASM") != 0) { /* allow running inside MMAIWASM too */ }
    { // harness self-test: six-in-a-row detection must work (else all-draws is a bug)
      int tb[19][19]; for(int x=0;x<19;x++)for(int y=0;y<19;y++)tb[x][y]=0;
      for(int k=0;k<6;k++) tb[5+k][9]=1;
      bool ok = sixThrough(tb,7,9,1) && sixThrough(tb,5,9,1) && !sixThrough(tb,7,8,1);
      int tv[19][19]; for(int x=0;x<19;x++)for(int y=0;y<19;y++)tv[x][y]=0;
      for(int k=0;k<6;k++) tv[9][4+k]=2;       // vertical six for P2
      ok = ok && sixThrough(tv,9,7,2);
      printf("  self-test: sixThrough %s\n", ok?"OK (detects horizontal+vertical sixes)":"*** BROKEN ***");
      if (!ok) return 1;
    }
    int games = (argc>1)?atoi(argv[1]):15;    // games per color per level
    int maxPlies = 100;                        // draw cap
    int openN = (argc>2)?atoi(argv[2]):10;     // random opening stones (variety + tactics)
    int levels[3] = {2,3,4};
    printf("Connect6 v3 strength: self-play A(v3-ON) vs B(baseline-OFF), %d games/color/level, maxPlies=%d, openN=%d\n",
           games, maxPlies, openN);
    for (int li=0; li<3; li++) {
        int L = levels[li];
        long Awins=0,Bwins=0,draws=0, hitsA=0,hitsB=0, totPlies=0; int ng=0;
        for (int g=0; g<games; g++) {
            { CAi A(13,L,true);  A.c6ForceEnabled=true;
              CAi B(13,L,true);  B.c6ForceEnabled=false;
              int pl; int w = playGame(A,B, 1000+g, hitsA, hitsB, pl, maxPlies, openN);   // A=P1
              totPlies+=pl; ng++;
              if (w==1) Awins++; else if (w==2) Bwins++; else draws++; }
            { CAi A(13,L,true);  A.c6ForceEnabled=true;
              CAi B(13,L,true);  B.c6ForceEnabled=false;
              int pl; int w = playGame(B,A, 1000+g, hitsB, hitsA, pl, maxPlies, openN);   // A=P2
              totPlies+=pl; ng++;
              if (w==2) Awins++; else if (w==1) Bwins++; else draws++; }
        }
        printf("  L=%d: A(v3) wins=%ld  B(base) wins=%ld  draws=%ld  (of %d games)  | v3-recursion fires: A=%ld B=%ld  avgPlies=%.0f\n",
               L, Awins, Bwins, draws, ng, hitsA, hitsB, (double)totPlies/ng);
    }
    // Harness sanity: a much stronger engine (L=4) vs a weak one (L=1) should win
    // decisively. If this ALSO all-draws, the finding is that these engines cannot
    // force wins in Connect6 within maxPlies (genuine drawishness), not a bug.
    { long sW=0,wW=0,dr=0,hh=0; int pl;
      for (int g=0; g<8; g++) {
          CAi S(13,4,true),  Wk(13,1,true);
          int w1 = playGame(S,Wk, 5000+g, hh,hh, pl, maxPlies, openN);
          if (w1==1) sW++; else if (w1==2) wW++; else dr++;
          CAi S2(13,4,true), Wk2(13,1,true);
          int w2 = playGame(Wk2,S2, 5000+g, hh,hh, pl, maxPlies, openN);   // strong is P2
          if (w2==2) sW++; else if (w2==1) wW++; else dr++;
      }
      printf("  SANITY strong(L4) vs weak(L1), 16 games: strongWins=%ld weakWins=%ld draws=%ld\n", sW,wW,dr);
    }
    printf("  DIAG: max bscr seen across all games=%ld  win-claims(bscr>=10000)=%ld\n", g_maxBscr, g_winClaims);
    printf("DONE\n");
    return 0;
}
