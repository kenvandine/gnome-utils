bin_PROGRAMS = gcharmap

INCLUDES =\
	-DG_LOG_DOMAIN=\"gcharmap\"\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"\
	-I$(includedir)\
	-I$(top_srcdir)/gcharmap/pixmaps\
	$(GNOME_INCLUDEDIR)

gcharmap_SOURCES = \
        main.c\
        interface.c interface.h\
        menus.c menus.h\
        callbacks.c callbacks.h\
        asciiselect.c asciiselect.h

gcharmap_LDADD = $(GNOME_LIBDIR) $(GNOME_LIBS) $(GNOMEUI_LIBS) $(INTLLIBS)

EXTRA_DIST = $(gcharmap_SOURCES)
