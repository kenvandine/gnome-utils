## Process this file with automake to produce Makefile.in

DB_DIR = stripchart
SGML_FILES = stripchart.sgml
HTML_FILES = \
 index.html usage.html options.html preferences.html parameters.html \
 files.html bugs.html authors.html license.html

stripchart_helpdir = $(datadir)/gnome/help/stripchart/C
stripchart_help_DATA = topic.dat $(HTML_FILES)

EXTRA_DIST = topic.dat stripchart $(SGML_FILES)

all:

$(HTML_FILES):		$(DB_DIR)/docbook.css
	-cp $(DB_DIR)/$@ .

$(DB_DIR)/docbook.css:	$(SGML_FILES)
	-db2html $<

dist-hook:
	$(mkinstalldirs) $(distdir)/stripchart
	-cp $(srcdir)/stripchart/*.html stripchart/*.css $(distdir)/stripchart
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

install-data-local:	$(HTML_FILES)
	$(mkinstalldirs) $(DESTDIR)$(stripchart_helpdir)/images
	-for file in $(srcdir)/stripchart/*.html $(srcdir)/stripchart/*.css;do\
	    basefile=`basename $$file`; \
	    $(INSTALL_DATA) $$file $(DESTDIR)$(stripchart_helpdir)/$$basefile;\
	done
	-for file in $(srcdir)/*.png ; do \
	    $(INSTALL_DATA) $$file $(DESTDIR)$(stripchart_helpdir);\
	done
	$(mkinstalldirs) $(DESTDIR)$(stripchart_helpdir)/stylesheet-images
	-for file in $(srcdir)/stripchart/stylesheet-images/*.gif; do \
	    basefile=`basename $$file`; \
	    $(INSTALL_DATA) $$file \
	        $(DESTDIR)$(stripchart_helpdir)/stylesheet-images/$$basefile; \
	done
