Utilitiesdir = $(datadir)/applications

SUBDIRS = data help libgnomeui-deprecated

INCLUDES = 	\
	$(GNOME_UTILS_CFLAGS)					\
	$(GIO_UNIX_CFLAGS)					\
	-I$(srcdir)/libgnomeui-deprecated			\
	-I$(top_srcdir)/libeggsmclient			\
	-DG_DISABLE_DEPRECATED					\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"			\
	-DDATADIR=\""$(datadir)"\"				\
	-DGREP_COMMAND=\""$(GREP_COMMAND)"\"

libgnomeui_deprecated_LIB = $(builddir)/libgnomeui-deprecated/libgnomeui-deprecated.la
libeggsmclient_LIB = $(top_builddir)/libeggsmclient/libeggsmclient.la

bin_PROGRAMS = gnome-search-tool

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_file = gnome-search-tool.schemas.in
schema_DATA = $(schema_in_file:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

gnome_search_tool_SOURCES =     \
	gsearchtool-spinner.c   \
	gsearchtool-spinner.h   \
	gsearchtool-support.c   \
	gsearchtool-support.h   \
	gsearchtool-callbacks.c \
	gsearchtool-callbacks.h \
	gsearchtool.c	        \
	gsearchtool.h

gnome_search_tool_LDADD =            \
	$(GNOME_UTILS_LIBS)          \
	$(GIO_UNIX_LIBS)             \
	$(libgnomeui_deprecated_LIB) \
	$(libeggsmclient_LIB)

man_MANS = gnome-search-tool.1

Utilities_in_files = gnome-search-tool.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(schema_in_file) \
	$(schema_DATA) \
	$(man_MANS)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_srcdir)/gsearchtool/$(schema_DATA) ; \
	fi
else
install-data-local:
endif

CLEANFILES = 				\
	gnome-search-tool.desktop	\
	gnome-search-tool.schemas

disthook:
	cd $(distdir) ; rm -f $(CLEANFILES)
