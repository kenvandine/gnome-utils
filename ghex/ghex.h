/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* ghex.h - defines GHex ;)

   Copyright (C) 1998, 1999, 2000 Free Software Foundation

   GHex is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   GHex is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GHex; see the file COPYING.
   If not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
*/

#ifndef GHEX_H
#define GHEX_H
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <stdio.h>

#include "hex-document.h"
#include "gtkhex.h"

#define NO_BUFFER_LABEL "No buffer"

#define DEFAULT_FONT    "-adobe-courier-medium-r-normal--12-*-*-*-*-*-*-*"

#define MAX_MAX_UNDO_DEPTH 100000

extern GnomeUIInfo help_menu[], file_menu[], view_menu[], main_menu[], tools_menu[];

#define DATA_TYPE_HEX   0
#define DATA_TYPE_ASCII 1

#define NUM_MDI_MODES 4

#define CHILD_MENU_PATH "_File"
#define CHILD_LIST_PATH "Fi_les/"
#define GROUP_MENU_PATH "_Edit/_Group Data As/"

typedef struct _PropertyUI {
	GnomePropertyBox *pbox;
	GtkRadioButton *mdi_type[NUM_MDI_MODES];
	GtkRadioButton *group_type[3];
	GtkWidget *font_button, *spin;
	GtkWidget *offset_menu, *offset_choice[3];
	GtkWidget *format, *offsets_col;
} PropertyUI;

typedef struct _JumpDialog {
	GtkWidget *window;
	GtkWidget *int_entry;
	GtkWidget *ok, *cancel;
} JumpDialog;

typedef struct _ReplaceDialog {
	GtkWidget *window;
	GtkWidget *f_string, *r_string;
	GtkWidget *replace, *replace_all, *next, *close;
	GtkWidget *type_button[2];
	
	gint search_type;
} ReplaceDialog; 

typedef struct _FindDialog {
	GtkWidget *window;
	GtkWidget *f_string;
	GtkWidget *f_next, *f_prev, *f_close;
	GtkWidget *type_button[2];
	
	gint search_type;
} FindDialog;

typedef struct _Converter {
	GtkWidget *window;
	GtkWidget *entry[4];
	GtkWidget *close;
	GtkWidget *get;

	gulong value;
} Converter;

extern int restarted;
extern const struct poptOption options[];

extern GnomeMDI *mdi;
extern gint mdi_mode;

extern GtkWidget *file_sel;

extern FindDialog *find_dialog;
extern ReplaceDialog *replace_dialog;
extern JumpDialog *jump_dialog;
extern Converter *converter;
extern GtkWidget *char_table;

extern PropertyUI *prefs_ui;

extern GtkCheckMenuItem *save_config_item;

extern GdkFont *def_font;
extern gchar *def_font_name;

extern guint max_undo_depth;
extern gchar *offset_fmt;
extern gboolean show_offsets_column;

extern gint def_group_type;
extern guint group_type[3];
extern gchar *group_type_label[3];

extern guint mdi_type[NUM_MDI_MODES];
extern gchar *mdi_type_label[NUM_MDI_MODES];

extern guint search_type;
extern gchar *search_type_label[2];

extern GSList *cl_files;

FindDialog    *create_find_dialog    (void);
ReplaceDialog *create_replace_dialog (void);
JumpDialog    *create_jump_dialog    (void);
Converter     *create_converter      (void);
GtkWidget     *create_char_table     (void);
PropertyUI    *create_prefs_dialog   (void);

void create_dialog_title   (GtkWidget *, gchar *);

gint ask_user              (GnomeMessageBox *);
GtkWidget *create_button   (GtkWidget *, const gchar *, gchar *);

/* config stuff */
void save_configuration    (void);
void load_configuration    (void);

/* hide the widget passed as user data after delete event or click on a cancel button */
gint delete_event_cb(GtkWidget *, gpointer, GtkWidget *);
void cancel_cb(GtkWidget *, GtkWidget *);

/* session managment */
int save_state      (GnomeClient        *client,
                     gint                phase,
                     GnomeRestartStyle   save_style,
                     gint                shutdown,
                     GnomeInteractStyle  interact_style,
                     gint                fast,
                     gpointer            client_data);

gint client_die     (GnomeClient *client, gpointer client_data);


#endif
