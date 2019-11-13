export const GameState = {
    State: {
        NOT_STARTED: 1,
        STARTED: 2,
        PAUSED: 3,
        HALFSET: 4
    },
    DPenteState: {
        NO_CHOICE: 0,
        SWAPPED: 1,
        NOT_SWAPPED: 2
    },
    GoState: {
        PLAY: 0,
        MARK_STONES: 1,
        EVALUATE_STONES: 2
    }
};

const coordinateLetters = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'];

export class Game {
    constructor() {
        this.game = 1;
        this.me = 'white';
        this.rated = false;
        this.winner = undefined;
        this.abstractBoard = [[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        ];
        this.reset();
    }
    newInstance = () => {
        const newGame = new Game();
        newGame.captures = this.captures;
        newGame.moves = this.moves;
        newGame.goGroupsByPlayerAndID = this.goGroupsByPlayerAndID;
        newGame.goStoneGroupIDsByPlayer = this.goStoneGroupIDsByPlayer;
        newGame.koMove = this.koMove;
        newGame.suicideAllowed = this.suicideAllowed;
        newGame.goTerritoryByPlayer = this.goTerritoryByPlayer;
        newGame.hasPass = this.hasPass;
        newGame.doublePass = this.doublePass;
        newGame.goDeadStonesByPlayer = this.goDeadStonesByPlayer;
        newGame.setGame(this.game);
        newGame.rated = this.rated;
        newGame.me = this.me;
        newGame.gameState = { ...this.gameState };
        // for (let i = 0; i < 19; i++) {
        //     for (let j = 0; j < 19; j++) {
        //         newGame.abstractBoard[i][j] = this.abstractBoard[i][j];
        //     }
        // }
        newGame.abstractBoard = this.abstractBoard;
        newGame.mark_dead_stones_player = this.mark_dead_stones_player;
        newGame.evaluate_stones_player = this.evaluate_stones_player;
        newGame.until = this.until;
        newGame.winner = this.winner;

        return newGame;
    };
    
    setGame = (game) => {
        this.game = game;
        if (game < 21) {
            this.gridSize = 19;
        } else if (game < 23) {
            this.gridSize = 9;
        } else if (game < 25) {
            this.gridSize = 13;
        } else {
            this.gridSize = 19;
        }
    };
    
    critical_captures = (color) => {
        let threshold = 7;
        if (this.game === 3 || this.game === 4 || this.game === 17 || this.game === 18 || this.game === 25 || this.game === 26) {
            threshold = 11;
        } else if (this.isGo()) {
            return false;
        } 
        return this.captures[color] > threshold;
    };

    game_name = (g) => {
        if (g === undefined) {
            g = this.game;
        }
        let name;
        if (g < 3) {
            name = 'Pente';
        } else if (g < 5) {
            name = 'Keryo-Pente';
        } else if (g < 7) {
            name = 'Gomoku';
        } else if (g < 9) {
            name = 'D-Pente';
        } else if (g < 11) {
            name = 'G-Pente';
        } else if (g < 13) {
            name = 'Poof-Pente';
        } else if (g < 15) {
            name = 'Connect6';
        } else if (g < 17) {
            name = 'Boat-Pente';
        } else if (g < 19) {
            name = 'DK-Pente';
        } else if (g < 21) {
            name = 'Go';
        } else if (g < 23) {
            name = 'Go (9x9)';
        } else if (g < 25) {
            name = 'Go (13x13)';
        } else if (g < 27) {
            name = 'O-Pente';
        }
        if (g % 2 === 0) {
            return 'Speed '+name;
        }
        return name;
    };

    gameHasCaptures = () => {
        return this.game < 5 || this.game > 6;
    };
    
    isGameOver = () => {
        if (this.winner) {
            return true;
        }
        if (this.game === 1) {
            if (this.captures[1] >= 10) {
                this.winner = 2;
            }
            if (this.captures[2] >= 10) {
                this.winner = 1;
            }
        } else if (this.game === 3) {
            if (this.captures[1] >= 15) {
                this.winner = 2;
            }
            if (this.captures[2] >= 15) {
                this.winner = 1;
            }
        }
        if (this.winner) {
            return true;
        } else {
            return false;
        }
    };

    player_color = (p) => {
        if (this.game > 18 && this.game < 25) {
            p = 3 - p;
        }
        if (p === 1) {
            return 'white-stone-gradient';
        }
        return 'black-stone-gradient';
    };

    last_move = () => {
        let l; 
        l = this.until - 1;
        if (this.until === undefined) {
            l = this.moves.length - 1;
        }
        if (this.isConnect6()) {
            if (l === 0 || l%4 === 1 || l%4 === 3) {
                return [this.moves[l]];
            } else {
                return [this.moves[l-1], this.moves[l]];
            }
        } else if (this.isGo() && l>-1) {
            const move = this.moves[l];
            if (move !== this.gridSize*this.gridSize) {
                return [move];
            }
        } else if (l > -1) {
            return [this.moves[l]];
        }
        return [];
    };
    
    goForwardBack = (forward) => {
        if (this.until === undefined) {
            this.until = this.moves.length;
        } 
        if (forward) {
            if (this.isConnect6()) {
                if (this.until%4 === 1 || this.until%4 === 3) {
                    if (this.until < this.moves.length - 1) {
                        this.until += 1;
                        this.addMoveFromList(this.until - 1);
                    }
                }
                if (this.until < this.moves.length) {
                    this.until += 1;
                    this.addMoveFromList(this.until - 1);
                }
            } else {
                if (this.until < this.moves.length) {
                    this.until += 1;
                    this.addMoveFromList(this.until - 1);
                } 
            }
        } else {
            if (this.isConnect6()) {
                if (this.until > 1) {
                    if (this.until % 4 === 1 || this.until % 4 === 3) {
                        this.until -= 1;
                    }
                    this.until -= 1;
                    this.replayGame(this.until);
                }
            } else {
                if (this.until > 1) {
                    this.until -= 1;
                    this.replayGame(this.until);
                } 
            }
        }
    };
    
    goto_move = (i) => {
        if (!this.until || this.until > i) {
            this.until = i;
            this.replayGame(this.until);
        } else {
            while (this.until < i && this.until < this.moves.length) {
                this.until += 1;
                this.addMoveFromList(this.until - 1);
            }
        }
    };
    
    moves_strings = () => {
        if (this.isConnect6()) {
            const movestrs = [];
            if (this.moves.length > 0) {
                movestrs.push(this.getMoveCoord(this.moves[0]));
            }
            for (let i = 1; i < this.moves.length; i++) {
                let str = this.getMoveCoord(this.moves[i]) + ' - ';
                if (i < this.moves.length - 1) {
                    i++;
                    str += this.getMoveCoord(this.moves[i]);
                }
                movestrs.push(str);
            }
            return movestrs;
        } else if (this.isGo()) {
            const passMove = this.gridSize*this.gridSize;
            return this.moves.map(move => move === passMove?'PASS':this.getMoveCoord(move));
        } else {
            return this.moves.map(move => this.getMoveCoord(move));
        }
    };

    canNotLeave = () => {
        return this.gameState.state === GameState.State.STARTED || this.gameState.state === GameState.State.HALFSET;    
    };
    
    currentPlayer = () => {
        if (this.isConnect6()) {
            return (((this.moves.length % 4) === 0) || ((this.moves.length % 4) === 3)) ? 1 : 2;
        } else if (this.#isDPente()) {
            if (this.moves.length < 4) {
                return 1;
            } else if (this.moves.length === 4) {
                if (this.gameState.dPenteState === GameState.DPenteState.NO_CHOICE) {
                    return 2;                    
                } else {
                    return 1;
                }
            }
        } else if (this.isGo() && this.gameState.goState === GameState.GoState.MARK_STONES) {
            return this.mark_dead_stones_player;
        } else if (this.isGo() && this.gameState.goState === GameState.GoState.EVALUATE_STONES) {
            return this.evaluate_stones_player;
        } 
        return (1 + (this.moves.length % 2));
    };

    isMyTurn = () => {
        return (this.currentPlayer() === 1 && this.me === 'white') || (this.currentPlayer() === 2 && this.me === 'black');
    };

    currentColor = () => {
        if (this.isConnect6()) {
            return (((this.moves.length % 4) === 0) || ((this.moves.length % 4) === 3)) ? 1 : 2;
        }
        // const currentColor = 1 + (this.moves.length % 2);
        return 1 + (this.moves.length % 2);
        // if (this.isGo()) {
        //     return 3 - currentColor;
        // }
        // return currentColor;
    };
    // #isPoofPente = () => {
    //     return this.game === 11 || this.game === 12;    
    // };
    #isDPente = () => {
        return this.game === 7 || this.game === 8 || this.game === 17 || this.game === 18;
    };
    isConnect6 = () => {
        return this.game === 13 || this.game === 14;
    };
    isGo = () => {
        return this.game > 18 && this.game < 25;
    };
    
