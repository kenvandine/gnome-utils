## Process this file with automake to produce Makefile.in.
##
gcalc_helpdir = $(datadir)/gnome/help/gcalc/it

SGML_FILES = \
	gcalc.sgml

HTML_FILES = authors.html	license.html	usage.html	bugs.html \
	index.html	ln7.html


EXTRA_DIST = \
	topic.dat $(SGML_FILES)

gcalc_help_DATA = ${HTML_FILES} topic.dat

#
# This is icky but insures that the doc files are generated when the sgml 
# file changes, plus it only generates em once. The distclean target 
# will take care of clobbering the generated html and the directory that
# db2html makes, while the dist-hook will make sure that the html files 
# and directory gets copied into a dist (so mere mortals w/o docbook can 
# build this) The install-data-local target copies all the bits and pieces
# where the runtime help system can find it.
#
#
# The layout this expects things to be in...
# ./               all *.html files
# ./<doc name>/    output of db2html winds up here
# ./*.png       any images/figures needed for the documentation.
${HTML_FILES} : $(SGML_FILES)
	if [ ! -d gcalc ] ; then cd $(srcdir) && db2html gcalc.sgml ; fi
	-cd $(srcdir) && cp gcalc/$@ .
	if [ -d gcalc.junk ] ; then cd $(srcdir) && rm -rf gcalc.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gcalc 
	$(mkinstalldirs) $(distdir)/gcalc/stylesheet-images
	-cp $(srcdir)/gcalc/*.html gcalc/*.css $(distdir)/gcalc
	-cp $(srcdir)/gcalc/stylesheet-images/*.gif \
	 $(distdir)/gcalc/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gcalc

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gcalc_helpdir)/stylesheet-images
	-for file in $(srcdir)/gcalc/*.html $(srcdir)/gcalc/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcalc_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcalc_helpdir); \
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcalc_helpdir); \
	done

	-for file in $(srcdir)/gcalc/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcalc_helpdir)/stylesheet-images/$$basefile; \
	done
