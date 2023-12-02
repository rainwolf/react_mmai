import React, { useState } from 'react';
import { connect } from 'react-redux';
import PropTypes from 'prop-types';
import { REMOVE_SNACK } from "../redux_actions/actionTypes";
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import ErrorIcon from '@mui/icons-material/Error';
import Snackbar from '@mui/material/Snackbar';
import SnackbarContent from '@mui/material/SnackbarContent';
import {withStyles} from '@mui/styles';
import classNames from 'classnames';
import green from '@mui/material/colors/green';
import amber from '@mui/material/colors/amber';
import red from '@mui/material/colors/red';
import blue from '@mui/material/colors/blue';

const mapStateToProps = state => {
    return {
        snack: state.snack
    }
};

const mapDispatchToProps = dispatch => {
    return {
        close_snack: () => {
            dispatch( { type: REMOVE_SNACK });
        }
    }
};

const styles = theme => ({
    success: {
        backgroundColor: green[600],
    },
    error: {
        backgroundColor: red[500],
    },
    info: {
        backgroundColor: blue[500],
    },
    warning: {
        backgroundColor: amber[700],
    },
    icon: {
        fontSize: 20,
    },
    iconVariant: {
        opacity: 0.9,
    },
    message: {
        display: 'flex',
        alignItems: 'center',
    },
});


const UnConnectedSnack = (props) => {
    const { snack, close_snack, classes } = props;
    
    let [open, toggle] = useState(snack !== undefined);

    if (!open) {
        open = snack !== undefined && snack !== '';
    } 
    
    const close = () => {
        toggle(false);
        close_snack();
    };

    // console.log(snack)
    // console.log(open)
    
    let variant, message, Icon;
    variant = 'error';
    message = 'Game over, you lose.';
    Icon = ErrorIcon;
    if (snack)  {
        if (snack === 1) { // i win
            variant = 'success';
            message = 'Game over, you win.';
            Icon = CheckCircleIcon;
        } else { // i lose
            variant = 'error';
            message = 'Game over, you lose.';
            Icon = ErrorIcon;
        }
    }
    
    return (
        <Snackbar
            anchorOrigin={{
                vertical: 'bottom',
                horizontal: 'left',
            }}
            open={open}
            autoHideDuration={4000}
            onClose={close}
        >
            <SnackbarContent
                className={classes[variant]}
                aria-describedby="client-snackbar"
                message={
                    <span id="client-snackbar" className={classes.message}>
                    <Icon className={classNames(classes.icon, classes.iconVariant)}/>
                        {message}
                    </span>
                }
            />
        </Snackbar>        
    );
};

UnConnectedSnack.propTypes = {
    snack: PropTypes.number
};

const Snack = connect(mapStateToProps, mapDispatchToProps)(withStyles(styles)(UnConnectedSnack));

export default Snack;
