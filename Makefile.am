if BUILD_GFLOPPY
gfloppy = gfloppy
endif

#if GUILE
gtt = gtt
#endif

SUBDIRS = po idl $(gfloppy) logview gcharmap \
	gcalc gsearchtool  gdictsrc \
	gdialog  \
	meat-grinder exe-handler omf-install 
	  
#splash stripchart mini-utils $(gtt)

EXTRA_DIST = autogen.sh gnome-utils.spec.in \
	xml-i18n-extract.in \
        xml-i18n-merge.in \
        xml-i18n-update.in

dist-hook: gnome-utils.spec
	cp gnome-utils.spec $(distdir)
