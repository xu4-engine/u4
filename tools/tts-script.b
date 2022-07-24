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

vars: ["$V" voice "$L" line "$M" model "$N" name "$I" id2 "$F" fw "$P" pitch]

forall args [
    bind vars spec: context load first args

    either path? vpath: spec/voice [
        ; Use Coqui TTS if voice is path! (model_name/speaker_idx)
        model: slice vpath -1
        spec/voice: last vpath
        cmd: {tts --model_name $M --speaker_idx $V --text "$L" --out_path /tmp/$N-$I-$F.wav^/}
    ][
        ; Otherwise use Larynx
        cmd: either zero? spec/pitch
            {larynx -v $V "$L" >/tmp/$N-$I-$F.wav^/}
            {larynx -v $V "$L" | sox - /tmp/$N-$I-$F.wav pitch $P^/}
    ]

    id: 0
    foreach line spec/lines [
        id2: either lt? id 10 [join '0' id] id
        either string? line [
            fw: first-words line
            print construct cmd vars
        ][
            print construct {ln -s sq_blip_22k.wav /tmp/$N-$I-blip.wav^/} vars
        ]
        ++ id
    ]
]
