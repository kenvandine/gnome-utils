# Process this file with automake to produce Makefile.in

gstripchart_helpdir = $(datadir)/gnome/help/gstripchart/C

gstripchart_help_DATA = topic.dat index.html prefs.html usage.html

SGML_FILES = gstripchart.sgml

EXTRA_DIST = $(SGML_FILES) topic.dat gstripchart

all:

index.html: gstripchart/index.html
	-cd $(srcdir) && cp gstripchart/index.html .

prefs.html: gstripchart/prefs.html
	-cd $(srcdir) && cp gstripchart/prefs.html .

usage.html: gstripchart/usage.html
	-cd $(srcdir) && cp gstripchart/usage.html .

gstripchart/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/prefs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gstripchart
	-cp $(srcdir)/gstripchart/*.html gstripchart/*.css $(distdir)/gstripchart
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(srcdir)/*.jpg $(srcdir)/*.gif $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gstripchart_helpdir)
	-for file in $(srcdir)/gstripchart/*.html $(srcdir)/gstripchart/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png $(srcdir)/*.jpg ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir);\
	done

gstripchart.ps: gstripchart.sgml
	-db2ps $<
