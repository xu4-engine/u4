Name:      xu4
Version:   1.4.3
Release:   %autorelease
Summary:   Ultima IV Recreated
License:   GPL-3.0-or-later
URL:       https://xu4.sourceforge.net/
Source0:   https://downloads.sourceforge.net/xu4/1.4/xu4-%{version}.tar.gz
Source1:   https://downloads.sourceforge.net/urlan/Boron/boron-2.0.8.tar.gz
Source2:   https://downloads.sourceforge.net/xu4/Ultima%204%20VGA%20Upgrade/1.3/u4upgrad.zip
BuildRequires: g++, make, libpng-devel, libXcursor-devel, mesa-libGL-devel, zlib-devel, faun-devel

%global debug_package %{nil}

%description
XU4 is a recreation of the classic computer game Ultima IV. The purpose of
the project is to make it easy and convenient to play on modern operating
systems. Graphics, sounds & music can be customized via separate module files.

NOTE: The ultima4.zip (or ultima4-1.01.zip) must be installed manually.

%prep
%setup -q -n xu4-%{version}
%setup -T -D -a 1
mv boron-2.0.8 boron
mkdir boron/include/boron
cp boron/include/*.h boron/include/boron
sed -i -e 's~<zlib.h>~<zlib.h>\n#define USE_ZLIB~' -e 's~ZLIB_H~USE_ZLIB~' boron/eval/compress.c
sed -i s~DR-1.0~%{version}~ src/Makefile.common
sed -i -e 's~-Isupp~-I../boron/include -Isupp~' -e 's~-lboron~-L../boron -lboron~' -e 's~-O3~-fPIE -O3~' src/Makefile

%build
cd boron && ./configure --no-execute --no-socket --static --thread && make
cd ..
make src/xu4
boron/boron -s tools/pack-xu4.b -f module/render -o render.pak
boron/boron -s tools/pack-xu4.b module/Ultima-IV
boron/boron -s tools/pack-xu4.b module/U4-Upgrade

%install
%__mkdir_p %{buildroot}%{_bindir} %{buildroot}%{_datadir}/xu4
install -s -m 755 src/xu4 %{buildroot}%{_bindir}
install -D -m 644 icons/shield.png %{buildroot}%{_datadir}/icons/hicolor/48x48/apps/xu4.png
install -D -m 644 dist/xu4.desktop %{buildroot}%{_datadir}/applications/xu4.desktop
install -m 644 Ultima-IV.mod U4-Upgrade.mod render.pak %{buildroot}%{_datadir}/xu4
install -m 644 %{SOURCE2} %{buildroot}%{_datadir}/xu4

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README.md ChangeLog
%license COPYING
%dir %{_datadir}/xu4
%{_bindir}/xu4
%{_datadir}/icons/hicolor/48x48/apps/xu4.png
%{_datadir}/applications/xu4.desktop
%{_datadir}/xu4/render.pak
%{_datadir}/xu4/Ultima-IV.mod
%{_datadir}/xu4/U4-Upgrade.mod
%{_datadir}/xu4/u4upgrad.zip

%changelog
* Wed Jul 05 2023 Karl Robillard <wickedsmoke@users.sourceforge.net>
- Update for v1.3

* Mon Oct 31 2005 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- changed sourceforge mirror from aleron (defunct?) to easynews

* Sun Oct 02 2005 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- updated for v1.0beta3, minor cleanup

* Sun Nov 28 2004 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- flatten conf directory
- download u4 upgrade from sourceforge instead of www.moongates.com
- package ultima4 zip file as ultima4-1.01.zip

* Thu Nov 25 2004 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- kill libstdc++ for all executables
- added xml files in subdirectories to files section
- fixed to build utils again
- updated for 1.0beta1, kill libstdc++ dependency

* Sat Feb 28 2004 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- replaced lzwenc, lzwdec, rleenc, rledec with u4enc and u4dec

* Tue Feb 23 2004 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- updated for v0.9, minor cleanup

* Tue Jan 27 2004 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added sound files to files

* Thu Jan 08 2004 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- replaced ancient /etc/X11/applnk menu entry with more modern /usr/share/applications entry

* Thu Dec 10 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- split graphics into ega and vga subdirectories

* Thu Nov 12 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added zlib-devel to build dependencies

* Thu Oct 23 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added ultima 4 zipfile to project

* Wed Oct 22 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added graphics files, cleanup

* Sat Aug 23 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added music.xml to files

* Mon Jul 21 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added armors.xml and weapons.xml to files

* Tue Apr 29 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added tools.txt to documentation

* Mon Apr  7 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added tlkconv tool
- added SDL_mixer-devel to build dependencies

* Tue Feb 25 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- install config file plus encoding and decoding tools

* Mon Dec 12 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added SDL-devel to build dependencies

* Mon Sep 25 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added quiet flag (-q) to %setup rule to reduce visual clutter

* Mon Jun  4 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added dumpsavegame binary

* Mon Jun  4 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added music files

* Mon May 13 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added new doc files

* Mon May  6 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added pixmaps, desktop entry

* Mon Apr 23 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- updated description

* Mon Apr  8 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- initial revision
