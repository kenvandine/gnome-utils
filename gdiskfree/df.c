/* df - summarize free disk space
   Copyright (C) 91, 95, 1996 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.
   --human-readable and --megabyte options added by lm@sgi.com.  */
/* gnomification by Gregory McLean <gregm@comstar.net>. */

#include <config.h>

#include <glibtop.h>
#include <glibtop/fsusage.h>
#include <glibtop/mountlist.h>

#include <gnome.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <errno.h>

#include "save-cwd.h" 
#include "util.h"

#define STREQ(s1, s2) ((strcmp (s1, s2) == 0))

void strip_trailing_slashes ();
char *xgetcwd ();

/* The maximum length of a human-readable string.  Be pessimistic
   and assume `int' is 64-bits wide.  Converting 2^63 - 1 gives the
   14-character string, 8796093022208G.  The number being converted
   is the number of 1024-byte blocks, so we divide by 1024 * 1024.  */
#define LONGEST_HUMAN_READABLE_1K_BYTE_BLOCKS 14

/* If nonzero, show inode information. */
static int inode_format;

/* If nonzero, show even filesystems with zero size or
   uninteresting types. */
static int show_all_fs;

/* If nonzero, output data for each filesystem corresponding to a
   command line argument -- even if it's a dummy (automounter) entry.  */
static int show_listed_fs;

/* If nonzero, use variable sized printouts instead of 512-byte blocks. */
static int human_blocks;

#ifdef NEED_UNUSED_VARS
/* If nonzero, use 1K blocks instead of 512-byte blocks. */
static int kilobyte_blocks;

/* If nonzero, use 1M blocks instead of 512-byte blocks. */
static int megabyte_blocks;
#endif

/* If nonzero, use the POSIX output format.  */
static int posix_format;

/* If nonzero, invoke the `sync' system call before getting any usage data.
   Using this option can make df very slow, especially with many or very
   busy disks.  Note that this may make a difference on some systems --
   SunOs4.1.3, for one.  It is *not* necessary on Linux.  */
static int require_sync = 0;

/* Nonzero if errors have occurred. */
static int exit_status;

/* A filesystem type to display. */

struct fs_type_list
{
  char *fs_name;
  struct fs_type_list *fs_next;
};

/* Linked list of filesystem types to display.
   If `fs_select_list' is NULL, list all types.
   This table is generated dynamically from command-line options,
   rather than hardcoding into the program what it thinks are the
   valid filesystem types; let the user specify any filesystem type
   they want to, and if there are any filesystems of that type, they
   will be shown.

   Some filesystem types:
   4.2 4.3 ufs nfs swap ignore io vm efs dbg */

static struct fs_type_list *fs_select_list;

/* Linked list of filesystem types to omit.
   If the list is empty, don't exclude any types.  */

static struct fs_type_list *fs_exclude_list;

/* Linked list of mounted filesystems. */
static glibtop_mountentry *mount_list;
static unsigned mount_list_entries;

/* If nonzero, print filesystem type as well.  */
static int print_type;

static int       selected_fstype       (const char *fstype);
static int       excluded_fstype       (const char *fstype);
static void      exit_menu_cb          (GtkWidget  *widget,
					gpointer   data);
static void      about_cb              (GtkWidget  *widget,
					gpointer   data);
/* GtkWidget   **t_dial; */

