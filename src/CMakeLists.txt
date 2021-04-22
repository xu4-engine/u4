
set(XU4_INC
	annotation.h
	area.h
	aura.h
	camp.h
	cheat.h
	city.h
	codex.h
	combat.h
	config.h
	context.h
	controller.h
	conversation.h
	coords.h
	creature.h
	cursors.h
	death.h
	debug.h
	dialogueloader.h
	dialogueloader_hw.h
	dialogueloader_lb.h
	dialogueloader_tlk.h
	direction.h
	dungeon.h
	dungeonview.h
	error.h
	event.h
	filesystem.h
	game.h
	image.h
	imageloader.h
	imagemgr.h
	imageview.h
	intro.h
	item.h
	location.h
	map.h
	mapmgr.h
	menu.h
	menuitem.h
	moongate.h
	movement.h
	names.h
	object.h
	observable.h
	observer.h
	person.h
	player.h
	portal.h
	progress_bar.h
	rle.h
	savegame.h
	scale.h
	screen.h
	script.h
	settings.h
	shrine.h
	sound.h
	spell.h
	stats.h
	textcolor.h
	textview.h
	tile.h
	tileanim.h
	tilemap.h
	tileset.h
	tileview.h
	types.h
	u4.h
	u4file.h
	u4_pch.h
	unzip.h
	utils.h
	view.h
	weapon.h
	xml.h
)

set(XU4_SRC
	annotation.cpp
	aura.cpp
	camp.cpp
	cheat.cpp
	city.cpp
	codex.cpp
	combat.cpp
	config.cpp
	controller.cpp
	context.cpp
	conversation.cpp
	creature.cpp
	death.cpp
	debug.cpp
	dialogueloader.cpp
	dialogueloader_hw.cpp
	dialogueloader_lb.cpp
	dialogueloader_tlk.cpp
	direction.cpp
	dungeon.cpp
	dungeonview.cpp
	error.cpp
	event.cpp
	filesystem.cpp
	game.cpp 
	image.cpp
	imageloader.cpp 
	imagemgr.cpp
	imageview.cpp 
	intro.cpp 
	item.cpp
	location.cpp 
	map.cpp 
	maploader.cpp 
	mapmgr.cpp 
	menu.cpp 
	menuitem.cpp 
	moongate.cpp 
	movement.cpp
	names.cpp 
	object.cpp 
	person.cpp 
	player.cpp 
	portal.cpp 
	progress_bar.cpp
	rle.cpp 
	savegame.cpp 
	scale.cpp 
	screen.cpp 
	script.cpp 
	settings.cpp 
	shrine.cpp
	spell.cpp 
	stats.cpp 
	textview.cpp 
	tileanim.cpp 
	tile.cpp 
	tilemap.cpp
	tileset.cpp 
	tileview.cpp 
	u4.cpp 
	u4file.cpp 
	utils.cpp 
	unzip.c 
	view.cpp 
	xml.cpp lzw/hash.c 
	lzw/lzw.c 
	lzw/u6decode.cpp 
	lzw/u4decode.cpp
)

set(XU4_SDL
	event_sdl.cpp 
	screen_sdl.cpp
	sound_sdl.cpp 
)

set(XU4_SDL2
	event_sdl2.cpp 
	screen_sdl2.cpp
	sound_sdl2.cpp 
)


include_directories (
	# ${UXU4_SOURCE_DIR}/src
	${SDL_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${SDL_MIXER_INCLUDE_DIR} #Latest verified CMake 3.0
	${LIBXML2_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${PNG_PNG_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)
add_executable (u4 
	${XU4_SRC}
	${XU4_INC}
	${XU4_SDL}
)

set(xu4_required_libraries
	${SDL_LIBRARY}
	${SDLMIXER_LIBRARY}
	${SDL_MIXER_LIBRARY} #Latest verified CMake 3.0
	${LIBXML2_LIBRARY}
	${PNG_LIBRARY}
	${PNG_LIBRARY_RELEASE} #Latest verified CMake 3.0
	${ZLIB_LIBRARY_RELEASE}
)


target_link_libraries (
	u4
	${xu4_required_libraries}
)

add_definitions(-DVERSION="svn1.1.1.1")

# Copy_xu4_required_files_to_runtime 
#	will copy files to the Release/Debug folders, alongside
#	the generated xu4 executable.
#	This is completely optional and is included because
#	it makes IDE-based debugging and testing easier.
SET(DIRECTORIES_TO_COPY_TO_RUNTIME conf mid sound graphics) 	
add_custom_target(copy_files)
IF(WIN32)
	FOREACH(LIB ${xu4_required_libraries})
		FILE(GLOB DLLS ${LIB}/../*.dll)
		FOREACH(DLL ${DLLS})
			add_custom_command(TARGET copy_files
				COMMAND ${CMAKE_COMMAND} -E
				copy "${DLL}" "$<TARGET_FILE_DIR:u4>")
		ENDFOREACH()
	ENDFOREACH()
ENDIF(WIN32)
FOREACH(DIR ${DIRECTORIES_TO_COPY_TO_RUNTIME})
	add_custom_command(TARGET copy_files 
		COMMAND ${CMAKE_COMMAND} -E
		copy_directory "${CMAKE_SOURCE_DIR}/${DIR}" "$<TARGET_FILE_DIR:u4>/../${DIR}"
	)
ENDFOREACH()

file(DOWNLOAD
http://ultima.thatfleminggent.com/ultima4.zip
${CMAKE_CURRENT_BINARY_DIR}/ultima4.zip
)

file(DOWNLOAD
http://sourceforge.net/projects/xu4/files/Ultima%204%20VGA%20Upgrade/1.3/u4upgrad.zip
${CMAKE_CURRENT_BINARY_DIR}/u4upgrad.zip
)
