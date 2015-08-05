
/******************************************************************

	This is a RPG product query tool.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/16 20:55:08 $
 * $Id: user_query.c,v 1.5 2007/03/16 20:55:08 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <orpg.h> 
#include <infr.h> 
#include <orpgerr.h>

#define DB_NAME_SIZE 128

static int Message_id; 
static short User_id;
static char User_name[DB_NAME_SIZE]; 
static short Up_type;
static short Class;
static short Line_ind;

static char *Sql_text = NULL;
static char *Db_file = NULL;
static int Detailed_info;

static void Print_usage (char *argv[]);
static int Read_options (int argc, char *argv[]);
static void Print_query_results (void *results);
static void Print_user_profile (char *buf, int len);
static char *Print_parameters (short *params);
static void Add_section (char *str, char **vb);
static void Print_db_records ();


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char *argv[]) {
    void *qr;			/* query results */
    int ret, set;
    char buf[256], *vb, *lb_name;

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (0);

    if (Db_file != NULL)
	Print_db_records ();

    lb_name = ORPGDA_lbname (ORPGDAT_USER_PROFILES);
    if (lb_name == NULL) {
	printf ("ORPGDA_lbname (ORPGDAT_USER_PROFILES) failed\n");
	exit (1);
    }

    printf ("Server address: %s\n", lb_name);

    vb = NULL;
    set = 0;
    if (Message_id >= 0) {
	sprintf (buf, "msg_id = %d", Message_id);
	Add_section (buf, &vb);
	set = 1;
    }
    if (User_id >= 0) {
	sprintf (buf, "user_id = %d", User_id);
	Add_section (buf, &vb);
	set = 1;
    }
    if (strlen (User_name) > 0) {
	sprintf (buf, "user_name = %s", User_name);
	Add_section (buf, &vb);
	set = 1;
    }
    if (Up_type >= 0) {
	sprintf (buf, "up_type = %d", Up_type);
	Add_section (buf, &vb);
	set = 1;
    }
    if (Class >= 0) {
	sprintf (buf, "class_num = %d", Class);
	Add_section (buf, &vb);
	set = 1;
    }
    if (Line_ind >= 0) {
	sprintf (buf, "line_ind = %d", Line_ind);
	Add_section (buf, &vb);
	set = 1;
    }

    if (Sql_text == NULL && set)
	Sql_text = vb;
    if (Sql_text == NULL) {
	printf ("Empty query\n");
	exit (1);
    }
    ret = SDQ_select (lb_name, Sql_text, (void *)&qr);
    if (ret == CSS_SERVER_DOWN) {
	ORPGMGR_start_rpgdbm (lb_name);
	ret = SDQ_select (lb_name, Sql_text, (void *)&qr);
    }
    if (ret < 0) {
	printf ("SDQ_select (%s) failed, ret %d\n", Sql_text, ret);
	exit (1);
    }

    Print_query_results (qr);

    if (qr != NULL)
	free (qr);

    exit (0);
}

/**************************************************************************

    Appends SQL section of "str" to "vb".

**************************************************************************/

static void Add_section (char *str, char **vb) {

    if (*vb == NULL || **vb == '\0')
	*vb = STR_copy (*vb, str);
    else {
	*vb = STR_cat (*vb, " and ");
	*vb = STR_cat (*vb, str);
    }
}

/**************************************************************************

    Description: Prints a query results.

    Inputs:	results - query results.

**************************************************************************/

