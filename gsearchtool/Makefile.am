Utilitiesdir = $(datadir)/apps/Utilities

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

EXTRA_DIST = gsearchtool.desktop

Utilities_DATA = gsearchtool.desktop 
