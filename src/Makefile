#
# Makefile for Linux
#

-include ../make.config

UI ?= glv
GPU ?= scale
SOUND=faun

ifeq ($(UI), allegro)
ifeq ($(SOUND),allegro)
	UILIBS=-lallegro_acodec -lallegro_audio -lallegro
else
	UILIBS=-lallegro
endif
endif

ifeq ($(UI), sdl)
UILIBS=$(shell sdl-config --libs)
UIFLAGS=$(shell sdl-config --cflags)
ifeq ($(SOUND), sdl)
	UILIBS+=-lSDL_mixer
endif
endif

ifeq ($(GPU),all)
	UIFLAGS+=-DGPU_RENDER
endif

ifeq ($(SOUND), faun)
	UILIBS+=-lfaun
endif

#ifeq ($(CONF),boron)
	UIFLAGS+=-DUSE_BORON -DCONF_MODULE
	UILIBS+=-lboron
#endif

CXXFLAGS=-Wall -I. -Isupport $(UIFLAGS) -DVERSION=\"$(VERSION)\"
#CXXFLAGS+=-rdynamic -DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1

# Choose one of these for debug/release mode.
#CXXFLAGS+=-g -DDEBUG
CXXFLAGS+=-O3 -DNDEBUG

ifeq ($(UI), glv)
CXXFLAGS+=-Iglv/x11
GLV_SRC=glv/x11/glv.c
UILIBS+=-lXcursor -lX11
CFLAGS=$(CXXFLAGS) -DUSE_CURSORS
else
CFLAGS=$(CXXFLAGS)
endif

LIBS=$(UILIBS) -lGL -lpng -lz

ifeq ($(STATIC_GCC_LIBS),true)
    LDFLAGS+=-L. -static-libgcc
endif

include Makefile.common

# use GCC's automatic dependency generation
.depends::
	rm -f .depends
	$(CC) $(CFLAGS) -MM $(CSRCS) >> .depends
	$(CXX) $(CXXFLAGS) -MM $(CXXSRCS) >> .depends

# a target to build without dependencies on libstdc++, libgcc_s
all.static_gcc_libs:
	ln -s `$(CXX) -print-file-name=libstdc++.a` .
	$(MAKE) STATIC_GCC_LIBS=true
	rm -f libstdc++.a

-include .depends

