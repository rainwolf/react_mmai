#include <emscripten/emscripten.h>
#include "Ai.h"


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
