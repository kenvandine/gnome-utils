# Note that this is NOT a relocatable package
%define ver      0.30
%define rel      1
%define prefix   /usr

Summary: GNOME utility programs
Name: gnome-utils
Version: %ver
Release: %rel
Copyright: LGPL
Group: X11/Libraries
Source: ftp://ftp.gnome.org/pub/GNOME/sources/gnome-utils-%{ver}.tar.gz
BuildRoot: /var/tmp/gnome-utils-root
Obsoletes: gnome
Requires: gnome-libs >= 0.30
Requires: libgtop >= 0.25.0

Packager: Michael Fulbright <msf@redhat.com>
URL: http://www.gnome.org
Docdir: %{prefix}/doc

%description
GNOME utility programs.

GNOME is the GNU Network Object Model Environment.  That's a fancy
name but really GNOME is a nice GUI desktop environment.  It makes
using your computer easy, powerful, and easy to configure.

%changelog

* Tue Sep 22 1998 Michael Fulbright <msf@redhat.com>

- Update to gnome-libs-0.30

* Mon Apr  6 1998 Marc Ewing <marc@redhat.com>

- Integrate into gnome-utils CVS source tree

%prep
%setup

%build
# Needed for snapshot releases.
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=%prefix
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%doc AUTHORS COPYING ChangeLog NEWS README
%{prefix}/bin/*
%{prefix}/man/*/*

%{prefix}/lib/*

%{prefix}/share/locale/*/*/*
%{prefix}/share/apps
%{prefix}/share/gnome/help/*
