## Process this file with automake to produce Makefile.in.

EXTRA_DIST = topic.dat # gtt-db.sgml index.html

gtt_helpdir = $(datadir)/gnome/help/gtt/de

install-data-local:
	$(mkinstalldirs) $(gtt_helpdir)
	#$(INSTALL_DATA)  $(srcdir)/index.html $(gtt_helpdir)
	( cd $(gtt_helpdir) && \
	  rm -f index.html && \
	  ln -s ../C/index.html . )
	$(INSTALL_DATA)  $(srcdir)/topic.dat $(gtt_helpdir)

