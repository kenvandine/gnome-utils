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

typedef enum {
	NUL=0,
	MEMO = 1,
	TASK_TIME,
	BILLABLE,
	BILLRATE,
	VALUE,
	BILLABLE_VALUE,
	START_DATIME =100,
	STOP_DATIME,
	ELAPSED,

} TableCol;

#define NCOL 30

struct gtt_phtml_s
{
	/* stream interface for writing */
	void (*open_stream) (GttPhtml *, gpointer);
	void (*write_stream) (GttPhtml *, const char *, size_t len, gpointer);
	void (*close_stream) (GttPhtml *, gpointer);
	void (*error) (GttPhtml *, int errcode, const char * msg, gpointer);
	gpointer user_data;

	/* parseing state */
	int ntask_cols;
	TableCol task_cols[NCOL];

	int ninvl_cols;
	TableCol invl_cols[NCOL];
};

/* ============================================================== */

static void
show_table (GttPhtml *phtml, GttProject *prj, int show_links, int invoice)
{
	int i;
	GList *node;
	char *p;
	char buff[2000];

	p = buff;
	p = stpcpy (p, "<table border=1>");

	/* write out the table header */
	if (0 < phtml->ntask_cols)
	{
		p = stpcpy (p, "<tr>");
	}
	for (i=0; i<phtml->ntask_cols; i++)
	{
		switch (phtml->task_cols[i]) 
		{
			case MEMO:
			{
				int mcols;
				mcols = phtml->ninvl_cols - phtml->ntask_cols;
				if (0 >= mcols) mcols = 1; 
				p += sprintf (p, "<th colspan=%d>", mcols);
				p = stpcpy (p, _("Memo"));
				break;
			}
			case TASK_TIME:
				p = stpcpy (p, "<th>");
				p = stpcpy (p, _("Task Time"));
				break;
			case BILLABLE:
				p = stpcpy (p, "<th>");
				p = stpcpy (p, _("Billable"));
				break;
			case BILLRATE:
				p = stpcpy (p, "<th>");
				p = stpcpy (p, _("Bill Rate"));
				break;
			case VALUE:
				p = stpcpy (p, "<th>");
				p = stpcpy (p, _("Value"));
				break;
			case BILLABLE_VALUE:
				p = stpcpy (p, "<th>");
				p = stpcpy (p, _("Billable Value"));
				break;
			default:
				p = stpcpy (p, "<th>");
				p = stpcpy (p, _("Error - Unknown"));
		}
	}

	if (0 < phtml->ninvl_cols)
	{
		p = stpcpy (p, "<tr>");
	}
	for (i=0; i<phtml->ninvl_cols; i++)
	{
		p = stpcpy (p, "<th>");
		switch (phtml->invl_cols[i]) 
		{
			case START_DATIME:
				p = stpcpy (p, _("Start"));
				break;
			case STOP_DATIME:
				p = stpcpy (p, _("Stop"));
				break;
			case ELAPSED:
				p = stpcpy (p, _("Elapsed"));
				break;
			default:
				p = stpcpy (p, _("Error - Unknown"));
		}
	}


	(phtml->write_stream) (phtml, buff, p-buff, phtml->user_data);

	for (node = gtt_project_get_tasks(prj); node; node=node->next)
	{
		GttBillable billable;
		GttBillRate billrate;
		const char *pp;
		time_t prev_stop = 0;
		GList *in;
		GttTask *tsk = node->data;
		int task_secs;
		double hours, value=0.0, billable_value=0.0;
		
		/* set up data */
		billable = gtt_task_get_billable (tsk);
		billrate = gtt_task_get_billrate (tsk);

		/* if we are in invoice mode, then skip anything not billable */
		if (invoice)
		{
			if ((GTT_HOLD == billable) || 
			    (GTT_NOT_BILLABLE == billable)) continue;
		}

		task_secs = gtt_task_get_secs_ever(tsk);
		hours = task_secs;
		hours /= 3600;
		switch (billrate)
		{
			case GTT_REGULAR:
				value = hours * gtt_project_get_billrate (prj);
				break;
			case GTT_OVERTIME:
				value = hours * gtt_project_get_overtime_rate (prj);
				break;
			case GTT_OVEROVER:
				value = hours * gtt_project_get_overover_rate (prj);
				break;
			case GTT_FLAT_FEE:
				value = gtt_project_get_flat_fee (prj);
				break;
		}

		switch (billable)
		{
			case GTT_HOLD:
				billable_value = 0.0;
				break;
			case GTT_BILLABLE:
				billable_value = value;
				break;
			case GTT_NOT_BILLABLE:
				billable_value = 0.0;
				break;
			case GTT_NO_CHARGE:
				billable_value = 0.0;
				break;
		}

		p = buff;

		/* write the task data */
		if (0 < phtml->ntask_cols)
		{
			p = stpcpy (p, "<tr>");
		}
		for (i=0; i<phtml->ntask_cols; i++)
		{
			switch (phtml->task_cols[i]) 
			{
				case MEMO:
				{
					int mcols;
					mcols = phtml->ninvl_cols - phtml->ntask_cols;
					if (0 >= mcols) mcols = 1; 
					p += sprintf (p, "<td colspan=%d>", mcols);
					if (show_links) 
					{
						p = stpcpy (p, "<a href=\"gtt:task:");
						p += sprintf (p, "%p\">", tsk);
					}
					pp = gtt_task_get_memo(tsk);
					if (!pp || !pp[0]) pp = _("(empty)");
					p = stpcpy (p, pp);
					if (show_links) p = stpcpy (p, "</a>");
					break;
				}

				case TASK_TIME:
					p = stpcpy (p, "<td align=right>");
					p = print_hours_elapsed (p, 40, task_secs, TRUE);
					break;

				case BILLABLE:
					p = stpcpy (p, "<td>");
					switch (billable)
					{
						case GTT_HOLD:
							p = stpcpy (p, _("Hold"));
							break;
						case GTT_BILLABLE:
							p = stpcpy (p, _("Billable"));
							break;
						case GTT_NOT_BILLABLE:
							p = stpcpy (p, _("Not Billable"));
							break;
						case GTT_NO_CHARGE:
							p = stpcpy (p, _("No Charge"));
							break;
					}
					break;

				case BILLRATE:
					p = stpcpy (p, "<td>");
					switch (billrate)
					{
						case GTT_REGULAR:
							p = stpcpy (p, _("Regular"));
							break;
						case GTT_OVERTIME:
							p = stpcpy (p, _("Overtime"));
							break;
						case GTT_OVEROVER:
							p = stpcpy (p, _("Double Overtime"));
							break;
						case GTT_FLAT_FEE:
							p = stpcpy (p, _("Flat Fee"));
							break;
					}
					break;

				case VALUE:
					p = stpcpy (p, "<td align=right>");
					
					/* hack alert should use i18n currency/monetary printing */
					p += sprintf (p, "$%.2f", value+0.0049);
					break;

				case BILLABLE_VALUE:
					p = stpcpy (p, "<td align=right>");
					/* hack alert should use i18n currency/monetary printing */
					p += sprintf (p, "$%.2f", billable_value+0.0049);
					break;

				default:
					p = stpcpy (p, "<th>");
					p = stpcpy (p, _("Error - Unknown"));
			}
		}

		(phtml->write_stream) (phtml, buff, p-buff, phtml->user_data);
		
		/* write out intervals */
		for (in=gtt_task_get_intervals(tsk); in; in=in->next)
		{
			size_t len;
			GttInterval *ivl = in->data;
			time_t start, stop, elapsed;
			int prt_start_date = 1;
			int prt_stop_date = 1;

			/* data setup */
			start = gtt_interval_get_start (ivl);
			stop = gtt_interval_get_stop (ivl);
			elapsed = stop - start;

			/* print hour only or date too? */
			prt_stop_date = is_same_day(start, stop);
			if (0 != prev_stop) {
				prt_start_date = is_same_day(start, prev_stop);
			}
			prev_stop = stop;


			/* -------------------------- */
			p = buff;
			p = stpcpy (p, "<tr>");
			for (i=0; i<phtml->ninvl_cols; i++)
			{

				switch (phtml->invl_cols[i]) 
				{
	case START_DATIME:
	{
		p = stpcpy (p, "<td align=right>&nbsp;&nbsp;");
		if (show_links)
		{
			p += sprintf (p, "<a href=\"gtt:interval:%p\">", ivl);
		}
		if (prt_start_date) {
			p = print_date_time (p, 100, start);
		} else {
			p = print_time (p, 100, start);
		}
		if (show_links) p = stpcpy (p, "</a>");
		p = stpcpy (p, "&nbsp;&nbsp;");
		break;
	}
	case STOP_DATIME:
	{
		p = stpcpy (p, "<td align=right>&nbsp;&nbsp;");
		if (show_links)
		{
			p += sprintf (p, "<a href=\"gtt:interval:%p\">", ivl);
		}
		if (prt_stop_date) {
			p = print_date_time (p, 100, stop);
		} else {
			p = print_time (p, 100, stop);
		}
		if (show_links) p = stpcpy (p, "</a>");
		p = stpcpy (p, "&nbsp;&nbsp;");
		break;
	}
	case ELAPSED:
	{
		p = stpcpy (p, "<td>&nbsp;&nbsp;");
		p = print_hours_elapsed (p, 40, elapsed, TRUE);
		p = stpcpy (p, "&nbsp;&nbsp;");
		break;
	}
	default:
		p = stpcpy (p, "<td>");
				}
			}

			len = p - buff;
			(phtml->write_stream) (phtml, buff, len, phtml->user_data);
		}

	}
	
	p = "</table>";
	(phtml->write_stream) (phtml, p, strlen(p), phtml->user_data);

}

