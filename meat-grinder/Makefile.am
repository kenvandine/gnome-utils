bin_PROGRAMS = meat-grinder

SUBDIRS = help

INCLUDES = \
	-I$(top_srcdir) 				\
	-DG_LOG_DOMAIN=\"MeatGrinder\" 			\
	$(GNOME_INCLUDEDIR)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

meat_grinder_SOURCES = 	\
	meat-grinder.c

meat_grinder_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = meat-grinder.desktop

EXTRA_DIST = meat-grinder.desktop
