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

man_MANS = meat-grinder.1

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_in_files = meat-grinder.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

EXTRA_DIST = meat-grinder.desktop.in meat-grinder.1
