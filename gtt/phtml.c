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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "gtt.h"
#include "phtml.h"
#include "proj.h"
#include "util.h"


/* ============================================================== */

static void
show_journal (GttPhtml *phtml, GttProject*prj)
{
	GList *node;
	const char *buf;

	buf = _("<table border=1><caption><em>hey hey hey!</em></caption>"
		"<tr><th colspan=4>Memo"
		"<tr><th>&nbsp;<th>Start<th>Stop<th>Elapsed");
	(phtml->write_stream) (phtml, buf, strlen(buf));

	for (node = gtt_project_get_tasks(prj); node; node=node->next)
	{
		char prn[1232], *p;
		int prt_date = 1;
		time_t prev_stop = 0;
		GList *in;
		GttTask *tsk = node->data;
		
		p = prn;
		p = stpcpy (p, "<tr><td colspan=4>"
			"<a href=\"gtt:memo:");
		p += sprintf (p, "%p\">", tsk);

		buf = gtt_task_get_memo(tsk);
		buf = buf? buf : _("(null)");
		p = stpcpy (p, buf);
		p = stpcpy (p, "</a>");

		(phtml->write_stream) (phtml, prn, p-prn);

		
		for (in=gtt_task_get_intervals(tsk); in; in=in->next)
		{
			size_t len;
			GttInterval *ivl = in->data;
			time_t start, stop, elapsed;
			start = gtt_interval_get_start (ivl);
			stop = gtt_interval_get_stop (ivl);
			elapsed = stop - start;
			
			p = prn;
			p = stpcpy (p, 
				"<tr><td>&nbsp;&nbsp;&nbsp;"
				"<td align=right>&nbsp;&nbsp;"
				"<a href=\"gtt:interval:");
			p += sprintf (p, "%p\">", ivl);

			/* print hour only or date too? */
			if (0 != prev_stop) {
				prt_date = is_same_day(start, prev_stop);
			}
			if (prt_date) {
				p = print_date_time (p, 100, start);
			} else {
				p = print_time (p, 100, start);
			}

			/* print hour only or date too? */
			prt_date = is_same_day(start, stop);
			p = stpcpy (p, 
				"</a>&nbsp;&nbsp;"
				"<td>&nbsp;&nbsp;"
				"<a href=\"gtt:interval:");
			p += sprintf (p, "%p\">", ivl);
			if (prt_date) {
				p = print_date_time (p, 70, stop);
			} else {
				p = print_time (p, 70, stop);
			}

			prev_stop = stop;

			p = stpcpy (p, "</a>&nbsp;&nbsp;<td>&nbsp;&nbsp;");
			p = print_hours_elapsed (p, 40, elapsed, TRUE);
			p = stpcpy (p, "&nbsp;&nbsp;");
			len = p - prn;
			(phtml->write_stream) (phtml, prn, len);
		}

	}
	
	buf = "</table>";
	(phtml->write_stream) (phtml, buf, strlen(buf));

}

/* ============================================================== */

static void
dispatch_phtml (GttPhtml *phtml, char *tok, GttProject*prj)
{
	if (!prj) return;
	while (tok)
	{

		if (0 == strncmp (tok, "$project_title", 13))
		{
			const char * str = gtt_project_get_title(prj);
			str = str? str : _("(null)");
			(phtml->write_stream) (phtml, str, strlen(str));
		}
		else
		if (0 == strncmp (tok, "$project_desc", 13))
		{
			const char * str = gtt_project_get_desc(prj);
			str = str? str : _("(null)");
			(phtml->write_stream) (phtml, str, strlen(str));
		}
		else
		if (0 == strncmp (tok, "$journal", 8))
		{
			show_journal (phtml, prj);
		}
		else
		{
			const char * str;
			str = _("unknown token: >>>>");
			(phtml->write_stream) (phtml, str, strlen(str));
			str = tok;
			(phtml->write_stream) (phtml, str, strlen(str));
			str = "<<<<";
			(phtml->write_stream) (phtml, str, strlen(str));
		}

		/* find the next one */
		tok ++;
		tok = strchr (tok, '$');
	}
}

/* ============================================================== */

void
gtt_phtml_display (GttPhtml *phtml, const char *path_fragment)
{
	FILE *ph;
	char * fullpath;

	if (!phtml)
		
	if (!path_fragment)
	{
		(phtml->error) (phtml, 404, NULL);
		return;
	}

	/* XXX hack alert FIXME  need to get the full 
	 * path e.g. /usr/share/gtt, and also need to get i18n path */
	fullpath = g_strconcat ("phtml/C/", path_fragment, 0);
	
	/* try to get the phtml file ... */
	ph = fopen (fullpath, "r");
	if (!ph)
	{
		(phtml->error) (phtml, 404, fullpath);
		g_free (fullpath);
		return;
	}
	g_free (fullpath); fullpath = NULL;

	/* Now open the output stream for writing */
	(phtml->open_stream) (phtml);

	while (!feof (ph))
	{
#define BUFF_SIZE 4000
		size_t nr;
		char *start, *end, *tok;
		char buff[BUFF_SIZE+1];
		nr = fread (buff, 1, BUFF_SIZE, ph);
		if (0 >= nr) break;  /* EOF I presume */
		buff[nr] = 0x0;
		
		start = buff;
		while (start)
		{
			tok = NULL;
			
			/* look for special gtt markup */
			end = strstr (start, "<?gtt");
			if (end)
			{
				nr = end - start;
				*end = 0x0;
				tok = end+5;
				end = strstr (tok, "?>");
				if (end)
				{
					*end = 0;
					end += 2;
				}
			}
			else
			{
				nr = strlen (start);
			}
			
			/* write everything that we got before the markup */
			(phtml->write_stream) (phtml, start, nr);

			/* if there is markup, then dispatch */
			if (tok)
			{
				tok = strchr (tok, '$');
				dispatch_phtml (phtml, tok, cur_proj);
			}
			start = end;
		}
			
	}
	fclose (ph);

	(phtml->close_stream) (phtml);

}

/* ===================== END OF FILE ==============================  */
