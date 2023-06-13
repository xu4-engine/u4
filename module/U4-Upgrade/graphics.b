graphics: [
    texture: atlas 64,4096 [
        tiles        0, 0
        hud         17, 0
        tile_guard  17,32
        charset     17,97

        ; Erase animated areas.
        /brush  0,0,0,0
        /rect  10, 258,4,2  ; Ship flags
                7, 274,1,2
                7, 290,4,2
                6, 306,2,2
               10,2050,5,2  ; Pirate ship flags
                7,2066,1,1
                7,2082,5,2
                5,2098,3,2
        /brush  0,0,0,255
        /rect   3,1208,7,5  ; Campfire flames
    ]
    image (name: tiles filename: "u4u/shapes.vga" width: 16 height: 4096 depth: 8 filetype: u4raw tiles: 256 fixup: blackTransparencyHack) [
        at 0,0 size 16,16
        ; subimages
        tile_sea
        tile_water
        tile_shallows
        tile_swamp
        tile_grass
        tile_brush
        tile_forest
        tile_hills
        tile_mountains
        tile_dungeon
        tile_city
        tile_castle
        tile_town
        tile_lcb_west
        tile_lcb_entrance
        tile_lcb_east
        tile_ship   4
        tile_horse  2
        tile_dungeon_floor
        tile_bridge
        tile_balloon
        tile_bridge_n
        tile_bridge_s
        tile_up_ladder
        tile_down_ladder
        tile_ruins
        tile_shrine
        tile_avatar
        tile_mage       2
        tile_bard       2
        tile_fighter    2
        tile_druid      2
        tile_tinker     2
        tile_paladin    2
        tile_ranger     2
        tile_shepherd   2
        tile_column
        tile_waterside_sw
        tile_waterside_se
        tile_waterside_nw
        tile_waterside_ne
        tile_shipmast
        tile_shipwheel
        tile_rocks
        tile_corpse
        tile_stone_wall
        tile_locked_door
        tile_door
        tile_chest
        tile_ankh
        tile_brick_floor
        tile_wood_floor
        tile_moongate_opening   3
        tile_moongate
        tile_poison_field
        tile_energy_field
        tile_fire_field
        tile_sleep_field
        tile_solid
        tile_secret_door
        tile_altar
        tile_campfire
        tile_lava
        tile_miss_flash
        tile_magic_flash
        tile_hit_flash
        tile_unused     2
        tile_villager   2
        tile_bard_singing   2
        tile_jester     2
        tile_beggar     2
        tile_child      2
        tile_bull       2
        tile_lord_british   2
        tile_A
        tile_B
        tile_C
        tile_D
        tile_E
        tile_F
        tile_G
        tile_H
        tile_I
        tile_J
        tile_K
        tile_L
        tile_M
        tile_N
        tile_O
        tile_P
        tile_Q
        tile_R
        tile_S
        tile_T
        tile_U
        tile_V
        tile_W
        tile_X
        tile_Y
        tile_Z
        tile_space
        tile_space_r
        tile_space_l
        tile_window
        tile_black
        tile_brick_wall
        tile_pirate_ship    4
        tile_nixie      2
        tile_giant_squid    2
        tile_sea_serpent    2
        tile_sea_horse      2
        tile_whirlpool      2
        tile_twister        2
        tile_rat        4
        tile_bat        4
        tile_spider     4
        tile_ghost      4
        tile_slime      4
        tile_troll      4
        tile_gremlin    4
        tile_mimic      4
        tile_reaper     4
        tile_insect_swarm   4
        tile_gazer      4
        tile_phantom    4
        tile_orc        4
        tile_skeleton   4
        tile_rogue      4
        tile_python     4
        tile_ettin      4
        tile_headless   4
        tile_cyclops    4
        tile_wisp       4
        tile_evil_mage  4
        tile_liche      4
        tile_lava_lizard    4
        tile_zorn       4
        tile_daemon     4
        tile_hydra      4
        tile_dragon     4
        tile_balron     4
        at 0,1120 tile_fire_phantom 2
    ]
    image (name: tile_guard filename: "vga/tile_guard.png" tiles: 4 fixup: blackTransparencyHack)
    image (name: charset filename: "u4u/charset.vga" width: 8 height: 1024 depth: 8 filetype: u4raw tiles: 128)
    image (name: borders filename: "u4u/start.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    tree: "vutne/145-tree.png" [
        moongate  0,152, 20,24
        items    24,152, 20,24
    ]
    portal:  "vutne/146-portal.png"
    outside: "vutne/147-outside.png"
    inside:  "vutne/148-inside.png"
    wagon:   "vutne/149-wagon.png"
    gypsy:   "vutne/14a-gypsy.png"
    abacus:  "vutne/14b-abacus.png" [
        whitebead 12,181, 7,11
        blackbead 23,181, 7,11
        bead_pos  128,18, 8,16
    ]
    cards1: "vutne/cards1.png" [
        honestycard         0,0, 80,112
        compassioncard     80,0, 80,112
        valorcard         160,0, 80,112
        justicecard       240,0, 80,112
    ]
    cards2: "vutne/cards2.png" [
        sacrificecard       0,0, 80,112
        honorcard          80,0, 80,112
        spiritualitycard  160,0, 80,112
        humilitycard      240,0, 80,112
    ]
    image (name: key filename: "u4u/key7.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: honesty filename: "u4u/honesty.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: compassn filename: "u4u/compassn.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: valor filename: "u4u/valor.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: justice filename: "u4u/justice.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: sacrific filename: "u4u/sacrific.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: honor filename: "u4u/honor.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: spirit filename: "u4u/spirit.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: humility filename: "u4u/humility.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: truth filename: "u4u/truth.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: love filename: "u4u/love.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: courage filename: "u4u/courage.old" width: 320 height: 200 depth: 8 filetype: u4rle fixup: abyss)
    image (name: stoncrcl filename: "u4u/stoncrcl.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: infinity filename: "u4u/rune_0.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune1 filename: "u4u/rune_1.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune2 filename: "u4u/rune_2.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune3 filename: "u4u/rune_3.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune4 filename: "u4u/rune_4.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune5 filename: "u4u/rune_5.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune6 filename: "u4u/rune_6.ega" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune7 filename: "u4u/rune_7.ega" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune8 filename: "u4u/rune_8.ega" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: gemtiles filename: "vga/gem.png" tiles: 128)
    dung_hall: "vga/dungeonhall.png" [
        dung0_lft_ew    0, 0, 16,176
        dung0_rgt_ew  160, 0, 16,176
        dung1_lft_ew   16, 8, 32,160
        dung1_rgt_ew  128, 8, 32,160
        dung2_lft_ew   48,24, 24,128
        dung2_rgt_ew  104,24, 24,128
        dung3_lft_ew   72,72, 16, 32
        dung3_rgt_ew   88,72, 16, 32
        dung0_lft_ns    0, 0, 16,176
        dung0_rgt_ns  160, 0, 16,176
        dung1_lft_ns   16, 8, 32,160
        dung1_rgt_ns  128, 8, 32,160
        dung2_lft_ns   48,24, 24,128
        dung2_rgt_ns  104,24, 24,128
        dung3_lft_ns   72,72, 16, 32
        dung3_rgt_ns   88,72, 16, 32
    ]
    dung_door: "vga/dungeonhall_door.png" [
        dung0_lft_ew_door    0, 0, 16,176
        dung0_rgt_ew_door  160, 0, 16,176
        dung1_lft_ew_door   16, 8, 32,160
        dung1_rgt_ew_door  128, 8, 32,160
        dung2_lft_ew_door   48,24, 24,128
        dung2_rgt_ew_door  104,24, 24,128
        dung3_lft_ew_door   72,72, 16, 32
        dung3_rgt_ew_door   88,72, 16, 32
        dung0_lft_ns_door    0, 0, 16,176
        dung0_rgt_ns_door  160, 0, 16,176
        dung1_lft_ns_door   16, 8, 32,160
        dung1_rgt_ns_door  128, 8, 32,160
        dung2_lft_ns_door   48,24, 24,128
        dung2_rgt_ns_door  104,24, 24,128
        dung3_lft_ns_door   72,72, 16, 32
        dung3_rgt_ns_door   88,72, 16, 32
    ]
    dungback0: "vga/dungeonback0.png" [
        dung0_mid_ew  0,0, 176,176
        dung0_mid_ns  0,0, 176,176
    ]
    dungback1: "vga/dungeonback1.png" [
        dung1_mid_ew  0,0, 176,176
        dung1_xxx_ew  0,8,  16,160
        dung1_mid_ns  0,0, 176,176
        dung1_xxx_ns  0,8,  16,160
    ]
    dungback2: "vga/dungeonback2.png" [
        dung2_mid_ew  16,48, 144,80
        dung2_xxx_ew  16,48,  48,80
        dung2_mid_ns  16,48, 144,80
        dung2_xxx_ns  16,48,  48,80
    ]
    dungback3: "vga/dungeonback3.png" [
        dung3_mid_ew  56,72, 48,32
        dung3_xxx_ew  48,72, 24,32
        dung3_mid_ns  56,72, 48,32
        dung3_xxx_ns  48,72, 24,32
    ]
    dung0_mid_ew_door: "vga/dungeonback0_door.png"
    dung0_mid_ns_door: "vga/dungeonback0_door.png"
    dung1_mid_door_image: "vga/dungeonback1_door.png" [
        dung1_mid_ew_door  0,16, 176,160
        dung1_mid_ns_door  0,16, 176,160
    ]
    dung2_mid_door_image: "vga/dungeonback2_door.png" [
        dung2_mid_ew_door 16,48, 144,80
        dung2_mid_ns_door 16,48, 144,80
    ]
    dung3_mid_door_image: "vga/dungeonback3_door.png" [
        dung3_mid_ew_door 48,72, 80,32
        dung3_mid_ns_door 48,72, 80,32
    ]
    dung0_ladderup_image: "vga/ladderup0.png" [
        dung0_ladderup 45,0, 88,87
    ]
    dung0_ladderup_side_image: "vga/ladderup0_side.png" [
        dung0_ladderup_side 45,0, 88,87
    ]
    dung1_ladderup_image: "vga/ladderup1.png" [
        dung1_ladderup 64,40, 50,48
    ]
    dung1_ladderup_side_image: "vga/ladderup1_side.png" [
        dung1_ladderup_side 64,40, 50,48
    ]
    dung2_ladderup_image: "vga/ladderup2.png" [
        dung2_ladderup 77,68, 22,19
    ]
    dung2_ladderup_side_image: "vga/ladderup2_side.png" [
        dung2_ladderup_side 77,68, 22,19
    ]
    dung3_ladderup_image: "vga/ladderup3.png" [
        dung3_ladderup  84,82, 8,6
    ]
    dung3_ladderup_side_image: "vga/ladderup3_side.png" [
        dung3_ladderup_side  84,82, 8,6
    ]
    dung0_ladderdown_image: "vga/ladderdown0.png" [
        dung0_ladderdown  45,87, 88,89
    ]
    dung0_ladderdown_side_image: "vga/ladderdown0_side.png" [
        dung0_ladderdown_side  45,87, 88,89
    ]
    dung1_ladderdown_image: "vga/ladderdown1.png" [
        dung1_ladderdown  64,86, 50,50
    ]
    dung1_ladderdown_side_image: "vga/ladderdown1_side.png" [
        dung1_ladderdown_side  64,86, 50,50
    ]
    dung2_ladderdown_image: "vga/ladderdown2.png" [
        dung2_ladderdown  77,86, 22,22
    ]
    dung2_ladderdown_side_image: "vga/ladderdown2_side.png" [
        dung2_ladderdown_side  77,86, 22,22
    ]
    dung3_ladderdown_image: "vga/ladderdown3.png" [
        dung3_ladderdown  84,88, 8,8
    ]
    dung3_ladderdown_side_image: "vga/ladderdown3_side.png" [
        dung3_ladderdown_side  84,88, 8,8
    ]
    dung0_ladderupdown_image: "vga/ladderupdown0.png" [
        dung0_ladderupdown  45,0, 88,176
    ]
    dung0_ladderupdown_side_image: "vga/ladderupdown0_side.png" [
        dung0_ladderupdown_side  45,0, 88,176
    ]
    dung1_ladderupdown_image: "vga/ladderupdown1.png" [
        dung1_ladderupdown  64,40, 50,96
    ]
    dung1_ladderupdown_side_image: "vga/ladderupdown1_side.png" [
        dung1_ladderupdown_side  64,40, 50,96
    ]
    dung2_ladderupdown_image: "vga/ladderupdown2.png" [
        dung2_ladderupdown  77,68, 22,40
    ]
    dung2_ladderupdown_side_image: "vga/ladderupdown2_side.png" [
        dung2_ladderupdown_side  77,68, 22,40
    ]
    dung3_ladderupdown_image: "vga/ladderupdown3.png" [
        dung3_ladderupdown  84,82, 8,12
    ]
    dung3_ladderupdown_side_image: "vga/ladderupdown3_side.png" [
        dung3_ladderupdown_side  84,82, 8,12
    ]
    dung_traps: "vga/traps.png" [
        dung0_hole  38,  0,100,10
        dung1_hole  54, 26, 68,13
        dung2_hole  71, 57, 34, 7
        dung0_pit   38,166,100,10
        dung1_pit   54,137, 68,13
        dung2_pit   71,112, 34, 7
    ]
]

tileanim: [
    scroll:     [scroll 1]
    scroll_pool:[scroll 1,0,0,2]    ; 2=tile_shallows
    frame:      [random 50 frame]
    slow_frame: [random  5 frame]
    cityflag:   [random 50 invert 5,2,5,4]
    lcbflag:    [random 50 invert 5,0,7,4]
    castleflag: [random 50 invert 5,0,4,4]
    shipflag: [
        random 50
        invert 10,1,5,4 context-frame 0
        invert 6,1,3,3  context-frame 1
        invert 7,2,5,2  context-frame 2
        invert 6,2,2,2  context-frame 3
    ]
    pirateflag: [
        random 50
        invert 10,2,6,2 context-frame 0
        invert  6,2,3,2 context-frame 1
        invert  7,2,5,2 context-frame 2
        invert  6,2,2,2 context-frame 3
    ]
    campfire: [
        pixel_color 4,7,5,7 [
             97,  0, 0
            255,200, 0
        ]
        pixel_color 4,7,5,7 [
              0, 0, 0
            156, 0, 0
        ]
    ]
    phantom_flicker: [frame random 50 scroll 1]
]
