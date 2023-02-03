#!/usr/bin/boron -s
; Process Wave files with Sox using .lines pre-encode commands.

dry: false
single: none

forall args [
    switch first args [
        "-d" [dry: true]
        "-s" [single: second ++ args]
        [stream: first args]
    ]
]

first-words: func [text] [
    parse text [thru ' ' to ' ' :text]
    lowercase construct text [
        '^'' none  '!' none  '?' none  ',' none  '.' none  ':' none
        ' ' '_'
    ]
]

vars: ["$N" name "$I" id2 "$F" fw '@' stream]

process: func [
    spec context!
    /extern name id2 fw
][
    ifn sox-opt: spec/pre-encode [exit]
    if all [single ne? spec/name single] [exit]
    bind vars spec

    id: 0
    foreach line spec/lines [
        id2: either lt? id 10 [join '0' id] id
        if string? line [
            fw: first-words line
            in:  construct "@/orig/$N-$I-$F.wav" vars
            out: construct "@/$N-$I-$F.wav" vars
            ifn exists? in [
                either dry [
                    print ['mv out in]
                ][
                    rename out in
                ]
            ]
            cmd: rejoin ["sox " in ' ' out ' ' sox-opt]
            either dry [print cmd] [execute cmd]
        ]
        ++ id
    ]
]

ifn dry [
    make-dir join stream %/orig
]

spec: load join stream %.lines
either eq? 'context first spec [
    foreach it spec [
        if block? it [process context it]
    ]
][
    process context spec
]
