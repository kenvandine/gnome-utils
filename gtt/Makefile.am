SUBDIRS = help

INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gtt

gtt_SOURCES = \
	dialog.c	\
	dialog.h	\
	main.c		\
	menus.c		\
	menus.h		\
	proj.c		\
	timer.c		\
	err.c		\
	menucmd.c	\
	options.c	\
	prop.c		\
	toolbar.c	\
	toolbar.h	\
	gtt.h		\
	menucmd.h	\
	app.c		\
	log.c		\
	clist.c

gtt_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

EXTRA_DIST = \
	gtt.desktop

Productivitydir = $(datadir)/gnome/apps/Productivity
Productivity_DATA = gtt.desktop


