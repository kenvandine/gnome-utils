
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
Root_in_files = gnome-run.desktop.in
Root_DATA = $(Root_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

EXTRA_DIST = gnome-run.desktop.in $(top_srcdir)/sgmldocs.make
