NULL =

INCLUDES = \
	-DPREFIX=\""$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBDIR=\""$(libdir)"\"			\
	-DDATADIR=\""$(datadir)"\" 			\
	-DUIDATADIR=\""$(datadir)/gnome-2.0/ui"\" 	\
	-DPKGDATADIR=\""$(datadir)/gnome-dictionary"\" 	\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

bin_PROGRAMS = gnome-dictionary

if BUILD_GDICT_APPLET
libexec_PROGRAMS = gnome-dictionary-applet
endif

gnome_dictionary_SOURCES = \
	gdict-about.c		\
	gdict-about.h		\
	gdict-app.c		\
	gdict-app.h		\
	gdict-common.c 		\
	gdict-common.h		\
	gdict-pref-dialog.c	\
	gdict-pref-dialog.h	\
	gdict-print.c 		\
	gdict-print.h 		\
	gdict-sidebar.c		\
	gdict-sidebar.h		\
	gdict-source-dialog.c 	\
	gdict-source-dialog.h 	\
	gdict-window.c 		\
	gdict-window.h 		\
	main.c			\
	$(NULL)
gnome_dictionary_CFLAGS = \
	-I$(top_builddir)/gnome-dictionary \
	-I$(top_srcdir)/gnome-dictionary \
	$(GNOME_UTILS_CFLAGS) \
	$(NULL)
gnome_dictionary_LDADD = \
	-lm			\
	$(top_builddir)/gnome-dictionary/libgdict/libgdict-1.0.la \
	$(GNOME_UTILS_LIBS) \
	$(NULL)

if BUILD_GDICT_APPLET

gnome_dictionary_applet_SOURCES = \
	gdict-about.c 		\
	gdict-about.h		\
	gdict-aligned-window.c	\
	gdict-aligned-window.h	\
	gdict-applet.c		\
	gdict-applet.h		\
	gdict-common.c		\
	gdict-common.h		\
	gdict-pref-dialog.c	\
	gdict-pref-dialog.h	\
	gdict-print.c 		\
	gdict-print.h 		\
	gdict-sidebar.c		\
	gdict-sidebar.h		\
	gdict-source-dialog.c 	\
	gdict-source-dialog.h 	\
	$(NULL)
gnome_dictionary_applet_CFLAGS = \
	-I$(top_builddir)/gnome-dictionary \
	-I$(top_srcdir)/gnome-dictionary \
	$(GNOME_UTILS_CFLAGS) \
	$(APPLET_CFLAGS) \
	$(NULL)
gnome_dictionary_applet_LDADD = \
	-lm			\
	$(top_builddir)/gnome-dictionary/libgdict/libgdict-1.0.la \
	$(GNOME_UTILS_LIBS) \
	$(APPLET_LIBS) \
	$(NULL)

endif # BUILD_GDICT_APPLET
