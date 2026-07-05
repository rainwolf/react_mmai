// Poof-Pente fixtures (native, not wasm). Drives the real engine through
// getMove() / dmov() and asserts on the internal board + capture counters.
// Test-only: expose internals so we can inspect brd[0]/bd/ccc directly.
#define protected public
#include "Ai.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// Deterministic clock (same trick as regress.cpp): binds Ai.o's time() here so
// srand((unsigned)time(NULL)) inside Move() sees a fixed seed every run.
extern "C" time_t time(time_t *t) {
    if (t) *t = 777;
    return 777;
}

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  PASS: %s\n", msg); } \
    else { printf("  FAIL: %s\n", msg); failures++; } \
} while (0)

static inline int M(int x, int y) { return y * 19 + x; }

// --- case 1: single poof -------------------------------------------------
// E _ O E horizontally at y=9: P1 plays the gap (7,9) with behind=(6,9)=E,
// own neighbor=(8,9)=O, far=(9,9)=E -> both P1 stones poof, opp gets +2.
static void case1_single() {
    printf("case 1: single poof (E _ O E)\n");
    CAi ai(11, 2, true);
    int moves[] = { M(8,9), M(6,9), M(2,2), M(9,9), M(7,9) };
    ai.getMove(moves, 5);
    CHECK(ai.brd[0][7][9] == -1, "played stone (7,9) poofed");
    CHECK(ai.brd[0][8][9] == -1, "own neighbor (8,9) poofed");
    CHECK(ai.ccc[0][2] == 2,     "opponent (P2) credited +2");
    CHECK(ai.ccc[0][1] == 0,     "mover (P1) credit unchanged");
    CHECK(ai.brd[0][6][9] == 2 && ai.brd[0][9][9] == 2, "enemy flanks intact");
}

// --- case 2: double poof -------------------------------------------------
// P1 plays (9,9); fires East (nbr 10,9 / far 11,9 / behind 8,9) AND
// North (nbr 9,8 / far 9,7 / behind 9,10). 3 stones removed, opp +3.
static void case2_double() {
    printf("case 2: double poof\n");
    CAi ai(11, 2, true);
    int moves[] = { M(10,9), M(8,9), M(9,8), M(11,9), M(2,2),
                    M(9,10), M(2,4), M(9,7), M(9,9) };
    ai.getMove(moves, 9);
    CHECK(ai.brd[0][9][9]  == -1, "played stone (9,9) poofed");
    CHECK(ai.brd[0][10][9] == -1, "East neighbor (10,9) poofed");
    CHECK(ai.brd[0][9][8]  == -1, "North neighbor (9,8) poofed");
    CHECK(ai.ccc[0][2] == 3,      "opponent (P2) credited +3");
    CHECK(ai.ccc[0][1] == 0,      "mover (P1) credit unchanged");
    CHECK(ai.brd[0][8][9]  == 2 && ai.brd[0][11][9] == 2 &&
          ai.brd[0][9][10] == 2 && ai.brd[0][9][7]  == 2, "enemy flanks intact");
}

// --- case 3: poof + capture in the same move -----------------------------
// P1 plays (9,9): poofs East (nbr 10,9), and captures the P2 pair
// (9,10),(9,11) to the South (anchored by P1 at 9,12).
static void case3_poof_and_capture() {
    printf("case 3: poof + capture, same move\n");
    CAi ai(11, 2, true);
    int moves[] = { M(10,9), M(8,9), M(9,12), M(11,9), M(2,2),
                    M(9,10), M(4,2), M(9,11), M(9,9) };
    ai.getMove(moves, 9);
    CHECK(ai.ccc[0][1] == 2, "P1 credited +2 for the capture");
    CHECK(ai.ccc[0][2] == 2, "P2 credited +2 for the poof");
    CHECK(ai.brd[0][9][9]  == -1 && ai.brd[0][10][9] == -1, "poofed pair gone");
    CHECK(ai.brd[0][9][10] == -1 && ai.brd[0][9][11] == -1, "captured pair gone");
    CHECK(ai.brd[0][9][12] == 1, "capture anchor (9,12) intact");
    CHECK(ai.brd[0][8][9]  == 2 && ai.brd[0][11][9] == 2, "poof flanks intact");
}

