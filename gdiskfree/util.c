/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 * 
 * Misc util functions that don't fit anywhere else
 */

#include <gnome.h>

GtkWidget *
get_widget (GtkWidget *widget, const gchar *name)
{
  GtkWidget   *found;

  g_return_val_if_fail (GTK_IS_OBJECT (widget), NULL);
  
  found = NULL;
  if (widget->parent)
    widget = gtk_widget_get_toplevel (widget);
  found = gtk_object_get_data (GTK_OBJECT (widget), name);
  return found;
}
