/*
 *  Gnome Character Map
 *  main.c: what do you think this file is?
 *
 *  Copyright (C) 2000 Hongli Lai
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MAIN_C_
#define _MAIN_C_

#include <config.h>
#include <gnome.h>
#include "interface.h"

extern void install_notifiers();

static int save_session(GnomeClient        *client,
			gint                phase,
			GnomeRestartStyle   save_style,
			gint                shutdown,
			GnomeInteractStyle  interact_style,
			gint                fast,
			gpointer            client_data) {

    gchar *argv[]= { NULL };

    argv[0] = (char*) client_data;
    gnome_client_set_clone_command (client, 1, argv);
    gnome_client_set_restart_command (client, 1, argv);

    return TRUE;
}

static gint client_die(GnomeClient *client, gpointer client_data)
{
        exit (0);

        return FALSE;
}


int
main (int argc, char *argv[])
{
    GnomeClient *client;
    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    gnome_program_init ("gnome-character-map",
                        VERSION,
                        LIBGNOMEUI_MODULE,
                        argc, argv,
			GNOME_PARAM_APP_DATADIR,DATADIR,NULL);


    client = gnome_master_client ();
    g_signal_connect (G_OBJECT (client), "save_yourself",
		      G_CALLBACK (save_session), (gpointer) argv[0]);
    g_signal_connect (G_OBJECT (client), "die",
		      G_CALLBACK (client_die), NULL);


    main_app_new ();
    install_notifiers();
    gtk_widget_show (GTK_WIDGET (mainapp->window));
    init_prefs();
    gtk_main ();
    return 0;
}

#endif /* _MAIN_C_ */
