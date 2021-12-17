#DBUILD=docker build
DBUILD=buildah bud

.PHONY: cbuild-images cbuild-sdks clean-sdks

cbuild-images:
	$(DBUILD) -f Dockerfile.fedora33 -t xu4/fedora33:base .
	$(DBUILD) -f Dockerfile.gcc -t xu4/f33-gcc:1.0 .
	$(DBUILD) -f Dockerfile.mingw -t xu4/f33-mingw:1.0 .

XU4_DL=http://xu4.sourceforge.net/download
BORON_SDK_LINUX=boron-x86_64-static-2.0.7.tar.bz2
BORON_SDK_MINGW=boron-x86_64-w64-mingw32-static-2.0.7.zip
DLL_MINGW=allegro-dll.tar.bz2

ALLEGRO_DL=https://github.com/liballeg/allegro5/releases/download/5.2.7.0
ALLEGRO_SDK_MINGW=allegro-x86_64-w64-mingw32-gcc-10.2.0-posix-seh-static-5.2.7.0.zip

sdk:
	mkdir sdk

cbuild-sdks: sdk
	curl -sSL -o sdk/$(BORON_SDK_LINUX) $(XU4_DL)/$(BORON_SDK_LINUX)
	curl -sSL -o sdk/$(BORON_SDK_MINGW) $(XU4_DL)/$(BORON_SDK_MINGW)
	curl -sSL -o sdk/$(DLL_MINGW) $(XU4_DL)/$(DLL_MINGW)
	curl -sSL -o sdk/$(ALLEGRO_SDK_MINGW) $(ALLEGRO_DL)/$(ALLEGRO_SDK_MINGW)

clean-sdks:
	rm -rf sdk

boron:
	git clone git://git.code.sf.net/p/urlan/boron/code boron
	cd boron && git checkout d84cb774 && ./configure --no-execute --no-socket --static --thread

sdk/$(BORON_SDK_LINUX): sdk boron
	rm -rf /tmp/boron
	make -C boron DESTDIR=/tmp/boron libboron.a install-dev
	-mv /tmp/boron/lib64 /tmp/boron/lib
	tar cjf $@ -C /tmp --exclude=share boron