static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_MENU_EXIT_ITEM(exit_menu_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP ("GDiskFree"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (about_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo app_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE(file_menu),
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
	GNOMEUIINFO_END
};


static void
exit_menu_cb (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}
GtkWidget *window;
static void 
build_app_window ()
{
  GtkWidget           *w_box;
  GtkWidget           *frame;
  GtkWidget           *dial;
  GtkAdjustment       *adjustment;
  int                 i;
  gchar               *buf;

  window = gnome_app_new ("GDiskFree", "Disk Free");
  gtk_signal_connect(GTK_OBJECT(window),"destroy",
		     GTK_SIGNAL_FUNC(gtk_main_quit),NULL);
  
  w_box = gtk_vbox_new (FALSE, 0);
  for (i = 0; i < mount_list_entries; i++) 
    {
      if (!excluded_fstype (mount_list[i].type)) 
	{
	  buf = g_malloc (strlen (mount_list[i].devname) + 20);
	  snprintf (buf, (strlen (mount_list[i].devname) + 20), 
		    "%s_%s", mount_list[i].devname,
		    "dial_frame");
	  frame = gtk_frame_new (mount_list[i].devname);
	  gtk_widget_set_name (GTK_WIDGET (window), buf);
	  gtk_object_set_data (GTK_OBJECT (frame), buf, frame);
	  adjustment = GTK_ADJUSTMENT (gtk_adjustment_new 
				       (0.0, 0.0, 100.0, 0.01,
					0.1, 0.0));
	  dial = gtk_dial_new (adjustment);
	  snprintf (buf, (strlen (mount_list[i].devname) + 20), 
		    "%s_%s", mount_list[i].devname,
		    "dial_widget");
	  gtk_widget_set_name (GTK_WIDGET (dial), buf);
	  gtk_object_set_data (GTK_OBJECT (window), buf, dial);
	  g_free (buf);
	  gtk_dial_set_view_only(GTK_DIAL(dial), TRUE);
	  gtk_widget_set_usize (dial, 75, 60);
	  gtk_container_add (GTK_CONTAINER (frame), dial);
	  gtk_box_pack_start (GTK_BOX (w_box), frame, FALSE, FALSE, 0);
	}
    }
  
  gnome_app_set_contents (GNOME_APP (window), w_box);
  gnome_app_create_menus (GNOME_APP (window), app_menu);
  gtk_widget_show_all (window);
}

static void
about_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget     *about;
  static const char         *authors[] = {
    "Gregory McLean <gregm@comstar.net>",
    NULL
  };

  about = gnome_about_new ("GDiskFree", "0.1",
			   "(C) 91, 95, 1996 Free Software Foundation, Inc.",
			   authors,
			   _("A quick hack to show the amount of space free "
			     "on mounted file systems uitilizing a dial."),
			   NULL );
  gtk_widget_show (about);
}

/* Convert N_1K_BYTE_BLOCKS to a more readable string than %d would.
   Most people visually process strings of 3-4 digits effectively,
   but longer strings of digits are more prone to misinterpretation.
   Hence, converting to an abbreviated form usually improves readability.
   Use a suffix indicating multiples of 1024 (M) and 1024*1024 (G).
   For example, 8500 would be converted to 8.3M, 133456345 to 127G,
   and so on.  Numbers smaller than 1024 get the `K' suffix.  */

#ifdef NEED_UNUSED_FUNCTIONS
static char *
human_readable_1k_blocks (int n_1k_byte_blocks, char *buf, int buf_len)
{
  const char *suffix;
  double amt;
  char *p;

  assert (buf_len > LONGEST_HUMAN_READABLE_1K_BYTE_BLOCKS);

  p = buf;
  amt = n_1k_byte_blocks;

  if (amt >= 1024 * 1024)
    {
      amt /= (1024 * 1024);
      suffix = "G";
    }
  else if (amt >= 1024)
    {
      amt /= 1024;
      suffix = "M";
    }
  else
    {
      suffix = "K";
    }

  if (amt >= 10)
    {
      sprintf (p, "%4.0f%s", amt, suffix);
    }
  else if (amt == 0)
    {
      strcpy (p, "0");
    }
  else
    {
      sprintf (p, "%4.1f%s", amt, suffix);
    }
  return (p);
}
#endif

/* If FSTYPE is a type of filesystem that should be listed,
   return nonzero, else zero. */

static int
selected_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_select_list == NULL || fstype == NULL)
    return 1;
  for (fsp = fs_select_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return 1;
  return 0;
}

/* If FSTYPE is a type of filesystem that should be omitted,
   return nonzero, else zero. */

static int
excluded_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_exclude_list == NULL || fstype == NULL)
    return 0;
  for (fsp = fs_exclude_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return 1;
  return 0;
}

