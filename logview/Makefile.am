# Process this file with automake to produce Makefile.in

SUBDIRS = help

Utilitiesdir = $(datadir)/applications
additionaldir  = $(datadir)/gnome-system-log

INCLUDES = \
	$(GNOME_UTILS_CFLAGS)					\
	-DG_LOG_DOMAIN=\"gnome-system-log\"			\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DDATADIR=\""$(datadir)"\"				\
	-DLOGVIEWINSTALLPREFIX=\""$(prefix)/\""

bin_PROGRAMS = gnome-system-log

gnome_system_log_SOURCES = 	\
	about.c			\
	about.h			\
	calendar.c		\
	calendar.h		\
	info.c			\
	info.h			\
	log_repaint.c		\
	log_repaint.h		\
	logrtns.c		\
	logrtns.h		\
	logview.c		\
	logview.h		\
	misc.c			\
	misc.h			\
	monitor.c       	\
	monitor.h		\
	userprefs.c             \
	userprefs.h		\
	logview-findbar.c	\
	logview-findbar.h 

gnome_system_log_LDADD = \
	$(GNOME_UTILS_LIBS)

man_MANS = gnome-system-log.1

Utilities_in_files = gnome-system-log.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemasdir   = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_file = logview.schemas.in
schemas_DATA = $(schemas_in_file:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

additional_DATA  = gnome-system-log-descript.db \
                   gnome-system-log-regexp.db \
		   gnome-system-log-actions.db

EXTRA_DIST = \
	$(Utilities_in_files)	\
	$(Utilities_DATA)	\
	$(additional_DATA)	\
	$(schemas_in_file) 		\
	gnome-system-log-pam		\
	gnome-system-log-security.in	\
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

install-data-local: gnome-system-log-security install-gconf-schemas

if CONSOLE_HELPER
	$(INSTALL_DATA) -D gnome-system-log-pam $(DESTDIR)$(PAM_PREFIX)/pam.d/gnome-system-log
	$(INSTALL_DATA) -D gnome-system-log-security $(DESTDIR)$(PAM_PREFIX)/security/console.apps/gnome-system-log
	$(INSTALL_DATA) -m 755 -d $(DESTDIR)$(sbindir)
	mv -f $(DESTDIR)$(bindir)/gnome-system-log $(DESTDIR)$(sbindir)
	$(LN_S) @CONSOLE_HELPER@ $(DESTDIR)$(bindir)/gnome-system-log
endif

CLEANFILES = 				\
	gnome-system-log.desktop	\
	logview.schemas

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)

