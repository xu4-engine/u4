module [
    author: "xu4 developers"
    about: {{
        Ultima IV with original graphics.
    }}
    version: "1.0-beta"
    rules: 'Ultima-IV
]

armors: [
    ; Name      Defense  Special
    "Skin"         96
    "Cloth"       128
    "Leather"     144   not  [mage]
    "Chain Mail"  160   only [fighter tinker paladin]
    "Plate Mail"  176   only [fighter tinker paladin]
    "Magic Chain" 192   only [fighter paladin]
    "Magic Plate" 208   only [paladin]
    "Mystic Robe" 248
]

weapons: [
    ; Abbr  Name     Range  Damage  Special
    "HND" "Hands"        1     8
    "STF" "Staff"        1    16
    "DAG" "Dagger"      10    24    losewhenranged
    "SLN" "Sling"       10    32
    "MAC" "Mace"         1    40    not [mage]
    "AXE" "Axe"          1    48    not [mage druid]
    "SWD" "Sword"        1    64    not [mage druid]
    "BOW" "Bow"         10    40    not [mage shepherd]
    "XBO" "Crossbow"    10    56    not [mage shepherd]
    "OIL" "Flaming Oil"  9    64    lose choosedistance
                                        leavetile: fire_field
    "HAL" "Halberd"      2    96    absolute_range attackthroughobjects
                                        dontshowtravel
                                        only [fighter tinker paladin]
    "+AX" "Magic Axe"   10    96    magic returns only [tinker paladin]
    "+SW" "Magic Sword"  1   128    magic only [fighter tinker paladin ranger]
    "+BO" "Magic Bow"   10    80    magic not  [mage fighter shepherd]
    "WND" "Magic Wand"  10   160    magic only [mage bard druid]
                                        hittile: magic_flash
                                        misstile: magic_flash
   "^^SW" "Mystic Sword" 1   255    magic
]

