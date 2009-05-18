INCLUDES = -DGTK_DISABLE_DEPRECATED \
           -DGDK_DISABLE_DEPRECATED \
           -DG_DISABLE_DEPRECATED

noinst_LTLIBRARIES = libeggsmclient.la

libeggsmclient_la_LIBADD = $(GNOME_UTILS_LIBS)
libeggsmclient_la_CFLAGS = $(GNOME_UTILS_CFLAGS) \
                           $(WARN_CFLAGS)
libeggsmclient_la_SOURCES = eggdesktopfile.h \
                            eggdesktopfile.c \
                            eggsmclient.h \
                            eggsmclient.c \
                            eggsmclient-private.h \
                            eggsmclient-xsmp.c
