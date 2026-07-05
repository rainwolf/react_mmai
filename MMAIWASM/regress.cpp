#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include "Ai.h"

// Deterministic clock: Ai.cpp seeds srand((unsigned)time(NULL)) inside Move().
// Defining time() inside the executable makes the linker bind Ai.o's call
// here instead of libc, so both old and new builds see the same seed.
extern "C" time_t time(time_t *t) {
    if (t) *t = 777;
    return 777;
}

int main(int argc, char **argv) {
    if (argc < 3) { printf("usage: regress <MMAIWASMdir> <gameId>\n"); return 2; }
    if (chdir(argv[1]) != 0) { printf("bad dir\n"); return 2; }
    int gameId = atoi(argv[2]);

    const int levels[] = {1, 2, 3, 4};
    const int lens[] = {5, 8, 11, 14, 19, 24};

    for (int li = 0; li < 4; li++) {
        for (int ni = 0; ni < 6; ni++) {
            for (int seed = 1; seed <= 3; seed++) {
                // deterministic distinct cells in [4,14]^2
                unsigned lcg = 1000003u * (unsigned)(seed * 131 + lens[ni] * 17 + levels[li]);
                int moves[32];
                bool used[361] = {false};
                int n = 0;
                while (n < lens[ni]) {
                    lcg = lcg * 1664525u + 1013904223u;
                    int x = 4 + (lcg >> 16) % 11;
                    lcg = lcg * 1664525u + 1013904223u;
                    int y = 4 + (lcg >> 16) % 11;
                    int m = y * 19 + x;
                    if (used[m]) continue;
                    used[m] = true;
                    moves[n++] = m;
                }
                CAi ai(gameId, levels[li], true);
                int mv = ai.getMove(moves, lens[ni]);
                printf("L%d n%d s%d -> %d\n", levels[li], lens[ni], seed, mv);
                fflush(stdout);
            }
        }
    }
    // fixed position from test.cpp
    {
        CAi ai(gameId, 5, true);
        int moves[7] = {180, 161, 104, 200, 144, 198, 201};
        int mv = ai.getMove(moves, 7);
        printf("fixed7 -> %d\n", mv);
    }
    return 0;
}
