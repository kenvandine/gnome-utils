xmldir = $(pkgdatadir)
xml_DATA = logview-toolbar.xml logview-filter.ui

desktopdir = $(datadir)/applications
desktop_in_files = gnome-system-log.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
$(desktop_in_files): $(desktop_in_files:.desktop.in=.desktop.in.in)
	@sed -e "s|\@VERSION\@|@VERSION@|" $< > $@
@INTLTOOL_DESKTOP_RULE@

man_MANS = gnome-system-log.1

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gnome-system-log.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local: $(schemas_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		for p in $^ ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
		done \
	fi
endif

EXTRA_DIST = 			\
	gnome-system-log.desktop.in.in	\
	$(xml_DATA)		\
	$(man_MANS)		\
	$(schemas_in_files)

DISTCLEANFILES =		\
	$(desktop_DATA)		\
	$(desktop_in_files)	\
	$(schemas_DATA)
