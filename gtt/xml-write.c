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


#include <gnome-xml/tree.h>
#include <stdio.h>

#include "err-throw.h"
#include "gtt.h"
#include "xml-gtt.h"

static xmlNodePtr gtt_project_list_to_dom_tree (GList *list);

/* Note: most of this code is a tediously boring cut-n-paste
 * of the same thing over & over again, and could//should be 
 * auto-generated.  Diatribe: If the creators and true 
 * beleivers of XML knew some scheme/lisp, or of IDL's, of
 * Corba or RPC fame, or even had an inkling of what 'object 
 * introspection' was, then DTD's wouldn't be the abortion that
 * they are, and XML wouldn't be so sucky to begin with ...
 * Alas ...
 */

/* ======================================================= */

/* convert one interval to a dom tree */

static xmlNodePtr
gtt_xml_interval_to_dom_tree (GttInterval *ivl)
{
	gboolean running;
	char buff[80];
	xmlNodePtr node, topnode;

	if (!ivl) return NULL;

	topnode = xmlNewNode (NULL, "gtt:interval");

	g_snprintf (buff, sizeof(buff), "%ld", gtt_interval_get_start (ivl));
	node = xmlNewNode (NULL, "start");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	g_snprintf (buff, sizeof(buff), "%ld", gtt_interval_get_stop (ivl));
	node = xmlNewNode (NULL, "stop");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	running = gtt_interval_get_running (ivl);
	node = xmlNewNode (NULL, "running");
        if (running) {
		xmlNodeAddContent(node, "T");
	} else {
		xmlNodeAddContent(node, "F");
	}
	xmlAddChild (topnode, node);

	return topnode;
}

/* convert a list of gtt tasks into a DOM tree */
static xmlNodePtr
gtt_interval_list_to_dom_tree (GList *list)
{
	GList *p;
	xmlNodePtr topnode;
	xmlNodePtr node;

	if (!list) return NULL;

	topnode = xmlNewNode (NULL, "gtt:interval-list");

	for (p=list; p; p=p->next)
	{
		GttInterval *ivl = p->data;
		node = gtt_xml_interval_to_dom_tree (ivl);
		xmlAddChild (topnode, node);
	}

   	return topnode;
}

/* ======================================================= */

/* convert one task to a dom tree */

static xmlNodePtr
gtt_xml_task_to_dom_tree (GttTask *task)
{
	GList *p;
	const char * str;
	xmlNodePtr node, topnode;

	if (!task) return NULL;

	topnode = xmlNewNode (NULL, "gtt:task");

	str = gtt_task_get_memo(task);
	if (str)
	{
		node = xmlNewNode (NULL, "memo");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);
	}

	str = gtt_task_get_notes(task);
	if (str)
	{
		node = xmlNewNode (NULL, "notes");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);
	}
	{
		GttBillable billable = gtt_task_get_billable(task);
		switch (billable)
		{
			case GTT_HOLD: str = "HOLD"; break;
			case GTT_BILLABLE: str = "BILLABLE"; break;
			case GTT_NOT_BILLABLE: str = "NOT_BILLABLE"; break;
			case GTT_NO_CHARGE: str = "NO_CHARGE"; break;
		}
		node = xmlNewNode (NULL, "billable");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);
	}

	{
		GttBillRate billrate = gtt_task_get_billrate(task);
		switch (billrate)
		{
			case GTT_REGULAR: str = "REGULAR"; break;
			case GTT_OVERTIME: str = "OVERTIME"; break;
			case GTT_OVEROVER: str = "OVEROVER"; break;
			case GTT_FLAT_FEE: str = "FLAT_FEE"; break;
		}
		node = xmlNewNode (NULL, "billrate");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);
	}

	/* add list of intervals */
	p = gtt_task_get_intervals (task);
	node = gtt_interval_list_to_dom_tree (p);
	xmlAddChild (topnode, node);

	return topnode;
}

/* convert a list of gtt tasks into a DOM tree */
static xmlNodePtr
gtt_task_list_to_dom_tree (GList *list)
{
	GList *p;
	xmlNodePtr topnode;
	xmlNodePtr node;

	if (!list) return NULL;

	topnode = xmlNewNode (NULL, "gtt:task-list");

	for (p=list; p; p=p->next)
	{
		GttTask *task = p->data;
		node = gtt_xml_task_to_dom_tree (task);
		xmlAddChild (topnode, node);
	}

   	return topnode;
}

/* ======================================================= */

