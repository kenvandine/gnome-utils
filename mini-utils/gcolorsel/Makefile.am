Utilitiesdir = $(datadir)/apps/Utilities

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gcolorsel

gcolorsel_SOURCES = gcolorsel.c

gcolorsel_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

EXTRA_DIST = gcolorsel.desktop gnome-color-browser.png

Utilities_DATA = gcolorsel.desktop

CLEANFILES=*~
