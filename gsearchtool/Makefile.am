Utilitiesdir = $(datadir)/applications

SUBDIRS = help

INCLUDES = 	\
	$(GNOME_UTILS_CFLAGS)					\
	$(GNOMEVFS_CFLAGS)					\
	$(GNOMEDESKTOP_CFLAGS)					\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"			\
	-DDATADIR=\""$(datadir)"\"

bin_PROGRAMS = gnome-search-tool

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = gnome-search-tool.schemas

gnome_search_tool_SOURCES =	\
	gsearchtool-alert-dialog.c \
	gsearchtool-alert-dialog.h \
	gsearchtool-spinner.c \
	gsearchtool-spinner.h \
	gsearchtool-support.c   \
	gsearchtool-support.h   \
	gsearchtool-callbacks.c \
	gsearchtool-callbacks.h \
	gsearchtool.c	        \
	gsearchtool.h

gnome_search_tool_LDADD =   \
	$(GNOME_UTILS_LIBS) \
	$(GNOMEVFS_LIBS)    \
	$(GNOMEDESKTOP_LIBS)

man_MANS = gnome-search-tool.1

Utilities_in_files = gnome-search-tool.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
	
EXTRA_DIST = \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(schema_DATA) \
	$(man_MANS)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
else
install-data-local:
endif

CLEANFILES = gnome-search-tool.desktop

disthook:
	cd $(distdir) ; rm -f $(CLEANFILES)
