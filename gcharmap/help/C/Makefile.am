gcharmap_helpdir = $(datadir)/gnome/help/gcharmap/C
gcharmap_help_DATA = index.html authors.html bugs.html usage.html topic.dat \
			license.html ln7.html

SGML_FILES =\
	gcharmap.sgml

EXTRA_DIST = 		\
	topic.dat 	\
	$(SGML_FILES)

all:

index.html: gcharmap/index.html
	-cd $(srcdir) && cp gcharmap/index.html .

authors.html: gcharmap/authors.html
	-cd $(srcdir) && cp gcharmap/authors.html .

usage.html: gcharmap/usage.html
	-cd $(srcdir) && cp gcharmap/usage.html .

bugs.html: gcharmap/bugs.html
	-cd $(srcdir) && cp gcharmap/bugs.html .

license.html: gcharmap/license.html
	-cd $(srcdir) && cp gcharmap/license.html .

ln7.html: gcharmap/ln7.html
	-cd $(srcdir) && cp gcharmap/ln7.html .

gcharmap/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcharmap.sgml

gcharmap/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcharmap.sgml

gcharmap/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcharmap.sgml

gcharmap/bugs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcharmap.sgml

gcharmap/license.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcharmap.sgml

gcharmap/ln7.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcharmap.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gcharmap
	-cp $(srcdir)/gcharmap/*.html gcharmap/*.css $(distdir)/gcharmap
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)/images
	-cp $(srcdir)/images/*.png $(distdir)/images

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_helpdir)/images
	-for file in $(srcdir)/gcharmap/*.html $(srcdir)/gcharmap/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gcharmap_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gcharmap_helpdir)/images;\
	done
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_helpdir)/stylesheet-images
	-for file in $(srcdir)/gcharmap/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gcharmap_helpdir)/stylesheet-images/$$basefile; \
	done

