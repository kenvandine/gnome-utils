
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gnome-run

gnome_run_SOURCES = \
	gnome-run.c

gnome_run_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

Rootdir = $(datadir)/gnome/apps
Root_DATA = gnome-run.desktop

EXTRA_DIST = gnome-run.desktop $(top_srcdir)/sgmldocs.make
