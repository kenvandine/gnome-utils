## Process this file with automake to produce Makefile.in.
##
gsearch_helpdir = $(datadir)/gnome/help/gsearchtool/C

SGML_FILES = \
	gsearchtool.sgml

HTML_FILES = index.html authors.html criteria.html \
	license.html	locate.html	usage.html

EXTRA_DIST = \
	topic.dat gsearchtool $(SGML_FILES)

gsearch_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d gsearchtool ] ; then cd $(srcdir) && db2html gsearchtool.sgml ; fi
	-cd $(srcdir) && cp gsearchtool/$@ .
	if [ -d gsearchtool.junk ] ; then cd $(srcdir) && rm -rf gsearchtool.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gsearchtool
	-cp $(srcdir)/gsearchtool/*.html gsearchtool/*.css $(distdir)/gsearchtool
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gsearchtool

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gsearch_helpdir)/images $(DESTDIR)$(gsearch_helpdir)/stylesheet-images
	-for file in $(srcdir)/gsearchtool/*.html $(srcdir)/gsearchtool/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gsearch_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gsearch_helpdir); \
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gsearch_helpdir); \
	done
	-for file in $(srcdir)/gsearchtool/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gsearch_helpdir)/stylesheet-images/$$basefile; \
	done