creatures: [
    (id: 0 name: "Horse" tile: horse basehp: 255 exp: 10 good: true wontattack: true)
    (id: 1 name: "Horse" tile: horse basehp: 255 exp: 10 good: true wontattack: true)

    ; Classes
    (id: 2 name: "Mage" tile: mage basehp: 112 exp: 8 ranged: true rangedhittile: magic_flash rangedmisstile: magic_flash good: true wontattack: true)
    (id: 3 name: "Bard" tile: bard basehp: 48 exp: 7 good: true wontattack: true)
    (id: 4 name: "Fighter" tile: fighter basehp: 96 exp: 7 good: true wontattack: true)
    (id: 5 name: "Druid" tile: druid basehp: 64 exp: 10 good: true wontattack: true)
    (id: 6 name: "Tinker" tile: tinker basehp: 96 exp: 9 good: true wontattack: true)
    (id: 7 name: "Paladin" tile: paladin basehp: 128 exp: 4 good: true wontattack: true)
    (id: 8 name: "Ranger" tile: ranger basehp: 144 exp: 3 good: true wontattack: true)
    (id: 9 name: "Shepherd" tile: shepherd basehp: 48 exp: 9 good: true wontattack: true)

    ; Misc town creatures
    (id: 10 name: "Guard" tile: guard basehp: 128 exp: 13 good: true wontattack: true)
    (id: 11 name: "Merchant" tile: villager basehp: 48 exp: 9 good: true wontattack: true)
    (id: 12 name: "Bard" tile: bard_singing basehp: 48 exp: 9 good: true wontattack: true)
    (id: 13 name: "Jester" tile: jester basehp: 48 exp: 9 good: true wontattack: true)
    (id: 14 name: "Beggar" tile: beggar basehp: 32 exp: 13 good: true wontattack: true)
    (id: 15 name: "Child" tile: child basehp: 32 exp: 10 good: true wontattack: true)
    (id: 16 name: "Bull" tile: bull basehp: 128 exp: 11 good: true)
    (id: 17 name: "Lord British" tile: lord_british basehp: 255 exp: 16 ranged: true rangedhittile: magic_flash rangedmisstile: magic_flash good: true wontattack: true)

    ; Water creatures
    (id: 18 encounterSize: 1 name: "Pirate Ship" tile: pirate_ship exp: 16 sails: true)
    (id: 19 encounterSize: 12 name: "Nixie" tile: nixie basehp: 64 exp: 5 ranged: true swims: true leader: 22)
    (id: 20 encounterSize: 4 name: "Giant Squid" tile: giant_squid basehp: 96 exp: 7 ranged: true rangedhittile: energy_field rangedmisstile: energy_field poisons: true swims: true leader: 21)
    (id: 21 encounterSize: 4 name: "Sea Serpent" tile: sea_serpent basehp: 128 exp: 9 ranged: true worldrangedtile: hit_flash rangedhittile: hit_flash rangedmisstile: hit_flash swims: true leader: 20)
    (id: 22 encounterSize: 8 name: "Seahorse" tile: sea_horse basehp: 128 exp: 9 movement: wanders ranged: true rangedhittile: magic_flash rangedmisstile: magic_flash swims: true good: true leader: 19)

    ; Forces of nature
    (id: 23 encounterSize: 1 name: "Whirlpool" forceOfNature: true tile: whirlpool basehp: 255 exp: 16 swims: true movement: wanders cantattack: true canMoveOntoCreatures: true canMoveOntoAvatar: true wontattack: true)
    (id: 24 encounterSize: 1 name: "Twister" forceOfNature: true tile: twister basehp: 255 exp: 16 flies: true movement: wanders cantattack: true canMoveOntoCreatures: true canMoveOntoAvatar: true wontattack: true)

    ; Main creatures
    (id: 25 encounterSize: 12 name: "Rat" tile: rat basehp: 48 exp: 4 movement: wanders good: true leader: 38)
    (id: 26 encounterSize: 12 name: "Bat" tile: bat basehp: 48 exp: 4 flies: true movement: wanders good: true leader: 47 nochest: true)
    (id: 27 encounterSize: 6 name: "Giant Spider" tile: spider basehp: 64 exp: 5 ranged: true rangedhittile: poison_field rangedmisstile: poison_field spawntile: swamp movement: wanders good: true poisons: true leader: 25)
    (id: 28 encounterSize: 4 name: "Ghost" tile: ghost basehp: 80 exp: 6 undead: true resists: sleep incorporeal: true leader: 46)
    (id: 29 encounterSize: 15 name: "Slime" tile: slime ambushes: true basehp: 48 exp: 4 divides: true nochest: true)
    (id: 30 encounterSize: 6 name: "Troll" tile: troll ambushes: true basehp: 96 exp: 7 ranged: true leader: 41)
    (id: 31 encounterSize: 15 name: "Gremlin" tile: gremlin basehp: 48 exp: 4 steals: food)
    (id: 32 encounterSize: 1 name: "Mimic" tile: mimic basehp: 192 exp: 13 ranged: true rangedhittile: poison_field rangedmisstile: poison_field movement: none camouflage: true camouflageTile: chest)
    (id: 33 encounterSize: 1 name: "Reaper" tile: reaper basehp: 255 exp: 16 ranged: true rangedhittile: random rangedmisstile: random movement: none casts: sleep)
    (id: 34 encounterSize: 15 name: "Insect Swarm" tile: insect_swarm ambushes: true basehp: 48 exp: 4 good: true movement: wanders leader: 25 nochest: true)
    (id: 35 encounterSize: 4 name: "Gazer" tile: gazer basehp: 240 exp: 16 ranged: true rangedhittile: sleep_field rangedmisstile: sleep_field leader: 36 spawnsOnDeath: 34)
    (id: 36 encounterSize: 8 name: "Phantom" tile: phantom basehp: 128 exp: 9 undead: true resists: sleep leader: 28)
    (id: 37 encounterSize: 10 name: "Orc" tile: orc ambushes: true basehp: 80 exp: 6 leader: 30)
    (id: 38 encounterSize: 12 name: "Skeleton" tile: skeleton ambushes: true basehp: 48 exp: 4 undead: true resists: sleep leader: 45)
    (id: 39 encounterSize: 10 name: "Rogue" tile: rogue ambushes: true basehp: 80 exp: 6 steals: gold)
    (id: 40 encounterSize: 12 name: "Python" tile: python basehp: 48 ambushes: true exp: 4 ranged: true rangedhittile: poison_field rangedmisstile: poison_field poisons: true good: true movement: wanders leader: 25)
    (id: 41 encounterSize: 6 name: "Ettin" tile: ettin basehp: 112 exp: 8 ranged: true rangedhittile: rocks rangedmisstile: rocks leader: 49)
    (id: 42 encounterSize: 8 name: "Headless" tile: headless basehp: 64 exp: 5 leader: 35)
    (id: 43 encounterSize: 6 name: "Cyclops" tile: cyclops basehp: 128 exp: 9 ranged: true rangedhittile: rocks rangedmisstile: rocks leader: 48)
    (id: 44 encounterSize: 12 name: "Wisp" tile: wisp ambushes: true basehp: 64 exp: 5 teleports: true leader: 36 nochest: true)
    (id: 45 encounterSize: 6 name: "Mage" tile: evil_mage basehp: 176 exp: 12 ranged: true rangedhittile: magic_flash rangedmisstile: magic_flash leader: 49)
    (id: 46 encounterSize: 4 name: "Liche" tile: liche basehp: 192 exp: 13 ranged: true rangedhittile: magic_flash rangedmisstile: magic_flash undead: true resists: sleep leader: 49)
    (id: 47 encounterSize: 8 name: "Lava Lizard" tile: lava_lizard basehp: 96 exp: 7 ranged: true worldrangedtile: hit_flash rangedhittile: lava rangedmisstile: lava resists: fire leader: 50 leavestile: true)
    (id: 48 encounterSize: 4 name: "Zorn" tile: zorn basehp: 240 exp: 16 casts: negate incorporeal: true leader: 35)
    (id: 49 encounterSize: 6 name: "Daemon" tile: daemon basehp: 112 exp: 8 ranged: true rangedhittile: magic_flash rangedmisstile: magic_flash flies: true resists: fire leader: 52)
    (id: 50 encounterSize: 4 name: "Hydra" tile: hydra basehp: 208 exp: 14 ranged: true worldrangedtile: hit_flash rangedhittile: hit_flash rangedmisstile: hit_flash resists: fire leader: 51)
    (id: 51 encounterSize: 4 name: "Dragon" tile: dragon basehp: 224 exp: 15 ranged: true worldrangedtile: hit_flash rangedhittile: hit_flash rangedmisstile: hit_flash flies: true resists: fire leader: 52)
    (id: 52 encounterSize: 1 name: "Balron" tile: balron basehp: 255 exp: 16 ranged: true rangedhittile: random rangedmisstile: random flies: true casts: sleep resists: fire)

    ; Non-standard creatures
    ; Ankh
    (id: 53 name: "Phantom" tile: phantom basehp: 48 exp: 14 good: true)
    ; Campfire
    (id: 54 name: "Phantom" tile: campfire basehp: 48 exp: 4 good: true)
    ; Wounded Villager
    (id: 55 name: "Villager" tile: corpse basehp: 32 exp: 13 good: true)
    ; Avatar tile
    (id: 56 name: "Adventurer" tile: avatar basehp: 160 exp: 11 good: true)
    ; Water in LBs castle
    (id: 57 name: "Water" tile: shallows basehp: 255 exp: 16 good: true swims: true)
    ; Ankh in Skara Brae
    (id: 58 name: "Ankh" tile: ankh basehp: 255 exp: 16 good: true)
    ; Peculiar Fire field phantoms found in Destard LV4
    (id: 59 name: "Phantom" tile: fire_phantom basehp: 48 exp: 14
     resists: fire canMoveOntoAvatar: true rangedhittile: random
     rangedmisstile: fire_field leader: 45 divides: true u4SaveId: 70)
]

