#include <config.h>
#include <gnome.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/types.h>
#include <linux/hdreg.h>

#define APPNAME "idetool"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#ifndef VERSION
#define VERSION "0.0.1"
#endif

static char *bit32_names[]=
{
	"Default",
	"16-bit",
	"32-bit",
	"32-bit w/sync"
};

static char *power_names[]={
	"Unknown",
	"Sleeping",
	"Active/Idle",
	"Standby"
};

static int get_power_mode(int fd)
{
	unsigned char args[4] = {0x98, 0, 0 , 0};
	
	if(ioctl(fd, HDIO_DRIVE_CMD, &args))
	{
		if(errno!=EIO || args[0]!=0 || args[1]!=0)
			return 0;
		return 1;
	}
	else
	{
		if(args[2]==255)
			return 2;
		else
			return 3;
	}
}

static char *make_buffers(struct hd_driveid *hd)
{
	static char buf[256];
	char *n="";
	if(hd->buf_size==0)
		return("None");
	switch(hd->buf_type)
	{
		case 1:
			n=_("(1 Sector)");break;
		case 2:
			n=_("(Dual Port)");break;
		case 3:
			n=_("(Dual Port Cache)");break;
	}
	sprintf(buf, "%dKb %s\n", hd->buf_size/2, n);
	return buf;
}

static char *make_geom(struct hd_driveid *hd)
{
	static char buf[256];
	sprintf(buf,"%d/%d/%d", hd->cyls, hd->heads, hd->sectors);
	return buf;
}

static void clist_add(GtkCList *cl, char *a, char *b, int blen)
{
	char buf[256];
	const gchar *ptr[2];
	memcpy(buf, b, blen);
	buf[blen]=0;
	ptr[0]=a;
	ptr[1]=buf;
	gtk_clist_append(cl, (gchar **)ptr);
}

static char *make_multi(int n)
{
	static char buf[128];
	if(n)
		sprintf(buf, "Yes (%d sector%s)", n, n==1?"":"s");
	else
		sprintf(buf, "No");
	return buf;
}


