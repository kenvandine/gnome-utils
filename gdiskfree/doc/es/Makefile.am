## Process this file with automake to produce Makefile.in.
##
gdiskfree_helpdir = $(datadir)/gnome/help/gdiskfree/es

SGML_FILES = \
	gdiskfree.sgml

HTML_FILES = index.html authors.html 	prefs.html	usage.html	\
	ln7.html

EXTRA_DIST = \
	topic.dat gdiskfree $(SGML_FILES)

gdiskfree_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d gdiskfree ] ; then cd $(srcdir) && db2html gdiskfree.sgml ; fi
	-cd $(srcdir) && cp gdiskfree/$@ .
	if [ -d gdiskfree.junk ] ; then cd $(srcdir) && rm -rf gdiskfree.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gdiskfree 
	$(mkinstalldirs) $(distdir)/gdiskfree/stylesheet-images
	-cp $(srcdir)/gdiskfree/*.html gdiskfree/*.css $(distdir)/gdiskfree
	-cp $(srcdir)/gdiskfree/stylesheet-images/*.gif \
	 $(distdir)/gdiskfree/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.sgml $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gdiskfree

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gdiskfree_helpdir)/stylesheet-images
	-for file in $(srcdir)/gdiskfree/*.html $(srcdir)/gdiskfree/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir); \
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir); \
	done

	-for file in $(srcdir)/gdiskfree/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gdiskfree_helpdir)/stylesheet-images/$$basefile; \
	done



