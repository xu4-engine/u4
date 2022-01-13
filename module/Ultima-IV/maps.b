maps: [
    map (id: 0 type: world fname: "world.map" width: 256 height: 256 levels: 1 chunk-dim: 32 borderbehavior: wrap music: 1) [
        portal (x: 86 y: 107 destmapid: 1 startx: 15 starty: 30 action: enter savelocation: true transport: footorhorse)
        portal (x: 218 y: 107 destmapid: 2 startx: 15 starty: 30 action: enter savelocation: true transport: footorhorse)
        portal (x: 28 y: 50 destmapid: 3 startx: 15 starty: 30 action: enter savelocation: true transport: footorhorse)
        portal (x: 146 y: 241 destmapid: 4 startx: 15 starty: 30 action: enter savelocation: true transport: footorhorse)
        portal (x: 232 y: 135 destmapid: 5 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 82 y: 106 destmapid: 6 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 36 y: 222 destmapid: 7 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 58 y: 43 destmapid: 8 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 159 y: 20 destmapid: 9 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 106 y: 184 destmapid: 10 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 22 y: 128 destmapid: 11 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 187 y: 169 destmapid: 12 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 98 y: 145 destmapid: 13 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 136 y: 158 destmapid: 14 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 201 y: 59 destmapid: 15 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 136 y: 90 destmapid: 16 startx: 1 starty: 15 action: enter savelocation: true transport: footorhorse)
        portal (x: 240 y: 73 destmapid: 17 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 91 y: 67 destmapid: 18 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 72 y: 168 destmapid: 19 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 126 y: 20 destmapid: 20 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 156 y: 27 destmapid: 21 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 58 y: 102 destmapid: 22 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 239 y: 240 destmapid: 23 startx: 1 starty: 1 action: enter savelocation: true transport: foot)
        portal (x: 233 y: 233 destmapid: 24 startx: 1 starty: 1 action: enter condition: abyss savelocation: true message: "Enter the Great Stygian Abyss!^/^/" transport: foot)
        portal (x: 233 y: 66 destmapid: 25 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        portal (x: 128 y: 92 destmapid: 26 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        portal (x: 36 y: 229 destmapid: 27 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        portal (x: 73 y: 11 destmapid: 28 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        portal (x: 205 y: 45 destmapid: 29 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        portal (x: 81 y: 207 destmapid: 30 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        portal (x: 231 y: 216 destmapid: 32 startx: 0 starty: 0 action: enter condition: shrine savelocation: true transport: footorhorse)
        moongates [
          ; phase, x, y
            0,224,133
            1, 96,102
            2, 38,224
            3, 50, 37
            4,166, 19
            5,104,194
            6, 23,126
            7,187,167
        ]
        labels [
            balloon     233,242
            lockelake   127, 78
            mandrake1   182, 54
            mandrake2   100,165
            nightshade1  46,149
            nightshade2 205, 44
            bell        176,208
            horn         45,173
            wheel        96,215
            skull       197,245
            blackstone  224,133
            whitestone   64, 80
            lasergun     48, 22
        ]
    ]
    map (id: 1 type: city fname: "lcb_1.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 9) [
        city (name: "Britannia" type: castle tlk_fname: "lcb.tlk") [
            personrole (role: healer id: 29)
            personrole (role: hawkwind id: 30)
        ]
        portal (x: 3 y: 3 destmapid: 56 startx: 3 starty: 3 action: klimb savelocation: false message: "Klimb to second floor!^/" transport: foot)
        portal (x: 27 y: 3 destmapid: 56 startx: 27 starty: 3 action: klimb savelocation: false message: "Klimb to second floor!^/" transport: foot)
        portal (x: 7 y: 2 destmapid: 23 startx: 5 starty: 5 action: descend savelocation: false message: "Descend into the depths!^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        labels [spiritualityrune 17,8]
    ]
    map (id: 56 type: city fname: "lcb_2.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 9) [
        city (name: "Britannia" type: castle tlk_fname: "lcb.tlk") [
            personrole (role: lordbritish id: 32)
        ]
        portal (x: 3 y: 3 destmapid: 1 startx: 3 starty: 3 action: descend savelocation: false message: "Descend to first floor!^/" transport: foot)
        portal (x: 27 y: 3 destmapid: 1 startx: 27 starty: 3 action: descend savelocation: false message: "Descend to first floor!^/" transport: foot)
    ]
    map (id: 2 type: city fname: "lycaeum.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 9) [
        city (name: "Lycaeum" type: castle tlk_fname: "lycaeum.tlk") [
            personrole (role: healer id: 23)
        ]
        labels [
            book 6,6
            telescope 22,3
        ]
    ]
    map (id: 3 type: city fname: "empath.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 9) [
        city (name: "Empath Abbey" type: castle tlk_fname: "empath.tlk") [
            personrole (role: healer id: 30)
        ]
        labels [mysticarmor 22,4]
    ]
    map (id: 4 type: city fname: "serpent.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 9) [
        city (name: "Serpents Hold" type: castle tlk_fname: "serpent.tlk") [
            personrole (role: healer id: 31)
        ]
        labels [mysticswords 8,15]
    ]
    map (id: 5 type: city fname: "moonglow.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Moonglow" type: towne tlk_fname: "moonglow.tlk") [
            personrole (role: companion id: 32)
            personrole (role: foodvendor id: 26)
            personrole (role: reagentsvendor id: 24)
            personrole (role: healer id: 25)
            personrole (role: innkeeper id: 30)
        ]
        labels [honestyrune 8,6]
    ]
    map (id: 6 type: city fname: "britain.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Britain" type: towne tlk_fname: "britain.tlk") [
            personrole (role: companion id: 32)
            personrole (role: weaponsvendor id: 29)
            personrole (role: armorvendor id: 28)
            personrole (role: foodvendor id: 27)
            personrole (role: tavernkeeper id: 26)
            personrole (role: healer id: 31)
            personrole (role: innkeeper id: 25)
        ]
        labels [compassionrune 25,1]
    ]
    map (id: 7 type: city fname: "jhelom.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Jhelom" type: towne tlk_fname: "jhelom.tlk") [
            personrole (role: companion id: 32)
            personrole (role: weaponsvendor id: 29)
            personrole (role: armorvendor id: 28)
            personrole (role: tavernkeeper id: 30)
            personrole (role: healer id: 25)
            personrole (role: healer id: 26)
            personrole (role: healer id: 27)
            personrole (role: innkeeper id: 31)
        ]
        labels [valorrune 30,30]
    ]
    map (id: 8 type: city fname: "yew.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Yew" type: towne tlk_fname: "yew.tlk") [
            personrole (role: companion id: 32)
            personrole (role: foodvendor id: 27)
            personrole (role: healer id: 26)
        ]
        labels [justicerune 13,6]
    ]
    map (id: 9 type: city fname: "minoc.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Minoc" type: towne tlk_fname: "minoc.tlk") [
            personrole (role: companion id: 32)
            personrole (role: weaponsvendor id: 30)
            personrole (role: innkeeper id: 31)
        ]
        labels [sacrificerune 28,30]
    ]
    map (id: 10 type: city fname: "trinsic.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Trinsic" type: towne tlk_fname: "trinsic.tlk") [
            personrole (role: companion id: 32)
            personrole (role: weaponsvendor id: 29)
            personrole (role: armorvendor id: 28)
            personrole (role: tavernkeeper id: 31)
            personrole (role: innkeeper id: 30)
        ]
        labels [honorrune 2,29]
    ]
    map (id: 11 type: city fname: "skara.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Skara Brae" type: towne tlk_fname: "skara.tlk") [
            personrole (role: companion id: 32)
            personrole (role: foodvendor id: 28)
            personrole (role: reagentsvendor id: 30)
            personrole (role: healer id: 31)
            personrole (role: innkeeper id: 29)
        ]
    ]
    map (id: 12 type: city fname: "magincia.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Magincia" type: ruins tlk_fname: "magincia.tlk") [
            personrole (role: companion id: 32)
        ]
    ]
    map (id: 13 type: city fname: "paws.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Paws" type: village tlk_fname: "paws.tlk") [
            personrole (role: armorvendor id: 27)
            personrole (role: foodvendor id: 31)
            personrole (role: tavernkeeper id: 30)
            personrole (role: tavernkeeper id: 29)
            personrole (role: reagentsvendor id: 28)
            personrole (role: horsevendor id: 18)
        ]
        labels [humilityrune 29,29]
    ]
    map (id: 14 type: city fname: "den.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Buccaneers Den" type: village tlk_fname: "den.tlk") [
            personrole (role: weaponsvendor id: 28)
            personrole (role: armorvendor id: 27)
            personrole (role: tavernkeeper id: 26)
            personrole (role: reagentsvendor id: 30)
            personrole (role: guildvendor id: 29)
        ]
    ]
    map (id: 15 type: city fname: "vesper.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Vesper" type: village tlk_fname: "vesper.tlk") [
            personrole (role: weaponsvendor id: 25)
            personrole (role: tavernkeeper id: 23)
            personrole (role: innkeeper id: 26)
            personrole (role: guildvendor id: 24)
        ]
    ]
    map (id: 16 type: city fname: "cove.ult" width: 32 height: 32 levels: 1 borderbehavior: exit music: 2) [
        city (name: "Cove" type: village tlk_fname: "cove.tlk") [
            personrole (role: healer id: 31)
        ]
        labels [candle 22,1]
    ]
    map (id: 17 type: dungeon fname: "deceit.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Deceit" rooms: 16)
        portal (x: 1 y: 1 z: 7 destmapid: 17 startx: 1 starty: 1 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Deceit^/" transport: foot) [
            retroActiveDest (x: 240 y: 73 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 22 startx: 1 starty: 1 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 23 startx: 1 starty: 1 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 20 startx: 1 starty: 1 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        labels [bluestone 1,7,6]
    ]
    map (id: 18 type: dungeon fname: "despise.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Despise" rooms: 16)
        portal (x: 3 y: 3 z: 7 destmapid: 18 startx: 3 starty: 3 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Despise^/" transport: foot) [
            retroActiveDest (x: 91 y: 67 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 20 startx: 3 starty: 3 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 23 startx: 3 starty: 3 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 21 startx: 3 starty: 3 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        labels [yellowstone 3,5,4]
    ]
    map (id: 19 type: dungeon fname: "destard.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Destard" rooms: 16)
        portal (x: 7 y: 7 z: 7 destmapid: 19 startx: 7 starty: 7 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Destard^/" transport: foot) [
            retroActiveDest (x: 72 y: 168 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 21 startx: 7 starty: 7 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 23 startx: 7 starty: 7 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 22 startx: 7 starty: 7 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        labels [redstone 3,7,6]
    ]
    map (id: 20 type: dungeon fname: "wrong.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Wrong" rooms: 16)
        portal (x: 1 y: 1 z: 7 destmapid: 17 startx: 1 starty: 1 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Deceit^/" transport: foot) [
            retroActiveDest (x: 240 y: 73 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 22 startx: 1 starty: 1 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 23 startx: 1 starty: 1 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 20 startx: 1 starty: 1 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 18 startx: 3 starty: 3 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Despise^/" transport: foot) [
            retroActiveDest (x: 91 y: 67 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 20 startx: 3 starty: 3 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 23 startx: 3 starty: 3 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 21 startx: 3 starty: 3 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        labels [greenstone 6,3,7]
    ]
    map (id: 21 type: dungeon fname: "covetous.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Covetous" rooms: 16)
        portal (x: 3 y: 3 z: 7 destmapid: 18 startx: 3 starty: 3 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Despise^/" transport: foot) [
            retroActiveDest (x: 91 y: 67 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 20 startx: 3 starty: 3 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 23 startx: 3 starty: 3 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 21 startx: 3 starty: 3 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 19 startx: 7 starty: 7 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Destard^/" transport: foot) [
            retroActiveDest (x: 72 y: 168 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 21 startx: 7 starty: 7 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 23 startx: 7 starty: 7 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 22 startx: 7 starty: 7 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        labels [orangestone 7,1,6]
    ]
    map (id: 22 type: dungeon fname: "shame.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Shame" rooms: 16)
        portal (x: 1 y: 1 z: 7 destmapid: 17 startx: 1 starty: 1 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Deceit^/" transport: foot) [
            retroActiveDest (x: 240 y: 73 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 22 startx: 1 starty: 1 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 23 startx: 1 starty: 1 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 20 startx: 1 starty: 1 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 19 startx: 7 starty: 7 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Destard^/" transport: foot) [
            retroActiveDest (x: 72 y: 168 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 21 startx: 7 starty: 7 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 23 startx: 7 starty: 7 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 22 startx: 7 starty: 7 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        labels [purplestone 7,7,1]
    ]
    map (id: 23 type: dungeon fname: "hythloth.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "Hythloth" rooms: 16)
        portal (x: 1 y: 1 z: 7 destmapid: 17 startx: 1 starty: 1 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Deceit^/" transport: foot) [
            retroActiveDest (x: 240 y: 73 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 22 startx: 1 starty: 1 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 23 startx: 1 starty: 1 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 1 y: 1 z: 7 destmapid: 20 startx: 1 starty: 1 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 18 startx: 3 starty: 3 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Despise^/" transport: foot) [
            retroActiveDest (x: 91 y: 67 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 20 startx: 3 starty: 3 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Wrong^/" transport: foot) [
            retroActiveDest (x: 126 y: 20 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 23 startx: 3 starty: 3 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 3 y: 3 z: 7 destmapid: 21 startx: 3 starty: 3 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 19 startx: 7 starty: 7 startlevel: 7 action: exit_north savelocation: false message: "into Dungeon Destard^/" transport: foot) [
            retroActiveDest (x: 72 y: 168 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 21 startx: 7 starty: 7 startlevel: 7 action: exit_east savelocation: false message: "into Dungeon Covetous^/" transport: foot) [
            retroActiveDest (x: 156 y: 27 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 23 startx: 7 starty: 7 startlevel: 7 action: exit_south savelocation: false message: "into Dungeon Hythloth^/" transport: foot) [
            retroActiveDest (x: 239 y: 240 mapid: 0)
        ]
        portal (x: 7 y: 7 z: 7 destmapid: 22 startx: 7 starty: 7 startlevel: 7 action: exit_west savelocation: false message: "into Dungeon Shame^/" transport: foot) [
            retroActiveDest (x: 58 y: 102 mapid: 0)
        ]
    ]
    map (id: 24 type: dungeon fname: "abyss.dng" width: 8 height: 8 levels: 8 borderbehavior: wrap firstperson: true music: 7) [
        dungeon (name: "The Great Stygian Abyss" rooms: 64)
    ]
    map (id: 25 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Honesty" mantra: ahm)
    ]
    map (id: 26 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Compassion" mantra: mu)
    ]
    map (id: 27 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Valor" mantra: ra)
    ]
    map (id: 28 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Justice" mantra: beh)
    ]
    map (id: 29 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Sacrifice" mantra: cah)
    ]
    map (id: 30 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Honor" mantra: summ)
    ]
    map (id: 31 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Spirituality" mantra: om)
    ]
    map (id: 32 type: shrine fname: "shrine.con" width: 11 height: 11 levels: 1 borderbehavior: fixed music: 3) [
        shrine (virtue: "Humility" mantra: lum)
    ]
    map (id: 33 type: combat fname: "brick.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 34 type: combat fname: "bridge.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 35 type: combat fname: "brush.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 36 type: combat fname: "camp.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 37 type: combat fname: "dng0.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 38 type: combat fname: "dng1.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 39 type: combat fname: "dng2.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 40 type: combat fname: "dng3.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 41 type: combat fname: "dng4.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 42 type: combat fname: "dng5.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 43 type: combat fname: "dng6.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 44 type: combat fname: "dungeon.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 45 type: combat fname: "forest.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 46 type: combat fname: "grass.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 47 type: combat fname: "hill.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 48 type: combat fname: "inn.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 49 type: combat fname: "marsh.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 50 type: combat fname: "shipsea.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 51 type: combat fname: "shipship.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 52 type: combat fname: "shipshor.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 53 type: combat fname: "shore.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 54 type: combat fname: "shorship.con" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
    map (id: 55 type: combat fname: "camp.dng" width: 11 height: 11 levels: 1 borderbehavior: fixed nolineofsight: true music: 8)
]
