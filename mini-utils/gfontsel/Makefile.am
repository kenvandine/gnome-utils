SUBDIRS = help

Utilitiesdir = $(datadir)/applications

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_UTILS_CFLAGS)

bin_PROGRAMS = gfontsel

gfontsel_SOURCES = gfontsel.c

gfontsel_LDADD = \
	$(GNOME_UTILS_LIBS)

man_MANS = gfontsel.1

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-fontsel.png

Utilities_in_files = gfontsel.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(man_MANS) \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(pixmap_DATA) \
	$(top_srcdir)/sgmldocs.make

