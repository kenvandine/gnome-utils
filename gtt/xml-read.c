/*   XML I/O routines for GTimeTracker
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <gnome-xml/parser.h>
#include <stdio.h>
#include <stdlib.h>

#include "err-throw.h"
#include "gtt.h"
#include "xml-gtt.h"


/* Note: most of this code is a tediously boring cut-n-paste
 * of the same thing over & over again, and could//should be
 * auto-generated.  Diatribe: If the creators and true
 * beleivers of XML knew some scheme/lisp, or of IDL's, of
 * Corba or RPC fame, or even had an inkling of what 'object
 * introspection' was, then DTD's wouldn't be the abortion that
 * they are, and XML wouldn't be so sucky to begin with ...
 * Alas ...
 */

/* =========================================================== */


#define GET_TEXT(node)  ({					\
	char * sstr = NULL;					\
	xmlNodePtr text;					\
	text = node->childs;					\
	if (!text) { 						\
		gtt_err_set_code (GTT_FILE_CORRUPT);		\
	}							\
	else if (strcmp ("text", text->name)) {			\
                gtt_err_set_code (GTT_FILE_CORRUPT);		\
	} 							\
	else sstr = text->content;				\
	sstr;							\
})

/* =========================================================== */

static GttInterval *
parse_interval (xmlNodePtr interval)
{
	char * str;
	xmlNodePtr node;
	GttInterval *ivl = NULL;

	if (!interval) { gtt_err_set_code (GTT_FILE_CORRUPT); return ivl; }

	if (strcmp ("interval", interval->name)) {
		gtt_err_set_code (GTT_FILE_CORRUPT); return ivl; }

	ivl = gtt_interval_new ();
	for (node=interval->childs; node; node=node->next)
	{
		if (0 == strcmp ("start", node->name))
		{
			time_t thyme;
			str = GET_TEXT (node);
			thyme = atol (str);
			gtt_interval_set_start (ivl, thyme);
		} 
		else
		if (0 == strcmp ("stop", node->name))
		{
			time_t thyme;
			str = GET_TEXT (node);
			thyme = atol (str);
			gtt_interval_set_stop (ivl, thyme);
		} 
		else
		if (0 == strcmp ("running", node->name))
		{
			gboolean ru;
			str = GET_TEXT (node);
			ru = atol (str);
			gtt_interval_set_running (ivl, ru);
		} 
		else
		{ 
			gtt_err_set_code (GTT_FILE_CORRUPT);
		}
	}
	return ivl;
}

/* =========================================================== */

static GttTask *
parse_task (xmlNodePtr task)
{
	char * str;
	xmlNodePtr node;
	GttTask *tsk = NULL;

	if (!task) { gtt_err_set_code (GTT_FILE_CORRUPT); return tsk; }

	if (strcmp ("task", task->name)) {
		gtt_err_set_code (GTT_FILE_CORRUPT); return tsk; }

	tsk = gtt_task_new ();
	for (node=task->childs; node; node=node->next)
	{
		if (0 == strcmp ("memo", node->name))
		{
			str = GET_TEXT (node);
			gtt_task_set_memo (tsk, str);
		} 
		else
		if (0 == strcmp ("notes", node->name))
		{
			str = GET_TEXT (node);
			gtt_task_set_notes (tsk, str);
		} 
		else
		if (0 == strcmp ("billable", node->name))
		{
			GttBillable billable = GTT_HOLD;
			str = GET_TEXT (node);
			if (!strcmp ("HOLD", str)) billable=GTT_HOLD;
			else if (!strcmp ("BILLABLE", str)) billable=GTT_BILLABLE;
			else if (!strcmp ("NOT_BILLABLE", str)) billable=GTT_NOT_BILLABLE;
			else if (!strcmp ("NO_CHARGE", str)) billable=GTT_NO_CHARGE;
			gtt_task_set_billable (tsk, billable);
		} 
		else
		if (0 == strcmp ("billrate", node->name))
		{
			GttBillRate billrate = GTT_REGULAR;
			str = GET_TEXT (node);
			if (!strcmp ("REGULAR", str)) billrate=GTT_REGULAR;
			else if (!strcmp ("OVERTIME", str)) billrate=GTT_OVERTIME;
			else if (!strcmp ("OVEROVER", str)) billrate=GTT_OVEROVER;
			else if (!strcmp ("FLAT_FEE", str)) billrate=GTT_FLAT_FEE;
			gtt_task_set_billrate (tsk, billrate);
		} 
		else
		if (0 == strcmp ("interval-list", node->name))
		{
			xmlNodePtr tn;
			for (tn=node->childs; tn; tn=tn->next)
			{
				GttInterval *ival;
				ival = parse_interval (tn);
				gtt_task_append_interval (tsk, ival);
			}
		} 
		else
		{ 
			gtt_err_set_code (GTT_FILE_CORRUPT);
		}
	}
	return tsk;
}

