## Process this file with automake to produce Makefile.in.
##
gcharmap_helpdir = $(datadir)/gnome/help/gcharmap/C

SGML_FILES = \
	gcharmap.sgml

HTML_FILES = index.html authors.html 	usage.html	\
	ln7.html	bugs.html	license.html

EXTRA_DIST = \
	topic.dat gcharmap $(SGML_FILES)

gcharmap_help_DATA = ${HTML_FILES} topic.dat

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
# ./images/*       any images/figures needed for the documentation.
${HTML_FILES} : $(SGML_FILES)
	if [ ! -d gcharmap ] ; then cd $(srcdir) && db2html gcharmap.sgml ; fi
	-cd $(srcdir) && cp gcharmap/$@ .
	if [ -d gcharmap.junk ] ; then cd $(srcdir) && rm -rf gcharmap.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/gcharmap 
	$(mkinstalldirs) $(distdir)/images
	$(mkinstalldirs) $(distdir)/gcharmap/stylesheet-images
	-cp $(srcdir)/gcharmap/*.html gcharmap/*.css $(distdir)/gcharmap
	-cp $(srcdir)/gcharmap/stylesheet-images/*.gif \
	 $(distdir)/gcharmap/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/images/*.png $(distdir)/images

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf gcharmap

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_helpdir)/stylesheet-images
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_helpdir)/images
	-for file in $(srcdir)/gcharmap/*.html $(srcdir)/gcharmap/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcharmap_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcharmap_helpdir)/images; \
	done
	-for file in $(srcdir)/gcharmap/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(gcharmap_helpdir)/stylesheet-images/$$basefile; \
	done



