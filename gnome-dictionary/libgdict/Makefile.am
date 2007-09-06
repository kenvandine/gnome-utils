## Process this file with automake to produce Makefile.in

NULL =

if ENABLE_DEBUG
GDICT_ENABLE_DEBUG=-DGDICT_ENABLE_DEBUG=1
else
GDICT_ENABLE_DEBUG=
endif

INCLUDES = -DG_LOG_DOMAIN=\"Gdict\" 				\
	-DDATADIR=\""$(datadir)"\" 				\
	-DLIBDIR=\""$(libdir)"\" 				\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 			\
	-DPREFIX=\""$(prefix)"\" 				\
	-DGNOMELOCALEDIR=\""$(gnomeutilslocaledir)"\" 		\
	-DGDICTSOURCESDIR=\""$(datadir)/gdict-1.0/sources"\" 	\
	-DGDICT_ENABLE_INTERNALS=1 				\
	$(GDICT_ENABLE_DEBUG)					\
	$(DISABLE_DEPRECATED_CFLAGS) 				\
	$(WARN_CFLAGS) 						\
	$(NULL)

lib_LTLIBRARIES = libgdict-1.0.la

# GNOME Dictionary shared library
libgdict_1_0_la_SOURCES =	\
	gdict-private.h		\
	gdict-debug.c		\
	gdict-debug.h		\
	gdict-context.c		\
	gdict-context.h		\
	gdict-context-private.h	\
	gdict-client-context.c	\
	gdict-client-context.h	\
	gdict-database-chooser.h \
	gdict-database-chooser.c \
	gdict-defbox.c 		\
	gdict-defbox.h 		\
	gdict-enum-types.c	\
	gdict-enum-types.h	\
	gdict-marshal.c		\
	gdict-marshal.h		\
	gdict-source.c		\
	gdict-source.h		\
	gdict-source-loader.c	\
	gdict-source-loader.h	\
	gdict-speller.c		\
	gdict-speller.h		\
	gdict-strategy-chooser.h \
	gdict-strategy-chooser.c \
	gdict-utils.c		\
	gdict-utils.h		\
	$(NULL)

libgdict_1_0_la_CFLAGS = $(LIBGDICT_CFLAGS)
libgdict_1_0_la_LDFLAGS = \
	-version-info $(LIBGDICT_LT_VERSION) \
	-export-dynamic \
	-no-undefined
libgdict_1_0_la_LIBADD = $(LIBGDICT_LIBS)

libgdict_includedir = $(includedir)/gdict-1.0/gdict
libgdict_include_HEADERS = 	\
	gdict-context.h		\
	gdict-client-context.h	\
	gdict-database-chooser.h \
	gdict-defbox.h 		\
	gdict-enum-types.h	\
	gdict-source.h		\
	gdict-source-loader.h	\
	gdict-speller.h		\
	gdict-strategy-chooser.h \
	gdict-utils.h		\
	gdict-version.h 	\
	gdict.h			\
	$(NULL)

$(libgdict_1_0_la_OBJECTS): $(BUILT_SOURCES)

gdict_built_files = 		\
	gdict-enum-types.h	\
	gdict-enum-types.c	\
	gdict-marshal.h		\
	gdict-marshal.c

stamp_files = stamp-gdict-enum-types.h stamp-gdict-marshal.h

gen_sources = xgen-gmh xgen-gmc xgen-geth xgen-getc
CLEANFILES = $(gen_sources) $(stamp_files)
MAINTAINERCLEANFILES = $(stamp_files) $(gdict_built_files)

EXTRA_DIST = 				\
	gdict-version.h.in 		\
	gdict-enum-types.h.in		\
	gdict-enum-types.c.in		\
	gdict-marshal.list		\
	gdict-1.0.pc.in			\
	gdict-1.0-uninstalled.pc.in	\
	$(NULL)

BUILT_SOURCES = $(gdict_built_files) 

gdict-marshal.h: stamp-gdict-marshal.h
	@true
stamp-gdict-marshal.h: gdict-marshal.list $(GLIB_GENMARSHAL) Makefile
	$(GLIB_GENMARSHAL) $< --header --prefix=gdict_marshal >> xgen-gmh \
	&& (cmp -s xgen-gmh gdict-marshal.h || cp xgen-gmh gdict-marshal.h) \
	&& rm -f xgen-gmh \
	&& echo timestamp > $(@F)

gdict-marshal.c: gdict-marshal.list $(GLIB_GENMARSHAL) Makefile
	@echo "#include \"gdict-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gdict_marshal >> xgen-gmc \
	&& cp xgen-gmc gdict-marshal.c \
	&& rm -f xgen-gmc

libgdict_enum_headers = \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-context.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-client-context.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-database-chooser.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-defbox.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-source.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-source-loader.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-speller.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-strategy-chooser.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-utils.h \
	$(NULL)

gdict-enum-types.h: stamp-gdict-enum-types.h
	@true
stamp-gdict-enum-types.h: $(libgdict_enum_headers) Makefile
	( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/gdict-enum-types.h.in \
	  $(libgdict_enum_headers) ) >> xgen-ceth && \
	(cmp -s xgen-ceth gdict-enum-types.h || cp xgen-ceth gdict-enum-types.h) && \
	rm -f xgen-ceth && \
	echo timestamp > $(@F)

gdict-enum-types.c: $(libgdict_enum_headers) Makefile
	( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/gdict-enum-types.c.in \
	  $(libgdict_enum_headers) ) >> xgen-cetc && \
	cp xgen-cetc gdict-enum-types.c && \
	rm -f xgen-cetc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdict-1.0.pc

distclean-local:
	if test $(srcdir) != .; then \
		rm -f $(MAINTAINERCLEANFILES); \
	fi
	
