#include <gnome.h>
#include <stdio.h>
#include <string.h>

static GList *mime_types = NULL;


GList*
g_list_insert_sorted_if_not_member (GList        *list,
				    gpointer      data,
				    GCompareFunc  func,
				    gboolean *inserted)
{
  GList *tmp_list = list;
  GList *new_list;
  gint cmp;

  *inserted = FALSE;
  g_return_val_if_fail (func != NULL, list);
  
  if (!list) 
    {
      new_list = g_list_alloc();
      new_list->data = data;
      *inserted = TRUE;
      return new_list;
    }
  
  cmp = (*func) (data, tmp_list->data);
  
  while ((tmp_list->next) && (cmp > 0))
    {
      tmp_list = tmp_list->next;
      cmp = (*func) (data, tmp_list->data);
    }

  if (cmp == 0) {
    *inserted = FALSE;
    return list;
  } else {
    *inserted = TRUE;
    new_list = g_list_alloc();
    new_list->data = data;
    
    if ((!tmp_list->next) && (cmp > 0))
      {
	tmp_list->next = new_list;
	new_list->prev = tmp_list;
	return list;
      }
    
    if (tmp_list->prev)
      {
	tmp_list->prev->next = new_list;
	new_list->prev = tmp_list->prev;
      }
    new_list->next = tmp_list;
    tmp_list->prev = new_list;
    
    if (tmp_list == list)
      return new_list;
    else
      return list;
  }
}


void
init_mime_types()
{
  char *gnome_mime_path;
  FILE *gnome_mime_file;
  char *mime_magic_path;
  FILE *mime_magic_file;
  gboolean(inserted);
  int index;

  char line[512];
  char *tline;

  gnome_mime_path = gnome_datadir_file("mime-info/gnome.mime");
  mime_magic_path = gnome_config_file("mime-magic");

  gnome_mime_file=fopen(gnome_mime_path, "r");

  while (NULL != fgets(line, 512, gnome_mime_file)) {
    if (line[0]!='#' && line[0] != ' ' &&
	NULL != strchr(line, '/')) {
  
      index= strlen(line) -1;
      while(NULL != strchr(" \t\n\r",line[index])) {
	line[index]=0;
	index--;
      }

      tline = strdup(line);
      mime_types = g_list_insert_sorted_if_not_member (mime_types, tline,
						       strcmp, &inserted);
      if (!inserted) {
	free(tline); 
      }
    }
  }
  fclose(gnome_mime_file);

  mime_magic_file=fopen(mime_magic_path, "r");
  while (NULL != fgets(line, 512, mime_magic_file)) {
    char *tmp1;
    char *tmp2;
    
    index= strlen(line) -1;
    while(NULL != strchr(" \t\n\r",line[index])) {
      line[index]=0;
      index--;
    }
    
    tmp1 = strrchr(line,' ');
    tmp2 = strrchr(line,'\t');

    tmp1 = MAX(tmp1, tmp2);
    

    if (tmp1 != NULL) {
      /* cheap HACK! */
      if (!strcmp(tmp1, " binary")) {
	break;
      } else {
	while(0!=tmp1[0] && NULL!=strchr(" \t\n\r",tmp1[0])) {
	  tmp1++;
	}
	tline = strdup(tmp1);
	mime_types = g_list_insert_sorted_if_not_member (mime_types, tline,
							 strcmp, &inserted);
	if (!inserted) {
	  free(tline);
	}	
      }
    }
  }
}

GtkWidget *
mimetype_option_menu_new()
{
  GtkOptionMenu *option;
  GtkMenu *menu;
  GtkMenuItem *cur_item;
  
  if (mime_types==NULL) {
    init_mime_types();
  }
}