static void
update_dev (const char *disk, const char *mount_point, const char *fstype,
	    int i)
{
  GtkWidget       *tmp;
  glibtop_fsusage fsu;
  long            blocks_used;
  long            blocks_percent_used;
  const char      *stat_file;
  gchar           *buf;

  if (!selected_fstype (fstype) || excluded_fstype (fstype)) 
    return;

  if (require_sync)
    sync ();
  buf = (gchar *)g_malloc (strlen (disk) + 20);
  stat_file = mount_point ? mount_point : disk;
  glibtop_get_fsusage (&fsu, stat_file);

  fsu.blocks /= 2*1024;
  fsu.bfree  /= 2*1024;
  fsu.bavail /= 2*1024;

  blocks_used = fsu.blocks - fsu.bfree;
  blocks_percent_used = (long) 
    (blocks_used * 100.0 / (blocks_used + fsu.bavail) + 0.5);
  snprintf (buf, (strlen (disk) + 20), "%s_%s",
	    disk, "dial_widget");
  tmp = (GtkWidget *)get_widget ((GtkWidget *)window, buf);
  g_free (buf);
  if (tmp)
    gtk_dial_set_percentage ( (GtkDial *)tmp, 
			      (blocks_percent_used / 100.0)); 
}


/* Display a space listing for the disk device with absolute path DISK.
   If MOUNT_POINT is non-NULL, it is the path of the root of the
   filesystem on DISK.
   If FSTYPE is non-NULL, it is the type of the filesystem on DISK.
   If MOUNT_POINT is non-NULL, then DISK may be NULL -- certain systems may
   not be able to produce statistics in this case.  */
#ifdef NEED_UNUSED_FUNCTIONS
static void
show_dev (const char *disk, const char *mount_point, const char *fstype)
{
  glibtop_fsusage fsu;
  long blocks_used;
  long blocks_percent_used;
  long inodes_used;
  long inodes_percent_used;
  const char *stat_file;

  if (!selected_fstype (fstype) || excluded_fstype (fstype))
    return;

  /* If MOUNT_POINT is NULL, then the filesystem is not mounted, and this
     program reports on the filesystem that the special file is on.
     It would be better to report on the unmounted filesystem,
     but statfs doesn't do that on most systems.  */
  stat_file = mount_point ? mount_point : disk;

  glibtop_get_fsusage (&fsu, stat_file);

  if (fsu.flags == 0)
    {
      error (0, errno, "%s", stat_file);
      exit_status = 1;
      return;
    }

  if (megabyte_blocks)
    {
      fsu.blocks /= 2*1024;
      fsu.bfree /= 2*1024;
      fsu.bavail /= 2*1024;
    }
  else if (kilobyte_blocks)
    {
      fsu.blocks /= 2;
      fsu.bfree /= 2;
      fsu.bavail /= 2;
    }

  if (fsu.blocks == 0)
    {
      if (!show_all_fs && !show_listed_fs)
	return;
      blocks_used = fsu.bavail = blocks_percent_used = 0;
    }
  else
    {
      blocks_used = fsu.blocks - fsu.bfree;
      blocks_percent_used = (long)
	(blocks_used * 100.0 / (blocks_used + fsu.bavail) + 0.5);
    }

  if (fsu.files == 0)
    {
      inodes_used = fsu.ffree = inodes_percent_used = 0;
    }
  else
    {
      inodes_used = fsu.files - fsu.ffree;
      inodes_percent_used = (long)
	(inodes_used * 100.0 / fsu.files + 0.5);
    }

  if (! disk)
    disk = "-";			/* unknown */

  printf ((print_type ? "%-13s" : "%-20s"), disk);
  if ((int) strlen (disk) > (print_type ? 13 : 20) && !posix_format)
    printf ((print_type ? "\n%13s" : "\n%20s"), "");

  if (! fstype)
    fstype = "-";		/* unknown */
  if (print_type)
    printf (" %-5s ", fstype);

  if (inode_format)
    printf (" %7ld %7ld %7ld %5ld%%",
	    (long)fsu.files,
	    (long)inodes_used, (long)fsu.ffree, (long)inodes_percent_used);
  else if (human_blocks)
    {
      char buf[3][LONGEST_HUMAN_READABLE_1K_BYTE_BLOCKS + 1];
      printf (" %4s %4s  %5s  %5ld%% ",
	      human_readable_1k_blocks (fsu.blocks, buf[0],
				    LONGEST_HUMAN_READABLE_1K_BYTE_BLOCKS + 1),
	      human_readable_1k_blocks (blocks_used, buf[1],
				    LONGEST_HUMAN_READABLE_1K_BYTE_BLOCKS + 1),
	      human_readable_1k_blocks (fsu.bavail, buf[2],
				    LONGEST_HUMAN_READABLE_1K_BYTE_BLOCKS + 1),
	      blocks_percent_used);
    }
  else
    printf (" %7ld %7ld  %7ld  %5ld%% ",
	    (long)fsu.blocks, blocks_used, (long)fsu.bavail, blocks_percent_used);

  if (mount_point)
    {
#ifdef HIDE_AUTOMOUNT_PREFIX
      /* Don't print the first directory name in MOUNT_POINT if it's an
	 artifact of an automounter.  This is a bit too aggressive to be
	 the default.  */
      if (strncmp ("/auto/", mount_point, 6) == 0)
	mount_point += 5;
      else if (strncmp ("/tmp_mnt/", mount_point, 9) == 0)
	mount_point += 8;
#endif
      printf ("  %s", mount_point);
    }
  putchar ('\n');
}
#endif

