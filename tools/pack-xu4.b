#!/usr/bin/boron -s
; Create xu4 module package.

usage: {{
Usage: pack-xu4 [<OPTIONS>] <module-path>

Options:
  -h           Print this help and exit.
  -o <module>  Output module file.
  -v <level>   Set verbose level. (1-2, default is 1)
  --version    Print version and exit.
}}

root-path: %module/u4/
module-file: none
verbose: 1

forall args [
    switch first args [
        "-v" [verbose: to-int second ++ args]
        "-h" [print usage quit]
        "-o" [module-file: to-file second ++ args]
        "--version" [print "pack-xu4 0.1.1" quit]
        [root-path: to-file first args]
    ]
]

fatal: func ['code msg] [
    print msg
    quit/return select [
        usage   64  ; EX_USAGE
        noinput 66  ; EX_NOINPUT
        config  78  ; EX_CONFIG
    ] code
]

config-file: join terminate root-path '/' %config.b
ifn exists? config-file [
    fatal noinput ["Cannot find config" config-file]
]

ifn module-file [
    module-file: join last split root-path '/' %.mod
]

; Matches ConfigValues in config_boron.cpp.
config: context [
    author:
    about:
    version:
    rules:
    armors:
    weapons:
    creatures:
    graphics:
    layouts:
    maps:
    tile-rules:
    tileset:
    u4-save-ids:
    music:
    sound:
    vendors:
    ega-palette:
        none
]

apair: func [series a b] [append append series a b]

enum-value: func [list val] [
    either pos: find list val [sub index? pos 1] 0
]

class-constraint: context [
    mage:   0x01  bard:    0x02  fighter: 0x04  druid:    0x08
    tinker: 0x10  paladin: 0x20  ranger:  0x40  shepherd: 0x80
]

; Spec is ['not block! | 'only block!]
make-constraint: func [spec] [
    ifn word? first spec [return 0xff]
    usable: add 0 reduce bind second spec class-constraint
    if eq? 'not first spec [
        usable: xor 0xff usable
    ]
    usable
]

weapon-flags: swap [
    0x01 lose
    0x02 losewhenranged
    0x04 choosedistance
    0x08 alwayshits
    0x10 magic
    0x40 attackthroughobjects
    0x80 absolute_range
   0x100 returns
   0x200 dontshowtravel
]

weapon-tiles: context [
    hittile: misstile: leavetile: none
    reset: does [hittile: misstile: leavetile: none]
]

direction-mask: [
    none    0x01
    west    0x02
    north   0x04
    east    0x08
    south   0x10
    advance 0x20
    retreat 0x40
    all     0x7e
]

attribute-flags: func [blk spec] [
    flags: 0
    foreach [mask name] spec [
        if select blk name [flags: or flags mask]
    ]
    flags
]

none-zero: func [v] [either v v 0]
none-neg1: func [v] [either v v -1]

;---------------------------------------
; CDI Package

; The construct binary! rules for the Table Of Contents.
toc: []

pkg-len: 0

