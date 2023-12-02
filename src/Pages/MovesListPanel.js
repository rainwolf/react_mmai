import React, { useEffect } from 'react';
import { MOVE_FORWARD, MOVE_BACK, MOVE_GOTO } from "../redux_actions/actionTypes";
import { connect } from 'react-redux';
import { Game } from "../Classes/GameClass";
import PropTypes from 'prop-types';
import { withStyles } from '@mui/styles';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableRow from '@mui/material/TableRow';
import Grid from '@mui/material/Grid';
import Button from '@mui/material/Button';
import Paper from '@mui/material/Paper';
import Typography from '@mui/material/Typography';
import SkipNextIcon from '@mui/icons-material/SkipNext';
import SkipPreviousIcon from '@mui/icons-material/SkipPrevious';

const styles = theme => ({
    root: {
        width: '100%',
        // marginTop: theme.spacing.unit * 3,
        alignItems: 'center',
        height: '100%',
        overflow: 'auto'
    },
    table: {
        maxWidth: '100%',
        height: '100px',
        align: 'center',
        cursor: 'pointer',
    },
});

const mapStateToProps = state => {
    return {
        game: state.game,
    }
};

const mapDispatchToProps = dispatch => {
    return {
        move: (forward) => {
            dispatch({ type: forward?MOVE_FORWARD:MOVE_BACK })
        },
        goto_move: (i) => {
            dispatch({ type: MOVE_GOTO, payload: i })
        }
    }
};

const UnconnectedMovesListPanel = (props) => {
    const { classes, game } = props;

    useEffect(() => {
        let element = document.getElementById('moveslist');
        element.scrollTop = element.scrollHeight;
    });

    const move_strs = game.moves_strings();
    let move_cells;
    if (game.isConnect6()) {
        move_cells = [];
        if (move_strs.length > 0) {
            move_cells.push(
                <TableCell key={0} align='center' onClick={() => props.goto_move(1)}>
                    <Typography variant={(game.until === 1)?'h5':'h6'} color={(game.until === 1)?'error':'textPrimary'}>
                    {move_strs[0]}
                    </Typography>
                </TableCell>)
        }
        for (let i = 1; i < move_strs.length; i++) {
            move_cells.push(
                <TableCell key={i} align='center' 
                                       onClick={() => props.goto_move(1 + 2*i)}>
                    <Typography variant={(game.until === 1 + 2*i || game.until === 2*i)?'h5':'h6'} color={(game.until === 1 + 2*i || game.until === 2*i)?'error':'textPrimary'}>
                        {move_strs[i]}
                    </Typography>
            </TableCell>)
        }
    } else {
        move_cells = move_strs.map((m,i) => (<TableCell key={i} align='center' onClick={() => props.goto_move(i+1)}>
            <Typography variant={(game.until === 1 + i)?'h5':'h6'} color={(game.until === 1 + i)?'error':'textPrimary'}>
            {m}
            </Typography>
        </TableCell>));
    }
    let move_rows = [];
    for (let i = 0; i < move_cells.length; i++) {
        if (i < move_cells.length - 1) {
            move_rows.push(
                    <TableRow key={i}>
                        <TableCell selected={i===4} align={'center'} style={{width: '15%'}}>{1+Math.floor(i/2)}</TableCell>
                        {move_cells[i]}
                        {move_cells[i+1]}
                    </TableRow>
                );
            i += 1;
        } else {
            move_rows.push(
                <TableRow key={i}>
                    <TableCell align={'center'} style={{width: '15%'}}>{1+Math.floor(i/2)}</TableCell>
                    {move_cells[i]}
                    <TableCell key={'rest'}/>
                </TableRow>
            );
        }
    }
    
    return (
        <div style={{margin: '0 auto', width: '70%', height: '100%'}}>
            <Grid container direction={'column'} alignItems={'stretch'} wrap={'nowrap'}
                  style={{flex: '1 1 auto', minHeight: '0px', maxHeight: '100%', width: '100%', height: '100%'}}>
                <Grid item style={{flex: '1 1 auto', minHeight: '0px', maxHeight: '100%'}}>
                    <Paper id='moveslist' className={classes.root}>
                        <Table className={classes.table}>
                            <TableBody>
                                {move_rows}
                            </TableBody>
                        </Table>
                    </Paper>
                </Grid>
                <Grid item>
                    <Grid container direction={'row'} alignItems={'center'} wrap={'nowrap'}
                          style={{width: '100%'}}>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary"
                                        onClick={() => props.move(false)}>
                                    <SkipPreviousIcon/>
                                </Button>
                            </div>
                        </Grid>
                        <Grid item xs>
                            <div style={{display: 'table', margin: '0 auto'}}>
                                <Button variant="outlined" color="primary" onClick={() => props.move(true)}>
                                    <SkipNextIcon/>
                                </Button>
                            </div>
                        </Grid>
                    </Grid>
                </Grid>
            </Grid>
        </div>
    );
};

UnconnectedMovesListPanel.propTypes = {
    game: PropTypes.instanceOf(Game).isRequired    
};



const MovesListPanel = connect(mapStateToProps, mapDispatchToProps)(withStyles(styles)(UnconnectedMovesListPanel));

export default MovesListPanel;