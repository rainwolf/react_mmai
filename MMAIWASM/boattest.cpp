// Boat-Pente fixtures (native, not wasm). Mirrors pooftest.cpp: drives the real
// engine and asserts on Eval()/getMove() so the provisional-five logic is
// load-bearing (a neutered boatRunProof or survived-check must fail a case).
// Test-only: expose internals so we can build positions and call Eval directly.
#define protected public
#include "Ai.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// Deterministic clock (same trick as regress.cpp/pooftest.cpp): bind Ai.o's
// time() here so srand((unsigned)time(NULL)) inside Move() sees a fixed seed.
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

// replay()/primeEval(): identical to pooftest.cpp -- apply a move list through
// addMove()/dmov(), then reproduce the minimal state Move()/Tree() set up so
// Eval(x,y) can be called directly on a hand-built position.
static void replay(CAi &ai, int *moves, int n) {
    for (int i = 0; i < n; i++) ai.addMove(moves[i], i + 1);
    ai.moveNum = n;
    ai.tn = n + 1;
}

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
    ai.lvl++;
    ai.cc[ai.lvl][1] = ai.cc[ai.lvl - 1][1];
    ai.cc[ai.lvl][2] = ai.cc[ai.lvl - 1][2];
    ai.fr = ai.cp - 1 + ai.lvl;
    while (ai.fr > ai.np) ai.fr -= ai.np;
    ai.en = ai.fr + 1;
    if (ai.en > ai.np) ai.en = 1;
    for (int i = 0; i < 1444; i++) ai.pFk[i] = 0;
    ai.fhn = 0;
    ai.ferr = 0;
}

// evalMove(): build a position for `game`, then return Eval() of `mover`
// playing (ex,ey). A result >=10000 is a terminal win-level score.
static int evalMove(int game, int *moves, int n, int mover, int ex, int ey) {
    CAi ai(game, 2, true);
    replay(ai, moves, n);
    primeEval(ai, mover);
    return ai.Eval(ex, ey);
}

// --- case 1: unbreakable five is still a win in game 15 -------------------
// P1 completes (5,9)..(9,9) with no enemy stone anywhere near the run, so no
// run stone is pair-capturable. Boat must not weaken a clean five.
static void case1_unbreakable() {
    printf("case 1: unbreakable five stays terminal\n");
    int moves[] = { M(6,9),M(2,2), M(7,9),M(3,2), M(8,9),M(4,2), M(9,9),M(5,2) };
    int e15 = evalMove(15, moves, 8, 1, 5, 9);
    int e1  = evalMove( 1, moves, 8, 1, 5, 9);
    printf("  (game15 Eval=%d, game1 Eval=%d)\n", e15, e1);
    CHECK(e1  >= 10000, "game 1: clean five is a win");
    CHECK(e15 >= 10000, "game 15: clean five is still a win");
}

// --- case 2: breakable five is provisional in game 15 --------------------
// Five (5,9)..(9,9); the vertical pair (7,8),(7,9) crossing the run is
// capturable, so the whole five is provisional. Both flank orders tested.
static void case2_breakable() {
    printf("case 2: breakable five is provisional (both flank orders)\n");
    // 2a  enemy...empty : P2 at (7,7), (7,10) empty
    {
        int moves[] = { M(6,9),M(7,7), M(7,9),M(2,2), M(8,9),M(3,2),
                        M(9,9),M(4,2), M(7,8),M(5,2) };
        int e15 = evalMove(15, moves, 10, 1, 5, 9);
        int e1  = evalMove( 1, moves, 10, 1, 5, 9);
        printf("  2a (game15 Eval=%d, game1 Eval=%d)\n", e15, e1);
        CHECK(e1  >= 10000, "2a game 1: five wins outright");
        CHECK(e15 <  10000, "2a game 15: enemy...empty pair -> provisional");
    }
    // 2b  empty...enemy : P2 at (7,10), (7,7) empty
    {
        int moves[] = { M(6,9),M(7,10), M(7,9),M(2,2), M(8,9),M(3,2),
                        M(9,9),M(4,2), M(7,8),M(5,2) };
        int e15 = evalMove(15, moves, 10, 1, 5, 9);
        int e1  = evalMove( 1, moves, 10, 1, 5, 9);
        printf("  2b (game15 Eval=%d, game1 Eval=%d)\n", e15, e1);
        CHECK(e1  >= 10000, "2b game 1: five wins outright");
        CHECK(e15 <  10000, "2b game 15: empty...enemy pair -> provisional");
    }
}

