SUBDIRS = help

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
	topic.dat.es		\
	gshutdown.desktop

gshutdown_helpdir = $(datadir)/gnome/help/gshutdown/C
gshutdown_help_DATA = gshutdown.html

gshutdown_es_helpdir = $(datadir)/gnome/help/gshutdown/es
gshutdown_es_help_DATA = gshutdown.es.html 

install-data-local:
	 $(mkinstalldirs) $(gshutdown_helpdir)
	 $(INSTALL_DATA) $(srcdir)/topic.dat $(gshutdown_helpdir)/topic.dat
	 $(INSTALL_DATA) $(srcdir)/topic.dat.es $(gshutdown_es_helpdir)/topic.dat

Systemdir = $(datadir)/apps/System
System_DATA = gshutdown.desktop