tile-rules: [
    (name: default)
    (name: water cantwalkon: all swimable: true sailable: true onWaterOnlyReplacement: true)
    (name: shallows cantwalkon: all swimable: true onWaterOnlyReplacement: true)
    (name: swamp speed: slow effect: poison)
    (name: grass canlandballoon: true replacement: true)
    (name: brush speed: vslow)
    (name: hills speed: vvslow)
    (name: mountains cantwalkon: all unflyable: true creatureunwalkable: true)
    (name: lcb cantwalkon: all creatureunwalkable: true)
    (name: lcb_entrance cantwalkon: south cantwalkoff: north creatureunwalkable: true)
    (name: ship ship: true creatureunwalkable: true)
    (name: horse horse: true creatureunwalkable: true)
    (name: floors replacement: true)
    (name: balloon balloon: true creatureunwalkable: true)
    (name: person cantwalkon: all livingthing: true)
    (name: solid cantwalkon: all)
    (name: solid_attackover cantwalkon: all canattackover: true)
    (name: walls cantwalkon: all unflyable: true)
    (name: locked_door cantwalkon: all lockeddoor: true)
    (name: door cantwalkon: all door: true)
    (name: secret_door cantwalkon: retreat)
    (name: chest chest: true)
    (name: poison_field effect: poisonfield dispel: true)
    (name: energy_field cantwalkon: all effect: electricity creatureunwalkable: true unflyable: true dispel: true)
    (name: fire_field speed: vvslow effect: fire dispel: true)
    (name: sleep_field effect: sleep dispel: true)
    (name: lava effect: lava replacement: true)
    (name: signs cantwalkon: all talkover: true)
    (name: spacers cantwalkon: all)
    (name: monster cantwalkon: all livingthing: true)
    (name: dng_altar creatureunwalkable: true unflyable: true)
    (name: dng_door cantwalkon: retreat)
    (name: dng_room creatureunwalkable: true cantwalkon: retreat unflyable: true)
]

