import React  from 'react';
import BoardSquare from './BoardSquare';
import { GameState, Game } from "../Classes/GameClass";
import { ADD_MOVE } from "../redux_actions/actionTypes";
import { connect } from 'react-redux';
import PropTypes from 'prop-types';
// import {send_message} from "../redux_actions/actionTypes";

const mapStateToProps = state => {
    return {
        game: state.game,
        game_id: state.game.game,
        started: state.started
    }
};

const mapDispatchToProps = dispatch => {
    return {
        send_move: move => {
            dispatch({type: ADD_MOVE, payload: move});
        },
    }
};


const UnconnectedBoard = (props) => {
    const { started, game_id, game, send_move } = props;

    // console.log(JSON.stringify(game.abstractBoard))

    const makeBoard = (gridsize) => {
        if (game === undefined) { return []; }
        let board = [];
        let player_colors = [undefined, 'white-stone-gradient', 'black-stone-gradient'];
        if (game.isGo()) { player_colors = [undefined, 'black-stone-gradient', 'white-stone-gradient']; }
        let hover = player_colors[game.currentColor()];
        if (game.isGo() && game.gameState.goState === GameState.GoState.MARK_STONES) {
            hover = 'red-stone-gradient';
        }
        // const myTurn = table.isMyTurn(game) && game.gameState.state === GameState.State.STARTED;
        const myTurn = started && game.isMyTurn();
        // console.log('my turn: ', myTurn);
        // console.log('my turn: ', table.isMyTurn(game));
        // console.log('makeBoard');
        for(let j = 0; j < gridsize; j++) {
            for (let i = 0; i < gridsize; i++) {
                const m = j*gridsize + i;
                let squaretype;
                if (m === 0) {
                    squaretype = 1;
                } else if (m === gridsize - 1) {
                    squaretype = 3;
                } else if (m === gridsize * gridsize - 1) {
                    squaretype = 9;
                } else if (m === gridsize * (gridsize - 1)) {
                    squaretype = 7;
                } else if (j === 0) {
                // } else if (Math.floor(m / gridsize) === 0) {
                    squaretype = 2;
                } else if (j === gridsize - 1) {
                // } else if (Math.floor(m / gridsize) === gridsize - 1) {
                    squaretype = 8;
                // } else if (m % gridsize === 0) {
                } else if (i === 0) {
                    squaretype = 4;
                // } else if (m % gridsize === gridsize - 1) {
                } else if (i === gridsize - 1) {
                    squaretype = 6;
                } else {
                    squaretype = 5;
                }

                const stone = player_colors[game.abstractBoard[i][j]];
                let clickHandler = undefined;
                if (myTurn) {
                    if (game.abstractBoard[i][j] === 0) {
                        clickHandler = send_move;
                    }
                    if (game.isGo() && game.gameState.goState === GameState.GoState.MARK_STONES) {
                        if (clickHandler === undefined) {
                            clickHandler = send_move;
                        } else {
                            clickHandler = undefined;
                        }
                    }
                }
                board.push({ key: m, gridsize: gridsize, id: m,
                            part: squaretype, stone: stone,
                            clickHandler: clickHandler,
                            hover: hover});
            }
        }
        if (game.isGo() && game.gameState.goState > GameState.GoState.PLAY) {
            for (let i = 1; i < 3; i++) {
                game.goDeadStonesByPlayer[i].forEach(s => {
                    board[s].deadStone = player_colors[i];
                });
            }
            for (let i = 1; i < 3; i++) {
                // console.log(JSON.stringify(game.goTerritoryByPlayer[i]))
                game.goTerritoryByPlayer[i].forEach(s => {
                    board[s].territory = i;
                });
            }
        }
        if (game_id < 19 || game_id > 24) {
            const circles = [120, 126, 180, 234, 240];
            circles.forEach(c => { board[c].part = 51; });
        } else {
            let dots = [];
            if (game_id === 19 || game_id === 20) {
                dots = [60, 66, 72, 174, 180, 186, 288, 294, 300];
            } else if (game_id === 21 || game_id === 22) {
                dots = [20, 24, 40, 56, 60];
            } else if (game_id === 23 || game_id === 24) {
                dots = [42, 45, 48, 81, 84, 87, 120, 123, 126];
            }
            dots.forEach(d => { board[d].part = 52; });
        }
        const lastMoves = game.last_move();
        lastMoves.forEach(move => {
            if (move !== undefined) {
                board[move].last_move = true;
            }
        });

        return board.map(p => <BoardSquare {...p}/>);
    };
    const makeCoordinateBoundaries = (gridsize) => {
        const coordinateLetters =['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'];
        let coordinates = [];
        for ( let i = 1; i <= gridsize; i++ ) {
            coordinates.push(
                <text key={i} y={1} fontSize={4} transform={'translate(0,'+(10*i)+')'}>
                    {gridsize - i + 1}
                </text>
            );
            coordinates.push(
                <text key={coordinateLetters[i]+i} y={4} fontSize={4} transform={'translate('+(10*i)+',0)'}>
                    {coordinateLetters[i-1]}
                </text>
            )
        }
        return coordinates;
    };

    let style;
    if (game_id < 3) {
        style = 'pente'
    } else if (game_id < 5) {
        style = 'keryo-pente'
    } else if (game_id < 7) {
        style = 'gomoku'
    } else if (game_id < 9) {
        style = 'd-pente'
    } else if (game_id < 11) {
        style = 'g-pente'
    } else if (game_id < 13) {
        style = 'poof-pente'
    } else if (game_id < 15) {
        style = 'connect6'
    } else if (game_id < 17) {
        style = 'boat-pente'
    } else if (game_id < 19) {
        style = 'dk-pente'
    } else if (game_id < 25) {
        style = 'go'
    } else {
        style = 'o-pente'
    }
    let gridsize = 19;
    if (game_id === 21 || game_id === 22) { gridsize = 9; }
    if (game_id === 23 || game_id === 24) { gridsize = 13; }
    return (
        <svg id="svgboard" height={'100%'} viewBox={'0 0 '+(10*(gridsize + 1))+' '+(10*(gridsize + 1))}>
            <g id={'whole'} transform={'translate(5,5)'}>
            <g id="boardgroup" transform={'scale(0.95, 0.95) translate(5,5)'}>
                <filter id="f3" x="0" y="0" width="150%" height="150%">
                    <feOffset result="offOut" in="SourceAlpha" dx={4} dy={4} />
                    <feGaussianBlur result="blurOut" in="offOut" stdDeviation="3" />
                    <feBlend in="SourceGraphic" in2="blurOut" mode="normal" />
                </filter>
                <rect height={10*gridsize} width={10*gridsize} filter={"url(#f3)"} className={'board '+style}/>
                {makeBoard(gridsize)}
            </g>
            <g id="coordinates" transform={'scale(0.95, 0.95)'}>
            {makeCoordinateBoundaries(gridsize)}
            </g>
            </g>
        </svg>
    );
};

UnconnectedBoard.propTypes = {
    game: PropTypes.instanceOf(Game).isRequired
};

const Board = connect(mapStateToProps, mapDispatchToProps)(UnconnectedBoard);

export default Board;