
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

#ifdef FIXME
int ReadNPagesUp (Log * lg, Page * pg, int n);
int ReadNPagesDown (Log * lg, Page * pg, int n);
int ReadPageUp (Log * lg, Page * pg);
int ReadPageDown (Log *log, LogLine ***inp_mon_lines, gboolean exec_actions);
void MoveToMark ();
#endif

int isLogFile (char *filename, gboolean show_error);
int isSameDay (time_t day1, time_t day2);
int WasModified (Log *log);
void ParseLine (char *buff, LogLine * line);
void ReadLogStats (Log * log, gchar **buffer_lines);
void UpdateLogStats( Log *log );
void CloseLog (Log * log);
time_t GetDate (char *line);
Log * OpenLogFile (char *filename);
gchar **ReadLastPage (Log *log);
gchar **ReadNewLines (Log *log);

#endif /* __LOGRTNS_H__ */
