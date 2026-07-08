// Connect6 (game 13/14) fixtures (native, not wasm). Mirrors pooftest/boattest/
// opentest: drives the real engine through getMove()/dmov()/Eval() and asserts on
// internal board + packed two-stone return so the Connect6 turn rotation
// (ownerOf), the two-stone packed output, and Score6's 6-window win detection are
// all load-bearing (the mutation gate in the spec must break a case here).
// Test-only: expose internals so we can build positions and call Eval directly.
#define protected public
#include "Ai.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// Deterministic clock (same trick as regress.cpp/pooftest.cpp): binds Ai.o's
// time() so srand((unsigned)time(NULL)) inside Move() sees a fixed seed, making
// every search (and thus the packed result) deterministic.
extern "C" time_t time(time_t *t) {
    if (t) *t = 777;
    return 777;
}

static int failures = 0;
static int g_fallback = 0;   // total defensive second-stone fallback hits across cases
#define CHECK(cond, msg) do { \
    if (cond) { printf("  PASS: %s\n", msg); } \
    else { printf("  FAIL: %s\n", msg); failures++; } \
} while (0)

static inline int M(int x, int y) { return y * 19 + x; }
static inline int DM1(int packed) { return packed / 362; }
static inline int DM2(int packed) { return packed % 362; }

// Replay a move list through the same addMove()/dmov() path getMove() uses for
// input moves, leaving tn/moveNum positioned right after the last move (no
// search). Owners are assigned internally by dmov() (ownerOf for game 13).
static void replayOnly(CAi &ai, int *mv, int n) {
    for (int i = 0; i < n; i++) ai.addMove(mv[i], i + 1);
    ai.moveNum = n;
    ai.tn = n + 1;
}

// Hand-build the Eval() scratch state (mirrors pooftest primeEval) so Eval(x,y)
// can be called directly on a position placed straight into brd[0], with `mover`
// as fr. Connect6 has no captures/poofs, so cc stays 0 and the fukumi table is
// cleared for good measure.
static void primeEvalC6(CAi &ai, int mover) {
    for (int x = 0; x < 19; x++)
        for (int y = 0; y < 19; y++)
            ai.bd[x][y] = ai.brd[0][x][y];
    ai.cc[0][1] = 0; ai.cc[0][2] = 0;
    ai.cp = mover; ai.np = 2; ai.gf = 0; ai.multipbem = 0;
    ai.lvl = 0; ai.lvl++;                 // Tree()'s "do { lvl++; ..."
    ai.cc[ai.lvl][1] = 0; ai.cc[ai.lvl][2] = 0;
    ai.fr = mover;
    ai.en = 3 - mover;
    for (int i = 0; i < 1444; i++) ai.pFk[i] = 0;
    ai.fhn = 0; ai.ferr = 0;
}

// Can `oppO` complete 6 on row y=9 within one two-stone turn? A 6-window is a
// live threat iff it has no engine block, >=4 opp stones, and <=2 empties. The
// engine's two stones are brd[0]==engO plus the not-yet-applied m2 cell.
static bool oppThreatRow9(CAi &ai, int engO, int oppO, int m2cell) {
    for (int x0 = 0; x0 <= 13; x0++) {
        int opp = 0, eng = 0, empt = 0;
        for (int k = 0; k < 6; k++) {
            int xx = x0 + k, cell = 9 * 19 + xx;
            bool isEng = (ai.brd[0][xx][9] == engO) || (cell == m2cell);
            if (isEng) eng++;
            else if (ai.brd[0][xx][9] == oppO) opp++;
            else empt++;
        }
        if (eng == 0 && opp >= 4 && empt <= 2) return true;
    }
    return false;
}

