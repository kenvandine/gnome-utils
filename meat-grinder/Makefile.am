bin_PROGRAMS = meat-grinder

INCLUDES = \
	-I$(top_srcdir) 				\
	-DG_LOG_DOMAIN=\"MeatGrinder\" 			\
	$(GNOME_INCLUDEDIR)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

meat_grinder_SOURCES = 	\
	meat-grinder.c

meat_grinder_LDADD = 	\
	$(GNOME_LIBS)
