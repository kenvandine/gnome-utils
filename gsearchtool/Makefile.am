Utilitiesdir = $(datadir)/applications

SUBDIRS = help

INCLUDES = 	\
	$(GNOME_UTILS_CFLAGS)				\
	$(GNOMEVFS_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					
	
bin_PROGRAMS = gsearchtool 

gsearchtool_SOURCES =	\
	gsearchtool.c	\
	gsearchtool.h

gsearchtool_LDADD = \
	$(GNOME_UTILS_LIBS) \
	$(GNOMEVFS_LIBS)

man_MANS = gsearchtool.1

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-searchtool.png

Utilities_in_files = gsearchtool.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(pixmap_DATA) \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(man_MANS)
