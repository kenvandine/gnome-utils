INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR) $(LIBGTOP_INCS)

bin_PROGRAMS = gdiskfree

gdiskfree_SOURCES =				\
	df.c					\
	pathmax.h				\
	xgetcwd.c				\
	save-cwd.c				\
	save-cwd.h


gdiskfree_LDADD = $(GNOME_LIBDIR) $(GTKXMHTML_LIBS) \
		  $(GNOMEUI_LIBS) $(INTLLIBS) -lgtktty \
		  $(LIBGTOP_LIBS) $(LIBGTOP_EXTRA_LIBS)
