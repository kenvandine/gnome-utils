INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gdiskfree

gdiskfree_SOURCES =				\
	df.c					\
	mountlist.c				\
	mountlist.h				\
	fsusage.c				\
	fsusage.h				\
	pathmax.h				\
	xgetcwd.c				\
	save-cwd.c				\
	save-cwd.h

gdiskfree_LDADD = $(GNOME_LIBDIR) $(GTKXMHTML_LIBS) \
		   $(GNOMEUI_LIBS) $(INTLLIBS) -lgtktty
