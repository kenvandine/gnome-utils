## Process this file with automake to produce Makefile.in.

EXTRA_DIST = gtt-db.sgml \
	     index.html \
	     topic.dat

gtt_helpdir = $(datadir)/gnome/help/gtt/es

install-data-local:
	$(mkinstalldirs) $(gtt_helpdir)
	$(INSTALL_DATA)  $(srcdir)/index.html $(gtt_helpdir)
	$(INSTALL_DATA)  $(srcdir)/topic.dat $(gtt_helpdir)

