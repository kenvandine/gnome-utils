Utilitiesdir = $(datadir)/gnome/apps/Utilities

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gfontsel

gfontsel_SOURCES = gfontsel.c

gfontsel_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

EXTRA_DIST = gfontsel.desktop

Utilities_DATA = gfontsel.desktop