// --- case 4: negative cases (no removal, no credit) ----------------------
static void case4_negatives() {
    printf("case 4: negatives (no poof)\n");
    // (a) E O _ O E: behind is own -> safe triple
    {
        CAi ai(11, 2, true);
        int moves[] = { M(6,9), M(5,9), M(8,9), M(9,9), M(7,9) };
        ai.getMove(moves, 5);
        CHECK(ai.brd[0][7][9] == 1 && ai.brd[0][6][9] == 1 && ai.brd[0][8][9] == 1,
              "4a E O _ O E: all own stones stay");
        CHECK(ai.ccc[0][1] == 0 && ai.ccc[0][2] == 0, "4a no capture credit");
    }
    // (b) empty far flank: E _ O _
    {
        CAi ai(11, 2, true);
        int moves[] = { M(8,9), M(6,9), M(7,9) };
        ai.getMove(moves, 3);
        CHECK(ai.brd[0][7][9] == 1 && ai.brd[0][8][9] == 1, "4b empty far: no poof");
        CHECK(ai.ccc[0][1] == 0 && ai.ccc[0][2] == 0, "4b no credit");
    }
    // (c) far flank off the board edge: play (1,9), far West is (-1,9)
    {
        CAi ai(11, 2, true);
        int moves[] = { M(0,9), M(2,9), M(1,9) };
        ai.getMove(moves, 3);
        CHECK(ai.brd[0][1][9] == 1 && ai.brd[0][0][9] == 1, "4c edge far: no poof");
        CHECK(ai.ccc[0][1] == 0 && ai.ccc[0][2] == 0, "4c no credit");
    }
    // (d) enemy behind missing (empty): _ [play] O E
    {
        CAi ai(11, 2, true);
        int moves[] = { M(9,9), M(10,9), M(8,9) };
        ai.getMove(moves, 3);
        CHECK(ai.brd[0][8][9] == 1 && ai.brd[0][9][9] == 1, "4d empty behind: no poof");
        CHECK(ai.ccc[0][1] == 0 && ai.ccc[0][2] == 0, "4d no credit");
    }
}

// --- case 5: variant gating ----------------------------------------------
// The case-1 position poofs under O-Pente (25) but is inert for 1/2/15.
static void case5_variants() {
    printf("case 5: variant gating\n");
    int moves[] = { M(8,9), M(6,9), M(2,2), M(9,9), M(7,9) };
    {
        CAi ai(25, 2, true);
        ai.getMove(moves, 5);
        CHECK(ai.brd[0][7][9] == -1 && ai.brd[0][8][9] == -1 && ai.ccc[0][2] == 2,
              "game 25 (O-Pente): pair poofs");
    }
    int inert[] = { 1, 2, 15 };
    for (int k = 0; k < 3; k++) {
        int mv[] = { M(8,9), M(6,9), M(2,2), M(9,9), M(7,9) };
        CAi ai(inert[k], 2, true);
        ai.getMove(mv, 5);
        char buf[64];
        snprintf(buf, sizeof buf, "game %d: no poof (stones + counters intact)", inert[k]);
        CHECK(ai.brd[0][7][9] == 1 && ai.brd[0][8][9] == 1 &&
              ai.ccc[0][1] == 0 && ai.ccc[0][2] == 0, buf);
    }
}

// --- case 6: search integrity (Tree fully unwinds poofs) ------------------
static void integrity_check(int *moves, int n, const char *name) {
    CAi ai(11, 4, true);
    int mv = ai.getMove(moves, n);
    int bad = 0;
    for (int x = 0; x < 19; x++)
        for (int y = 0; y < 19; y++)
            if (ai.bd[x][y] > 0 || ai.brd[0][x][y] > 0)
                if (ai.bd[x][y] != ai.brd[0][x][y]) bad++;
    char buf[96];
    snprintf(buf, sizeof buf, "%s: bd == brd[0] on every stone cell", name);
    CHECK(bad == 0, buf);
    int mx = mv % 19, my = mv / 19;
    snprintf(buf, sizeof buf, "%s: returned move %d is a legal empty square", name, mv);
    CHECK(mv >= 0 && mv <= 360 && ai.brd[0][mx][my] <= 0, buf);
}

