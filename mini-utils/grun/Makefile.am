
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR) 

bin_PROGRAMS = grun

grun_SOURCES = \
	grun.c

grun_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)


Utilitiesdir = $(datadir)/gnome/apps/Utilities
