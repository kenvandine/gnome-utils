bin_PROGRAMS = gnome-exe-handler

#SUBDIRS = help

INCLUDES = \
	-DG_LOG_DOMAIN=\"ExeHandler\" 			\
	-I$(top_srcdir) 				\
	$(GNOME_UTILS_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

gnome_exe_handler_SOURCES = 	\
	gnome-exe-handler.c

gnome_exe_handler_LDADD = 	\
	$(GNOME_UTILS_LIBS)

man_MANS = gnome-exe-handler.1

Mimedir = $(datadir)/mime-info
Mime_in_files = gnome-exe.keys.in
Mime_DATA = $(Mime_in_files:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@

Appregdir = $(datadir)/application-registry
Appreg_DATA = gnome-exe.applications

EXTRA_DIST = gnome-exe-handler.1 $(Mime_in_files) $(Appreg_DATA)
