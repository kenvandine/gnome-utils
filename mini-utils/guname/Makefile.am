INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR) $(LIBGTOP_NAMES_INCS)

bin_PROGRAMS = guname

if HAVE_LIBGTOP
moreinfo_c		= moreinfo.c
moreinfo_h		= moreinfo.h
endif

guname_SOURCES = \
	guname.c	\
	info.c		\
	list.c		\
	$(moreinfo_c)	\
	AUTHORS.h

noinst_HEADERS = \
	info.h		\
	list.h		\
	$(moreinfo_h)

guname_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)		\
	$(LIBGTOP_NAMES_LIBS)	\
	$(LIBGTOP_EXTRA_LIBS)

Rootdir = $(datadir)/gnome/apps/Utilities
Root_DATA = guname.desktop

EXTRA_DIST = 		\
	guname.desktop	\
	moreinfo.c	\
	moreinfo.h