static void Print_query_results (void *results) {
    int i;

    printf ("n_records_found %d  n_records_returned %d  query_error %d\n", 
		SDQ_get_n_records_found (results), 
		SDQ_get_n_records_returned (results), 
		SDQ_get_query_error (results));
    if (SDQ_get_query_error (results) != 0)
	return;

    printf ("    Records:\n");
    for (i = 0; i < SDQ_get_n_records_returned (results); i++) {
	RPG_up_rec_t *rec;

	SDQ_get_query_record (results, i, (void **)&rec);

	printf ("    rec: msg_id %d user_name %s user_id %d up_type %d\n", 
	      rec->msg_id, rec->user_name, rec->user_id, rec->up_type);
	printf ("         class %d line_ind %d distri_method %d\n", 
	      rec->class_num, rec->line_ind, rec->distri_method);
	if (Detailed_info) {
	    char *buf;
	    int len = ORPGDA_read (ORPGDAT_USER_PROFILES, 
				&buf, LB_ALLOC_BUF, rec->msg_id);
	    if (len <= 0) {
		printf ("ORPGDA_read ORPGDAT_USER_PROFILES, msg %d, failed (ret %d)\n",
						rec->msg_id, len);
	    }
	    else {
		Print_user_profile (buf, len);
		free (buf);
	    }
	}
    }
}

/**************************************************************************

    Prints user profile of "msg_id"

**************************************************************************/

static void Print_user_profile (char *buf, int len) {
    int i;
    Pd_user_entry *up;

    up = (Pd_user_entry *)buf;
    if (len < sizeof (Pd_user_entry)) {
	printf ("Unexpected UP record (%d) - Too short\n", len);
	return;
    }

    printf ( 
	"    size %d, u_id %d, u_type %d, line_ind %d, class %d, u_name %s\n",
			up->entry_size, up->user_id, up->up_type, 
			up->line_ind, up->class, up->user_name);
    if (!Detailed_info)
	return;
    printf ( 
	"    cntl %x, defined %x, max_con_time %d, max_n_reqs %d, wait_time_for_rps %d, pswd %s\n", 
			up->cntl, up->defined, up->max_connect_time, 
			up->n_req_prods, up->wait_time_for_rps, 
			up->user_password);

    printf ("    PMS list: len %d\n", up->pms_len);
    if (up->pms_len > 0) {
	Pd_pms_entry *entry;

	entry = (Pd_pms_entry *)((char *)up + up->pms_list);
	for (i = 0; i < up->pms_len; i++) {
	    if (i < 2 || i == up->pms_len - 1) {
		printf ( 
	"        prod_id %d, wx_modes %2x, types %2x\n",
			entry->prod_id, entry->wx_modes, entry->types);
	    }
	    if (i == 2 && up->pms_len != 3)
		printf ( "        ......\n");
	    entry++;
	}
    }

    printf ("    Default Distri list: len %d\n", up->dd_len);
    if (up->dd_len > 0) {
	Pd_prod_item *entry;

	entry = (Pd_prod_item *)((char *)up + up->dd_list);
	for (i = 0; i < up->dd_len; i++) {
	    if (i < 2 || i == up->dd_len - 1) {
		printf ( 
	"        pid %d, wx %2x, prd %d, num %d, map %d, prio %d, prms %s\n",
			entry->prod_id, entry->wx_modes, 
			entry->period, entry->number, entry->map_requested, 
			entry->priority, Print_parameters (entry->params));
	    }
	    if (i == 2 && up->dd_len != 3)
		printf ( "        ......\n");
	    entry++;
	}
    }

    printf ("    Map list: len %d\n", up->map_len);
    if (up->map_len > 0) {
	short *entry;
	char buf[128];

	entry = (short *)((char *)up + up->map_list);
	sprintf (buf, "        maps: ");
	for (i = 0; i < up->map_len; i++) {
	    if (i > 6) 
		break;
	    if (i == 0)
		sprintf (buf + strlen (buf), "%d", *entry);
	    else
		sprintf (buf + strlen (buf), ", %d", *entry);
	    entry++;
	}
	if (i < up->map_len)
	    strcat (buf, " ... ");
	printf (buf);
    }
}

/**************************************************************************

    Description: This function prints the product parameters.

    Inputs:	params - the product parameters.

    Return:	A pointer to the buffer of the printed text.

**************************************************************************/

