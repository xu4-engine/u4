#!/usr/bin/boron
; Dump xu4 input recording.

key-str: func [u4-key] [
    any [
        select [
            '['  "UP"   '/'  "DN"   ';'  "LT"   '^'' "RT"
            '^A' "LF"   '^D' "CR"   '^(F8)' "Alt+X"
        ] u4-key
        to-char u4-key
    ]
]

key-rule: [bits [key: u8 delay: u16]]

f: first args
parse read f [
    #{da7a4fc0}
    bits [seed: u32]    (print ['seed to-hex seed])
    some [
        '^1' key-rule   (print format [5 3 -4] ['key  key-str key delay])
      | '^2' key-rule   (print format [5 3 -4] ['key1 add 0x100 key delay])
      | '^(ff)'         (print 'end)
    ]
]
