
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gsu-test-server gsu-server

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
	gsu.c		\
	gsu-gui-api.c

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
