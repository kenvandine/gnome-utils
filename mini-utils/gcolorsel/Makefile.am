# Process this file with automake to produce Makefile.in

SUBDIRS = glade doc

Utilities_DATA = gcolorsel.desktop

gladedir = $(datadir)/gcolorsel/glade
Utilitiesdir = $(datadir)/gnome/apps/Utilities

sysdir = $(datadir)/applets/Utility
sys_DATA = gcolorsel_applet.desktop

gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = gcolorsel_applet.gnorba

CFLAGS += -DGCOLORSEL_GLADEDIR=\""$(gladedir)/"\"

if BUILD_GCOLORSEL_APPLET

GCOLORSEL_APPLET_CFLAGS = -DHAVE_GNOME_APPLET
GCOLORSEL_APPLET_LIBS = $(GNOME_APPLETS_LIBS)

endif

INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR) $(GCOLORSEL_APPLET_CFLAGS) @PIXBUF_CFLAGS@

bin_PROGRAMS = gcolorsel

gcolorsel_SOURCES = 	dialog-new-doc.c		\
			dialog-new-view.c		\
			dialog-prefs.c			\
			dialogs.h			\
			gcolorsel.c			\
			gcolorsel.h			\
			idle.c				\
			idle.h				\
			menus.c				\
			menus.h				\
			mdi-color-generic.c		\
			mdi-color-generic.h		\
			mdi-color.c			\
			mdi-color.h			\
			mdi-color-file.c		\
			mdi-color-file.h		\
			mdi-color-virtual.c		\
			mdi-color-virtual.h		\
			mdi-color-virtual-monitor.c	\
			mdi-color-virtual-monitor.h	\
			mdi-color-virtual-rgb.c		\
			mdi-color-virtual-rgb.h		\
			session.c			\
			session.h			\
			utils.c				\
			utils.h				\
			view-color-edit.c		\
			view-color-edit.h		\
			view-color-generic.c		\
			view-color-generic.h		\
			view-color-list.c		\
			view-color-list.h		\
			view-color-grid.c		\
			view-color-grid.h		\
			widget-color-grid.c		\
			widget-color-grid.h		\
			widget-control-generic.c	\
			widget-control-generic.h	\
			widget-control-virtual-rgb.c	\
			widget-control-virtual-rgb.h

gcolorsel_LDADD = $(GNOME_LIBDIR) \
	    	  $(GNOMEUI_LIBS) \
	     	  $(INTLLIBS) \
	          -lxml \
	          -lglade \
	          -lglade-gnome \
		  $(GCOLORSEL_APPLET_LIBS) \
		  @PIXBUF_LIBS@

EXTRA_DIST = gcolorsel.desktop \
	     gcolorsel_applet.desktop \
	     gcolorsel_applet.gnorba

CLEANFILES=*~

install-exec-local:
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f gcolorsel_applet && \
	$(LN_S) gcolorsel gcolorsel_applet
