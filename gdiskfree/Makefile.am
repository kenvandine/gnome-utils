INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gdiskfree

gdiskfree_SOURCES =				\
	df.c					\
	pathmax.h				\
	xgetcwd.c				\
	fsusage.c                               \
	mountlist.c				\
	save-cwd.c				\
	save-cwd.h                              \
	mountlist.h				\
	fsusage.h


gdiskfree_LDADD = $(GNOME_LIBDIR) $(GTKXMHTML_LIBS) \
		   $(GNOMEUI_LIBS) $(INTLLIBS) -lgtktty
