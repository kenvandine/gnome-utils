xpmhelp_base = $(datadir)/gnome/help/cal

CFLAGS += -Wall

INCLUDES = 			\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)				\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I../../gcalendar

bin_PROGRAMS = gnomecard

gnomecard_SOURCES =	\
        $(CORBA_SRCLIST) \
	addresslist.c   \
	addresslist.h   \
	card.c		\
	card.h		\
	columnhdrs.c    \
	columnhdrs.h    \
	cardtypes.h	\
	canvas.c	\
	canvas.h	\
	del.c		\
	del.h		\
	dialog.c        \
	dialog.h        \
	gnomecard.c	\
	gnomecard.h	\
	images.h        \
	init.c          \
	init.h          \
	list.c		\
	list.h		\
	my.c            \
	my.h            \
	phonelist.c     \
	phonelist.h     \
	pix.c           \
	pix.h           \
	popup-menu.c	\
	popup-menu.h	\
	sort.c          \
	sort.h          \
	pairs.h		\
	gnome-pim-impl.c
#         \
#	tree.c          \
#	tree.h

CORBA_SRCLIST = gnome-pim-common.c \
	gnome-pim-skels.c	\
	gnome-pim-stubs.c	\
	gnome-pim.h

CLEANFILES += $(CORBA_SRCLIST)

$(CORBA_SRCLIST): $(top_srcdir)/idl/gnome-pim.idl $(ORBIT_IDL)
	orbit-idl $(top_srcdir)/idl/gnome-pim.idl

LINK_FLAGS = \
	$(INTLLIBS) ../versit/libversit.a

gnomecard_LDADD = $(LINK_FLAGS)\
	$(GNOME_LIBDIR) \
	$(GNOMEGNORBA_LIBS)

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

install-data-local:
	$(mkinstalldirs) $(prefix)/etc/CORBA/servers
	$(INSTALL_DATA)  $(srcdir)/gnomecard.gnorba \
		$(prefix)/etc/CORBA/servers/gnomecard.gnorba

