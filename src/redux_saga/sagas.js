import { call, put, takeEvery, all, select, delay } from 'redux-saga/effects'
import { ADD_MOVE, STORE_MOVE, RESET_BOARD,
        START_THINKING, STOP_THINKING } from '../redux_actions/actionTypes';
import move_sound_file from '../sounds/move_sound.mp3';

const move_sound = new Audio(move_sound_file);

async function playSound(sound) {
    try {
        await sound.play();
    } catch(err) {
        if (process.env.NODE_ENV === 'development') {
            console.log('oops, no sound ', err);
        }
    }
}

function* addMove(action) {
    yield put({type: STORE_MOVE, payload: action.payload});
    yield delay(100);
    let game = yield select((state) => state.game);
    if (!game.isMyTurn() && !game.isGameOver()) {
        yield put({type: START_THINKING});
        yield delay(100);
        const Module = document.ai_module;
        const _arrayToHeap = function(typedArray){
            let numBytes = typedArray.length * typedArray.BYTES_PER_ELEMENT;
            let ptr = Module._malloc(numBytes);
            let heapBytes = new Uint8Array(Module.HEAPU8.buffer, ptr, numBytes);
            heapBytes.set(new Uint8Array(typedArray.buffer));
            return heapBytes;
        };
        let typedArray = new Int32Array(game.moves);
        let heapBytes = _arrayToHeap(typedArray);
        // Pass the canonical game id straight through. The WASM engine accepts
        // 1/3/11/15/25 (and legacy 2 = Keryo). ccall arg order matches
        // getAIMove(game, level, openingBook, moves*, numMoves) in MMAIWASM/mmai.cpp.
        let level = yield select((state) => state.level);
        let o = yield select((state) => state.opening_book);
        let move = Module.ccall('getAIMove', 'number',['number','number','number','number','number'], [game.game, level, o, heapBytes.byteOffset, typedArray.length]);
        yield Module._free(heapBytes.byteOffset);
        yield put({type: ADD_MOVE, payload: move});
        yield call(playSound, move_sound);
        yield put({type: STOP_THINKING});
    }
}
function* startGame(action) {
    yield put({type: RESET_BOARD});
    yield put({type: ADD_MOVE, payload: 180});
}



function* addMoveSaga() {
    yield takeEvery("ADD_MOVE", addMove)
}
function* startGameSaga() {
    yield takeEvery("START_GAME", startGame)
}

export default function* rootSaga() {
    yield all([
        addMoveSaga(),
        startGameSaga()
    ])
}