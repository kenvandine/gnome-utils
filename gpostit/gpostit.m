#include <obgnome/obgnome.h>

@interface PostitWindow : Gtk_Dialog
{
  id parent_app;
  id ent_title;
  id txt_message;
  GString *tmpstr;
}
- initForApp:(id) anapp;
- initPostitFromConfig:(char *) path
		forApp:(id) theapp;
- saveToConfig:(char *) path;
- (char *)getTitle;
- (BOOL)delete_event:(id) anObj :(GdkEvent *) event;
@end

@interface PostitApp : Gnome_App
{
  GList *notes;
  GString *tmpstr;
  id btnwin, new_btn, exit_btn;
}
- initPostitApp:(int)argc
	       :(char **)argv;
- newNote;
- removeNote:(id) theNote;
- clicked:(id) anObj;
- loadFromConfig:(char *) path;
- saveToConfig:(char *) path;
@end

int main(int argc, char *argv[])
{
  id myapp = [[PostitApp alloc] initPostitApp:argc :argv];

  [myapp loadFromConfig:"/gpostit"];
  [myapp run];
  [myapp saveToConfig:"/gpostit"];
  [myapp free];
  return 0;
}

@implementation PostitApp
- initPostitApp:(int)argc
	       :(char **)argv
{
  self = [super initApp:"gpostit" :NULL :argc :argv :0 :NULL];
  notes = NULL;
  tmpstr = g_string_new(NULL);
  btnwin = [[Gtk_Window alloc] initWithWindowType:GTK_WINDOW_DIALOG];
  new_btn = [[[Gtk_Button alloc] initWithLabel:"New note..."] show];
  exit_btn = [[[Gtk_Button alloc] castGtkButton:GTK_BUTTON(gnome_stock_button(GNOME_STOCK_BUTTON_CLOSE))] show];
  [btnwin add:new_btn];
  [btnwin add:exit_btn];
  [btnwin show];
  [btnwin connectObj:"delete_event" :self];
  [new_btn connectObj:"clicked" :self];
  [exit_btn connectObj:"clicked" :self];
  return self;
}

- newNote
{
  id theNote = [[[PostitWindow alloc] initForApp:self] show];
  notes = g_list_append(notes, theNote);
  return self;
}

- removeNote:(id) theNote
{
  notes = g_list_remove(notes, theNote);
  return self;
}

- (BOOL)delete_event:(id) anObj :(GdkEvent *) event
{
  [btnwin hide];
  [self quit];
  return TRUE;
}

- clicked:(id) anObj
{
  if(anObj == new_btn)
    [self newNote];
  else if(anObj == exit_btn)
    [self quit];
  return self;
}

- loadFromConfig:(char *) path
{
  gpointer iter;
  gchar *key;

  for(iter = gnome_config_init_iterator_sections(path),
	iter = gnome_config_iterator_next(iter, &key, NULL);
      iter;
      iter = gnome_config_iterator_next(iter, &key, NULL))
    {
      g_string_sprintf(tmpstr, "%s/%s", path, key);
      notes = g_list_append(notes,
			    [[[PostitWindow alloc]
			      initPostitFromConfig:tmpstr->str
			       forApp:self] show]);
      g_free(key);
    }

  return self;
}

- saveToConfig:(char *) path
{
  GList *ent;

  gnome_config_clean_file(path);
  
  for(ent = notes; ent; ent = ent->next)
    {
      g_string_sprintf(tmpstr, "%s/%s", path, [(id)ent->data getTitle]);
      [(id)ent->data saveToConfig:tmpstr->str];
    }
  gnome_config_sync();
  return self;
}
@end

@implementation PostitWindow
- initForApp:(id) anapp
{
  self = [super init];

  parent_app = anapp;
  tmpstr = g_string_new(NULL);

  ent_title = [[Gtk_Entry new] show];
  [vbox add:ent_title];

  txt_message = [[[[Gtk_Text new] set_editable:TRUE] set_point:0] show];
  [vbox add:txt_message];

  [self add:vbox];

  [self connect:"delete_event"];
  
  return self;
}

- initPostitFromConfig:(char *) path
		forApp:(id) theapp
{
  gchar *tmp;
  gint x, y;
  self = [self initForApp:theapp];

  g_string_sprintf(tmpstr, "%s/title", path);
  tmp = gnome_config_get_string(tmpstr->str);
  if(tmp)
    {
      [ent_title set_text:tmp];
      g_free(tmp);
    }

  
  g_string_sprintf(tmpstr, "%s/message", path);
  tmp = gnome_config_get_string(tmpstr->str);
  if(tmp)
    {
      [txt_message realize];
      [txt_message insert:NULL
		   colFg:&GTK_WIDGET([txt_message getGtkObject])->style->black
		   colBg:NULL
		   insChars:tmp
		   insLen:strlen(tmp)];
      g_free(tmp);
    }

  g_string_sprintf(tmpstr, "%s/x", path);
  x = gnome_config_get_int(tmpstr->str);
  g_string_sprintf(tmpstr, "%s/y", path);
  y = gnome_config_get_int(tmpstr->str);
  [self set_uposition:x :y];

  return self;
}

- saveToConfig:(char *) path
{
  gint x, y;

  gdk_window_get_origin(gtkwidget->window, &x, &y);

  g_string_sprintf(tmpstr, "%s/title", path);
  gnome_config_set_string(tmpstr->str, [ent_title get_text]);

  g_string_sprintf(tmpstr, "%s/message", path);
  gnome_config_set_string(tmpstr->str,
			  GTK_TEXT([txt_message getGtkObject])->text);

  g_string_sprintf(tmpstr, "%s/x", path);
  gnome_config_set_int(tmpstr->str, x);

  g_string_sprintf(tmpstr, "%s/y", path);
  gnome_config_set_int(tmpstr->str, y);

  return self;
}

- (char *)getTitle
{
  return [ent_title get_text];
}

- (BOOL)delete_event:(id) anObj :(GdkEvent *) event
{
  [parent_app removeNote:self];
  return FALSE;
}
@end
