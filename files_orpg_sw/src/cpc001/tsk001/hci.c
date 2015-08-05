/************************************************************************
 *									*
 *	Module:	hci.c							*
 *									*
 *	Description:	This is the top level module for the ORPG	*
 *			Human-Computer-Interface (HCI) RPG Control/     *
 *			Status task.  It is the launching point for	*
 *			all other HCI tasks.				*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/06/17 17:37:31 $
 * $Id: hci.c,v 1.104 2011/06/17 17:37:31 ccalvert Exp $
 * $Revision: 1.104 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>
static int Argc;
static char **Argv;

/************************************************************************
 *	Description: This is the main function for the HCI.		*
 *									*
 *	Input: argc - number of commandline arguments			*
 *	       argv - pointer to commandline arguments data		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void Check_install ()
{
  int argc;
  char **argv;
  int status = -1;
  int adapt_loaded_flag = -1;
  int devices_configured_flag = -1;

  argc = Argc;
  argv = Argv;

  /* Set IP of RPG to query. */

  status = hci_install_info_set_rpg_host( argc, argv );

  if( status < 0 )
  {
    HCI_LE_error( "Unable to set rpg host" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Find out if adaptation data has been installed. */

  adapt_loaded_flag = hci_get_install_info_adapt_loaded();

  if( adapt_loaded_flag < 0 )
  {
    HCI_LE_error( "Unable to determine adaptation data loaded flag" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Find out if hardware devices have been configured. */

  devices_configured_flag = hci_get_install_info_dev_configured();

  if( devices_configured_flag < 0 )
  {
    HCI_LE_error( "Unable to determine hardware configuration flag" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Check flags to determine appropriate action. */

  if( adapt_loaded_flag == HCI_NO_FLAG && devices_configured_flag == HCI_NO_FLAG )
  {
    /* Adaptation data has not been loaded and hardware
       devices have not been configured. Make user install
       adaptation data first. */
    hci_force_adapt_load( argc, &argv[ 0 ] );
  }
  else if( adapt_loaded_flag == HCI_NO_FLAG )
  {
    /* Adaptation data not installed, but hardware devices
       have been configured. Make user install adaptation
       data. */
    hci_force_adapt_load( argc, &argv[ 0 ] );
  }
  else if( devices_configured_flag == HCI_NO_FLAG )
  {
    /* Hardware devices not configured, but adaptation data
       has been installed. Make user configure devices. */
    hci_force_dev_configure( argc, &argv[ 0 ] );
  }
}

int main( int argc, char *argv[] )
{
    int i;

  /* Initialize LE services. */

    LE_set_option( "LE name", HCI_STRING );
    LE_init( argc, argv );

    for (i = 0; i < argc; i++) {
	if (strcmp (argv[i], "-h") == 0 ||
	    strcmp (argv[i], "-S") == 0)
	    break;
    }
    Argc = argc;
    Argv = argv;
    if (i < argc)
	HCI_set_check_install (Check_install);
    else
	Check_install ();

    /* Initialize HCI */
    HCI_init( argc, argv, HCI_TASK );
    /* Initialize callbacks */
    hci_register_callbacks();
    /* Everything is loaded/configured, start up the HCI. */
    hci_control_panel( argc, &argv[ 0 ] );

  return 0;
}

