bin_PROGRAMS = meat-grinder

SUBDIRS = help

INCLUDES = \
	-I$(top_srcdir) 				\
	-DG_LOG_DOMAIN=\"MeatGrinder\" 			\
	$(GNOME_UTILS_CFLAGS)				\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

meat_grinder_SOURCES = 	\
	meat-grinder.c

meat_grinder_LDADD = \
	$(GNOME_UTILS_LIBS)

man_MANS = meat-grinder.1

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_in_files = meat-grinder.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(man_MANS) \
	$(Utilities_in_files) \
	$(Utilities_DATA)
