SUBDIRS = help

INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-DCONFDIR=\"$(datadir)/gstripchart\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS = gstripchart
gstripchart_SOURCES = gstripchart.c

gstripchart_LDADD =					\
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(LIBGTOP_LIBS)					\
	$(LIBGTOP_EXTRA_LIBS)				\
	$(GNOME_APPLETS_LIBS)

EXTRA_DIST = 						\
	gnome-stripchart.png				\
	gstripchart.conf				\
	gstripchart_applet.desktop			\
	gstripchart_applet.gnorba

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-stripchart.png

confdir = $(datadir)/gstripchart
conf_DATA = gstripchart.conf

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = gstripchart.desktop

if HAVE_GNOME_APPLET

CFLAGS += -DHAVE_GNOME_APPLET

sysdir = $(datadir)/applets/Monitors
sys_DATA = gstripchart_applet.desktop

gnorbadir  = $(sysconfdir)/CORBA/servers
gnorba_DATA = gstripchart_applet.gnorba

install-exec-local:
	mkdir -p $(bindir)
	cd $(bindir) &&					\
	rm -f gstripchart_applet &&			\
	$(LN_S) gstripchart gstripchart_applet

endif
