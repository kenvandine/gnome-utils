Utilitiesdir = $(datadir)/applications

SUBDIRS = help

INCLUDES = 	\
	$(GNOME_UTILS_CFLAGS)				\
	$(GNOMEVFS_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-DDATADIR=\""$(datadir)"\"

bin_PROGRAMS = gnome-search-tool

gnome_search_tool_SOURCES =	\
	gsearchtool.c	\
	gsearchtool.h

gnome_search_tool_LDADD = \
	$(GNOME_UTILS_LIBS) \
	$(GNOMEVFS_LIBS)

man_MANS = gnome-search-tool.1

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-searchtool.png

Utilities_in_files = gnome-search-tool.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(pixmap_DATA) \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(man_MANS)
