#!/usr/bin/boron -s
; Create xu4 module package.

usage: {{
Usage: pack-xu4 [<OPTIONS>] <module-path>

Options:
  -f           File package mode (skip config).
  -h           Print this help and exit.
  -o <module>  Output module file.
  -v <level>   Set verbose level. (0-2, default is 1)
  --version    Print version and exit.
}}

root-path: %module/Ultima-IV/
module-file: none
file-package: false
verbose: 1

forall args [
	switch first args [
		"-v" [verbose: to-int second ++ args]
		"-f" [file-package: true]
		"-h" [print usage quit]
		"-o" [module-file: to-file second ++ args]
		"--version" [print "pack-xu4 0.1.1" quit]
		[root-path: terminate to-file first args '/']
	]
]

fatal: func ['code msg] [
	print msg
	quit/return select [
		usage	64  ; EX_USAGE
		data	65  ; EX_DATAERR
		noinput	66  ; EX_NOINPUT
		config	78  ; EX_CONFIG
	] code
]

path-slash: charset "/\"
basename: func [path] [
	if pos: find/last path path-slash [return next pos]
	path
]

ifn module-file [
	module-file: join last split root-path path-slash
					either file-package %.pak %.mod
]
if gt? size? basename module-file 39 [
	fatal config "Module filename must be less than 40 characters"
]

enum-value: func [list val] [
	either pos: find list val [sub index? pos 1] 0
]

file-ext: func [file] [find/last file '.']

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
	none	0x01
	west	0x02
	north	0x04
	east	0x08
	south	0x10
	advance	0x20
	retreat	0x40
	all		0x7e
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
; TMX Map

tmx-fatal: func [file msg] [fatal data ["TMX" file '-' msg]]

