SUBDIRS = doc glade phtml

bin_PROGRAMS = gtt

gtt_SOURCES = 		\
	app.c		\
	ctree.c		\
	dialog.c	\
	err.c		\
	err-throw.c	\
	file-io.c	\
	journal.c	\
	log.c		\
	main.c		\
	menucmd.c	\
	menus.c		\
	menus.h		\
	options.c	\
	phtml.c		\
	proj.c		\
	proj-prop.c	\
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
	journal.h	\
	menucmd.h	\
	phtml.h		\
	proj.h		\
	proj_p.h	\
	shorts.h	\
	toolbar.h	\
	util.h		\
	xml-gtt.h
	

CFLAGS = @CFLAGS@ \
	${GCONF_CFLAGS} \
	${GTKHTML_CFLAGS} \
	${GLADE_CFLAGS} 

INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)
						 
gtt_LDADD = \
	$(GLADE_LIBS)           \
	$(GNOME_LIBDIR)         \
	$(GNOME_XML_LIB)        \
	$(GTKHTML_LIBS)		\
	$(GNOMEUI_LIBS) 	\
	$(GCONF_LIBS)	 	\
	$(INTLLIBS)

EXTRA_DIST = 		\
	.cvsignore	\
	design.txt	\
	gtt.desktop 	\
	$(top_srcdir)/sgmldocs.make

Productivitydir = $(datadir)/gnome/apps/Applications
Productivity_DATA = gtt.desktop


