
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gsu-helper

EXTRA_PROGRAMS = gsu

gsu_SOURCES = \
	gsu.c

gsu_helper_SOURCES = \
	gsu-helper.c		\
	xmalloc.c		\
	xstrdup.c

gsu_helper_LDADD = \
	$(GLIB_LIBS)		\
	$(PAM_LIBS)

gsu_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)
