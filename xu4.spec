Summary: xu4 - Ultima IV Recreated
Name: xu4
Version: 0.8
Release: 1
URL: http://xu4.sourceforge.net/
Source0: http://download.sourceforge.net/xu4/xu4-%{version}.tar.gz
License: GPL
Group: Amusements/Games
BuildRequires: SDL-devel SDL_mixer-devel libxml2-devel

BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

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
%setup -n u4 -q

%build
cd src && make bindir=%{_bindir} datadir=%{_datadir} libdir=%{_libdir}

%install
cd src && %{makeinstall} desktopdir=$RPM_BUILD_ROOT/etc/X11/applnk
cp ~/ultima4-1.01.zip $RPM_BUILD_ROOT/%{_libdir}/u4

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING doc/FileFormats.txt doc/tools.txt
%{_bindir}/u4
%{_datadir}/pixmaps/u4.bmp
%{_datadir}/pixmaps/u4.png
%{_libdir}/u4/music/*.mid
%{_libdir}/u4/dumpsavegame
%{_libdir}/u4/lzwenc
%{_libdir}/u4/lzwdec
%{_libdir}/u4/rleenc
%{_libdir}/u4/rledec
%{_libdir}/u4/tlkconv
%{_libdir}/u4/*.xml
%{_libdir}/u4/graphics/*.rle
%{_libdir}/u4/ultima4-1.01.zip
/etc/X11/applnk/Games/u4.desktop

%changelog
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
- added SDL_mixer-devel to build dependancies

* Tue Feb 25 2003 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- install config file plus encoding and decoding tools

* Mon Dec 12 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- added SDL-devel to build dependancies

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
