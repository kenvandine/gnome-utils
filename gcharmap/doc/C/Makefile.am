EXTRA_DIST = author.html index.html intro.html legal.html \
             topic.dat tutor.html usage.html


gcharmap_helpdir = $(datadir)/gnome/help/gcharmap/C

dist-hook:
	-cp $(EXTRA_DIST) $(distdir)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) author.html $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) index.html $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) intro.html $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) legal.html $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) topic.dat $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) tutor.html $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) usage.html $(DESTDIR)$(gcharmap_helpdir)
