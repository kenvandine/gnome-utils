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

EXTRA_DIST = \
	gshutdown.desktop \
	gnome-shutdown.png  \
	$(top_srcdir)/sgmldocs.make \
	gshutdown-security.in

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gnome-shutdown.png

Systemdir = $(datadir)/gnome/apps/System
System_DATA = gshutdown.desktop

install-data-local: gshutdown-security
if CONSOLE_HELPER
	$(INSTALL_DATA) -D gshutdown-pam @PAM_PREFIX@/pam.d/gshutdown
	$(INSTALL_DATA) -D gshutdown-security @PAM_PREFIX@/security/console.apps/gshutdown
	$(INSTALL_DATA) -m 755 -d $(sbindir)
	mv $(bindir)/gshutdown $(sbindir)
	$(LN_S) @CONSOLE_HELPER@ $(bindir)/gshutdown
endif