cdi-begin: func [app_id /extern pkg-len] [
    clear toc
    write module-file bin: rejoin [#{DA7A7000} app_id "....----"]
    pkg-len: size? bin
]

cdi-end: does [
    toc: construct binary! toc
    write/append module-file toc

    ; Poke toc offset & length.
    fh: open/write module-file
    write skip fh 8 construct binary! reduce [
        'u32 pkg-len size? toc
    ]
    close fh
]

; Add toc entry and append data to pack-bin.
cdi-chunk: func [cdi-format name data /extern pkg-len] [
    append toc reduce [
        #{DA7A} 'big-endian 'u16 cdi-format
        to-binary name
        'little-endian 'u32
        pkg-len
        size? data
    ]
    write/append module-file data
    pkg-len: add pkg-len size? data
]

; Make CDI String Table (Form 1).
cdi-string-table1: func [dict] [
    store: make binary! 2048
    v16: make vector! 'u16
    foreach str dict [
        append v16 size? store
        ifn string? str [str: to-string str]
        apair store str '^0'
    ]
    construct binary! reduce [
        'u8 1 0 'u16 'big-endian size? v16
        to-binary v16
        store
    ]
]

file-dict: make block! 128
file-id-seen: false

; File-id returns the zero-based index of str in file-dict.
; The global file-id-seen is set to true if str was already present in
; file-dict, or false if it was not.
file-id: func [str /extern file-id-seen] [
    either file-id-seen: find file-dict str [
        idx: sub index? file-id-seen 1
    ][
        idx: size? file-dict
        append file-dict str
    ]
    idx
]

img_id: "IM^0^0"
file_buf: make binary! 4096

; Return app_id of PNG chunk.
pack-png: func [path filename] [
    n: file-id filename
    poke img_id 3 div n 256
    poke img_id 4 and n 255

    ifn file-id-seen [
        cdi-chunk 0x1002 img_id read/into join path filename file_buf
    ]
    img_id
]

print-toc: does [
    print "table-of-contents: ["
    ascii: charset "0-9a-zA-Z"
    v32: #[]
    while [not tail? toc] [
        app_id: slice toc 4,4
        if parse app_id [4 ascii] [
            app_id: to-string app_id
        ]

        append clear v32 slice toc 8,8
        print format ["  " 0 ' ' 11 -10 -9] [
            slice toc 4
            app_id
            to-hex first  v32
            second v32
        ]
        toc: skip toc 16
    ]
    print ']'
]

;---------------------------------------
; Load module configuration

module: func [blk] [do blk]

includes: []
include: func [file] [append includes file]

cfg: make config load config-file

foreach file includes [
    do bind load join root-path file cfg
]

process-cfg: func [blk] [do bind blk cfg]

;---------------------------------------
; Build module package.

process-sound: func [blk app_id] [
    path: root-path
    n: 0
    app_id: copy app_id
    parse blk [some[
        tok: file!  (
            fname: first tok
            fmt: select [
                ".wav" 0x2006
                ".mp3" 0x2007
                ".ogg" 0x2008
            ] ext: find/last fname '.'
            ifn fmt [fatal config ["Unknown audio file extension" ext]]

            poke app_id 3 div n 256
            poke app_id 4 and n 255
            ++ n

            cdi-chunk fmt app_id read join path first tok
        )
      | 'path file! (path: terminate join root-path second tok '/')
    ]]
    n
]

/*
  Parse data block! with rules and replace the data with a new block.
  The global 'blk variable is the output block which 'rules should append to.
*/
process-blk: func ['data rules /extern blk] [
    blk: make block! 16
    ifn parse get data [some rules] [
        fatal config join "Invalid " data
    ]
    set data blk
]

/*
  Src is a block of paren!, each containing name & value pairs.
  Each paren! gets assigned to the 'it variable for use by transform.
  The 'dest variable is the output block.
*/
emit-attr-block: func [blk src transform /extern dest it] [
    either empty? src [
        append blk none
    ][
        append/block blk dest: make block! 0
        foreach it src transform
    ]
]

current-trans: none
new-transform: func [type data /extern current-trans] [
    current-trans: tail transforms
    either coord? data [
        x: first  data
        y: second data
        w: third  data
        h: pick data 4
    ][
        x: data
        y: w: h: 0
    ]
    append transforms construct binary! [    ; struct TileAnimTransform
        u8  type 0 0 0
        u16 x y w h
        u32 0 0
    ]
]

map-labels: []
map-portals: []
map-moongates: []
map-roles: []

cdi-begin "xu4^1"

process-cfg [
    music: process-sound music "MU^0^0"
    sound: process-sound sound "SO^0^0"

    process-blk armors [
        tok: string! int! opt [word! block!] (
            apair blk first tok to-coord reduce [
                second tok
                make-constraint skip tok 2
            ]
        )
    ]

    process-blk weapons [
           ; Abbr.   Name   Range Damage
        tok: string! string! int! int!
        (constr: 0xff flags: 0 weapon-tiles/reset) any [
            stok:
            word! block!    (constr: make-constraint stok)
          | set-word! word! (set in weapon-tiles first stok second stok)
          | word!           (flags: or flags select weapon-flags first stok)
        ] (
            append blk slice tok 2
                                      ;[range  damage  flags  canuse]
            append blk to-coord reduce [third tok  pick tok 4  flags  constr]
            val: slice values-of weapon-tiles 3
            ifn empty? collect word! val [
                append blk val
            ]
        )
    ]

    process-blk creatures [
        set at paren! (
            append blk mark-sol to-coord reduce [
                at/id
                none-zero at/leader
                none-zero at/spawnsOnDeath
                none-zero at/basehp
                none-zero at/exp
                none-zero at/encounterSize
            ]
            apair blk at/name at/tile

            amask: or none-zero select [food 0x01  gold 0x02] at/steals
                      none-zero select [sleep 0x04  negate 0x80] at/casts

            mmask: none-zero select [none 0x01  wanders 0x02] at/movement

            append blk to-coord reduce [
                or amask attribute-flags at [
                     ;0x01 stealFood
                     ;0x02 stealGold
                     ;0x04 castsSleep
                      0x08 undead
                      0x10 good
                      0x20 swims        ; water
                      0x20 sails        ; water
                      0x40 cantattack   ; nonAttackable
                     ;0x80 negate
                    0x0100 camouflage
                    0x0200 wontattack   ; noAttack
                    0x0400 ambushes
                   ;0x0800 randomRanged
                    0x1000 incorporeal
                    0x2000 noChest
                    0x4000 divides
                    0x8000 spawnsOnDeath
                ]
                or mmask attribute-flags at [
                     ;0x01 stationary
                     ;0x02 wanders
                      0x04 swims
                      0x08 sails
                      0x10 flies
                      0x20 teleports
                      0x40 canMoveOntoCreatures
                      0x80 canMoveOntoAvatar
                     0x100 forceOfNature
                ]
                none-zero select [fire 1  sleep 2  poison 4] at/resists
                attribute-flags at [
                    0x1 ranged
                    0x2 leavestile
                ]
            ]

            ctiles: reduce [
                at/rangedHitTile
                at/rangedMissTile
                at/camouflageTile
                at/worldRangedTile
            ]
            ifn empty? collect word! ctiles [
                append blk ctiles
            ]
        )
    ]

    layout-subimages: func [spec block! out block! width none!/int!] [
        ; Stack vertically if width not provided.
        if none? width [width: 0]

        pos: 0,0
        size: 16,16
        next-tile: [
            x: add first pos first size
            y: second pos
            if ge? x width [
                x: 0
                y: add y second size
            ]
            pos: to-coord [x y]
        ]
        emit-name: [append out mark-sol first tok]

        parse spec [some[
            tok:
            'at   set pos  coord!
          | 'size set size coord!
          | word! coord! (do emit-name append out second tok)
          | word! int! (
              frames: second tok
              do emit-name append out to-coord [pos size frames]
              do next-tile
              loop sub frames 1 [
                  apair out mark-sol '_cel to-coord [pos size]
                  do next-tile
              ]
            )
          | word! (
              do emit-name append out to-coord [pos size]
              do next-tile
            )
        ]]
    ]

    image-path: join root-path %image/
    n: 0

    process-blk graphics [
        'imageset set name word!/path! (
            either word? name [
                extends: none
            ][
                extends: first name
                name: second name
            ]
            apair blk mark-sol 'imageset name
            append blk extends
            append/block blk img-blk: make block! 0
        ) into [some [
            'image set at paren! tok: opt block! (
                fname: at/filename
                ext: find/last fname '.'
                either eq? ".png" ext [
                    fname: copy pack-png image-path fname
                    ftype: 'png
                ][
                    ftype: at/filetype
                ]

                apair img-blk mark-sol at/name fname
                apair img-blk to-coord reduce [
                    none-neg1 at/width
                    none-neg1 at/height
                    none-neg1 at/depth
                ] to-coord reduce [
                    enum-value [
                        none png u4raw u4rle u4lzw
                        u5lzw fmtowns fmtowns-pic fmtowns-tif
                    ] ftype
                    ; Skip name which can be 'tiles (assuming name is first).
                    none-zero select skip at 2 'tiles
                    enum-value [
                        none intro abyss abacus dungns
                        blackTransparencyHack fmtownsscreen
                    ] at/fixup
                ]

                if block? first tok [
                    append/block img-blk make block! 8
                    layout-subimages first tok last img-blk at/width
                ]
            )
          | tok: set-word! 'atlas coord! block! (
                apair img-blk mark-sol to-word first tok 'atlas
                append img-blk third tok
                append/block img-blk pick tok 4
            )
        ]]
      | 'tileanimset set name word! (
            apair blk mark-sol 'tileanims name
            anim-dict: make block! 8
            transforms: make binary! 512
        ) into [some [
            tok: set-word! (
                apair anim-dict
                    to-word first tok
                    to-coord reduce [
                        div size? transforms 20     ; sizeof(TileAnimTransform)
                        none-zero select second tok 'random
                    ]
            ) into [some [
                'random int!    ; Ignore, handled above.
              | 'invert set n coord! (new-transform 0 n)
              | 'scroll set n int!   (new-transform 1 n)
              | 'frame               (new-transform 2 0)
              | 'pixel_color set n coord! (new-transform 3 n) into [
                    2 coord!
                ]
              | 'context-frame set n int! (
                    poke current-trans 3 1
                    poke current-trans 4 n
                )
            ]]
        ]] (append/block blk append anim-dict mark-sol transforms)
    ]

    process-blk tile-rules [
        set at paren! (
            apair blk mark-sol at/name to-coord reduce [
                enum-value [fast slow vslow vvslow]
                    at/speed
                enum-value [none fire sleep poison poisonfield electricity lava]
                    at/effect
                xor 0x7e none-zero select direction-mask at/cantwalkon
                xor 0x7e none-zero select direction-mask at/cantwalkoff
                attribute-flags at [
                      0x01 ship
                      0x02 horse
                      0x04 balloon
                      0x08 dispel
                      0x10 talkover
                      0x20 door
                      0x40 lockeddoor
                      0x80 chest
                    0x0100 canattackover
                    0x0200 canlandballoon
                    0x0400 replacement
                    0x0800 onWaterOnlyReplacement
                    0x1000 foreground
                    0x2000 livingthing
                ]
                attribute-flags at [        ; movement
                    1 swimable
                    2 sailable
                    4 unflyable
                    8 creatureunwalkable
                ]
            ]
        )
    ]

    process-blk tileset [
        set at paren! (
            apair blk mark-sol at/name at/rule
            apair blk at/image at/animation
            apair blk at/directions to-coord reduce [
                none-zero at/frames
                none-zero select [square 1 round 2] at/opaque
                attribute-flags at [
                    2 usesReplacementTileAsBackground
                    4 usesWaterReplacementTileAsBackground
                    8 tiledInDungeon
                ]
            ]
        )
    ]

    process-blk maps [
        'map set at paren! (
            apair blk mark-sol to-file at/fname to-coord reduce [
                at/id
                enum-value [world city shrine combat dungeon] at/type
                enum-value [wrap exit fixed] at/borderbehavior
                at/width
                at/height
                at/levels
            ]
            append blk to-coord reduce [
                none-zero at/chunkwidth
                none-zero at/chunkheight
                attribute-flags at [
                    0x02 nolineofsight
                    0x04 firstperson
                ]
                none-zero at/music
            ]

            clear map-labels
            clear map-portals
            clear map-moongates
            clear map-roles
        )
      | into [some[
            'portal set at2 paren! content: opt block! (
                append/block map-portals at2
                if block? first content [
                    ; Merge retroActiveDest into attributes to be used below.
                    if it: content/1/retroActiveDest [
                        apair at2 to-set-word 'retroActiveDest
                            to-coord reduce [it/x it/y none-zero it/z it/mapid]
                    ]
                ]
            )
          | 'label set at2 paren! (append/block map-labels at2)
          | 'city  set at2 paren! into [any [
                'personrole tok: paren! (append/block map-roles first tok)
            ]] (
                append blk reduce [at2/name at2/type to-file at2/tlk_fname]
                emit-attr-block blk map-roles [
                    append dest to-coord reduce [
                        enum-value [
                            companion    weaponsvendor  armorvendor
                            foodvendor   tavernkeeper   reagentsvendor
                            healer       innkeeper      guildvendor
                            horsevendor  lordbritish    hawkwind
                        ] it/role
                        it/id
                    ]
                ]
            )
          | 'dungeon  set at2 paren! (apair blk at2/name at2/rooms)
          | 'shrine   set at2 paren! (
                apair blk at2/mantra enum-value [
                    "honesty" "compassion" "valor" "justice"
                    "sacrifice" "honor" "spirituality" "humility"
                ] at2/virtue
            )
          | 'moongate set at2 paren! (append/block map-moongates at2)
        ]] (
            emit-attr-block blk map-labels [
                apair dest mark-sol it/name to-coord reduce [
                    none-zero it/x
                    none-zero it/y
                    none-zero it/z
                ]
            ]

            emit-attr-block blk map-portals [
                apair dest mark-sol it/message it/condition
                append dest to-coord reduce [
                    none-zero it/x
                    none-zero it/y
                    none-zero it/z
                    none-zero it/startx
                    none-zero it/starty
                    none-zero it/startlevel
                ]
                append dest to-coord reduce [
                    none-zero it/destmapid
                    none-zero select [
                       enter    1
                       klimb    2
                       descend  4
                       exit_north   8
                       exit_east    0x10
                       exit_south   0x20
                       exit_west    0x40
                    ] it/action
                    either eq? 'true it/savelocation 1 0
                    none-zero select [
                        foot    1
                        horse   2
                        ship    4
                        balloon 8
                        footorhorse 3
                    ] it/transport
                ]
                append dest it/retroActiveDest
            ]

            emit-attr-block blk map-moongates [
                append dest to-coord reduce [it/phase it/x it/y]
            ]
        )
    ]

    bin: make binary! mul 16 4
    foreach it ega-palette [
        apair bin first it second it
        apair bin third it 255
    ]
    ega-palette: bin
]

; Pull in shader files
if exists? spath: join root-path %shader/ [
    sl_id: "SL^0^0"
    foreach file read spath [
        switch find/last file '.' [
            %.glsl [
                n: file-id file
                poke sl_id 3 div n 256
                poke sl_id 4 and n 255
                ifn file-id-seen [
                    cdi-chunk 0x0001 sl_id
                        /*compress*/ read/into join spath file file_buf
                ]
            ]
            %.png [
                pack-png spath file
            ]
        ]
    ]
]


if ge? verbose 2 [
    probe cfg
    probe file-dict
]

cdi-chunk 0x7FC0 "CONF" serialize reduce [cfg]
cdi-chunk 0x0006 "FNAM" cdi-string-table1 file-dict
cdi-end

if ge? verbose 1 [print-toc]
