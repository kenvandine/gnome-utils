SUBDIRS = src pixmaps help

desktopdir = $(datadir)/applications
desktop_in_files = gcharmap.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	BUGS \
	$(desktop_in_files) \
	$(desktop_DATA)
