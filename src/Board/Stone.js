import React from 'react';

function Stone(props) {
    return (
        <svg key={props.id} height={props.size*1.5} width={props.size*1.5}>
            <defs>
                <filter id="shadow">
                    <feDropShadow dx="0.7" dy="0.7" stdDeviation="0.35" floodColor={'#434A50'}/>
                </filter>
                <radialGradient id={props.id} cx="40%" cy="40%" r="50%">
                    <stop offset="0%" stopColor='var(--color1)' />
                    <stop offset="100%" stopColor='var(--color2)' />
                </radialGradient>
            </defs>
            {/*<filter id="f2" x="0" y="0" width="150%" height="150%">*/}
                {/*<feOffset result="offOut" in="SourceAlpha" dx={0.75} dy={0.75} />*/}
                {/*<feGaussianBlur result="blurOut" in="offOut" stdDeviation="0.75" />*/}
                {/*<feBlend in="SourceGraphic" in2="blurOut" mode="normal" />*/}
            {/*</filter>*/}
            <circle cx={props.size/2} cy={props.size/2} r={props.size/2}
                    fill={"url(#"+props.id+")"} fillOpacity={props.opacity}
                    pointerEvents={'none'} filter={"url(#shadow)"}/>
        </svg>
    );
}

export default Stone;