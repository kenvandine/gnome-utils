## Process this file with automake to produce Makefile.in.

EXTRA_DIST = index.html \
	     topic.dat

gshutdown_helpdir = $(datadir)/gnome/help/gshutdown/es

install-data-local:
	$(mkinstalldirs) $(gshutdown_helpdir)
	$(INSTALL_DATA)  $(srcdir)/index.html $(ghex_helpdir)
	$(INSTALL_DATA)  $(srcdir)/topic.dat $(ghex_helpdir)

