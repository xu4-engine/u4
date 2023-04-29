# The boron, faun, & png libraries are expected in the android/sdk/ directory.

LOCAL_PATH := $(call my-dir)

SDK_LIB := sdk/libs/$(TARGET_ARCH_ABI)


include $(CLEAR_VARS)
LOCAL_MODULE := boron
LOCAL_SRC_FILES := $(SDK_LIB)/libboron.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/sdk/include
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := faun
LOCAL_SRC_FILES := $(SDK_LIB)/libfaun.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := vorbisfile
LOCAL_SRC_FILES := $(SDK_LIB)/libvorbisfile.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := vorbis
LOCAL_SRC_FILES := $(SDK_LIB)/libvorbis.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ogg
LOCAL_SRC_FILES := $(SDK_LIB)/libogg.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := png
LOCAL_SRC_FILES := $(SDK_LIB)/libpng16.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := glv
LOCAL_SRC_FILES := ../src/glv/android/gesture.c \
	../src/glv/android/glv_activity.c \
	../src/glv/android/glv.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../src/glv/android
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := xu4-activity
LOCAL_CFLAGS := -DUSE_GL -DUSE_BORON -DCONF_MODULE -DVERSION=\"1.1A\"
LOCAL_CPPFLAGS := -Wno-format-security
LOCAL_CPP_FEATURES := rtti
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src $(LOCAL_PATH)/../src/support
LOCAL_SRC_FILES := app_main.c \
	../src/screen_glv.cpp \
	../src/sound_faun.cpp \
	../src/config_boron.cpp \
	../src/module.c \
	../src/support/cdi.c \
	../src/annotation.cpp \
	../src/aura.cpp \
	../src/camp.cpp \
	../src/cheat.cpp \
	../src/city.cpp \
	../src/codex.cpp \
	../src/combat.cpp \
	../src/controller.cpp \
	../src/context.cpp \
	../src/creature.cpp \
	../src/death.cpp \
	../src/debug.cpp \
	../src/direction.cpp \
	../src/discourse.cpp \
	../src/dungeon.cpp \
	../src/dungeonview.cpp \
	../src/error.cpp \
	../src/event.cpp \
	../src/filesystem.cpp \
	../src/game.cpp \
	../src/gamebrowser.cpp \
	../src/gui.cpp \
	../src/image.cpp \
	../src/imageloader.cpp \
	../src/imagemgr.cpp \
	../src/imageview.cpp \
	../src/intro.cpp \
	../src/item.cpp \
	../src/location.cpp \
	../src/map.cpp \
	../src/maploader.cpp \
	../src/menu.cpp \
	../src/menuitem.cpp \
	../src/movement.cpp \
	../src/names.cpp \
	../src/object.cpp \
	../src/party.cpp \
	../src/person.cpp \
	../src/portal.cpp \
	../src/progress_bar.cpp \
	../src/rle.cpp \
	../src/savegame.cpp \
	../src/scale.cpp \
	../src/screen.cpp \
	../src/settings.cpp \
	../src/shrine.cpp \
	../src/spell.cpp \
	../src/stats.cpp \
	../src/textview.cpp \
	../src/tileanim.cpp \
	../src/tile.cpp \
	../src/tileset.cpp \
	../src/tileview.cpp \
	../src/u4file.cpp \
	../src/view.cpp \
	../src/xu4.cpp \
	../src/lzw/hash.c \
	../src/lzw/lzw.c \
	../src/lzw/u6decode.cpp \
	../src/lzw/u4decode.cpp \
	../src/support/notify.c \
	../src/support/stringTable.c \
	../src/support/txf_draw.c \
	../src/support/unzip.c

LOCAL_STATIC_LIBRARIES := boron faun vorbisfile vorbis ogg glv png
LOCAL_LDLIBS := -llog -laaudio -landroid -lEGL -lGLESv3 -lz
include $(BUILD_SHARED_LIBRARY)
