SUBDIRS = glade
gladedir = $(datadir)/gcolorsel/glade
Utilitiesdir = $(datadir)/gnome/apps/Utilities

CFLAGS += -DGCOLORSEL_GLADEDIR=\""$(gladedir)/"\" -g

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gcolorsel

gcolorsel_SOURCES = 	gcolorsel.c			\
			gcolorsel.h			\
			utils.c				\
			utils.h				\
			menus.c				\
			menus.h				\
			mdi-color-generic.c		\
			mdi-color-generic.h		\
			mdi-color.c			\
			mdi-color.h			\
			mdi-color-file.c		\
			mdi-color-file.h		\
			mdi-color-virtual.c		\
			mdi-color-virtual.h		\
			session.c			\
			session.h			\
			view-color-edit.c		\
			view-color-edit.h		\
			view-color-generic.c		\
			view-color-generic.h		\
			view-color-list.c		\
			view-color-list.h		\
			view-color-grid.c		\
			view-color-grid.h		\
			widget-color-grid.c		\
			widget-color-grid.h		\
			widget-control-generic.c	\
			widget-control-generic.h	\
			widget-control-virtual.c	\
			widget-control-virtual.h

gcolorsel_LDADD = \
	$(GNOME_LIBDIR)  				\
	$(GNOMEUI_LIBS) 				\
	$(INTLLIBS)					\
	-lxml						\
	-lglade 					\
	-lglade-gnome

EXTRA_DIST = gcolorsel.desktop

Utilities_DATA = gcolorsel.desktop

CLEANFILES=*~
