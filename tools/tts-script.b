#!/usr/bin/boron
; Generate Text-To-Speech script for Larynx container.

ifn args [
    print "Usage: tts-script <spec> ..."
    quit/return 64
]

first-words: func [text] [
    parse text [thru ' ' to ' ' :text]
    lowercase construct text [
        '^'' none
        '!'  none
        '?'  none
        ','  none
        '.'  none
        ' ' '_'
    ]
]

vars: ["$V" voice "$L" line "$N" name "$I" id "$F" fw "$P" pitch]

forall args [
    bind vars spec: context load first args

    id: 0
    foreach line spec/lines [
        fw: first-words line
        cmd: either zero? spec/pitch
            {larynx -v $V "$L" >/tmp/$N-$I-$F.wav^/}
            {larynx -v $V "$L" | sox - /tmp/$N-$I-$F.wav pitch $P^/}
        print construct cmd vars
        ++ id
    ]
]
