Utilitiesdir = $(datadir)/applications

SUBDIRS = help

INCLUDES = \
	@GNOME_UTILS_CFLAGS@ \
	-DDATADIR=\""$(datadir)"\" \
	-DGNOMELOCALEDIR=\""$(gnomeutilslocaledir)"\" 

bin_PROGRAMS = gnome-calculator 

gnome_calculator_SOURCES = \
	gcalc.c		\
	gnome-calc.c	\
	gnome-calc.h	\
	sr.h		\
	sr.c

gnome_calculator_LDADD = \
	@GNOME_UTILS_LIBS@	\
	$(X_LIBS)

man_MANS = gnome-calculator.1

Utilities_in_files = gnome-calculator.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(man_MANS) \
	$(Utilities_in_files) \
	$(Utilities_DATA)
