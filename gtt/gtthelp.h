/*   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GTT_HELP_H__
#define __GTT_HELP_H__

#include <gnome.h>
#include <gtk-xmhtml/gtk-xmhtml.h>

BEGIN_GNOME_DECLS

#define GTT_HELP(obj) GTK_CHECK_CAST(obj, gtt_help_get_type(), GttHelp)
#define GTT_HELP_CLASS(klass) GTK_CHECK_CAST_CLASS(klass, gtt_help_get_type(), GttHelpClass)
#define GTT_IS_HELP(obj) GTK_CHECK_TYPE(obj, gtt_help_get_type())

typedef struct _GttHelp GttHelp;
typedef struct _GttHelpClass GttHelpClass;

struct _GttHelp {
	GnomeApp parent_object;
	GtkXmHTML *gtk_xmhtml;
	char *home;
	char *html_source;
	char document_path[1024];
};

struct _GttHelpClass {
	GnomeAppClass parent_class;
};

guint gtt_help_get_type(void);
GtkWidget *gtt_help_new(char *title,
			char *filename);	/* filename can contain "#bla" */

void gtt_help_goto(GttHelp *help,
		   char *filename);		/* filename can contain "#bla" */
void gtt_help_on_help(GttHelp *help);

END_GNOME_DECLS


#endif /* __GTT_HELP_H__ */

