# Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

Utilitiesdir = $(datadir)/applications
additionaldir  = $(datadir)/gnome-system-log

INCLUDES = -DG_LOG_DOMAIN=\"gnome-system-log\"	$(GNOME_UTILS_CFLAGS) 		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-I$(includedir) -I$(top_srcdir)/logview/pixmaps\
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

additional_DATA  = gnome-system-log-descript.db \
                   gnome-system-log-regexp.db \
		   gnome-system-log-actions.db

EXTRA_DIST = \
	$(Utilities_in_files)	\
	$(Utilities_DATA)	\
	$(additional_DATA)	\
	gnome-system-log-pam		\
	gnome-system-log-security.in	\
	$(man_MANS)

install-data-local: gnome-system-log-security
if CONSOLE_HELPER
	$(INSTALL_DATA) -D gnome-system-log-pam $(DESTDIR)$(PAM_PREFIX)/pam.d/gnome-system-log
	$(INSTALL_DATA) -D gnome-system-log-security $(DESTDIR)$(PAM_PREFIX)/security/console.apps/gnome-system-log
	$(INSTALL_DATA) -m 755 -d $(sbindir)
	mv -f $(bindir)/gnome-system-log $(sbindir)
	$(LN_S) @CONSOLE_HELPER@ $(bindir)/gnome-system-log
endif
