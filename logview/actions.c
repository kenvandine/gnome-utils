#include <config.h>
#include "logview.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <gnome.h>

#define MAX_NUM_MATCHES     10
#define DELIM               ":"

void mon_edit_actions (GtkWidget *widget, gpointer data);

static GList *copy_actions_db (GList *db);
void free_actions_db (GList **db);
void free_action (Action *action);
void print_actions_db (GList *db);
void make_tree_from_actions_db (GList *db);
void clear_actions_db (GList *db);

static void edit_actions_entry_cb (GtkWidget *widget, gpointer data);
static void remove_actions_entry_cb (GtkWidget *widget, gpointer data);
static void add_actions_entry_cb (GtkWidget *widget, gpointer data);
static void edit_action_entry (Action *action);
static void set_atk_relation (GtkWidget *label, GtkWidget *widget);

int exec_action_in_db (Log *log, LogLine *line, GList *db);
int read_actions_db (char *filename, GList **db);
int write_actions_db (char *filename, GList *db);



extern GList *actions_db;


static GList *local_actions_db = NULL;

GtkWidget *actions_dialog = NULL;
GtkWidget *ctree_view;
GtkTreeStore *ctree = NULL;
GtkTreeIter *selected_node = NULL;
GtkTreeIter *ctree_parent = NULL;

static void
apply_actions (GtkWidget *w, gpointer data)
{
	char *fname;

	free_actions_db (&actions_db);
	actions_db = local_actions_db;
	local_actions_db = NULL;

	fname = g_strdup_printf ("%s/.gnome/logview-actions.db",
				 g_get_home_dir ());
	if (write_actions_db (fname, actions_db)) {
		gtk_widget_destroy (actions_dialog);
	}
	g_free (fname);
}


/* ----------------------------------------------------------------------
   NAME:        mon_edit_actions
   DESCRIPTION: Create widow where actions are added and changed.
   ---------------------------------------------------------------------- */

void
mon_edit_actions (GtkWidget *widget, gpointer data)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *swin;
  GtkBox *vbox;
  GtkWidget *vbox2;
  const gchar *title[] = {N_("Action database")};
  GtkCellRenderer *cell_renderer;
  GtkTreeViewColumn *column; 
  GtkTooltips *tips;

  tips = gtk_tooltips_new ();

  /* Create main window ------------------------------------------------  */
  actions_dialog = gtk_dialog_new ();
  gtk_container_set_border_width (GTK_CONTAINER (actions_dialog), 5);
  gtk_window_set_title (GTK_WINDOW (actions_dialog), _("Actions"));
  gtk_widget_set_size_request (actions_dialog, 400, -1);

  button = gtk_dialog_add_button (GTK_DIALOG (actions_dialog),
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy),
                      GTK_OBJECT (actions_dialog));
  button = gtk_dialog_add_button (GTK_DIALOG (actions_dialog),
                      GTK_STOCK_OK, GTK_RESPONSE_OK);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (apply_actions),
                      GTK_OBJECT (actions_dialog));
  gtk_dialog_set_default_response (GTK_DIALOG (actions_dialog), GTK_RESPONSE_OK);

  vbox = GTK_BOX (GTK_DIALOG (actions_dialog)->vbox);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* List with actions */

  ctree = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  cell_renderer = gtk_cell_renderer_text_new ();
  ctree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ctree));
  g_object_unref (G_OBJECT (ctree));
  column = gtk_tree_view_column_new_with_attributes (*title, cell_renderer,
                                                     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ctree_view), column);
  selected_node = NULL;
  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (ctree_view));
  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (ctree_view), TRUE);
  gtk_widget_set_size_request (GTK_WIDGET (ctree_view), -1, 300);

