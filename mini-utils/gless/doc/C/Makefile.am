## Process this file with automake to produce Makefile.in.
##
gless_helpdir = $(datadir)/gnome/help/gless/C

SGML_FILES = \
	gless.sgml

HTML_FILES = authors.html	index.html	ln7.html	usage.html


EXTRA_DIST = \
	topic.dat gless $(SGML_FILES)

gless_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d gless ] ; then cd $(srcdir) && db2html gless.sgml ; fi
	-cd $(srcdir) && cp gless/$@ .
	if [ -d gless.junk ] ; then cd $(srcdir) && rm -rf gless.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gless 
	$(mkinstalldirs) $(distdir)/gless/stylesheet-images
	-cp $(srcdir)/gless/*.html gless/*.css $(distdir)/gless
	-cp $(srcdir)/gless/stylesheet-images/*.gif \
	 $(distdir)/gless/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gless

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gless_helpdir)/stylesheet-images
	-for file in $(srcdir)/gless/*.html $(srcdir)/gless/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gless_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gless_helpdir); \
	done
	-for file in $(srcdir)/gless/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gless_helpdir)/stylesheet-images/$$basefile; \
	done



