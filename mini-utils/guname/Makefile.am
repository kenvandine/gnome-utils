INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR) $(LIBGTOP_NAMES_INCS)

bin_PROGRAMS = guname

if HAVE_LIBGTOP
moreinfo_c		= moreinfo.c
moreinfo_h		= moreinfo.h
endif

BUILT_SOURCES = AUTHORS.h
guname_SOURCES = \
	guname.c	\
	info.c		\
	list.c		\
	$(moreinfo_c)

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

guname_DEPENDENCIES=AUTHORS.h

Rootdir = $(datadir)/apps
Root_DATA = guname.desktop

EXTRA_DIST = guname.desktop AUTHORS.h

AUTHORS.h: $(wildcard $(top_srcdir)/../gnome-*/AUTHORS)
	cat $(top_srcdir)/../gnome-*/AUTHORS \
	| sed -e 's,<[^>]*>,,g' -e 's,([^)]*),,g' -e 's, *$$,,g' \
	| grep -v '^$$' | sort -u \
	| awk '{print "\""$$0"\","}' > AUTHORS.h
