# Process this file with automake to produce Makefile.in

gw_helpdir = $(datadir)/gnome/help/gw/C
gw_help_DATA = index.html authors.html usage.html topic.dat 

SGML_FILES = \
	gw.sgml

EXTRA_DIST = \
	topic.dat gw $(SGML_FILES)

all:

index.html: gw/index.html
	-cd $(srcdir) && cp gw/index.html .

authors.html: gw/authors.html
	-cd $(srcdir) && cp gw/authors.html .

usage.html: gw/usage.html
	-cd $(srcdir) && cp gw/usage.html .

gw/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gw.sgml

gw/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gw.sgml

gw/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gw.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gw
	-cp $(srcdir)/gw/*.html gw/*.css $(distdir)/gw
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gw_helpdir)/images
	-for file in $(srcdir)/gw/*.html $(srcdir)/gw/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir);\
	done
	$(mkinstalldirs) $(DESTDIR)$(gw_helpdir)/stylesheet-images
	-for file in $(srcdir)/gw/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir)/stylesheet-images/$$basefile; \
	done


