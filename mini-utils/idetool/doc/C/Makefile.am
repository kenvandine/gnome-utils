## Process this file with automake to produce Makefile.in.
##
idetool_helpdir = $(datadir)/gnome/help/idetool/C

SGML_FILES = \
	idetool.sgml

HTML_FILES = authors.html configuring.html index.html ln7.html viewing.html \
	bugs.html license.html

EXTRA_DIST = \
	topic.dat idetool $(SGML_FILES)

idetool_help_DATA = ${HTML_FILES} topic.dat

#
# This is icky but insures that the doc files are generated when the sgml 
# file changes, plus it only generates em once. The distclean target 
# will take care of clobbering the generated html and the directory that
# db2html makes, while the dist-hook will make sure that the html files 
# and directory gets copied into a dist (so mere mortals w/o docbook can 
# build this) The install-data-local target copies all the bits and pieces
# where the runtime help system can find it.
#
${HTML_FILES} : $(SGML_FILES)
	if [ ! -d idetool ] ; then cd $(srcdir) && db2html idetool.sgml ; fi
	-cd $(srcdir) && cp idetool/$@ .
	if [ -d idetool.junk ] ; then cd $(srcdir) && rm -rf idetool.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/idetool 
	$(mkinstalldirs) $(distdir)/idetool/stylesheet-images
	-cp $(srcdir)/idetool/*.html idetool/*.css $(distdir)/idetool
	-cp $(srcdir)/idetool/stylesheet-images/*.gif \
	 $(distdir)/idetool/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.sgml $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf idetool

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(idetool_helpdir)/stylesheet-images
	-for file in $(srcdir)/idetool/*.html $(srcdir)/idetool/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir); \
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir); \
	done
	-for file in $(srcdir)/idetool/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(idetool_helpdir)/stylesheet-images/$$basefile; \
	done



