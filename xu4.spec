Summary: xu4 - Ultima IV Recreated
Name: xu4
Version: 0.05
Release: 1
URL: http://xu4.sourceforge.net/
Source0: http://download.sourceforge.net/xu4/xu4-%{version}.tar.gz
License: GPL
Group: Amusements/Games
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
%setup -n u4

%build
cd src && make datadir=%{_datadir} libdir=%{_libdir}

%install
cd src && %{makeinstall} desktopdir=$RPM_BUILD_ROOT/etc/X11/applnk


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING doc/FileFormats.txt
%{_bindir}/u4
%{_datadir}/pixmaps/u4.bmp
%{_datadir}/pixmaps/u4.png
%{_libdir}/u4/music/Castles.mid
%{_libdir}/u4/music/Combat.mid
%{_libdir}/u4/music/Dungeon.mid
%{_libdir}/u4/music/Fanfare_Of_Lord_British.mid
%{_libdir}/u4/music/Shopping.mid
%{_libdir}/u4/music/Shrines.mid
%{_libdir}/u4/music/Towns.mid
%{_libdir}/u4/music/Wanderer.mid
/etc/X11/applnk/Games/u4.desktop

%changelog
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
