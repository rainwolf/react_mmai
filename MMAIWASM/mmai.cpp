#include <emscripten/emscripten.h>
#include "Ai.h"


// Return-value encoding by variant (same int signature for all):
//   * Every variant EXCEPT Connect6 returns a single move index m in 0..360
//     (m = y*19 + x).
//   * Connect6 (game 13/14, cfg.stonesPerTurn==2) plays TWO stones per turn and
//     returns them packed as  m1*362 + m2  (base 362, NOT 361 -- base 361 is
//     ambiguous since m1*361+361 == (m1+1)*361+0) where:
//       - m1 in 0..360 is the first stone (always a real move);
//       - m2 in 0..360 is the second stone, OR the sentinel 361 when this turn
//         places only one stone (the opening move 0, or a mid-pair invocation).
//     Decode: m1 = packed / 362;  m2 = packed % 362  (m2==361 => single stone).
//     Max packed value: 360*362 + 361 = 130681 (fits comfortably in int).
extern "C"  {
	EMSCRIPTEN_KEEPALIVE int getAIMove(int game, int level, int openingBook, int *moves, int numMoves) {
			CAi ai(game, level, openingBook);
			// int movs[4] = {180, 181, 200, 220};
			int newMove = ai.getMove(moves, numMoves);
		// CAi ai(game, level, openingBook);
		// int newMove = ai.getMove(moves, numMoves);
		return newMove;
	}
}