/* ----------------------------------------------------------------------
  gtk_signal_connect (GTK_OBJECT (ctree), "button_press_event",
  GTK_SIGNAL_FUNC (button_press), NULL);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "button_press_event",
  GTK_SIGNAL_FUNC (after_press), NULL);
  gtk_signal_connect (GTK_OBJECT (ctree), "button_release_event",
  GTK_SIGNAL_FUNC (button_release), NULL);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "button_release_event",
  GTK_SIGNAL_FUNC (after_press), NULL);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "tree_move",
  GTK_SIGNAL_FUNC (after_move), NULL);
  -------------------------------------------------------------------- */

  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (swin), TRUE, TRUE, 0);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
  gtk_tree_view_column_set_alignment ( GTK_TREE_VIEW_COLUMN (column), 0.1);
  gtk_tree_selection_set_mode ( (GtkTreeSelection *)gtk_tree_view_get_selection
                               (GTK_TREE_VIEW (ctree_view)),
                                GTK_SELECTION_SINGLE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_tree_view_column_set_fixed_width ( GTK_TREE_VIEW_COLUMN (column), 300);
  gtk_widget_show_all (GTK_WIDGET(swin));
      
      
  /* Buttons in the side ------------------------------------------------ */

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
  gtk_box_set_spacing (GTK_BOX (hbox), GNOME_PAD_BIG);
  gtk_widget_show (vbox2);

  button = gtk_button_new_with_mnemonic (_("_Add"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (add_actions_entry_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  gtk_tooltips_set_tip (tips, button, "Add an action", NULL);
  
  button = gtk_button_new_with_mnemonic (_("_Edit"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (edit_actions_entry_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  gtk_tooltips_set_tip (tips, button, "Edit an action", NULL);
  
  button = gtk_button_new_with_mnemonic (_("_Remove"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (remove_actions_entry_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  gtk_tooltips_set_tip (tips, button, "Remove an action", NULL);

  /* Copy all actions */
  free_actions_db (&local_actions_db);
  local_actions_db = copy_actions_db (actions_db);

  /* Generate tree with action database */
  make_tree_from_actions_db (local_actions_db);

  gtk_widget_show (actions_dialog);
}


/* ----------------------------------------------------------------------
   NAME:          make_tree_from_actions_db
   DESCRIPTION:   Create the tree from the database.
   ---------------------------------------------------------------------- */

void
make_tree_from_actions_db (GList *db)
{
  char buffer[50];
  GList *item;
  GList *sibling = NULL;
  Action *action;
  GtkTreeIter newiter;

  /* Create first item */
  ctree_parent = NULL;
  buffer[0] = '\0';

  for(item = db; item != NULL; item=item->next)
    {
      action = (Action *)item->data;
      if (strncmp (buffer, action->tag, 49) != 0) {
	  strncpy (buffer, action->tag, 50);
	  buffer[49] = '\0';
          /* Add an action with a new tag as a top level node(parent node) */  
          gtk_tree_store_append (ctree, &newiter, NULL);
          gtk_tree_store_set (ctree, &newiter, 0, buffer, 1,
                              (gpointer)action, -1);
          ctree_parent = gtk_tree_iter_copy (&newiter);
	}
      else {
          /* If Tag of new action is same as an existing action's tag, add 
             the new action as a child to the existing action(parent) */

	  gtk_tree_store_append (ctree, &newiter, ctree_parent); 
	  gtk_tree_store_set (ctree, &newiter, 0, buffer, 1,
			      (gpointer)action, -1);
	}
    }

  /* done. */
  return;

}

static int
action_compare (gconstpointer a, gconstpointer b)
{
	const Action *aa = a;
	const Action *bb = b;

	return strcmp (aa->tag, bb->tag);
}



/* ----------------------------------------------------------------------
   NAME:        read_action_db
   DESCRIPTION: Reads the database with regular expressions to match.
   ---------------------------------------------------------------------- */

int
read_actions_db (char *filename, GList **db)
{
  Action *item;
  FILE *fp;
  char buffer[1024];
  char *c1, *tok;
  int done;

  /* Open regexp DB */
  fp = fopen (filename, "r");
  if (fp == NULL)
    {
      g_snprintf (buffer, sizeof (buffer),
		  _("Cannot open action data base <%s>! Open failed."), 
		  filename);
      ShowErrMessage (buffer);
      return(-1);
    }


  /* Start parsing file */
  done = FALSE;
  buffer[1023] = '\0';
  while (!done)
    {

      /* Read line */
      if (fgets( buffer, 1023, fp) == NULL)
	{
	  done = TRUE;
	  continue;
	}
      /* Skip spaces */
      c1 = buffer;
      while (*c1 == ' ' || *c1 == '\t') c1++;
      if (*c1 == '\0' || *c1 == '\n')
	continue; /* Nothing to do here */
      /* Ignore lines that begin with '#' */
      if (*c1 == '#')
	continue;

      /* Alloc memory for item */
      item = g_new0 (Action, 1);

      /* Read TAG */
      tok = strtok (c1, DELIM);
      if (tok == NULL)
	{
	  ShowErrMessage (_("Error parsing actions data base"));
	  free_actions_db (db);
	  return (-1);
	}
      strncpy (item->tag, tok, 49);
      item->tag[49] = '\0';

      /* Read log regexp . */
      tok = strtok (NULL, DELIM);
      if (tok != NULL)
	item->log_name = g_strdup (tok);
      else
	{
	  ShowErrMessage (_("Error parsing actions data base"));
	  free_actions_db (db);
	  return (-1);
	}

      /* Read process regexp . */
      tok = strtok (NULL, DELIM);
      if (tok != NULL)
	item->process = g_strdup (tok);
      else
	{
	  ShowErrMessage (_("Error parsing actions data base"));
	  free_actions_db (db);
	  return (-1);
	}

      /* Read message regexp . */
      tok = strtok (NULL, DELIM);
      if (tok != NULL)
	item->message = g_strdup (tok);
      else
	{
	  ShowErrMessage (_("Error parsing actions data base"));
	  free_actions_db (db);
	  return (-1);
	}

      /* Read description regexp . */
      tok = strtok (NULL, DELIM);
      if (tok != NULL)
	item->description = g_strdup (tok);
      else
	{
	  ShowErrMessage (_("Error parsing actions data base"));
	  free_actions_db (db);
	  return (-1);
	}

      /* Read action regexp . */
      tok = strtok (NULL, "\0\n");
      if (tok != NULL)
	item->action = g_strdup (tok);
      else
	{
	  ShowErrMessage (_("Error parsing actions data base"));
	  free_actions_db (db);
	  return (-1);
	}

      
      /* Add item to list */
      if (item != NULL)
	*db = g_list_append (*db, item);
    }

  *db = g_list_sort (*db, action_compare);

  return TRUE;
}


/* ----------------------------------------------------------------------
   NAME:        write_regexp_db
   DESCRIPTION: This file generates the actions DB file. It assumes that
                all the entries in each action are valid.
   ---------------------------------------------------------------------- */

int
write_actions_db (char *filename, GList *db)
{
  GList *item;
  Action *action;
  FILE *fp;

  
  fp = fopen (filename, "w");
  if (fp == NULL)
    {
      ShowErrMessage (_("Can't write to actions database!"));
      return FALSE;
    }
  
  /* Write a  header to the DB file */
  fprintf (fp, "#\n# Actions DB\n#\n# Generated by logview. Don't edit by hand\n");
  fprintf (fp, "# unless you know what you are doing!\n#\n\n");
  
  /* Search for daemon in our list */
  item = g_list_first (db);
  for (item = g_list_first (db); item != NULL; item=item->next)
    {
      action = (Action *)item->data;
      /* Check that there is a valid entry */
      if (*action->tag == '\0' || action->log_name == NULL ||
	   action->process == NULL || action->message == NULL ||
	   action->description == NULL ||
	   action->action == NULL)
	continue;

      /* write entry */
      fprintf (fp, "%s:%s:%s:%s:%s:%s\n", action->tag,
	       action->log_name, action->process, 
	       action->message, action->description,
	       action->action);
    }

  fclose (fp);

  return TRUE;
}

/* ----------------------------------------------------------------------
   NAME:        free_actions_db
   DESCRIPTION: 
   ---------------------------------------------------------------------- */

void
free_actions_db (GList **db)
{
	if (*db != NULL) {
		clear_actions_db (*db);
		g_list_free (*db);
		*db = NULL;
	}
}

/* ----------------------------------------------------------------------
   NAME:        exec_action_in_db
   DESCRIPTION: Try to find the error message in line in the database.
   ---------------------------------------------------------------------- */

int
exec_action_in_db (Log *log, LogLine *line, GList *db)
{
  GList *item;
  Action *cur_action = NULL;
  regex_t preg;
  regmatch_t matches[MAX_NUM_MATCHES];
  int doesnt_match;
  pid_t pid;

  /* Search for daemon in our list */
  for (item = db; item != NULL; item = item->next)
    {
      doesnt_match = FALSE;
      cur_action = (Action *)item->data;
      regcomp (&preg, cur_action->log_name, REG_EXTENDED);
      doesnt_match = regexec (&preg, log->name, MAX_NUM_MATCHES, matches, 0);
      regfree (&preg);
      if (doesnt_match)
	continue;
      regcomp (&preg, cur_action->process, REG_EXTENDED);
      doesnt_match = regexec (&preg, line->process, MAX_NUM_MATCHES, matches, 0);
      regfree (&preg);
      if (doesnt_match)
	continue;
      regcomp (&preg, cur_action->message, REG_EXTENDED);
      doesnt_match = regexec (&preg, line->message, MAX_NUM_MATCHES, matches, 0);
      regfree (&preg);
      if (doesnt_match)
	continue;
      
      break;
    }

  if (item == NULL)
      return FALSE; /* not in our list */

  /* If there is a non-null action execute it */
  if (cur_action != NULL)
   {
     if ((pid = fork()) < 0)
      {
        return FALSE;
      }
     else if (pid == 0)
      {
        if (execlp(cur_action->action, cur_action->action, NULL) == -1)
         {
	   ShowErrMessage (_("Error while executing specified action"));
	   exit(1);
         }
      }
   }

  return TRUE;
}

/* ----------------------------------------------------------------------
   NAME:        add_actions_entry_cb
   DESCRIPTION: Add an entry to the actions DB.
   ---------------------------------------------------------------------- */

static void
add_actions_entry_cb (GtkWidget *widget, gpointer data)
{
  Action *action;

  action = g_new (Action, 1);
  
  g_snprintf (action->tag, sizeof (action->tag), _("<empty>"));
  action->log_name = g_strdup (_("log name regexp"));
  action->process = g_strdup (_("process regexp"));
  action->message = g_strdup (_("message regexp"));
  action->action = g_strdup (_("action to execute when regexps are TRUE"));
  action->description = g_strdup (_("description"));

  edit_action_entry (action);

}

static void
remove_actions_entry_cb (GtkWidget *widget, gpointer data)
{
  Action *action;
  GtkTreeSelection *selection;
  GtkTreeIter newiter;
  GtkTreeModel *model = NULL;
  gboolean selected;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ctree_view));
  selected = gtk_tree_selection_get_selected (selection, &model, &newiter);
  if (selected == FALSE)
	  return;

  selected_node = gtk_tree_iter_copy (&newiter);
  gtk_tree_model_get (GTK_TREE_MODEL (model), selected_node, 1,
		      (gpointer)&action, -1);

  if (action != NULL) {
	  local_actions_db = g_list_remove (local_actions_db, action);
	  gtk_tree_store_clear (ctree);
	  selected_node = NULL;
	  make_tree_from_actions_db (local_actions_db);
  }
}

/* ----------------------------------------------------------------------
   NAME:        edit_actions_entry_cb
   DESCRIPTION: Function called when the edit button is pressed.
   ---------------------------------------------------------------------- */

static void
edit_actions_entry_cb (GtkWidget *widget, gpointer data)
{
  Action *action;
  GtkTreeSelection *selection;
  GtkTreeIter newiter;
  GtkTreeModel *model = NULL;
  gboolean selected;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ctree_view));
  selected = gtk_tree_selection_get_selected (selection, &model, &newiter);
  if (selected == FALSE)
	  return;

  selected_node = gtk_tree_iter_copy (&newiter);
  gtk_tree_model_get (GTK_TREE_MODEL (model), selected_node, 1,
		      (gpointer)&action, -1);

  edit_action_entry (action);
}

static Action *edited_action = NULL;

static void
apply_edit (GtkWidget *w, gpointer data)
{
	char *text;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	GtkWidget *action_record = data;
	GtkWidget *tag =
		gtk_object_get_data (GTK_OBJECT (action_record), "tag");
	GtkWidget *log_name =
		gtk_object_get_data (GTK_OBJECT (action_record), "log_name");
	GtkWidget *process =
		gtk_object_get_data (GTK_OBJECT (action_record), "process");
	GtkWidget *message =
		gtk_object_get_data (GTK_OBJECT (action_record), "message");
	GtkWidget *action =
		gtk_object_get_data (GTK_OBJECT (action_record), "action");
	GtkWidget *description =
		gtk_object_get_data (GTK_OBJECT (action_record), "description");

	text = gtk_editable_get_chars (GTK_EDITABLE (tag), 0, -1);
	strncpy (edited_action->tag, text, 50);
	edited_action->tag[49] = '\0';

	g_free (edited_action->log_name);
	edited_action->log_name =
		gtk_editable_get_chars (GTK_EDITABLE (log_name), 0, -1);
	g_free (edited_action->process);
	edited_action->process =
		gtk_editable_get_chars (GTK_EDITABLE (process), 0, -1);
	g_free (edited_action->message);
	edited_action->message =
		gtk_editable_get_chars (GTK_EDITABLE (message), 0, -1);
	g_free (edited_action->action);
	edited_action->action =
		gtk_editable_get_chars (GTK_EDITABLE (action), 0, -1);
	g_free (edited_action->description);
	edited_action->description =
		gtk_editable_get_chars (GTK_EDITABLE (description), 0, -1);

	if ( ! g_list_find (local_actions_db, edited_action))
		local_actions_db = g_list_prepend (local_actions_db,
						   edited_action);

	local_actions_db = g_list_sort (local_actions_db,
					action_compare);

	gtk_tree_store_clear (ctree);
	selected_node = NULL;
	make_tree_from_actions_db (local_actions_db);
}

/* ----------------------------------------------------------------------
   NAME:        edit_action_entry
   DESCRIPTION: Edit a DB record on a window.
   ---------------------------------------------------------------------- */

static void
edit_action_entry (Action *action)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *text;
  GtkWidget *entry;
  GtkWidget *action_record;
  GtkWidget *button;
  GtkBox *vbox;
  GtkTooltips *tips;

  if (!action)
    return;

  edited_action = action;
  
  /* Create main window ------------------------------------------------  */
  action_record = gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (action_record), "Edit Action");
  button = gtk_dialog_add_button (GTK_DIALOG (action_record),
               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy),
                      GTK_OBJECT (action_record));

  button = gtk_dialog_add_button (GTK_DIALOG (action_record),
               GTK_STOCK_OK, GTK_RESPONSE_OK);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (apply_edit),
                      action_record);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy),
                      GTK_OBJECT (action_record));
  gtk_dialog_set_default_response (GTK_DIALOG (action_record), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (action_record), 5);
  gtk_widget_set_size_request (action_record, 375, -1); 


  vbox = GTK_BOX (GTK_DIALOG(action_record)->vbox);

  /* Show fields to edit ----------------------------------------------- */
  tips = gtk_tooltips_new ();

  /* Tag */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new_with_mnemonic ("_Tag:");
  gtk_widget_set_size_request (label, 60, -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_editable (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), action->tag);
  gtk_tooltips_set_tip (tips, entry, "Tag that identifies the log file.", NULL);
  gtk_widget_show (entry); 
  gtk_object_set_data (GTK_OBJECT (action_record), "tag", entry);
  set_atk_relation (label, entry);

  /* log name */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new_with_mnemonic ("_Log name:");
  gtk_widget_set_size_request (label, 60, -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_editable (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), action->log_name);
  gtk_tooltips_set_tip (tips, entry, "Regular expression that will match the log name.", 
			NULL);
  gtk_widget_show (entry); 
  gtk_object_set_data (GTK_OBJECT (action_record), "log_name", entry);
  set_atk_relation (label, entry);
      
  /* Process */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new_with_mnemonic ("_Process:");
  gtk_widget_set_size_request (label, 60, -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_editable (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), action->process);
  gtk_tooltips_set_tip (tips, entry, "Regular expression that will match process part of message.", 
			NULL);
  gtk_widget_show (entry); 
  gtk_object_set_data (GTK_OBJECT (action_record), "process", entry);
  set_atk_relation (label, entry);

  /* Message */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new_with_mnemonic ("_Message:");
  gtk_widget_set_size_request (label, 60, -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_editable (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), action->message);
  gtk_tooltips_set_tip (tips, entry, "Regular expression that will match the message.", 
			NULL);
  gtk_widget_show (entry); 
  gtk_object_set_data (GTK_OBJECT (action_record), "message", entry);
  set_atk_relation (label, entry);

  /* Action */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new_with_mnemonic ("_Action:");
  gtk_widget_set_size_request (label, 60, -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_editable (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), action->action);
  gtk_tooltips_set_tip (tips, entry, "Action that will be executed if all previous regexps. are matched. This is executed by a system command: system (action).", 
			NULL);
  gtk_widget_show (entry); 
  gtk_object_set_data (GTK_OBJECT (action_record), "action", entry);
  set_atk_relation (label, entry);

  /* Description */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new_with_mnemonic (_("_Description:"));
  gtk_widget_set_size_request (label, 60, -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  text = gtk_entry_new ();
  gtk_widget_set_size_request (text, 200, -1);
  gtk_entry_set_editable (GTK_ENTRY (text), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (text), action->description);
  gtk_tooltips_set_tip (tips, text, _("Description of this entry."), NULL);
  gtk_widget_show (text); 
  gtk_object_set_data (GTK_OBJECT (action_record), "description", text);
  set_atk_relation (label, text);

  gtk_widget_show (action_record); 
}

