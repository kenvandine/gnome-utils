SUBDIRS = doc

INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gshutdown

gshutdown_SOURCES = \
	gshutdown.c

gshutdown_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

man_MANS = gshutdown.8

EXTRA_DIST = \
	gshutdown.desktop \
	gnome-shutdown.png  \
	gshutdown.8 \
	$(top_srcdir)/sgmldocs.make \
	gshutdown-security.in \
	gshutdown-pam

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gnome-shutdown.png

Systemdir = $(datadir)/gnome/apps/System
System_DATA = gshutdown.desktop

install-data-local: gshutdown-security
if CONSOLE_HELPER
	$(INSTALL_DATA) -D gshutdown-pam $(DESTDIR)$(PAM_PREFIX)/pam.d/gshutdown
	$(INSTALL_DATA) -D gshutdown-security $(DESTDIR)$(PAM_PREFIX)/security/console.apps/gshutdown
	$(INSTALL_DATA) -m 755 -d $(sbindir)
	mv -f $(bindir)/gshutdown $(sbindir)
	$(LN_S) @CONSOLE_HELPER@ $(bindir)/gshutdown
endif
