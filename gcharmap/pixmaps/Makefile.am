# Process this file with automake to produce Makefile.in

EXTRA_DIST = gcharmap.xpm Character_Map.xpm logo.xpm

gcharmap_sharedir = $(datadir)/gcharmap

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_sharedir)
	$(INSTALL_DATA) gcharmap.xpm $(DESTDIR)$(gcharmap_sharedir)
	$(INSTALL_DATA) Character_Map.xpm $(DESTDIR)$(gcharmap_sharedir)
	$(INSTALL_DATA) logo.xpm $(DESTDIR)$(gcharmap_sharedir)
