# Process this file with automake to produce Makefile.in

SUBDIRS = doc

INCLUDES = -DG_LOG_DOMAIN=\"gless\" 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gless

gless_SOURCES = \
	gless.c

gless_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = gless.desktop

EXTRA_DIST = gless.desktop

