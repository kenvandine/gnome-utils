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

guname_DEPENDENCIES=AUTHORS.h

Rootdir = $(datadir)/apps
Root_DATA = guname.desktop

EXTRA_DIST = guname.desktop

AUTHORS.h: $(wildcard ../../../gnome-*/AUTHORS)
	cat ../../../gnome-*/AUTHORS \
	| sed -e 's,<[^>]*>,,g' -e 's,([^)]*),,g' -e 's, *$$,,g' \
	| grep -v '^$$' | sort -u \
	| awk '{print "\""$$0"\","}' > AUTHORS.h
