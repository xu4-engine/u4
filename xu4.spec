Summary: xu4 - Ultima IV Recreated
Name: xu4
Version: 0.01
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
it -- sorry.

%prep
%setup -n u4

%build
cd src && make

%install
cd src && %{makeinstall}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README doc/FileFormats.txt
%{_bindir}/u4

%changelog
* Mon Apr  8 2002 Andrew Taylor <andrewtaylor@users.sourceforge.net> 
- initial revision