/* Identify the directory, if any, that device
   DISK is mounted on, and show its disk usage.  */

#ifdef NEED_UNUSED_FUNCTIONS
static void
show_disk (const char *disk)
{
  int i;

  for (i = 0; i < mount_list_entries; i++)
    if (STREQ (disk, mount_list [i].devname))
      {
	show_dev (mount_list [i].devname, mount_list [i].mountdir,
		  mount_list [i].type);
	return;
      }
  /* No filesystem is mounted on DISK. */
  show_dev (disk, (char *) NULL, (char *) NULL);
}

/* Return the root mountpoint of the filesystem on which FILE exists, in
   malloced storage.  FILE_STAT should be the result of stating FILE.  */
static char *
find_mount_point (const char *file, const struct stat *file_stat)
{
  struct saved_cwd cwd;
  struct stat last_stat;
  char *mp = 0;			/* The malloced mount point path.  */

  if (save_cwd (&cwd))
    return NULL;

  if (S_ISDIR (file_stat->st_mode))
    /* FILE is a directory, so just chdir there directly.  */
    {
      last_stat = *file_stat;
      if (chdir (file) < 0)
	return NULL;
    }
  else
    /* FILE is some other kind of file, we need to use its directory.  */
    {
      int rv;
      char *tmp = g_strdup (file);
      char *dir;

      dir = g_dirname (tmp);
      g_free (tmp);
      rv = chdir (dir);
      g_free (dir);

      if (rv < 0)
	return NULL;

      if (stat (".", &last_stat) < 0)
	goto done;
    }

  /* Now walk up FILE's parents until we find another filesystem or /,
     chdiring as we go.  LAST_STAT holds stat information for the last place
     we visited.  */
  for (;;)
    {
      struct stat st;
      if (stat ("..", &st) < 0)
	goto done;
      if (st.st_dev != last_stat.st_dev || st.st_ino == last_stat.st_ino)
	/* cwd is the mount point.  */
	break;
      if (chdir ("..") < 0)
	goto done;
      last_stat = st;
    }

  /* Finally reached a mount point, see what it's called.  */
  mp = xgetcwd ();

done:
  /* Restore the original cwd.  */
  {
    int save_errno = errno;
    if (restore_cwd (&cwd, 0, mp))
      exit (1);			/* We're scrod.  */
    free_cwd (&cwd);
    errno = save_errno;
  }

  return mp;
}
#endif

/* Figure out which device file or directory POINT is mounted on
   and show its disk usage.
   STATP is the results of `stat' on POINT.  */
