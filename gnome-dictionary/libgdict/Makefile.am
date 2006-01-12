## Process this file with automake to produce Makefile.in

NULL =

INCLUDES = -DG_LOG_DOMAIN=\"Gdict\" 				\
	-DDATADIR=\""$(datadir)"\" 				\
	-DLIBDIR=\""$(libdir)"\" 				\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 			\
	-DPREFIX=\""$(prefix)"\" 				\
	-DGNOMELOCALEDIR=\""$(gnomeutilslocaledir)"\" 		\
	-DGDICTSOURCESDIR=\""$(datadir)/gdict-1.0/sources"\" 	\
	-DGDICT_ENABLE_INTERNALS=1 				\
	$(DISABLE_DEPRECATED_CFLAGS) 				\
	$(WARN_CFLAGS) 						\
	$(NULL)

lib_LTLIBRARIES = libgdict-1.0.la

# GNOME Dictionary shared library
libgdict_1_0_la_SOURCES =	\
	gdict-context.c		\
	gdict-context.h		\
	gdict-context-private.h	\
	gdict-client-context.c	\
	gdict-client-context.h	\
	gdict-defbox.c 		\
	gdict-defbox.h 		\
	gdict-entry.c		\
	gdict-entry.h		\
	gdict-enum-types.c	\
	gdict-enum-types.h	\
	gdict-marshal.c		\
	gdict-marshal.h		\
	gdict-source.c		\
	gdict-source.h		\
	gdict-source-loader.c	\
	gdict-source-loader.h	\
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
	gdict-defbox.h 		\
	gdict-entry.h		\
	gdict-enum-types.h	\
	gdict-source.h		\
	gdict-source-loader.h	\
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
CLEANFILES = $(gen_sources)
MAINTAINERCLEANFILES = $(stamp_files) $(gdict_built_files)

EXTRA_DIST = 				\
	gdict-version.h.in 		\
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
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-defbox.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-entry.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-source.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-source-loader.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-utils.h \
	$(NULL)

gdict-enum-types.h: stamp-gdict-enum-types.h
	@true
stamp-gdict-enum-types.h: $(libgdict_enum_headers) Makefile
	$(GLIB_MKENUMS) \
		--fhead "#ifndef __GDICT_ENUM_TYPES_H__\n" \
		--fhead "#define __GDICT_ENUM_TYPES_H__\n\n" \
		--fhead "G_BEGIN_DECLS\n\n" \
		--ftail "G_END_DECLS\n\n" \
		--ftail "#endif\n" \
		--fprod "/* --- @filename@ --- */\n" \
		--eprod "#define GDICT_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
		--eprod "GType @enum_name@_get_type (void);\n\n" \
	$(libgdict_enum_headers) >> xgen-geth \
	&& (cmp xgen-geth gdict-enum-types.h || cp xgen-geth gdict-enum-types.h) \
	&& rm -f xgen-geth \
	&& echo timestamp > $(@F)

gdict-enum-types.c: $(libgdict_enum_headers) Makefile
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fhead "#include \"gdict-enum-types.h\"\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--fprod "\n#include \"@filename@\"" \
		--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
		--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\n" \
		--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
		--vtail "  static GType type = 0;\n\n" \
		--vtail "  if (!type)\n" \
		--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
		--vtail "  return type;\n}\n\n" \
	$(libgdict_enum_headers) >> xgen-getc \
	&& cp xgen-getc gdict-enum-types.c \
	&& rm -f xgen-getc
	

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdict-1.0.pc

distclean-local:
	if test $(srcdir) != .; then \
		rm -f $(MAINTAINERCLEANFILES); \
	fi
	
