# Process this file with automake to produce Makefile.in

guname_helpdir = $(datadir)/gnome/help/guname/C
guname_help_DATA = index.html authors.html usage.html topic.dat 

SGML_FILES = \
	guname.sgml

EXTRA_DIST = \
	topic.dat guname $(SGML_FILES)

all:

index.html: guname/index.html
	-cd $(srcdir) && cp guname/index.html .

authors.html: guname/authors.html
	-cd $(srcdir) && cp guname/authors.html .

usage.html: guname/usage.html
	-cd $(srcdir) && cp guname/usage.html .

guname/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html guname.sgml

guname/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html guname.sgml

guname/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html guname.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/guname
	-cp $(srcdir)/guname/*.html guname/*.css $(distdir)/guname
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(guname_helpdir)/images
	-for file in $(srcdir)/guname/*.html $(srcdir)/guname/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(guname_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(guname_helpdir);\
	done

