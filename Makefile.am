if BUILD_GFLOPPY
gfloppy = gfloppy
endif

if GUILE
gtt = gtt
endif

SUBDIRS = po idl $(gfloppy) logview gcharmap \
	gcalc gsearchtool $(gtt) gdictsrc \
	mini-utils gdialog \
	meat-grinder exe-handler omf-install
#splash stripchart

EXTRA_DIST = gnome-utils.spec.in

dist-hook: gnome-utils.spec
	cp gnome-utils.spec $(distdir)
