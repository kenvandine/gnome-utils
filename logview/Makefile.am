# Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

Utilitiesdir = $(datadir)/gnome/apps/System
additionaldir  = $(datadir)/logview

INCLUDES = -DG_LOG_DOMAIN=\"logview\"					\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-I$(includedir) $(GNOME_INCLUDEDIR) -I$(top_srcdir)/logview/pixmaps\
	-DLOGVIEWINSTALLPREFIX=\""$(prefix)/\""

bin_PROGRAMS = logview

logview_SOURCES = 	\
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

logview_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

Utilities_DATA = logview.desktop

additional_DATA  = logview-descript.db \
                   logview-regexp.db \
		   logview-actions.db

EXTRA_DIST = \
	logview.desktop		\
	logview-descript.db	\
	logview-regexp.db	\
	logview-actions.db	\
	logview-pam		\
	logview-security.in

install-data-local: logview-security
if CONSOLE_HELPER
	$(INSTALL_DATA) -D logview-pam @PAM_PREFIX@/pam.d/logview
	$(INSTALL_DATA) -D logview-security @PAM_PREFIX@/security/console.apps/logview
	$(INSTALL_DATA) -m 755 -d $(sbindir)
	mv -f $(bindir)/logview $(sbindir)
	$(LN_S) @CONSOLE_HELPER@ $(bindir)/logview
endif
