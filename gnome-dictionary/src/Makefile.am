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

libexec_PROGRAMS = gnome-dictionary-applet

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
	-I$(top_srcdir)/gnome-dictionary \
	$(GNOME_UTILS_CFLAGS) \
	$(LIBGLADE_CFLAGS) \
	$(LIBGNOMEPRINT_CFLAGS) \
	$(NULL)
gnome_dictionary_LDADD = \
	$(top_builddir)/gnome-dictionary/libgdict/libgdict-1.0.la \
	$(GNOME_UTILS_LIBS) \
	$(LIBGLADE_LIBS) \
	$(LIBGNOMEPRINT_LIBS) \
	$(NULL)

# the applet container widget
noinst_LTLIBRARIES = libgtkalignedwindow.la
libgtkalignedwindow_la_SOURCES = \
	gtkalignedwindow.h \
	gtkalignedwindow.c \
	$(NULL)
libgtkalignedwindow_la_CFLAGS = $(GNOME_UTILS_CFLAGS)
libgtkalignedwindow_la_LIBADD = $(GNOME_UTILS_LIBS)

gnome_dictionary_applet_SOURCES = \
	gdict-about.c 		\
	gdict-about.h		\
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
	-I$(top_srcdir)/gnome-dictionary \
	$(GNOME_UTILS_CFLAGS) \
	$(LIBGLADE_CFLAGS) \
	$(LIBGNOMEPRINT_CFLAGS) \
	$(APPLET_CFLAGS) \
	$(NULL)
gnome_dictionary_applet_LDADD = \
	$(top_builddir)/gnome-dictionary/libgdict/libgdict-1.0.la \
	libgtkalignedwindow.la \
	$(GNOME_UTILS_LIBS) \
	$(LIBGLADE_LIBS) \
	$(LIBGNOMEPRINT_LIBS) \
	$(APPLET_LIBS) \
	$(NULL)
