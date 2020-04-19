#!/bin/bash
emcc Ai.cpp mmai.cpp -O3 -o ai.wasm -s WASM=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" -std=c++11 --embed-file files/
