/*   Generate gtt-parsed html output for GTimeTracker - a time tracker
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

#include "config.h"

#ifndef __GTT_PHTML_H__
#define __GTT_PHTML_H__

/* PHTML == parsed html.  These routines will read in specially-marked
 * up gtt-style html, plug in gtt-specific data, and output planin-old
 * html to the indicated stream.
 *
 * By appropriately supplying the stream structure, gtt HTML data
 * can be sent anywhere desired. For example, this could, in theory
 * be used inside a cgi-bin script.  (this is a plannned, multi-user,
 * web-based version that we hope to code up someday).  Currently, 
 * the only user of this interface is GtkHTML
 */

typedef struct gtt_phtml_s GttPhtml;

struct gtt_phtml_s
{
	void (*open_stream) (GttPhtml *);
	void (*write_stream) (GttPhtml *, const char *, size_t len);
	void (*close_stream) (GttPhtml *);
	void (*error) (GttPhtml *, int errcode, const char * msg);
};

/* The gtt_phtml_display() routine will parse the indicated gtt file, 
 * and output standard HTML to the indicated stream
 */
void gtt_phtml_display (GttPhtml *, const char *path_frag, GttProject *prj);

#endif /* __GTT_PHTML_H__ */

