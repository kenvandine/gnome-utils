# Process this file with automake to produce Makefile.in

gtt_helpdir = $(datadir)/gnome/help/gtt/C
gtt_help_DATA = index.html authors.html usage.html topic.dat 

SGML_FILES = \
	gtt.sgml

EXTRA_DIST = \
	topic.dat gtt $(SGML_FILES)

all:

index.html: gtt/index.html
	-cd $(srcdir) && cp gtt/index.html .

authors.html: gtt/authors.html
	-cd $(srcdir) && cp gtt/authors.html .

usage.html: gtt/usage.html
	-cd $(srcdir) && cp gtt/usage.html .

gtt/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtt.sgml

gtt/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtt.sgml

gtt/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtt.sgml

dist-hook:
	mkdir $(distdir)/gtt
	-cp $(srcdir)/gtt/*.html gtt/*.css $(distdir)/gtt
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gtt_helpdir)/images
	-for file in $(srcdir)/gtt/*.html $(srcdir)/gtt/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gtt_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gtt_helpdir);\
	done
	$(mkinstalldirs) $(DESTDIR)$(gtt_helpdir)/stylesheet-images
	-for file in $(srcdir)/gtt/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gtt_helpdir)/stylesheet-images/$$basefile; \
	done
