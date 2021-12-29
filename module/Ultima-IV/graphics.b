graphics: [

imageset EGA [
    texture: atlas 64,4096 [
        tiles    0, 0
        hud     17, 0
        charset 17,33
    ]
    image (name: material filename: "ega/material.png")
    image (name: tiles filename: "u4/shapes.ega" width: 16 height: 4096 depth: 4 filetype: u4raw tiles: 256 fixup: blackTransparencyHack) [
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
        tile_guard      2
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
        tile_sea_horse  2
        tile_whirlpool  2
        tile_twister    2
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
    image (name: charset filename: "u4/charset.ega" width: 8 height: 2048 depth: 4 filetype: u4raw tiles: 256)
    image (name: borders filename: "u4/start.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: title filename: "u4/title.ega" width: 320 height: 200 depth: 4 filetype: u4lzw fixup: intro) [
        options_top 0, 32, 320,128
        options_btm 0,112, 320, 80
    ]
    image (name: tree filename: "u4/tree.ega" width: 320 height: 200 depth: 4 filetype: u4lzw) [
        moongate  0,152, 24,24
        items    24,152, 24,24
    ]
    image (name: portal filename: "u4/portal.ega" width: 320 height: 200 depth: 4 filetype: u4lzw)
    image (name: outside filename: "u4/outside.ega" width: 320 height: 200 depth: 4 filetype: u4lzw)
    image (name: inside filename: "u4/inside.ega" width: 320 height: 200 depth: 4 filetype: u4lzw)
    image (name: wagon filename: "u4/wagon.ega" width: 320 height: 200 depth: 4 filetype: u4lzw)
    image (name: gypsy filename: "u4/gypsy.ega" width: 320 height: 200 depth: 4 filetype: u4lzw)
    image (name: abacus filename: "u4/abacus.ega" width: 320 height: 200 depth: 4 filetype: u4lzw fixup: abacus) [
        whitebead  8,187, 8,12
        blackbead 24,187, 8,12
    ]
    image (name: honcom filename: "u4/honcom.ega" width: 320 height: 200 depth: 4 filetype: u4lzw) [
        honestycard     12,12, 90,124
        compassioncard 218,12, 90,124
    ]
    image (name: valjus filename: "u4/valjus.ega" width: 320 height: 200 depth: 4 filetype: u4lzw) [
        valorcard    12,12, 90,124
        justicecard 218,12, 90,124
    ]
    image (name: sachonor filename: "u4/sachonor.ega" width: 320 height: 200 depth: 4 filetype: u4lzw) [
        sacrificecard   12,12, 90,124
        honorcard      218,12, 90,124
    ]
    image (name: spirhum filename: "u4/spirhum.ega" width: 320 height: 200 depth: 4 filetype: u4lzw) [
        spiritualitycard   12,12, 90,124
        humilitycard      218,12, 90,124
    ]
    image (name: beasties filename: "u4/animate.ega" width: 320 height: 200 depth: 4 filetype: u4lzw) [
        beast0frame00   0,  0, 55,31
        beast0frame01   0, 32, 55,31
        beast0frame02   0, 64, 55,31
        beast0frame03   0, 96, 55,31
        beast0frame04   0,128, 55,31
        beast0frame05   0,160, 55,31
        beast0frame06  56,  0, 55,31
        beast0frame07  56, 32, 55,31
        beast0frame08  56, 64, 55,31
        beast0frame09  56, 96, 55,31
        beast0frame10  56,128, 55,31
        beast0frame11  56,160, 55,31
        beast0frame12 112,  0, 55,31
        beast0frame13 112, 32, 55,31
        beast0frame14 112, 64, 55,31
        beast0frame15 112, 96, 55,31
        beast0frame16 112,128, 55,31
        beast0frame17 112,160, 55,31
        beast1frame00 176,  0, 48,31
        beast1frame01 176, 32, 48,31
        beast1frame02 176, 64, 48,31
        beast1frame03 176, 96, 48,31
        beast1frame04 176,128, 48,31
        beast1frame05 176,160, 48,31
        beast1frame06 224,  0, 48,31
        beast1frame07 224, 32, 48,31
        beast1frame08 224, 64, 48,31
        beast1frame09 224, 96, 48,31
        beast1frame10 224,128, 48,31
        beast1frame11 224,160, 48,31
        beast1frame12 272,  0, 48,31
        beast1frame13 272, 32, 48,31
        beast1frame14 272, 64, 48,31
        beast1frame15 272, 96, 48,31
        beast1frame16 272,128, 48,31
        beast1frame17 272,160, 48,31
    ]
    image (name: key filename: "u4/key7.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: honesty filename: "u4/honesty.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: compassn filename: "u4/compassn.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: valor filename: "u4/valor.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: justice filename: "u4/justice.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: sacrific filename: "u4/sacrific.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: honor filename: "u4/honor.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: spirit filename: "u4/spirit.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: humility filename: "u4/humility.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: truth filename: "u4/truth.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: love filename: "u4/love.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: courage filename: "u4/courage.ega" width: 320 height: 200 depth: 4 filetype: u4rle transparentIndex: 0)
    image (name: stoncrcl filename: "u4/stoncrcl.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune0 filename: "u4/rune_5.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune1 filename: "u4/rune_1.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune2 filename: "u4/rune_2.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune3 filename: "u4/rune_0.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune4 filename: "u4/rune_1.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune5 filename: "u4/rune_2.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune6 filename: "u4/rune_1.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune7 filename: "u4/rune_3.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: rune8 filename: "u4/rune_4.ega" width: 320 height: 200 depth: 4 filetype: u4rle)
    image (name: hud filename: "ega/hud.png") [
        at 0,0 size 16,16
        reticle
        cross
    ]
    image (name: gemtiles filename: "ega/gem.png" tiles: 128)
    image (name: dungew filename: "ega/dungeonhall.png") [
        dung0_lft_ew    0, 0, 32,176
        dung0_rgt_ew  144, 0, 32,176
        dung1_lft_ew   32,32, 32,112
        dung1_rgt_ew  112,32, 32,112
        dung2_lft_ew   64,64, 16, 48
        dung2_rgt_ew   96,64, 16, 48
        dung3_lft_ew   80,80,  8, 16
        dung3_rgt_ew   88,80,  8, 16
    ]
    image (name: dungns filename: "ega/dungeonhall.png" fixup: dungns) [
        dung0_lft_ns    0, 0, 32,176
        dung0_rgt_ns  144, 0, 32,176
        dung1_lft_ns   32,32, 32,112
        dung1_rgt_ns  112,32, 32,112
        dung2_lft_ns   64,64, 16, 48
        dung2_rgt_ns   96,64, 16, 48
        dung3_lft_ns   80,80,  8, 16
        dung3_rgt_ns   88,80,  8, 16
    ]
    image (name: dungew_door filename: "ega/dungeonhall_door.png") [
        dung0_lft_ew_door    0, 0, 32,176
        dung0_rgt_ew_door  144, 0, 32,176
        dung1_lft_ew_door   32,32, 32,112
        dung1_rgt_ew_door  112,32, 32,112
        dung2_lft_ew_door   64,64, 16, 48
        dung2_rgt_ew_door   96,64, 16, 48
        dung3_lft_ew_door   80,80,  8, 16
        dung3_rgt_ew_door   88,80,  8, 16
    ]
    image (name: dungns_door filename: "ega/dungeonhall_door.png" fixup: dungns) [
        dung0_lft_ns_door   0, 0, 32,176
        dung0_rgt_ns_door 144, 0, 32,176
        dung1_lft_ns_door  32,32, 32,112
        dung1_rgt_ns_door 112,32, 32,112
        dung2_lft_ns_door  64,64, 16, 48
        dung2_rgt_ns_door  96,64, 16, 48
        dung3_lft_ns_door  80,80,  8, 16
        dung3_rgt_ns_door  88,80,  8, 16
     ]
    image (name: dungbackew filename: "ega/dung0ma.png") [
        dung0_mid_ew  0, 0, 176,176
        dung1_mid_ew  0,32, 176,112
        dung1_xxx_ew  0, 0,  32,112
        dung2_mid_ew  0,64, 176, 48
        dung2_xxx_ew  0, 0,  64, 48
        dung3_mid_ew  0,80, 176, 16
        dung3_xxx_ew  0, 0,  80, 16
    ]
    image (name: dungbackns filename: "ega/dung0ma.png" fixup: dungns) [
        dung0_mid_ns  0, 0, 176,176
        dung1_mid_ns  0,32, 176,112
        dung1_xxx_ns  0, 0,  32,112
        dung2_mid_ns  0,64, 176, 48
        dung2_xxx_ns  0, 0,  64, 48
        dung3_mid_ns  0,80, 176, 16
        dung3_xxx_ns  0, 0,  80, 16
    ]
    image (name: dung0_mid_ew_door filename: "ega/dung0ma_door.png")
    image (name: dung0_mid_ns_door filename: "ega/dung0ma_door.png" fixup: dungns)
    image (name: dung1_mid_ew_door_image filename: "ega/dung1ma_door.png") [
        dung1_mid_ew_door  0,32, 176,112
    ]
    image (name: dung1_mid_ns_door_image filename: "ega/dung1ma_door.png" fixup: dungns) [
        dung1_mid_ns_door  0,32, 176,112
    ]
    image (name: dung2_mid_ew_door_image filename: "ega/dung2ma_door.png") [
        dung2_mid_ew_door  0,64, 176,48
    ]
    image (name: dung2_mid_ns_door_image filename: "ega/dung2ma_door.png" fixup: dungns) [
        dung2_mid_ns_door  0,64, 176,48
    ]
    image (name: dung3_mid_ew_door_image filename: "ega/dung3ma_door.png") [
        dung3_mid_ew_door  0,80, 176,16
    ]
    image (name: dung3_mid_ns_door_image filename: "ega/dung3ma_door.png" fixup: dungns) [
        dung3_mid_ns_door  0,80, 176,16
    ]
    image (name: dung0_ladderup_image filename: "ega/ladderup0.png") [
        dung0_ladderup  45,0, 88,87
    ]
    image (name: dung0_ladderup_side_image filename: "ega/ladderup0_side.png") [
        dung0_ladderup_side  45,0, 88,87
    ]
    image (name: dung1_ladderup_image filename: "ega/ladderup1.png") [
        dung1_ladderup  64,40, 50,48
    ]
    image (name: dung1_ladderup_side_image filename: "ega/ladderup1_side.png") [
        dung1_ladderup_side  64,40, 50,48
    ]
    image (name: dung2_ladderup_image filename: "ega/ladderup2.png") [
        dung2_ladderup  77,68, 22,19
    ]
    image (name: dung2_ladderup_side_image filename: "ega/ladderup2_side.png") [
        dung2_ladderup_side  77,68, 22,19
    ]
    image (name: dung3_ladderup_image filename: "ega/ladderup3.png") [
        dung3_ladderup  84,82, 8,6
    ]
    image (name: dung3_ladderup_side_image filename: "ega/ladderup3_side.png") [
        dung3_ladderup_side  84,82, 8,6
    ]
    image (name: dung0_ladderdown_image filename: "ega/ladderdown0.png") [
        dung0_ladderdown  45,87, 88,89
    ]
    image (name: dung0_ladderdown_side_image filename: "ega/ladderdown0_side.png") [
        dung0_ladderdown_side  45,87, 88,89
    ]
    image (name: dung1_ladderdown_image filename: "ega/ladderdown1.png") [
        dung1_ladderdown  64,86, 50,50
    ]
    image (name: dung1_ladderdown_side_image filename: "ega/ladderdown1_side.png") [
        dung1_ladderdown_side  64,86, 50,50
    ]
    image (name: dung2_ladderdown_image filename: "ega/ladderdown2.png") [
        dung2_ladderdown  77,86, 22,22
    ]
    image (name: dung2_ladderdown_side_image filename: "ega/ladderdown2_side.png") [
        dung2_ladderdown_side  77,86, 22,22
    ]
    image (name: dung3_ladderdown_image filename: "ega/ladderdown3.png") [
        dung3_ladderdown  84,88, 8,8
    ]
    image (name: dung3_ladderdown_side_image filename: "ega/ladderdown3_side.png") [
        dung3_ladderdown_side  84,88, 8,8
    ]
    image (name: dung0_ladderupdown_image filename: "ega/ladderupdown0.png") [
        dung0_ladderupdown  45,0, 88,176
    ]
    image (name: dung0_ladderupdown_side_image filename: "ega/ladderupdown0_side.png") [
        dung0_ladderupdown_side  45,0, 88,176
    ]
    image (name: dung1_ladderupdown_image filename: "ega/ladderupdown1.png") [
        dung1_ladderupdown  64,40, 50,96
    ]
    image (name: dung1_ladderupdown_side_image filename: "ega/ladderupdown1_side.png") [
        dung1_ladderupdown_side  64,40, 50,96
    ]
    image (name: dung2_ladderupdown_image filename: "ega/ladderupdown2.png") [
        dung2_ladderupdown  77,68, 22,40
    ]
    image (name: dung2_ladderupdown_side_image filename: "ega/ladderupdown2_side.png") [
        dung2_ladderupdown_side  77,68, 22,40
    ]
    image (name: dung3_ladderupdown_image filename: "ega/ladderupdown3.png") [
        dung3_ladderupdown  84,82, 8,12
    ]
    image (name: dung3_ladderupdown_side_image filename: "ega/ladderupdown3_side.png") [
        dung3_ladderupdown_side  84,82, 8,12
    ]
    image (name: dung_traps filename: "ega/traps.png") [
        dung0_hole  45,  0, 87,17
        dung1_hole  63, 40, 49,17
        dung2_hole  77, 68, 22, 9
        dung0_pit   45,159, 87,17
        dung1_pit   63,119, 49,17
        dung2_pit   77, 99, 22, 9
    ]
]

imageset EGA/VGA [
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
    image (name: tree filename: "vutne/145-tree.png") [
        moongate  0,152, 20,24
        items    24,152, 20,24
    ]
    image (name: portal  filename: "vutne/146-portal.png")
    image (name: outside filename: "vutne/147-outside.png")
    image (name: inside  filename: "vutne/148-inside.png")
    image (name: wagon   filename: "vutne/149-wagon.png")
    image (name: gypsy   filename: "vutne/14a-gypsy.png")
    image (name: abacus  filename: "vutne/14b-abacus.png")
    image (name: abacus-beads filename: "u4/abacus.ega" width: 320 height: 200 depth: 4 filetype: u4lzw fixup: abacus) [
        whitebead  8,187, 8,12
        blackbead 24,187, 8,12
    ]
    image (name: cards1 filename: "vutne/cards1.png") [
        honestycard         0,0, 80,112
        compassioncard     80,0, 80,112
        valorcard         160,0, 80,112
        justicecard       240,0, 80,112
    ]
    image (name: cards2 filename: "vutne/cards2.png") [
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
    image (name: rune0 filename: "u4u/rune_5.old" width: 320 height: 400 depth: 8 filetype: u4rle)
    image (name: rune1 filename: "u4u/rune_1.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune2 filename: "u4u/rune_2.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune3 filename: "u4u/rune_3.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune4 filename: "u4u/rune_4.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune5 filename: "u4u/rune_5.old" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune6 filename: "u4u/rune_6.ega" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune7 filename: "u4u/rune_7.ega" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: rune8 filename: "u4u/rune_8.ega" width: 320 height: 200 depth: 8 filetype: u4rle)
    image (name: gemtiles filename: "vga/gem.png" tiles: 128)
    image (name: dung filename: "vga/dungeonhall.png") [
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
    image (name: dung_door filename: "vga/dungeonhall_door.png") [
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
    image (name: dungback0 filename: "vga/dungeonback0.png") [
        dung0_mid_ew  0,0, 176,176
        dung0_mid_ns  0,0, 176,176
    ]
    image (name: dungback1 filename: "vga/dungeonback1.png") [
        dung1_mid_ew  0,0, 176,176
        dung1_xxx_ew  0,8,  16,160
        dung1_mid_ns  0,0, 176,176
        dung1_xxx_ns  0,8,  16,160
    ]
    image (name: dungback2 filename: "vga/dungeonback2.png") [
        dung2_mid_ew  16,48, 144,80
        dung2_xxx_ew  16,48,  48,80
        dung2_mid_ns  16,48, 144,80
        dung2_xxx_ns  16,48,  48,80
    ]
    image (name: dungback3 filename: "vga/dungeonback3.png") [
        dung3_mid_ew  56,72, 48,32
        dung3_xxx_ew  48,72, 24,32
        dung3_mid_ns  56,72, 48,32
        dung3_xxx_ns  48,72, 24,32
    ]
    image (name: dung0_mid_ew_door filename: "vga/dungeonback0_door.png")
    image (name: dung0_mid_ns_door filename: "vga/dungeonback0_door.png")
    image (name: dung1_mid_door_image filename: "vga/dungeonback1_door.png") [
        dung1_mid_ew_door  0,16, 176,160
        dung1_mid_ns_door  0,16, 176,160
    ]
    image (name: dung2_mid_door_image filename: "vga/dungeonback2_door.png") [
        dung2_mid_ew_door 16,48, 144,80
        dung2_mid_ns_door 16,48, 144,80
    ]
    image (name: dung3_mid_door_image filename: "vga/dungeonback3_door.png") [
        dung3_mid_ew_door 48,72, 80,32
        dung3_mid_ns_door 48,72, 80,32
    ]
    image (name: dung0_ladderup_image filename: "vga/ladderup0.png") [
        dung0_ladderup 45,0, 88,87
    ]
    image (name: dung0_ladderup_side_image filename: "vga/ladderup0_side.png") [
        dung0_ladderup_side 45,0, 88,87
    ]
    image (name: dung1_ladderup_image filename: "vga/ladderup1.png") [
        dung1_ladderup 64,40, 50,48
    ]
    image (name: dung1_ladderup_side_image filename: "vga/ladderup1_side.png") [
        dung1_ladderup_side 64,40, 50,48
    ]
    image (name: dung2_ladderup_image filename: "vga/ladderup2.png") [
        dung2_ladderup 77,68, 22,19
    ]
    image (name: dung2_ladderup_side_image filename: "vga/ladderup2_side.png") [
        dung2_ladderup_side 77,68, 22,19
    ]
    image (name: dung3_ladderup_image filename: "vga/ladderup3.png") [
        dung3_ladderup  84,82, 8,6
    ]
    image (name: dung3_ladderup_side_image filename: "vga/ladderup3_side.png") [
        dung3_ladderup_side  84,82, 8,6
    ]
    image (name: dung0_ladderdown_image filename: "vga/ladderdown0.png") [
        dung0_ladderdown  45,87, 88,89
    ]
    image (name: dung0_ladderdown_side_image filename: "vga/ladderdown0_side.png") [
        dung0_ladderdown_side  45,87, 88,89
    ]
    image (name: dung1_ladderdown_image filename: "vga/ladderdown1.png") [
        dung1_ladderdown  64,86, 50,50
    ]
    image (name: dung1_ladderdown_side_image filename: "vga/ladderdown1_side.png") [
        dung1_ladderdown_side  64,86, 50,50
    ]
    image (name: dung2_ladderdown_image filename: "vga/ladderdown2.png") [
        dung2_ladderdown  77,86, 22,22
    ]
    image (name: dung2_ladderdown_side_image filename: "vga/ladderdown2_side.png") [
        dung2_ladderdown_side  77,86, 22,22
    ]
    image (name: dung3_ladderdown_image filename: "vga/ladderdown3.png") [
        dung3_ladderdown  84,88, 8,8
    ]
    image (name: dung3_ladderdown_side_image filename: "vga/ladderdown3_side.png") [
        dung3_ladderdown_side  84,88, 8,8
    ]
    image (name: dung0_ladderupdown_image filename: "vga/ladderupdown0.png") [
        dung0_ladderupdown  45,0, 88,176
    ]
    image (name: dung0_ladderupdown_side_image filename: "vga/ladderupdown0_side.png") [
        dung0_ladderupdown_side  45,0, 88,176
    ]
    image (name: dung1_ladderupdown_image filename: "vga/ladderupdown1.png") [
        dung1_ladderupdown  64,40, 50,96
    ]
    image (name: dung1_ladderupdown_side_image filename: "vga/ladderupdown1_side.png") [
        dung1_ladderupdown_side  64,40, 50,96
    ]
    image (name: dung2_ladderupdown_image filename: "vga/ladderupdown2.png") [
        dung2_ladderupdown  77,68, 22,40
    ]
    image (name: dung2_ladderupdown_side_image filename: "vga/ladderupdown2_side.png") [
        dung2_ladderupdown_side  77,68, 22,40
    ]
    image (name: dung3_ladderupdown_image filename: "vga/ladderupdown3.png") [
        dung3_ladderupdown  84,82, 8,12
    ]
    image (name: dung3_ladderupdown_side_image filename: "vga/ladderupdown3_side.png") [
        dung3_ladderupdown_side  84,82, 8,12
    ]
    image (name: dung_traps filename: "vga/traps.png") [
        dung0_hole  38,  0,100,10
        dung1_hole  54, 26, 68,13
        dung2_hole  71, 57, 34, 7
        dung0_pit   38,166,100,10
        dung1_pit   54,137, 68,13
        dung2_pit   71,112, 34, 7
    ]
]

tileanimset EGA [
    scroll:     [scroll 0]
    scroll_pool:[scroll 1,0,0,2]    ; 2=tile_shallows
    frame:      [random 50 frame]
    slow_frame: [random  5 frame]
    cityflag:   [random 50 invert 5,2,4,4]
    lcbflag:    [random 50 invert 4,0,4,4]
    castleflag: [random 50 invert 4,0,4,4]
    shipflag: [
        random 50
        invert 6,1,3,4 context-frame 0
        invert 0,0,0,0 context-frame 1
        invert 7,1,3,4 context-frame 2
        invert 0,0,0,0 context-frame 3
    ]
    pirateflag: [random 50 invert 0,1,16,2]
    campfire: [
        pixel_color 3,5,7,11 [
              0, 0, 0
            180, 0, 0
        ]
        pixel_color 4,8,5,6 [
             97,  0, 0
            256,200, 0
        ]
    ]
    phantom_flicker: [frame random 50 scroll 1]
]

tileanimset VGA [
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
            256,200, 0
        ]
        pixel_color 4,7,5,7 [
              0, 0, 0
            156, 0, 0
        ]
    ]
    phantom_flicker: [frame random 50 scroll 1]
]

]