// After a Connect6 getMove, brd[0] holds every input stone plus the engine's m1
// (m2 is decoded but not yet applied). Return true iff placing BOTH returned
// stones yields a run of >=6 `player` on any axis -- the real "win taken"
// invariant, independent of which stone of the pair happens to complete the run
// or in what order they were emitted.
static bool pairMakesSix(CAi &ai, int m1, int m2, int player) {
    static const int dxx[4] = {1,0,1,1}, dyy[4] = {0,1,1,-1};
    int b[19][19];
    for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = ai.brd[0][x][y];
    if (m1 >= 0 && m1 <= 360) b[m1%19][m1/19] = player;
    if (m2 >= 0 && m2 <= 360) b[m2%19][m2/19] = player;
    for (int y = 0; y < 19; y++) for (int x = 0; x < 19; x++) {
        if (b[x][y] != player) continue;
        for (int a = 0; a < 4; a++) {
            int n = 1, cx = x + dxx[a], cy = y + dyy[a];
            while (cx>=0 && cx<19 && cy>=0 && cy<19 && b[cx][cy]==player) { n++; cx+=dxx[a]; cy+=dyy[a]; }
            if (n >= 6) return true;
        }
    }
    return false;
}

// Drive a real packed getMove() at each listed level and assert the engine's two
// stones make a six. Used by the win-in-pair cases (open/half-open 4). Levels
// include L1 on purpose: at depth 1 the search never expands the same-player
// second stone, so ONLY Score6's win-in-pair terminal can find the win -- this is
// the assertion the mutation gate (disable that terminal) is designed to break.
static void assertWinAtLevels(const char *label, int *mv, int n, int mover,
                              const int *levels, int nl) {
    for (int li = 0; li < nl; li++) {
        int L = levels[li];
        CAi ai(13, L, true);
        int packed = ai.getMove(mv, n);
        g_fallback += ai.c6FallbackHits;
        int m1 = DM1(packed), m2 = DM2(packed);
        char buf[160];
        snprintf(buf, sizeof buf,
                 "%s L%d: pair completes six (m1=%d(%d,%d) m2=%d(%d,%d))",
                 label, L, m1, m1%19, m1/19, m2, m2%19, m2/19);
        CHECK(pairMakesSix(ai, m1, m2, mover), buf);
    }
}

// --- case 1: ownerOf owner sequence --------------------------------------
// idx : 0 1 2 3 4 5 6 7 8 9 10 11
// own : 1 2 2 1 1 2 2 1 1 2 2  1   (P1 opens with a lone stone at idx 0)
// Mutant A (ownerOf returns 1+idx%2 for c6) flips idx 2,6,10 -> P1 and fails here.
static void case1_ownerOf() {
    printf("case 1: ownerOf owner sequence\n");
    CAi ai(13, 2, true);
    const int want[12] = {1,2,2,1,1,2,2,1,1,2,2,1};
    int ok = 1;
    for (int i = 0; i < 12; i++) if (ai.ownerOf(i) != want[i]) ok = 0;
    CHECK(ok, "ownerOf(0..11) == 1,2,2,1,1,2,2,1,1,2,2,1");
}

// --- case 2: packed return decode ----------------------------------------
static void case2_packed_decode() {
    printf("case 2: packed return decode\n");
    { // count=0 -> lone opening stone, center forced, m2 sentinel 361.
        // Base 362 (not 361) makes the pack unambiguous: move 0 always returns
        // exactly 180*362+361 == 65521, and DM1/DM2 (packed/362, packed%362)
        // decode it uniquely -- no turn-context side information needed.
        CAi ai(13, 3, true);
        int mv[1];
        int packed = ai.getMove(mv, 0);
        CHECK(packed == 65521, "count=0 -> 180*362+361 == 65521 (center, m2 sentinel)");
        CHECK(DM1(packed) == 180 && DM2(packed) == 361,
              "count=0 unambiguous decode: m1 == 180, m2 == 361 sentinel");
    }
    { // count=1 -> engine (P2) plays a real two-stone turn
        CAi ai(13, 3, true);
        int mv[1] = { M(9,9) };           // P1's opening stone (idx 0)
        int packed = ai.getMove(mv, 1);
        g_fallback += ai.c6FallbackHits;
        int m1 = DM1(packed), m2 = DM2(packed);
        int m1x = m1 % 19, m1y = m1 / 19, m2x = m2 % 19, m2y = m2 / 19;
        CHECK(m1 >= 0 && m1 <= 360 && m2 >= 0 && m2 <= 360, "count=1 both stones in 0..360");
        CHECK(m1 != m2, "count=1 two distinct stones");
        CHECK(m1 != M(9,9) && m2 != M(9,9), "count=1 neither stone on the enemy cell");
        CHECK(ai.brd[0][m2x][m2y] <= 0, "count=1 m2 is a legal empty cell");
        (void)m1x; (void)m1y;
    }
}