static void case6_search_integrity() {
    printf("case 6: search integrity\n");
    int a[] = { M(10,9), M(8,9), M(9,8), M(11,9), M(2,2), M(9,10), M(2,4), M(9,7) };
    integrity_check(a, 8, "posA (double-poof pending)");
    int b[] = { M(10,9), M(8,9), M(9,12), M(11,9), M(2,2), M(9,10), M(4,2), M(9,11) };
    integrity_check(b, 8, "posB (poof+capture pending)");
    int c[] = { M(8,9), M(6,9), M(2,2), M(9,9) };
    integrity_check(c, 4, "posC (single-poof pending)");
}

// --- case 7: Score()'s poof math is actually load-bearing in Eval() --------
// The old case 7 ("suicide avoidance") only proved the *search* never chose
// a losing move; it passed unchanged even with the poof scoring neutered
// (mutation testing showed Score()'s poof-scan and its scoring tail can both
// be gutted with zero test impact). These cases instead call Eval() directly
// -- the same call Tree() makes over every candidate square -- and assert on
// its return value, so they fail if that scoring math is disabled.
//
// replay(): apply a move list through the same addMove()/dmov() path the
// other cases use, leaving tn/moveNum positioned right after the last move.
static void replay(CAi &ai, int *moves, int n) {
    for (int i = 0; i < n; i++) ai.addMove(moves[i], i + 1);
    ai.moveNum = n;
    ai.tn = n + 1;      // next move to be considered
}

// primeEval(): reproduce the minimal state Move() sets up before calling
// Tree(), plus the level-1 bookkeeping Tree() does before its first Eval()
// call in the candidate loop (Ai.cpp: Move() ~line 450-460, Tree() ~line
// 874-901) -- board copy brd[0]->bd, cc[0][*]->cc[1][*], fr/en for the
// mover, and clearing the fukumi scratch table -- so Eval(x,y) can be
// called directly on a hand-built position without running Tree()'s search.
static void primeEval(CAi &ai, int mover) {
    for (int x = 0; x < 19; x++)
        for (int y = 0; y < 19; y++)
            ai.bd[x][y] = ai.brd[0][x][y];
    ai.cc[0][1] = ai.ccc[0][1];
    ai.cc[0][2] = ai.ccc[0][2];
    ai.cp = mover;
    ai.np = 2;
    ai.gf = 0;
    ai.multipbem = 0;
    ai.lvl = 0;
    ai.lvl++;                                  // Tree()'s "do { lvl++; ..."
    ai.cc[ai.lvl][1] = ai.cc[ai.lvl - 1][1];
    ai.cc[ai.lvl][2] = ai.cc[ai.lvl - 1][2];
    ai.fr = ai.cp - 1 + ai.lvl;
    while (ai.fr > ai.np) ai.fr -= ai.np;
    ai.en = ai.fr + 1;
    if (ai.en > ai.np) ai.en = 1;
    for (int i = 0; i < 1444; i++) ai.pFk[i] = 0; // clear fukumi table
    ai.fhn = 0;
    ai.ferr = 0;
}

// --- case 7a: a plain poof must score worse than the identical Pente move -
// E _ O E on y=9 (same geometry as case 1): P1 to move, gap at (7,9) not yet
// played. Under real Score(), playing the gap poofs P1's own neighbor and
// hands the opponent material, which the poof tail folds into sco[3-fr]
// (quadrupled in Eval()'s final s0 = sco[fr] - sco[3-fr]*4). Both mutants
// suppress that penalty for the game-11 engine, so ai11's Eval collapses
// toward ai1's (plain Pente, no poof rule at all) plain positional score.
static void case7a_poof_penalty() {
    printf("case 7a: poof penalty visible in Eval\n");
    int moves[] = { M(8,9), M(6,9), M(2,2), M(9,9) }; // own nbr, near, dummy, far
    CAi ai11(11, 2, true);
    replay(ai11, moves, 4);
    primeEval(ai11, 1);
    int e11 = ai11.Eval(7, 9);

    CAi ai1(1, 2, true);
    replay(ai1, moves, 4);
    primeEval(ai1, 1);
    int e1 = ai1.Eval(7, 9);

    printf("  (ai11 Eval=%d, ai1 Eval=%d)\n", e11, e1);
    CHECK(e11 < e1, "poof-aware Eval (game 11) scores the poofing move strictly lower than plain Pente (game 1)");
}