/* convert one project into a dom tree */
static xmlNodePtr
gtt_xml_project_to_dom_tree (GttProject *prj)
{
	GList *children, *tasks;
	const char * str;
	char buff[80];
	xmlNodePtr node, topnode;

	if (!prj) return NULL;
	topnode = xmlNewNode (NULL, "gtt:project");

	node = xmlNewNode (NULL, "title");
	xmlNodeAddContent(node, gtt_project_get_title(prj));
	xmlAddChild (topnode, node);

	str = gtt_project_get_desc(prj);
	if (str && 0 != str[0])
	{
		node = xmlNewNode (NULL, "desc");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);	
	}

	str = gtt_project_get_notes(prj);
	if (str && 0 != str[0])
	{
		node = xmlNewNode (NULL, "notes");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);	
	}

	str = gtt_project_get_custid(prj);
	if (str && 0 != str[0])
	{
		node = xmlNewNode (NULL, "custid");
		xmlNodeAddContent(node, str);
		xmlAddChild (topnode, node);	
	}

	/* store id */
	g_snprintf (buff, sizeof(buff), "%d",
                    gtt_project_get_id(prj));
	node = xmlNewNode (NULL, "id");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* store price */
	g_snprintf (buff, sizeof(buff), "%.18g",
                    gtt_project_get_billrate(prj));
	node = xmlNewNode (NULL, "billrate");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* store price */
	g_snprintf (buff, sizeof(buff), "%.18g",
                    gtt_project_get_overtime_rate(prj));
	node = xmlNewNode (NULL, "overtime_rate");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* store price */
	g_snprintf (buff, sizeof(buff), "%.18g",
                    gtt_project_get_overover_rate(prj));
	node = xmlNewNode (NULL, "overover_rate");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* store price */
	g_snprintf (buff, sizeof(buff), "%.18g",
                    gtt_project_get_flat_fee(prj));
	node = xmlNewNode (NULL, "flat_fee");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* store min_interval */
	g_snprintf (buff, sizeof(buff), "%d",
                    gtt_project_get_min_interval(prj));
	node = xmlNewNode (NULL, "min_interval");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* store auto_merge_interval */
	g_snprintf (buff, sizeof(buff), "%d",
                    gtt_project_get_auto_merge_interval(prj));
	node = xmlNewNode (NULL, "auto_merge_interval");
	xmlNodeAddContent(node, buff);
	xmlAddChild (topnode, node);

	/* handle tasks */
	tasks = gtt_project_get_tasks(prj);
	node = gtt_task_list_to_dom_tree (tasks);
	xmlAddChild (topnode, node);	

	/* handle sub-projects */
	children = gtt_project_get_children (prj);
	if (children)
	{
		node = gtt_project_list_to_dom_tree (children);
		xmlAddChild (topnode, node);
	}

	return topnode;
}

/* convert a list of gtt projects into a DOM tree */
static xmlNodePtr
gtt_project_list_to_dom_tree (GList *list)
{
	GList *p;
	xmlNodePtr topnode;
	xmlNodePtr node;

	if (!list) return NULL;

	topnode = xmlNewNode (NULL, "gtt:project-list");

	for (p=list; p; p=p->next)
	{
		GttProject *prj = p->data;
		node = gtt_xml_project_to_dom_tree (prj);
		xmlAddChild (topnode, node);
	}

   	return topnode;
}

/* ======================================================= */

/* convert all gtt state into a DOM tree */
static xmlNodePtr
gtt_to_dom_tree (void)
{
	xmlNodePtr topnode;
	xmlNodePtr node;

	topnode = xmlNewNode(NULL, "gtt");
	xmlSetProp(topnode, "version", "1.0.0");

	node = gtt_project_list_to_dom_tree (gtt_get_project_list()); 
	if (node) xmlAddChild (topnode, node);

   	return topnode;

}

/* Write all gtt data to xml file */

void
gtt_xml_write_file (const char * filename)
{
	xmlNodePtr topnode;
	FILE *fh;

	fh = fopen (filename, "w");
	if (!fh) { gtt_err_set_code (GTT_CANT_OPEN_FILE); return; }

	topnode = gtt_to_dom_tree();

	fprintf(fh, "<?xml version=\"1.0\"?>\n");
	xmlElemDump (fh, NULL, topnode);
	xmlFreeNode (topnode);

	fprintf(fh, "\n");

	/* hack alert XXX we should check for error condition
	 * to make sure that we didn't run out of disk space, 
	 * have i/o errorsm, etc. */
	fflush (fh);
	fclose (fh);
}

/* ===================== END OF FILE ================== */
