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
	plug-in.c	\
	proj.c		\
	props-invl.c	\
	props-proj.c	\
	props-task.c	\
	shorts.c	\
	timer.c		\
	toolbar.c	\
	util.c		\
	xml-read.c	\
	xml-write.c

noinst_HEADERS = 	\
	ctree.h		\
	cur-proj.h	\
	dialog.h	\
	err-throw.h	\
	file-io.h	\
	gtt.h		\
	journal.h	\
	menucmd.h	\
	phtml.h		\
	plug-in.h	\
	proj.h		\
	proj_p.h	\
	props-invl.h	\
	props-proj.h	\
	props-task.h	\
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

Productivitydir = $(datadir)/gnome/apps/Applications
Productivity_in_files = gtt.desktop.in
Productivity_DATA = $(Productivity_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

EXTRA_DIST = 		\
	down.xpm	\
	left.xpm	\
	design.txt	\
	$(Productivity_in_files) \
	$(Productivity_DATA) \
	$(top_srcdir)/sgmldocs.make

