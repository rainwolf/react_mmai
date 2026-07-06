// porttest.cpp -- native regression for the mobile portability seam (plan
// phases 1-4). Proves three things the WASM golden harness cannot:
//
//   1. Persistent-instance re-entrancy (plan section 4): the Android/iOS
//      wrappers hold ONE CAi across many moves; WASM builds a fresh CAi per
//      call. getMove() on a reused instance replaying a GROWN move list must
//      return the SAME value as a fresh instance. Checked for games
//      1,3,11,15,25,13 with two prefix lengths.
//
//   2. Hook callbacks (section 3): with a listener + mask, aiEvaluated() and
//      aiVisualization() fire (count > 0); with mask 0 the visualization is
//      silent but the move is identical; with no listener the move is also
//      identical (callbacks never perturb the search).
//
//   3. Stop flag (section 2): requestStop() fired from the first aiEvaluated()
//      terminates the search early (far fewer evals than a full search) and
//      still returns a legal move.
//
// The resource-directory seam (section 1) is exercised implicitly: every CAi
// below is constructed with an explicit filesDir argument (argv[1] + "/files")
// and we never chdir, so a passing run also proves the ctor loads its data
// files from an arbitrary absolute path.
//
// Build (native, from a scratch dir):
//   g++ -O2 -std=c++11 -o porttest porttest.cpp Ai.cpp CPoint.cpp
//   ./porttest <MMAIWASMdir>

#include <cstdio>
#include <cstring>
#include <string>
#include "Ai.h"

// Deterministic clock, exactly as regress.cpp: Ai.cpp seeds
// srand((unsigned)time(NULL)) inside Move(); binding time() here makes every
// search reproducible so fresh-vs-persistent comparisons are meaningful.
extern "C" time_t time(time_t *t) {
    if (t) *t = 777;
    return 777;
}

static std::string FILESDIR;

// Distinct midboard cells in [4,14]^2. Seeded by `seed` only (NOT length), so
// genMoves(seed,8) is a strict prefix of genMoves(seed,12): the growing move
// lists the persistent test needs. Midboard + off-centre => never matches the
// centre-based opening book, so dmov()/cmove() draw no rand() before Move()'s
// srand(777) and every result is srand-deterministic regardless of the
// process-global rand() state on entry.
static void genMoves(unsigned seed, int len, int *out) {
    unsigned lcg = 1000003u * seed;
    bool used[361] = {false};
    int n = 0;
    while (n < len) {
        lcg = lcg * 1664525u + 1013904223u;
        int x = 4 + (lcg >> 16) % 11;
        lcg = lcg * 1664525u + 1013904223u;
        int y = 4 + (lcg >> 16) % 11;
        int m = y * 19 + x;
        if (used[m]) continue;
        used[m] = true;
        out[n++] = m;
    }
}

static bool inList(const int *moves, int n, int m) {
    for (int i = 0; i < n; i++) if (moves[i] == m) return true;
    return false;
}

// Counts callbacks; optionally requests stop on the first aiEvaluated().
struct CountListener : CAiListener {
    long evalCount = 0;
    long visCount = 0;
    int lastVisLen = -1;
    int lastVisMarks = -1;   // number of cells == 3 in the last visualization
    CAi *ai = nullptr;       // set to request stop from inside the callback
    bool stopOnFirst = false;

    void aiEvaluated() override {
        evalCount++;
        if (stopOnFirst && ai && evalCount == 1) ai->requestStop();
    }
    void aiVisualization(const int *data, int len) override {
        visCount++;
        lastVisLen = len;
        int marks = 0;
        for (int i = 0; i < len; i++) if (data[i] == 3) marks++;
        lastVisMarks = marks;
    }
};

static int failures = 0;
static void check(bool cond, const char *msg) {
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", msg);
    if (!cond) failures++;
}

