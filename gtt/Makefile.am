
GttHelpdir = $(datadir)/gtt

INCLUDES = 			\
	-I$(includedir)		\
	-DHAS_GNOME=1		\
	-DHELP_PATH=\"$(GttHelpdir)\" \
	$(X_CFLAGS)

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
	clist.c		\
	gtthelp.c	\
	gtthelp.h	\
	gtt-features.h

gtt_LDADD = \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS)

GttHelp_DATA = \
	index.html		\
	ch-dialogs.html		\
	ch-main.html		\
	ch-intro.html		\
	ch-working.html

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

#install-data-local:
#	 $(mkinstalldirs) $(Utilitiesdir)
#	 $(INSTALL_DATA) $(srcdir)/gtt.desktop $(Utilitiesdir)/gtt.desktop
#	 $(mkinstalldirs) $(GTT_HELP_DIR)
#	 @list='$(GTT_HELP_FILES)' ; for i in $$list; do \
#		 echo "$(INSTALL_DATA) $(srcdir)/$$i $(GTT_HELP_DIR)/$$i" ; \
#		 $(INSTALL_DATA) $(srcdir)/$$i $(GTT_HELP_DIR)/$$i ; \
#	 done
