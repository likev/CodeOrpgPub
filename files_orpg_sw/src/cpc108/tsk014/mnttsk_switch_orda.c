
/*************************************************************************

    Initializes the RPG product distribution adaptation/configuration data.

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:53 $
 * $Id: mnttsk_switch_orda.c,v 1.3 2005/12/27 16:41:53 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

/* Constant Definitions/Macro Definitions/Type Definitions */
#define MAX_N_COMMS_LINKS 256

/* Static Global Variables */

/* Static Function Prototypes */
static int Get_command_line_options (int argc, char *argv[], char **action);


/**************************************************************************

    The main function.

**************************************************************************/

int main (int argc, char *argv[]) {
    char *p, *new_names;
    int ret, n_misc_lines, i, rda_link, n_links;
    double npvcs[MAX_N_COMMS_LINKS];
    char *action, *db_name;

    /* Initialize log-error services */
    ORPGMISC_init (argc, argv, 200, 0, -1, 0);

    if ((ret = Get_command_line_options (argc, argv, &action)) < 0)
	exit (1);

    if (action[0] == '\0')
	exit (0);

    db_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (db_name == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	exit (1);
    }
    DEAU_LB_name (db_name);

    /* Update site info in DEA database. */
    ret = 0;
    if (strcmp (action, "orda") == 0)
	ret = DEAU_set_values ( "site_info.is_orda", 1, "Yes", 1, 0 );
    else if (strcmp (action, "rda") == 0)
	ret = DEAU_set_values ( "site_info.is_orda", 1, "No", 1, 0 );
    if (ret < 0) {
        LE_send_msg( GL_ERROR, 
		"DEAU_set_values (site_info.is_orda) failed (%d)\n", ret);
	exit (1);
    }

    /* Update comms info in DEA database. */
    n_misc_lines = DEAU_get_string_values ("comms.misc_spec", &p);
    rda_link = -1;
    for (i = 0; i < n_misc_lines; i++) {
	if (strstr (p, "RDA_link") != NULL &&
	    sscanf (p + strlen ("RDA_link"), "%d", &rda_link) != 1)
	    rda_link = -1;
	p += strlen (p) + 1;
    }
    if (rda_link < 0) {
        LE_send_msg (GL_ERROR, "RDA link not found in comms DEA");
        exit (1);
    }

    n_links = DEAU_get_string_values ("comms.cm_name", &p);
    ret = DEAU_get_values ("comms.n_pvcs", npvcs, MAX_N_COMMS_LINKS);
    if (n_links < 0 || n_links != ret) {
        LE_send_msg (GL_ERROR, 
	"Failed in getting cm_name or p_pvcs from DEA (%d %d)", n_links, ret);
        exit (1);
    }

    new_names = NULL;
    for (i = 0; i < n_links; i++) {
	char *name;
	if (i == rda_link) {
	    if (strcmp (action, "orda") == 0) {
		name = "cm_tcp";
		npvcs[i] = 1;
	    }
	    else if (strcmp (action, "rda") == 0) {
		name = "cm_atlas";
		npvcs[i] = 0;
	    }
	    else
		name = action;
	}
	else
	    name = p;
	new_names = STR_append (new_names, name, strlen (name) + 1);
	p += strlen (p) + 1;
    }
    if ((ret = DEAU_set_values 
			("comms.cm_name", 1, new_names, n_links, 0)) < 0 ||
	(ret = DEAU_set_values ("comms.n_pvcs", 0, npvcs, n_links, 0)) < 0) {
        LE_send_msg (GL_ERROR, 
	"DEAU_set_values comms.cm_name or comms.n_pvcs failed (%d)", ret);
        exit (1);
    }
    STR_free (new_names);

    if (strcmp (action, "rda") == 0)
       LE_send_msg (GL_INFO, "The RPG is set to LEGACY RDA\n");
    else if (strcmp (action, "orda") == 0)
       LE_send_msg (GL_INFO, "The RPG is set to ORDA\n");
    else
       LE_send_msg (GL_INFO, "The WB comms manager is set to %s\n", action);
    exit (0);
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP or RESTART)

   Returns:
      exits on error, or returns 0 on success.

*****************************************************************************/

static int Get_command_line_options (int argc, char *argv[], char **action) {
    extern char *optarg;
    extern int optind;
    int c, err;

    *action = "";
    err = 0;
    while ((c = getopt (argc, argv, "ht:")) != EOF) {

	switch (c) {

	    case 't':
		*action = optarg;
		break;

	    case 'h':
	    case '?':
	    default:
		err = 1;
		break;
	}
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s [options]\n", MISC_string_basename (argv [0]));
	printf ("    Change WB comms adaptation.\n");
	printf ("    options:\n");
	printf ("    -t orda: Change RPG adaptation to ORDA\n");
	printf ("    -t rda: Change RPG adaptation to legacy RDA\n");
	printf ("    -t cm_name: Change WB comms manager name to cm_name\n");
	printf ("    -h (print usage msg and exit)\n");
	exit (1);
    }

    return (0);
}
