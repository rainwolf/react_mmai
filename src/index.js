import React from 'react';
import './index.css';
import App from './App';
import * as serviceWorker from './serviceWorker';
import mmaiApp from './redux_reducers/rootReducer';
import { createStore, applyMiddleware } from 'redux';
import { Provider } from 'react-redux';
import createSagaMiddleware from 'redux-saga';
import rootSaga from './redux_saga/sagas';

import { disableReactDevTools } from '@fvilers/disable-react-devtools';
import {createRoot} from "react-dom/client";

if (process.env.NODE_ENV === 'production') {
    disableReactDevTools();
}

const sagaMiddleware = createSagaMiddleware();
const store = createStore(mmaiApp, applyMiddleware(sagaMiddleware));

sagaMiddleware.run(rootSaga);

const root = createRoot(document.getElementById("root"));
root.render(
   // <React.StrictMode>
      <Provider store={store}>
         <App />
      </Provider>
   // </React.StrictMode>
);

// If you want your app to work offline and load faster, you can change
// unregister() to register() below. Note this comes with some pitfalls.
// Learn more about service workers: https://bit.ly/CRA-PWA
serviceWorker.unregister();
