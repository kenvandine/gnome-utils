# Process this file with automake to produce Makefile.in

gshutdown_helpdir = $(datadir)/gnome/help/gshutdown/C
gshutdown_help_DATA = index.html authors.html usage.html topic.dat 

SGML_FILES = \
	gshutdown.sgml

EXTRA_DIST = \
	topic.dat gshutdown $(SGML_FILES)

all:

index.html: gshutdown/index.html
	-cd $(srcdir) && cp gshutdown/index.html .

authors.html: gshutdown/authors.html
	-cd $(srcdir) && cp gshutdown/authors.html .

usage.html: gshutdown/usage.html
	-cd $(srcdir) && cp gshutdown/usage.html .

gshutdown/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gshutdown.sgml

gshutdown/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gshutdown.sgml

gshutdown/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gshutdown.sgml

dist-hook:
	mkdir $(distdir)/gshutdown
	-cp $(srcdir)/gshutdown/*.html gshutdown/*.css $(distdir)/gshutdown
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gshutdown_helpdir)/images
	-for file in $(srcdir)/gshutdown/*.html $(srcdir)/gshutdown/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gshutdown_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gshutdown_helpdir);\
	done

