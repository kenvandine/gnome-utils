## Process this file with automake to produce Makefile.in.
##
fontsel_helpdir = $(datadir)/gnome/help/fontsel/C

SGML_FILES = \
	fontsel.sgml

HTML_FILES = index.html

EXTRA_DIST = \
	topic.dat fontsel $(SGML_FILES)

fontsel_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d fontsel ] ; then cd $(srcdir) && db2html fontsel.sgml ; fi
	-cd $(srcdir) && cp fontsel/$@ .
	if [ -d fontsel.junk ] ; then cd $(srcdir) && rm -rf fontsel.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/fontsel 
	$(mkinstalldirs) $(distdir)/fontsel/stylesheet-images
	-cp $(srcdir)/fontsel/*.html fontsel/*.css $(distdir)/fontsel
	-cp $(srcdir)/fontsel/stylesheet-images/*.gif \
	 $(distdir)/fontsel/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf fontsel

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(fontsel_helpdir)/stylesheet-images
	-for file in $(srcdir)/fontsel/*.html $(srcdir)/gfontsel/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(fontsel_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(fontsel_helpdir); \
	done
	-for file in $(srcdir)/fontsel/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(fontsel_helpdir)/stylesheet-images/$$basefile; \
	done



