SUBDIRS = src pixmaps help

desktopdir = $(datadir)/applications
desktop_in_files = gnome-character-map.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gcharmap.schemas

EXTRA_DIST = \
	BUGS \
	$(desktop_in_files) \
	$(desktop_DATA)	\
	$(schema_DATA)


install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
