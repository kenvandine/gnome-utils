
INCLUDES = 			\
	-I$(includedir)		\
        $(GNOME_INCLUDEDIR)	

bin_PROGRAMS = gpenguin

gpenguin_SOURCES = \
	gpenguin.c

gpenguin_LDADD = \
        $(GNOME_LIBDIR)         \
        $(GNOMEUI_LIBS)		\
	$(INTLLIBS)

pixmapdir = $(datadir)/pixmaps

BITMAPS = penguin1.png

pixmap_DATA = $(BITMAPS)

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = gpenguin.desktop

EXTRA_DIST = penguin1.png gpenguin.desktop $(top_srcdir)/app-docs.make

