/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/04/15 19:25:11 $
 * $Id: hci_init_config.c,v 1.8 2009/04/15 19:25:11 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include <hci.h>

#define	HCI_PASSWORD_LEN	HCI_BUF_64

/**************************************************************************
 Description: Read the command-line options
              variables.
       Input: argc, argv,

      Output: none
     Returns: > 0 upon success, 0 otherwise
     Globals: Prog_name
       Notes:
 **************************************************************************/
static int Read_options(int argc, char **argv)
{
	int retval = 1;
	int input;

	while ((input = getopt(argc,argv,"h")) != -1)
	{
      	    switch(input)
	    {
		default:
			retval = 0;
			printf ("\n\tUsage:\t%s [options]\n",argv[0]);
			printf ("\n\tDescription:\n");
			printf ("\n\t\tInitialize hci configuration information\n");
			printf ("\n\tOptions:\n\n");
			printf ("\t\t-h \tprint usage information\n");
			HCI_task_exit( HCI_EXIT_SUCCESS );
	    }

	    if (!retval)
	       break;
        }

       return(retval);
       /*END of Read_options()*/
}

void Err_func(char* msg)
{
 	HCI_LE_error(msg);
}


/*
	Initialize hci passwords from a passwords configuration file
*/
int main (int argc, char** argv) {
    char *pwd;
    char urcbuf[HCI_PASSWORD_LEN];
    char rocbuf[HCI_PASSWORD_LEN];
    char abuf[HCI_PASSWORD_LEN];
    int ret;

    LE_set_option( "LE name", HCI_STRING );
    LE_set_option( "LB size", HCI_NUM_LE_LOG_MSGS );
    LE_init( argc, argv );

    ORPGMISC_deau_init();

    if (Read_options(argc, argv) < 0)
	HCI_task_exit( HCI_EXIT_SUCCESS );

    CS_error ((void (*)())Err_func);

    CS_cfg_name ("hci_passwords");
    CS_control (CS_COMMENT | '#');

    CS_entry ("Passwords", 0, 0, NULL);
    if (CS_level (CS_DOWN_LEVEL) < 0) {
	HCI_LE_error("Password section is missing from the hci_passwords file");
	HCI_task_exit( HCI_EXIT_FAIL );
    }

    if (CS_entry ("agency_password", 1, HCI_PASSWORD_LEN, (void*)abuf) <= 0) {
	HCI_LE_error("agency_password missing from hci_passwords file");
	HCI_task_exit( HCI_EXIT_FAIL );
    }
    if (CS_entry("roc_password", 1, HCI_PASSWORD_LEN, (void*)rocbuf) <= 0) {
	HCI_LE_error("roc_password missing from hci_passwords file");
	HCI_task_exit( HCI_EXIT_FAIL );
    }
    if (CS_entry ("urc_password", 1, HCI_PASSWORD_LEN, (void *)urcbuf) <= 0) {
	HCI_LE_error("urc_password missiong from hci_passwords file");
	HCI_task_exit( HCI_EXIT_FAIL );
    }
    CS_cfg_name("");

    /* encrypt the passwords before writing them to file. */

    if( HCI_urc_password( &pwd ) <= 0 || pwd == NULL )
    {
      pwd = ORPGMISC_crypt( urcbuf );
      if( pwd == NULL )
      {
        HCI_LE_error( "ORPGMISC_crypt(%s) failed", urcbuf );
        HCI_task_exit( HCI_EXIT_FAIL );
      }
      else
      {
        if( ( ret = HCI_set_urc_password( pwd ) ) < 0 )
        {
          HCI_LE_error("Unable to set URC password (%d)", ret);
          HCI_task_exit( HCI_EXIT_FAIL );
        }
      }
    }

    if( HCI_roc_password( &pwd ) <= 0 || pwd == NULL)
    {
      pwd = ORPGMISC_crypt( rocbuf );
      if( pwd == NULL )
      {
        HCI_LE_error( "ORPGMISC_crypt(%s) failed", rocbuf );
        HCI_task_exit( HCI_EXIT_FAIL );
      }
      else
      {
        if( ( ret = HCI_set_roc_password( pwd ) ) < 0 )
        {
          HCI_LE_error("Unable to set ROC password (%d)", ret);
          HCI_task_exit( HCI_EXIT_FAIL );
        }
      }
    }

    if( HCI_agency_password( &pwd ) <= 0 || pwd == NULL)
    {
      pwd = ORPGMISC_crypt( abuf );
      if( pwd == NULL )
      {
        HCI_LE_error( "ORPGMISC_crypt(%s) failed", abuf );
        HCI_task_exit( HCI_EXIT_FAIL );
      }
      else
      {
        if( ( ret = HCI_set_agency_password( pwd ) ) < 0 )
        {
          HCI_LE_error("Unable to set AGENCY password (%d)", ret);
          HCI_task_exit( HCI_EXIT_FAIL );
        }
      }
    }

    HCI_LE_log("HCI passwords initialized successfully");

    return 0;
}

