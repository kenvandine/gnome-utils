bin_PROGRAMS = gnome-character-map

INCLUDES =\
	-DG_LOG_DOMAIN=\"gcharmap\"\
	-I$(srcdir)/../pixmaps\
	-DDATADIR=\""$(datadir)"\"	\
	$(GNOME_UTILS_CFLAGS) \
	$(LIBBONOBOUI_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

gnome_character_map_SOURCES = \
        main.c\
        interface.c interface.h\
        menus.c menus.h\
        callbacks.c callbacks.h\
        asciiselect.c asciiselect.h\
	notifiers.c

gnome_character_map_LDADD = $(GNOME_UTILS_LIBS) $(LIBBONOBOUI_LIBS) $(INTLLIBS)

man_MANS = gnome-character-map.1

EXTRA_DIST = $(gnome_character_map_SOURCES) gnome-character-map.1
