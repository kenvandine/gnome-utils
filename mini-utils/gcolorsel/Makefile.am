Utilitiesdir = $(datadir)/gnome/apps/Utilities

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gcolorsel

gcolorsel_SOURCES = gcolorsel.c			        \
		    utils.c				\
		    utils.h				\
		    widget-color-list.c 		\
	            widget-color-list.h 		\
		    widget-color-search.c 		\
		    widget-color-search.h 		\
		    widget-editable-label.c		\
		    widget-editable-label.h

gcolorsel_LDADD = \
	$(GNOME_LIBDIR)  				\
	$(GNOMEUI_LIBS) 				\
	$(INTLLIBS)

EXTRA_DIST = gcolorsel.desktop

Utilities_DATA = gcolorsel.desktop

CLEANFILES=*~
