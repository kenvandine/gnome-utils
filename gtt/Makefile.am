Utilitiesdir = $(datadir)/apps/Utilities
GTT_HELP_DIR = $(datadir)/gtt

# DEFS += -ggdb -Wall -DDEBUG

INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	-DHAS_GNOME=1		\
	-DHELP_PATH=\"$(GTT_HELP_DIR)\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	$(X_CFLAGS)

bin_PROGRAMS = gtt

gtt_SOURCES = 	\
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
	gtthelp.c	\
	gtthelp.h	\
	gtt-features.h

gtt_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/gtk-xmhtml/libgtkxmhtml.la	\
	$(top_builddir)/intl/libintl.a			\
	$(XPM_LIBS)					\
	$(GTK_LIBS)					\
        $(X_LIBS) $(x_libs) $(INTLLIBS)

GTT_HELP_FILES = \
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
	gtt.sgml		\
	$(GTT_HELP_FILES)	\
	gtt.text		\
	gtt.desktop

install-data-local:
	$(mkinstalldirs) $(Utilitiesdir)
	$(INSTALL_DATA) $(srcdir)/gtt.desktop $(Utilitiesdir)/gtt.desktop
	$(mkinstalldirs) $(GTT_HELP_DIR)
	@list='$(GTT_HELP_FILES)' ; for i in $$list; do \
		echo "$(INSTALL_DATA) $(srcdir)/$$i $(GTT_HELP_DIR)/$$i" ; \
		$(INSTALL_DATA) $(srcdir)/$$i $(GTT_HELP_DIR)/$$i ; \
	done