tileset: [
    (name: sea rule: water animation: scroll)
    (name: water rule: water animation: scroll)
    (name: shallows rule: shallows animation: scroll)
    (name: swamp rule: swamp)
    (name: grass rule: grass)
    (name: brush rule: brush)
    (name: forest rule: brush opaque: round)
    (name: hills rule: hills)
    (name: mountains rule: mountains opaque: round)
    (name: dungeon)
    (name: city animation: cityflag usesReplacementTileAsBackground: true)
    (name: castle animation: castleflag usesReplacementTileAsBackground: true)
    (name: town usesReplacementTileAsBackground: true)
    (name: lcb_west rule: lcb usesReplacementTileAsBackground: true)
    (name: lcb_entrance rule: lcb_entrance animation: lcbflag usesReplacementTileAsBackground: true)
    (name: lcb_east rule: lcb usesReplacementTileAsBackground: true)
    (name: ship  rule: ship  frames: 4 directions: wnes animation: shipflag)
    (name: horse rule: horse frames: 2 directions: we)
    (name: dungeon_floor rule: floors)
    (name: bridge usesWaterReplacementTileAsBackground: true usesReplacementTileAsBackground: true)
    (name: balloon rule: balloon)
    (name: bridge_n usesWaterReplacementTileAsBackground: true usesReplacementTileAsBackground: true)
    (name: bridge_s usesWaterReplacementTileAsBackground: true usesReplacementTileAsBackground: true)
    (name: up_ladder usesReplacementTileAsBackground: true)
    (name: down_ladder usesReplacementTileAsBackground: true)
    (name: ruins usesReplacementTileAsBackground: true)
    (name: shrine usesReplacementTileAsBackground: true)
    (name: avatar rule: person)
    (name: mage rule: person frames: 2 animation: frame)
    (name: bard rule: person frames: 2 animation: frame)
    (name: fighter rule: person frames: 2 animation: frame)
    (name: druid rule: person frames: 2 animation: frame)
    (name: tinker rule: person frames: 2 animation: frame)
    (name: paladin rule: person frames: 2 animation: frame)
    (name: ranger rule: person frames: 2 animation: frame)
    (name: shepherd rule: person frames: 2 animation: frame)
    (name: column rule: solid usesReplacementTileAsBackground: true)
    (name: waterside_sw rule: solid_attackover animation: scroll_pool usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: waterside_se rule: solid_attackover animation: scroll_pool usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: waterside_nw rule: solid_attackover animation: scroll_pool usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: waterside_ne rule: solid_attackover animation: scroll_pool usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: shipmast rule: solid)
    (name: shipwheel rule: solid)
    (name: rocks rule: solid usesReplacementTileAsBackground: true)
    (name: corpse rule: solid usesReplacementTileAsBackground: true)
    (name: stone_wall rule: walls)
    (name: locked_door rule: locked_door)
    (name: door rule: door)
    (name: chest rule: chest usesReplacementTileAsBackground: true)
    (name: ankh rule: solid usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: brick_floor rule: floors)
    (name: wood_floor rule: floors)
    (name: moongate_opening rule: solid frames: 3)
    (name: moongate)
    (name: poison_field rule: poison_field animation: scroll tiledInDungeon: true usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: energy_field rule: energy_field animation: scroll tiledInDungeon: true usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: fire_field rule: fire_field animation: scroll tiledInDungeon: true usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: sleep_field rule: sleep_field animation: scroll tiledInDungeon: true usesReplacementTileAsBackground: true usesWaterReplacementTileAsBackground: true)
    (name: solid rule: solid_attackover)
    (name: secret_door rule: secret_door opaque: square)
    (name: altar usesReplacementTileAsBackground: true)
    (name: campfire rule: solid animation: campfire usesReplacementTileAsBackground: true)
    (name: lava rule: lava animation: scroll)
    (name: miss_flash usesReplacementTileAsBackground: true)
    (name: magic_flash usesReplacementTileAsBackground: true)
    (name: hit_flash usesReplacementTileAsBackground: true)
    (name: guard rule: person frames: 2 animation: slow_frame)
    (name: villager rule: person frames: 2 animation: frame)
    (name: bard_singing rule: person frames: 2 animation: frame)
    (name: jester rule: person frames: 2 animation: frame)
    (name: beggar rule: person frames: 2 animation: frame)
    (name: child rule: person frames: 2 animation: frame)
    (name: bull rule: person frames: 2 animation: frame)
    (name: lord_british rule: person frames: 2 animation: frame)
    (name: A rule: signs)
    (name: B rule: signs)
    (name: C rule: signs)
    (name: D rule: signs)
    (name: E rule: signs)
    (name: F rule: signs)
    (name: G rule: signs)
    (name: H rule: signs)
    (name: I rule: signs)
    (name: J rule: signs)
    (name: K rule: signs)
    (name: L rule: signs)
    (name: M rule: signs)
    (name: N rule: signs)
    (name: O rule: signs)
    (name: P rule: signs)
    (name: Q rule: signs)
    (name: R rule: signs)
    (name: S rule: signs)
    (name: T rule: signs)
    (name: U rule: signs)
    (name: V rule: signs)
    (name: W rule: signs)
    (name: X rule: signs)
    (name: Y rule: signs)
    (name: Z rule: signs)
    (name: space rule: signs)
    (name: space_r rule: spacers)
    (name: space_l rule: spacers)
    (name: window rule: spacers)
    (name: black rule: spacers)
    (name: brick_wall rule: walls opaque: square)
    (name: pirate_ship rule: monster frames: 4 directions: wnes animation: pirateflag)
    (name: nixie rule: monster frames: 2 animation: frame)
    (name: giant_squid rule: monster frames: 2 animation: frame)
    (name: sea_serpent rule: monster frames: 2 animation: frame)
    (name: sea_horse rule: monster frames: 2 animation: frame)
    (name: whirlpool rule: water frames: 2 animation: frame)
    (name: twister frames: 2 animation: frame)
    (name: rat rule: monster frames: 4 animation: frame)
    (name: bat rule: monster frames: 4 animation: frame)
    (name: spider rule: monster frames: 4 animation: frame)
    (name: ghost rule: monster frames: 4 animation: frame)
    (name: slime rule: monster frames: 4 animation: frame)
    (name: troll rule: monster frames: 4 animation: frame)
    (name: gremlin rule: monster frames: 4 animation: frame)
    (name: mimic rule: monster frames: 4 animation: frame)
    (name: reaper rule: monster frames: 4 animation: frame)
    (name: insect_swarm rule: monster frames: 4 animation: frame)
    (name: gazer rule: monster frames: 4 animation: frame)
    (name: phantom rule: monster frames: 4 animation: frame)
    (name: orc rule: monster frames: 4 animation: frame)
    (name: skeleton rule: monster frames: 4 animation: frame)
    (name: rogue rule: monster frames: 4 animation: frame)
    (name: python rule: monster frames: 4 animation: frame)
    (name: ettin rule: monster frames: 4 animation: frame)
    (name: headless rule: monster frames: 4 animation: frame)
    (name: cyclops rule: monster frames: 4 animation: frame)
    (name: wisp rule: monster frames: 4 animation: frame)
    (name: evil_mage rule: monster frames: 4 animation: frame)
    (name: liche rule: monster frames: 4 animation: frame)
    (name: lava_lizard rule: monster frames: 4 animation: frame)
    (name: zorn rule: monster frames: 4 animation: frame)
    (name: daemon rule: monster frames: 4 animation: frame)
    (name: hydra rule: monster frames: 4 animation: frame)
    (name: dragon rule: monster frames: 4 animation: frame)
    (name: balron rule: monster frames: 4 animation: frame)
    (name: up_down_ladder image: tile_bridge)
    (name: ceiling_hole image: tile_brick_floor)
    (name: floor_hole image: tile_brick_floor)
    (name: magic_orb image: tile_magic_flash)
    (name: fountain image: tile_shallows animation: scroll)
    (name: dungeon_room image: tile_solid rule: dng_room)
    (name: dungeon_door image: tile_door rule: dng_door)
    (name: dungeon_altar image: tile_altar rule: dng_altar)
    (name: fire_phantom rule: fire_field frames: 2 animation: phantom_flicker)
]

