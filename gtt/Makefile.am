SUBDIRS = doc

INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gtt

gtt_SOURCES = 		\
	app.c		\
	ctree.c		\
	dialog.c	\
	err.c		\
	err-throw.c	\
	file-io.c	\
	log.c		\
	main.c		\
	menucmd.c	\
	menus.c		\
	menus.h		\
	options.c	\
	proj.c		\
	prop.c		\
	shorts.c	\
	timer.c		\
	toolbar.c	\
	util.c		\
	xml-read.c	\
	xml-write.c

noinst_HEADERS = 	\
	ctree.h		\
	dialog.h	\
	err-throw.h	\
	file-io.h	\
	gtt.h		\
	menucmd.h	\
	proj.h		\
	proj_p.h	\
	shorts.h	\
	toolbar.h	\
	util.h		\
	xml-gtt.h
	

gtt_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOME_XML_LIB)        \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

EXTRA_DIST = 		\
	.cvsignore	\
	gtt.desktop 	\
	$(top_srcdir)/sgmldocs.make

Productivitydir = $(datadir)/gnome/apps/Applications
Productivity_DATA = gtt.desktop


