Utilitiesdir = $(datadir)/gnome/apps/Utilities

SUBDIRS = doc

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-I$(includedir) \
	$(GNOME_INCLUDEDIR)	\
	@PIXBUF_CFLAGS@

bin_PROGRAMS = gcalc 

gcalc_SOURCES = \
	gcalc.c		\
	gnome-calc.c	\
	gnome-calc.h	\
	sr.h		\
	sr.c

gcalc_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS) \
	@PIXBUF_LIBS@

man_MANS = gcalc.1

EXTRA_DIST = \
	gcalc.desktop	\
	gcalc.1

Utilities_DATA = gcalc.desktop 