// ---------------------------------------------------------------------------
static void testPersistent() {
    printf("== Test 1: persistent-instance re-entrancy (fresh == reused) ==\n");
    const int games[] = {1, 3, 11, 15, 25, 13};
    const char *names[] = {"pente(1)", "keryo(3)", "poof(11)", "boat(15)",
                           "opente(25)", "connect6(13)"};
    const int level = 3;
    const int n1 = 8, n2 = 12;

    for (int gi = 0; gi < 6; gi++) {
        int game = games[gi];
        int moves[24];
        genMoves(101u + game, n2, moves);   // moves[0..n1) is a prefix of moves[0..n2)

        int fresh1, fresh2, pers1, pers2;
        { CAi a(game, level, true, FILESDIR.c_str()); fresh1 = a.getMove(moves, n1); }
        { CAi a(game, level, true, FILESDIR.c_str()); fresh2 = a.getMove(moves, n2); }
        {   // one instance, two grown calls in a row
            CAi a(game, level, true, FILESDIR.c_str());
            pers1 = a.getMove(moves, n1);
            pers2 = a.getMove(moves, n2);
        }
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "%-13s n=%d  fresh=%d reused=%d", names[gi], n1, fresh1, pers1);
        check(fresh1 == pers1, buf);
        snprintf(buf, sizeof(buf),
                 "%-13s n=%d fresh=%d reused=%d", names[gi], n2, fresh2, pers2);
        check(fresh2 == pers2, buf);
    }
}

// ---------------------------------------------------------------------------
// Test 1b: persistent-instance re-entrancy in OPENING-BOOK territory. Test 1's
// genMoves() deliberately avoids the opening book so it cannot catch a
// reset() regression rooted in the obfl latch (cleared inside cmove()'s
// random early-exit; only restored by reset()). These sequences are real
// rows from the loaded opngbk.pen table, with n1/n2 chosen strictly below
// that row's own recorded length so obfl is still live at both prefixes --
// exercising the exact code path reset() exists to guard.
static void testPersistentBookTerritory() {
    printf("== Test 1b: persistent-instance re-entrancy (opening-book territory) ==\n");
    // Pin the process-global rand() stream so this test's outcome does not
    // depend on how much entropy testPersistent() consumed before it (Ai.cpp's
    // arc4random_uniform() is a thin rand() wrapper; only Move() reseeds via
    // srand(time(NULL)), and these book-active decisions are resolved before
    // Move() is ever reached). srand(1) matches libc's default unseeded state,
    // i.e. the same state these sequences were selected under.
    struct Case { int game; const char *name; int n1, n2; int moves[9]; };
    Case cases[] = {
        { 1, "pente(1)", 8, 9, {180,182,183,178,143,218,200,198,238} },
        { 3, "keryo(3)", 8, 9, {180,181,256,219,200,220,218,160,221} },
    };
    const int level = 3;
    for (int ci = 0; ci < 2; ci++) {
        Case &c = cases[ci];
        // Reseed before each case: this sequence was discovered against an
        // isolated process's rand() stream (unseeded == srand(1) per the C
        // standard). Reseeding per-case keeps every case's stream position
        // identical to how it was discovered, independent of how many draws
        // earlier cases (or testPersistent(), which runs before this
        // function) consumed.
        srand(1);
        int fresh1, fresh2, pers1, pers2;
        { CAi a(c.game, level, true, FILESDIR.c_str()); fresh1 = a.getMove(c.moves, c.n1); }
        { CAi a(c.game, level, true, FILESDIR.c_str()); fresh2 = a.getMove(c.moves, c.n2); }
        {   // one instance, two grown calls in a row
            CAi a(c.game, level, true, FILESDIR.c_str());
            pers1 = a.getMove(c.moves, c.n1);
            pers2 = a.getMove(c.moves, c.n2);
        }
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "%-13s n=%d  fresh=%d reused=%d", c.name, c.n1, fresh1, pers1);
        check(fresh1 == pers1, buf);
        snprintf(buf, sizeof(buf),
                 "%-13s n=%d fresh=%d reused=%d", c.name, c.n2, fresh2, pers2);
        check(fresh2 == pers2, buf);
    }
}

