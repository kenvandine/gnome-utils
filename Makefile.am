if GUILE
guile_DIRS = cromagnon find-file notepad
endif

SUBDIRS = po intl macros \
	edit-menus @PROGRAMS_GENIUS@ gcalc ghex gncal \
	@PROGRAMS_GTOP@ gtt mini-utils $(guile_DIRS)

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
