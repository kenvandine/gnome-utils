Utilitiesdir = $(datadir)/gnome/apps/Utilities

SUBDIRS = help

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gsearchtool 

gsearchtool_SOURCES =	\
	gsearchtool.c	\
	gsearchtool.h	\
        outdlg.c	\
	outdlg.h

gsearchtool_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

man_MANS = gsearchtool.1

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-searchtool.png

EXTRA_DIST = gnome-searchtool.png gsearchtool.desktop gsearchtool.1

Utilities_DATA = gsearchtool.desktop 
