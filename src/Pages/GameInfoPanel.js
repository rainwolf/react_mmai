import React from 'react';
import { connect } from 'react-redux';
import { START_GAME, CHANGE_GAME, CHANGE_LEVEL, 
    UNDO_MOVE, CHANGE_COLOR, OPENING_BOOK } from "../redux_actions/actionTypes";
import Grid from '@mui/material/Grid';
import ListItem from '@mui/material/ListItem';
import MenuItem from '@mui/material/MenuItem';
import OutlinedInput from '@mui/material/OutlinedInput';
import Select from '@mui/material/Select';
import Captures from './Captures';
import { withStyles } from '@mui/styles';
import Paper from '@mui/material/Paper';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import MovesListPanel from './MovesListPanel';

const styles = theme => ({
    root: {
        flexGrow: 1,
    },
    paper: {
        textAlign: 'center',
    },
});

const mapStateToProps = state => {
    return {
        pressed_play: state.pressed_play,
        game: state.game,
        level: state.level,
        color: state.game.me,
        use_book: state.opening_book,
        thinking: state.thinking
    }
};

const mapDispatchToProps = dispatch => {
    return {
        play_pressed: () => {
            dispatch({ type: START_GAME })
        },
        change_game: () => {
            dispatch({ type: CHANGE_GAME })
        },
        change_level: (event) => {
            dispatch({ type: CHANGE_LEVEL, payload: event.target.value })
        },
        undo_move: () => {
            dispatch({ type: UNDO_MOVE })
        },
        change_color: () => {
            dispatch({ type: CHANGE_COLOR })
        },
        toggle_openingbook: () => {
            dispatch({ type: OPENING_BOOK })
        },
    }
};





const UnconnectedGameInfoPanel = (props) => {
    const { game, play_pressed, change_game, change_level, 
        level, undo_move, change_color, color, 
        toggle_openingbook, use_book, thinking } = props;
    
    return (
        <div style={{width: '100%', height: '100%'}}>
            <Grid container direction={'column'} alignItems={'stretch'} wrap={'nowrap'} 
                  style={{width: '100%', height: '100%'}}>
                <Grid item>
                    <Paper style={{textAlign: 'center'}}>
                        <Typography variant="h3" color={thinking?'error':'textPrimary'}>
                            {game.game_name()}
                        </Typography>
                    </Paper>
                    <hr/>
                </Grid>
                {game.gameHasCaptures() &&
                <Grid item>
                    <Grid container direction={'row'} alignItems={'stretch'} wrap={'nowrap'}
                          style={{width: '100%', height: '100%'}}>
                        <Grid item xs style={{paddingLeft: 10}}>
                            <Typography variant='h4'>
                                Captures:
                            </Typography>
                        </Grid>
                        <Grid item xs>
                            <Captures seat={1}/>
                        </Grid>
                        <Grid item xs>
                            <Captures seat={2}/>
                        </Grid>
                    </Grid>
                </Grid>
                }
                <Grid item>
                    <Grid container direction={'row'} alignItems={'stretch'} wrap={'nowrap'}
                          style={{width: '100%', height: '100%'}}>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary" onClick={play_pressed} className={'button-glow'}>
                                    play
                                </Button>
                            </div>
                        </Grid>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <ListItem key='game' >
                                    <Select
                                        onChange={change_level}
                                        value={level}
                                        input={
                                            <OutlinedInput
                                                // labelWidth={0}
                                                name="game"
                                                id="outlined-age-simple"
                                            />
                                        }
                                    >
                                        {/*{[1,3,5,7,9,11,13,15,17,19,21,23].map(game =>*/}
                                        {/*<MenuItem key={game} value={game}>{table.game_name(game)}</MenuItem>*/}
                                        {/*)}*/}
                                        {Array.from({length: 12}, (v, i) => i+1).map(lvl =>
                                            <MenuItem key={lvl} value={lvl}>Level {lvl}</MenuItem>
                                        )}
                                    </Select>
                                </ListItem>
                            </div>
                        </Grid>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary" onClick={undo_move}>
                                    Undo
                                </Button>
                            </div>
                        </Grid>
                    </Grid>
                </Grid>
                <Grid item>
                    <Grid container direction={'row'} alignItems={'stretch'} wrap={'nowrap'}
                          style={{width: '100%', height: '100%'}}>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary" onClick={change_game}>
                                    change game
                                </Button>
                            </div>
                        </Grid>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary" onClick={change_color}>
                                    play as {color}
                                </Button>
                            </div>
                        </Grid>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary" onClick={toggle_openingbook}>
                                    {use_book === 1?'use':'no'} opening book
                                </Button>
                            </div>
                        </Grid>
                    </Grid>
                </Grid>
                <Grid item style={{flex: '1 1 auto', minHeight: '0px'}}>
                    <MovesListPanel />
                </Grid>    
            </Grid>
        </div>
    );
};


const GameInfoPanel = connect(mapStateToProps, mapDispatchToProps)(withStyles(styles)(UnconnectedGameInfoPanel))

export default GameInfoPanel;