u4-save-ids: [
    sea
    water
    shallows
    swamp
    grass
    brush
    forest
    hills
    mountains
    dungeon
    city
    castle
    town
    lcb_west
    lcb_entrance
    lcb_east
    4 ship
    2 horse
    dungeon_floor
    bridge
    balloon
    bridge_n
    bridge_s
    up_ladder
    down_ladder
    ruins
    shrine
    avatar
    2 mage
    2 bard
    2 fighter
    2 druid
    2 tinker
    2 paladin
    2 ranger
    2 shepherd
    column
    waterside_sw
    waterside_se
    waterside_nw
    waterside_ne
    shipmast
    shipwheel
    rocks
    corpse
    stone_wall
    locked_door
    door
    chest
    ankh
    brick_floor
    wood_floor
    3 moongate_opening
    moongate
    poison_field
    energy_field
    fire_field
    sleep_field
    solid
    secret_door
    altar
    campfire
    lava
    miss_flash
    magic_flash
    hit_flash
    2 guard
    2 villager
    2 bard_singing
    2 jester
    2 beggar
    2 child
    2 bull
    2 lord_british
    A B C D E F G H I J
    K L M N O P Q R S T
    U V W X Y Z
    space
    space_r
    space_l
    window
    black
    brick_wall
    4 pirate_ship
    2 nixie
    2 giant_squid
    2 sea_serpent
    2 sea_horse
    2 whirlpool
    2 twister
    4 rat
    4 bat
    4 spider
    4 ghost
    4 slime
    4 troll
    4 gremlin
    4 mimic
    4 reaper
    4 insect_swarm
    4 gazer
    4 phantom
    4 orc
    4 skeleton
    4 rogue
    4 python
    4 ettin
    4 headless
    4 cyclops
    4 wisp
    4 evil_mage
    4 liche
    4 lava_lizard
    4 zorn
    4 daemon
    4 hydra
    4 dragon
    4 balron
]

