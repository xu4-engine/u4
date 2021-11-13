options [
	os_api: 'allegro	"Platform API ('allegro 'sdl)"
	use_gl: false
	use_boron: true
	boron_sdk: none		"Path to Boron headers and libraries"
	gpu_render: true
	make_util: true
]

libxml2: does [
	unix [
		include_from %/usr/include/libxml2
		libs %xml2
	]
	win32 [
		either msvc [
			cflags "/DLIBXML_STATIC"
			libs_from %../usr/lib [%libxml2_a]
		][
			libs %xml2
		]
	]
]

exe %u4 [
	include_from [%src %src/lzw %src/support]
	win32 [
		include_from %../usr/include
		include-define "_WIN32"		; Needed to archive glad.*
	]

	switch os_api [
		allegro [
			unix [
				libs [%allegro_acodec %allegro_audio %allegro]
			]
			win32 [
				libs_from %../usr/lib [%allegro_acodec %allegro_audio %allegro]
			]
			sources_from %src [
				%event_allegro.cpp
				%screen_allegro.cpp
				%sound_allegro.cpp
			]
		]
		sdl [
			include_from %/usr/include/SDL
			libs [%SDL %SDL_mixer]
			sources_from %src [
				%event_sdl.cpp
				%screen_sdl.cpp
				%sound_sdl.cpp
			]
		]
	]

	either use_boron [
		cflags "-DUSE_BORON -DCONF_MODULE"
		unix [
			either [boron_sdk] [
				include_from join boron_sdk %/include
				libs_from    join boron_sdk %/lib %boron
				libs %pthread
			][
				libs %boron
			]
		]
		win32 [
			libs_from %../usr/lib either msvc %libboron %boron
			libs %ws2_32
		]
		sources_from %src [
			%config_boron.cpp
			%support/cdi.c
		]
	][
		libxml2
		sources_from %src [
			%config.cpp
			%script.cpp
			%xml.cpp
			%support/SymbolTable.cpp
		]
	]

	if use_gl [
		if gpu_render [cflags "-DGPU_RENDER"]
		cflags "-DUSE_GL"
		opengl
	]

	unix [
		cflags "-Wno-unused-parameter"
		libs [%png %z]
	]
	win32 [
		either msvc [
			libs_from %../usr/lib [%libpng16 %zlib]
			libs [%User32]
		][
			cflags "-Wno-unused-parameter"
			;lflags "-static-libgcc"	; Causes problems with Allegro libs.
			lflags "-static-libstdc++"
			libs [%png %z]
		]
	]
	cflags {-DVERSION=\"KR-1.0\"}

	sources_from %src [
		%anim.c
		%annotation.cpp
		%aura.cpp
		%camp.cpp
		%cheat.cpp
		%city.cpp
		%codex.cpp
		%combat.cpp
		%controller.cpp
		%context.cpp
		%conversation.cpp
		%creature.cpp
		%death.cpp
		%debug.cpp
		%dialogueloader.cpp
		%dialogueloader_hw.cpp
		%dialogueloader_lb.cpp
		%dialogueloader_tlk.cpp
		%direction.cpp
		%dungeon.cpp
		%dungeonview.cpp
		%error.cpp
		%event.cpp
		%filesystem.cpp
		%game.cpp
		%image.cpp
		%imageloader.cpp
		%imagemgr.cpp
		%imageview.cpp
		%intro.cpp
		%item.cpp
		%location.cpp
		%map.cpp
		%maploader.cpp
		%menu.cpp
		%menuitem.cpp
		%movement.cpp
		%names.cpp
		%object.cpp
		%person.cpp
		%player.cpp
		%portal.cpp
		%progress_bar.cpp
		%rle.cpp
		%savegame.cpp
		%scale.cpp
		%screen.cpp
		%settings.cpp
		%shrine.cpp
		%spell.cpp
		%stats.cpp
		%textview.cpp
		%tileanim.cpp
		%tile.cpp
		%tileset.cpp
		%tileview.cpp
		%u4.cpp
		%u4file.cpp
		%utils.cpp
		%unzip.c
		%view.cpp

		%lzw/hash.c
		%lzw/lzw.c
		%lzw/u6decode.cpp
		%lzw/u4decode.cpp
	]
]

if make_util [
	exe %coord   [console sources [%src/util/coord.c]]
	exe %tlkconv [console libxml2 sources [%src/util/tlkconv.c]]
	exe %dumpmap [console sources [%src/util/dumpmap.c]]
	exe %dumpsavegame [
		console
		include_from %src
		sources [
			%src/util/dumpsavegame.cpp
		]
	]
]