/* =========================================================== */

static GttProject *
parse_project (xmlNodePtr project)
{
	char * str;
	xmlNodePtr node;
	GttProject *prj = NULL;

	if (!project) { gtt_err_set_code (GTT_FILE_CORRUPT); return prj; }

	if (strcmp ("project", project->name)) {
		gtt_err_set_code (GTT_FILE_CORRUPT); return prj; }

	prj = gtt_project_new ();
	for (node=project->childs; node; node=node->next)
	{
		if (0 == strcmp ("title", node->name))
		{
			str = GET_TEXT (node);
			gtt_project_set_title (prj, str);
		} 
		else
		if (0 == strcmp ("desc", node->name))
		{
			str = GET_TEXT (node);
			gtt_project_set_desc (prj, str);
		} 
		else
		if (0 == strcmp ("notes", node->name))
		{
			str = GET_TEXT (node);
			gtt_project_set_notes (prj, str);
		} 
		else
		if (0 == strcmp ("custid", node->name))
		{
			str = GET_TEXT (node);
			gtt_project_set_custid (prj, str);
		} 
		else
		if (0 == strcmp ("billrate", node->name))
		{
			double rate;
			str = GET_TEXT (node);
			rate = atof (str);
			gtt_project_set_billrate (prj, rate);
		} 
		else
		if (0 == strcmp ("overtime_rate", node->name))
		{
			double rate;
			str = GET_TEXT (node);
			rate = atof (str);
			gtt_project_set_overtime_rate (prj, rate);
		} 
		else
		if (0 == strcmp ("overover_rate", node->name))
		{
			double rate;
			str = GET_TEXT (node);
			rate = atof (str);
			gtt_project_set_overover_rate (prj, rate);
		} 
		else
		if (0 == strcmp ("flat_fee", node->name))
		{
			double rate;
			str = GET_TEXT (node);
			rate = atof (str);
			gtt_project_set_flat_fee (prj, rate);
		} 
		else
		if (0 == strcmp ("min_interval", node->name))
		{
			int ivl;
			str = GET_TEXT (node);
			ivl = atoi (str);
			gtt_project_set_min_interval (prj, ivl);
		} 
		else
		if (0 == strcmp ("auto_merge_interval", node->name))
		{
			int ivl;
			str = GET_TEXT (node);
			ivl = atoi (str);
			gtt_project_set_auto_merge_interval (prj, ivl);
		} 
		else
		if (0 == strcmp ("id", node->name))
		{
			int id;
			str = GET_TEXT (node);
			id = atoi (str);
			gtt_project_set_id (prj, id);
		} 
		else
		if (0 == strcmp ("task-list", node->name))
		{
			xmlNodePtr tn;
			for (tn=node->childs; tn; tn=tn->next)
			{
				GttTask *tsk;
				tsk = parse_task (tn);
				gtt_project_add_task (prj, tsk);
			}
		} 
		else
		if (0 == strcmp ("project-list", node->name))
		{
			xmlNodePtr tn;
			for (tn=node->childs; tn; tn=tn->next)
			{
				GttProject *child;
				child = parse_project (tn);
				gtt_project_append_project (prj, child);
			}
		} 
		else
		{ 
			g_warning ("unexpected node %s\n", node->name);
			gtt_err_set_code (GTT_FILE_CORRUPT);
		}
	}
	return prj;
}

/* =========================================================== */

void
gtt_xml_read_file (const char * filename)
{
	xmlDocPtr doc;
	xmlNodePtr root, project_list, project;

	doc = xmlParseFile (filename);

	if (!doc) { gtt_err_set_code (GTT_CANT_OPEN_FILE); return; }
	root = doc->root;
	if (strcmp ("gtt", root->name)) {
		gtt_err_set_code (GTT_NOT_A_GTT_FILE); return; }

	project_list = root->childs;

	/* If no children, then no projects -- a clean slate */
	if (!project_list) return;

	if (strcmp ("project-list", project_list->name)) {
		gtt_err_set_code (GTT_FILE_CORRUPT); return; }

	for (project=project_list->childs; project; project=project->next)
	{
		GttProject *prj;
		prj = parse_project (project);
		gtt_project_list_append (prj);
	}

	/* recompute the cached counters */
	gtt_project_list_compute_secs ();
}

/* ====================== END OF FILE =============== */
