/*   ctree implementation of main window for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001 Linas Vepstas
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

#ifndef __GTT_CTREE_H__
#define __GTT_CTREE_H__

#include <gnome.h>
#include "proj.h"

extern int clist_header_width_set;

GtkWidget *create_ctree(void);
void setup_ctree(void);
void ctree_add(GttProject *p, GtkCTreeNode *parent);
void ctree_insert_before(GttProject *p, GttProject *insert_before_me);
void ctree_insert_after(GttProject *p, GttProject *insert_after_me);
void ctree_remove(GttProject *p);
void ctree_update_label(GttProject *p);
void ctree_update_title(GttProject *p);
void ctree_update_desc(GttProject *p);


#endif /* __GTT_CTREE_H__ */
