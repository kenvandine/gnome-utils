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
	gnome-shutdown.png 

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gnome-shutdown.png

Systemdir = $(datadir)/gnome/apps/System
System_DATA = gshutdown.desktop