static void ide_stat_drive(char *drive, int fd, GtkWidget *notebook)
{
	int bits32, keepsetting, mask, multi, dma;
	int id=1;
	struct hd_driveid hd;
	int pmode;
	GtkWidget *vbox;
	GtkCList *cl;
	char *titles[2]= { _("Setting"), _("Value") };
	
	/*
	 *	Process the drive.
	 */
	 
	if(ioctl(fd, HDIO_GET_32BIT, &bits32)==-1)
		perror("32bit mode");
	if(ioctl(fd, HDIO_GET_KEEPSETTINGS, &keepsetting)==-1)
		perror("keep settings");
	if(ioctl(fd, HDIO_GET_UNMASKINTR, &mask)==-1)
		perror("mask");
	if(ioctl(fd, HDIO_GET_MULTCOUNT, &multi)==-1)
		perror("multi");
	if(ioctl(fd, HDIO_GET_DMA, &dma)==-1)
		perror("dma");
		
	/*
	 *	See what the drive state is
	 */
	 
	pmode=get_power_mode(fd);
	
	memset(&hd, 0, sizeof(hd));
	if(ioctl(fd, HDIO_GET_IDENTITY, &hd)==-1)
	{
		id=0;
	}
	
	/*
	 *	Add this drive to the notebook
	 */
	 
	vbox=gtk_vbox_new(FALSE, GNOME_PAD);
	gtk_widget_show(vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(drive));
	cl=GTK_CLIST(gtk_clist_new(2));
	gtk_clist_set_selection_mode(cl, GTK_SELECTION_BROWSE);
	gtk_clist_set_policy(cl, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_clist_column_titles_passive(cl);
	gtk_clist_set_column_width(cl, 0, 70);
	gtk_clist_freeze(cl);
	gtk_widget_set_usize(GTK_WIDGET(cl), -1, 200);
	gtk_clist_set_border(cl, GTK_SHADOW_IN);
	
	if(id)
	{
		clist_add(cl, _("Model"), hd.model, 40);
		clist_add(cl,_("Firmware"), hd.fw_rev, 8);
		clist_add(cl,_("Serial No."), hd.serial_no, 8);
		/* do config here */
		clist_add(cl,_("Geometry"), make_geom(&hd), 255);
		clist_add(cl,_("Cache"), make_buffers(&hd),255);
		clist_add(cl,_("Status"), power_names[pmode],255);
	}
	else
	{
		clist_add(cl, _("Model"), _("Legacy MFM/RLL drive"), 40);
	}
	
	clist_add(cl,_("DMA Mode"), dma?_("Enabled"):_("Disabled"),255);
	clist_add(cl,_("IO Mode"), bit32_names[bits32], 255);
	clist_add(cl,_("IRQ Unmask"), mask?_("Enabled"):_("Disabled"), 255);
	clist_add(cl,_("Multisector"), make_multi(multi),255);
	clist_add(cl,_("On Reset"), keepsetting?_("Keep settings"):_("Default"),255);
	
	gtk_clist_thaw(cl);
	gtk_container_add(GTK_CONTAINER(vbox), GTK_WIDGET(cl));
	gtk_widget_show(GTK_WIDGET(cl));
	gtk_widget_show(vbox);
}

static int ide_parser(void)
{
	char i;
	GtkWidget *toplevel;
	GtkWidget *tbox;
	GtkWidget *pixmap;
	GtkWidget *notebook;
	unsigned char *pic;
	int drives=0;
	
	/*
	 *	Set up the outer display first
	 */
	
	
	toplevel = gnome_dialog_new(_("IDE Status"), 
			GNOME_STOCK_BUTTON_OK, NULL);
	gnome_dialog_set_close(GNOME_DIALOG(toplevel), TRUE);
	
	tbox=gtk_hbox_new(TRUE, GNOME_PAD);
	
	pic=gnome_pixmap_file("ide-disk-drive.xpm");
	if(pic)
	{
		pixmap = gnome_pixmap_new_from_file(pic);
		g_free(pic);
	}
	if(pixmap==NULL || pic==NULL)
		pixmap = gtk_label_new(_("Disk Logo Not Found"));
	
	gtk_box_pack_start(GTK_BOX(tbox), pixmap, TRUE, TRUE, GNOME_PAD);
	
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(toplevel)->vbox), tbox,
		FALSE, FALSE, GNOME_PAD);
		
	
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(toplevel)->vbox), notebook,
		TRUE, TRUE, GNOME_PAD);
	
	gtk_widget_show(pixmap);
	gtk_widget_show(tbox);
	gtk_widget_show(notebook);
	
	for(i='a'; i<='h';i++)
	{
		char buf[128];
		int fd;
		
		sprintf(buf, "/dev/hd%c", i);
		fd=open(buf,O_RDONLY);
		if(fd==-1)
		{
			continue;
		}
		ide_stat_drive(buf+5, fd, notebook);
		drives++;
		close(fd);
	}
	gtk_signal_connect(GTK_OBJECT(toplevel), "close",
		GTK_SIGNAL_FUNC(gtk_main_quit),
		NULL);

	if(drives)		
		gtk_widget_show(toplevel);
	return drives;
}


int main(int argc, char *argv[])
{
	argp_program_version = VERSION;

	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init (APPNAME, 0, argc, argv, 0, 0);

	if(geteuid())
	{
		GtkWidget *w=gnome_message_box_new(_("Only the superuser can use this tool"),
					GNOME_MESSAGE_BOX_ERROR,
					GNOME_STOCK_BUTTON_OK, NULL);
		gtk_signal_connect(GTK_OBJECT(w), "destroy", 
				GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
		gtk_widget_show(w);
		gtk_main();
		exit(1);
	}
	if(ide_parser()==0)
	{
		GtkWidget *w=gnome_message_box_new(_("I can find no IDE drives"),
					GNOME_MESSAGE_BOX_ERROR,
					GNOME_STOCK_BUTTON_OK, NULL);
		gtk_signal_connect(GTK_OBJECT(w), "destroy", 
				GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
		gtk_widget_show(w);
		gtk_main();
		exit(1);
	}
	gtk_main();
	exit(EXIT_SUCCESS);
}

