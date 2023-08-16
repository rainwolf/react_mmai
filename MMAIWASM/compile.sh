#!/bin/bash
emcc Ai.cpp mmai.cpp -O3 -o ai.js -s WASM=1 -s "EXPORTED_RUNTIME_METHODS=['ccall']" -std=c++11 --embed-file files/
