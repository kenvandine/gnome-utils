
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
	topic.dat		\
	gshutdown.html		\
	gshutdown.desktop

gshutdown_helpdir = $(datadir)/gnome/help/gshutdown/C

install-data-local:
	 $(mkinstalldirs) $(gshutdown_helpdir)
	 $(INSTALL_DATA) $(srcdir)/topic.dat $(gshutdown_helpdir)/topic.dat
	 $(INSTALL_DATA) $(srcdir)/gshutdown.html $(gshutdown_helpdir)/index.html

Systemdir = $(datadir)/apps/System
System_DATA = gshutdown.desktop
