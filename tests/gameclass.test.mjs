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
console.log(`\n${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
