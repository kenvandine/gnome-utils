bin_PROGRAMS = gcharmap

INCLUDES =\
	-DG_LOG_DOMAIN=\"gcharmap\"\
	-I$(srcdir)/../pixmaps\
	$(GNOME_UTILS_CFLAGS) \
	$(LIBBONOBOUI_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

gcharmap_SOURCES = \
        main.c\
        interface.c interface.h\
        menus.c menus.h\
        callbacks.c callbacks.h\
        asciiselect.c asciiselect.h

gcharmap_LDADD = $(GNOME_UTILS_LIBS) $(LIBBONOBOUI_LIBS) $(INTLLIBS)

man_MANS = gcharmap.1

EXTRA_DIST = $(gcharmap_SOURCES) gcharmap.1
