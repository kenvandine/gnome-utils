## Process this file with automake to produce Makefile.in.

man_MANS = gtt.1

EXTRA_DIST = gtt-db.sgml \
             gtt.1	 \
             index.html  \
	     topic.dat

gtt_helpdir = $(datadir)/gnome/help/gtt/C

install-data-local:
	$(mkinstalldirs) $(gtt_helpdir)
	$(INSTALL_DATA)  $(srcdir)/index.html $(gtt_helpdir)
	$(INSTALL_DATA)  $(srcdir)/topic.dat $(gtt_helpdir)

