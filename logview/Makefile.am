# Process this file with automake to produce Makefile.in

SUBDIRS = data help tests

Utilitiesdir = $(datadir)/applications

INCLUDES = \
	$(GNOME_UTILS_CFLAGS)					\
	$(LIBGNOME_CFLAGS)					\
	-DG_LOG_DOMAIN=\"gnome-system-log\"			\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DDATADIR=\""$(datadir)"\"				\
	-DLOGVIEWINSTALLPREFIX=\""$(prefix)/\""			\
	-DLOGVIEW_DATADIR=\""$(pkgdatadir)"\"			\
	$(SUN_OS)						\
	$(NULL)

bin_PROGRAMS = gnome-system-log

BUILT_SOURCES = 		\
	logview-marshal.c	\
	logview-marshal.h

gnome_system_log_SOURCES = 	\
	logview-main.c		\
	logview-about.h		\
	logview-manager.c	\
	logview-manager.h	\
	logview-utils.c		\
	logview-utils.h		\
	logview-loglist.c	\
	logview-loglist.h	\
	logview-window.c	\
	logview-window.h	\
	logview-log.h		\
	logview-log.c		\
	logview-findbar.h	\
	logview-findbar.c	\
	logview-prefs.c		\
	logview-prefs.h		\
	$(BUILT_SOURCES)

gnome_system_log_LDADD = \
	$(GNOME_UTILS_LIBS)	\
	$(LIBGNOME_LIBS)

logview-marshal.h: logview-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=logview_marshal >> $@

logview-marshal.c: logview-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"logview-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=logview_marshal >> $@

man_MANS = gnome-system-log.1

Utilities_in_files = gnome-system-log.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)

$(Utilities_in_files): $(Utilities_in_files:.desktop.in=.desktop.in.in)
	@sed -e "s|\@VERSION\@|@VERSION@|" $< > $@

@INTLTOOL_DESKTOP_RULE@

schemasdir   = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_file = logview.schemas.in
schemas_DATA = $(schemas_in_file:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	gnome-system-log.desktop.in.in	\
	$(schemas_in_file) 		\
	$(man_MANS)

if GCONF_SCHEMAS_INSTALL
install-gconf-schemas:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/logview/$$p ; \
		done \
	fi
else
install-gconf-schemas:
endif

install-data-local: install-gconf-schemas

CLEANFILES = 				\
	gnome-system-log.desktop	\
	gnome-system-log.desktop.in	\
	logview.schemas			\
	$(BUILT_SOURCES)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)

