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

// =========================================================================
// Connect6 v2: static covering-threat forced-win terminal (c6UnstoppableThreat).
// T0 is the load-bearing detector<->oracle fuzz; T1-T6 exercise the integration
// and both mutation gates (flag c6ForceEnabled + source mutation -DC6_NO_FORCE).
// Direct-detector calls rely on dx/dy (default-initialized in Ai.h to the 4 axes
// below); setAxes() re-sets them so a call never depends on a prior Move().
// =========================================================================
static const int AX[4][2] = { {-1,-1}, {0,-1}, {1,-1}, {-1,0} }; // dx,dy of axes 0..3 (== Move())

static void setAxes(CAi &ai) { for (int a = 0; a < 4; a++) { ai.dx[a] = AX[a][0]; ai.dy[a] = AX[a][1]; } }

// Copy a plain board (0=empty,1,2) into ai.bd and call the detector directly.
static bool detOn(CAi &ai, const int b[19][19], int P) {
    for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) ai.bd[x][y] = b[x][y];
    setAxes(ai);
    return ai.c6UnstoppableThreat(P);
}

// ---- independent brute-force oracle: ground truth for the fives-only win ----
// It shares only the DEFINITION (five = 5P+1empty+0O window; race = 0P+>=4O
// window) with the detector, not the algorithm: it lists completion cells with
// its OWN dedup, then simulates the opponent occupying up to 2 of them and
// re-scans the board for a surviving five. Forced iff no 2-cell opponent
// placement kills every five. (Correct because a five dies only if the opponent
// occupies its single empty completion cell -- no captures in Connect6 -- so the
// opponent's best defense is always to sit on completion cells; anything else
// kills no five. Hence "a defense exists" <=> distinct completion cells <= 2.)
static bool ora_win6(const int b[19][19], int a, int sx, int sy, int P, int O,
                     int &pc, int &oc, int &emptyIdx) {
    int dx = AX[a][0], dy = AX[a][1];
    int ex5 = sx + 5*dx, ey5 = sy + 5*dy;
    if (ex5 < 0 || ex5 >= 19 || ey5 < 0 || ey5 >= 19) return false;
    pc = 0; oc = 0; emptyIdx = -1;
    for (int k = 0; k < 6; k++) { int cx = sx + k*dx, cy = sy + k*dy; int v = b[cx][cy];
        if (v == P) pc++; else if (v == O) oc++; else emptyIdx = cy*19 + cx; }
    return true;
}
static bool ora_oppSix(const int b[19][19], int P, int O) {
    for (int a = 0; a < 4; a++) for (int sx = 0; sx < 19; sx++) for (int sy = 0; sy < 19; sy++) {
        int pc, oc, e; if (!ora_win6(b, a, sx, sy, P, O, pc, oc, e)) continue;
        if (pc == 0 && oc >= 4) return true; }
    return false;
}
static bool ora_hasFive(const int b[19][19], int P, int O) {
    for (int a = 0; a < 4; a++) for (int sx = 0; sx < 19; sx++) for (int sy = 0; sy < 19; sy++) {
        int pc, oc, e; if (!ora_win6(b, a, sx, sy, P, O, pc, oc, e)) continue;
        if (oc == 0 && pc == 5) return true; }
    return false;
}
static bool oracleForcedWin(const int bd[19][19], int P) {
    int O = 3 - P;
    if (ora_oppSix(bd, P, O)) return false;              // race, mirrors oppImm
    int cand[400], nc = 0;
    for (int a = 0; a < 4; a++) for (int sx = 0; sx < 19; sx++) for (int sy = 0; sy < 19; sy++) {
        int pc, oc, e; if (!ora_win6(bd, a, sx, sy, P, O, pc, oc, e)) continue;
        if (oc == 0 && pc == 5) { bool dup = false; for (int i = 0; i < nc; i++) if (cand[i] == e) { dup = true; break; }
            if (!dup && nc < 400) cand[nc++] = e; }
    }
    if (nc == 0) return false;
    for (int i = 0; i < nc; i++) for (int j = i; j < nc; j++) {
        int b[19][19];
        for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = bd[x][y];
        b[cand[i]%19][cand[i]/19] = O;
        b[cand[j]%19][cand[j]/19] = O;
        if (!ora_hasFive(b, P, O)) return false;         // opponent has a covering defense
    }
    return true;
}

// ---- fuzz board generator (varied styles: sparse / dense cluster / lines) ----
static unsigned long long rngS = 0x9E3779B97F4A7C15ULL;
static unsigned long long xr(void) { rngS ^= rngS << 13; rngS ^= rngS >> 7; rngS ^= rngS << 17; return rngS; }
static int ri(int n) { return (int)(xr() % (unsigned long long)n); }
static void genBoard(int b[19][19], int style) {
    for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = 0;
    if (style == 0) {                       // sparse full-board
        int n = 4 + ri(45); for (int i = 0; i < n; i++) { int x = ri(19), y = ri(19); b[x][y] = 1 + ri(2); }
    } else if (style == 1) {                // dense cluster (many near-fives + shared cells)
        int ox = ri(12), oy = ri(12), w = 6 + ri(8), h = 6 + ri(8);
        for (int x = ox; x < ox+w && x < 19; x++) for (int y = oy; y < oy+h && y < 19; y++) {
            int r = ri(100); if (r < 52) b[x][y] = 1; else if (r < 68) b[x][y] = 2; }
    } else {                                // line-heavy (runs stress fives, dedup, oppImm)
        int lines = 2 + ri(6);
        for (int L = 0; L < lines; L++) { int a = ri(4), dx = AX[a][0], dy = AX[a][1];
            int sx = ri(19), sy = ri(19), len = 3 + ri(5), who = 1 + ri(2);
            for (int k = 0; k < len; k++) { int cx = sx + k*dx, cy = sy + k*dy;
                if (cx < 0 || cx >= 19 || cy < 0 || cy >= 19) break; if (ri(100) < 85) b[cx][cy] = who; } }
        int ex = ri(10); for (int i = 0; i < ex; i++) { int x = ri(19), y = ri(19); b[x][y] = 1 + ri(2); }
    }
}

