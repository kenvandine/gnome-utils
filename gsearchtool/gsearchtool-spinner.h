/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * GNOME Search Tool
 *
 *  File:  gsearchtool-spinner.h
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Nautilus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 *
 * Ephy port by Marco Pesenti Gritti <marco@it.gnome.org>
 * Gsearchtool port by Dennis Cranston <dennis_cranston@yahoo.com>
 *
 * This is the header file for the throbber on the location bar
 *
 * $Id$
 */

#ifndef GSEARCHTOOL_SPINNER_H
#define GSEARCHTOOL_SPINNER_H

#include <gtk/gtkeventbox.h>

G_BEGIN_DECLS

#define GSEARCH_TYPE_SPINNER	     (gsearch_spinner_get_type ())
#define GSEARCH_SPINNER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSEARCH_TYPE_SPINNER, GsearchSpinner))
#define GSEARCH_SPINNER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSEARCH_TYPE_SPINNER, GsearchSpinnerClass))
#define GSEARCH_IS_SPINNER(o)	     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSEARCH_TYPE_SPINNER))
#define GSEARCH_IS_SPINNER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSEARCH_TYPE_SPINNER))
#define GSEARCH_SPINNER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSEARCH_TYPE_SPINNER, GsearchSpinnerClass))

typedef struct GsearchSpinner GsearchSpinner;
typedef struct GsearchSpinnerClass GsearchSpinnerClass;
typedef struct GsearchSpinnerDetails GsearchSpinnerDetails;

struct GsearchSpinner {
	GtkEventBox parent;

	/*< private >*/
	GsearchSpinnerDetails *details;
};

struct GsearchSpinnerClass {
	GtkEventBoxClass parent_class;
};

GType         gsearch_spinner_get_type       (void);

GtkWidget    *gsearch_spinner_new            (void);

void          gsearch_spinner_start          (GsearchSpinner *throbber);

void          gsearch_spinner_stop           (GsearchSpinner *throbber);

void	      gsearch_spinner_set_small_mode (GsearchSpinner *spinner,
					      gboolean new_mode);

G_END_DECLS

#endif /* GSEARCHTOOL_SPINNER_H */
