## Process this file with automake to produce Makefile.in

gdiskfree_helpdir = $(datadir)/gnome/help/gdiskfree/C
gdiskfree_help_DATA = index.html authors.html usage.html topic.dat


SGML_FILES = 		\
	gdiskfree.sgml	gnome-legalnotice.sgml

EXTRA_DIST = \
	topic.dat	gdiskfree	$(SGML_FILES)

all:

index.html: gdiskfree/index.html
	-cd $(srcdir) && cp gdiskfree/index.html .

authors.html: gdiskfree/authors.html
	-cd $(srcdir) && cp gdiskfree/authors.html .

usage.html: gdiskfree/usage.html
	-cd $(srcdir) && cp gdiskfree/usage.html .


gdiskfree/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gdiskfree.sgml

gdiskfree/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gdiskfree.sgml

gdiskfree/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gdiskfree.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gdiskfree
	-cp $(srcdir)/gdiskfree/*.html gdiskfree/*.css $(distdir)/gdiskfree
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)/images
	-cp $(srcdir)/images/*.png $(distdir)/images

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gdiskfree_helpdir)/images
	-for file in $(srcdir)/gdiskfree/*.html $(srcdir)/gdiskfree/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/images;\
	done
	$(mkinstalldirs) $(DESTDIR)$(gdiskfree_helpdir)/stylesheet-images
	-for file in $(srcdir)/gdiskfree/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/stylesheet-images/$$basefile; \
	done
