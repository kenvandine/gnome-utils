EXTRA_DIST = gcharmap.xpm "Character Map.xpm" logo.xpm


gcharmap_sharedir = $(datadir)/gcharmap

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_sharedir)
	$(INSTALL_DATA) gcharmap.xpm $(DESTDIR)$(gcharmap_sharedir)
	$(INSTALL_DATA) "Character Map.xpm" $(DESTDIR)$(gcharmap_sharedir)
	$(INSTALL_DATA) logo.xpm $(DESTDIR)$(gcharmap_sharedir)
	ln -s $(DESTDIR)$(gcharmap_sharedir)/"Character Map.xpm" $(DESTDIR)$(datadir)/pixmaps/