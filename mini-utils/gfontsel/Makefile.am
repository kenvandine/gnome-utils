SUBDIRS = help

Utilitiesdir = $(datadir)/gnome/apps/Utilities

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gfontsel

gfontsel_SOURCES = gfontsel.c

gfontsel_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-fontsel.png

EXTRA_DIST = gfontsel.desktop gnome-fontsel.png

Utilities_DATA = gfontsel.desktop
