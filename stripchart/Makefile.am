SUBDIRS = doc

INCLUDES = -DG_LOG_DOMAIN=\"stripchart\" \
 -DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
 -DLOCALEDIR=\"$(datadir)/locale\" \
 -DCONFDIR=\"$(datadir)/stripchart\" \
 -I$(includedir) $(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS = stripchart stripchart-applet

stripchart_SOURCES = \
 stripchart.c \
 chart-app.c chart.c eval.c params.c pen.c prefs.c strip.c utils.c \
 chart-app.h chart.h eval.h params.h pen.h prefs.h strip.h utils.h

stripchart_LDADD = \
 $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS) \
 $(LIBGTOP_LIBS) $(LIBGTOP_EXTRA_LIBS) $(GNOME_XML_LIB) 

stripchart_applet_SOURCES = \
 stripchart-applet.c \
 chart-app.c chart.c eval.c params.c pen.c prefs.c strip.c utils.c \
 chart-app.h chart.h eval.h params.h pen.h prefs.h strip.h utils.h

stripchart_applet_LDADD = \
 $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS) $(GNOME_APPLETS_LIBS) \
 $(LIBGTOP_LIBS) $(LIBGTOP_EXTRA_LIBS) $(GNOME_XML_LIB) 

EXTRA_DIST = \
 gnome-stripchart.png stripchart.conf stripchart.params \
 stripchart.desktop stripchart-applet.desktop $(top_srcdir)/app-docs.make

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-stripchart.png

confdir = $(datadir)/stripchart
conf_DATA = stripchart.conf stripchart.params

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = stripchart.desktop

Appletdir = $(datadir)/applets/Monitors
Applet_DATA = stripchart-applet.desktop

sanitize:	clean
	$(RM) core *~ doc/C/*.html doc/C/*~
	$(RM) -r .deps doc/C/stripchart doc/C/stripchart.junk