static char *Print_parameters (short *params)
{
    static char buf[64];	/* buffer for the parameter text */
    char *pt;
    int i;

    pt = buf;
    for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++) {
	int p;

	p = params[i];
	switch (p) {
	    case PARAM_UNUSED:
		strcpy (pt, "UNU ");
		pt += 4;
		break;
	    case PARAM_ANY_VALUE:
		strcpy (pt, "ANY ");
		pt += 4;
		break;
	    case PARAM_ALG_SET:
		strcpy (pt, "ALG ");
		pt += 4;
		break;
	    case PARAM_ALL_VALUES:
		strcpy (pt, "ALL ");
		pt += 4;
		break;
	    case PARAM_ALL_EXISTING:
		strcpy (pt, "EXS ");
		pt += 4;
		break;
	    default:
		sprintf (pt, "%6d ", p);
		pt += strlen (pt);
		break;
	}
    }
    return (buf);
}

/******************************************************************

    Prints all records in Db_file.

******************************************************************/

static void Print_db_records () {
    int fd, cnt;

    fd = LB_open (Db_file, 0, NULL);
    if (fd < 0) {
	printf ("LB_open (%s) failed, ret %d\n", Db_file, fd);
	exit (1);
    }
    LB_seek (fd, 0, LB_FIRST, NULL);
    cnt = 0;
    while (1) {
	char *buf;
	int ret = LB_read (fd, &buf, LB_ALLOC_BUF, LB_NEXT);
	if (ret == LB_TO_COME)
	    break;
	else if (ret < 0) {
	    printf ("LB_read %s failed (ret %d)\n", Db_file, ret);
	    exit (1);
	}
	cnt++;
	Print_user_profile (buf, ret);
	free (buf);
    }
    printf ("%d records in %s\n", cnt, Db_file);
    exit (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char *argv[])
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;           /* error flag */

    err = 0;
    Message_id = -1;
    User_id = -1;
    strcpy (User_name, "");
    Up_type = -1;
    Class = -1;
    Line_ind = -1;
    Detailed_info = 0;

    while ((c = getopt (argc, argv, "t:u:m:i:c:l:q:f:dh?")) != EOF) {
	switch (c) {
	    int t;
	    
	    case 'q':
		Sql_text = STR_copy (Sql_text, optarg);
		break;

	    case 'm':
		if (sscanf (optarg, "%d", &Message_id) != 1)
		    err = -1;
		break;

	    case 'i':
		if (sscanf (optarg, "%d", &t) != 1)
		    err = -1;
		User_id = t;
		break;

	    case 'u':
		if (sscanf (optarg, "%s", User_name) != 1)
		    err = -1;
		break;

	    case 't':
		if (sscanf (optarg, "%d", &t) != 1)
		    err = -1;
		Up_type = t;
		break;

	    case 'c':
		if (sscanf (optarg, "%d", &t) != 1)
		    err = -1;
		Class = t;
		break;

	    case 'l':
		if (sscanf (optarg, "%d", &t) != 1)
		    err = -1;
		Line_ind = t;
		break;

	    case 'd':
		Detailed_info = 1;
		break;
	    
	    case 'f':
		Db_file = STR_copy (Db_file, optarg);
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }
    if (err) {
	fprintf (stderr, "Bad command line option detected");
	exit (1);
    }
	
    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char *argv[]) {
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("    Queries the product user DB and prints the result.\n");
    printf ("    Options:\n");
    printf ("    -m msg_id (Search by message id)\n");
    printf ("    -i user_id (Search by user id)\n");
    printf ("    -u user_name (Search by user name)\n");
    printf ("    -t type (Search by UP type)\n");
    printf ("       UP type: 0 - line user; 1 - dial or dedicated user; 3 - class.\n");
    printf ("    -c class (Search by user class)\n");
    printf ("    -l line (Search by line index)\n");
    printf ("    -q text (Search by SQL text)\n");
    printf ("       Keys: msg_id user_id user_name up_type class_num line_ind\n");
    printf ("       e.g. -q \"user_name = ADS4 and up_type = 1\"\n");
    printf ("    -d (print detailed user profile info)\n");
    printf ("    -f file (Prints all records in database file)\n");
    printf ("    -h (Print usage info)\n");
    exit (0);
}



