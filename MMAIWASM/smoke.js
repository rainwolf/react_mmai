// Smoke test the WASM engine under Node.
// The emscripten glue's `var Module` hoists over global.Module in CJS scope,
// so we require a temp copy with a prelude that pre-assigns Module.
const fs = require('fs');
const path = require('path');

const aiJs = process.argv[2]; // absolute path to ai.js
const dir = path.dirname(aiJs);
const tmp = path.join(dir, '.smoke_ai.js');

const games = [1, 2, 3, 4, 11, 13, 14, 15, 25, 26];
const moves = [180, 161, 104, 200, 144, 198, 201];

// Connect6 (game 13/14) returns TWO stones packed as m1*362 + m2 (base 362,
// not 361 -- base 361 is ambiguous since m1*361+361 == (m1+1)*361+0), where
// m2 is a real cell 0..360 or the sentinel 361 (single-stone turn). Every
// other variant returns a plain move index 0..360. See mmai.cpp for the
// encoding.
function validMove(g, mv) {
  if (!Number.isInteger(mv) || mv < 0) return false;
  if (g === 13 || g === 14) {
    const m1 = Math.floor(mv / 362), m2 = mv % 362;
    const m1ok = m1 >= 0 && m1 <= 360 && !moves.includes(m1);
    const m2ok = m2 >= 0 && m2 <= 361 && (m2 === 361 || !moves.includes(m2));
    return m1ok && m2ok && (m2 === 361 || m1 !== m2);
  }
  return mv <= 360 && !moves.includes(mv);
}

global.__SMOKE_M = {
  onRuntimeInitialized: () => {
    const M = global.__SMOKE_M;
    const ptr = M._malloc(moves.length * 4);
    const bytes = new Uint8Array(new Int32Array(moves).buffer);
    let ok = true;
    for (const g of games) {
      M.HEAPU8.set(bytes, ptr);
      const mv = M.ccall(
        'getAIMove', 'number',
        ['number', 'number', 'number', 'number', 'number'],
        [g, 3, 1, ptr, moves.length]
      );
      const valid = validMove(g, mv);
      if (!valid) ok = false;
      const decoded = (g === 13 || g === 14)
        ? ` (m1=${Math.floor(mv / 362)}, m2=${mv % 362})` : '';
      console.log(`game ${g} -> ${mv}${decoded} ${valid ? 'OK' : 'INVALID'}`);
    }
    M._free(ptr);
    fs.unlinkSync(tmp);
    console.log(ok ? 'SMOKE PASS' : 'SMOKE FAIL');
    process.exit(ok ? 0 : 1);
  },
};

fs.writeFileSync(tmp, 'var Module = global.__SMOKE_M;\n' + fs.readFileSync(aiJs, 'utf8'));
setInterval(() => {}, 100);
setTimeout(() => { try { fs.unlinkSync(tmp); } catch (e) {} console.log('SMOKE TIMEOUT'); process.exit(2); }, 60000);
require(tmp);
