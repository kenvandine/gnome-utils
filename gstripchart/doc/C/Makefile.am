# Process this file with automake to produce Makefile.in

gstripchart_helpdir = $(datadir)/gnome/help/gstripchart/C

gstripchart_help_DATA = topic.dat index.html \
	authors.html bugs.html license.html ln7.html prefs.html usage.html

SGML_FILES = gstripchart.sgml

EXTRA_DIST = $(SGML_FILES) topic.dat gstripchart

all:

index.html: gstripchart/index.html
	-cd $(srcdir) && cp $< .

authors.html: gstripchart/authors.html
	-cd $(srcdir) && cp $< .

bugs.html: gstripchart/bugs.html
	-cd $(srcdir) && cp $< .

license.html: gstripchart/license.html
	-cd $(srcdir) && cp $< .

ln7.html: gstripchart/ln7.html
	-cd $(srcdir) && cp $< .

prefs.html: gstripchart/prefs.html
	-cd $(srcdir) && cp $< .

usage.html: gstripchart/usage.html
	-cd $(srcdir) && cp $< .

gstripchart/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/bugs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/license.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/ln7.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/prefs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gstripchart
	-cp $(srcdir)/gstripchart/*.html gstripchart/*.css $(distdir)/gstripchart
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	-for file in $(srcdir)/gstripchart/*.html $(srcdir)/gstripchart/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir);\
	done
