if GUILE
if FALSE
guile_extra_DIRS = cromagnon
endif
guile_DIRS = $(guile_extra_DIRS)
endif

if BUILD_GFLOPPY
gfloppy = gfloppy
endif

SUBDIRS = po idl $(gfloppy) logview gcharmap \
	gcalc gsearchtool gtt gdictsrc \
	$(guile_DIRS) mini-utils splash gdialog \
	meat-grinder exe-handler omf-install

EXTRA_DIST = gnome-utils.spec.in

dist-hook: gnome-utils.spec
	cp gnome-utils.spec $(distdir)
