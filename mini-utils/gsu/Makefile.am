
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gsu-helper gsu-test gsu-test-server gsu-server

EXTRA_PROGRAMS = gsu

CORBA_GENERATED = \
	gsu-common.c	\
	gsu-skels.c	\
	gsu-stubs.c	\
	gsu.h

idl_dir = $(top_srcdir)/idl

IDL_FLAGS = -I $(idl_dir) -I $(datadir)/idl -I $(GNOME_IDL_DIR) -I$(includedir) $(GNOME_INCLUDEDIR)

$(CORBA_GENERATED): $(idl_dir)/gsu.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(idl_dir)/gsu.idl

BUILT_SOURCES = \
	$(CORBA_GENERATED)

gsu_SOURCES = \
	gsu.c

gsu_helper_SOURCES = \
	gsu-helper.c		\
	xmalloc.c		\
	xstrdup.c

gsu_test_SOURCES = \
	gsu-lib.c		\
	gsu-lib.h		\
	gsu-test.c

gsu_test_server_SOURCES = \
	$(CORBA_GENERATED)	\
	gsu-test-server.c

gsu_server_SOURCES = \
	$(CORBA_GENERATED)	\
	gsu-server.c		\
	gsu-impl.c		\
	gsu-impl.h		\
	gsu-lib.c		\
	gsu-lib.h

gsu_helper_LDADD = \
	$(PAM_LIBS)

gsu_test_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOMEUI_LIBS)		\
	$(INTLLIBS)

gsu_test_server_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOMEUI_LIBS)		\
	$(GNOMEGNORBA_LIBS)	\
	$(INTLLIBS)

gsu_server_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOMEUI_LIBS)		\
	$(GNOMEGNORBA_LIBS)	\
	$(INTLLIBS)

gsu_LDADD = \
	$(GNOME_LIBDIR)         \
	$(GTKXMHTML_LIBS)	\
	$(GNOMEUI_LIBS) 	\
	$(INTLLIBS)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = gsu.gnorba

install-data-local:
	$(INSTALL_DATA) $(srcdir)/gsu.gnorba \
		$(sysconfdir)/CORBA/servers/gsu.gnorba
