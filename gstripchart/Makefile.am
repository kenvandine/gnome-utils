
#CFLAGS += -Wall

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
	$(LIBGTOP_EXTRA_LIBS)


EXTRA_DIST = 						\
	gnome-stripchart.png            \
	gstripchart.conf				\
	gstripchart.desktop	 			\
	gstripchart.html				\
	topic.dat

pixmapdir =                         \
    $(datadir)/pixmaps

pixmap_DATA =                       \
    gnome-stripchart.png

Utilitiesdir =						\
	$(datadir)/gnome/apps/Utilities

Utilities_DATA =					\
	gstripchart.desktop

help_base =						\
	$(datadir)/gnome/help/gstripchart

install-data-local:
	$(mkinstalldirs) $(datadir)/gstripchart
	$(INSTALL_DATA) $(srcdir)/gstripchart.conf $(datadir)/gstripchart/
	$(mkinstalldirs) $(help_base)/C
	$(INSTALL_DATA) $(srcdir)/topic.dat $(help_base)/C/
	$(INSTALL_DATA) $(srcdir)/gstripchart.html $(help_base)/C/
	rm -f $(help_base)/C/index.html
	cd $(help_base)/C && $(LN_S) gstripchart.html index.html     
