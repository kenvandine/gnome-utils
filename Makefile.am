if OBJECTIVE_C
objc_DIRS =  gpostit
endif

if GUILE
guile_DIRS = #cromagnon find-file notepad
endif

SUBDIRS = po intl macros \
	@PROGRAMS_GENIUS@ gcalc ghex gncal gedit\
	@PROGRAMS_GTOP@ gtt $(guile_DIRS) $(objc_DIRS)

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
