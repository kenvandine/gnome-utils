EXTRA_DIST = author.htm index.html intro.htm legal.htm \
             topic.dat tutor.htm usage.htm


gcharmap_helpdir = $(datadir)/gnome/help/gcharmap/C

dist-hook:
	-cp $(EXTRA_DIST) $(distdir)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) author.htm $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) index.html $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) intro.htm $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) legal.htm $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) topic.dat $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) tutor.htm $(DESTDIR)$(gcharmap_helpdir)
	$(INSTALL_DATA) usage.htm $(DESTDIR)$(gcharmap_helpdir)