
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = guname

guname_SOURCES = \
	guname.c	\
	info.c		\
	list.c		\
	moreinfo.c

noinst_HEADERS = \
	info.h		\
	list.h		\
	moreinfo.h	


guname_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

