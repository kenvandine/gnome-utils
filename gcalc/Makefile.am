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
	gcalc.c \
	gnome-calc.c \
	gnome-calc.h

gcalc_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS) \
	@PIXBUF_LIBS@

EXTRA_DIST = \
	gcalc.desktop

Utilities_DATA = gcalc.desktop 
