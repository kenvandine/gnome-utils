if OBJECTIVE_C
objc_DIRS = #gpostit
endif

if GUILE
guile_DIRS = #cromagnon find-file notepad
endif

if P_GENIUS
genius = genius
endif

if P_GTOP
gtop = gtop
endif

SUBDIRS = po intl macros \
	gcalc gedit $(genius) ghex gsearchtool gdiskfree $(gtop) gtt \
	$(guile_DIRS) $(objc_DIRS) mini-utils gncal splash gdialog

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
