Utilitiesdir = $(datadir)/gnome/apps/Utilities

SUBDIRS = doc

INCLUDES = 						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gcalc 

gcalc_SOURCES = gcalc.c

gcalc_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

EXTRA_DIST = \
	gcalc.desktop

Utilities_DATA = gcalc.desktop 
