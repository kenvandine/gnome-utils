## Process this file with automake to produce Makefile.in

gdiskfree_helpdir = $(datadir)/gnome/help/gdiskfree/C
gdiskfree_help_DATA = index.html topic.dat

SGML_FILES = 		\
	gdiskfree.sgml
EXTRA_DIST = \
	topic.dat	index.html 	$(SGML_FILES)

all:

index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gdiskfree.sgml