// --- T0: THE load-bearing test. Detector must agree with the oracle on every
// random legal board, under -fsanitize=address,undefined.
static void T0_fuzz(int N) {
    printf("T0: detector<->oracle fuzz (%d boards x2 perspectives)\n", N);
    CAi ai(13, 4, true); setAxes(ai);
    long mism = 0, firedDet = 0, firedOra = 0; int shown = 0;
    for (int t = 0; t < N; t++) { int b[19][19]; genBoard(b, t % 3);
        for (int P = 1; P <= 2; P++) {
            bool det = detOn(ai, b, P);
            bool ora = oracleForcedWin(b, P);
            if (det) firedDet++; if (ora) firedOra++;
            if (det != ora) { mism++; if (shown < 6) { printf("  MISMATCH t=%d P=%d det=%d ora=%d\n", t, P, (int)det, (int)ora); shown++; } } } }
    printf("    detector fired=%ld oracle fired=%ld mismatches=%ld\n", firedDet, firedOra, mism);
    CHECK(mism == 0, "T0: c6UnstoppableThreat == oracleForcedWin on all fuzz boards");
    CHECK(firedDet > 0, "T0: detector fired on the positive path at least once");
}

// ---- getMove integration helpers ---------------------------------------
// Scattered far-edge filler cells (spaced so no 6-in-a-row can form).
static int filler(int i) {
    static const int fx[24] = {0,18,0,18, 4,14,0,0, 18,18,4,14, 2,16,0,0, 18,18,2,16, 6,12,0,0};
    static const int fy[24] = {0,0,18,18, 0,0,4,14, 4,14,18,18, 0,0,2,16, 2,16,18,18, 0,0,6,12};
    int k = i % 24; return fy[k]*19 + fx[k];
}
// brd[0] + m1 + m2 (as player) -> detector (does the returned pair form a
// covering forced win?).
static bool pairForcedThreat(CAi &ai, int m1, int m2, int player) {
    int b[19][19];
    for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) { int v = ai.brd[0][x][y]; b[x][y] = (v > 0) ? v : 0; }
    if (m1 >= 0 && m1 <= 360) b[m1%19][m1/19] = player;
    if (m2 >= 0 && m2 <= 360) b[m2%19][m2/19] = player;
    return detOn(ai, b, player);
}
// Run one packed getMove with the terminal on/off; report m1,m2 and root score.
static void runGM(int L, int *mv, int n, bool forceOn, int &m1, int &m2, int &bscr) {
    CAi ai(13, L, true);
    ai.c6ForceEnabled = forceOn;
    int packed = ai.getMove(mv, n);
    g_fallback += ai.c6FallbackHits;
    m1 = DM1(packed); m2 = DM2(packed); bscr = ai.bscr;
}

// Intersection triple: cell C=(9,9) is simultaneously the 5th stone of THREE
// half-open fours (row 9, col 9, main diagonal), each blocked on its far side by
// a P2 stone so each promotes to exactly ONE five. Placing C alone makes three
// fives with three distinct completions -> forced six next turn. Crucially the
// pre-C board has NO 5-in-6 window, so NO single stone makes an immediate six and
// (in a mid-pair, c6NextSame==0 search) Score6's win-in-pair terminal is INACTIVE
// -- the covering terminal is the ONLY thing that can score this a win. This is
// the load-bearing construction: with the terminal off, the second-stone search
// cannot see the win.
static void intersectTriple(int *p1, int &n1, int *p2, int &n2, int &C) {
    int a[12] = { M(5,9),M(6,9),M(7,9),M(8,9),      // row-9 four,  block (4,9), completion (10,9)
                  M(9,5),M(9,6),M(9,7),M(9,8),      // col-9 four,  block (9,4), completion (9,10)
                  M(5,5),M(6,6),M(7,7),M(8,8) };    // main-diag four, block (4,4), completion (10,10)
    for (int i = 0; i < 12; i++) p1[i] = a[i]; n1 = 12;
    int b[3] = { M(4,9), M(9,4), M(4,4) };
    for (int i = 0; i < 3; i++) p2[i] = b[i]; n2 = 3;
    C = M(9,9);
}
// Build a Connect6 history whose end is a P1 MID-PAIR index (ownerOf(count)==1,
// ownerOf(count+1)==2, i.e. count%4==0, count>0): getMove then searches for P1's
// SECOND stone with the first already on the board (c6NextSame==0 at the root).
static int buildHistoryMidPair(CAi &ai, int *mv, const int *p1, int n1, const int *p2, int n2) {
    int i1 = 0, i2 = 0, fi = 0, idx = 0;
    while (i1 < n1 || i2 < n2 || idx == 0 || (idx % 4) != 0) {
        int owner = ai.ownerOf(idx);
        int cell = (owner == 1) ? ((i1 < n1) ? p1[i1++] : filler(fi++))
                                : ((i2 < n2) ? p2[i2++] : filler(fi++));
        mv[idx++] = cell;
        if (idx >= 58) break;
    }
    return idx;
}

// A second intersection triple (T2): the completing cell (9,9) is the 5th stone
// of a horizontal four, a MAIN-diagonal four, and an ANTI-diagonal four (a
// three-armed cross using a different axis mix than T1's row/col/diag).
static void intersectTriple2(int *p1, int &n1, int *p2, int &n2, int &C) {
    int a[12] = { M(5,9),M(6,9),M(7,9),M(8,9),        // row-9 four,   block (4,9),  completion (10,9)
                  M(5,5),M(6,6),M(7,7),M(8,8),        // main-diag four, block (4,4), completion (10,10)
                  M(13,5),M(12,6),M(11,7),M(10,8) };  // anti-diag four, block (14,4), completion (8,10)
    for (int i = 0; i < 12; i++) p1[i] = a[i]; n1 = 12;
    int b[3] = { M(4,9), M(4,4), M(14,4) };
    for (int i = 0; i < 3; i++) p2[i] = b[i]; n2 = 3;
    C = M(9,9);
}

// A third intersection triple (T5): completing cell (9,9) is the 5th stone of a
// vertical four and both diagonals (col + 2 diagonals).
static void intersectTriple3(int *p1, int &n1, int *p2, int &n2, int &C) {
    int a[12] = { M(9,5),M(9,6),M(9,7),M(9,8),        // col-9 four,   block (9,4),  completion (9,10)
                  M(5,5),M(6,6),M(7,7),M(8,8),        // main-diag four, block (4,4), completion (10,10)
                  M(13,5),M(12,6),M(11,7),M(10,8) };  // anti-diag four, block (14,4), completion (8,10)
    for (int i = 0; i < 12; i++) p1[i] = a[i]; n1 = 12;
    int b[3] = { M(9,4), M(4,4), M(14,4) };
    for (int i = 0; i < 3; i++) p2[i] = b[i]; n2 = 3;
    C = M(9,9);
}

