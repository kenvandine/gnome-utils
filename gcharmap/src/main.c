/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Sat Apr 15 20:18:35 CEST 2000
    copyright            : (C) 2000 by Hongli Lai
    email                : hongli@telekabel.nl
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config.h>
#include <gnome.h>

#include "interface.h"

int 
main (int argc, char *argv[]) 
{
  bindtextdomain(PACKAGE,GNOMELOCALEDIR);
  textdomain(PACKAGE);
  
  gnome_init ("Character Map", VERSION, argc, argv);
  main_new ();
  gtk_main ();
  return 0;
}
