
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS = guname

guname_SOURCES = \
	guname.c	\
	info.c		\
	list.c		\
	moreinfo.c	\
	fsusage.c	\
	mountlist.c

noinst_HEADERS = \
	info.h		\
	list.h		\
	moreinfo.h	\
	fsusage.h	\
	mountlist.h


guname_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)		\
	$(LIBGTOP_LIBS)

Rootdir = $(datadir)/apps
Root_DATA = guname.desktop

EXTRA_DIST = guname.desktop
