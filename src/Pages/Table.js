import React, { useState, useEffect, useRef } from 'react';
import { connect } from 'react-redux';
import PropTypes from 'prop-types';
import Board from '../Board/Board';
import Grid from '@material-ui/core/Grid';
import AdSense from 'react-adsense';
import Snack from './Snack';
import GameInfoPanel from "./GameInfoPanel";

const mapStateToProps = state => {
    return {
        subscriber: state.subscriber
        // subscriber: true
    }
};

const mapDispatchToProps = dispatch => {
    return {
        send_message: message => {
            dispatch();
        },
    }
};


const UnconnectedTable = (props) => {

    const { table, subscriber } = props;
    const [height, setHeight] = useState(0);
    const [width, setWidth] = useState(0);
    const ref = useRef(null);

    // eslint-disable-next-line
    useEffect(() => {
        if (ref.current !== null) {
            setHeight(ref.current.clientHeight - (!subscriber?90:0));
            // console.log(ref.current.clientHeight, ref.current.clientWidth)
            setWidth(ref.current.clientWidth);
        } 
    });
    

    return (
        <div ref={ref} style={{height: '100%', width: '100%'}}>

            <Grid container direction={'column'} alignItems={'center'} wrap={'nowrap'} style={{height: '100%', width: '100%'}}>
                {!subscriber &&
                <Grid item style={{ display:'inline-block',width:'970px',height:'90px' }}>
                    <AdSense.Google
                        client='ca-pub-3326997956703582'
                        slot='3134525496'
                        style={{ display:'inline-block',width:'970px',height:'90px' }}
                        format=''
                    />
                </Grid>
                }
                <Grid item style={{flex: '1', minHeight: '0px'}}>
        <div style={{height: '100%', width: Math.min(width, height + 600)}}>
            <Grid container direction={'row'} alignItems={'stretch'} wrap={'nowrap'} style={{height: '100%'}}>
                <Grid item style={{height:'100%'}}>
                    <div style={{height: '100%', width: height}}>
                        <Board />
                    </div>
                </Grid>
                <Grid item style={{height:'100%', width: Math.min(600, width - height)}}>
                    <Grid container direction={'column'} alignItems={'stretch'}  wrap={'nowrap'}
                          style={{height:'100%', width: '100%'}}>
                        <Grid item style={{height: '100%', borderWidth: '1px', borderStyle: 'solid'}}>
                            <div style={{width: '100%', height: '100%', backgroundColor: '#fffff'}}>
                                <GameInfoPanel/>
                            </div>
                        </Grid>
                    </Grid>
                </Grid>
            </Grid>

            <Snack/>
        </div>
                </Grid>
            </Grid>
        </div>
    );
};

UnconnectedTable.propTypes = {
};


const TableView = connect(mapStateToProps, mapDispatchToProps)(UnconnectedTable);

export default TableView;