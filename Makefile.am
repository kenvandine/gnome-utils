if OBJECTIVE_C
objc_DIRS =  gpostit
endif

if GUILE
guile_DIRS = #cromagnon find-file notepad
endif

SUBDIRS = po intl macros \
	gcalc gedit @PROGRAMS_GENIUS@ ghex gncal gsearchtool gdiskfree\
	@PROGRAMS_GTOP@ gtt $(guile_DIRS) $(objc_DIRS)

# since some of the dirs are conditionally built, we need to tell
# `automake' all the subdirs that make up `gnome-utils'.
DIST_SUBDIRS = po intl macros \
	cromagnon find-file gcalc gedit genius ghex gncal gpostit \
	gsearchtool gtop gtt mini-utils notepad

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
