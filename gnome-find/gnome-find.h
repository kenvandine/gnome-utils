#ifndef GNOME_FIND_H
#define GNOME_FIND_H

typedef enum {
  UIL_BASIC = 0,
  UIL_DYNAMIC = 1,
  UIL_ADVANCED = 2
} UILevel;

GtkWidget *init_spec_vbox (int advanced);
void spec_vbox_add_spec (int advanced);
void spec_vbox_remove_spec (void);
void spec_vbox_set_advanced (int advanced);

#endif /*  GNOME_FIND_H */
