Utilitiesdir = $(datadir)/gnome/apps/Utilities

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gcolorsel

gcolorsel_SOURCES = gcolorsel.c			        \
		    gcolorsel.h				\
		    utils.c				\
		    utils.h				\
		    menus.c				\
		    menus.h				\
		    mdi-color-generic.c			\
		    mdi-color-generic.h			\
		    mdi-color-file.c			\
		    mdi-color-file.h			\
		    mdi-color-virtual.c			\
		    mdi-color-virtual.h			\
		    widget-color-list.c 		\
	            widget-color-list.h 		\
		    widget-color-grid.c			\
		    widget-color-grid.h			\
		    widget-control-generic.c		\
		    widget-control-generic.h		\
		    widget-control-virtual.c		\
		    widget-control-virtual.h

gcolorsel_LDADD = \
	$(GNOME_LIBDIR)  				\
	$(GNOMEUI_LIBS) 				\
	$(INTLLIBS)

EXTRA_DIST = gcolorsel.desktop

Utilities_DATA = gcolorsel.desktop

CLEANFILES=*~
