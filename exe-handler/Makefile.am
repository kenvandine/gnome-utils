bin_PROGRAMS = gnome-exe-handler

INCLUDES = \
	-DG_LOG_DOMAIN=\"ExeHandler\" 			\
	-I$(top_srcdir) 				\
	$(GNOME_INCLUDEDIR)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

gnome_exe_handler_SOURCES = 	\
	gnome-exe-handler.c

gnome_exe_handler_LDADD = 	\
	$(GNOME_LIBS)

man_MANS = gnome-exe-handler.1

Mimedir = $(datadir)/mime-info
Mime_DATA = gnome-exe.keys gnome-exe.applications

EXTRA_DIST = gnome-exe-handler.1 $(Mime_DATA)