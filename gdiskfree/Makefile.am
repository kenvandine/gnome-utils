INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS = gdiskfree

gdiskfree_SOURCES =				\
	df.c					\
	util.c					\
	util.h					\
	pathmax.h				\
	xgetcwd.c				\
	save-cwd.c				\
	save-cwd.h


gdiskfree_LDADD = $(GNOME_LIBDIR) $(GTKXMHTML_LIBS) \
		  $(GNOMEUI_LIBS) $(INTLLIBS) \
		  $(LIBGTOP_LIBS) $(LIBGTOP_EXTRA_LIBS)

EXTRA_DIST = gnome-diskfree.png gdiskfree.desktop

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-diskfree.png

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = gdiskfree.desktop

