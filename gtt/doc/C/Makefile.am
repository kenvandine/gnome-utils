## Process this file with automake to produce Makefile.in.
##
gtt_helpdir = $(datadir)/gnome/help/gtt/C

SGML_FILES = \
	gtt.sgml

HTML_FILES = authors.html	dialogs.html	index.html	ln10.html \
	bugs.html	license.html	preferences.html	usage.html

EXTRA_DIST = \
	topic.dat gtt $(SGML_FILES)

gtt_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d gtt ] ; then cd $(srcdir) && db2html gtt.sgml ; fi
	-cd $(srcdir) && cp gtt/$@ .
	if [ -d gtt.junk ] ; then cd $(srcdir) && rm -rf gtt.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gtt 
	$(mkinstalldirs) $(distdir)/gtt/stylesheet-images
	-cp $(srcdir)/gtt/*.html gtt/*.css $(distdir)/gtt
	-cp $(srcdir)/gtt/stylesheet-images/*.gif \
	 $(distdir)/gtt/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gtt

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gtt_helpdir)/stylesheet-images
	-for file in $(srcdir)/gtt/*.html $(srcdir)/gtt/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gtt_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gtt_helpdir); \
	done
	-for file in $(srcdir)/gtt/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gtt_helpdir)/stylesheet-images/$$basefile; \
	done



