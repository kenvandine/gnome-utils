Utilitiesdir = $(datadir)/apps/Utilities

INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	-DHAS_GNOME=1		\
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
	$(top_builddir)/intl/libintl.a			\
	$(GTK_LIBS)					\
        $(X_LIBS) $(x_libs)

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
	gtt.desktop

install-data-local:
	$(mkinstalldirs) $(Utilitiesdir)
	$(INSTALL_DATA) $(srcdir)/gtt.desktop $(Utilitiesdir)/gtt.desktop

