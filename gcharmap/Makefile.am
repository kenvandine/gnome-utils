# Process this file with automake to produce Makefile.in

EXTRA_DIST = README TODO BUGS gcharmap.desktop 
SUBDIRS = src doc pixmaps

sysdir = $(datadir)/gnome/apps/Applications
sys_DATA = gcharmap.desktop

install-data-local:
	$(mkinstalldirs) $(prefix)/doc/gcharmap/
	$(INSTALL_DATA) AUTHORS $(prefix)/doc/gcharmap/AUTHORS
	$(INSTALL_DATA) COPYING $(prefix)/doc/gcharmap/COPYING
	$(INSTALL_DATA) ChangeLog $(prefix)/doc/gcharmap/ChangeLog
	$(INSTALL_DATA) NEWS $(prefix)/doc/gcharmap/NEWS
	$(INSTALL_DATA) README $(prefix)/doc/gcharmap/README
	$(INSTALL_DATA) TODO $(prefix)/doc/gcharmap/TODO
