SUBDIRS = doc

bin_PROGRAMS = gdialog

gdialog_SOURCES =	\
	colors.h	\
	dialog.h	\
	checklist.c 	\
	dialog.c 	\
	guage.c 	\
	inputbox.c 	\
	menubox.c 	\
	mouse.c 	\
	msgbox.c 	\
	rc.c 		\
	textbox.c 	\
	util.c 		\
	yesno.c

INCLUDES=-I. $(GNOME_UTILS_CFLAGS) $(CURSES_INCLUDEDIR) 

gdialog_LDADD = \
	$(GNOME_UTILS_LIBS) \
	$(GTKXMHTML_LIBS) \
	$(INTLLIBS) \
	$(CURSES_LIBS)

man_MANS = gdialog.1

EXTRA_DIST = gdialog.1
