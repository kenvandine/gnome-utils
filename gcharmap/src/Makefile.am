bin_PROGRAMS = gcharmap

gcharmap_SOURCES = \
        asciiselect.c  asciiselect.h \
        interface.c    interface.h \
        main.c

gcharmap_LDFLAGS = `gnome-config --libs gnome gnomeui`
CFLAGS = -Wall -ansi `gnome-config --cflags gnome gnomeui`

EXTRA_DIST = asciiselect.c  asciiselect.h \
             interface.c    interface.h   \
             main.c