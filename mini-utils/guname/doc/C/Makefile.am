## Process this file with automake to produce Makefile.in.
##
guname_helpdir = $(datadir)/gnome/help/guname/C

SGML_FILES = \
	guname.sgml

HTML_FILES = authors.html 	index.html	ln7.html	\
	usage.html	license.html

EXTRA_DIST = \
	topic.dat guname $(SGML_FILES)

guname_help_DATA = ${HTML_FILES} topic.dat

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
	if [ ! -d guname ] ; then cd $(srcdir) && db2html guname.sgml ; fi
	-cd $(srcdir) && cp guname/$@ .
	if [ -d guname.junk ] ; then cd $(srcdir) && rm -rf guname.junk ; fi

dist-hook:
	$(mkinstalldirs) $(distdir)/guname 
	$(mkinstalldirs) $(distdir)/guname/stylesheet-images
	-cp $(srcdir)/guname/*.html guname/*.css $(distdir)/guname
	-cp $(srcdir)/guname/stylesheet-images/*.gif \
	 $(distdir)/guname/stylesheet-images
	-cp $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/*.sgml $(distdir)
	-cp $(srcdir)/*.png $(distdir)

distclean-generic:
	-rm -f Makefile
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-rm -f $(HTML_FILES)
	-rm -rf guname

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(guname_helpdir)/stylesheet-images
	-for file in $(srcdir)/guname/*.html $(srcdir)/guname/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(guname_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(guname_helpdir); \
	done
	-for file in $(srcdir)/*.sgml ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(guname_helpdir); \
	done
	-for file in $(srcdir)/guname/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(guname_helpdir)/stylesheet-images/$$basefile; \
	done



