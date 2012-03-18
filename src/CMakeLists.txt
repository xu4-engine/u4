
add_executable (xu4 annotation.cpp armor.cpp aura.cpp camp.cpp cheat.cpp city.cpp codex.cpp
	combat.cpp config.cpp controller.cpp conversation.cpp creature.cpp death.cpp
	debug.cpp dialogueloader.cpp dialogueloader_hw.cpp dialogueloader_lb.cpp dialogueloader_tlk.cpp
	direction.cpp dungeon.cpp dungeonview.cpp error.cpp event.cpp event_sdl.cpp filesystem.cpp
	game.cpp imageloader.cpp imageloader_fmtowns.cpp imageloader_png.cpp imageloader_u4.cpp
	imageloader_u5.cpp imagemgr.cpp image_sdl.cpp imageview.cpp intro.cpp io.cpp item.cpp
	location.cpp map.cpp maploader.cpp mapmgr.cpp menu.cpp menuitem.cpp moongate.cpp movement.cpp
	music.cpp music_sdl.cpp names.cpp object.cpp person.cpp player.cpp portal.cpp progress_bar.cpp
	rle.cpp savegame.cpp scale.cpp screen.cpp screen_sdl.cpp script.cpp settings.cpp shrine.cpp
	sound.cpp sound_sdl.cpp spell.cpp stats.cpp textview.cpp tileanim.cpp tile.cpp tilemap.cpp
	tileset.cpp tileview.cpp u4.cpp u4file.cpp u4_sdl.cpp utils.cpp unzip.c view.cpp weapon.cpp
	xml.cpp lzw/hash.c lzw/lzw.c lzw/u6decode.cpp lzw/u4decode.cpp
	#   WIN32 # Only if you don't want the DOS prompt to appear in the background in Windows
   #MACOSX_BUNDLE
)

target_link_libraries (
	xu4
	${SDL_LIBRARY}
	${SDLMIXER_LIBRARY}
	${LIBXML2_LIBRARIES}
	${PNG_LIBRARY}
)

include_directories (
	${UXU4_SOURCE_DIR}/src
	${SDL_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${LIBXML2_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
)
add_definitions(-DVERSION="svn1.1.1.1")
#CMAKE_CXX__FLAGS=-DVERSION="0.1"
