# Process this file with automake to produce Makefile.in

SUBDIRS = help

Utilitiesdir = $(datadir)/applications
additionaldir  = $(datadir)/gnome-system-log

INCLUDES = -DG_LOG_DOMAIN=\"gnome-system-log\"	$(GNOME_UTILS_CFLAGS) 		\
	-DGNOMELOCALEDIR=\""$(gnomeutilslocaledir)"\"			\
	-I$(top_srcdir)/logview/pixmaps\
	-DDATADIR=\""$(datadir)"\" \
	-DLOGVIEWINSTALLPREFIX=\""$(prefix)/\""

bin_PROGRAMS = gnome-system-log

gnome_system_log_SOURCES = 	\
	about.c		\
	actions.c	\
	calendar.c	\
	config.c	\
	desc_db.c       \
	info.c		\
	log_repaint.c	\
	logrtns.c	\
	logrtns.h	\
	logview.c	\
	logview.h	\
	menus.c		\
	misc.c		\
	monitor.c       \
	userprefsdialog.c       \
	zoom.c

gnome_system_log_LDADD = \
	$(GNOME_UTILS_LIBS)

man_MANS = gnome-system-log.1

Utilities_in_files = gnome-system-log.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemadir   = $(sysconfdir)/gconf/schemas
schemas_in_file = logview.schemas.in
schema_DATA = $(schemas_in_file:.schemas.in=.schemas)
schema_source_data = $(foreach file, $(schema_DATA), $(srcdir)/$(file))
@INTLTOOL_SCHEMAS_RULE@

additional_DATA  = gnome-system-log-descript.db \
                   gnome-system-log-regexp.db \
		   gnome-system-log-actions.db

EXTRA_DIST = \
	$(Utilities_in_files)	\
	$(Utilities_DATA)	\
	$(additional_DATA)	\
	$(schema_DATA) 		\
	gnome-system-log-pam		\
	gnome-system-log-security.in	\
	$(man_MANS)

install-gconf-schemas:
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_source_data) ; \
	fi

install-data-local: gnome-system-log-security install-gconf-schemas
	
if CONSOLE_HELPER
	$(INSTALL_DATA) -D gnome-system-log-pam $(DESTDIR)$(PAM_PREFIX)/pam.d/gnome-system-log
	$(INSTALL_DATA) -D gnome-system-log-security $(DESTDIR)$(PAM_PREFIX)/security/console.apps/gnome-system-log
	$(INSTALL_DATA) -m 755 -d $(DESTDIR)$(sbindir)
	mv -f $(DESTDIR)$(bindir)/gnome-system-log $(DESTDIR)$(sbindir)
	$(LN_S) @CONSOLE_HELPER@ $(DESTDIR)$(bindir)/gnome-system-log
endif
