
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = edit-menus

edit_menus_SOURCES = \
	iconbrowse.c     \
        is_image.c       \	
 	menuentrywiz.c   \
 	menu.c           \
 	icewm-parser.c   \
 	icewm-saver.c    \
 	run_command.c    \
	edit-menus.c

edit_menus_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)