/* ============================================================== */
/* a simpler, hard-coded version of show_table */

static void
show_journal (GttPhtml *phtml, GttProject*prj)
{
	GList *node;
	char *p;
	char prn[2000];

	p = prn;
	p += sprintf (p, "<table border=1><tr><th colspan=4>%s\n"
		"<tr><th>&nbsp;<th>%s<th>%s<th>%s",
		_("Memo"), _("Start"), _("Stop"), _("Elapsed"));

	(phtml->write_stream) (phtml, prn, p-prn, phtml->user_data);

	for (node = gtt_project_get_tasks(prj); node; node=node->next)
	{
		const char *pp;
		int prt_date = 1;
		time_t prev_stop = 0;
		GList *in;
		GttTask *tsk = node->data;
		
		p = prn;
		p = stpcpy (p, "<tr><td colspan=4>"
			"<a href=\"gtt:task:");
		p += sprintf (p, "%p\">", tsk);

		pp = gtt_task_get_memo(tsk);
		if (!pp || !pp[0]) pp = _("(empty)");
		p = stpcpy (p, pp);
		p = stpcpy (p, "</a>");

		(phtml->write_stream) (phtml, prn, p-prn, phtml->user_data);

		
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
			(phtml->write_stream) (phtml, prn, len, phtml->user_data);
		}

	}
	
	p = "</table>";
	(phtml->write_stream) (phtml, p, strlen(p), phtml->user_data);

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
			if (!str || !str[0]) str = _("(empty)");
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
		}
		else
		if (0 == strncmp (tok, "$project_desc", 13))
		{
			const char * str = gtt_project_get_desc(prj);
			str = str? str : "";
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
		}
		else
		if (0 == strncmp (tok, "$journal", 8))
		{
			show_journal (phtml, prj);
		}
		else
		if (0 == strncmp (tok, "$table", 6))
		{
			show_table (phtml, prj, 1, 0);
			phtml->ntask_cols = 0;
			phtml->ninvl_cols = 0;
		}
		else
		if (0 == strncmp (tok, "$invoice", 8))
		{
			show_table (phtml, prj, 1, 1);
			phtml->ntask_cols = 0;
			phtml->ninvl_cols = 0;
		}
		else
		if (0 == strncmp (tok, "$start_datime", 13))
		{
			phtml->invl_cols[phtml->ninvl_cols] = START_DATIME;
			if (NCOL-1 > phtml->ninvl_cols) phtml->ninvl_cols ++;
		}
		else
		if (0 == strncmp (tok, "$stop_datime", 12))
		{
			phtml->invl_cols[phtml->ninvl_cols] = STOP_DATIME;
			if (NCOL-1 > phtml->ninvl_cols) phtml->ninvl_cols ++;
		}
		else
		if (0 == strncmp (tok, "$elapsed", 8))
		{
			phtml->invl_cols[phtml->ninvl_cols] = ELAPSED;
			if (NCOL-1 > phtml->ninvl_cols) phtml->ninvl_cols ++;
		}
		else
		if (0 == strncmp (tok, "$memo", 5))
		{
			phtml->task_cols[phtml->ntask_cols] = MEMO;
			if (NCOL-1 > phtml->ntask_cols) phtml->ntask_cols ++;
		}
		else
		if (0 == strncmp (tok, "$task_time", 10))
		{
			phtml->task_cols[phtml->ntask_cols] = TASK_TIME;
			if (NCOL-1 > phtml->ntask_cols) phtml->ntask_cols ++;
		}
		else
		if (0 == strncmp (tok, "$billable", 9))
		{
			phtml->task_cols[phtml->ntask_cols] = BILLABLE;
			if (NCOL-1 > phtml->ntask_cols) phtml->ntask_cols ++;
		}
		else
		if (0 == strncmp (tok, "$billrate", 9))
		{
			phtml->task_cols[phtml->ntask_cols] = BILLRATE;
			if (NCOL-1 > phtml->ntask_cols) phtml->ntask_cols ++;
		}
		else
		if (0 == strncmp (tok, "$value", 6))
		{
			phtml->task_cols[phtml->ntask_cols] = VALUE;
			if (NCOL-1 > phtml->ntask_cols) phtml->ntask_cols ++;
		}
		else
		if (0 == strncmp (tok, "$bill_value", 11))
		{
			phtml->task_cols[phtml->ntask_cols] = BILLABLE_VALUE;
			if (NCOL-1 > phtml->ntask_cols) phtml->ntask_cols ++;
		}
		else
		{
			const char * str;
			str = _("unknown token: >>>>");
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
			str = tok;
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
			str = "<<<<";
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
		}

		/* find the next one */
		tok ++;
		tok = strchr (tok, '$');
	}
}

