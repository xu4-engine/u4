options [
	os_api: 'allegro	"Platform API ('allegro 'glv 'sdl)"
	use_faun: true
	sdk_dir: none		"Path to Boron/Faun headers and libraries (UNIX only)"
	gpu_render: false
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

exe %xu4 [
	include_from [%src %src/lzw %src/support]
	unix [
		if sdk_dir [
			include_from join sdk_dir %/include
			lflags rejoin ["-L" sdk_dir %/lib]
		]
	]
	win32 [
		include_from %../usr/include
		include-define "_WIN32"		; Needed to archive glad.*
	]

	switch os_api [
		allegro [
			unix [
				libs pick [
					[%allegro %faun %pulse-simple %pulse %vorbisfile]
					[%allegro_acodec %allegro_audio %allegro]
				] use_faun
			]
			win32 [
				libs_from %../usr/lib pick [
					[%allegro %faun %vorbisfile %ole32]
					[%allegro_acodec %allegro_audio %allegro]
				] use_faun
			]
			sources_from %src reduce [
				%event_allegro.cpp
				%screen_allegro.cpp
				either use_faun
					%sound_faun.cpp
					%sound_allegro.cpp
			]
		]
		glv [
			unix [
				include_from %src/glv/x11
				libs [%Xcursor %X11]
				sources/flags [%src/glv/x11/glv.c] "-DUSE_CURSORS"
			]
			libs [%faun]
			sources_from %src [
				%screen_glv.cpp
				%sound_faun.cpp
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

	;if use_boron [
		cflags "-DUSE_BORON -DCONF_MODULE"
		unix [
			libs %boron
			if sdk_dir [
					; Needed for static libboron & libfaun.
					libs [%pthread %pulse-simple %pulse %vorbisfile]
			]
		]
		win32 [
			libs_from %../usr/lib either msvc %libboron %boron
			libs %ws2_32
		]
		sources_from %src [
			%config_boron.cpp
			%module.c
			%support/cdi.c
		]
	;]

	if gpu_render [cflags "-DGPU_RENDER"]
	cflags "-DUSE_GL"
	opengl

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
		include_from %src/win32
		sources/flags [%src/win32/xu4.rc] "-I src/win32"
	]
	cflags {-DVERSION=\"DR-1.0\"}

	sources_from %src [
		%annotation.cpp
		%aura.cpp
		%camp.cpp
		%cheat.cpp
		%city.cpp
		%codex.cpp
		%combat.cpp
		%controller.cpp
		%context.cpp
		%creature.cpp
		%death.cpp
		%debug.cpp
		%direction.cpp
		%discourse.cpp
		%dungeon.cpp
		%dungeonview.cpp
		%error.cpp
		%event.cpp
		%filesystem.cpp
		%game.cpp
		%gamebrowser.cpp
		%gui.cpp
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
		%names.cpp
		%object.cpp
		%party.cpp
		%person.cpp
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
		%u4file.cpp
		%view.cpp
		%xu4.cpp

		%lzw/hash.c
		%lzw/lzw.c
		%lzw/u6decode.cpp
		%lzw/u4decode.cpp

		%support/notify.c
		%support/stringTable.c
		%support/txf_draw.c
		%support/unzip.c
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

; cbuild makes the archive for mingw & allegro so add the Linux files as well.
dist [
	%src/screen_glv.cpp
	%src/glv/x11/glv.c
	%src/glv/x11/glv.h
	%src/glv/x11/glv_keys.h
]
