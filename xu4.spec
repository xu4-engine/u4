Name:      xu4
Version:   1.0beta3
Release:   1
Epoch:     0
Summary:   xu4 - Ultima IV Recreated

Group:     Amusements/Games
License:   GPL
URL:       http://xu4.sourceforge.net/
Source0:   http://download.sourceforge.net/xu4/xu4-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Prefix:    /usr

BuildRequires: SDL-devel SDL_mixer-devel libxml2-devel zlib-devel

%description
XU4 is a recreation of the classic computer game Ultima IV. The
purpose of the project is to make it easy and convenient to play on
modern operating systems.  xu4 is primarily inspired by the much more
ambitious project Exult.  Linux is the primary development platform,
but it should be trivial to port to any system with SDL support.

XU4 isn't a new game based on the Ultima IV story -- it is a faithful
recreation of the old game, right up to the crappy graphics.  If you
are looking for a game with modern gameplay and graphics, this is not
it -- yet.  New features that improve the gameplay and keep with the
spirit of the original game will be added.

%prep
%setup -n u4

%build
cd src && make bindir=%{_bindir} datadir=%{_datadir} libdir=%{_libdir} all.static_gcc_libs

%install
cd src && %{makeinstall}
wget http://aleron.dl.sourceforge.net/sourceforge/xu4/ultima4-1.01.zip -O %{buildroot}/%{_libdir}/u4/ultima4-1.01.zip
wget http://aleron.dl.sourceforge.net/sourceforge/xu4/u4upgrad.zip -O %{buildroot}/%{_libdir}/u4/u4upgrad.zip

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING doc/FileFormats.txt doc/tools.txt
%{_bindir}/u4
%{_datadir}/pixmaps/u4.bmp
%{_datadir}/pixmaps/u4.png
%{_datadir}/applications/u4.desktop
%{_libdir}/u4/music/*.mid
%{_libdir}/u4/sound/*.wav
%{_libdir}/u4/sound/*.ogg
%{_libdir}/u4/dumpsavegame
%{_libdir}/u4/u4enc
%{_libdir}/u4/u4dec
%{_libdir}/u4/tlkconv
%{_libdir}/u4/*.xml
%{_libdir}/u4/dtd/*.dtd
%{_libdir}/u4/graphics/ega/*.rle
%{_libdir}/u4/graphics/ega/*.png
%{_libdir}/u4/graphics/vga/*.png
%{_libdir}/u4/ultima4-1.01.zip
%{_libdir}/u4/u4upgrad.zip

%changelog
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