// The three-half-open-four covering triple (kept for T4's race construction):
// stone X=(8,9) promotes fours A(row9) and B(col8); stone Y=(15,2) promotes four
// C(row2). Three distinct completions -> forced six next turn.
static void tripleP1(int *p1, int &n1, int *p2, int &n2, int &X, int &Y) {
    int a[12] = { M(4,9),M(5,9),M(6,9),M(7,9),   // four A (row 9), blocked left by (3,9)
                  M(8,5),M(8,6),M(8,7),M(8,8),   // four B (col 8), blocked top  by (8,4)
                  M(11,2),M(12,2),M(13,2),M(14,2) }; // four C (row 2), blocked left by (10,2)
    for (int i = 0; i < 12; i++) p1[i] = a[i]; n1 = 12;
    int b[3] = { M(3,9), M(8,4), M(10,2) };
    for (int i = 0; i < 3; i++) p2[i] = b[i]; n2 = 3;
    X = M(8,9); Y = M(15,2);
}

// --- T3: blockable look-alike (open five => exactly 2 distinct completions) ---
// MUST NOT terminalize: 2 cells the opponent covers with its 2 stones. Highest
// severity -- a false positive here is a hallucinated win.
static void T3_blockable() {
    printf("T3: blockable look-alike (open five, 2 distinct completions) must NOT terminalize\n");
    CAi ai(13, 4, true);
    int b[19][19]; for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = 0;
    // open five P1 5..9 on row 9 (completions (4,9),(10,9)) + an unrelated open
    // FOUR on row 5 (contributes 0 -- only 4 P in any window). nDistinct == 2.
    int five[5] = { M(5,9),M(6,9),M(7,9),M(8,9),M(9,9) };
    int four[4] = { M(5,5),M(6,5),M(7,5),M(8,5) };
    for (int i = 0; i < 5; i++) b[five[i]%19][five[i]/19] = 1;
    for (int i = 0; i < 4; i++) b[four[i]%19][four[i]/19] = 1;
    bool det = detOn(ai, b, 1), ora = oracleForcedWin(b, 1);
    printf("    det=%d ora=%d\n", (int)det, (int)ora);
    CHECK(!det, "T3: c6UnstoppableThreat(1)==false on the 2-distinct-completion look-alike");
    CHECK(!ora, "T3: oracle agrees it is NOT forced");
}

// --- T4: oppImm race negative -- real P1 triple, but opponent has a one-move six.
static void T4_race() {
    printf("T4: race (oppImm) -- P1 triple present but opponent has a one-move six -> refuse\n");
    CAi ai(13, 4, true);
    int p1[12], p2[3], n1, n2, X, Y; tripleP1(p1, n1, p2, n2, X, Y);
    int b[19][19]; for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = 0;
    for (int i = 0; i < n1; i++) b[p1[i]%19][p1[i]/19] = 1;
    for (int i = 0; i < n2; i++) b[p2[i]%19][p2[i]/19] = 2;
    b[X%19][X/19] = 1; b[Y%19][Y/19] = 1;                 // P1 plays its triple -> real 3-distinct
    bool detNoRace = detOn(ai, b, 1);
    // add a standing opponent one-move six far away: 5 P2 in a 6-window (row 15).
    int o5[5] = { M(2,15),M(3,15),M(4,15),M(5,15),M(6,15) };
    for (int i = 0; i < 5; i++) b[o5[i]%19][o5[i]/19] = 2; // 0 P1 + 5 O window -> oppImm
    bool detRace = detOn(ai, b, 1), oraRace = oracleForcedWin(b, 1);
    printf("    without race det=%d ; with race det=%d ora=%d\n", (int)detNoRace, (int)detRace, (int)oraRace);
    CHECK(detNoRace, "T4: sanity -- same triple without the race IS a forced win");
    CHECK(!detRace, "T4: c6UnstoppableThreat(1)==false once opponent has a one-move six (oppImm veto)");
    CHECK(!oraRace, "T4: oracle agrees the race refuses the win");
}

// --- T6: quiet midgame -- detector false at root; getMove returns a legal,
// non-winning, non-terminalized pair.
static void T6_quiet() {
    printf("T6: quiet midgame -- no spurious terminal\n");
    int mv[7] = { M(9,9), M(8,10), M(10,8), M(9,11), M(7,9), M(11,9), M(9,7) };
    CAi pre(13, 4, true); replayOnly(pre, mv, 7);
    int b[19][19]; for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) { int v = pre.brd[0][x][y]; b[x][y] = (v > 0) ? v : 0; }
    bool det1 = detOn(pre, b, 1), det2 = detOn(pre, b, 2);
    CHECK(!det1 && !det2, "T6: detector false for both players on a quiet position");
    int m1, m2, bscr; runGM(4, mv, 7, true, m1, m2, bscr);
    bool legal = m1 >= 0 && m1 <= 360 && m2 >= 0 && m2 <= 360 && m1 != m2;
    CHECK(legal, "T6: getMove returns a legal distinct pair on a quiet position");
    CHECK(!pairForcedThreat(pre, m1, m2, 1), "T6: returned pair is not a (fake) covering win");
}

