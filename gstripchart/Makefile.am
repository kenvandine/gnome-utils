SUBDIRS = help

INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-DCONFDIR=\"$(datadir)/gstripchart\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS =						\
	gstripchart

gstripchart_SOURCES =					\
	gstripchart.c

gstripchart_LDADD =					\
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(LIBGTOP_LIBS)					\
	$(LIBGTOP_EXTRA_LIBS)				\
	$(GNOME_APPLETS_LIBS)

EXTRA_DIST = 						\
	gnome-stripchart.png				\
	gstripchart.conf

pixmapdir =						\
	$(datadir)/pixmaps

pixmap_DATA =						\
	gnome-stripchart.png

confdir =						\
	$(datadir)/gstripchart

conf_DATA =						\
	gstripchart.conf

Utilitiesdir =						\
	$(datadir)/gnome/apps/Utilities

Utilities_DATA =					\
	gstripchart.desktop
