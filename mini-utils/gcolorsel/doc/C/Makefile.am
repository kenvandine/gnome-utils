## Process this file with automake to produce Makefile.in
gcolorsel_helpdir = $(datadir)/gnome/help/gcolorsel/C

SGML_FILES = \
	gcolorsel.sgml

HTML_FILES = \
	index.html

EXTRA_DIST = \
	topic.dat gcolorsel $(SGML_FILES)

gcolorsel_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d gcolorsel ] ; then cd $(srcdir) && db2html gcolorsel.sgml ; fi
	-cd $(srcdir) && cp gcolorsel/$@ .
	if [ -d gcolorsel.junk ] ; then cd $(srcdir) && rm -rf gcolorsel.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gcolorsel 
	$(mkinstalldirs) $(distdir)/gcolorsel/stylesheet-images
	-cp $(srcdir)/gcolorsel/*.html gcolorsel/*.css $(distdir)/gcolorsel
	-cp $(srcdir)/gcolorsel/stylesheet-images/*.gif \
	 $(distdir)/gcolorsel/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gcolorsel

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gcolorsel_helpdir)/stylesheet-images
	-for file in $(srcdir)/gcolorsel/*.html $(srcdir)/gcolorsel/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcolorsel_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcolorsel_helpdir); \
	done
	-for file in $(srcdir)/gcolorsel/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcolorsel_helpdir)/stylesheet-images/$$basefile; \
	done