/* ----------------------------------------------------------------------
   NAME:        print_actions_db
   DESCRIPTION: Prints the database (for debbuging purposes).
   ---------------------------------------------------------------------- */

void
print_actions_db (GList *db)
{
  GList *item;
  Action *action;

  /* Search for daemon in our list */
  
  for (item = g_list_first (db); item != NULL; item = item->next)
    {
      action = (Action *)item->data;
      printf (_("tag: [%s]\nlog_name: [%s]\nprocess: [%s]\nmessage: [%s]\n"
		"description: [%s]\naction: [%s]\n"), 
	      action->tag,
	      action->log_name,
	      action->process,
	      action->message,
	      action->description,
	      action->action);
    }
      
  return;

}


/* ----------------------------------------------------------------------
   NAME:        clear_actions_db
   DESCRIPTION: Free all memory used by actions DB.
   ---------------------------------------------------------------------- */

void
clear_actions_db (GList *db)
{
  GList *item;
  Action *action;

  for (item = g_list_first (db); item != NULL; item = item->next)
    {
      action = (Action *)item->data;
      free_action (action);
      item->data = NULL;
    }
   
  return;

}

/* ----------------------------------------------------------------------
   NAME:        free_action
   DESCRIPTION: Given an action struct, clear it.
   ---------------------------------------------------------------------- */

