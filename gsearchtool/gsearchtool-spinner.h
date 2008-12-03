/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * GNOME Search Tool
 *
 *  File:  gsearchtool-spinner.h
 *
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author: Andy Hertzfeld <andy@eazel.com>
 *
 *  Ephy port by Marco Pesenti Gritti <marco@it.gnome.org>
 *  Gsearchtool port by Dennis Cranston <dennis_cranston@yahoo.com>
 *
 */

#ifndef _GSEARCHTOOL_SPINNER_H
#define _GSEARCHTOOL_SPINNER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSEARCH_TYPE_SPINNER            (gsearch_spinner_get_type ())
#define GSEARCH_SPINNER(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), GSEARCH_TYPE_SPINNER, GSearchSpinner))
#define GSEARCH_SPINNER_CLASS(k)        (G_TYPE_CHECK_CLASS_CAST((k), GSEARCH_TYPE_SPINNER, GSearchSpinnerClass))
#define GSEARCH_IS_SPINNER(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSEARCH_TYPE_SPINNER))
#define GSEARCH_IS_SPINNER_CLASS(k)     (G_TYPE_CHECK_CLASS_TYPE ((k), GSEARCH_TYPE_SPINNER))
#define GSEARCH_SPINNER_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS ((o), GSEARCH_TYPE_SPINNER, GSearchSpinnerClass))

typedef struct _GSearchSpinner          GSearchSpinner;
typedef struct _GSearchSpinnerClass     GSearchSpinnerClass;
typedef struct _GSearchSpinnerDetails   GSearchSpinnerDetails;

struct _GSearchSpinner {
	GtkEventBox parent;

	/*< private >*/
	GSearchSpinnerDetails * details;
};

struct _GSearchSpinnerClass {
	GtkEventBoxClass parent_class;
};

GType
gsearch_spinner_get_type (void);

GtkWidget *
gsearch_spinner_new (void);

void
gsearch_spinner_start (GSearchSpinner * throbber);

void
gsearch_spinner_stop (GSearchSpinner * throbber);

void
gsearch_spinner_set_size (GSearchSpinner * spinner,
                          GtkIconSize size);

G_END_DECLS

#endif /* _GSEARCHTOOL_SPINNER_H */
