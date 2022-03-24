#!/usr/bin/boron -s
; Concatenate Wave files and encode as OGG Vorbis.

dry: false
character:
out-file:
sox-opt: none
wave: []
parts: []

forall args [
    switch first args [
        "-d" [dry: true]
        "-o" [out-file: second ++ args]
        "-s" [sox-opt: second ++ args]
        "-c" [
            character: second ++ args
            out-file: join character %.ogg
            sox-opt: select load join character %.lines 'pre-encode
        ]
        [append wave first args]
    ]
]

cmd: make string! 1024
buf: make string! 64
append cmd "sox "
total: 0.0

basename: func [path] [
    if name: find/last path '/' [return next name]
    path
]

make-dir tmp: %/tmp/xu4-vocalize/

foreach fn wave [
    tn: join tmp basename fn
    pad-cmd: construct "sox < @ ? pad 0 1.0"
                        ['<' fn '@' tn '?' sox-opt]

    either dry [
        print pad-cmd
    ][
        execute pad-cmd
        execute/out join "soxi -D " tn buf
        dur: to-double buf

        append parts reduce [mark-sol tn sub dur 1.0 total]
        total: add total dur
    ]

    append append cmd tn ' '
]
append cmd "/tmp/xu4-vocalize.wav"

encode: "oggenc -q 0 /tmp/xu4-vocalize.wav"
if out-file [
    appair encode " -o " out-file
]
either dry [
    print cmd
    print encode
][
    either character [
        save join character %-parts.b parts
    ][
        probe parts
    ]
    execute cmd
    execute encode
]
