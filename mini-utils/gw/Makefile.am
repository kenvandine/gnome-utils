
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gw

gw_SOURCES = \
	gw.c

gw_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = gw.desktop

EXTRA_DIST = gw.desktop