// --- case 3: win-now (gap fill + extension), 2-stone turn ----------------
static void case3_win_now() {
    printf("case 3: win-now completes six\n");
    // Direct Eval: five contiguous P1 on y=9, both extensions win.
    {
        CAi ai(13, 3, true);
        int p1[5] = { M(4,9), M(5,9), M(6,9), M(7,9), M(8,9) };
        for (int i = 0; i < 5; i++) { int c = p1[i]; ai.brd[0][c%19][c/19] = 1; }
        primeEvalC6(ai, 1);
        int eL = ai.Eval(3, 9);   // extension left  -> 3..8
        int eR = ai.Eval(9, 9);   // extension right -> 4..9
        printf("    (Eval ext-left (3,9)=%d, ext-right (9,9)=%d)\n", eL, eR);
        CHECK(eL >= 10000, "Eval of extension cell (3,9) >= 10000");
        CHECK(eR >= 10000, "Eval of extension cell (9,9) >= 10000");
    }
    // Direct Eval: gap fill XXX_XX -> six.
    {
        CAi ai(13, 3, true);
        int p1[5] = { M(4,9), M(5,9), M(6,9), M(8,9), M(9,9) };
        for (int i = 0; i < 5; i++) { int c = p1[i]; ai.brd[0][c%19][c/19] = 1; }
        primeEvalC6(ai, 1);
        int eGap = ai.Eval(7, 9);   // gap fill -> 4..9 all P1
        printf("    (Eval gap-fill (7,9)=%d)\n", eGap);
        CHECK(eGap >= 10000, "Eval of gap-fill cell (7,9) >= 10000");
    }
    // getMove pair: engine (P1) at a real 2-stone turn with the 5 already down.
    {
        CAi ai(13, 4, true);
        // owners by index: P1=0,3,4,7,8 ; P2=1,2,5,6,9,10 ; engine (P1) to play 11,12
        int mv[11] = { M(4,9), M(2,2), M(3,2), M(5,9), M(6,9),
                       M(2,3), M(3,3), M(7,9), M(8,9), M(2,4), M(3,4) };
        int packed = ai.getMove(mv, 11);
        g_fallback += ai.c6FallbackHits;
        int m1 = DM1(packed), m2 = DM2(packed);
        printf("    (getMove packed=%d -> m1=%d(%d,%d) m2=%d(%d,%d))\n",
               packed, m1, m1%19, m1/19, m2, m2%19, m2/19);
        // The win is "taken" iff BOTH stones together make a six -- which stone of
        // the pair completes the run (and in what order) is an engine detail. With
        // five already down, the pair may complete the six (e.g. (9,9)+(?)) or push
        // it to a seven ((9,9)+(10,9)); both are wins. (Old check pinned m1 to the
        // single completing cell and broke when Score6's win-in-pair terminal let
        // the engine pick an equally-winning pair ordering.)
        CHECK(pairMakesSix(ai, m1, m2, 1), "engine's pair completes a six (win taken)");
        CHECK(ai.brd[0][m1%19][m1/19] == 1, "m1 was applied as engine (P1) stone");
    }
}

// --- case 4: overline (completing makes 7) still a win --------------------
static void case4_overline() {
    printf("case 4: overline scored as win\n");
    CAi ai(13, 3, true);
    int p1[6] = { M(4,9), M(5,9), M(6,9), M(7,9), M(8,9), M(10,9) };
    for (int i = 0; i < 6; i++) { int c = p1[i]; ai.brd[0][c%19][c/19] = 1; }
    primeEvalC6(ai, 1);
    int e = ai.Eval(9, 9);   // fills 9 -> 4..10 = seven contiguous
    printf("    (Eval overline-fill (9,9)=%d)\n", e);
    CHECK(e >= 10000, "Eval completing an overline (makes 7) >= 10000");
}

