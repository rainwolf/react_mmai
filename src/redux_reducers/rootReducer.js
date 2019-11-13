import '../redux_actions/actionTypes';
import {
    START_GAME, MOVE_BACK, MOVE_FORWARD, MOVE_GOTO,
    REMOVE_SNACK, STORE_MOVE, ADD_MOVE, RESET_BOARD,
    START_THINKING, STOP_THINKING, LOAD_WASM, CHANGE_GAME,
    CHANGE_LEVEL, UNDO_MOVE, CHANGE_COLOR, SUBSCRIBER
} from "../redux_actions/actionTypes";
import { Game } from '../Classes/GameClass';

const initialState = {
    subscriber: true,
    game: new Game(),
    started: false,
    thinking: false,
    level: 1
};

// async function loadWasm(state) {
//     // const response = await fetch('http://localhost:8080/MMAIWASM/ai.wasm');
//     // const buffer = await response.arrayBuffer();
//     // const obj = await WebAssembly.instantiate(buffer);    // const wasmImport = await import('../wasm/ai.js');
//     // state.wsm = obj;
//     const { instance } = await WebAssembly.instantiateStreaming(fetch('http://localhost:8080/MMAIWASM/ai.wasm'));
//     state.wasm = instance;
// }

function mmaiApp (state = initialState, action) {
    let newState = { ...state };
    let newGame;
    // console.log(JSON.stringify(action))
    switch(action.type) {
        // case LOAD_WASM:
        //     loadWasm(newState);
        //     break;
        case SUBSCRIBER:
            newState.subscriber = action.payload;
            break;
        case CHANGE_COLOR:
            newGame = newState.game.newInstance();
            newGame.reset();
            if (newGame.me === 'white') {
                newGame.me = 'black';
            } else {
                newGame.me = 'white';
            }
            newState.game = newGame;
            newState.started = false;
            newState.snack = undefined;
            break;
        case CHANGE_LEVEL:
            newState.level = parseInt(action.payload);
            break;
        case UNDO_MOVE:
            if (newState.started) {
                newGame = newState.game.newInstance();
                newGame.undoMove();
                if (!newGame.isMyTurn()) {
                    newGame.undoMove();
                }
                newState.game = newGame;
            }
            break;
        case CHANGE_GAME:
            newGame = newState.game.newInstance();
            newGame.reset();
            if (newGame.game === 1) {
                newGame.setGame(3);
            } else {
                newGame.setGame(1);
            }
            newState.game = newGame;
            newState.started = false;
            newState.snack = undefined;
            break;
        case RESET_BOARD:
            newGame = newState.game.newInstance();
            newGame.reset();
            newState.game = newGame;
            newState.started = true;
            newState.snack = undefined;
            break;
        case STORE_MOVE:
            newGame = newState.game.newInstance();
            newGame.addMove(action.payload);
            newState.game = newGame;
            if (newGame.isGameOver()) {
                newState.started = false;
                if ((newGame.me === 'white' && newGame.winner === 1) || (newGame.me === 'black' && newGame.winner === 2)) {
                    newState.snack = 1;
                } else {
                    newState.snack = 0;
                }
            }
            break;
        case START_THINKING:
            newState.thinking = true;
            break;
        case STOP_THINKING:
            newState.thinking = false;
            break;
        case MOVE_BACK:
            // moveForwardBack(false, newState);
            break;
        case MOVE_FORWARD:
            // moveForwardBack(true, newState);
            break;
        case MOVE_GOTO:
            // moveGoTo(action.payload, newState);
            break;
        case REMOVE_SNACK:
            delete newState.snack;
            break;
        default: break;
    }
    return newState;
}

export default mmaiApp;