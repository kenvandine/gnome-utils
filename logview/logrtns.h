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

#ifndef __LOGRTNS_H__
#define __LOGRTNS_H__

#include "logview.h"

int isLogFile (char *filename, gboolean show_error);
int WasModified (Log *log);
void ParseLine (char *buff, LogLine * line, gboolean no_dates_in_log);
void CloseLog (Log * log);
time_t GetDate (char *line);
Log * OpenLogFile (char *filename);
gchar **ReadLastPage (Log *log);
gchar **ReadNewLines (Log *log);

#endif /* __LOGRTNS_H__ */
