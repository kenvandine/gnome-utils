INCLUDES = -DGTK_DISABLE_DEPRECATED \
           -DGDK_DISABLE_DEPRECATED \
           -DG_DISABLE_DEPRECATED


noinst_LTLIBRARIES = libgnomeui-deprecated.la

libgnomeui_deprecated_la_LIBADD = $(GNOME_UTILS_LIBS)
libgnomeui_deprecated_la_CFLAGS = $(GNOME_UTILS_CFLAGS) \
                                  $(WARN_CFLAGS)
libgnomeui_deprecated_la_SOURCES = gsearchtool-entry.h \
                                   gsearchtool-entry.c