// --- case 5: block a naked open four --------------------------------------
static void case5_block() {
    printf("case 5: block the open four\n");
    // owners by index: P1=0,3,4,7,8 (corners, harmless) ; P2=1,2,5,6,9,10
    // P2's open four sits at (5,9),(6,9),(7,9),(8,9); it completes six within its
    // next two stones unless blocked. Engine is P1.
    int mv[11] = { M(0,0), M(5,9), M(6,9), M(18,0), M(0,18),
                   M(7,9), M(8,9), M(18,18), M(9,0), M(0,5), M(18,5) };

    CAi pre(13, 4, true);
    replayOnly(pre, mv, 11);
    bool threatBefore = oppThreatRow9(pre, /*eng*/1, /*opp*/2, /*m2*/-1);
    CHECK(threatBefore, "sanity: opponent open four is a live 6-threat pre-move");

    CAi ai(13, 4, true);
    int packed = ai.getMove(mv, 11);
    g_fallback += ai.c6FallbackHits;
    int m1 = DM1(packed), m2 = DM2(packed);
    printf("    (block packed=%d -> m1=%d(%d,%d) m2=%d(%d,%d))\n",
           packed, m1, m1%19, m1/19, m2, m2%19, m2/19);
    bool threatAfter = oppThreatRow9(ai, /*eng*/1, /*opp*/2, /*m2*/m2);
    // engine could instead have an outright faster win of its own (it can't here,
    // its stones are scattered corners) -- accept either per spec.
    bool fasterWin = false;
    bool bothEnds = ((m1 == M(4,9) || m2 == M(4,9)) && (m1 == M(9,9) || m2 == M(9,9)));
    printf("    (bothEndsCovered=%d threatAfter=%d)\n", bothEnds, threatAfter);
    CHECK(!threatAfter || fasterWin, "engine pair neutralizes the open four");
}

// --- case 6: an exactly-blocked five is NOT a win ------------------------
static void case6_blocked_five() {
    printf("case 6: blocked five is not a win\n");
    CAi ai(13, 3, true);
    // P1: 5,6 and 8,9 on y=9 (gap at 7); enemy P2 flanks at 4 and 10 -> the five
    // 5..9 can never extend to six.
    int p1[4] = { M(5,9), M(6,9), M(8,9), M(9,9) };
    int p2[2] = { M(4,9), M(10,9) };
    for (int i = 0; i < 4; i++) { int c = p1[i]; ai.brd[0][c%19][c/19] = 1; }
    for (int i = 0; i < 2; i++) { int c = p2[i]; ai.brd[0][c%19][c/19] = 2; }
    primeEvalC6(ai, 1);
    int e = ai.Eval(7, 9);   // completes a five that is dead on both ends
    printf("    (Eval blocked-five completion (7,9)=%d)\n", e);
    CHECK(e < 10000, "Eval completing a fully-blocked five < 10000");
}

// --- case 7: game 13 never captures; game 1 with the same moves does -----
static void case7_no_capture() {
    printf("case 7: no captures under Connect6\n");
    // O X X O flank on y=9. In Pente the last stone (8,9) captures the pair.
    int mv[5] = { M(5,9), M(6,9), M(2,2), M(7,9), M(8,9) };
    { // game 13: no capture mechanic; the two pair cells stay on the board.
        CAi ai(13, 2, true);
        ai.getMove(mv, 5);
        CHECK(ai.ccc[0][1] == 0 && ai.ccc[0][2] == 0, "game 13: no capture credit");
        CHECK(ai.brd[0][6][9] > 0 && ai.brd[0][7][9] > 0, "game 13: pair cells still on board");
        CHECK(ai.brd[0][5][9] == 1, "game 13: input stone (5,9) intact");
    }
    { // game 1 (Pente): the same replay DOES capture (cross-check).
        CAi ai(1, 2, true);
        ai.getMove(mv, 5);
        CHECK(ai.ccc[0][1] == 2, "game 1: P1 credited +2 for the capture");
        CHECK(ai.brd[0][6][9] == -1 && ai.brd[0][7][9] == -1, "game 1: captured pair removed");
    }
}