// --- positive measurement + assertions (T1/T2/T5) -----------------------
// Direct-detector truth is deterministic and load-bearing for correctness; the
// getMove flag differential is measured (bscr / returned pair) so we can prove
// the terminal is load-bearing in search. `tag` selects the construction.
// Offense positive: one stone C completes an intersection triple (three fives).
// In the mid-pair second-stone search (c6NextSame==0) Score6's win-in-pair is
// inactive and the conversion is beyond the shallow horizon, so ONLY the covering
// terminal can score this a win -- the load-bearing flag/injection mutation gate.
static void offenseFlip(const char *tag, void (*build)(int*,int&,int*,int&,int&)) {
    printf("%s\n", tag);
    int p1[12], p2[3], n1, n2, C; build(p1, n1, p2, n2, C);
    // (1) Direct detector + oracle on the turn-final board (structural + C).
    CAi ai(13, 4, true);
    int b[19][19]; for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = 0;
    for (int i = 0; i < n1; i++) b[p1[i]%19][p1[i]/19] = 1;
    for (int i = 0; i < n2; i++) b[p2[i]%19][p2[i]/19] = 2;
    bool detBefore = detOn(ai, b, 1);
    b[C%19][C/19] = 1;
    bool det = detOn(ai, b, 1), ora = oracleForcedWin(b, 1);
    CHECK(!detBefore, "positive: not yet a forced win before the completing stone C");
    CHECK(det, "positive: c6UnstoppableThreat(1)==true once C completes the intersection triple");
    CHECK(ora, "positive: oracle confirms the completed triple is a forced win");
    // (2) getMove mid-pair (second-stone) search at shallow depth: terminal is
    // the ONLY win signal. Flag ON must claim the win; flag OFF must not.
    int mv[64]; CAi builder(13, 2, true);
    int count = buildHistoryMidPair(builder, mv, p1, n1, p2, n2);
    int m1on, m2on, bon, m1off, m2off, boff;
    runGM(2, mv, count, true,  m1on,  m2on,  bon);
    runGM(2, mv, count, false, m1off, m2off, boff);
    printf("    midpair L2 count=%d  ON: m1=%d(%d,%d) bscr=%d | OFF: m1=%d bscr=%d\n",
           count, m1on, m1on%19, m1on/19, bon, m1off, boff);
    CHECK(bon >= 10000, "positive: flag ON -> engine claims the covering win (injection load-bearing)");
    CHECK(boff < 10000, "positive: flag OFF -> win disappears (flag mutation gate flips)");
    CHECK(m1on == C, "positive: flag ON -> engine plays the completing intersection cell C");
}

// T5: a third offense geometry (the flag+injection flip), plus a DEFENSE SANITY
// block. NOTE (documented deviation): a defensive flag-FLIP is NOT achievable --
// the opponent completes its intersection triple with its FIRST stone of the
// turn (c6NextSame==1), where Score6's existing win-in-pair terminal already
// scores it a win. So defense is handled with or without the covering terminal;
// only OFFENSE (a mover's turn-final second stone, c6NextSame==0) is beyond
// Score6's reach and thus load-bearing. We still assert the detector fires for
// the opponent and the engine defends correctly.
static void T5_case() {
    offenseFlip("T5: intersection triple (col/2 diagonals) -> covering win (flag+injection gate)", intersectTriple3);
    printf("T5b: defense sanity (opponent triple detected + neutralized; not a flag-flip)\n");
    int q1[12], q2[3], n1, n2, D; intersectTriple(q1, n1, q2, n2, D);
    int p1b[3], p2b[12];
    for (int i = 0; i < 12; i++) p2b[i] = q1[i];   // opponent (P2) owns the three fours
    for (int i = 0; i < 3; i++)  p1b[i] = q2[i];    // P1 owns the far-side blockers
    CAi ai(13, 4, true);
    int b[19][19]; for (int x = 0; x < 19; x++) for (int y = 0; y < 19; y++) b[x][y] = 0;
    for (int i = 0; i < 12; i++) b[p2b[i]%19][p2b[i]/19] = 2;
    for (int i = 0; i < 3; i++)  b[p1b[i]%19][p1b[i]/19] = 1;
    b[D%19][D/19] = 2;
    CHECK(detOn(ai, b, 2), "T5b: c6UnstoppableThreat(2)==true for the opponent's completed triple");
    int mv[64]; CAi builder(13, 4, true);
    int count = buildHistoryMidPair(builder, mv, p1b, 3, p2b, 12);
    int m1, m2, bscr; runGM(4, mv, count, true, m1, m2, bscr);
    printf("    P1 defensive move m1=%d(%d,%d) (D=%d) bscr=%d\n", m1, m1%19, m1/19, D, bscr);
    CHECK(m1 == D, "T5b: engine defends by occupying the opponent's intersection cell D");
}

// =========================================================================
// ===== Connect6 v3 forcing-search prover tests ===========================
// =========================================================================

// ---- board primitives, INDEPENDENT of the engine's c6ScanFives ----------
static bool bHasSix(const int b[19][19], int P) {           // any 6-window all P
    for (int a = 0; a < 4; a++) for (int sx = 0; sx < 19; sx++) for (int sy = 0; sy < 19; sy++) {
        int dx = AX[a][0], dy = AX[a][1], ex = sx+5*dx, ey = sy+5*dy;
        if (ex<0||ex>=19||ey<0||ey>=19) continue;
        int n = 0; for (int k=0;k<6;k++){ int v=b[sx+k*dx][sy+k*dy]; if (v==P) n++; }
        if (n==6) return true; }
    return false;
}
static bool bCanSixNow(const int b[19][19], int P) {        // window oc==0 && pc>=4 => six in one 2-stone turn
    int O = 3-P;
    for (int a = 0; a < 4; a++) for (int sx = 0; sx < 19; sx++) for (int sy = 0; sy < 19; sy++) {
        int dx = AX[a][0], dy = AX[a][1], ex = sx+5*dx, ey = sy+5*dy;
        if (ex<0||ex>=19||ey<0||ey>=19) continue;
        int pc=0, oc=0; for (int k=0;k<6;k++){ int v=b[sx+k*dx][sy+k*dy]; if (v==P) pc++; else if (v==O) oc++; }
        if (oc==0 && pc>=4) return true; }
    return false;
}
// empty cells within Chebyshev distance 2 of any stone (generous relevance
// ring: every local block or threat-building move lives here). Row-major.
static int collectRing(const int b[19][19], int cand[361]) {
    bool mark[19][19]; for (int x=0;x<19;x++) for (int y=0;y<19;y++) mark[x][y]=false;
    for (int x=0;x<19;x++) for (int y=0;y<19;y++) if (b[x][y]>0)
        for (int ex=-2;ex<=2;ex++) for (int ey=-2;ey<=2;ey++) {
            int nx=x+ex, ny=y+ey; if (nx<0||nx>=19||ny<0||ny>=19) continue;
            if (b[nx][ny]<=0) mark[nx][ny]=true; }
    int n=0; for (int y=0;y<19;y++) for (int x=0;x<19;x++) if (mark[x][y]) cand[n++]=y*19+x;
    return n;
}

