#Utilitiesdir = $(datadir)/gnome/apps/Utilities

INCLUDES = 							\
	-I$(includedir)						\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl		\
	$(GNOME_INCLUDEDIR)					\
	-DGNOMEHELPPATH=\""$(datadir)/gnome/help\"" 	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""	\
	-DVERSION=\""$(VERSION)"\"

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        $(INTLLIBS)

bin_PROGRAMS = gnome-find nfind 

gnome_find_SOURCES = gnome-find.c specs.c ftypes.c
nfind_SOURCES = nfind.c glob.c getdate.y

dist_EXTRA = getdate.c

noinst_HEADERS = gnome-find.h