// --- case 8: search integrity + both stones legal -----------------------
static void integrity_c6(int *mv, int n, int level, const char *name) {
    CAi ai(13, level, true);
    int packed = ai.getMove(mv, n);
    g_fallback += ai.c6FallbackHits;
    int m1 = DM1(packed), m2 = DM2(packed);
    int bad = 0;
    for (int x = 0; x < 19; x++)
        for (int y = 0; y < 19; y++)
            if (ai.brd[0][x][y] > 0 && ai.bd[x][y] != ai.brd[0][x][y]) bad++;
    char buf[128];
    snprintf(buf, sizeof buf, "%s: bd == brd[0] on every stone cell", name);
    CHECK(bad == 0, buf);
    snprintf(buf, sizeof buf, "%s: m1=%d,m2=%d both legal & distinct", name, m1, m2);
    bool ok = m1 >= 0 && m1 <= 360 && m2 >= 0 && m2 <= 360 && m1 != m2 &&
              ai.brd[0][m2%19][m2/19] <= 0 &&      // m2 not yet applied -> empty
              ai.brd[0][m1%19][m1/19] > 0;         // m1 applied as engine stone
    CHECK(ok, buf);
}
static void case8_integrity() {
    printf("case 8: search integrity\n");
    int a[5] = { M(9,9), M(8,8), M(10,10), M(7,9), M(11,9) };          // count 5 (2 stones)
    integrity_c6(a, 5, 3, "posA n5 L3");
    int b[7] = { M(9,9), M(8,10), M(10,8), M(9,11), M(7,9), M(11,9), M(9,7) }; // count 7
    integrity_c6(b, 7, 4, "posB n7 L4");
    int c[9] = { M(9,9), M(8,8), M(10,10), M(8,10), M(10,8),
                 M(7,7), M(11,11), M(7,11), M(11,7) };                 // count 9
    integrity_c6(c, 9, 3, "posC n9 L3");
}

// --- case 9: determinism -------------------------------------------------
static void case9_determinism() {
    printf("case 9: determinism\n");
    int mv[5] = { M(9,9), M(8,8), M(10,10), M(7,9), M(11,9) };
    CAi a1(13, 4, true); int p1 = a1.getMove(mv, 5); g_fallback += a1.c6FallbackHits;
    CAi a2(13, 4, true); int p2 = a2.getMove(mv, 5); g_fallback += a2.c6FallbackHits;
    printf("    (run1=%d run2=%d)\n", p1, p2);
    CHECK(p1 == p2, "same input twice -> identical packed result");
}

// --- case 10: single-search board integrity (stone 1 only) ---------------
// case 8's integrity_c6() only checks bd == brd[0] AFTER getMove() fully
// returns both stones -- by then the SECOND search's Move() entry has
// already re-copied brd[0] into bd, so a bug that leaves bd stale right
// after the FIRST search would go unnoticed. This case replays a mid-game
// position and invokes exactly the sequence getMove() uses for stone 1
// (seat = ownerOf(count); cmove(); fall back to Move() if cmove() punts),
// then checks bd immediately afterward -- before any second search exists
// to paper over a stale bd.
static void case10_single_search_integrity() {
    printf("case 10: single-search board integrity (stone 1 only)\n");
    int mv[7] = { M(9,9), M(8,10), M(10,8), M(9,11), M(7,9), M(11,9), M(9,7) }; // mid-game, count=7
    CAi ai(13, 3, true);
    replayOnly(ai, mv, 7);
    int count = 7;
    ai.seat = ai.ownerOf(count);
    int m1 = ai.cmove();
    if (m1 == -1) m1 = ai.Move();
    int bad = 0;
    for (int x = 0; x < 19; x++)
        for (int y = 0; y < 19; y++)
            if (ai.brd[0][x][y] > 0 && ai.bd[x][y] != ai.brd[0][x][y]) bad++;
    CHECK(bad == 0, "single search (stone 1 only): bd == brd[0] on every stone cell");
    CHECK(m1 >= 0 && m1 <= 360 && ai.brd[0][m1%19][m1/19] <= 0,
          "single search (stone 1 only): m1 is a legal empty cell");
}

