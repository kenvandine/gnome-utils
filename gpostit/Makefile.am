dtdir=$(datadir)/apps/Utilities

INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	-I$(top_srcdir)/programs	\
	$(X_CFLAGS)

SUFFIXES = .m

.m.o:
	$(COMPILE) -c $<

COMPILE = $(CC) $(DEFS) $(GNOME_INCLUDEDIR) $(CPPFLAGS) $(CFLAGS)
LINK    = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) $(OBGNOME_LIBDIR) -o $@

bin_PROGRAMS = gpostit

gpostit_SOURCES = 	\
	gpostit.m

gpostit_LDADD = \
        $(OBGNOME_LIBS) -lgtktty $(OBGTK_LIBS) $(GNOMEUI_LIBS) \
        $(OBJC_LIBS) $(GNOME_LIBDIR)

EXTRA_DIST = gpostit.desktop

install-data-local:
	$(mkinstalldirs) $(dtdir)
	$(INSTALL_DATA) $(srcdir)/gpostit.desktop $(dtdir)/gpostit.desktop
