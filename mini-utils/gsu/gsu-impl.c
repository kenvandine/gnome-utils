#include <gsu-impl.h>
#include <gsu-lib.h>

#include <errno.h>

#ifndef max
#define max(a,b) ((a > b) ? a : b)
#endif

typedef struct {
    POA_GNOME_Su servant;
    PortableServer_POA poa;
} impl_POA_GNOME_Su;

/*** Implementation stub prototypes ***/

static void impl_GNOME_Su__destroy (impl_POA_GNOME_Su * servant,
				    CORBA_Environment * ev);

static void impl_GNOME_Su_su_command (GNOME_Su _obj, const CORBA_char * user,
				      const CORBA_char * command,
				      const GNOME_Su_arg_seq * args,
				      CORBA_Environment * ev);

/*** epv structures ***/

static PortableServer_ServantBase__epv impl_GNOME_Su_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_GNOME_Su__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

static POA_GNOME_Su__epv impl_GNOME_Su_epv =
{
    NULL,			/* _private */
    &impl_GNOME_Su_su_command,
};

/*** vepv structures ***/

static POA_GNOME_Su__vepv impl_GNOME_Su_vepv =
{
    &impl_GNOME_Su_base_epv,
    &impl_GNOME_Su_epv,
};

/*** Stub implementations ***/

GNOME_Su
impl_GNOME_Su__create (PortableServer_POA poa, CORBA_Environment * ev)
{
    GNOME_Su retval;
    impl_POA_GNOME_Su *newservant;
    PortableServer_ObjectId *objid;

    newservant = g_new0 (impl_POA_GNOME_Su, 1);
    newservant->servant.vepv = &impl_GNOME_Su_vepv;
    newservant->poa = poa;
    POA_GNOME_Su__init ((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object (poa, newservant, ev);
    CORBA_free (objid);
    retval = PortableServer_POA_servant_to_reference (poa, newservant, ev);
    
    return retval;
}

/* You shouldn't call this routine directly without first deactivating the
 * servant... */
static void
impl_GNOME_Su__destroy (impl_POA_GNOME_Su * servant, CORBA_Environment * ev)
{
    POA_GNOME_Su__fini ((PortableServer_Servant) servant, ev);
    g_free (servant);
}

static void
impl_GNOME_Su_su_command (GNOME_Su _obj,
			  const CORBA_char * user,
			  const CORBA_char * command,
			  const GNOME_Su_arg_seq * args,
			  CORBA_Environment * ev)
{
    gchar *helper_pathname = gsu_get_helper (), *password;
    int passwd_pipe [2], message_pipe [2], error_pipe [2];
    gchar passwd_pipe_str [10], message_pipe_str [10];
    gchar error_pipe_str [10], message [BUFSIZ];
    gchar *error_message = NULL;
    int len, max_fd, retval;
    fd_set rdfds;
    pid_t pid;

    if (!helper_pathname)
	goto no_helper;

    if (pipe (passwd_pipe))
	goto no_helper;

    if (pipe (message_pipe))
	goto no_helper;

    if (pipe (error_pipe))
	goto no_helper;

    sprintf (passwd_pipe_str, "%d", passwd_pipe [0]);
    sprintf (message_pipe_str, "%d", message_pipe [1]);
    sprintf (error_pipe_str, "%d", error_pipe [1]);

    password = gsu_getpass (user);
    if (!password)
	goto user_abort;

    /* Let's fork () a child ... */

    pid = fork ();
    if (pid < 0)
	goto no_helper;

    if (pid) {
	memset (password, 0, strlen (password));

	close (passwd_pipe [1]);
	close (message_pipe [0]);
	close (error_pipe [0]);

	execl (helper_pathname, "gsu", "-c", "xterm",
	       passwd_pipe_str, error_pipe_str,
	       message_pipe_str, NULL);
	
	goto no_helper;
    }

    /* We are the child. */

    len = strlen (password)+1;
		
    close (passwd_pipe [0]);
    close (message_pipe [1]);
    close (error_pipe [1]);

    if (write (passwd_pipe [1], &len, sizeof (len)) != sizeof (len)) {
	error_message = g_strdup_printf ("writing password length: %s",
					 strerror (errno));
	goto unknown_error;
    }

    if (write (passwd_pipe [1], password, len) != len) {
	error_message = g_strdup_printf ("writing password: %s",
					 strerror (errno));
	goto unknown_error;
    }

    close (passwd_pipe [1]);

 select_loop:

    FD_ZERO (&rdfds);
    FD_SET (message_pipe [0], &rdfds);
    FD_SET (error_pipe [0], &rdfds);

    max_fd = max (message_pipe [0], error_pipe [0]);

    fprintf (stderr, "Starting select on %d and %d ...\n",
	     message_pipe [0], error_pipe [0]);

    retval = select (max_fd+1, &rdfds, NULL, NULL, NULL);

    if (retval == -1) {
	error_message = g_strdup_printf ("select: %s", strerror (errno));
	goto unknown_error;
    }

    fprintf (stderr, "select: %d - %s - %d - %d\n",
	     retval, strerror (errno),
	     FD_ISSET (message_pipe [0], &rdfds),
	     FD_ISSET (error_pipe [0], &rdfds));

    if (FD_ISSET (error_pipe [0], &rdfds)) {
	retval = read (error_pipe [0], &len, sizeof (len));

	fprintf (stderr, "error read: %d - %d\n", retval, len);
	
	if (!retval)
	    fprintf (stderr, "SU OK - error pipe!\n");
    }

    if (FD_ISSET (message_pipe [0], &rdfds)) {
	retval = read (message_pipe [0], &len, sizeof (len));

	if (retval != sizeof (len)) {
	    error_message = g_strdup_printf
		("read (message pipe): %s", strerror (errno));
	    goto unknown_error;
	}

	if (message_pipe [0] == error_pipe [0]) {
	    /* Return success. */
	    close (message_pipe [0]);
	    return;
	} else {
	    /* Close message pipe and make it point to the error pipe */
	    close (message_pipe [0]);
	    message_pipe [0] = error_pipe [0];
	    goto select_loop;
	}
    }

    if (len+1 > BUFSIZ) {
	error_message = g_strdup ("result too large");
	goto unknown_error;
    }
    
    if (read (message_pipe [0], message, len) != len) {
	error_message = g_strdup_printf
	    ("read (message pipe): %s", strerror (errno));
	goto unknown_error;
    }

    if (!strcmp (message, "incorrect password"))
	goto incorrect_password;

    error_message = g_strdup (message);
    close (message_pipe [0]);
    goto unknown_error;

 no_helper:
    {
	GNOME_Su_NoSuidHelper *exception =
	    GNOME_Su_NoSuidHelper__alloc ();
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_GNOME_Su_NoSuidHelper, exception);
	return;
    }

 user_abort:
    {
	GNOME_Su_UserAbort *exception =
	    GNOME_Su_UserAbort__alloc ();
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_GNOME_Su_UserAbort, exception);
	return;
    }

 incorrect_password:
    {
	GNOME_Su_IncorrectPassword *exception =
	    GNOME_Su_IncorrectPassword__alloc ();
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_GNOME_Su_IncorrectPassword, exception);
	return;
    }

 unknown_error:
    {
	GNOME_Su_UnknownError *exception =
	    GNOME_Su_UnknownError__alloc ();
	exception->message = CORBA_string_dup (error_message);
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_GNOME_Su_UnknownError, exception);
	return;
    }
}
