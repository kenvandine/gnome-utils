## Process this file with automake to produce Makefile.in

gstripchart_helpdir = $(datadir)/gnome/help/gstripchart/C
gstripchart_help_DATA = $(HTML_FILES) topic.dat main.png text.png param.png

EXTRA_DIST = gstripchart.sgml gnome-legalnotice.sgml topic.dat gstripchart

HTML_FILES = docbook.css \
 index.html ln7.html usage.html prefs.html bugs.html authors.html license.html

TMP_FILES = $(HTML_FILES:%=gstripchart/%)

$(HTML_FILES):  $(TMP_FILES)
	-cd $(srcdir) && cp $(TMP_FILES) .

$(TMP_FILES):   gstripchart.sgml
	db2html $<

dist-hook:
	$(mkinstalldirs) $(distdir)/gstripchart
	-cp $(srcdir)/*.png $(srcdir)/*.html $(distdir)
	-cp $(srcdir)/gstripchart/*.html $(distdir)/gstripchart
	-cp $(srcdir)/gstripchart/*.css $(distdir)/gstripchart
