
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DHAS_GNOME=1		\
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
	tb_copy.xpm		\
	tb_new.xpm		\
	tb_preferences.xpm	\
	tb_timer.xpm		\
	tb_cut.xpm		\
	tb_open.xpm		\
	tb_properties.xpm	\
	tb_prop_dis.xpm		\
	tb_timer_stopped.xpm	\
	tb_exit.xpm		\
	tb_paste.xpm		\
	tb_save.xpm		\
	tb_unknown.xpm		\
	tb_home.xpm		\
	tb_back.xpm		\
	tb_forward.xpm		\
	tb_home_.xpm		\
	tb_back_.xpm		\
	tb_forward_.xpm		\
	topic.dat		\
	gtt-db.sgml		\
	gtt-db.html		\
	gtt.desktop

Productivitydir = $(datadir)/apps/Productivity
Productivity_DATA = gtt.desktop


# maybe there is a macro for that?

gtt_helpdir = $(datadir)/gnome/help/gtt/C

install-data-local:
	 $(mkinstalldirs) $(gtt_helpdir)
	 $(INSTALL_DATA) $(srcdir)/topic.dat $(gtt_helpdir)/topic.dat
	 $(INSTALL_DATA) $(srcdir)/gtt-db.html $(gtt_helpdir)/index.html

