
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gtt

man_MANS = gtt.1

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
	topic.dat		\
	gtt-db.sgml		\
	gtt-db.html		\
	gtt.desktop		\
	gtt.1

Productivitydir = $(datadir)/apps/Productivity
Productivity_DATA = gtt.desktop


# maybe there is a macro for that?

gtt_helpdir = $(datadir)/gnome/help/gtt/C

install-data-local:
	 $(mkinstalldirs) $(gtt_helpdir)
	 $(INSTALL_DATA) $(srcdir)/topic.dat $(gtt_helpdir)/topic.dat
	 $(INSTALL_DATA) $(srcdir)/gtt-db.html $(gtt_helpdir)/index.html

