#
# FIXME:
#  currently scrollkeeper-update does not seem to
#  remove its database when there is no useful
#  information contained there. Therefore, with
#  "make uninstall" you're left with loads of
#  stuff in /var/scrollkeeper and distcheck
#  fails.
#
#  http://mail.gnome.org/archives/gnome-doc-list/2003-May/msg00008.html
#
#  This overrides the uninstall check so distcheck
#  passes
#
# distuninstallcheck_listfiles = find . -type f -print | grep -v 'scrollkeeper' | grep -v 'omf' | grep -v 'figures'
distuninstallcheck:
	@:
                                                                                                                                                             
distcleancheck_listfiles = find . -type f -print | grep -v 'omf\.out' | grep -v 'legal.xml'

if BUILD_GFLOPPY
gfloppy = gfloppy
endif

always_built_SUBDIRS =   \
	po 		 \
	logview 	 \
	gsearchtool	 \
	gnome-dictionary \
	gnome-screenshot

SUBDIRS = \
	$(always_built_SUBDIRS)	\
	$(gfloppy)

DIST_SUBDIRS = \
	$(always_built_SUBDIRS)	\
	gfloppy

EXTRA_DIST = \
	m4 \
	gnome-utils.spec.in \
	gnome-utils.spec \
	omf.make \
	xmldocs.make \
	gnome-doc-utils.make \
	gtk-doc.make
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

DISTCLEANFILES =                \
	gtk-doc.make		\
	gnome-doc-utils.make	\
	intltool-extract        \
	intltool-merge          \
	intltool-update

DISTCHECK_CONFIGURE_FLAGS = \
	--disable-scrollkeeper \
	--enable-gtk-doc
