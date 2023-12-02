#!/bin/bash
emcc Ai.cpp mmai.cpp -o ai.js -s WASM=1 -s "EXPORTED_RUNTIME_METHODS=['ccall']" \
  -s TOTAL_STACK=32MB -s INITIAL_MEMORY=64MB \
  -s "EXPORTED_FUNCTIONS=['_malloc', '_free']" -std=c++11 --embed-file files/
