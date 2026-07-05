// Smoke test the WASM engine under Node.
// The emscripten glue's `var Module` hoists over global.Module in CJS scope,
// so we require a temp copy with a prelude that pre-assigns Module.
const fs = require('fs');
const path = require('path');

const aiJs = process.argv[2]; // absolute path to ai.js
const dir = path.dirname(aiJs);
const tmp = path.join(dir, '.smoke_ai.js');

const games = [1, 2, 3, 4, 11, 13, 15, 25];
const moves = [180, 161, 104, 200, 144, 198, 201];

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
      const valid = Number.isInteger(mv) && mv >= 0 && mv <= 360 && !moves.includes(mv);
      if (!valid) ok = false;
      console.log(`game ${g} -> ${mv} ${valid ? 'OK' : 'INVALID'}`);
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
