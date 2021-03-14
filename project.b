exe %u4 [
	include_from [
		%src
		%/usr/include/SDL
		%/usr/include/libxml2
	]
	libs [%SDL %SDL_mixer %xml2 %png %z]
	unix [cflags "-Wno-unused-parameter"]
	cflags {-DVERSION=\"KR-1.0\"}

	sources_from %src [
		%annotation.cpp
		%armor.cpp
		%aura.cpp
		%camp.cpp
		%cheat.cpp
		%city.cpp
		%codex.cpp
		%combat.cpp
		%config.cpp
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
		%imageloader.cpp
		%imageloader_fmtowns.cpp
		%imageloader_png.cpp
		%imageloader_u4.cpp
		%imageloader_u5.cpp
		%imagemgr.cpp
		%imageview.cpp
		%intro.cpp
		%io.cpp
		%item.cpp
		%location.cpp
		%map.cpp
		%maploader.cpp
		%mapmgr.cpp
		%menu.cpp
		%menuitem.cpp
		%moongate.cpp
		%movement.cpp
		%music.cpp
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
		%script.cpp
		%settings.cpp
		%shrine.cpp
		%sound.cpp
		%spell.cpp
		%stats.cpp
		%textview.cpp
		%tileanim.cpp
		%tile.cpp
		%tilemap.cpp
		%tileset.cpp
		%tileview.cpp
		%u4.cpp
		%u4file.cpp
		%utils.cpp
		%unzip.c
		%view.cpp
		%weapon.cpp

		%xml.cpp

		%lzw/hash.c
		%lzw/lzw.c
		%lzw/u6decode.cpp
		%lzw/u4decode.cpp

		; SDL
		%event_sdl.cpp
		%screen_sdl.cpp
		%sound_sdl.cpp
		%image_sdl.cpp
	]
]
