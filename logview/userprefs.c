/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */


#include <config.h>
#include <gnome.h>

#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "logview.h"

#define USERPREFSDIALOG_WIDTH            500
#define USERPREFSDIALOG_HEIGHT           300

/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

extern ConfigData *cfg;
extern UserPrefsStruct *user_prefs;
extern GtkWidget *app;

/* Struct to hold function data */
typedef struct 
{
   GtkWidget *userprefs_property;
   GtkWidget *frame;
   GtkWidget *table;
   GtkWidget *columns_tab_label;
   GtkWidget *hostname_width_label;
   GtkWidget *hostname_width_spinner;
   GtkAdjustment *hostname_width_spinner_adjust;
} UserPrefsDialogStruct;

UserPrefsDialogStruct upds;