#ifdef NEED_UNUSED_FUNCTION
static void
show_point (const char *point, const struct stat *statp)
{
  struct stat disk_stats;
  int i;

  for (i = 0; i < mount_list_entries; i++)
    {
      if (mount_list [i].dev == (dev_t) -1)
	{
	  if (stat (mount_list [i].mountdir, &disk_stats) == 0)
	    mount_list [i].dev = disk_stats.st_dev;
	  else
	    {
	      error (0, errno, "%s", mount_list [i].mountdir);
	      exit_status = 1;
	      /* So we won't try and fail repeatedly. */
	      mount_list [i].dev = (dev_t) -2;
	    }
	}

      if (statp->st_dev == mount_list [i].dev)
	{
	  /* Skip bogus mtab entries.  */
	  if (stat (mount_list [i].mountdir, &disk_stats) != 0 ||
	      disk_stats.st_dev != mount_list [i].dev)
	    continue;
	  show_dev (mount_list [i].devname, mount_list [i].mountdir,
		    mount_list [i].type);
	  return;
	}
    }

  /* We couldn't find the mount entry corresponding to POINT.  Go ahead and
     print as much info as we can; methods that require the device to be
     present will fail at a later point.  */
  {
    /* Find the actual mount point.  */
    char *mp = find_mount_point (point, statp);
    if (mp)
      {
	show_dev (0, mp, 0);
	free (mp);
      }
    else
      error (0, errno, "%s", point);
  }
}
#endif

/* Determine what kind of node PATH is and show the disk usage
   for it.  STATP is the results of `stat' on PATH.  */

#ifdef NEED_UNUSED_FUNCTIONS
static void
show_entry (const char *path, const struct stat *statp)
{
  if (S_ISBLK (statp->st_mode) || S_ISCHR (statp->st_mode))
    show_disk (path);
  else
    show_point (path, statp);
}

/* Show all mounted filesystems, except perhaps those that are of
   an unselected type or are empty. */

static void
show_all_entries (void)
{
  int i;

  for (i = 0; i < mount_list_entries; i++);
    show_dev (mount_list [i].devname, mount_list [i].mountdir,
	      mount_list [i].type);
}
#endif

static gboolean
update_stats (gpointer data)
{
  int i;

  for (i = 0; i < mount_list_entries; i++) 
    {
      if (!excluded_fstype (mount_list [i].type)) 
	{
	  update_dev (mount_list[i].devname, mount_list[i].mountdir,
		      mount_list[i].type, i);
	}
    }
  return TRUE;
}
/* Add FSTYPE to the list of filesystem types to display. */

#ifdef NEED_UNUSED_FUNCTION
static void
add_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = (struct fs_type_list *) g_malloc (sizeof (struct fs_type_list));
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_select_list;
  fs_select_list = fsp;
}
#endif

/* Add FSTYPE to the list of filesystem types to be omitted. */

static void
add_excluded_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;
  g_print ("Add %s to the excluded list \n",fstype);
  fsp = (struct fs_type_list *) g_malloc (sizeof (struct fs_type_list));
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_exclude_list;
  fs_exclude_list = fsp;
}

int
main (int argc, char **argv)
{
  int udp_timer;
  glibtop_mountlist mountlist;


  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init ("gdiskfree", VERSION, argc, argv);

  fs_select_list = NULL;
  fs_exclude_list = NULL;
  inode_format = 0;
  show_all_fs = 0;
  show_listed_fs = 0;
  human_blocks = 1;
  print_type = 0;
  posix_format = 0;
  exit_status = 0;

  add_excluded_fs_type ("proc");

  mount_list = glibtop_get_mountlist (&mountlist, show_all_fs);

  assert (mount_list != NULL);

  mount_list_entries = mountlist.number;
  
  /*  t_dial = (GtkWidget **) g_new (GtkWidget, mount_list_entries); */
  if (require_sync)
    sync ();
  
  build_app_window ();
  update_stats (NULL);
  udp_timer = gtk_timeout_add (1000, update_stats, NULL);
  gtk_main ();
  
  exit (exit_status);
}
