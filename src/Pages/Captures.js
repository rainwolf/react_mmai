import React from 'react';
import { connect } from 'react-redux';
import PropTypes from 'prop-types';
import { Game } from "../Classes/GameClass";
import SimpleStone from '../Board/SimpleStone';
import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';

const mapStateToProps = state => {
    return {
        game: state.game
    }
};


const UnconnectedCaptures = (props) => {
    const { seat, game } = props;
    
    return (
        <Grid container direction={'row'} alignItems={'stretch'} wrap={'nowrap'}
              style={{width: '100%', height: '100%'}}>
            <Grid item xs>
                <div style={{float: 'right', marginRight: 10}}>
                    <SimpleStone size={40} id={game.player_color(3-seat)}/>
                </div>
            </Grid>
            <Grid item xs>
                <Typography variant="h4" color={game.critical_captures(3 - seat)?'error':'textPrimary'}>
                    x {game.captures[3 - seat]}
                </Typography>
            </Grid>
        </Grid>
    );
};

UnconnectedCaptures.propTypes = {
    seat: PropTypes.number.isRequired,
    game: PropTypes.instanceOf(Game).isRequired,
};


const Captures = connect(mapStateToProps)(UnconnectedCaptures);

export default Captures;