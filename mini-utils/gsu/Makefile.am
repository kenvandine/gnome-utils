
INCLUDES = 			\
	-I$(includedir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gsu-helper gsu-test gsu-test-server gsu-server

EXTRA_PROGRAMS = gsu

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
	$(CORBA_SRCLIST)	\
	gsu-test-server.c

gsu_server_SOURCES = \
	$(CORBA_SRCLIST)	\
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

CORBA_SRCLIST = gsu-common.c \
	gsu-skels.c	\
	gsu-stubs.c	\
	gsu.h

CLEANFILES += $(CORBA_SRCLIST) my_gsu_idl

EXTRA_DIST = gsu.gnorba

$(CORBA_SRCLIST): my_gsu_idl

my_gsu_idl: $(top_srcdir)/idl/gsu.idl $(ORBIT_IDL)
	orbit-idl $(top_srcdir)/idl/gsu.idl

install-data-local:
	$(INSTALL_DATA) $(srcdir)/gsu.gnorba \
		$(sysconfdir)/CORBA/servers/gsu.gnorba
