xmldir = $(pkgdatadir)
xml_DATA = logview-toolbar.xml

desktopdir = $(datadir)/applications
desktop_in_files = gnome-system-log.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

man_MANS = gnome-system-log.1

EXTRA_DIST = 			\
	$(xml_DATA)		\
	$(desktop_in_files)	\
	$(man_MANS)

DISTCLEANFILES =		\
	$(desktop_DATA)