    dPenteChoice = () => {
        return this.#isDPente() && this.moves.length === 4 &&
            this.gameState.state === GameState.State.STARTED &&
            this.gameState.dPenteState === GameState.DPenteState.NO_CHOICE;
    };
    
    reset = () => {
        this.resetBoard();
        
        this.gameState = {
            state: GameState.State.NOT_STARTED,
            dPenteState: GameState.DPenteState.NO_CHOICE,
            goState: GameState.GoState.PLAY
        }
    };
    resetBoard = () => {
        this.captures = [undefined, 0, 0];
        this.moves = [];
        this.goGroupsByPlayerAndID = {1: {}, 2: {}};
        this.goStoneGroupIDsByPlayer = {1: {}, 2: {}};
        this.koMove = -1;
        this.suicideAllowed = false;
        this.goTerritoryByPlayer = {1: [], 2: []};
        this.hasPass = false; this.doublePass = false;
        this.goDeadStonesByPlayer = {1: [], 2: []};
        this.winner = undefined;
        this.#resetAbstractBoard();
    };
    #resetAbstractBoard = () => {
        for (let i = 0; i < 19; i++) {
            for (let j = 0; j < 19; j++) {
                this.abstractBoard[i][j] = 0;
            }
        }
    };
    
    undoMove = () => {
        const new_moves = this.moves.slice(0, this.moves.length - 1);
        if (this.until === this.moves.length) {
            this.until -= 1;
        } 
        this.moves = new_moves;
        this.replayGame();
    };
    
    replayGame = (until) => {
        if (until === undefined) {
            until = this.moves.length;
        }
        let moves = this.moves;
        this.resetBoard();
        this.moves = moves;
        if (this.game < 3) {
            this.#replayPenteGame(until);
        } else if (this.game < 5) {
            this.#replayKeryoPenteGame(until);
        } else if (this.game < 7) {
            this.#replayGomokuGame(until);
        } else if (this.game < 9) {
            let r = this.rated; this.rated = false;
            this.#replayPenteGame(until);
            this.rated = r;
        } else if (this.game < 11) {
            this.#replayGPenteGame(until);
        } else if (this.game < 13) {
            this.#replayPoofPenteGame(until);
        } else if (this.game < 15) {
            this.#replayConnect6Game(until);
        } else if (this.game < 17) {
            this.#replayPenteGame(until);
        } else if (this.game < 19) {
            let r = this.rated; this.rated = false;
            this.#replayKeryoPenteGame(until);
            this.rated = r;
        } else if (this.game < 25) {
            this.#replayGoGame(until);
        } else if (this.game < 27) {
            this.#replayOPenteGame(until);
        }
    };
    
    addMoveFromList = (i) => {
        const move = this.moves[i];
        const x = move % this.gridSize, y = Math.floor(move / this.gridSize);
        if (this.game < 3) {
            let player = 1 + (i%2);
            this.#addPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        } else if (this.game < 5) {
            let player = 1 + (i%2);
            this.#addKeryoPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        } else if (this.game < 7) {
            let player = 1 + (i%2);
            this.#addGomokuMove(x, y, player);
        } else if (this.game < 9) {
            let player = 1 + (i%2);
            this.#addPenteMove(x, y, player);
        } else if (this.game < 11) {
            let player = 1 + (i%2);
            this.#addPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyGPenteRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoGPenteRule();
            }
        } else if (this.game < 13) {
            let player = 1 + (i%2);
            this.#addPoofPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        } else if (this.game < 15) {
            let player = (((i % 4) === 0) || ((i % 4) === 3)) ? 1 : 2;
            this.#addGomokuMove(x, y, player);
        } else if (this.game < 17) {
            let player = 1 + (i%2);
            this.#addPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        } else if (this.game < 19) {
            let player = 1 + (i%2);
            this.#addKeryoPenteMove(x, y, player);
        } else if (this.game < 25) {
            let player = 1 + (i%2);
            this.#addGoMove(move, player);
        } else if (this.game < 27) {
            let player = 1 + (i%2);
            this.#addOPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        }
    };

    addMove = (move) => {
        if (this.until && this.until < this.moves.length) {
            while (this.until < this.moves.length) {
                this.until += 1;
                this.addMoveFromList(this.until - 1);
            }
        } 
        const x = move % this.gridSize, y = Math.floor(move / this.gridSize);
        this.moves.push(move);
        this.until = this.moves.length;
        if (this.game < 3) {
            let player = 2 - (this.moves.length%2);
            this.#addPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
            if (this.#detectPenteOf(player, move)) {
                this.winner = player;
            }
        } else if (this.game < 5) {
            let player = 2 - (this.moves.length%2);
            this.#addKeryoPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
            if (this.#detectPenteOf(player, move)) {
                this.winner = player;
            }
        } else if (this.game < 7) {
            let player = 2 - (this.moves.length%2);
            this.#addGomokuMove(x, y, player);
        } else if (this.game < 9) {
            let player = 2 - (this.moves.length%2);
            this.#addPenteMove(x, y, player);
        } else if (this.game < 11) {
            let player = 2 - (this.moves.length%2);
            this.#addPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyGPenteRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoGPenteRule();
            }
        } else if (this.game < 13) {
            let player = 2 - (this.moves.length%2);
            this.#addPoofPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        } else if (this.game < 15) {
            let player = (((this.moves.length % 4) === 1) || ((this.moves.length % 4) === 0)) ? 1 : 2;
            this.#addGomokuMove(x, y, player);
        } else if (this.game < 17) {
            let player = 2 - (this.moves.length%2);
            this.#addPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        } else if (this.game < 19) {
            let player = 2 - (this.moves.length%2);
            this.#addKeryoPenteMove(x, y, player);
        } else if (this.game < 25) {
            let player = 2 - (this.moves.length%2);
            this.#addGoMove(move, player);
        } else if (this.game < 27) {
            let player = 2 - (this.moves.length%2);
            this.#addOPenteMove(x, y, player);
            if (this.rated && this.moves.length === 2) {
                this.#applyTournamentRule();
            } else if (this.rated && this.moves.length === 3) {
                this.#undoTournamentRule();
            }
        }
    };

    #applyTournamentRule = () => {
        for(let i = 7; i < 12; ++i) {
            for(let j = 7; j < 12; ++j) {
                if (this.abstractBoard[i][j] === 0) {
                    this.abstractBoard[i][j] = -1;
                }
            }
        }
    };
    #undoTournamentRule = () => {
        for(let i = 7; i < 12; ++i) {
            for(let j = 7; j < 12; ++j) {
                if (this.abstractBoard[i][j] === -1) {
                    this.abstractBoard[i][j] = 0;
                }
            }
        }
    };
    #applyGPenteRule = () => {
        this.#applyTournamentRule();
        for(let i = 1; i < 3; i++) {
            if (this.abstractBoard[9][11 + i] === 0) {
                this.abstractBoard[9][11 + i] = -1;
            }
            if (this.abstractBoard[9][7 - i] === 0) {
                this.abstractBoard[9][7 - i] = -1;
            }
            if (this.abstractBoard[11 + i][9] === 0) {
                this.abstractBoard[11 + i][9] = -1;
            }
            if (this.abstractBoard[7 - i][9] === 0) {
                this.abstractBoard[7 - i][9] = -1;
            }
        }
    };
    #undoGPenteRule = () => {
        this.#undoTournamentRule();
        for(let i = 1; i < 3; i++) {
            if (this.abstractBoard[9][11 + i] === -1) {
                this.abstractBoard[9][11 + i] = 0;
            }
            if (this.abstractBoard[9][7 - i] === -1) {
                this.abstractBoard[9][7 - i] = 0;
            }
            if (this.abstractBoard[11 + i][9] === -1) {
                this.abstractBoard[11 + i][9] = 0;
            }
            if (this.abstractBoard[7 - i][9] === -1) {
                this.abstractBoard[7 - i][9] = 0;
            }
        }
    };
    #addGomokuMove = (x, y,  player) => {
        this.abstractBoard[x][y] = player;
    };
    #replayGomokuGame = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = 1 + (i%2);
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addGomokuMove(x, y, color);
        }
    };
    #addPenteMove = (x, y, player) => {
        this.#addGomokuMove(x, y, player);
        this.#detectPenteCapture(x, y, player);
    };
    #replayPenteGame = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = 1 + (i%2);
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addPenteMove(x, y, color);
        }
        if (this.rated && (this.moves.length === 2)) {
            this.#applyTournamentRule();
        }
    };
    #addKeryoPenteMove = (x, y, player) => {
        this.#addPenteMove(x, y, player);
        this.#detectKeryoPenteCapture(x, y, player);
    };
    #replayKeryoPenteGame = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = 1 + (i%2);
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addKeryoPenteMove(x, y, color);
        }
        if (this.rated && (this.moves.length === 2)) {
            this.#applyTournamentRule();
        }
    };
    #replayConnect6Game = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = (((i % 4) === 0) || ((i % 4) === 3)) ? 1 : 2;
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addGomokuMove(x, y, color);
        }
    };
    #replayGPenteGame = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = 1 + (i%2);
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addPenteMove(x, y, color);
        }
        if (this.moves.length === 2) {
            this.#applyGPenteRule();
        }
    };
    #addPoofPenteMove = (x, y, player) => {
        this.#addGomokuMove(x, y, player);
        this.#detectPoof(x, y, player);
        this.#detectPenteCapture(x, y, player);
    };
    #replayPoofPenteGame = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = 1 + (i%2);
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addPoofPenteMove(x, y, color);
        }
        if (this.rated && (this.moves.length === 2)) {
            this.#applyTournamentRule();
        }
    };
    #addOPenteMove = (x, y, player) => {
        this.#addGomokuMove(x, y, player);
        this.#detectPoof(x, y, player);
        this.#detectKeryoPoof(x, y, player);
        this.#detectPenteCapture(x, y, player);
        this.#detectKeryoPenteCapture(x, y, player);
    };
    #replayOPenteGame = (until) => {
        // this.#resetAbstractBoard();
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let color = 1 + (i%2);
            let x = this.moves[i] % 19, y = Math.floor(this.moves[i] / 19);
            this.#addOPenteMove(x, y, color);
        }
        if (this.rated && (this.moves.length === 2)) {
            this.#applyTournamentRule();
        }
    };


    #replayGoGame = (until) => {
        this.#resetAbstractBoard();
        // const passMove = this.gridSize*this.gridSize;
        this.goGroupsByPlayerAndID = {1: {}, 2: {}};
        this.goStoneGroupIDsByPlayer = {1: {}, 2: {}};
        this.goDeadStonesByPlayer = {1: [], 2: []};
        // let p1DeadStones = this.goDeadStonesByPlayer[1], p2DeadStones = this.goDeadStonesByPlayer[2];
        this.koMove = -1;
        this.hasPass = false; this.doublePass = false;
        this.gameState.goState = GameState.GoState.PLAY;
        this.captures = [undefined, 0, 0];
        for (let i = 0; i < Math.min(this.moves.length, until); i++) {
            let move = this.moves[i];
            // if (move === passMove) {
            //     if (this.hasPass) {
            //         this.doublePass = true;
            //     } else {
            //         this.hasPass = true;
            //     }
            // } else {
            //     this.hasPass = false;
            // }
            let player = 1 + (i%2);
            // this.abstractBoard[move % this.gridSize][Math.floor(move / this.gridSize)] = player;
            
            this.#addGoMove(move, player);
            // if (move !== passMove && !this.doublePass) {
            //     let player = 1 + (i%2);
            //     this.abstractBoard[move % this.gridSize][Math.floor(move / this.gridSize)] = player;
            //     this.#addGoMove(move, 3-player);
            // } else if (this.doublePass && move !== passMove) {
            //     let pos = this.getPosition(move);
            //     if (pos === 1) {
            //         p1DeadStones.push(move);
            //     } else if (pos === 2) {
            //         p2DeadStones.push(move);
            //     }
            //     this.#setPosition(move, 0);
            // }
        }
    };

    #find_dead_stones_player = () => {
        let pass = false;
        const pass_move = this.gridSize*this.gridSize;
        // console.log('moves = ', JSON.stringify(this.moves))
        for (let i = 0; i < this.moves.length; i++) {
            if (this.moves[i] === pass_move) {
                if (pass) {
                    this.mark_dead_stones_player = 1 + (i+1)%2;
                    break;
                } else {
                    pass = true;
                }
            } else {
                pass = false;
            }
        }    
    };

    #addGoMove = (move, currentPlayer) => {
        if (move === this.gridSize*this.gridSize) {
            if (this.gameState.goState === GameState.GoState.MARK_STONES) {
                this.gameState.goState = GameState.GoState.EVALUATE_STONES;
                this.evaluate_stones_player = 3 - this.mark_dead_stones_player;
            } else if (this.gameState.goState === GameState.GoState.PLAY) {
                if (this.hasPass) {
                    this.doublePass = true;
                    this.gameState.goState = GameState.GoState.MARK_STONES;
                    this.#getTerritories();
                    this.#find_dead_stones_player();
                    // console.log('dead player = ', this.mark_dead_stones_player)
                } else {
                    this.hasPass = true;
                }
            }
        } else {
            this.hasPass = false;
            this.doublePass = false;
        }
        if (move >= this.gridSize*this.gridSize) {
            return;
        }
        // if (this.doublePass) {
        if (this.gameState.goState === GameState.GoState.MARK_STONES) {
            let pos = this.getPosition(move);
            this.goDeadStonesByPlayer[pos].push(move);
            this.#setPosition(move, 0);
            this.#getTerritories();
            return;
        }
        this.#setPosition(move, currentPlayer);

        let opponent = 3 - currentPlayer;

        let groupsByID = this.goGroupsByPlayerAndID[currentPlayer];
        let stoneGroupIDs = this.goStoneGroupIDsByPlayer[currentPlayer];

        this.#settleGroups(move, groupsByID, stoneGroupIDs);
        groupsByID = this.goGroupsByPlayerAndID[opponent];
        stoneGroupIDs = this.goStoneGroupIDsByPlayer[opponent];
        this.#makeCaptures(move, groupsByID, stoneGroupIDs, opponent);

        if (this.suicideAllowed === true) {
            groupsByID = this.goGroupsByPlayerAndID[currentPlayer];
            stoneGroupIDs = this.goStoneGroupIDsByPlayer[currentPlayer];
            let moveGroupID = stoneGroupIDs[move];
            let moveGroup = groupsByID[moveGroupID];
            if (!this.#groupHasLiberties(moveGroup)) {
                this.captures[currentPlayer] += moveGroup.size();
                // if (currentPlayer !== 1) {
                //     whiteCaptures += moveGroup.size();
                // } else {
                //     blackCaptures += moveGroup.size();
                // }
                this.#captureGroup(moveGroupID, groupsByID, stoneGroupIDs);
            }
        }
    };

    #makeCaptures = (move, groupsByID, stoneGroupIDs, colorToCapture) => {
        let captures = 0;
        if (move%this.gridSize !== 0) {
            let neighborStone = move - 1;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            captures = this.#getCaptures(move, groupsByID, stoneGroupIDs, captures, neighborStone, neighborStoneGroupID);
        }
        if (move%this.gridSize !== this.gridSize - 1) {
            let neighborStone = move + 1;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            captures = this.#getCaptures(move, groupsByID, stoneGroupIDs, captures, neighborStone, neighborStoneGroupID);
        }
        if (Math.floor(move/this.gridSize) !== 0) {
            let neighborStone = move - this.gridSize;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            captures = this.#getCaptures(move, groupsByID, stoneGroupIDs, captures, neighborStone, neighborStoneGroupID);
        }
        if (Math.floor(move/this.gridSize) !== this.gridSize - 1) {
            let neighborStone = move + this.gridSize;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            captures = this.#getCaptures(move, groupsByID, stoneGroupIDs, captures, neighborStone, neighborStoneGroupID);
        }
        if (captures !== 1) {
            this.koMove = -1;
        }
        this.captures[colorToCapture] += captures;
        // console.log(captures);
        // if (colorToCapture === 1) {
        //     blackCaptures += captures;
        // } else {
        //     whiteCaptures += captures;
        // }
    };

    #getCaptures = (move, groupsByID, stoneGroupIDs, captures, neighborStone, neighborStoneGroupID) => {
        if (neighborStoneGroupID === undefined) {
            return captures;
        }
        let newCaptures = captures;
        let neighborStoneGroup = groupsByID[neighborStoneGroupID];
        // console.log(getMoveCoord(move));
        // console.log(groupsByID);
        if (neighborStoneGroup !== undefined) {
            if (this.#groupHasLiberties(neighborStoneGroup) === false) {
                if (this.koMove < 0 && neighborStoneGroup.length === 1 && this.#checkKo(move)) {
                    this.koMove = neighborStone;
                } else {
                    this.koMove = -1;
                }
                newCaptures = captures + neighborStoneGroup.length;
                this.#captureGroup(neighborStoneGroupID, groupsByID, stoneGroupIDs);
            }
        }
        return newCaptures;
    };
    #checkKo = (move) => {
        let position = this.getPosition(move);
        if (move%this.gridSize !== 0) {
            let neighborStone = move - 1;
            let neighborPosition = this.getPosition(neighborStone);
            if (position !== 3 - neighborPosition) {
                return false;
            }
        }
        if (move%this.gridSize !== this.gridSize - 1) {
            let neighborStone = move + 1;
            let neighborPosition = this.getPosition(neighborStone);
            if (position !== 3 - neighborPosition) {
                return false;
            }
        }
        if (Math.floor(move/this.gridSize) !== 0) {
            let neighborStone = move - this.gridSize;
            let neighborPosition = this.getPosition(neighborStone);
            if (position !== 3 - neighborPosition) {
                return false;
            }
        }
        if (Math.floor(move/this.gridSize) !== this.gridSize - 1) {
            let neighborStone = move + this.gridSize;
            let neighborPosition = this.getPosition(neighborStone);
            if (position !== 3 - neighborPosition) {
                return false;
            }
        }
        return true;
    };
    #captureGroup = (groupID, groupsByID, stoneGroupIDs) => {
        let group = groupsByID[groupID];
        for(let i = 0; i < group.length; ++i) {
            this.#setPosition(group[i], 0);
            delete stoneGroupIDs[group[i]];
        }
        delete groupsByID[groupID];
    };
    #groupHasLiberties = (group) => {
        for (let i = 0; i < group.length; i++) {
            if (this.#stoneHasLiberties(group[i]) === true) {
                return true;
            }
        }
        return false;
    };

    #stoneHasLiberties = (stone) => {
        if (stone%this.gridSize !== 0) {
            let neighborStone = stone - 1;
            let position = this.getPosition(neighborStone);
            if (position !== 1 && position !== 2) {
                return true;
            }
        }
        if (stone%this.gridSize !== this.gridSize - 1) {
            let neighborStone = stone + 1;
            let position = this.getPosition(neighborStone);
            if (position !== 1 && position !== 2) {
                return true;
            }
        }
        if (Math.floor(stone/this.gridSize) !== 0) {
            let neighborStone = stone - this.gridSize;
            let position = this.getPosition(neighborStone, true);
            if (position !== 1 && position !== 2) {
                return true;
            }
        }
        if (Math.floor(stone/this.gridSize) !== this.gridSize - 1) {
            let neighborStone = stone + this.gridSize;
            let position = this.getPosition(neighborStone);
            if (position !== 1 && position !== 2) {
                return true;
            }
        }
        return false;
    };

    getMoveCoord = (move) => {
        let letter = coordinateLetters[move%this.gridSize];
        let number = this.gridSize-Math.floor(move/this.gridSize);
        return letter + number;
    };
    getPosition = (move) => {
        return this.abstractBoard[move%this.gridSize][Math.floor(move/this.gridSize)];
    };
    #setPosition = (move, val) => {
        this.abstractBoard[move%this.gridSize][Math.floor(move/this.gridSize)] = val;
    };

    #settleGroups = (move, groupsByID, stoneGroupIDs) => {
        let newGroup = [];
        newGroup.push(move);
        groupsByID[move] = newGroup;
        stoneGroupIDs[move] = move;

        if (move%this.gridSize !== 0) {
            let neighborStone = move - 1;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            if (neighborStoneGroupID !== undefined) {
                this.#mergeGroups(move, neighborStoneGroupID, groupsByID, stoneGroupIDs);
            }
        }
        if (move%this.gridSize !== this.gridSize - 1) {
            let neighborStone = move + 1;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            if (neighborStoneGroupID !== undefined) {
                this.#mergeGroups(stoneGroupIDs[move], neighborStoneGroupID, groupsByID, stoneGroupIDs);
            }
        }
        if (Math.floor(move/this.gridSize) !== 0) {
            let neighborStone = move - this.gridSize;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            if (neighborStoneGroupID !== undefined) {
                this.#mergeGroups(stoneGroupIDs[move], neighborStoneGroupID, groupsByID, stoneGroupIDs);
            }
        }
        if (Math.floor(move/this.gridSize) !== this.gridSize - 1) {
            let neighborStone = move + this.gridSize;
            let neighborStoneGroupID = stoneGroupIDs[neighborStone];
            if (neighborStoneGroupID !== undefined) {
                this.#mergeGroups(stoneGroupIDs[move], neighborStoneGroupID, groupsByID, stoneGroupIDs);
            }
        }
    };
    #mergeGroups = (group1, group2, groupsByID, stoneGroupIDs) => {
        if (group1 === group2) {
            return;
        }
        let oldGroup, newGroup;
        let oldGroupID, newGroupID;
        if (group1 < group2) {
            oldGroup = groupsByID[group1];
            newGroup = groupsByID[group2];
            oldGroupID = group1;
            newGroupID = group2;
        } else {
            oldGroup = groupsByID[group2];
            newGroup = groupsByID[group1];
            oldGroupID = group2;
            newGroupID = group1;
        }
        for(let i = 0; i < oldGroup.length; ++i) {
            newGroup.push(oldGroup[i]);
            stoneGroupIDs[oldGroup[i]] = newGroupID;
        }
        delete groupsByID[oldGroupID];
    };
    #getEmptyNeighbour = (move) => {
        if (move%this.gridSize !== 0) {
            let neighborStone = move - 1;
            if (this.getPosition(neighborStone) === 0) {
                return neighborStone;
            }
        }
        if (move%this.gridSize !== this.gridSize - 1) {
            let neighborStone = move + 1;
            if (this.getPosition(neighborStone) === 0) {
                return neighborStone;
            }
        }
        if (Math.floor(move/this.gridSize) !== 0) {
            let neighborStone = move - this.gridSize;
            if (this.getPosition(neighborStone) === 0) {
                return neighborStone;
            }
        }
        if (Math.floor(move/this.gridSize) !== this.gridSize - 1) {
            let neighborStone = move + this.gridSize;
            if (this.getPosition(neighborStone) === 0) {
                return neighborStone;
            }
        }
        return -1;
    };
    #floodFillWorker = (move, value) => {
        this.#setPosition(move, value);
        let neighbourStone = this.#getEmptyNeighbour(move);
        while (neighbourStone !== -1) {
            this.#floodFillWorker(neighbourStone, value);
            neighbourStone = this.#getEmptyNeighbour(move);
        }
    };
    #resetGoBeforeFlood = () => {
        for (let i = 0; i < this.gridSize; i++ ) {
            for (let j = 0; j < this.gridSize; j++ ) {
                if (this.abstractBoard[i][j] !== 1 && this.abstractBoard[i][j] !== 2) {
                    this.abstractBoard[i][j] = 0;
                }
            }
        }
    };
    #floodPlayer = (player) => {
        for (let move = 0; move < this.gridSize*this.gridSize; move++) {
            if (this.getPosition(move) === player) {
                let neighbourStone = this.#getEmptyNeighbour(move);
                while (neighbourStone > -1) {
                    this.#floodFillWorker(neighbourStone, player + 2);
                    neighbourStone = this.#getEmptyNeighbour(move);
                }
            }
        }
    };
    #getMovesForValue = (value) => {
        let result = [];
        for (let j = 0; j < this.gridSize; j++ ) {
            for (let i = 0; i < this.gridSize; i++ ) {
                if (this.abstractBoard[i][j] === value) {
                    result.push(j*this.gridSize+i);
                }
            }
        }
        return result;
    };
    #getTerritories = () => {
        this.goTerritoryByPlayer = {1: [], 2: []};
        this.#resetGoBeforeFlood();
        this.#floodPlayer(1);
        let p1Territory = this.#getMovesForValue(3);
        this.#resetGoBeforeFlood();
        this.#floodPlayer(2);
        let p2Territory = this.#getMovesForValue(4);
        this.#resetGoBeforeFlood();
        let i = 0, j = 0;
        while (i < p1Territory.length && j < p2Territory.length) {
            let p1Stone = p1Territory[i], p2Stone = p2Territory[j];
            if (p1Stone === p2Stone) {
                p1Territory.splice(i, 1);
                p2Territory.splice(j, 1);
            } else {
                if (p1Stone < p2Stone) {
                    i += 1;
                } else {
                    j += 1;
                }
            }
        }
        this.goTerritoryByPlayer[1] = p1Territory;
        this.goTerritoryByPlayer[2] = p2Territory;
    };
    
    goScores = () => {
        this.#getTerritories();
        let score = {1: 0, 2: 0};
        for (let i = 0; i < this.gridSize; i++) {
            for (let j = 0; j < this.gridSize; j++) {
                const pos = this.abstractBoard[i][j];
                if (pos > 0) {
                    score[pos] += 1;
                }
            }
        }
        for (let i = 1; i < 3; i++) {
            score[i] += this.goTerritoryByPlayer[i].length;
        }
        return score;
    };

    evaluateScore = () => {
        return this.isGo() && this.gameState.state === GameState.State.STARTED 
            && this.gameState.goState === GameState.GoState.EVALUATE_STONES;
    };

    rejectGoState = () => {
        let pass = false;
        const pass_move = this.gridSize*this.gridSize;
        // console.log('moves = ', JSON.stringify(this.moves))
        for (let i = 0; i < this.moves.length; i++) {
            if (this.moves[i] === pass_move) {
                if (pass) {
                    this.moves = this.moves.slice(0, i-1);
                    // const newMoves = this.moves.slice(0, i-1);
                    // this.moves = newMoves;
                    this.replayGame();
                    break;
                } else {
                    pass = true;
                }
            } else {
                pass = false;
            }
        }

    };

    #detectPenteCapture = (i, j, myColor) => {
        let opponentColor = 1 + (myColor % 2);
        if ((i-3) > -1) {
            if (this.abstractBoard[i-3][j] === myColor) {
                if ((this.abstractBoard[i-1][j] === opponentColor) && (this.abstractBoard[i-2][j] === opponentColor)) {
                    this.abstractBoard[i-1][j] = 0;
                    this.abstractBoard[i-2][j] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if (((i-3) > -1) && ((j-3) > -1)) {
            if (this.abstractBoard[i-3][j-3] === myColor) {
                if ((this.abstractBoard[i-1][j-1] === opponentColor) && (this.abstractBoard[i-2][j-2] === opponentColor)) {
                    this.abstractBoard[i-1][j-1] = 0;
                    this.abstractBoard[i-2][j-2] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if ((j-3) > -1) {
            if (this.abstractBoard[i][j-3] === myColor) {
                if ((this.abstractBoard[i][j-1] === opponentColor) && (this.abstractBoard[i][j-2] === opponentColor)) {
                    this.abstractBoard[i][j-1] = 0;
                    this.abstractBoard[i][j-2] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if (((i+3) < 19) && ((j-3) > -1)) {
            if (this.abstractBoard[i+3][j-3] === myColor) {
                if ((this.abstractBoard[i+1][j-1] === opponentColor) && (this.abstractBoard[i+2][j-2] === opponentColor)) {
                    this.abstractBoard[i+1][j-1] = 0;
                    this.abstractBoard[i+2][j-2] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if ((i+3) < 19) {
            if (this.abstractBoard[i+3][j] === myColor) {
                if ((this.abstractBoard[i+1][j] === opponentColor) && (this.abstractBoard[i+2][j] === opponentColor)) {
                    this.abstractBoard[i+1][j] = 0;
                    this.abstractBoard[i+2][j] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if (((i+3) < 19) && ((j+3) < 19)) {
            if (this.abstractBoard[i+3][j+3] === myColor) {
                if ((this.abstractBoard[i+1][j+1] === opponentColor) && (this.abstractBoard[i+2][j+2] === opponentColor)) {
                    this.abstractBoard[i+1][j+1] = 0;
                    this.abstractBoard[i+2][j+2] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if ((j+3) < 19) {
            if (this.abstractBoard[i][j+3] === myColor) {
                if ((this.abstractBoard[i][j+1] === opponentColor) && (this.abstractBoard[i][j+2] === opponentColor)) {
                    this.abstractBoard[i][j+1] = 0;
                    this.abstractBoard[i][j+2] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
        if (((i-3) > -1) && ((j+3) < 19)) {
            if (this.abstractBoard[i-3][j+3] === myColor) {
                if ((this.abstractBoard[i-1][j+1] === opponentColor) && (this.abstractBoard[i-2][j+2] === opponentColor)) {
                    this.abstractBoard[i-1][j+1] = 0;
                    this.abstractBoard[i-2][j+2] = 0;
                    this.captures[opponentColor] += 2;
                }
            }
        }
    };
    #detectKeryoPenteCapture = (i, j, myColor) => {
        let opponentColor = 1 + (myColor % 2);
        if ((i-4) > -1) {
            if (this.abstractBoard[i-4][j] === myColor) {
                if ((this.abstractBoard[i-1][j] === opponentColor) && (this.abstractBoard[i-2][j] === opponentColor) && (this.abstractBoard[i-3][j] === opponentColor)) {
                    this.abstractBoard[i-1][j] = 0;
                    this.abstractBoard[i-2][j] = 0;
                    this.abstractBoard[i-3][j] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if (((i-4) > -1) && ((j-4) > -1)) {
            if (this.abstractBoard[i-4][j-4] === myColor) {
                if ((this.abstractBoard[i-1][j-1] === opponentColor) && (this.abstractBoard[i-2][j-2] === opponentColor) && (this.abstractBoard[i-3][j-3] === opponentColor)) {
                    this.abstractBoard[i-1][j-1] = 0;
                    this.abstractBoard[i-2][j-2] = 0;
                    this.abstractBoard[i-3][j-3] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if ((j-4) > -1) {
            if (this.abstractBoard[i][j-4] === myColor) {
                if ((this.abstractBoard[i][j-1] === opponentColor) && (this.abstractBoard[i][j-2] === opponentColor) && (this.abstractBoard[i][j-3] === opponentColor)) {
                    this.abstractBoard[i][j-1] = 0;
                    this.abstractBoard[i][j-2] = 0;
                    this.abstractBoard[i][j-3] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if (((i+4) < 19) && ((j-4) > -1)) {
            if (this.abstractBoard[i+4][j-4] === myColor) {
                if ((this.abstractBoard[i+1][j-1] === opponentColor) && (this.abstractBoard[i+2][j-2] === opponentColor) && (this.abstractBoard[i+3][j-3] === opponentColor)) {
                    this.abstractBoard[i+1][j-1] = 0;
                    this.abstractBoard[i+2][j-2] = 0;
                    this.abstractBoard[i+3][j-3] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if ((i+4) < 19) {
            if (this.abstractBoard[i+4][j] === myColor) {
                if ((this.abstractBoard[i+1][j] === opponentColor) && (this.abstractBoard[i+2][j] === opponentColor) && (this.abstractBoard[i+3][j] === opponentColor)) {
                    this.abstractBoard[i+1][j] = 0;
                    this.abstractBoard[i+2][j] = 0;
                    this.abstractBoard[i+3][j] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if (((i+4) < 19) && ((j+4) < 19)) {
            if (this.abstractBoard[i+4][j+4] === myColor) {
                if ((this.abstractBoard[i+1][j+1] === opponentColor) && (this.abstractBoard[i+2][j+2] === opponentColor) && (this.abstractBoard[i+3][j+3] === opponentColor)) {
                    this.abstractBoard[i+1][j+1] = 0;
                    this.abstractBoard[i+2][j+2] = 0;
                    this.abstractBoard[i+3][j+3] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if ((j+4) < 19) {
            if (this.abstractBoard[i][j+4] === myColor) {
                if ((this.abstractBoard[i][j+1] === opponentColor) && (this.abstractBoard[i][j+2] === opponentColor) && (this.abstractBoard[i][j+3] === opponentColor)) {
                    this.abstractBoard[i][j+1] = 0;
                    this.abstractBoard[i][j+2] = 0;
                    this.abstractBoard[i][j+3] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
        if (((i-4) > -1) && ((j+4) < 19)) {
            if (this.abstractBoard[i-4][j+4] === myColor) {
                if ((this.abstractBoard[i-1][j+1] === opponentColor) && (this.abstractBoard[i-2][j+2] === opponentColor) && (this.abstractBoard[i-3][j+3] === opponentColor)) {
                    this.abstractBoard[i-1][j+1] = 0;
                    this.abstractBoard[i-2][j+2] = 0;
                    this.abstractBoard[i-3][j+3] = 0;
                    this.captures[opponentColor] += 3;
                }
            }
        }
    };
    #detectPoof = (i, j, myColor) => {
        let opponentColor = 1 + (myColor % 2);
        let poofed = false;
        if (((i-2) > -1) && ((i+1) < 19)) {
            if (this.abstractBoard[i-1][j] === myColor) {
                if ((this.abstractBoard[i-2][j] === opponentColor) && (this.abstractBoard[i+1][j] === opponentColor)) {
                    this.abstractBoard[i-1][j] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((i-2) > -1) && ((j-2) > -1) && ((i+1) < 19) && ((j+1) < 19)) {
            if (this.abstractBoard[i-1][j-1] === myColor) {
                if ((this.abstractBoard[i-2][j-2] === opponentColor) && (this.abstractBoard[i+1][j+1] === opponentColor)) {
                    this.abstractBoard[i-1][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((j-2) > -1) && ((j+1) < 19)) {
            if (this.abstractBoard[i][j-1] === myColor) {
                if ((this.abstractBoard[i][j-2] === opponentColor) && (this.abstractBoard[i][j+1] === opponentColor)) {
                    this.abstractBoard[i][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((i-1) > -1) && ((j-2) > -1) && ((i+2) < 19) && ((j+1) < 19)) {
            if (this.abstractBoard[i+1][j-1] === myColor) {
                if ((this.abstractBoard[i-1][j+1] === opponentColor) && (this.abstractBoard[i+2][j-2] === opponentColor)) {
                    this.abstractBoard[i+1][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((i+2) < 19) && ((i-1) > -1)) {
            if (this.abstractBoard[i+1][j] === myColor) {
                if ((this.abstractBoard[i+2][j] === opponentColor) && (this.abstractBoard[i-1][j] === opponentColor)) {
                    this.abstractBoard[i+1][j] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((i-1) > -1) && ((j-1) > -1) && ((i+2) < 19) && ((j+2) < 19)) {
            if (this.abstractBoard[i+1][j+1] === myColor) {
                if ((this.abstractBoard[i-1][j-1] === opponentColor) && (this.abstractBoard[i+2][j+2] === opponentColor)) {
                    this.abstractBoard[i+1][j+1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((j+2) < 19) && ((j-1) > -1)) {
            if (this.abstractBoard[i][j+1] === myColor) {
                if ((this.abstractBoard[i][j-1] === opponentColor) && (this.abstractBoard[i][j+2] === opponentColor)) {
                    this.abstractBoard[i][j+1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (((i-2) > -1) && ((j-1) > -1) && ((i+1) < 19) && ((j+2) < 19)) {
            if (this.abstractBoard[i-1][j+1] === myColor) {
                if ((this.abstractBoard[i+1][j-1] === opponentColor) && (this.abstractBoard[i-2][j+2] === opponentColor)) {
                    this.abstractBoard[i-1][j+1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 1;
                    poofed = true;
                }
            }
        }
        if (poofed) {
            this.captures[myColor] += 1;
        }
    };

    #detectKeryoPoof = (i, j, myColor) => {
        let opponentColor = 1 + (myColor % 2);
        let poofed = false;
        if (((i-3) > -1) && ((i+1) < 19)) { // left
            if (this.abstractBoard[i-1][j] === myColor && this.abstractBoard[i-2][j] === myColor) {
                if ((this.abstractBoard[i-3][j] === opponentColor) && (this.abstractBoard[i+1][j] === opponentColor)) {
                    this.abstractBoard[i-2][j] = 0;
                    this.abstractBoard[i-1][j] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i-3) > -1) && ((j-3) > -1) && ((i+1) < 19) && ((j+1) < 19)) { // up left
            if (this.abstractBoard[i-1][j-1] === myColor && this.abstractBoard[i-2][j-2] === myColor) {
                if ((this.abstractBoard[i-3][j-3] === opponentColor) && (this.abstractBoard[i+1][j+1] === opponentColor)) {
                    this.abstractBoard[i-2][j-2] = 0;
                    this.abstractBoard[i-1][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((j-3) > -1) && ((j+1) < 19)) { // up
            if (this.abstractBoard[i][j-1] === myColor && this.abstractBoard[i][j-2] === myColor) {
                if ((this.abstractBoard[i][j-3] === opponentColor) && (this.abstractBoard[i][j+1] === opponentColor)) {
                    this.abstractBoard[i][j-2] = 0;
                    this.abstractBoard[i][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i-1) > -1) && ((j-3) > -1) && ((i+3) < 19) && ((j+1) < 19)) { // up right
            if (this.abstractBoard[i+1][j-1] === myColor && this.abstractBoard[i+2][j-2] === myColor) {
                if ((this.abstractBoard[i-1][j+1] === opponentColor) && (this.abstractBoard[i+3][j-3] === opponentColor)) {
                    this.abstractBoard[i+2][j-2] = 0;
                    this.abstractBoard[i+1][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i+3) < 19) && ((i-1) > -1)) { // right
            if (this.abstractBoard[i+1][j] === myColor && this.abstractBoard[i+2][j] === myColor) {
                if ((this.abstractBoard[i+3][j] === opponentColor) && (this.abstractBoard[i-1][j] === opponentColor)) {
                    this.abstractBoard[i+2][j] = 0;
                    this.abstractBoard[i+1][j] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i-1) > -1) && ((j-1) > -1) && ((i+3) < 19) && ((j+3) < 19)) { // down right
            if (this.abstractBoard[i+1][j+1] === myColor && this.abstractBoard[i+2][j+2] === myColor) {
                if ((this.abstractBoard[i-1][j-1] === opponentColor) && (this.abstractBoard[i+3][j+3] === opponentColor)) {
                    this.abstractBoard[i+2][j+2] = 0;
                    this.abstractBoard[i+1][j+1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((j+2) < 19) && ((j-1) > -1)) { // down
            if (this.abstractBoard[i][j+1] === myColor && this.abstractBoard[i][j+2] === myColor) {
                if ((this.abstractBoard[i][j-1] === opponentColor) && (this.abstractBoard[i][j+3] === opponentColor)) {
                    this.abstractBoard[i][j+1] = 0;
                    this.abstractBoard[i][j+2] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i-3) > -1) && ((j-1) > -1) && ((i+1) < 19) && ((j+3) < 19)) { // down left
            if (this.abstractBoard[i-1][j+1] === myColor && this.abstractBoard[i-2][j+2] === myColor) {
                if ((this.abstractBoard[i+1][j-1] === opponentColor) && (this.abstractBoard[i-3][j+3] === opponentColor)) {
                    this.abstractBoard[i-2][j+2] = 0;
                    this.abstractBoard[i-1][j+1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }

        // 4 directions with center of 3 stones placed to poof
        if (((i-2) > -1) && ((i+2) < 19)) { // horizontal
            if (this.abstractBoard[i-1][j] === myColor && this.abstractBoard[i+1][j] === myColor) {
                if ((this.abstractBoard[i-2][j] === opponentColor) && (this.abstractBoard[i+2][j] === opponentColor)) {
                    this.abstractBoard[i+1][j] = 0;
                    this.abstractBoard[i-1][j] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i-2) > -1) && ((j-2) > -1) && ((i+2) < 19) && ((j+2) < 19)) { // up left
            if (this.abstractBoard[i-1][j-1] === myColor && this.abstractBoard[i+1][j+1] === myColor) {
                if ((this.abstractBoard[i-2][j-2] === opponentColor) && (this.abstractBoard[i+2][j+2] === opponentColor)) {
                    this.abstractBoard[i+1][j+1] = 0;
                    this.abstractBoard[i-1][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((j-2) > -1) && ((j+2) < 19)) { // vertical
            if (this.abstractBoard[i][j-1] === myColor && this.abstractBoard[i][j+1] === myColor) {
                if ((this.abstractBoard[i][j-2] === opponentColor) && (this.abstractBoard[i][j+2] === opponentColor)) {
                    this.abstractBoard[i][j+1] = 0;
                    this.abstractBoard[i][j-1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }
        if (((i-2) > -1) && ((j-2) > -1) && ((i+2) < 19) && ((j+2) < 19)) { // up right
            if (this.abstractBoard[i+1][j-1] === myColor && this.abstractBoard[i+1][j-1] === myColor) {
                if ((this.abstractBoard[i-2][j+2] === opponentColor) && (this.abstractBoard[i+2][j-2] === opponentColor)) {
                    this.abstractBoard[i+1][j-1] = 0;
                    this.abstractBoard[i-1][j+1] = 0;
                    this.abstractBoard[i][j] = 0;
                    this.captures[myColor] += 2;
                    poofed = true;
                }
            }
        }

        if (poofed) {
            this.captures[myColor] += 1;
        }

    };

    #detectPenteOf = (color, rowCol) => {
        let pente = false;
        let penteCounter = 1;
        let col = Math.floor(rowCol / 19), row = rowCol % 19, i, j;
        i = row - 1;
        j = col;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
        if (color === this.abstractBoard[i][j]) {
            penteCounter += 1;
            pente = (penteCounter > 4);
        } else {
            break;
        }
        i -= 1;
        }
        i = row + 1;
        j = col;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            i += 1;
        }
        if (pente) {
            return pente;
        }
        penteCounter = 1;
        i = row;
        j = col - 1;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            j -= 1;
        }
        i = row;
        j = col + 1;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            j += 1;
        }
        if (pente) {
            return pente;
        }
        penteCounter = 1;
        i = row - 1;
        j = col - 1;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            j -= 1;
            i -= 1;
        }
        i = row + 1;
        j = col + 1;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            i += 1;
            j += 1;
        }
        if (pente) {
            return pente;
        }
        penteCounter = 1;
        i = row - 1;
        j = col + 1;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            j += 1;
            i -= 1;
        }
        i = row + 1;
        j = col - 1;
        while (i > 0 && i < 19 && j > 0 && j < 19 && !pente) {
            if (color === this.abstractBoard[i][j]) {
                penteCounter += 1;
                pente = (penteCounter > 4);
            } else {
                break;
            }
            i += 1;
            j -= 1;
        }
        
        return pente;
    }



}

// export default Game;