load-tmx: func [file /extern width height data /local it] [
	w: csv: none

	; See https://doc.mapeditor.org/en/stable/reference/tmx-map-format/
	parse read/text file [
		thru "<map "
		thru "<layer " thru {width="} w: thru {height="} h:
		thru "<data " thru {encoding="} enc: thru '>'
		csv: to "</data" :csv (
			w: to-int w
			h: to-int h
			enc: slice enc 3
		)
	]

	case [
		none? w			[tmx-fatal file "no map layer found"]
		none? csv		[tmx-fatal file "no map data found"]
		ne? enc "csv"	[tmx-fatal file "data encoding is not CSV"]
		not zero? and w 31 [tmx-fatal file "width is not a multiple of 32"]
		not zero? and h 31 [tmx-fatal file "height is not a multiple of 32"]
	]

	context [
		width: w
		height: h
		data: to-block construct csv [',' ' ']
		map it data [sub it 1]		; Convert indices to zero base.
	]
]

; Return data as a series of chunks.
; Chunk-dim is the tile width & height of a single chunk.
map-chunks: func [tile-w tile-h data chunk-dim] [
	cdata: make binary! size? data
	chunks-y: div tile-h chunk-dim
	loop div tile-w chunk-dim [
		x: 0
		loop chunks-y [
			in-row: skip data mul ++ x chunk-dim
			loop chunk-dim [
				append cdata slice in-row chunk-dim
				in-row: skip in-row tile-w
			]
		]
		data: skip data mul chunk-dim tile-w
	]
	cdata
]

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
		appair store str '^0'
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
tmx_id: "MA^0^0"
npc_id: "NC^0^0"
file_buf: make binary! 4096
module-layer: 0

poke-id: func [id n] [
	poke id 3 div n 256
	poke id 4 and n 255
]

; Return app_id of PNG chunk.
pack-png: func [path filename] [
	poke-id img_id add file-id filename module-layer

	ifn file-id-seen [
		cdi-chunk 0x1002 img_id read/into join path filename file_buf
	]
	img_id
]

; Return app_id of map chunk.
pack-tmx: func [id filename chunk-dim] [
	poke-id tmx_id id

	tmx: load-tmx filename
	data: tmx/data
	if chunk-dim [
		data: map-chunks tmx/width tmx/height data chunk-dim
	]
	cdi-chunk 0x1FC0 tmx_id data

	tmx_id
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

; Pull in font & shader files
pack-files: does [
	pack-dirs: []
	foreach it [%shader/ %font/] [
		if exists? spath: join root-path it [
			append pack-dirs spath
		]
	]

	ifn empty? pack-dirs [
		sout: make binary! 4096
		code: complement charset "^//"
		strip-shader: func [shader] [
			clear sout
			emit-span: [(append sout slice span end) span:]
			parse span: shader [any[
				end: some code
			  | "//" to '^/'   emit-span
			  | "/*" thru "*/" emit-span
			  | some '^/'      emit-span (append sout '^/')
			  | skip
			]]
			append sout slice span end
		]

		sl_id:  "SL^0^0"
		txf_id: "TF^0^0"

		foreach spath pack-dirs [
			foreach file read spath [
				switch file-ext file [
					%.glsl [
						poke-id sl_id add file-id file module-layer
						ifn file-id-seen [
							cdi-chunk 0x0001 sl_id
								strip-shader read/into join spath file file_buf
						]
					]
					%.png [
						pack-png spath file
					]
					%.txf [
						poke-id txf_id add file-id file module-layer
						ifn file-id-seen [
							cdi-chunk 0x5FC0 txf_id
								read/into join spath file file_buf
						]
					]
				]
			]
		]
	]
]

if file-package [
	cdi-begin "xuB^2"
	pack-files

	if ge? verbose 2 [probe file-dict]

	cdi-chunk 0x0006 "FNAM" cdi-string-table1 file-dict
	cdi-end

	if ge? verbose 1 [print-toc]
	quit
]

;---------------------------------------
; Load module configuration

config-file: join root-path %config.b
ifn exists? config-file [
	fatal noinput ["Cannot find config" config-file]
]

; Matches ModInfoValues in module.c.
modi: context [
	about:
	author:
	rules:
	version:
		none
]

module: func [blk] [do bind blk modi]

includes: []
include: func [file] [append includes file]

; Matches ConfigValues in config_boron.cpp.
cfg: make context [
	armors:
	weapons:
	creatures:
	graphics:
	draw-lists:
	tileanim:
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
] load config-file

ifn rules: modi/rules [
	error "Missing module/rules"
]
if all [string? rules find rules '/'] [
	module-layer: 0x2000
]

foreach file includes [
	do bind load join root-path file cfg
]

process-cfg: func [blk] [do bind blk cfg]

;---------------------------------------
; NPC Talk

context [
store: sdict: none
meld: func [data] [
	map it data [
		switch type? it [
			string! [
				ifn ss: pick sdict it [
					ss: tail store
					append store it
					poke sdict it ss: slice ss tail store
					append store '^0'
				]
				it: ss
			]
			block! [meld it]
		]
		it
	]
]

set 'meld-strings func [data /extern store sdict] [
	store: make string! 1024
	sdict: make hash-map! 256
	meld data
	sdict
]
]

npc-talk: context [
	name:
	pronoun:
	look: none
	turn-away: 0
	topics: none
]

; Return app_id of talk chunk.
pack-talk: func [filename /local it] [
	poke-id npc_id file-id filename
	ifn file-id-seen [
		meld-strings spec: load join root-path filename
		tblk: make block! mul size? spec 5
		foreach it spec [
			do bind it npc-talk
			append tblk mark-sol values-of npc-talk
		]
		if ge? verbose 2 [
			print [filename '>' to-binary npc_id]
			probe tblk
		]
		cdi-chunk 0x4007 npc_id serialize tblk
	]
	npc_id
]

;---------------------------------------
; Build module package.

process-sound: func [blk app_id] [
	ifn blk [return none]

	path: root-path
	n: 0
	app_id: copy app_id
	parse blk [some[
		tok: file! (
			fname: first tok
			fmt: select [
				".wav"  0x2006
				".mp3"  0x2007
				".ogg"  0x2008
				".flac" 0x2011
				".rfx"  0x2030
			] ext: file-ext fname
			ifn fmt [fatal config ["Unknown audio file extension" ext]]

			poke-id app_id ++ n

			cdi-chunk fmt app_id read join path first tok
		)
	  | int! (n: first tok)
	  | 'path file! (path: terminate join root-path second tok '/')
	]]
	n
]

/*
  Parse data block! with rules and replace the data with a new block.
  The global 'blk variable is the output block which 'rules should append to.
*/
process-blk: func ['data rules /extern blk] [
	ifn get data [return none]

	blk: make block! 16
	ifn parse get data [some rules] [
		fatal config join "Invalid " data
	]
	set data blk
]

/*
  Parse data block! with rules but keep the data.
*/
process-minimal: func ['data rules] [
	ifn orig: get data [return none]

	ifn parse orig [some rules] [
		fatal config join "Invalid " data
	]
	orig
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

transforms: make binary! 512
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
	append transforms construct binary! [	; struct TileAnimTransform
		u8  type 0 0 0
		u16 x y w h
		u32 0 0
	]
]

map-labels: none
map-portals: []
map-moongates: none
map-roles: none

cdi-begin "xuB^2"
pack-files

process-cfg [
	music: process-sound music "MU^0^0"
	sound: process-sound sound "SO^0^0"

	process-blk armors [
		tok: string! int! opt [word! block!] (
			appair blk first tok to-coord reduce [
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
			word! block!	(constr: make-constraint stok)
		  | set-word! word!	(set in weapon-tiles first stok second stok)
		  | word!			(flags: or flags select weapon-flags first stok)
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
			appair blk at/name at/tile

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
					  0x20 swims		; water
					  0x20 sails		; water
					  0x40 cantattack	; nonAttackable
					 ;0x80 negate
					0x0100 camouflage
					0x0200 wontattack	; noAttack
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
				none-zero at/u4SaveId
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
		  | 'image-width set width int!
		  | word! int! (
			  frames: second tok
			  do emit-name append out to-coord [pos size frames]
			  do next-tile
			  loop sub frames 1 [
				  appair out mark-sol '_cel to-coord [pos size]
				  do next-tile
			  ]
			)
		  | word! (
			  do emit-name append out to-coord [pos size]
			  do next-tile
			)
		]]
	]

	process-img: func [name at subimages] [
		fname: at/filename
		either eq? ".png" file-ext fname [
			fname: copy pack-png image-path fname
			ftype: 'png
		][
			ftype: at/filetype
		]

		appair blk mark-sol name fname
		appair blk to-coord reduce [
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
				none intro abyss abacus dungns transparent0
				blackTransparencyHack fmtownsscreen
			] at/fixup
		]

		either block? subimages [
			append/block blk make block! 8
			layout-subimages subimages last blk at/width
		][
			append blk none
		]
	]

	image-path: join root-path %image/
	n: 0

	process-blk graphics [
		'image set at paren! tok: opt block! (
			process-img at/name at first tok
		)
	  | tok: set-word! 'atlas coord! block! (
			append blk reduce [
				mark-sol to-word first tok
				'atlas
				third tok
				pick tok 4
				none
			]
		)
	  | set-word! string! opt block! (
			poke tmp-attr: [filename: none] 2 second tok
			process-img to-word first tok tmp-attr third tok
		)
	]

	; Use draw-lists as is except remove any string! from quads block.
	process-minimal draw-lists [
		word! coord! tok: block! (
			poke tok 1 collect coord! first tok
		)
	]

	process-blk tileanim [
		tok: set-word! (
			aspec: second tok
			anim-chance: 0
			if eq? 'random first aspec [anim-chance: second aspec]

			appair blk
				to-word first tok
				to-coord reduce [
					div size? transforms 20	 ; sizeof(TileAnimTransform)
					anim-chance
				]
			current-trans: none
		) into [some [
			'random set n int! (
				; Ignore initial 'random as it is handled above.
				if current-trans [poke current-trans 2 n]
			)
		  | 'invert set n coord!	  (new-transform 0 n)
		  | 'scroll set n int!/coord! (new-transform 1 n)
		  | 'frame					  (new-transform 2 0)
		  | 'pixel_color set n coord! (new-transform 3 n) into [
				set colA coord! set colB coord! (
					n: 13
					foreach chan reduce [
						first colA second colA third colA 0
						first colB second colB third colB 0
					][
						poke current-trans ++ n chan
					]
				)
			]
		  | 'context-frame set n int! (
				poke current-trans 3 1  ; ACON_FRAME
				poke current-trans 4 n
			)
		]]
	]
	ifn empty? transforms [
		append blk mark-sol transforms
	]

	process-blk tile-rules [
		set at paren! (
			appair blk mark-sol at/name to-coord reduce [
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
				attribute-flags at [		; movement
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
			appair blk mark-sol at/name at/rule
			appair blk at/image at/animation
			appair blk at/directions to-coord reduce [
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
			fname: at/fname
			chunk-dim: at/chunk-dim
			either eq? ".tmx" file-ext fname [
				copy pack-tmx at/id fname chunk-dim
				fname: none
			][
				fname: to-file fname
			]

			appair blk mark-sol fname to-coord reduce [
				at/id
				enum-value [world city shrine combat dungeon] at/type
				enum-value [wrap exit fixed] at/borderbehavior
				at/width
				at/height
				at/levels
			]
			append blk to-coord reduce [
				none-zero chunk-dim
				none-zero chunk-dim
				attribute-flags at [
					0x02 nolineofsight
					0x04 firstperson
				]
				none-zero at/music
			]

			map-labels: none
			clear map-portals
			map-moongates: none
			map-roles: none
		)
	  | into [some[
			'portal set at2 paren! content: opt block! (
				append/block map-portals at2
				if block? first content [
					; Merge retroActiveDest into attributes to be used below.
					if it: content/1/retroActiveDest [
						appair at2 to-set-word 'retroActiveDest
							to-coord reduce [it/x it/y none-zero it/z it/mapid]
					]
				]
			)
		  | 'labels set map-labels block!
		  | 'city  set at2 paren! into [any [
				'roles set map-roles block!
			]] (
				tlk_name: at2/tlk_fname
				either eq? ".tlk" file-ext tlk_name [
					tlk_name: to-file tlk_name
				][
					tlk_name: copy pack-talk tlk_name
				]

				append blk reduce [at2/name at2/type tlk_name]
				append/block blk either map-roles [
					dest: make block! 0
					foreach [role id] map-roles [
						append dest to-coord reduce [
							enum-value [
								companion	weaponsvendor	armorvendor
								foodvendor	tavernkeeper	reagentsvendor
								healer		innkeeper		guildvendor
								horsevendor	lordbritish		hawkwind
							] role
							id
						]
					]
					dest
				] none
			)
		  | 'dungeon  set at2 paren! (appair blk at2/name at2/rooms)
		  | 'shrine   set at2 paren! (
				appair blk at2/mantra enum-value [
					"honesty" "compassion" "valor" "justice"
					"sacrifice" "honor" "spirituality" "humility"
				] at2/virtue
			)
		  | 'moongates set map-moongates block!
		]] (
			append/block blk map-labels

			emit-attr-block blk map-portals [
				appair dest mark-sol it/message it/condition
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
					   enter	1
					   klimb	2
					   descend	4
					   exit_north	8
					   exit_east	0x10
					   exit_south	0x20
					   exit_west	0x40
					] it/action
					either eq? 'true it/savelocation 1 0
					none-zero select [
						foot	1
						horse	2
						ship	4
						balloon	8
						footorhorse 3
					] it/transport
				]
				append dest it/retroActiveDest
			]

			append/block blk map-moongates
		)
	]

	if ega-palette [
		bin: make binary! mul 16 4
		foreach it ega-palette [
			appair bin first it second it
			appair bin third it 255
		]
		ega-palette: bin
	]
]

if ge? verbose 2 [
	probe modi
	probe cfg
	probe file-dict
]

cdi-chunk 0x0006 "MODI" cdi-string-table1 values-of modi
cdi-chunk 0x7FC0 "CONF" serialize reduce [cfg]
ifn empty? file-dict [
	cdi-chunk 0x0006 "FNAM" cdi-string-table1 file-dict
]
cdi-end

if ge? verbose 1 [print-toc]
