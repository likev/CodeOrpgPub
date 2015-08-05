
/******************************************************************

    This tool prints the attributes for a given data element.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 14:01:17 $
 * $Id: edit_dea.c,v 1.5 2014/03/25 14:01:17 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

#define TOK_BUF_SIZE 128

static char *Dea_id = NULL;
static char *Db_file = NULL;
static char *New_value = NULL;
static char *Permission = NULL;
static int Value_ind = 0;
static int Update_baseline = 0;
static int N_rm_ids = 0;
static int N_add_files = 0;
static char *Rm_ids = NULL;
static char *Add_files = NULL;
static char U_value[TOK_BUF_SIZE];
static int U_type = 0;

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

    if (N_rm_ids > 0 || N_add_files > 0) {
	DEAU_special_suffix ("alg");
	ret = DEAU_rm_add_des (N_add_files, Add_files, N_rm_ids, Rm_ids);
	if (ret < 0) {
	    MISC_log ("DEAU_rm_add_des failed (%d)\n", ret);
	    exit (1);
	}
	exit (0);
    }

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

    if (Permission != NULL && de_found) {

       /* Valid permissions are "AGENCY", "ROC" and "URC".  Permissions
          should be entered delimited by commas.  */
       if( strlen( Permission ) != 0 ) {

          char delim[] = ",";
          char attr[128];
          char *result = NULL, *perm = NULL;

	  perm = STR_copy (perm, Permission);
          result = strtok (perm, delim);
          if (result == NULL) {
             MISC_log ("strtok (%s) (%s) Failed\n", Permission, delim);
             exit (1);
          }
          while (result != NULL){
             if ((strcmp (result, "AGENCY") != 0)
                                &&
                 (strcmp (result, "URC") != 0)
                                &&
                 (strcmp (result, "ROC") != 0)) {
                MISC_log ("Bad Permission: %s\n", result);
                exit(1);        
          
             }
             result = strtok (NULL, delim);
          }
          
          memset (attr, 0, 128);
          sprintf (attr, "[%s]", Permission);
          DEAU_update_attr (Dea_id, DEAU_AT_PERMISSION, attr);
       }
    }

    if (U_type > 0 && de_found) {
	ret = DEAU_update_attr (Dea_id, U_type, U_value);
	if (ret < 0) {
             MISC_log ("Update attr (%d,%s) failed (return %d)\n",
						U_type, U_value, ret);
             exit (1);
	}
    }

    if (New_value != NULL && de_found) {
	char *buf, *strs;
	int type, n, upd_size;
	double dvs[1024 + 1];

	buf = NULL;
	type = DEAU_get_data_type (at);
	if (type == DEAU_T_STRING)
	    n = DEAU_get_string_values (Dea_id, &strs);
	else
	    n = DEAU_get_values (Dea_id, dvs, 1024);
	if (n < 0) {
	    MISC_log ("DEAU_get_values (%s) failed (%d)\n", Dea_id, n);
	    exit (1);
	}
	if (Value_ind > n) {
	    MISC_log ("Array index (%d) is not valid\n", Value_ind);
	    exit (1);
	}
	upd_size = n;
	if (Value_ind >= n)
	    upd_size = n + 1;

	if (type == DEAU_T_STRING) {
	    int i;
	    for (i = 0; i < upd_size; i++) {
		if (i != Value_ind)
		    buf = STR_append (buf, strs, strlen (strs) + 1);
		else
		    buf = STR_append (buf, New_value, strlen (New_value) + 1);
		strs += strlen (strs) + 1;
	    }
	    ret = DEAU_set_values (Dea_id, 1, buf, upd_size, 0);
	    if (Update_baseline && ret == 0)
		DEAU_set_values (Dea_id, 1, buf, upd_size, 1);
	}
	else {
	    double d;
	    char c;
	    if (sscanf (New_value, "%lf%c", &d, &c) != 1) {
		MISC_log ("The new value (%d) is not numerical\n", New_value);
		exit (1);
	    }
	    dvs[Value_ind] = d;
	    ret = DEAU_set_values (Dea_id, 0, dvs, upd_size, 0);
	    if (Update_baseline && ret == 0)
		DEAU_set_values (Dea_id, 0, dvs, upd_size, 1);
	}
	if (ret < 0) {
	    if (ret == DEAU_BAD_RANGE)
		MISC_log ("DEAU_set_values failed - \"%s\" is not valid\n", 
							New_value);
	    else
		MISC_log ("DEAU_set_values (%s) failed (%d)\n", 
							New_value, ret);
	    exit (1);
	}
	if (Value_ind > 0)
	    printf ("%d-th value of %s set to %s\n", 
					Value_ind, Dea_id, New_value);
	else
	    printf ("Value of %s set to %s\n", Dea_id, New_value);
    }

    ret = DEAU_get_attr_by_id (Dea_id, &at);
    if (ret < 0) {
	MISC_log ("Unexpected DEAU_get_attr_by_id failed (%d)\n", ret);
	exit (1);
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
    char *u_str;

    err = 0;
    u_str = NULL;
    while ((c = getopt (argc, argv, "f:n:i:bp:r:a:u:h?")) != EOF) {
	switch (c) {

	    case 'f':
		Db_file = STR_copy (Db_file, optarg);
		break;

	    case 'p':
		Permission = STR_copy (Permission, optarg);
		break;

	    case 'n':
		New_value = STR_copy (New_value, optarg);
		break;

	    case 'b':
		Update_baseline = 1;
		break;

	    case 'i':
		if (sscanf (optarg, "%d", &Value_ind) != 1 ||
		    Value_ind < 0) {
		    MISC_log ("Bad -i option\n");
		    Print_usage (argv);
		}
		break;

	    case 'r':
		Rm_ids = STR_append (Rm_ids, optarg, strlen (optarg) + 1);
		N_rm_ids++;
		break;

	    case 'a':
		Add_files = STR_append (Add_files, optarg, strlen (optarg) + 1);
		N_add_files++;
		break;

	    case 'u':
		u_str = optarg;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (u_str != NULL) {
	char type[TOK_BUF_SIZE];
	int ret = MISC_get_token (u_str, "S,", 0, type, TOK_BUF_SIZE);
	if (ret <= 0) {
	    MISC_log ("Bad -u option\n");
	    Print_usage (argv);
	}
	strncpy (U_value, u_str + ret, TOK_BUF_SIZE);
	U_value[TOK_BUF_SIZE - 1] = '\0';
	if (strcmp (type, "range") == 0)
	    U_type = DEAU_AT_RANGE;
	else if (strcmp (type, "unit") == 0)
	    U_type = DEAU_AT_UNIT;
	else if (strcmp (type, "default") == 0)
	    U_type = DEAU_AT_DEFAULT;
	else if (strcmp (type, "enum") == 0)
	    U_type = DEAU_AT_ENUM;
	else if (strcmp (type, "description") == 0)
	    U_type = DEAU_AT_DESCRIPTION;
	else if (strcmp (type, "conversion") == 0)
	    U_type = DEAU_AT_CONVERSION;
	else if (strcmp (type, "exception") == 0)
	    U_type = DEAU_AT_EXCEPTION;
	else if (strcmp (type, "accuracy") == 0)
	    U_type = DEAU_AT_ACCURACY;
	else if (strcmp (type, "permission") == 0)
	    U_type = DEAU_AT_PERMISSION;
	else {
	    MISC_log ("Bad attribute type (%s) in -u option\n", type);
	    Print_usage (argv);
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
            -p Permission (DEA Element LOCA. The default is no change)\n\
            -n new_value (Sets value to new_value. The default is no change)\n\
            -i index (Sets index-th element in the value array to new_value.\n\
               The default is 0)\n\
            -b (Also update baseline value if the value is set)\n\
            -r id (Removes DE of id - can be used for multiple ids)\n\
            -a file (Adds DEs in file to the DEA DB - can be used for multiple\n\
               files)\n\
            -u type,value (Updates the value of attribute \"type\" to \"value\").\n\
               \"type\" can be any of the following: range, unit, default, enum,\n\
               description, conversion, exception, accuracy or permission. \n\
               \"value\" is a text string which must be in the format used in\n\
               DEA file (after =).\n\
            -h (show usage info)\n\
";

    printf ("Usage:  %s [options] [DE_ID]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
