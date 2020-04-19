const util = require('util');
const fs = require('fs');
var source = fs.readFileSync('./ai.wasm');
const env = {
  __memory_base: 0,
  tableBase: 0,
  memory: new WebAssembly.Memory({
    initial: 256
  }),
  table: new WebAssembly.Table({
    initial: 0,
    element: 'anyfunc'
  })
}

var typedArray = new Uint8Array(source);

WebAssembly.instantiate(typedArray, {
  env: env
}).then(result => {
  console.log(util.inspect(result, true, 0));
  console.log(result.instance.exports._getAIMove(10, 9, [10]));
}).catch(e => {
  // error caught
  console.log(e);
});