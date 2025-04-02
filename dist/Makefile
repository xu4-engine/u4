#DBUILD=docker build
DBUILD=buildah bud

.PHONY: cbuild-images cbuild-sdks clean-sdks

cbuild-images:
	$(DBUILD) -f Dockerfile.gcc -t xu4/f33-gcc .
	$(DBUILD) -f Dockerfile.mingw -t xu4/f41-mingw .

XU4_DL=http://xu4.sourceforge.net/download
BORON_SDK_LINUX=boron-x86_64-static-2.0.8.tar.bz2
BORON_SDK_MINGW=boron-x86_64-w64-mingw32-static-2.0.8.zip

#ALLEGRO_DL=https://github.com/liballeg/allegro5/releases/download/5.2.7.0
#ALLEGRO_SDK_MINGW=allegro-x86_64-w64-mingw32-gcc-10.2.0-posix-seh-static-5.2.7.0.zip

GLFW_DL=https://github.com/glfw/glfw/releases/download/3.4
GLFW_SDK_MINGW=glfw-3.4.bin.WIN64.zip

FAUN_DL=https://codeberg.org/WickedSmoke/faun/releases/download/v0.2.2
FAUN_SDK_LINUX=faun-x86_64-static-0.2.2.tar.bz2
FAUN_SDK_MINGW=faun-x86_64-w64-mingw32-static-0.2.2.zip

sdk:
	mkdir sdk

cbuild-sdks: sdk
	curl -sSL -o sdk/$(BORON_SDK_LINUX) $(XU4_DL)/$(BORON_SDK_LINUX)
	curl -sSL -o sdk/$(BORON_SDK_MINGW) $(XU4_DL)/$(BORON_SDK_MINGW)
	curl -sSL -o sdk/$(GLFW_SDK_MINGW) $(GLFW_DL)/$(GLFW_SDK_MINGW)
	curl -sSL -o sdk/$(FAUN_SDK_LINUX) $(FAUN_DL)/$(FAUN_SDK_LINUX)
	curl -sSL -o sdk/$(FAUN_SDK_MINGW) $(FAUN_DL)/$(FAUN_SDK_MINGW)

clean-sdks:
	rm -rf sdk

boron:
	git clone git://git.code.sf.net/p/urlan/boron/code boron
	cd boron && git checkout v2.0.8 && ./configure --no-execute --no-socket --static --thread

sdk/$(BORON_SDK_LINUX): sdk boron
	rm -rf /tmp/boron
	make -C boron DESTDIR=/tmp/boron libboron.a install-dev
	-mv /tmp/boron/lib64 /tmp/boron/lib
	tar cjf $@ -C /tmp --exclude=share boron
