# Note that this is NOT a relocatable package
%define ver      0.13
%define rel      SNAP
%define prefix   /usr

Summary: GNOME utility programs
Name: gnome-utils
Version: %ver
Release: %rel
Copyright: LGPL
Group: X11/Libraries
Source: ftp://ftp.gnome.org/pub/gnome-utils-%{ver}.tar.gz
BuildRoot: /tmp/gnome-utils-root
Obsoletes: gnome
Packager: Marc Ewing <marc@redhat.com>
URL: http://www.gnome.org
Docdir: %{prefix}/doc

%description
GNOME utility programs.

GNOME is the GNU Network Object Model Environment.  That's a fancy
name but really GNOME is a nice GUI desktop environment.  It makes
using your computer easy, powerful, and easy to configure.

%changelog

* Mon Apr 6 1998 Marc Ewing <marc@redhat.com>

- Integrate into gnome-utils CVS source tree

%prep
%setup
patch -p1 << EOF
--- gnome-utils/Makefile.am.msf	Mon May  4 20:35:51 1998
+++ gnome-utils/Makefile.am	Mon May  4 20:36:18 1998
@@ -1,10 +1,10 @@
 if GUILE
-guile_DIRS = cromagnon find-file notepad
+guile_DIRS = #cromagnon find-file notepad
 endif
 
 SUBDIRS = po intl macros \
-	edit-menus @PROGRAMS_GENIUS@ gcalc ghex gncal gedit \
-	@PROGRAMS_GTOP@ gtt mini-utils $(guile_DIRS)
+	@PROGRAMS_GENIUS@ gcalc ghex gncal \
+	@PROGRAMS_GTOP@ gtt $(guile_DIRS)
 
 ## to automatically rebuild aclocal.m4 if any of the macros in
 ## `macros/' change
EOF

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
%{prefix}/share/apps/*/*
%{prefix}/share/gtoprc
%{prefix}/share/gnome/help/*
