
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = idetool

idetool_SOURCES = \
	idetool.c

idetool_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

pixmapdir = $(datadir)/pixmaps

BITMAPS = ide-disk-drive.png

pixmap_DATA = $(BITMAPS)

EXTRA_DIST  = $(BITMAPS)
