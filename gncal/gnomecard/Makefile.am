xpmhelp_base = $(datadir)/gnome/help/cal

CFLAGS += -Wall

INCLUDES = 			\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)				\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I../../gcalendar

bin_PROGRAMS = gnomecard

gnomecard_SOURCES =	\
	card.c		\
	card.h		\
	cardtypes.h	\
	pairs.h		\
	gnomecard.c	\
	gnomecard.h	\
	images.h
	
LINK_FLAGS = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS) ../versit/libversit.a

gnomecard_LDADD = $(LINK_FLAGS)

EXTRA_DIST =		\
	addr.xpm	\
	addrtool.xpm	\
	cardedit.xpm	\
	cardnew.xpm	\
	cardfind.xpm	\
	email.xpm	\
	expl.xpm	\
	first.xpm	\
	geo.xpm		\
	ident.xpm	\
	last.xpm	\
	org.xpm		\
	sec.xpm		\
	tel.xpm		\
	teltool.xpm	\
	phone.xpm	\
	world.xpm
