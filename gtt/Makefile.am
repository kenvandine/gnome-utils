
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DHAS_GNOME=1

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
	gtt.sgml		\
	gtt.text		\
	gtt.desktop

Productivitydir = $(datadir)/apps/Productivity
Productivity_DATA = gtt.desktop


# maybe there is a macro for that?

gtt_helpdir = $(datadir)/gnome/help/gtt/C
gtt_helpfiles = \
	topic.dat		\
	index.html		\
	ch-dialogs.html		\
	ch-main.html		\
	ch-intro.html		\
	ch-working.html

install-data-local:
	 $(mkinstalldirs) $(gtt_helpdir)
	 @list='$(gtt_helpfiles)' ; for i in $$list; do \
		 echo "$(INSTALL_DATA) $(srcdir)/$$i $(gtt_helpdir)/$$i" ; \
		 $(INSTALL_DATA) $(srcdir)/$$i $(gtt_helpdir)/$$i ; \
	 done
