#DBUILD=docker build
DBUILD=buildah bud

.PHONY: cbuild-images

cbuild-images:
	$(DBUILD) -f Dockerfile.fedora33 -t xu4/fedora33:base .
	$(DBUILD) -f Dockerfile.gcc -t xu4/f33-gcc:1.0 .
	$(DBUILD) -f Dockerfile.mingw -t xu4/f33-mingw:1.0 .
