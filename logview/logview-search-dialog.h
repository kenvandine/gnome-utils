/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net> *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LOGVIEW_SEARCH_DIALOG_H__
#define __LOGVIEW_SEARCH_DIALOG_H__

#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkradiobutton.h>

#define LOGVIEW_TYPE_SEARCH_DIALOG		  (logview_search_dialog_get_type ())
#define LOGVIEW_SEARCH_DIALOG(obj)		  (GTK_CHECK_CAST ((obj), LOGVIEW_TYPE_SEARCH_DIALOG, LogviewSearchDialog))
#define LOGVIEW_SEARCH_DIALOG_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_SEARCH_DIALOG, LogviewSearchDialogClass))
#define LOGVIEW_IS_SEARCH_DIALOG(obj)	          (GTK_CHECK_TYPE ((obj), LOGVIEW_TYPE_SEARCH_DIALOG))
#define LOGVIEW_IS_SEARCH_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((obj), LOGVIEW_TYPE_SEARCH_DIALOG))
#define LOGVIEW_SEARCH_DIALOG_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), LOGVIEW_TYPE_SEARCH_DIALOG, LogviewSearchDialogClass))

typedef struct _LogviewSearchDialog LogviewSearchDialog;
typedef struct _LogviewSearchDialogClass LogviewSearchDialogClass;

struct _LogviewSearchDialog {
	GtkDialog parent_instance;

	GtkWidget *entry;
	GtkWidget *search_button;
};

struct _LogviewSearchDialogClass {
	GtkDialogClass parent_class;
};

GType logview_search_dialog_get_type (void);
GtkWidget *logview_search_dialog_new (GtkWindow *parent);

#endif /* __LOGVIEW_SEARCH_DIALOG_H__ */
