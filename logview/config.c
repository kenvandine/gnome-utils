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

#include "configdata.h"
#include <stdlib.h>

/* ----------------------------------------------------------------------
   NAME:	CreateConfig
   DESCRIPTION:	Allocate memory for configuartion data and set default 
   		values.
   ---------------------------------------------------------------------- */

ConfigData *
CreateConfig(void)
{
  ConfigData *newcfg;

  newcfg = (ConfigData *)malloc (sizeof(ConfigData));
  if (newcfg == NULL)
    return (ConfigData *) NULL;

  /* Set paths */
  newcfg->regexp_db_path = NULL;
  newcfg->descript_db_path = NULL;
  newcfg->action_db_path = NULL;

  return newcfg;

}

