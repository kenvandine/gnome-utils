/* GNOME Search Tool
 * Copyright (C) 1997 George Lebl.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>

#include "gsearchtool.h"
#include "unixcmd.h"

/*make a unix comamnd for the finfo structure*/
void
makecmd(char cmd[],char showcmd[],char appcmd[],struct finfo *f,int outpipe)
{
	char opt[512];

	if(f->methodfind) {
		opt[0]='\0';
		if(!(f->subdirs))
			strcat(opt,"-maxdepth 0");
		if(strlen(f->owner)>0) {
			strcat(opt," -user ");
			strcat(opt,f->owner);
		}
		if(strlen(f->group)>0) {
			strcat(opt," -group ");
			strcat(opt,f->group);
		}
		if(strlen(f->lastmod)>0) {
			strcat(opt," -mtime ");
			strcat(opt,f->lastmod);
		}
		if(f->mount)
			strcat(opt,"-mount ");
		
		strcat(opt," ");
		strcat(opt,f->extraopts);
		sprintf(cmd,"find %s -name '%s' -print0 %s",
			f->dir,
			f->file,
			opt);
	} else {
		if(f->subdirs) {
			sprintf(cmd,"locate '%s*/%s' '%s%s'",
				f->dir,
				f->file,
				f->dir,
				f->file);
		} else {
			sprintf(cmd,"locate '%s%s'",
				f->dir,
				f->file);
		}
	}

	/*add stuff to search through files*/
	if(strlen(f->searchtext)>0) {
		strcat(cmd," | xargs ");
		if(f->methodfind)
			strcat(cmd,"-0 ");

		if(f->searchegrep)
			strcat(cmd,"e");
		else if(f->searchfgrep)
			strcat(cmd,"f");
		strcat(cmd,"grep -l '");
		strcat(cmd,f->searchtext);
		strcat(cmd,"'");
	}

	/*now we differ*/
	strcpy(showcmd,cmd);
	sprintf(opt," >&%d",outpipe);
	strcat(cmd,opt);

	/*add stuff to apply commands (only to show and appcmd)*/
	if(strlen(f->applycmd)>0) {
		strcpy(appcmd,"xargs ");
		strcat(showcmd," | xargs ");
		/*unfortunately grep will only put '\n' terminated strings*/
		if(f->methodfind && f->searchtext[0]=='\0')
			strcat(showcmd,"-0 ");
		strcat(appcmd,"-0 ");

		strcat(appcmd,f->applycmd);
		strcat(showcmd,f->applycmd);
	}
}