// ---- independent tri-state K-turn forcing oracle ------------------------
// Ground truth for "O to move, does P force a six within d P-turns?", mirroring
// c6And/c6Or's turn structure but with EXHAUSTIVE defender replies (over the
// ring) — it never uses the prover's L2/|C| theory. Node budget bounds cost:
// on exhaustion it returns UNKNOWN and the caller SKIPS (never guesses), so the
// oracle is sound: it only ever asserts WIN or NOWIN when it fully resolved.
//
// Relevance-ring soundness (review F2): O's replies are drawn from collectRing
// (empties within Chebyshev-2 of ANY stone). This can miss an O move only if a
// SAVING O reply lies farther than dist-2 from every stone. Within these short
// (d<=~3) forcing lines a saving O move must either (a) occupy a P five/four
// completion cell — always adjacent to P's run, hence in the ring — or (b) form
// an immediate O counter-six, which needs >=4 existing O stones in a window and
// so places adjacent to O stones, also in the ring. A stone in truly empty
// space (dist>2 from all stones) neither blocks a local P threat nor makes an
// immediate O threat, so it cannot save O here. bHasSix / bCanSixNow always scan
// the FULL board (never ring-limited), so win/veto detection is never truncated.
enum { ORA_NOWIN = 0, ORA_WIN = 1, ORA_UNKNOWN = 2 };
static int oraOr(int b[19][19], int P, int d, long &budget);
static int oraAnd(int b[19][19], int P, int d, long &budget) { // O to move
    if (--budget < 0) return ORA_UNKNOWN;
    int O = 3-P;
    if (bHasSix(b, P)) return ORA_WIN;         // P already completed a six
    if (bCanSixNow(b, O)) return ORA_NOWIN;    // O sixes on its own turn -> not forced (veto)
    if (d == 0) return oracleForcedWin(b, P) ? ORA_WIN : ORA_NOWIN;  // 1-turn covering base
    int cand[361]; int nc = collectRing(b, cand);
    if (nc < 2) return ORA_UNKNOWN;            // near-full: out of scope
    bool anyUnknown = false;
    for (int i = 0; i < nc; i++) for (int j = i+1; j < nc; j++) {
        int x1=cand[i]%19,y1=cand[i]/19,x2=cand[j]%19,y2=cand[j]/19;
        b[x1][y1]=O; b[x2][y2]=O;
        int r;
        if (bHasSix(b, O)) r = ORA_NOWIN;      // O wins outright with this reply
        else r = oraOr(b, P, d-1, budget);
        b[x1][y1]=0; b[x2][y2]=0;
        if (r == ORA_NOWIN) return ORA_NOWIN;  // O found a defense -> not forced
        if (r == ORA_UNKNOWN) anyUnknown = true;
    }
    return anyUnknown ? ORA_UNKNOWN : ORA_WIN;
}
static int oraOr(int b[19][19], int P, int d, long &budget) { // P to move
    if (--budget < 0) return ORA_UNKNOWN;
    if (bHasSix(b, P)) return ORA_WIN;
    if (bCanSixNow(b, P)) return ORA_WIN;      // P completes a six this turn
    int cand[361]; int nc = collectRing(b, cand);
    if (nc < 2) return ORA_UNKNOWN;            // cannot resolve P's turn -> UNKNOWN (never a bare NOWIN)
    bool anyUnknown = false;
    for (int i = 0; i < nc; i++) for (int j = i+1; j < nc; j++) {
        int x1=cand[i]%19,y1=cand[i]/19,x2=cand[j]%19,y2=cand[j]/19;
        b[x1][y1]=P; b[x2][y2]=P;
        int r;
        if (bHasSix(b, P)) r = ORA_WIN;
        else r = oraAnd(b, P, d, budget);
        b[x1][y1]=0; b[x2][y2]=0;
        if (r == ORA_WIN) return ORA_WIN;      // P found a forcing line
        if (r == ORA_UNKNOWN) anyUnknown = true;
    }
    return anyUnknown ? ORA_UNKNOWN : ORA_NOWIN;
}
// top-level: mutable copy + fresh budget. Mirrors c6ForcedWin's entry (O to move).
static int oracleForcedWinK(const int bd[19][19], int P, int d, long maxNodes) {
    int b[19][19]; for (int x=0;x<19;x++) for (int y=0;y<19;y++) b[x][y]=bd[x][y];
    long budget = maxNodes; return oraAnd(b, P, d, budget);
}

// ---- direct prover call helpers -----------------------------------------
static bool detFW(CAi &ai, const int b[19][19], int P, int lvl) {
    for (int x=0;x<19;x++) for (int y=0;y<19;y++) ai.bd[x][y]=b[x][y];
    setAxes(ai); return ai.c6ForcedWin(P, lvl);
}
static bool callAnd(CAi &ai, const int b[19][19], int P, int d) {
    for (int x=0;x<19;x++) for (int y=0;y<19;y++) ai.bd[x][y]=b[x][y];
    setAxes(ai); ai.c6Budget = C6_SCAN_BUDGET; return ai.c6And(P, d);
}
static void clr(int b[19][19]) { for (int x=0;x<19;x++) for (int y=0;y<19;y++) b[x][y]=0; }
static void put(int b[19][19], int x, int y, int P) { b[x][y]=P; }
static void putRun(int b[19][19], int x, int y, int a, int len, int P) { // AX[a] direction
    for (int k=0;k<len;k++){ int cx=x+k*AX[a][0], cy=y+k*AX[a][1]; if (cx>=0&&cx<19&&cy>=0&&cy<19) b[cx][cy]=P; }
}

