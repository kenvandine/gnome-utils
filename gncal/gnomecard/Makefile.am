#help_base = $(datadir)/gnome/help/cal

CFLAGS += -Wall

INCLUDES = 			\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)				\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I../../gcalendar

bin_PROGRAMS = gnomecard

gnomecard_SOURCES = \
	card.c \
	card.h \
	pairs.h \
	gnomecard.c
	
LINK_FLAGS = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS) ../versit/libversit.a

gnomecard_LDADD = $(LINK_FLAGS)

EXTRA_DIST = \
	cardedit.xpm \
	cardnew.xpm \
	world.xpm

