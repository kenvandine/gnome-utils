SUBDIRS = help

Utilitiesdir = $(datadir)/gnome/apps/Utilities

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gfontsel

gfontsel_SOURCES = gfontsel.c

gfontsel_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

man_MANS = gfontsel.1

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-fontsel.png

EXTRA_DIST = gfontsel.desktop.in gnome-fontsel.png gfontsel.1 \
	$(top_srcdir)/sgmldocs.make

Utilities_in_files = gfontsel.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@
