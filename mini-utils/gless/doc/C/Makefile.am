# Process this file with automake to produce Makefile.in

gless_helpdir = $(datadir)/gnome/help/gless/C
gless_help_DATA = index.html authors.html usage.html topic.dat 

SGML_FILES = \
	gless.sgml

EXTRA_DIST = \
	topic.dat gless $(SGML_FILES)

all:

index.html: gless/index.html
	-cd $(srcdir) && cp gless/index.html .

authors.html: gless/authors.html
	-cd $(srcdir) && cp gless/authors.html .

usage.html: gless/usage.html
	-cd $(srcdir) && cp gless/usage.html .

gless/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gless.sgml

gless/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gless.sgml

gless/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gless.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gless
	-cp $(srcdir)/gless/*.html gless/*.css $(distdir)/gless
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)/images
	-cp $(srcdir)/images/*.png $(distdir)/images

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gless_helpdir)/images
	-for file in $(srcdir)/gless/*.html $(srcdir)/gless/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gless_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gless_helpdir)/images;\
	done

