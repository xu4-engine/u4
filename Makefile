# GNU Makefile for Linux & macOS.

OS := $(shell uname)
ifeq ($(OS), Darwin)
MFILE_OS=Makefile.macosx
else
MFILE_OS=Makefile
endif

all: src/u4

src/u4:
	make -C src -f $(MFILE_OS)

.PHONY: clean
clean:
	make -C src -f $(MFILE_OS) clean
