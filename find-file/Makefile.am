Utilitiesdir = $(datadir)/apps/Utilities

GNOMEG = $(bindir)/gnomeg

bin_SCRIPTS = find-file

find-file: nsearch.scm Makefile
	sed -e "s#@GNOMEG@#$(GNOMEG)#" -e "s#@MYVERSION@#$(VERSION)#"<$< >$@
	chmod +x $@

EXTRA_DIST = nsearch.scm nsearch.desktop

install-data-local:
	$(INSTALL_DATA) $(srcdir)/nsearch.desktop $(Utilitiesdir)/"Find files.desktop"
