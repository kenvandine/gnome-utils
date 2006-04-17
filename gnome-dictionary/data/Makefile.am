NULL =

man_MANS = gnome-dictionary.1

appicondir = $(datadir)/pixmaps
appicon_DATA = gnome-dictionary.png

gdictappdir = $(datadir)/applications
gdictapp_in_files = gnome-dictionary.desktop.in
gdictapp_DATA = $(gdictapp_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# dictionary sources definitions
dictsourcedir = $(datadir)/gdict-1.0/sources
dictsource_in_files = \
	default.desktop.in \
	spanish.desktop.in \
	$(NULL)
dictsource_DATA = $(dictsource_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

gladedir        = $(datadir)/gnome-dictionary
glade_DATA      = gnome-dictionary-preferences.glade

uidir           = $(datadir)/gnome-dictionary
ui_DATA         = gnome-dictionary-ui.xml

# applet menu definition
appletuidir	= $(datadir)/gnome-2.0/ui
appletui_DATA	= GNOME_DictionaryApplet.xml

serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_DictionaryApplet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

@INTLTOOL_SERVER_RULE@

schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_file = gnome-dictionary.schemas.in
schema_DATA = $(schemas_in_file:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	GNOME_DictionaryApplet.server.in.in 	\
	$(appicon_DATA) 			\
	$(dictsource_in_files)			\
	$(gdictapp_in_files) 			\
	$(gdictapp_DATA) 			\
	$(man_MANS) 				\
	$(schemas_in_file) 			\
	$(schema_DATA) 				\
	$(glade_DATA) 				\
	$(ui_DATA)				\
	$(appletui_DATA)			\
	$(NULL)

CLEANFILES = 				\
	$(dictsource_DATA)		\
	$(server_in_files) 		\
	$(server_DATA) 			\
	$(gdictapp_DATA) 		\
	gnome-dictionary.schemas	\
	$(NULL)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA) ; \
	fi
else
install-data-local:
endif
