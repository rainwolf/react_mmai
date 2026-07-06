// Engine<->referee sanity (gate 3): the WASM engine plays both sides for ~20 plies of
// each variant (11/15/25); every engine move is re-applied through GameClass.addMove and
// the board is checked for consistency (legal empty target, at most the placed stone
// added, everything else only removed, running stone-count == moves - removals).
// Run: npm run test:engine
import { readFileSync, writeFileSync, unlinkSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { pathToFileURL, fileURLToPath } from 'node:url';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const here = dirname(fileURLToPath(import.meta.url));

// GameClass as a temp *.mjs (see gameclass.test.mjs for why).
const gcTmp = join(tmpdir(), `gameclass.eng.${process.pid}.mjs`);
writeFileSync(gcTmp, readFileSync(join(here, '..', 'src', 'Classes', 'GameClass.js'), 'utf8'));
const { Game } = await import(pathToFileURL(gcTmp).href);
unlinkSync(gcTmp);

// WASM engine glue (emscripten CJS). Prepend `var Module = ...` per MMAIWASM/smoke.js and
// require it from public/ so ai.wasm resolves next to the glue.
const aiJs = join(here, '..', 'public', 'ai.js');
const glueTmp = join(dirname(aiJs), `.engref_ai.${process.pid}.js`);
const M = {};
const ready = new Promise((resolve) => { M.onRuntimeInitialized = () => resolve(); });
global.__ENGREF_M = M;
writeFileSync(glueTmp, 'var Module = global.__ENGREF_M;\n' + readFileSync(aiJs, 'utf8'));
require(glueTmp);
await ready;
try { unlinkSync(glueTmp); } catch (e) { /* ignore */ }

function engineMove(gameId, movesArr, level, book) {
    const ta = new Int32Array(movesArr);
    const ptr = M._malloc(ta.length * 4);
    M.HEAPU8.set(new Uint8Array(ta.buffer), ptr);
    const mv = M.ccall('getAIMove', 'number',
        ['number', 'number', 'number', 'number', 'number'],
        [gameId, level, book, ptr, ta.length]);
    M._free(ptr);
    return mv;
}

let pass = 0, fail = 0;
function check(name, cond, extra = '') {
    if (cond) { pass++; console.log(`ok   - ${name}`); }
    else { fail++; console.log(`FAIL - ${name}${extra ? '  :: ' + extra : ''}`); }
}
function freshGame(id) { const g = new Game(); g.setGame(id); g.reset(); g.rated = false; return g; }
function countStones(g) {
    let n = 0;
    for (let x = 0; x < 19; x++) for (let y = 0; y < 19; y++) {
        const v = g.abstractBoard[x][y];
        if (v === 1 || v === 2) n++;
    }
    return n;
}
function snapshot(g) { const s = []; for (let x = 0; x < 19; x++) s.push(g.abstractBoard[x].slice()); return s; }
// True if `color` has a run of 6+ contiguous stones anywhere (scans the board directly,
// independent of GameClass's own detector).
function hasSixContiguous(g, color) {
    const B = g.abstractBoard;
    const dirs = [[1, 0], [0, 1], [1, 1], [1, -1]];
    for (let x = 0; x < 19; x++) for (let y = 0; y < 19; y++) {
        if (B[x][y] !== color) continue;
        for (const [dx, dy] of dirs) {
            let n = 1, cx = x + dx, cy = y + dy;
            while (cx >= 0 && cx < 19 && cy >= 0 && cy < 19 && B[cx][cy] === color) { n++; cx += dx; cy += dy; }
            if (n >= 6) return true;
        }
    }
    return false;
}

for (const gameId of [11, 15, 25]) {
    const g = freshGame(gameId);
    g.addMove(180); // opening center (matches START_GAME saga), player 1
    let expected = countStones(g);
    let prev1 = g.captures[1], prev2 = g.captures[2];
    let ok = true, detail = '', plies = 0;
    for (let ply = 0; ply < 20 && !g.isGameOver(); ply++) {
        const before = snapshot(g);
        const mover = g.currentPlayer();
        const mv = engineMove(gameId, g.moves, 1, 0);
        if (!Number.isInteger(mv) || mv < 0 || mv > 360) { ok = false; detail = `illegal engine move ${mv} at ply ${ply}`; break; }
        const x = mv % 19, y = Math.floor(mv / 19);
        const cell = g.abstractBoard[x][y];
        if (!(cell === 0 || cell === -1)) { ok = false; detail = `engine move ${mv} onto occupied cell (${cell}) at ply ${ply}`; break; }

        g.addMove(mv);
        plies++;

        let added = 0, removed = 0, addedElsewhere = false, colorFlip = false;
        for (let x2 = 0; x2 < 19; x2++) for (let y2 = 0; y2 < 19; y2++) {
            const b = before[x2][y2], a = g.abstractBoard[x2][y2];
            const bOcc = (b === 1 || b === 2), aOcc = (a === 1 || a === 2);
            if (!bOcc && aOcc) { added++; if (!(x2 === x && y2 === y && a === mover)) addedElsewhere = true; }
            else if (bOcc && !aOcc) { removed++; }
            else if (bOcc && aOcc && a !== b) { colorFlip = true; }
        }
        if (added > 1 || addedElsewhere) { ok = false; detail = `added ${added} stones (elsewhere=${addedElsewhere}) at ply ${ply} move ${mv}`; break; }
        if (colorFlip) { ok = false; detail = `stone changed color at ply ${ply}`; break; }
        expected += added - removed;
        const actual = countStones(g);
        if (expected !== actual) { ok = false; detail = `stone-count desync expected ${expected} actual ${actual} at ply ${ply}`; break; }
        if (g.captures[1] < prev1 || g.captures[2] < prev2) { ok = false; detail = `captures decreased at ply ${ply}`; break; }
        prev1 = g.captures[1]; prev2 = g.captures[2];
    }
    check(`engine<->referee replay game ${gameId} (${plies} plies, caps ${prev1}-${prev2})`, ok, detail);

    // Guard against a premature-false win: the loop above exits early as soon as
    // g.isGameOver() goes true, and (if `ok` never got set to false) the check above would
    // still pass even if that happened after only a couple of plies due to a bogus win
    // condition. Require either a full-length replay, or a genuine, explained game-over.
    check(`engine<->referee game ${gameId} replay length is plausible (full 20 plies or a real isGameOver)`,
        plies === 20 || g.isGameOver() === true,
        `plies=${plies} caps=${JSON.stringify(g.captures)}`);
    check(`engine<->referee game ${gameId} did not end implausibly early (>= 8 plies)`,
        plies >= 8,
        `plies=${plies} caps=${JSON.stringify(g.captures)}`);
}

// ---------------------------------------------------------------------------
// Connect6 (game 13): the engine returns the AI's two stones PACKED in base 362
// (packed = m1*362 + m2; m2 === 361 is the single-stone sentinel, only on an empty
// board). Drive the engine as BOTH players for ~8 two-stone turns, decode each packed
// return, and apply m1/m2 individually through GameClass.addMove. Assert: every stone
// lands on an empty cell, GameClass currentPlayer agrees with the engine owner for the
// whole turn, stone-count === moves length, captures stay 0, and any real game-over has
// a genuine 6+ run for the winner.
{
    const gameId = 13;
    const g = freshGame(gameId);
    g.addMove(180); // opening center (matches START_GAME saga): P1's single first stone
    let ok = true, detail = '', turns = 0;
    for (let turn = 0; turn < 8 && !g.isGameOver(); turn++) {
        const mover = g.currentPlayer(); // owner of this whole two-stone turn
        const packed = engineMove(gameId, g.moves, 1, 0);
        if (!Number.isInteger(packed) || packed < 0) { ok = false; detail = `bad packed ${packed} turn ${turn}`; break; }
        const m1 = Math.floor(packed / 362), m2 = packed % 362;
        const stones = (m2 === 361) ? [m1] : [m1, m2];
        let bad = false;
        for (const mv of stones) {
            if (!Number.isInteger(mv) || mv < 0 || mv > 360) { ok = false; detail = `illegal stone ${mv} (packed ${packed}) turn ${turn}`; bad = true; break; }
            const x = mv % 19, y = Math.floor(mv / 19);
            if (g.abstractBoard[x][y] !== 0) { ok = false; detail = `stone ${mv} onto occupied (${g.abstractBoard[x][y]}) turn ${turn}`; bad = true; break; }
            if (g.currentPlayer() !== mover) { ok = false; detail = `currentPlayer ${g.currentPlayer()} != engine owner ${mover} mid-turn ${turn}`; bad = true; break; }
            g.addMove(mv);
        }
        if (bad) break;
        turns++;
        if (countStones(g) !== g.moves.length) { ok = false; detail = `stone-count ${countStones(g)} != moves ${g.moves.length} turn ${turn}`; break; }
        if (g.captures[1] !== 0 || g.captures[2] !== 0) { ok = false; detail = `captures nonzero ${JSON.stringify(g.captures)} turn ${turn}`; break; }
    }
    check(`engine<->referee connect6 (13) two-stone replay (${turns} turns)`, ok, detail);
    check('engine<->referee connect6 stone-count == moves length',
        countStones(g) === g.moves.length, `stones=${countStones(g)} moves=${g.moves.length}`);
    check('engine<->referee connect6 captures stay 0',
        g.captures[1] === 0 && g.captures[2] === 0, `caps=${JSON.stringify(g.captures)}`);
    if (g.isGameOver()) {
        check('engine<->referee connect6 game-over winner really has 6+ contiguous',
            hasSixContiguous(g, g.winner), `winner=${g.winner}`);
    } else {
        check('engine<->referee connect6 ran the full 8 turns without a bogus game-over',
            turns === 8, `turns=${turns}`);
    }
}

console.log(`\n${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