void
free_action (Action *action)
{
	if (action) {
		g_free (action->log_name);
		g_free (action->process);
		g_free (action->message);

		g_free (action->action);
		g_free (action->description);

		g_free (action);
	}
}

static GList *
copy_actions_db (GList *db)
{
	GList *li;
	GList *copy;

	if (db == NULL)
		return NULL;

	copy = g_list_copy (db);
	for (li = copy; li != NULL; li = li->next) {
		Action *old = li->data;
		Action *new = g_new0 (Action, 1);

		/* these are the same size so strcpy is safe */
		strcpy (new->tag, old->tag);

		new->log_name = g_strdup (old->log_name);
		new->process = g_strdup (old->process);
		new->message = g_strdup (old->message);
		new->action = g_strdup (old->action);
		new->description = g_strdup (old->description);
		li->data = new;
	}

	return copy;
}

static void
set_atk_relation (GtkWidget *label, GtkWidget *widget)
{
	AtkObject *atk_widget;
	AtkObject *atk_label;
	AtkRelationSet *relation_set;
	AtkRelation *relation;
	AtkObject *targets[1];
	atk_widget = gtk_widget_get_accessible (widget);
	atk_label = gtk_widget_get_accessible (label);

	/* Set label-for relation */
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);

	/* Check if gail is loaded */
	if (GTK_IS_ACCESSIBLE (atk_widget) == FALSE)
		return;

	/* Set labelled-by relation */
	relation_set = atk_object_ref_relation_set (atk_widget);
	targets[0] = atk_label;
	relation = atk_relation_new (targets, 1, ATK_RELATION_LABELLED_BY);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (G_OBJECT (relation));
}