// --- case 11: pinned search regression (player-rotation guard) -----------
// These two positions' packed results are pinned to the exact values the
// fixed engine produces (computed once, after the Tree() backtrack fix).
// The pins exist ONLY to catch player-rotation regressions in search (e.g.
// reverting the backtrack fr recompute to a bare fr--/fr=np, which silently
// corrupts fr for one player on every pop and changes the move actually
// returned) -- they are NOT a general correctness oracle. If a deliberate
// change to Eval()/Score6/search shape/ordering changes these values, the
// pins must be updated by hand (after confirming the new packed values are
// sane: legal cells, distinct stones, consistent with getMove()'s other
// invariants) -- do not treat a pin failure as automatically "the pin is
// stale" without checking it isn't a real regression first.
static void case11_pinned_regression() {
    printf("case 11: pinned search regression\n");
    { // posA (same position as case 8's posA), level 3
        CAi ai(13, 3, true);
        int a[5] = { M(9,9), M(8,8), M(10,10), M(7,9), M(11,9) };
        int packed = ai.getMove(a, 5);
        g_fallback += ai.c6FallbackHits;
        printf("    (posA L3 packed=%d)\n", packed);
        CHECK(packed == 64257, "posA L3 packed result pinned to 64257");
    }
    { // case 5's block position, level 3
        CAi ai(13, 3, true);
        int mv[11] = { M(0,0), M(5,9), M(6,9), M(18,0), M(0,18),
                       M(7,9), M(8,9), M(18,18), M(9,0), M(0,5), M(18,5) };
        int packed = ai.getMove(mv, 11);
        g_fallback += ai.c6FallbackHits;
        printf("    (block-pos L3 packed=%d)\n", packed);
        CHECK(packed == 63531, "block-position L3 packed result pinned to 63531");
    }
}

// --- case 12: win-in-pair, own OPEN FOUR while opponent has a live threat ---
// The engine (P1) already has an open four (5,9)-(8,9). The opponent (P2) has
// its OWN open four (5,5)-(8,5) elsewhere. P1 moves first this turn and can
// complete six THIS turn -- taking its own win strictly dominates blocking.
// Without Score6's win-in-pair terminal the extension scores only ~w6[5]=1600
// (nonterminal), while blocking the opponent four bids up to the *4 defense
// clamp, so a depth-1 (level 1) search blocks and MISSES the win. Levels 1/2/4.
static void case12_open4_win() {
    printf("case 12: open four win taken (opponent has own open four)\n");
    // owners: P1 = idx 0,3,4,7,8 ; P2 = idx 1,2,5,6,9,10 ; P1 plays idx 11,12.
    int mv[11] = { M(5,9), M(5,5), M(6,5), M(6,9), M(7,9),
                   M(7,5), M(8,5), M(8,9), M(2,2), M(0,18), M(18,18) };
    int levels[3] = { 1, 2, 4 };
    assertWinAtLevels("open4", mv, 11, 1, levels, 3);
}

// --- case 13: win-in-pair, own HALF-OPEN FOUR (one end enemy-blocked) --------
// P1 four (5,9)-(8,9) with the left end blocked by a P2 stone at (4,9); the only
// winning line is rightward, (9,9)+(10,9). The opponent also has an open four on
// row 5, so (as in case 12) a search too shallow to expand the second stone will
// block instead of win unless Score6 terminalizes the win-in-pair. Levels 1/2/4.
static void case13_halfopen4_win() {
    printf("case 13: half-open four win taken (blocked one end)\n");
    int mv[11] = { M(5,9), M(4,9), M(5,5), M(6,9), M(7,9),
                   M(6,5), M(7,5), M(8,9), M(2,2), M(8,5), M(18,18) };
    int levels[3] = { 1, 2, 4 };
    assertWinAtLevels("half-open4", mv, 11, 1, levels, 3);
}

