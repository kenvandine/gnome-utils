## Process this file with automake to produce Makefile.in

gdiskfree_helpdir = $(datadir)/gnome/help/gdiskfree/C
gdiskfree_help_DATA = index.html topic.dat

SGML_FILES = 		\
	gdiskfree.sgml	gnome-legalnotice.sgml

EXTRA_DIST = \
	topic.dat	gdiskfree.html	$(SGML_FILES)

all:

index.html: $(SGML_FILES)
	-cd $(srcdir) && cp gdiskfree/gdiskfree.html .

gdiskfree/gdiskfree.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gdiskfree.sgml

dist-hook:
	mkdir $(distdir)/gdiskfree
	-cp $(srcdir)/gdiskfree/*.html gdiskfree/*.css $(distdir)/gdiskfree
	-cp $(srcdir)/gdiskfree.html $(distdir)
	mkdir $(distdir)/images
	-cp $(srcdir)/images/*.png $(distdir)/images

install-data-local: gdiskfree.html
	$(mkinstalldirs) $(DESTDIR)$(gdiskfree_helpdir)/images
	-for file in $(srcdir)/gdiskfree/*.html $(srcdir)/gdiskfree/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.jpg $(srcdir)/images/*.gif; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/images;\
	done

