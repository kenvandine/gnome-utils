## Process this file with automake to produce Makefile.in

gcolorsel_helpdir = $(datadir)/gnome/help/gcolorsel/C
gcolorsel_help_DATA = index.html usage.html simple.html \
	search.html favorites.html topic.dat


SGML_FILES = 		\
	gcolorsel.sgml

EXTRA_DIST = \
	topic.dat	gcolorsel	$(SGML_FILES)

all:

index.html: gcolorsel/index.html
	-cd $(srcdir) && cp gcolorsel/index.html .

simple.html: gcolorsel/simple.html
	-cd $(srcdir) && cp gcolorsel/simple.html .

favorites.html: gcolorsel/favorites.html
	-cd $(srcdir) && cp gcolorsel/favorites.html .

usage.html: gcolorsel/usage.html
	-cd $(srcdir) && cp gcolorsel/usage.html .

search.html: gcolorsel/search.html
	-cd $(srcdir) && cp gcolorsel/search.html .

gcolorsel/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcolorsel.sgml

gcolorsel/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcolorsel.sgml

gcolorsel/simple.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcolorsel.sgml

gcolorsel/favorites.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcolorsel.sgml

gcolorsel/search.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gcolorsel.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gcolorsel
	-cp $(srcdir)/gcolorsel/*.html gcolorsel/*.css $(distdir)/gcolorsel
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gcolorsel_helpdir)
	-for file in $(srcdir)/gcolorsel/*.html $(srcdir)/gcolorsel/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gcolorsel_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gcolorsel_helpdir);\
	done
	$(mkinstalldirs) $(DESTDIR)$(gcolorsel_helpdir)/stylesheet-images
	-for file in $(srcdir)/gcolorsel/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gcolorsel_helpdir)/stylesheet-images/$$basefile; \
	done
