
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
Utilities_in_files = gpenguin.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

EXTRA_DIST = \
	$(pixmap_DATA) \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(top_srcdir)/sgmldocs.make
