// GameClass.js referee fixtures for the Poof/Boat/O-Pente variant plumbing.
// Run: npm run test:variants   (plain node, no test framework in this repo).
//
// GameClass.js is an ES module (`export class Game`) living in a package with no
// "type":"module", so Node would load it as CommonJS and choke on `export`. We copy
// it to a temp *.mjs (always ESM) and dynamic-import that.
import { readFileSync, writeFileSync, unlinkSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { pathToFileURL, fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const srcPath = join(here, '..', 'src', 'Classes', 'GameClass.js');
const tmpPath = join(tmpdir(), `gameclass.${process.pid}.mjs`);
writeFileSync(tmpPath, readFileSync(srcPath, 'utf8'));
const { Game } = await import(pathToFileURL(tmpPath).href);
unlinkSync(tmpPath);

let pass = 0, fail = 0;
function check(name, cond, extra = '') {
    if (cond) { pass++; console.log(`ok   - ${name}`); }
    else { fail++; console.log(`FAIL - ${name}${extra ? '  :: ' + extra : ''}`); }
}
const XY = (x, y) => y * 19 + x;
function freshGame(id) { const g = new Game(); g.setGame(id); g.reset(); g.rated = false; return g; }
function seed(g, cells) { for (const [x, y, v] of cells) g.abstractBoard[x][y] = v; }
function playSeq(g, moves) { for (const m of moves) g.addMove(m); }

// ---------------------------------------------------------------------------
// 1. Poof credit fixtures
// ---------------------------------------------------------------------------

// single pair poof (game 11): placed (5,5) with own stone (4,5) between op (3,5)/(6,5).
{
    const g = freshGame(11);
    seed(g, [[4, 5, 1], [3, 5, 2], [6, 5, 2]]);
    g.addMove(XY(5, 5)); // player 1 (moves was empty)
    check('poof-11 single pair credits 2', g.captures[1] === 2 && g.captures[2] === 0,
        `captures=${JSON.stringify(g.captures)}`);
    check('poof-11 single pair removes both stones',
        g.abstractBoard[4][5] === 0 && g.abstractBoard[5][5] === 0);
}

// mixed pair + triple poof (game 25): placed (5,5) triggers a horizontal pair poof AND
// a vertical triple poof. GameClass credits the placed stone once per poof function
// (the documented double-bonus): pair(1 neighbour +1 placed) + triple(2 neighbours +1
// placed) = 5.
{
    const g = freshGame(25);
    seed(g, [
        [4, 5, 1], [3, 5, 2], [6, 5, 2],          // horizontal pair
        [5, 4, 1], [5, 3, 1], [5, 2, 2], [5, 6, 2], // vertical triple (up)
    ]);
    g.addMove(XY(5, 5)); // player 1
    check('poof-25 pair+triple double-bonus credits 5', g.captures[1] === 5,
        `captures=${JSON.stringify(g.captures)}`);
    check('poof-25 removes placed + pair neighbour + 2 triple neighbours',
        g.abstractBoard[5][5] === 0 && g.abstractBoard[4][5] === 0 &&
        g.abstractBoard[5][4] === 0 && g.abstractBoard[5][3] === 0);
}

// ---------------------------------------------------------------------------
// 2. L1490 keryo-poof "down" bug fix
// ---------------------------------------------------------------------------

// Note: the two fixtures below document C++ parity for the old/new guard only -- `undefined` never equals a color in JS, so they are not themselves a JS-observable behavior change.

// down-poof at the deepest legal row (j=15 -> triple j..j+2, op flank at j+3=18): fires.
{
    const g = freshGame(25);
    seed(g, [[5, 16, 1], [5, 17, 1], [5, 14, 2], [5, 18, 2]]);
    g.addMove(XY(5, 15)); // player 1
    check('keryo-poof down at bottom edge fires (credit 3)', g.captures[1] === 3,
        `captures=${JSON.stringify(g.captures)}`);
    check('keryo-poof down removes the triple',
        g.abstractBoard[5][15] === 0 && g.abstractBoard[5][16] === 0 && g.abstractBoard[5][17] === 0);
}
// one row deeper (j=16) the op flank would be off-board (j+3=19): no poof, no phantom.
{
    const g = freshGame(25);
    seed(g, [[5, 17, 1], [5, 18, 1], [5, 15, 2]]);
    g.addMove(XY(5, 16)); // player 1
    check('keryo-poof down off-board flank does not poof', g.captures[1] === 0,
        `captures=${JSON.stringify(g.captures)}`);
}

// ---------------------------------------------------------------------------
// 3. #detectPenteOf row/col-0 exclusion fix (game 1 wires detectPenteOf in addMove)
// ---------------------------------------------------------------------------

// five along board row 0, touching col 0 as well: only detected after the >=0 fix.
{
    const g = freshGame(1);
    seed(g, [[0, 0, 1], [1, 0, 1], [2, 0, 1], [3, 0, 1]]);
    g.addMove(XY(4, 0)); // player 1 completes (0,0)..(4,0)
    check('detectPenteOf five touching row 0 / col 0 is detected', g.winner === 1,
        `winner=${g.winner}`);
}
// vertical five lying on column 0: exercises the same >=0 guard on the other axis
// (the "row"/x index is fixed at 0 for the whole scan, instead of the "col"/y index).
{
    const g = freshGame(1);
    seed(g, [[0, 0, 1], [0, 1, 1], [0, 2, 1], [0, 3, 1]]);
    g.addMove(XY(0, 4)); // player 1 completes (0,0)..(0,4)
    check('detectPenteOf vertical five touching column 0 is detected', g.winner === 1,
        `winner=${g.winner}`);
}
// diagonal five touching (0,0): hits the row-0 and col-0 guards simultaneously on the
// last step of the scan (i=0,j=0 together), the strictest corner case for the fix.
{
    const g = freshGame(1);
    seed(g, [[0, 0, 1], [1, 1, 1], [2, 2, 1], [3, 3, 1]]);
    g.addMove(XY(4, 4)); // player 1 completes (0,0)..(4,4)
    check('detectPenteOf diagonal five touching (0,0) is detected', g.winner === 1,
        `winner=${g.winner}`);
}
// a normal mid-board five is still detected (fix did not regress the common case).
{
    const g = freshGame(1);
    seed(g, [[5, 5, 1], [6, 5, 1], [7, 5, 1], [8, 5, 1]]);
    g.addMove(XY(9, 5)); // player 1 completes (5,5)..(9,5)
    check('detectPenteOf mid-board five still detected', g.winner === 1, `winner=${g.winner}`);
}

// ---------------------------------------------------------------------------
// 4. Capture-win thresholds in isGameOver
//    captures[c] = stones LOST by color c, so captures[loser] >= threshold -> 3-loser.
// ---------------------------------------------------------------------------

// Poof (11): threshold 10 WITH advantage.
{
    const g = freshGame(11); g.captures = [undefined, 10, 9];
    check('poof-11 capture win 10-9 -> winner 2', g.isGameOver() === true && g.winner === 2,
        `winner=${g.winner}`);
}
{
    const g = freshGame(11); g.captures = [undefined, 10, 10];
    check('poof-11 capture 10-10 continues (advantage)', g.isGameOver() === false, `winner=${g.winner}`);
}
// Boat (15): threshold 10, immediate (no advantage).
{
    const g = freshGame(15); g.captures = [undefined, 10, 5];
    check('boat-15 capture win 10 immediate -> winner 2', g.isGameOver() === true && g.winner === 2,
        `winner=${g.winner}`);
}
// O-Pente (25): threshold 15 WITH advantage.
{
    const g = freshGame(25); g.captures = [undefined, 15, 14];
    check('opente-25 capture win 15-14 -> winner 2', g.isGameOver() === true && g.winner === 2,
        `winner=${g.winner}`);
}
{
    const g = freshGame(25); g.captures = [undefined, 15, 15];
    check('opente-25 capture 15-15 continues (advantage)', g.isGameOver() === false, `winner=${g.winner}`);
}

// ---------------------------------------------------------------------------
// 5. Boat / O-Pente row-win survival fixtures (built via real move sequences)
// ---------------------------------------------------------------------------

// Boat (15) clean five: opponent to move, five is uncapturable -> game over, owner wins.
{
    const g = freshGame(15);
    playSeq(g, [XY(5, 5), XY(0, 18), XY(6, 5), XY(2, 18), XY(7, 5), XY(4, 18),
        XY(8, 5), XY(6, 18), XY(9, 5)]);
    check('boat-15 clean five -> gameOver, owner wins', g.isGameOver() === true && g.winner === 1,
        `winner=${g.winner}`);
}
// Boat (15) capturable five: opponent to move, one end pair is capturable -> not over.
{
    const g = freshGame(15);
    playSeq(g, [XY(5, 5), XY(5, 7), XY(6, 5), XY(0, 0), XY(7, 5), XY(2, 0),
        XY(8, 5), XY(4, 0), XY(5, 6), XY(6, 0), XY(9, 5)]);
    check('boat-15 capturable five -> not over', g.isGameOver() === false, `winner=${g.winner}`);
}
// Boat (15) capturable five survives to owner's next turn (opponent did not break it) -> win.
{
    const g = freshGame(15);
    playSeq(g, [XY(5, 5), XY(5, 7), XY(6, 5), XY(0, 0), XY(7, 5), XY(2, 0),
        XY(8, 5), XY(4, 0), XY(5, 6), XY(6, 0), XY(9, 5), XY(8, 0)]);
    check('boat-15 five survives opponent reply -> owner wins', g.isGameOver() === true && g.winner === 1,
        `winner=${g.winner}`);
}

// O-Pente (25) triple-capturable five: no pair is capturable but a triple is -> not over.
{
    const g = freshGame(25);
    playSeq(g, [XY(5, 5), XY(5, 8), XY(6, 5), XY(0, 0), XY(7, 5), XY(2, 0),
        XY(8, 5), XY(4, 0), XY(5, 6), XY(6, 0), XY(5, 7), XY(8, 0), XY(9, 5)]);
    check('opente-25 triple-capturable five -> not over', g.isGameOver() === false, `winner=${g.winner}`);
}
// O-Pente (25) triple-capturable five survives to owner's next turn -> win.
{
    const g = freshGame(25);
    playSeq(g, [XY(5, 5), XY(5, 8), XY(6, 5), XY(0, 0), XY(7, 5), XY(2, 0),
        XY(8, 5), XY(4, 0), XY(5, 6), XY(6, 0), XY(5, 7), XY(8, 0), XY(9, 5), XY(10, 0)]);
    check('opente-25 triple-capturable five survives -> owner wins', g.isGameOver() === true && g.winner === 1,
        `winner=${g.winner}`);
}

// ---------------------------------------------------------------------------
// 6. Connect6 (game 13): owner %4 pattern, 6+ win (overlines), no captures.
// ---------------------------------------------------------------------------

// owner of 0-based move index i is P1 if i%4 in {0,3}, else P2 -> 1,2,2,1,1,2,2,1,1,2,2,1.
{
    const g = freshGame(13);
    const owner = i => ((i % 4 === 0) || (i % 4 === 3)) ? 1 : 2;
    // scattered cells (all coords even -> never adjacent -> no accidental run/win).
    const cells = [];
    for (let i = 0; i < 12; i++) cells.push([(i % 5) * 2, Math.floor(i / 5) * 2]);
    let ok = true, detail = '';
    for (let i = 0; i < 12; i++) {
        const [x, y] = cells[i];
        g.addMove(XY(x, y));
        if (g.abstractBoard[x][y] !== owner(i)) { ok = false; detail = `move ${i} owner ${g.abstractBoard[x][y]} != ${owner(i)}`; break; }
    }
    check('connect6 owner sequence 1,2,2,1,1,2,2,1,1,2,2,1', ok, detail);
    check('connect6 scattered 12 moves set no winner', g.winner === undefined, `winner=${g.winner}`);
}

// currentPlayer stays with the owner across a two-stone turn boundary (does not flip
// after the FIRST stone of a two-stone turn, flips after the second).
{
    const g = freshGame(13);
    const seq = [g.currentPlayer()];                 // len 0 -> P1
    for (const [x, y] of [[0, 0], [2, 0], [4, 0], [6, 0], [8, 0], [10, 0]]) {
        g.addMove(XY(x, y));
        seq.push(g.currentPlayer());
    }
    // currentPlayer after 0..6 moves: 1, then 2,2 (P2 turn), then 1,1 (P1 turn), then 2,2.
    check('connect6 currentPlayer across two-stone turn boundary',
        JSON.stringify(seq) === JSON.stringify([1, 2, 2, 1, 1, 2, 2]), `seq=${JSON.stringify(seq)}`);
}

// six-in-a-row wins (placed stone completes (3,5)..(8,5) horizontally for P1).
{
    const g = freshGame(13);
    seed(g, [[3, 5, 1], [4, 5, 1], [5, 5, 1], [6, 5, 1], [7, 5, 1]]);
    g.addMove(XY(8, 5)); // first addMove -> owner(0)=P1
    check('connect6 six-in-row detected -> winner 1', g.winner === 1, `winner=${g.winner}`);
}

// OVERLINE of seven wins (6+ contiguous; allowOverlines(true) server parity).
{
    const g = freshGame(13);
    seed(g, [[3, 5, 1], [4, 5, 1], [5, 5, 1], [6, 5, 1], [7, 5, 1], [8, 5, 1]]);
    g.addMove(XY(9, 5)); // completes a seven-length run
    check('connect6 overline (7) wins -> winner 1', g.winner === 1, `winner=${g.winner}`);
}

// exactly five does NOT win.
{
    const g = freshGame(13);
    seed(g, [[3, 5, 1], [4, 5, 1], [5, 5, 1], [6, 5, 1]]);
    g.addMove(XY(7, 5)); // only five in a row
    check('connect6 five does NOT win', g.winner === undefined, `winner=${g.winner}`);
}

// five blocked on both ends by the opponent still does not win (only length matters).
{
    const g = freshGame(13);
    seed(g, [[2, 5, 2], [3, 5, 1], [4, 5, 1], [5, 5, 1], [6, 5, 1], [8, 5, 2]]);
    g.addMove(XY(7, 5)); // P1 five (3..7,5) flanked by P2 at (2,5) and (8,5)
    check('connect6 five blocked both ends stays no-win', g.winner === undefined, `winner=${g.winner}`);
}

// no captures: a Pente bracketing pattern (P1 - P2 P2 - P1) must NOT remove the pair.
{
    const g = freshGame(13);
    seed(g, [[3, 5, 1], [4, 5, 2], [5, 5, 2]]);
    g.addMove(XY(6, 5)); // P1 brackets the P2 pair -> would capture in Pente, must not here
    check('connect6 no captures through capture pattern (captures stay 0)',
        g.captures[1] === 0 && g.captures[2] === 0, `captures=${JSON.stringify(g.captures)}`);
    check('connect6 bracketed pair is NOT removed',
        g.abstractBoard[4][5] === 2 && g.abstractBoard[5][5] === 2);
}

// ---------------------------------------------------------------------------
// 7. undoMyTurn (UNDO_MOVE fix: pop back to a human-turn boundary, not a fixed
//    count of stones -- needed because Connect6 turns are two stones, not one).
// ---------------------------------------------------------------------------

// Pente (one-stone-per-turn), human P1 (me='white', default): H then AI's A.
// Old rootReducer behavior was undoMove() twice unconditionally; undoMyTurn()
// must reproduce that exactly here: pop both, landing back on an empty board
// where it is the human's turn again.
{
    const g = freshGame(1);
    g.addMove(XY(9, 9));  // H: move 0, owner P1 (human)
    g.addMove(XY(0, 0));  // A: move 1, owner P2 (AI)
    g.undoMyTurn();
    check('pente undoMyTurn pops both H and A -> empty board',
        g.moves.length === 0, `moves.length=${g.moves.length}`);
    check('pente undoMyTurn lands on human turn', g.isMyTurn() === true,
        `isMyTurn=${g.isMyTurn()}`);
}

// Connect6, human P1 (me='white'): human's opening single stone H, then the
// AI's full two-stone reply A1,A2. Before undo: moves = [H,A1,A2], length 3,
// currentPlayer(3) = owner(3) = P1 -> already the human's turn (they're about
// to start their own two-stone turn). Naively one might expect undoMyTurn to
// pop only the AI's reply (A2,A1), stopping at length 1. But currentPlayer(1)
// = owner(1) = P2 (AI) -- at length 1 it is the AI's turn to reply to H, not
// the human's -- so the loop must keep going and also pop H, landing on the
// empty board (length 0), where currentPlayer(0) = owner(0) = P1 = human.
{
    const g = freshGame(13);
    g.addMove(XY(0, 0));  // H:  move 0, owner P1 (human)
    g.addMove(XY(2, 0));  // A1: move 1, owner P2 (AI)
    g.addMove(XY(4, 0));  // A2: move 2, owner P2 (AI)
    check('connect6 (P1) sanity: human turn already at length 3 before undo',
        g.isMyTurn() === true, `isMyTurn=${g.isMyTurn()}`);
    g.undoMyTurn();
    check('connect6 (P1) undoMyTurn pops A2,A1,H all the way to empty board',
        g.moves.length === 0, `moves.length=${g.moves.length}`);
    check('connect6 (P1) undoMyTurn lands on human turn', g.isMyTurn() === true,
        `isMyTurn=${g.isMyTurn()}`);
}

// Connect6, human P2 (me='black'), mid-game across a full two-stone AI turn.
// Play 9 scattered non-adjacent moves (indices 0..8): owners by i%4 are
// 1,2,2,1,1,2,2,1,1 -- AI opens single at move 0, then two-stone turns
// human{1,2}, AI{3,4}, human{5,6}, AI{7,8}. After move 8, currentPlayer(9) =
// owner(9) = P2 = human's turn again (nothing played yet this turn).
// undoMyTurn from here must undo the AI's last full turn {7,8} and keep going
// past currentPlayer(7) = owner(7) = P1 (still the AI's turn to resume, not a
// valid stop) down to length 6, where currentPlayer(6) = owner(6) = P2 =
// human (mid the human's own prior two-stone turn, having already placed
// move 5) -- a safe, non-dead stopping point.
{
    const g = freshGame(13);
    g.me = 'black';
    const cells = [];
    for (let i = 0; i < 9; i++) cells.push([(i % 5) * 2, Math.floor(i / 5) * 2]);
    for (const [x, y] of cells) g.addMove(XY(x, y));
    check('connect6 (P2) sanity: human turn already at length 9 before undo',
        g.isMyTurn() === true, `isMyTurn=${g.isMyTurn()}`);
    g.undoMyTurn();
    check('connect6 (P2) undoMyTurn pops the AI\'s whole last turn (8,7) and stops at 6',
        g.moves.length === 6, `moves.length=${g.moves.length}`);
    check('connect6 (P2) undoMyTurn lands on human turn', g.isMyTurn() === true,
        `isMyTurn=${g.isMyTurn()}`);
}

// Connect6, AI as P1 (human me='black'), only the AI's forced single opening
// stone played and nothing else -- the human has not moved yet this turn (the
// Undo button has no disabled/guard condition in GameInfoPanel.js, so it can
// be pressed here). undoMyTurn unconditionally pops once (down to the empty
// board, length 0), where currentPlayer(0) = owner(0) = P1 = AI, so
// isMyTurn() is actually false. That is fine: an empty board is never a dead
// state (it's the same state the app starts a fresh AI-opens game from), so
// the safety invariant is isMyTurn() === true OR moves.length === 0, not
// isMyTurn() === true alone.
{
    const g = freshGame(13);
    g.me = 'black';
    g.addMove(XY(9, 9)); // AI's forced opening stone, move 0, owner P1
    check('connect6 (P2, only AI opening played) sanity: already human turn',
        g.isMyTurn() === true, `isMyTurn=${g.isMyTurn()}`);
    g.undoMyTurn();
    check('connect6 (P2, only AI opening played) undoMyTurn empties the board',
        g.moves.length === 0, `moves.length=${g.moves.length}`);
    check('connect6 (P2, only AI opening played) undoMyTurn: no dead state (isMyTurn or empty board)',
        g.isMyTurn() === true || g.moves.length === 0,
        `isMyTurn=${g.isMyTurn()}, moves.length=${g.moves.length}`);
}

// ---------------------------------------------------------------------------
console.log(`\n${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
