if OBJECTIVE_C
objc_DIRS = 
endif

if GUILE
if FALSE
guile_extra_DIRS = cromagnon
endif
guile_DIRS = $(guile_extra_DIRS)
endif

if HAVE_LIBGTOP
gdiskfree = gdiskfree
gstripchart = gstripchart
endif

if FALSE
extras = gnome-find
endif

SUBDIRS = po intl macros idl \
	gcalc gsearchtool $(gdiskfree) $(gstripchart) gtt gdictsrc \
	$(guile_DIRS) $(objc_DIRS) mini-utils splash gdialog $(extras)

EXTRA_DIST = gnome-utils.spec.in

dist-hook: gnome-utils.spec
	cp gnome-utils.spec $(distdir)

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
