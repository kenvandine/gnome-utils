## Process this file with automake to produce Makefile.in

SUBDIRS = doc

INCLUDES = -DG_LOG_DOMAIN=\"GDiskFree\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS = gdiskfree 

gdiskfree_SOURCES = \
	gdiskfree_app.c		gdiskfree_app.h		\
	gdiskfree_menus.c	gdiskfree_menus.h	\
	gdiskfree_options.c	gdiskfree_options.h	\
	gdiskfree.c


gdiskfree_LDADD = $(GNOME_LIBDIR) $(GTKXMHTML_LIBS) \
		  $(GNOMEUI_LIBS) $(INTLLIBS) \
		  $(LIBGTOP_LIBS) $(LIBGTOP_EXTRA_LIBS)

EXTRA_DIST = gdiskfree.desktop $(top_srcdir)/sgmldocs.make

Systemdir = $(datadir)/gnome/apps/System
System_DATA = gdiskfree.desktop

