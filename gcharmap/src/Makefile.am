# Process this file with automake to produce Makefile.in

bin_PROGRAMS = gcharmap

INCLUDES = -DG_LOG_DOMAIN=\"gcharmap\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR) 		\
	-I$(top_srcdir)/gcharmap/pixmaps

gcharmap_SOURCES = \
        asciiselect.c  asciiselect.h \
        interface.c    interface.h \
        main.c

gcharmap_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

