# Process this file with automake to produce Makefile.am

SUBDIRS = doc

INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gw

gw_SOURCES = \
	gw.c

gw_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

Systemdir = $(datadir)/gnome/apps/System
System_DATA = gw.desktop

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-who.png

EXTRA_DIST = gw.desktop gnome-who.png
