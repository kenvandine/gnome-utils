if OBJECTIVE_C
objc_DIRS = #gpostit
endif

if GUILE
guile_DIRS = #cromagnon find-file notepad
endif

if P_GENIUS
genius = genius
endif

if HAVE_LIBGTOP
gdiskfree = gdiskfree
endif

# We include `gtop' in a `FALSE' automake conditional so it is
# included in the tarballs and we don't need to list everything
# in DIST_SUBDIRS.

if FALSE
gtop = gtop
endif

SUBDIRS = po intl macros \
	gcalc gedit $(genius) ghex gsearchtool gtop $(gdiskfree) gtt \
	$(guile_DIRS) $(objc_DIRS) mini-utils gncal splash gdialog

EXTRA_DIST = gnome-utils.spec

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