// ---------------------------------------------------------------------------
static void testHooks() {
    printf("== Test 2: hook callbacks (aiEvaluated / aiVisualization / mask) ==\n");
    const int game = 1, level = 3, n = 8;
    int moves[8];
    genMoves(555u, n, moves);

    // (a) no listener -> baseline move
    int mvNone;
    { CAi a(game, level, true, FILESDIR.c_str()); mvNone = a.getMove(moves, n); }

    // (b) listener + mask on
    CountListener on;
    int mvOn;
    { CAi a(game, level, true, FILESDIR.c_str());
      a.setListener(&on); a.setCallbackMask(1);
      mvOn = a.getMove(moves, n); }

    // (c) listener + mask off
    CountListener off;
    int mvOff;
    { CAi a(game, level, true, FILESDIR.c_str());
      a.setListener(&off); a.setCallbackMask(0);
      mvOff = a.getMove(moves, n); }

    char buf[160];
    snprintf(buf, sizeof(buf), "mask on: aiEvaluated fired %ld times (>0)", on.evalCount);
    check(on.evalCount > 0, buf);
    snprintf(buf, sizeof(buf), "mask on: aiVisualization fired %ld times (>0)", on.visCount);
    check(on.visCount > 0, buf);
    snprintf(buf, sizeof(buf), "mask on: vis payload len==361 and exactly one '3' mark (len=%d marks=%d)",
             on.lastVisLen, on.lastVisMarks);
    check(on.lastVisLen == 361 && on.lastVisMarks == 1, buf);
    snprintf(buf, sizeof(buf), "mask off: aiEvaluated still fired %ld times (unconditional)", off.evalCount);
    check(off.evalCount > 0, buf);
    snprintf(buf, sizeof(buf), "mask off: aiVisualization silent (%ld calls)", off.visCount);
    check(off.visCount == 0, buf);
    snprintf(buf, sizeof(buf), "move identical none/on/off: %d / %d / %d", mvNone, mvOn, mvOff);
    check(mvNone == mvOn && mvOn == mvOff, buf);
}

// ---------------------------------------------------------------------------
static void testStop() {
    printf("== Test 3: requestStop() terminates search early, returns legal move ==\n");
    const int game = 1, level = 6, n = 8;   // deep level => full search does many evals
    int moves[8];
    genMoves(999u, n, moves);

    // Full search: count evals, capture move.
    CountListener full;
    int mvFull;
    { CAi a(game, level, true, FILESDIR.c_str());
      a.setListener(&full);            // mask 0: aiEvaluated counts, no vis overhead
      mvFull = a.getMove(moves, n); }

    // Stopped search: requestStop() from the first aiEvaluated().
    CountListener stop;
    stop.stopOnFirst = true;
    int mvStop;
    { CAi a(game, level, true, FILESDIR.c_str());
      stop.ai = &a; a.setListener(&stop);
      mvStop = a.getMove(moves, n); }

    char buf[160];
    snprintf(buf, sizeof(buf), "full search evals=%ld, stopped evals=%ld (stopped < full)",
             full.evalCount, stop.evalCount);
    check(stop.evalCount < full.evalCount, buf);
    bool legal = (mvStop >= 0 && mvStop <= 360 && !inList(moves, n, mvStop));
    snprintf(buf, sizeof(buf), "stopped move %d is a legal empty cell", mvStop);
    check(legal, buf);
}

// ---------------------------------------------------------------------------
// Test 4: constructor load-failure visibility (loadErr / ok()). A bogus
// filesDir means all three ctor fopen()s fail; ok() must report false. The
// real filesDir must load cleanly and report ok()==true.
static void testLoadErr() {
    printf("== Test 4: ok() reflects ctor file-load success/failure ==\n");
    {
        CAi a(1, 3, true, "/nonexistent/bogus/filesDir/xyz");
        check(a.ok() == false, "bogus filesDir -> ok()==false");
    }
    {
        CAi a(1, 3, true, FILESDIR.c_str());
        check(a.ok() == true, "real filesDir -> ok()==true");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("usage: porttest <MMAIWASMdir>\n"); return 2; }
    FILESDIR = std::string(argv[1]) + "/files";   // exercises the ctor filesDir seam
    printf("filesDir = %s\n", FILESDIR.c_str());

    testPersistent();
    testPersistentBookTerritory();
    testHooks();
    testStop();
    testLoadErr();

    printf(failures == 0 ? "\nPORTTEST PASS\n" : "\nPORTTEST FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}
