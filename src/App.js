import React, { useEffect } from 'react';
import './App.css';
import TableView from './Pages/Table';
import { connect } from 'react-redux';
import {SUBSCRIBER} from "./redux_actions/actionTypes";

const mapStateToProps = state => {
    return {
        wasm: state.wasm
    }
};

const mapDispatchToProps = dispatch => {
    return {
        set_subscriber: (sub) => {
            dispatch( { type: SUBSCRIBER, payload: sub });
        }
    }
};


function UnconnectedApp(props) {
    
    const { set_subscriber } = props;
    
    useEffect(() => {
        fetch('/gameServer/mmai/subscriber.jsp')
            .then(response => response.text())
            .then(text => {
                set_subscriber(text.indexOf('subscriber') === 0);
            });        
    });
    
    return (
        <div style={{width: '100vw', height: '100vh'}}>
            <TableView/>
        </div>
    );
}

const App = connect(mapStateToProps, mapDispatchToProps)(UnconnectedApp);

export default App;

