Utilitiesdir = $(datadir)/applications

SUBDIRS = help

INCLUDES = \
	@GNOME_UTILS_CFLAGS@ \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) 

bin_PROGRAMS = gcalc 

gcalc_SOURCES = \
	gcalc.c		\
	gnome-calc.c	\
	gnome-calc.h	\
	sr.h		\
	sr.c

gcalc_LDADD = \
	$(INTLLIBS) \
	@GNOME_UTILS_LIBS@

man_MANS = gcalc.1

Utilities_in_files = gcalc.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(man_MANS) \
	$(Utilities_in_files) \
	$(Utilities_DATA)