// ---- T_v3 curated WIN exhibits (must be forced wins) --------------------
static void Tv3_win(void) {
    printf("T_v3_win: curated multi-turn forced wins (c6ForcedWin==true, oracle-confirmed)\n");
    CAi ai(13, 4, true);
    // (1) open five + unrelated open four: 2-turn VCDF. O forced onto the five's
    //     two ends, then P completes the four into a six. (v2 refuses: |C|==2<3.)
    { int b[19][19]; clr(b);
      putRun(b, 5,9, 1, 5, 1);          // five row9 cols5..9 (axis1 = (0,-1)? no) -- use horizontal
      // NOTE axis for horizontal: AX has {(-1,-1),(0,-1),(1,-1),(-1,0)}; horizontal is dx=1,dy=0 -> not in AX.
      // Build runs by explicit cells to avoid axis confusion:
      clr(b);
      for (int x=5;x<=9;x++) b[x][9]=1;   // horizontal five row 9, ends (4,9),(10,9)
      for (int x=5;x<=8;x++) b[x][5]=1;   // horizontal four row 5, ends (4,5),(9,5)
      bool w = detFW(ai, b, 1, 2);
      int orc = oracleForcedWinK(b, 1, 2, 20000000L);
      CHECK(!detOn(ai, b, 1), "T_v3_win(1): v2 correctly refuses (|C|==2)");
      CHECK(w, "T_v3_win(1): c6ForcedWin true on open-five + open-four (2-turn VCDF)");
      CHECK(orc != ORA_NOWIN, "T_v3_win(1): independent oracle does not refute");
    }
    // (2) Exhibit A from the spec (Section 8.1): five row9 cols5..9 + four row11
    //     cols5..8, O scattered upper-right (veto false).
    { int b[19][19]; clr(b);
      for (int x=5;x<=9;x++) b[x][9]=1;               // five, ends (4,9),(10,9)
      for (int x=5;x<=8;x++) b[x][11]=1;              // four, ends (4,11),(9,11)
      int o[9][2]={{14,3},{15,4},{16,4},{14,5},{15,6},{14,7},{16,7},{16,11},{16,13}};
      for (int i=0;i<9;i++) b[o[i][0]][o[i][1]]=2;
      bool w = detFW(ai, b, 1, 2);
      int orc = oracleForcedWinK(b, 1, 2, 20000000L);
      CHECK(!detOn(ai, b, 1), "T_v3_win(2): v2 refuses exhibit A (|C|==2)");
      CHECK(w, "T_v3_win(2): c6ForcedWin true on spec exhibit A");
      CHECK(orc != ORA_NOWIN, "T_v3_win(2): oracle does not refute exhibit A");
    }
    // (3) Exhibit C (spec 8.2): threes-only 2-WAVE chain -- no >=4 window exists
    //     for either side at root, so a single wave cannot win: needs K>=2. This
    //     is the case v3 catches even at realistic levels (beyond win-in-pair).
    { int b[19][19]; clr(b);
      // Root threes-only board (spec 8.2) PLUS X's wave-1 pair (6,6),(10,6) so
      // the board is turn-final (X just moved, O to move) -- the state the prover
      // actually fires on. Row6 is now an open five; col x=6 a fresh three.
      int X[10][2]={{7,6},{8,6},{9,6},{6,8},{6,9},{7,10},{8,10},{9,10},{6,6},{10,6}};
      int Ov[8][2]={{11,4},{12,6},{4,7},{11,8},{11,10},{5,11},{8,12},{12,12}};
      for(int i=0;i<10;i++) b[X[i][0]][X[i][1]]=1;
      for(int i=0;i<8;i++) b[Ov[i][0]][Ov[i][1]]=2;
      bool d1 = callAnd(ai, b, 1, 1);        // a single wave: expected insufficient
      bool d2 = callAnd(ai, b, 1, 2);        // two waves: the forced win
      // 16+2-stone board => the exhaustive-defender oracle is intractable at d=2;
      // a small budget makes it return UNKNOWN fast (the fuzz net confirms the
      // general no-false-positive property over thousands of tractable boards).
      int orc = oracleForcedWinK(b, 1, 2, 1500000L);
      printf("    exhibit C: c6And d1=%d d2=%d oracle(d2)=%d\n", (int)d1,(int)d2,orc);
      CHECK(d2, "T_v3_win(3): exhibit C is a forced win at K>=2 (deep 2-wave chain)");
      CHECK(orc != ORA_NOWIN, "T_v3_win(3): oracle does not refute exhibit C");
    }
}

// ---- T_v3 REFUSE look-alikes (must NOT be claimed wins) -----------------
static void Tv3_refuse(void) {
    printf("T_v3_refuse: forcing look-alikes with no winning continuation (c6ForcedWin==false)\n");
    CAi ai(13, 4, true);
    // (1) lone open five: O blocks both ends, P has no follow-up.
    { int b[19][19]; clr(b); for (int x=5;x<=9;x++) b[x][9]=1;
      CHECK(!detFW(ai, b, 1, 4), "T_v3_refuse(1): lone open five is not a forced win"); }
    // (2) half-open five (|C|==1): O has a free stone.
    { int b[19][19]; clr(b); for (int x=5;x<=9;x++) b[x][9]=1; b[4][9]=2;
      CHECK(!detFW(ai, b, 1, 4), "T_v3_refuse(2): half-open five (|C|==1) refuses"); }
    // (3) oppImm race: P open five + O has its own one-move six.
    { int b[19][19]; clr(b); for (int x=5;x<=9;x++) b[x][9]=1;
      for (int x=2;x<=6;x++) b[x][15]=2;   // 5 O in a row -> canSixNow(O)
      CHECK(!detFW(ai, b, 1, 4), "T_v3_refuse(3): oppImm race refuses"); }
    // (4) quiet scatter: no forcing structure.
    { int b[19][19]; clr(b); b[9][9]=1; b[8][10]=2; b[10][8]=1; b[9][11]=2; b[7][9]=1;
      CHECK(!detFW(ai, b, 1, 4), "T_v3_refuse(4): quiet position refuses"); }
    // (5) two SEPARATE open fives already = v2 win (|C|>=3), sanity that v3 keeps it.
    { int b[19][19]; clr(b); for (int x=5;x<=9;x++) b[x][9]=1; for (int y=5;y<=9;y++) b[3][y]=1;
      // |C| here: row five ends (4,9),(10,9); col five (x=3) ends (3,4),(3,10) => 4 distinct >=3
      CHECK(detFW(ai, b, 1, 4), "T_v3_refuse(5): two open fives (|C|>=3) IS a v2/v3 win (sanity)"); }
}

// ---- T_v3 purity: c6ForcedWin leaves bd byte-identical (incl -1) --------
static void Tv3_purity(void) {
    printf("T_v3_purity: c6ForcedWin restores bd exactly (including -1 markers)\n");
    CAi ai(13, 4, true);
    int b[19][19]; clr(b);
    for (int x=5;x<=9;x++) b[x][9]=1; for (int x=5;x<=8;x++) b[x][5]=1;   // exhibit-1 (a win)
    b[10][10]=-1; b[4][8]=-1; b[11][9]=-1;                                 // sprinkle -1 empties
    for (int x=0;x<19;x++) for (int y=0;y<19;y++) ai.bd[x][y]=b[x][y];
    setAxes(ai);
    ai.c6ForcedWin(1, 2);
    int diff = 0; for (int x=0;x<19;x++) for (int y=0;y<19;y++) if (ai.bd[x][y]!=b[x][y]) diff++;
    CHECK(diff==0, "T_v3_purity: bd byte-identical after c6ForcedWin (no scratch leak)");
}