/* ============================================================== */

void
gtt_phtml_display (GttPhtml *phtml, const char *filepath,
                   GttProject *prj)
{
	FILE *ph;

	if (!phtml) return;

	/* reset the parser */
	phtml->ninvl_cols = 0;
	phtml->ntask_cols = 0;
		
	if (!filepath)
	{
		(phtml->error) (phtml, 404, NULL, phtml->user_data);
		return;
	}

	/* try to get the phtml file ... */
	ph = fopen (filepath, "r");
	if (!ph)
	{
		(phtml->error) (phtml, 404, filepath, phtml->user_data);
		return;
	}

	/* Now open the output stream for writing */
	(phtml->open_stream) (phtml, phtml->user_data);

	while (!feof (ph))
	{
#define BUFF_SIZE 4000
		size_t nr;
		char *start, *end, *tok, *comstart, *comend;
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

			/* look for comments, and blow past them. */
			comstart = strstr (start, "<!--");
			if (comstart && comstart < end)
			{
				comend = strstr (comstart, "-->");
				if (comend)
				{
					nr = comend - start;
					end = comend;
				}
				else
				{
					nr = strlen (start);
					end = NULL;
				}
				/* write everything that we got before the markup */
				(phtml->write_stream) (phtml, start, nr, phtml->user_data);
				start = end;
				continue;
			}

			/* look for  termination of gtt markup */
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
			(phtml->write_stream) (phtml, start, nr, phtml->user_data);

			/* if there is markup, then dispatch */
			if (tok)
			{
				tok = strchr (tok, '$');
				dispatch_phtml (phtml, tok, prj);
			}
			start = end;
		}
			
	}
	fclose (ph);

	(phtml->close_stream) (phtml, phtml->user_data);

}

/* ============================================================== */

GttPhtml *
gtt_phtml_new (void)
{
	GttPhtml *p;

	p = g_new0 (GttPhtml, 1);
	p->ntask_cols = 0;
	p->ninvl_cols = 0;

	return p;
}

void 
gtt_phtml_destroy (GttPhtml *p)
{
	if (!p) return;
	g_free (p);
}

void gtt_phtml_set_stream (GttPhtml *p, gpointer ud,
                                        GttPhtmlOpenStream op, 
                                        GttPhtmlWriteStream wr,
                                        GttPhtmlCloseStream cl, 
                                        GttPhtmlError er)
{
	if (!p) return;
	p->user_data = ud;
	p->open_stream = op;
	p->write_stream = wr;
	p->close_stream = cl;
	p->error = er;
}

/* ===================== END OF FILE ==============================  */
