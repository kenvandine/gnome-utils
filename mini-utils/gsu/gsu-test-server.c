#include <gsu-lib.h>
#include <libgnorba/gnorba.h>
#include <gsu.h>

static gchar *user = NULL;

const struct poptOption options [] = {
    { "user", 'u', POPT_ARG_STRING, &user, 0,
      N_("Target User"), N_("USER") },
    {NULL, '\0', 0, NULL, 0}
};

void
Exception (CORBA_Environment * ev)
{
    switch (ev->_major)	{
    case CORBA_SYSTEM_EXCEPTION:
	g_log ("GSU-Test", G_LOG_LEVEL_DEBUG, "CORBA system exception %s.\n",
	       CORBA_exception_id (ev));
	exit (1);
    case CORBA_USER_EXCEPTION:
	g_log ("GSU-Test", G_LOG_LEVEL_DEBUG, "CORBA user exception: %s.\n",
	       CORBA_exception_id (ev));
	exit (1);
    default:
	break;
    }
}

int
main (int argc, char* argv[])
{
    CORBA_ORB orb;
    CORBA_Environment ev;
    CORBA_Object gsu_server;
    GNOME_Su_arg_seq *args = NULL;
    gchar **cargv, **temp;
    poptContext ctx;
    gint cargc;
    int i;
  
    CORBA_exception_init (&ev);
  
    orb = gnome_CORBA_init_with_popt_table
	("gsu-test-server", VERSION, &argc, argv, options, 0, &ctx, 0, &ev);
    Exception (&ev);

    cargv = poptGetArgs (ctx);
    for (temp = cargv, cargc = 0; *temp; temp++, cargc++) ;

    if (cargc < 1) {
	fprintf (stderr, "Must give a command to execute.\n");
	exit (1);
    }

    gsu_server = goad_server_activate_with_repo_id
	(0, "IDL:GNOME/Su:1.0", GOAD_ACTIVATE_REMOTE, NULL);

    if (gsu_server == CORBA_OBJECT_NIL) {
	fprintf (stderr, "Cannot activate server\n");
	exit (1);
    }

    fprintf (stderr,"gsu-test-server gets server at IOR='%s'\n",
	     CORBA_ORB_object_to_string (orb, gsu_server, &ev));

    args = GNOME_Su_arg_seq__alloc ();
    args->_buffer = CORBA_sequence_CORBA_string_allocbuf (cargc-1);

    for (i = 1; i < cargc; i++)
	args->_buffer [i-1] = cargv [i];

    GNOME_Su_su_command (gsu_server, user, cargv [0], args, &ev);
    Exception (&ev);

    GNOME_Su_arg_seq__free (args, NULL, FALSE);

    poptFreeContext (ctx);

    return 0;
}
