Name:         gnome-utils
License:      GPL
Group:        System/GUI/GNOME
Version:      2.6.0
Release:      1
Distribution: Vanilla GNOME
Vendor:       The GNOME Foundation
Summary:      Basic Utilities for the GNOME 2.0 Desktop
Source:       http://ftp.gnome.org/pub/GNOME/sources/gnome-utils/2.6/%{name}-%{version}.tar.bz2
URL:          http://www.gnome.org
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
Docdir:       %{_defaultdocdir}/%{name}
Autoreqprov:  on
Prereq:       scrollkeeper

%define glib2_version 2.4.0
%define pango_version 1.3.1
%define gtk2_version 2.4.0
%define libgnome_version 2.6.0
%define libgnomeui_version 2.6.0
%define gail_version 1.4.0
%define gnome_panel_version 2.6.0
%define scrollkeeper_version 0.3.12

Requires:       libgnome >= %{libgnome_version}
Requires:       libgnomeui >= %{libgnomeui_version}
Requires:       scrollkeeper >= %{scrollkeeper_version}
BuildRequires:  glib2-devel >= %{glib2_version}
BuildRequires:  pango-devel >= %{pango_version}
BuildRequires:  gtk2-devel >= %{gtk2_version}
BuildRequires:  libgnome-devel >= %{libgnome_version}
BuildRequires:  libgnomeui-devel >= %{libgnomeui_version}
BuildRequires:  gail-devel >= %{gail_version}
BuildRequires:  gnome-panel >= %{gnome_panel_version}
BuildRequires:  scrollkeeper >= %{scrollkeeper_version}
BuildRequires:  e2fsprogs-devel
BuildRequires:  pam

%description
This package contains some essential utilities for the GNOME 2 Desktop.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS" \
  ./configure \
	--prefix=%{_prefix} \
	--sysconfdir=%{_sysconfdir} \
        --libexecdir=%{_libexecdir} \
	--localstatedir=/var/lib    \
	--enable-console-helper=yes
make

%install
export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
make -i install DESTDIR=$RPM_BUILD_ROOT
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL 

%clean
rm -rf $RPM_BUILD_ROOT

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
SCHEMAS=" gdict.schemas gfloppy.schemas gnome-search-tool.schemas"
for S in $SCHEMAS; do
        gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/$S >/dev/null
done
for i in zh_CN zh_TW ko_KR ja_JP de_DE es_ES fr_FR it_IT sv_SE ; do
        langtag=$i
        [ ${i:0:2} == "zh" ] || langtag=${i:0:2}
        [ -e  %{_datadir}/omf/gnome-utils/gnome-search-tool-$langtag.omf ] && \
        env LANG=$i LC_ALL=$i scrollkeeper-install -q %{_datadir}/omf/gnome-utils/gnome-search-tool-$langtag.omf
done
scrollkeeper-update -q

%postun
for i in zh_CN zh_TW ko_KR ja_JP de_DE es_ES fr_FR it_IT sv_SE ; do
        langtag=$i
        [ ${i:0:2} == "zh" ] || langtag=${i:0:2}
        [ -e  %{_datadir}/omf/gnome-utils/gnome-search-tool-$langtag.omf ] && \
        env LANG=$i LC_ALL=$i scrollkeeper-uninstall -q %{_datadir}/omf/gnome-utils/gnome-search-tool-$langtag.omf
done
scrollkeeper-update -q

%files
%defattr (-, root, root)
%{_bindir}/*
%{_datadir}/locale/*/LC_MESSAGES/*.mo
%{_datadir}/omf/gnome-utils
%{_datadir}/gnome/help
%{_datadir}/gnome-2.0/ui/*
%{_sysconfdir}/gconf/schemas
%{_libdir}/bonobo
%{_datadir}/applications
%{_datadir}/gnome-utils
%{_libexecdir}/*
%{_datadir}/mime-info/*

%changelog
* Mon Mar 22 2004 - glynn.foster@sun.com
- Attempt to resurrect this into something even
  slightly useful, even though I'm certain no 
  one uses it.