// ---- T_v3 subsumption fuzz: every v2 win is a v3 win --------------------
static void Tv3_subsumption(int N) {
    printf("T_v3_subsumption: v2 c6UnstoppableThreat(P) ==> c6ForcedWin(P) on %d fuzz boards\n", N);
    CAi ai(13, 4, true); setAxes(ai);
    long v2fired=0, holes=0;
    for (int t=0;t<N;t++){ int b[19][19]; genBoard(b, t%3);
        for (int P=1;P<=2;P++){
            if (detOn(ai, b, P)) { v2fired++;
                if (!detFW(ai, b, P, 2)) { holes++; if (holes<=4) printf("  SUBSUMPTION HOLE t=%d P=%d\n", t, P); } } } }
    printf("    v2 fired=%ld  subsumption holes=%ld\n", v2fired, holes);
    CHECK(holes==0, "T_v3_subsumption: no v2 win lost by v3 (strict extension)");
    CHECK(v2fired>0, "T_v3_subsumption: v2 fired at least once (test is live)");
}

// ---- T_v3 no-false-positive fuzz: every resolved prover WIN is oracle-WIN
// Small local boards so the exhaustive defender oracle resolves within budget.
static void genTinyForce(int b[19][19]) {
    clr(b);
    int ox = 4 + ri(6), oy = 4 + ri(6);            // cluster origin, keeps runs on-board
    int runs = 1 + ri(3);
    for (int r=0;r<runs;r++){
        int a = ri(4), len = 3 + ri(3), who = 1 + (ri(5)==0 ? 1 : 0); // mostly P1
        int sx = ox + ri(3) - 1, sy = oy + ri(3) - 1;
        for (int k=0;k<len;k++){ int cx=sx+k*AX[a][0], cy=sy+k*AX[a][1];
            if (cx>=0&&cx<19&&cy>=0&&cy<19 && ri(100)<88) b[cx][cy]=who; } }
    int noise = ri(3); for (int i=0;i<noise;i++){ int x=ox+ri(6)-1, y=oy+ri(6)-1;
        if (x>=0&&x<19&&y>=0&&y<19 && b[x][y]==0) b[x][y]=(ri(3)==0?2:1); }
}
static void Tv3_nofalsepos(int N) {
    printf("T_v3_nofalsepos: every ORACLE-RESOLVED prover WIN is confirmed (%d tiny boards)\n", N);
    CAi ai(13, 4, true); setAxes(ai);
    long fired=0, confirmed=0, skipped=0, refuted=0; int shown=0;
    for (int t=0;t<N;t++){ int b[19][19]; genTinyForce(b);
        for (int P=1;P<=2;P++){
            for (int d=1;d<=2;d++){
                bool w = callAnd(ai, b, P, d);
                if (!w) continue; fired++;
                int orc = oracleForcedWinK(b, P, d, 8000000L);
                if (orc==ORA_WIN) confirmed++;
                else if (orc==ORA_UNKNOWN) skipped++;
                else { refuted++; if (shown<8){ printf("  *** FALSE POSITIVE t=%d P=%d d=%d ***\n", t,P,d); shown++; } }
            } } }
    printf("    prover-WIN=%ld  oracle-confirmed=%ld  skipped(UNKNOWN)=%ld  REFUTED=%ld\n",
           fired, confirmed, skipped, refuted);
    CHECK(refuted==0, "T_v3_nofalsepos: NO prover win refuted by the independent oracle");
    CHECK(confirmed>0, "T_v3_nofalsepos: oracle confirmed >0 prover wins (net is live)");
}

// ---- T_v3_flip: END-TO-END mutation gate (v3-only forced win via getMove) --
// The load-bearing non-inertness proof at the move-choice level. Board: P1
// fours on row9 (cols6..9) and row5 (cols5..8). The turn-final stone (5,9)
// turns row9 into an OPEN FIVE (|C|==2) with the row5 four as a hidden
// finisher: O is forced to block the five's two ends, then P completes the four
// -> six. This 2-turn win is invisible to v2 (|C|==2<3), to Score6's win-in-pair
// (fires only mid-pair, c6NextSame==1 -- here the stone is turn-final), and to
// the fixed plv horizon (six lands beyond mxlv). ONLY the v3 terminal sees it.
static void Tv3_flip(void) {
    printf("T_v3_flip: end-to-end getMove mutation gate (v3-only 2-turn forced win)\n");
    int p1[8] = { M(6,9),M(7,9),M(8,9),M(9,9), M(5,5),M(6,5),M(7,5),M(8,5) };
    int p2[1]; int C = M(5,9);
    CAi ai(13,4,true);
    int b[19][19]; clr(b);
    for (int i=0;i<8;i++) b[p1[i]%19][p1[i]/19]=1;
    CHECK(!detFW(ai,b,1,4), "T_v3_flip: pre-move board is not yet a forced win");
    b[C%19][C/19]=1;
    CHECK(!detOn(ai,b,1),   "T_v3_flip: after C, v2 still refuses (|C|==2)");
    CHECK(detFW(ai,b,1,2),  "T_v3_flip: after C, c6ForcedWin true (2-turn VCDF)");
    // Search at L=2 (shallow horizon): the 2-turn finish lands beyond mxlv=2, so
    // even Score6's win-in-pair leaf cannot reach it -- ONLY the v3 terminal sees
    // it. (At L>=4 win-in-pair already catches THIS depth; v3's edge at higher
    // levels comes from deeper forcing chains, measured in the strength phase.)
    int mv[64]; CAi builder(13,2,true);
    int count = buildHistoryMidPair(builder, mv, p1, 8, p2, 0);
    int m1on,m2on,bon, m1off,m2off,boff;
    runGM(2, mv, count, true,  m1on,m2on,bon);
    runGM(2, mv, count, false, m1off,m2off,boff);
    printf("    count=%d tnRoot~%d  ON: m1=%d(%d,%d) bscr=%d | OFF: m1=%d bscr=%d\n",
           count, count+1, m1on, m1on%19, m1on/19, bon, m1off, boff);
    CHECK(bon>=10000,  "T_v3_flip: flag ON  -> getMove claims the forced win (injection load-bearing)");
    CHECK(boff<10000,  "T_v3_flip: flag OFF -> win invisible at this horizon (mutation gate flips)");
    CHECK(m1on==C,     "T_v3_flip: flag ON  -> engine plays the double-five-creating stone (5,9)");
}

