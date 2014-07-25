add_executable (xu4 annotation.cpp armor.cpp aura.cpp camp.cpp cheat.cpp city.cpp codex.cpp
	combat.cpp config.cpp controller.cpp context.cpp conversation.cpp creature.cpp death.cpp
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

set(xu4_required_libraries
	${SDL_LIBRARY}
	${SDLMIXER_LIBRARY}
	${SDL_MIXER_LIBRARY} #Latest verified CMake 3.0
	${LIBXML2_LIBRARIES}
	${PNG_LIBRARY}
	${PNG_LIBRARY_RELEASE} #Latest verified CMake 3.0
	${ZLIB_LIBRARY}
)

target_link_libraries (
	xu4
	${xu4_required_libraries}
)

include_directories (
	${UXU4_SOURCE_DIR}/src
	${SDL_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${SDL_MIXER_INCLUDE_DIR} #Latest verified CMake 3.0
	${LIBXML2_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${PNG_PNG_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)
add_definitions(-DVERSION="svn1.1.1.1")

# Copy_xu4_required_files_to_runtime 
#	will copy files to the Release/Debug folders, alongside
#	the generated xu4 executable.
#	This is completely optional and is included because
#	it makes IDE-based debugging and testing easier.
SET(DIRECTORIES_TO_COPY_TO_RUNTIME conf mid sound graphics) 	
add_custom_target(Copy_xu4_required_files_to_runtime)
IF(WIN32)
	FOREACH(LIB ${xu4_required_libraries})
		FILE(GLOB DLLS ${LIB}/../*.dll)
		FOREACH(DLL ${DLLS})
			add_custom_command(TARGET Copy_xu4_required_files_to_runtime
				COMMAND ${CMAKE_COMMAND} -E
				copy "${DLL}" "$<TARGET_FILE_DIR:xu4>")
		ENDFOREACH()
	ENDFOREACH()
ENDIF(WIN32)
FOREACH(DIR ${DIRECTORIES_TO_COPY_TO_RUNTIME})
	add_custom_command(TARGET Copy_xu4_required_files_to_runtime 
		COMMAND ${CMAKE_COMMAND} -E
		copy_directory "${CMAKE_SOURCE_DIR}/${DIR}" "$<TARGET_FILE_DIR:xu4>/${DIR}"
	)
ENDFOREACH()

