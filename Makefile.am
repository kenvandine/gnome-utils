if BUILD_GFLOPPY
gfloppy = gfloppy
endif

if BUILD_GDIALOG
gdialog = gdialog
endif

always_built_SUBDIRS = \
	po 		\
	logview 	\
	gsearchtool	\
	gdictsrc

SUBDIRS = \
	$(always_built_SUBDIRS)	\
	$(gfloppy)	\
	$(gdialog)

DIST_SUBDIRS = \
	$(always_built_SUBDIRS)	\
	gfloppy			\
	gdialog

EXTRA_DIST = \
	gnome-utils.spec.in \
	gnome-utils.spec \
	omf.make \
	xmldocs.make \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in