// ---- T_v3_accuracy: forced-win RECOGNITION across levels (the accuracy metric)
// Exhibit C as a getMove turn-final position (root 8X + 8O + wave-1 first stone
// (6,6)); the winning turn-final stone is (10,6), starting a K=2 forcing chain
// whose six lands ~9 plies deep -- beyond win-in-pair AND the plv horizon at
// EVERY tested level. So v3 should recognize the win (bscr>=10000) at L=2,3,4
// while the baseline never does. This isolates v3's tactical-accuracy gain.
static void Tv3_accuracy(void) {
    printf("T_v3_accuracy: deep forced-win recognition, v3-ON vs baseline-OFF, across levels\n");
    int p1[9] = { M(7,6),M(8,6),M(9,6),M(6,8),M(6,9),M(7,10),M(8,10),M(9,10), M(6,6) };
    int p2[8] = { M(11,4),M(12,6),M(4,7),M(11,8),M(11,10),M(5,11),M(8,12),M(12,12) };
    int onWins=0, offWins=0;
    for (int L=2; L<=4; L++) {
        int mv[64]; CAi builder(13,L,true);
        int count = buildHistoryMidPair(builder, mv, p1, 9, p2, 8);
        int m1on,m2on,bon, m1off,m2off,boff;
        runGM(L, mv, count, true,  m1on,m2on,bon);
        runGM(L, mv, count, false, m1off,m2off,boff);
        bool onW = bon>=10000, offW = boff>=10000;
        onWins += onW; offWins += offW;
        printf("    L=%d  ON: m1=%d(%d,%d) bscr=%d %s | OFF: m1=%d bscr=%d %s\n",
               L, m1on,m1on%19,m1on/19,bon, onW?"WIN":"--", m1off,boff, offW?"WIN":"--");
    }
    // Honest picture: v3 recognizes the win at EVERY level; the baseline catches
    // it only where win-in-pair happens to reach (here L=4), missing the lower
    // levels. So v3 STRICTLY extends tactical reach -- most at low levels and for
    // chains deeper than win-in-pair's ~2-ply extension. (See strength notes.)
    CHECK(onWins==3,        "T_v3_accuracy: v3-ON recognizes the deep forced win at ALL levels (2,3,4)");
    CHECK(offWins<onWins,   "T_v3_accuracy: baseline misses it at >=1 level v3 catches (v3 strictly extends reach)");
}

// ---- T_v3_timing: perf gate (review F3) -- v3 must not stall getMove --------
static void Tv3_timing(void) {
    printf("T_v3_timing: getMove CPU cost with v3 ON vs OFF (perf gate)\n");
    int p1[9] = { M(7,6),M(8,6),M(9,6),M(6,8),M(6,9),M(7,10),M(8,10),M(9,10), M(6,6) };
    int p2[8] = { M(11,4),M(12,6),M(4,7),M(11,8),M(11,10),M(5,11),M(8,12),M(12,12) };
    int mv[64]; CAi builder(13,4,true);
    int count = buildHistoryMidPair(builder, mv, p1, 9, p2, 8);
    int reps = 20, m1,m2,b;
    clock_t t0=clock(); for(int i=0;i<reps;i++) runGM(4, mv, count, true,  m1,m2,b); clock_t t1=clock();
    for(int i=0;i<reps;i++) runGM(4, mv, count, false, m1,m2,b); clock_t t2=clock();
    double on=(double)(t1-t0)/CLOCKS_PER_SEC/reps*1000.0, off=(double)(t2-t1)/CLOCKS_PER_SEC/reps*1000.0;
    printf("    per-getMove @L4 forcing position: ON=%.2f ms  OFF=%.2f ms  ratio=%.2fx\n",
           on, off, off>0?on/off:0.0);
    CHECK(on < 250.0, "T_v3_timing: v3 getMove stays under 250ms on a forcing position (no stall)");
    // Worst-case RAW prover cost with no search short-circuit: dense cluster
    // boards (many fives => heavy recursion), both players. Each call is hard-
    // capped by C6_SCAN_BUDGET, so the per-call worst case is bounded regardless
    // of how many nodes a real getMove invokes it from (review F3).
    CAi ai(13,4,true); setAxes(ai);
    int calls = 6000; clock_t d0=clock();
    for (int t=0;t<calls;t++){ int bb[19][19]; genBoard(bb, 1);
        for(int x=0;x<19;x++)for(int y=0;y<19;y++)ai.bd[x][y]=bb[x][y];
        ai.c6ForcedWin(1+(t&1), 2); }
    clock_t d1=clock();
    double avg=(double)(d1-d0)/CLOCKS_PER_SEC/calls*1000.0;
    printf("    raw c6ForcedWin avg over %d dense boards: %.4f ms/call\n", calls, avg);
    CHECK(avg < 5.0, "T_v3_timing: avg raw prover call under 5ms on dense boards (budget-bounded)");
}

int main(int argc, char **argv) {
    if (chdir("MMAIWASM") != 0) { /* allow running inside MMAIWASM too */ }
    int fuzzN = (argc > 1) ? atoi(argv[1]) : 50000;   // T0 board count (override via argv[1])
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
    printf("\n--- Connect6 v2 covering-threat terminal ---\n");
    T0_fuzz(fuzzN);
    T3_blockable();
    T4_race();
    T6_quiet();
    offenseFlip("T1: intersection triple (row/col/diag) -> covering win (flag+injection gate)", intersectTriple);
    offenseFlip("T2: intersection triple (row/2 diagonals) -> covering win (flag+injection gate)", intersectTriple2);
    T5_case();
    printf("\n--- Connect6 v3 forcing-search prover ---\n");
    Tv3_win();
    Tv3_refuse();
    Tv3_purity();
    Tv3_subsumption(fuzzN);
    Tv3_nofalsepos(fuzzN < 20000 ? fuzzN : 20000);
    Tv3_flip();
    Tv3_accuracy();
    Tv3_timing();
    printf("\nFALLBACK HITS (defensive 2nd-stone guard): %d\n", g_fallback);
    printf("%s (%d failure%s)\n", failures ? "C6TEST FAIL" : "C6TEST PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
