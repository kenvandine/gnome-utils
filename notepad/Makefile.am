## Process this file with automake to produce Makefile.in.

GNOMEG = $(bindir)/gnomeg

bin_SCRIPTS = notepad

notepad: notepad.scm Makefile
	sed -e "s#@GNOMEG@#$(GNOMEG)#" -e "s#@MYVERSION@#$(VERSION)#"<$< >$@
	chmod +x $@

EXTRA_DIST = notepad.scm Notepad.desktop.in $(top_srcdir)/sgmldocs.make

Utilitiesdir = $(datadir)/gnome/apps/Utilities
Utilities_in_files = Notepad.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

MOSTLYCLEANFILES = notepad
