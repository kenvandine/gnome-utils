## Process this file with automake to produce Makefile.in.
##
gw_helpdir = $(datadir)/gnome/help/gw/C

SGML_FILES = \
	gw.sgml

HTML_FILES = authors.html	license.html	prefs.html	usage.html \
	bugs.html	index.html	ln7.html

EXTRA_DIST = \
	topic.dat gw $(SGML_FILES)

gw_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d gw ] ; then cd $(srcdir) && db2html gw.sgml ; fi
	-cd $(srcdir) && cp gw/$@ .
	if [ -d gw.junk ] ; then cd $(srcdir) && rm -rf gw.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gw 
	$(mkinstalldirs) $(distdir)/gw/stylesheet-images
	-cp $(srcdir)/gw/*.html gw/*.css $(distdir)/gw
	-cp $(srcdir)/gw/stylesheet-images/*.gif \
	 $(distdir)/gw/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.sgml $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gw

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gw_helpdir)/stylesheet-images
	-for file in $(srcdir)/gw/*.html $(srcdir)/gw/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir); \
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir); \
	done
	-for file in $(srcdir)/gw/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gw_helpdir)/stylesheet-images/$$basefile; \
	done



