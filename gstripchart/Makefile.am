SUBDIRS = doc

INCLUDES = -DG_LOG_DOMAIN=\"gstripchart\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DCONFDIR=\"$(datadir)/gstripchart\" \
	-I$(includedir) $(GNOME_INCLUDEDIR) $(LIBGTOP_INCS) \
	$(GSTRIPCHART_APPLET_CFLAGS)

bin_PROGRAMS = gstripchart
gstripchart_SOURCES = gstripchart.c

gstripchart_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS) \
	$(LIBGTOP_LIBS) $(LIBGTOP_EXTRA_LIBS) $(GSTRIPCHART_APPLET_LIBS)

EXTRA_DIST = \
	gnome-stripchart.png gstripchart.conf \
	gstripchart.desktop.in gstripchart_applet.desktop.in \
	gstripchart_applet.gnorba $(top_srcdir)/sgmldocs.make

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-stripchart.png

confdir = $(datadir)/gstripchart
conf_DATA = gstripchart.conf

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_in_files = gstripchart.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

if BUILD_GSTRIPCHART_APPLET

GSTRIPCHART_APPLET_CFLAGS = -DHAVE_GNOME_APPLET
GSTRIPCHART_APPLET_LIBS = $(GNOME_APPLETS_LIBS)

sysdir = $(datadir)/applets/Monitors
sys_in_files = gstripchart_applet.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)

gnorbadir  = $(sysconfdir)/CORBA/servers
gnorba_DATA = gstripchart_applet.gnorba

install-exec-local:
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f gstripchart_applet && \
	$(LN_S) gstripchart gstripchart_applet

endif
