/*
 * gnome-exe-handler.c: Invoked when the user double clicks on an ELF
 * file, we need to figure out what to do with it.
 *
 * Author:
 *   Miguel de Icaza (miguel@ximian.com)
 */
#include <config.h>
#include <gnome.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static int
message (char *msg, int do_yesno)
{
	GtkWidget *box;
	char *b1, *b2;
	
	if (do_yesno){
		b1 = GNOME_STOCK_BUTTON_YES;
		b2 = GNOME_STOCK_BUTTON_NO;
	} else {
		b1 = GNOME_STOCK_BUTTON_OK;
		b2 = NULL;
	}
	
	box = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_ERROR, b1, b2, NULL);
	return gnome_dialog_run (GNOME_DIALOG (box));
}

static void
error (char *msg)
{
	message (msg, 0);
	exit (1);
}

void execute (char *file)
{
	char *msg = g_strdup_printf (
		_("Executing arbitrary programs that you downloaded from the network"
		  "might be dangerous.\n\nAre you sure you want to run `%s'?"), file);
	struct stat s;
	
	switch (message (msg, 1)){
	case 0:
		if (stat (file, &s) == -1){
			error (_("Could not access file permissions"));
		}
		
		if (chmod (file, s.st_mode | 00100) == -1){
			msg = g_strdup_printf (
				_("Could not execute the file `%s' due to a permission problem"),
				file);
			
			error (msg);
		}

		{
			char *argv [2];

			argv [0] = file;
			argv [1] = NULL;

			
			execv (file, argv);
		}

		msg = g_strdup_printf (_("Failure at executing `%s'"), file);
		error (msg);
		
	case 1:
	case -1:
		exit (0);
	}
}

static void
launch (char *file)
{
	char buffer [64];
	int n;
	int fd = open (file, O_RDONLY);
	
	if (fd == -1){
		char *msg = g_strdup_printf (
			_("It was not possible to open the file `%s'"),
			file);
		
		error (msg); 
	}

	n = read (fd, &buffer, sizeof (buffer));

	if (n == 0){
		char *msg = g_strdup_printf (_("The file `%s' is empty"), file);
		error (msg);
	}

	if (n < 0){
		char *msg = g_strdup_printf (_("There was an error `%s' is empty"), file);
		error (msg);
	}

	if (buffer [0] == 0x7f &&
	    buffer [1] == 'E' &&
	    buffer [2] == 'L' &&
	    buffer [3] == 'F'){
		unsigned char processor;
		
		/* We got an ELF file, check to see what kind it is */

		switch (buffer [16]){
		case 1:
		case 0:
			error (_("This is library and can not be executed"));
			
		case 2:
			break;
			
		case 4:
			error (_("The file is a core file and can not be executed"));

			
		default:
			error (_("This is an unknown kind of executable"));
		}

		/*
		 * This handles executables
		 */
		processor = buffer [18];

		if (
#if defined(__i386__)
			(processor == 3) ||
#elif defined(sparc) || defined(__sparc__) || defined(__sparcv9__)
			(processor == 2) || (processor == 18) || (processor == 43) ||
#elif defined(parisc) || defined (__parisc__)
			(processor == 15) ||
#elif defined(ppc) || defined(__ppc__)
			(processor == 20) ||
#elif defined(__superh__)
			(processor == 42) ||
#elif defined(__ia64__)
			(processor == 50) ||
#endif
			0){
			execute (file);
		}
		else
			error (_("The executable is for a different platform and can not be executed on this system"));

	}
	error (_("This is an unknown kind of executable"));
}

int
main (int argc, char *argv [])
{
	poptContext ctx;
	char **files;
	
	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init_with_popt_table ("gnome-exe-handler", VERSION, argc, argv, NULL, 0, &ctx);

	files = poptGetArgs (ctx);

	while (files && *files){
		launch (*files);
		files++;
	}
	
	return 0;
}
