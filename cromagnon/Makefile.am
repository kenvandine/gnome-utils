Utilitiesdir = $(datadir)/gnome/apps/Utilities
GNOMEG = $(bindir)/gnomeg

bin_SCRIPTS = cromagnon
Utilities_DATA = Cromagnon.desktop

cromagnon: cromagnon.scm Makefile
	sed -e "s#@GNOMEG@#$(GNOMEG)#" -e "s#@MYVERSION@#$(VERSION)#"<$< >$@
	chmod +x $@

EXTRA_DIST = cromagnon.scm Cromagnon.desktop