// --- case 7b: a five that poofs away must NOT score as a win --------------
// P1 completes five along y=9 by playing (10,9); that same move also poofs
// a P1 stone at (10,10) (own neighbor on a different axis, flanked by P2 at
// (10,8)/(10,11)). Score()'s poof tail explicitly clamps a >=10000 "row win"
// score down (sco[fr]/=8) because the five vanishes with the poofed stone.
// ai1 (plain Pente) has no poof rule, so its five just wins outright --
// that's the calibration for what Eval() returns for a genuine live five.
// ai11 must clamp far below that under real Score(); both mutants disable
// the clamp, so under them ai11 also reports a win.
static void case7b_phantom_five() {
    printf("case 7b: phantom five clamped by poof\n");
    int moves[] = {
        M(6,9),   M(10,11),  // tn1 P1, tn2 P2 (far poof flank)
        M(7,9),   M(10,8),   // tn3 P1, tn4 P2 (near/behind poof flank)
        M(8,9),   M(2,2),    // tn5 P1, tn6 P2 dummy
        M(9,9),   M(2,4),    // tn7 P1, tn8 P2 dummy
        M(10,10), M(2,6)     // tn9 P1 (own nbr that poofs), tn10 P2 dummy
    };
    CAi ai11(11, 2, true);
    replay(ai11, moves, 10);
    primeEval(ai11, 1);
    int e11 = ai11.Eval(10, 9);

    CAi ai1(1, 2, true);
    replay(ai1, moves, 10);
    primeEval(ai1, 1);
    int e1 = ai1.Eval(10, 9);

    printf("  (ai11 Eval=%d, ai1 Eval=%d)\n", e11, e1);
    CHECK(e1 >= 9000, "poof-free five (ai1, game 1) evaluates as a win-level score");
    CHECK(e11 < e1 / 2, "poofed five (ai11, game 11) is clamped far below the win-level score");
}

// --- case 7c: a poof that gifts the opponent a capture win ----------------
// Same E _ O E poof square as 7a, but the opponent already sits at 8
// captures, so the +2 poof credit reaches capWinCount(10). Score()'s poof
// tail specifically adds +11000 to sco[3-fr] when
// cc[lvl][3-fr]+capP+1>=cfg.capWinCount -- on top of the ordinary material
// gift -- so this must crater the mover's Eval far more than the plain
// (0-capture-opponent) poof from 7a. Both mutants suppress this too.
static void case7c_gift_the_win() {
    printf("case 7c: poof gifting the opponent a capture win\n");
    int moves[] = { M(8,9), M(6,9), M(2,2), M(9,9) };

    CAi ai0(11, 2, true);
    replay(ai0, moves, 4);
    primeEval(ai0, 1);
    int e0 = ai0.Eval(7, 9);            // baseline: opponent has 0 captures

    CAi ai8(11, 2, true);
    replay(ai8, moves, 4);
    ai8.ccc[0][2] = 8;                  // opponent 2 captures from a win
    primeEval(ai8, 1);
    int e8 = ai8.Eval(7, 9);

    printf("  (Eval with opp cc=0: %d, with opp cc=8: %d)\n", e0, e8);
    CHECK(e8 < e0 - 2000, "gifting the win craters the score far more than an ordinary poof");
}

int main() {
    if (chdir("MMAIWASM") != 0) {
        // allow running from inside MMAIWASM too
    }
    case1_single();
    case2_double();
    case3_poof_and_capture();
    case4_negatives();
    case5_variants();
    case6_search_integrity();
    case7a_poof_penalty();
    case7b_phantom_five();
    case7c_gift_the_win();
    printf("\n%s (%d failure%s)\n", failures ? "POOFTEST FAIL" : "POOFTEST PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
