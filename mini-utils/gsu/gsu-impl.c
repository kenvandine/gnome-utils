#include <gsu-impl.h>

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
    fprintf (stderr, "impl_GNOME_Su_su_command (%s - %s)\n",
	     user, command);
}
