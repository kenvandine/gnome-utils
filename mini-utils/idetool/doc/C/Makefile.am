## Process this file with automake to produce Makefile.in

idetool_helpdir = $(datadir)/gnome/help/idetool/C
idetool_help_DATA = index.html usage.html ln7.html license.html bugs.html \
	authors.html topic.dat


SGML_FILES = 		\
	idetool.sgml

EXTRA_DIST = \
	topic.dat	idetool	$(SGML_FILES)

all:

index.html: idetool/index.html
	-cd $(srcdir) && cp idetool/index.html .

usage.html: idetool/usage.html
	-cd $(srcdir) && cp idetool/usage.html .

ln7.html: idetool/ln7.html
	-cd $(srcdir) && cp idetool/ln7.html .

license.html: idetool/license.html
	-cd $(srcdir) && cp idetool/license.html .

bugs.html: idetool/bugs.html
	-cd $(srcdir) && cp idetool/bugs.html .

authors.html: idetool/authors.html
	-cd $(srcdir) && cp idetool/authors.html .

idetool/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html idetool.sgml

idetool/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html idetool.sgml

idetool/ln7.html: $(SGML_FILES)
	-cd $(srcdir) && db2html idetool.sgml

idetool/license.html: $(SGML_FILES)
	-cd $(srcdir) && db2html idetool.sgml

idetool/bugs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html idetool.sgml

idetool/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html idetool.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/idetool
	-cp $(srcdir)/idetool/*.html idetool/*.css idetool/*.png $(distdir)/idetool
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)/images
	-cp $(srcdir)/images/*.png $(distdir)/images

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(idetool_helpdir)/images
	-for file in $(srcdir)/idetool/*.html $(srcdir)/idetool/*.png $(srcdir)/idetool/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir)/images;\
	done
	$(mkinstalldirs) $(DESTDIR)$(idetool_helpdir)/stylesheet-images
	-for file in $(srcdir)/idetool/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir)/stylesheet-images/$$basefile; \
	done
