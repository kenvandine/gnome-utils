
Utilitiesdir =						\
	$(datadir)/apps/Utilities

help_base =						\
	$(datadir)/gnome/help/gstripchart

CFLAGS += -Wall

INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS =						\
	gstripchart

gstripchart_SOURCES =					\
	gstripchart.c

gstripchart_LDADD =					\
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(LIBGTOP_LIBS)					\
	$(LIBGTOP_EXTRA_LIBS)


EXTRA_DIST = 						\
	gstripchart.desktop	 			\
	gstripchart.html topic.dat			\
	gstripchart.conf

Utilities_DATA =					\
	gstripchart.desktop

install-data-local:
	$(mkinstalldirs) $(help_base)/C
	$(INSTALL_DATA) $(srcdir)/topic.dat $(help_base)/C/topic.dat
	$(INSTALL_DATA)					\
		$(srcdir)/gstripchart.html		\
		$(help_base)/C/gstripchart.html
	rm -f $(help_base)/C/index.html ; \
		$(LN_S) $(help_base)/C/gstripchart.html $(help_base)/C/index.html     
