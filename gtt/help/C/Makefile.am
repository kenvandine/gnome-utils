## Process this file with automake to produce Makefile.in.

EXTRA_DIST = gtt-db.sgml \
             index.html  \
	     topic.dat

gtt_helpdir = $(datadir)/gnome/help/gtt/C

install-data-local:
	$(mkinstalldirs) $(ghex_helpdir)
	$(INSTALL_DATA)  index.html $(ghex_helpdir)
	$(INSTALL_DATA)  topic.dat $(ghex_helpdir)

