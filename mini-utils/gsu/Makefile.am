
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gsu-helper gsu-test

EXTRA_PROGRAMS = gsu

gsu_SOURCES = \
	gsu.c

gsu_helper_SOURCES = \
	gsu-helper.c		\
	xmalloc.c		\
	xstrdup.c

gsu_test_SOURCES = \
	gsu-lib.c		\
	gsu-lib.h		\
	gsu-test.c

gsu_helper_LDADD = \
	$(PAM_LIBS)

gsu_test_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOMEUI_LIBS)		\
	$(INTLLIBS)

gsu_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)
