SUBDIRS = help

INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gshutdown

gshutdown_SOURCES = \
	gshutdown.c

gshutdown_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

EXTRA_DIST = \
	gshutdown.desktop

Systemdir = $(datadir)/gnome/apps/Settings
System_DATA = gshutdown.desktop
