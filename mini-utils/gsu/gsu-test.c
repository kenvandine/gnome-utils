#include <gsu-lib.h>

int
main (int argc, char **argv)
{
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init (PACKAGE, VERSION, argc, argv);

	gsu_call_helper (argc, argv);
}
