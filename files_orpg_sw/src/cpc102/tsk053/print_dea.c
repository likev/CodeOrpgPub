
/******************************************************************

    This tool prints the attributes for a given data element.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/06 14:25:08 $
 * $Id: print_dea.c,v 1.9 2005/04/06 14:25:08 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

static char *Dea_id = NULL;
static char *Db_file = NULL;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    static char *attr_names[] = {"id", "name", "type", "range", 
	"value", "unit", "default", "enum", "description", "conversion", 
	"exception", "accuracy", "permission", "baseline", "misc"
    };
    char *db_name;
    int ret, i, de_found;
    DEAU_attr_t *at;

    Dea_id = STR_copy (Dea_id, "");
    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Db_file == NULL) {
	db_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
	if (db_name == NULL) {
	    MISC_log ("ORPGDA_lbname (%d) failed\n", ORPGDAT_ADAPT_DATA);
	    exit (1);
	}
    }
    else
	db_name = Db_file;
    DEAU_LB_name (db_name);

    ret = DEAU_get_attr_by_id (Dea_id, &at);
    if (ret < 0) {
	if (ret != DEAU_DE_NOT_FOUND) {
	    MISC_log ("DEAU_get_attr_by_id (%s) failed (%d)\n", Dea_id, ret);
	    exit (1);
	}
	de_found = 0;
    }
    else
	de_found = 1;
    if (ret < 0) {
	char *p;
	ret = DEAU_get_branch_names (Dea_id, &p);
	if (ret <= 0) {
	    MISC_log ("DE %s not found\n", Dea_id);
	    exit (1);
	}
	if (Dea_id[0] == '\0')
	    printf ("The root node has the following branches:\n");
	else
	    printf ("Node %s has the following branches:\n", Dea_id);
	for (i = 0; i < ret; i++) {
	    printf ("    %s\n", p);
	    p += strlen (p) + 1;
	}
	exit (0);
    }

    for (i = 0; i < DEAU_AT_N_ATS; i++) {
	if (at->ats[i][0] != '\0') {
	    printf ("    %s: %s\n", attr_names[i], at->ats[i]);
	}
    }

    exit (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "f:h?")) != EOF) {
	switch (c) {

	    case 'f':
		Db_file = STR_copy (Db_file, optarg);
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1) {       /* get the id  */
	Dea_id = STR_copy (Dea_id, argv[optind]);
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Prints the attribute of Data Element DE_ID or the branch names \n\
        if DE_ID is a node name. The branch names of the root node is \n\
        printed if DE_ID is not specified.\n\
	Sets the value of Data Element DE_ID.\n\
        Options:\n\
            -f DB_file_name (DEA DB file name. The default is the RPG DEA DB)\n\
            -h (show usage info)\n\
";

    printf ("Usage:  %s [options] [DE_ID]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
