## Process this file with automake to produce Makefile.in.

GNOMEG = $(bindir)/gnomeg

bin_SCRIPTS = notepad

notepad: notepad.scm Makefile
	sed -e "s#@GNOMEG@#$(GNOMEG)#" -e "s#@MYVERSION@#$(VERSION)#"<$< >$@
	chmod +x $@

EXTRA_DIST = notepad.scm Notepad.desktop

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_DATA = Notepad.desktop

MOSTLYCLEANFILES = notepad
