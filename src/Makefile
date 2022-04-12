#
# Makefile for Linux
#

-include ../make.config

PREFIX ?= /usr/local
UI ?= allegro
GPU ?= scale

bindir=$(PREFIX)/bin
libdir=$(PREFIX)/lib
datadir=$(PREFIX)/share

ifeq ($(UI), sdl)
SOUND=sdl
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)
else
SOUND=faun
ifeq ($(SOUND),faun)
UILIBS=-lallegro -lfaun
else
UILIBS=-lallegro_acodec -lallegro_audio -lallegro
endif
UIFLAGS=
endif

ifeq ($(GPU),all)
	UIFLAGS+=-DGPU_RENDER
endif

#ifeq ($(CONF),boron)
	UIFLAGS+=-DUSE_BORON -DCONF_MODULE
	UILIBS+=-lboron
#endif

FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
DEBUGCXXFLAGS=-rdynamic -g
CXXFLAGS=$(FEATURES) -Wall -I. -Isupport $(UIFLAGS) -DVERSION=\"$(VERSION)\" $(DEBUGCXXFLAGS)
CFLAGS=$(CXXFLAGS)
LIBS=$(UILIBS) -lGL -lpng -lz
INSTALL=install

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