// --- case 3: whole-run rule (six-run, only the far end capturable) --------
// Play (4,9) to make the six (4,9)..(9,9). The clean 5-window (4,9)..(8,9)
// has no capturable stone, but the WHOLE run includes (9,9), whose vertical
// pair (9,8),(9,9) is capturable -> provisional per the oracle.
static void case3_whole_run() {
    printf("case 3: whole-run rule (six with one capturable end)\n");
    int moves[] = { M(5,9),M(9,7), M(6,9),M(2,2), M(7,9),M(3,2),
                    M(8,9),M(4,2), M(9,9),M(5,2), M(9,8),M(6,2) };
    int e15 = evalMove(15, moves, 12, 1, 4, 9);
    int e1  = evalMove( 1, moves, 12, 1, 4, 9);
    printf("  (game15 Eval=%d, game1 Eval=%d)\n", e15, e1);
    CHECK(e1  >= 10000, "game 1: six-run wins outright");
    CHECK(e15 <  10000, "game 15: capturable end of whole run -> provisional");
}

// --- case 4: non-capturable look-alikes stay wins in game 15 --------------
static void case4_lookalikes() {
    printf("case 4: look-alikes remain wins in game 15\n");
    // (a) edge five: off-board flank never counts as an empty to play into.
    {
        int moves[] = { M(1,9),M(10,2), M(2,9),M(11,2), M(3,9),M(12,2), M(4,9),M(13,2) };
        int e15 = evalMove(15, moves, 8, 1, 0, 9);
        printf("  4a edge five (game15 Eval=%d)\n", e15);
        CHECK(e15 >= 10000, "4a game 15: edge five (off-board flank) still wins");
    }
    // (b) both flanks enemy: a pair sandwiched by two enemies is not capturable.
    {
        int moves[] = { M(6,9),M(7,7), M(7,9),M(7,10), M(8,9),M(2,2),
                        M(9,9),M(3,2), M(7,8),M(4,2) };
        int e15 = evalMove(15, moves, 10, 1, 5, 9);
        printf("  4b both flanks enemy (game15 Eval=%d)\n", e15);
        CHECK(e15 >= 10000, "4b game 15: pair flanked by two enemies still wins");
    }
    // (c) flanking "empty" is actually own: no empty to play into -> not capturable.
    {
        int moves[] = { M(6,9),M(7,7), M(7,9),M(2,2), M(8,9),M(3,2),
                        M(9,9),M(4,2), M(7,8),M(5,2), M(7,10),M(6,2) };
        int e15 = evalMove(15, moves, 12, 1, 5, 9);
        printf("  4c own flank (game15 Eval=%d)\n", e15);
        CHECK(e15 >= 10000, "4c game 15: pair with own flank still wins");
    }
}

// --- case 5: defender breaks the opponent's provisional five (end-to-end) --
// Opponent P2 lands a provisional five (5,9)..(9,9) with BOTH row ends already
// blocked by P1 (4,9)/(10,9), so plain Pente sees a dead, non-extending cluster
// worth only the +2 of the pair capture. Its pair (5,8),(5,9) is capturable by
// P1 playing (5,10) (P1 sits at (5,7)). P1 also owns a far-away double-three
// fork: (1,14),(2,14) + (3,12),(3,13) let the hub (3,14) make two open threes.
// Under real code the survived check makes every non-breaking line an instant
// loss, so P1 must play (5,10); with it neutered (Mutant B) -- or in plain
// Pente, which never saw the five as a win -- P1 prefers the stronger fork, so
// the move differs. Blocking the ends is what makes the fork out-score the
// capture: with an extensible five even plain Pente breaks it (verified).
static void case5_defender() {
    printf("case 5: defender breaks provisional five (level 4)\n");
    int moves[] = {
        M(4,9),  M(6,9),   // P1 blocks left end, P2 five
        M(10,9), M(7,9),   // P1 blocks right end, P2 five
        M(1,14), M(8,9),   // P1 fork, P2 five
        M(2,14), M(5,8),   // P1 fork, P2 pair partner
        M(3,12), M(5,9),   // P1 fork, P2 five stone
        M(3,13), M(16,16), // P1 fork, P2 dummy
        M(5,7),  M(9,9)    // P1 enemy flank, P2 completes the five (last move)
    };
    const int BREAK = M(5,10);
    const int FORK  = M(3,14);

    CAi ai15(15, 4, true);
    int mv15 = ai15.getMove(moves, 14);
    CAi ai1(1, 4, true);
    int mv1 = ai1.getMove(moves, 14);
    printf("  (game15 move=%d [break=%d, fork=%d], game1 move=%d)\n",
           mv15, BREAK, FORK, mv1);
    CHECK(mv15 == BREAK, "game 15: engine breaks the five by capturing at (5,10)");
    CHECK(mv1  != BREAK, "game 1: plain Pente is not forced to the break (behaviors differ)");
}

int main() {
    if (chdir("MMAIWASM") != 0) {
        // allow running from inside MMAIWASM too
    }
    case1_unbreakable();
    case2_breakable();
    case3_whole_run();
    case4_lookalikes();
    case5_defender();
    printf("\n%s (%d failure%s)\n", failures ? "BOATTEST FAIL" : "BOATTEST PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
