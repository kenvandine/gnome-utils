## Process this file with automake to produce Makefile.in

gstripchart_helpdir = $(datadir)/gnome/help/gstripchart/C
gstripchart_help_DATA = index.html authors.html usage.html topic.dat


SGML_FILES = 		\
	gstripchart.sgml

EXTRA_DIST = \
	topic.dat	gstripchart	$(SGML_FILES)

all:

index.html: gstripchart/index.html
	-cd $(srcdir) && cp gstripchart/index.html .

authors.html: gstripchart/authors.html
	-cd $(srcdir) && cp gstripchart/authors.html .

usage.html: gstripchart/usage.html
	-cd $(srcdir) && cp gstripchart/usage.html .


gstripchart/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

gstripchart/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gstripchart.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gstripchart
	-cp $(srcdir)/gstripchart/*.html gstripchart/*.css $(distdir)/gstripchart
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.sgml $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gstripchart_helpdir)/images
	-for file in $(srcdir)/gstripchart/*.html $(srcdir)/gstripchart/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir);\
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir); \
	done
	$(mkinstalldirs) $(DESTDIR)$(gstripchart_helpdir)/stylesheet-images
	-for file in $(srcdir)/gstripchart/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gstripchart_helpdir)/stylesheet-images/$$basefile; \
	done