ega-palette: [
      0,   0,   0
      0,   0, 170
      0, 170,   0
      0, 170, 170
    170,   0,   0
    170,   0, 170
    170,  85,   0
    170, 170, 170
     85,  85,  85
     85,  85, 255
     85, 255,  80
     85, 255, 255
    255,  85,  85
    255,  85, 255
    255, 255,  85
    255, 255, 255
]

music: [
    path %music/minstrel
    %wanderer.ogg
    %townes.ogg
    %shrines.ogg
    %merchants.ogg
    %rule_britannia.ogg
    %fanfare.ogg
    %dungeons.ogg
    %combat.ogg
    %castles.ogg
]

sound: [
    path %../../sound
    %title_fade.ogg
    %walk_normal.ogg
    %walk_slowed.ogg
    %walk_combat.ogg
    %blocked_dos.ogg
    %error_dos.ogg
    %pc_attack.ogg
    %pc_struck.ogg
    %npc_attack.ogg
    %npc_struck.ogg
    %enemy_magic_proj_hit.ogg
    %enemy_magic_proj_hit.ogg
    %poison_effect.ogg
    %poison_damage_dos.ogg
    %evade_dos.ogg          ; Evaded
    %evade_dos.ogg          ; Flee
    %evade_dos.ogg          ; Item stolen
    %magic.ogg              ; LB heals
    %reaper_sleeper.ogg     ; Level up
    %elevate.ogg            ; Gain virtue
    %moongate_dos.ogg
    %enemy_magic_proj.ogg
    %fx_tremor.ogg
    %spell_precast_dos.ogg
    %spell_flash_25.ogg
    %fire_field_walking.ogg
    %fire_field_walking.ogg
    %gate_open.ogg
    %stone_falling.ogg
    %wind_gust.ogg
]

layouts: [
    gem         "Standard"      tileshape 4,4 viewport 32,32,32,32
    gem         "Full Viewport" tileshape 4,4 viewport  8, 8,44,44
    dungeon_gem "Standard"      tileshape 8,8 viewport  8, 8,22,22
]

include %maps.b
include %graphics.b
include %vendors.b