// --- case 14: priority -- own open four vs TWO opponent open fours -----------
// A denser competing-threat board (opponent open fours on rows 5 and 13). The
// engine must still take its own row-9 win rather than start blocking. This is
// the "engine open 4 AND opponent scary threat elsewhere" priority test with a
// threat strong enough to actually contest the score (an open three does not
// out-bid the extension even in the unfixed engine, so it would not exercise the
// fix). count=15 so P1 has a genuine two-stone turn (idx 15,16). Levels 1/2/4.
static void case14_priority_win() {
    printf("case 14: priority -- own open four beats two opponent open fours\n");
    // owners: P1 = idx 0,3,4,7,8,11,12 ; P2 = idx 1,2,5,6,9,10,13,14 ; P1 plays 15,16.
    int mv[15] = { M(5,9), M(5,5), M(6,5), M(6,9), M(7,9),   // own r9 + opp r5
                   M(7,5), M(8,5), M(8,9), M(1,1), M(5,13),  // filler + opp r13
                   M(6,13), M(1,17), M(17,1), M(7,13), M(8,13) };
    int levels[3] = { 1, 2, 4 };
    assertWinAtLevels("priority", mv, 15, 1, levels, 3);
}

// --- case 15: defense -- neutralize an opponent open four (engine cannot win) -
// The opponent (P2) has an open four on row 9; the engine (P1) has only scattered
// corner stones and cannot make its own six, so its pair must kill the threat:
// no live 6-window with 5 opponent stones + an empty may remain. Complements
// case 5 by exercising the same requirement at both a shallow and a deep level.
// Note: this defense already held before the win-in-pair terminal (via the *4
// blocking weight), so this case does not regression-cover this diff -- it guards
// against future eval changes weakening open-four defense. Levels 2 and 4.
static void case15_defense_open4() {
    printf("case 15: defense -- opponent open four neutralized\n");
    // owners: P1 = idx 0,3,4,7,8 (corners) ; P2 = idx 1,2,5,6 (open4) + 9,10 filler.
    int mv[11] = { M(0,0), M(5,9), M(6,9), M(18,0), M(0,18),
                   M(7,9), M(8,9), M(18,18), M(1,1), M(0,5), M(18,5) };
    CAi pre(13, 4, true);
    replayOnly(pre, mv, 11);
    bool before = oppThreatRow9(pre, /*eng*/1, /*opp*/2, /*m2*/-1);
    CHECK(before, "sanity: opponent open four is a live 6-threat pre-move");
    int levels[2] = { 2, 4 };
    for (int li = 0; li < 2; li++) {
        int L = levels[li];
        CAi ai(13, L, true);
        int packed = ai.getMove(mv, 11);
        g_fallback += ai.c6FallbackHits;
        int m2 = DM2(packed);
        bool after = oppThreatRow9(ai, /*eng*/1, /*opp*/2, /*m2*/m2);
        char buf[128];
        snprintf(buf, sizeof buf, "L%d: opponent open four neutralized (after=%d)", L, after);
        CHECK(!after, buf);
    }
}

int main() {
    if (chdir("MMAIWASM") != 0) { /* allow running inside MMAIWASM too */ }
    case1_ownerOf();
    case2_packed_decode();
    case3_win_now();
    case4_overline();
    case5_block();
    case6_blocked_five();
    case7_no_capture();
    case8_integrity();
    case9_determinism();
    case10_single_search_integrity();
    case11_pinned_regression();
    case12_open4_win();
    case13_halfopen4_win();
    case14_priority_win();
    case15_defense_open4();
    printf("\nFALLBACK HITS (defensive 2nd-stone guard): %d\n", g_fallback);
    printf("%s (%d failure%s)\n", failures ? "C6TEST FAIL" : "C6TEST PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
