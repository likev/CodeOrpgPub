
/******************************************************************

	This is a RPG product query tool.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/12/13 18:12:04 $
 * $Id: prod_query.c,v 1.24 2013/12/13 18:12:04 steves Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <prod_gen_msg.h> 
#include <orpg.h> 
#include <infr.h> 
#include <orpgerr.h>

#define DB_NAME_SIZE 128

static short Prod_code, Prod_code_max;
static int Start_vol_time, End_vol_time;
static short Elev_min, Elev_max;
static short Warehoused;
static int Full_product_listing;
static int Max_n_records;
static int Reverse_search;

static char *Sql_text = NULL;
static char *Save_dir = NULL;
static char *Lb_name;

static void Print_usage (char *argv[]);
static int Read_options (int argc, char *argv[]);
static void Print_query_results (void *results);
static char *Print_parameters (short *params);
static void Add_section (char *str, char **vb);
static time_t Parse_time_string (char *str);
static char *Process_time_string (char *sql_text);
static void Save_products (LB_id_t msg_id);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char *argv[])
{
    void *qr;			/* query results */
    int ret, set;
    char buf[256], *vb;

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (0);
    
    Lb_name = ORPGDA_lbname (ORPGDAT_PRODUCTS);
    if (Lb_name == NULL) {
	printf ("ORPGDA_lbname (ORPGDAT_PRODUCTS) failed\n");
	exit (1);
    }

    printf ("Server address: %s\n", Lb_name);

    vb = NULL;
    set = 0;
    if (Prod_code >= 0) {
	if (Prod_code_max >= 0)
	    sprintf (buf, "prod_code >= %d and prod_code <= %d", 
						Prod_code, Prod_code_max);
	else
	    sprintf (buf, "prod_code = %d", Prod_code);
	Add_section (buf, &vb);
	set = 1;
    }
    if (Start_vol_time > 0) {
	if (End_vol_time > 0)
	    sprintf (buf, "vol_t >= %d and vol_t <= %d", 
					Start_vol_time, End_vol_time);
	else
	    sprintf (buf, "vol_t = %d", Start_vol_time);
	Add_section (buf, &vb);
	set = 1;
    }
    if (Elev_min > -100) {
	if (Elev_max > -100)
	    sprintf (buf, "elev >= %d and elev <= %d", 
						Elev_min, Elev_max);
	else
	    sprintf (buf, "elev = %d", Elev_min);
	Add_section (buf, &vb);
	set = 1;
    }
    if (Warehoused >= 0) {
	sprintf (buf, "warehoused = %d", Warehoused);
	Add_section (buf, &vb);
	set = 1;
    }

    if (!set) {
	sprintf (buf, "vol_t >= 0 and vol_t <= %d", 0x7fffffff);
	Add_section (buf, &vb);
    }

    if (Sql_text == NULL)
	Sql_text = vb;
    Sql_text = Process_time_string (Sql_text);
    if (Reverse_search == 1)
	SDQ_set_query_mode (SDQM_HIGHEND_SEARCH);
    SDQ_set_maximum_records (Max_n_records);
    ret = SDQ_select (Lb_name, Sql_text, (void *)&qr);
    if (ret == CSS_SERVER_DOWN) {
	ORPGMGR_start_rpgdbm (Lb_name);
	if (Reverse_search == 1)
	    SDQ_set_query_mode (SDQM_HIGHEND_SEARCH);
	SDQ_set_maximum_records (Max_n_records);
	ret = SDQ_select (Lb_name, Sql_text, (void *)&qr);
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
    int yy,mm,dd,hh,mi,ss;

    printf ("n_records_found %d  n_records_returned %d  query_error %d\n", 
		SDQ_get_n_records_found (results), 
		SDQ_get_n_records_returned (results), 
		SDQ_get_query_error (results));
    if (SDQ_get_query_error (results) != 0)
	return;

    printf ("    Records:\n");
    for (i = 0; i < SDQ_get_n_records_returned (results); i++) {
	RPG_prod_rec_t *rec;
	char mnemonic[80], *mnem;
	int prod_id;
	LB_id_t msg_id;

	memset( mnemonic, 0, 80 );
	strcpy( mnemonic, "   " );

	SDQ_get_query_record (results, i, (void **)&rec);
	SDQ_get_msg_id (results, i, &msg_id);
	unix_time ((time_t *)&(rec->vol_t), &yy, &mm, &dd, &hh, &mi, &ss);

	prod_id = ORPGPAT_get_prod_id_from_code( (int) rec->prod_code );
	if( prod_id > 0 ){

	   mnem = ORPGPAT_get_mnemonic( (int) prod_id );
	      if( mnem != NULL )
		 strcpy( mnemonic, mnem );

	}

	printf ("   rec: msg_id %d  prod_code %d (%s)  elev %d  vol_time %.2d/%.2d/%d-%.2d:%.2d:%.2d\n", 
		msg_id, rec->prod_code, mnemonic, rec->elev, mm, dd, yy, hh, mi, ss);

	if( Full_product_listing ){

	   char resp_params[80], *resp;
	   char req_params[80], *req;
	   Prod_header phd;
	   int ret;
     
	   memset( resp_params, 0, 80 );
	   resp = Print_parameters( rec->params );
	   memcpy( resp_params, resp, strlen(resp) );

	   memset( req_params, 0, 80 );
	   req = Print_parameters( rec->req_params );
	   memcpy( req_params, req, strlen(req) );
	   printf ("       ---> resp params %s  req params %s\n", resp_params, req_params );

	   unix_time( &(rec->gen_t), &yy, &mm, &dd, &hh, &mi, &ss );
	   printf ("       ---> wx mode %d  gen_time %.2d/%.2d/%d-%.2d:%.2d:%.2d", 
		   rec->wx_mode, mm, dd, yy, hh, mi, ss);

	   ret = ORPGDA_read( ORPGDAT_PRODUCTS, &phd, sizeof(Prod_header), rec->msg_id );
	   if( (ret > 0) || (ret == LB_BUF_TOO_SMALL) )
	      printf ("  size (bytes) %d\n\n", phd.g.len);
	   else
	      printf ( "\n\n" );

	}

	if (Save_dir != NULL)
	    Save_products (msg_id);
    }
    return;
}

/**************************************************************************

    Save the product of "msg_id" in directory "Save_dir".

**************************************************************************/

static void Save_products (LB_id_t msg_id) {
    static int fd = -1;
    char fname[256], *prod, *icd_prod;
    int size, fl, p_code;

    if (fd < 0) {
	fd = LB_open (Lb_name, LB_READ, NULL);
	if (fd < 0) {
	    printf ("LB_open %s failed (%d)\n", Lb_name, fd);
	    exit (1);
	}
	if (MISC_mkdir (Save_dir) < 0) {
	    printf ("MISC_mkdir %s failed\n", Save_dir);
	    exit (1);
	}
    }
    size = LB_read (fd, (char *)&prod, LB_ALLOC_BUF, msg_id);
    if (size < 0) {
	printf ("LB_read (msg %d) failed (%d)\n", msg_id, size);
	return;
    }
    if (size < 96) {
	printf ("Product (msg_id %d) too small (%d)\n", msg_id, size);
	free (prod);
	return;
    }

    {				/* create file name */
	int y, mon, d, h, m, s;
	time_t t;
	Prod_header *phd;
	int pid, eind;

	phd = (Prod_header *)prod;
	t = phd->g.vol_t;
	pid = phd->g.prod_id;
	unix_time (&t, &y, &mon, &d, &h, &m, &s);
	p_code = ORPGPAT_get_code (pid);
	if (p_code > 0)
	    sprintf (fname, "%s/p%d.", Save_dir, p_code);
	else
	    sprintf (fname, "%s/m%d.", Save_dir, msg_id);
	eind = ORPGPAT_get_elevation_index (pid);
	if (eind >= 0) {
	    short ele = phd->g.resp_params[eind];
	    sprintf (fname + strlen (fname), "e%d.", ele);
	}
	sprintf (fname + strlen (fname), 
		"%d-%.2d-%.2d-%.2d:%.2d:%.2d", y, mon, d, h, m, s);
	icd_prod = prod;
	if (p_code > 0) {
	    icd_prod += sizeof (Prod_header);
	    size -= sizeof (Prod_header);
	}
    }

    fl = open (fname, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fl < 0) {
	printf ("open (create) %s failed\n", fname);
	free (prod);
	return;
    }
    if (write (fl, icd_prod, size) != size)
	printf ("write (%d bytes, to %s) failed\n", size, fname);
    free (prod);
    close (fl);
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

/**************************************************************************

    Replaces time string tokens in string "sql_text" which must be a STR
    pointer. Returns the modified STR string.

**************************************************************************/

static char *Process_time_string (char *sql_text) {
    char *buf, *t;

    buf = STR_copy (NULL, sql_text);
    t = strtok (buf, " \t");
    while (t != NULL) {
	time_t tm;
	if ((tm = Parse_time_string (t)) > 0) {
	    char s[128];
	    sprintf (s, "%lu", tm);
	    sql_text = STR_replace (sql_text, strstr (sql_text, t) - sql_text, 
					strlen (t), s, strlen (s));
	}
	t = strtok (NULL, " \t");
    }
    STR_free (buf);
    return (sql_text);
}

/**************************************************************************

    Converts time string in format "mm/dd/yyyy:hh:mm:ss" to UNIX time. 
    Returns the UNIX time on success or 0 on failure.

**************************************************************************/

static time_t Parse_time_string (char *str) {
    int cnt, mon, d, y, h, m, s;
    time_t tm;

    cnt = sscanf (str, "%d%*c%d%*c%d%*c%d%*c%d%*c%d", &mon, &d, &y, &h, &m, &s);    if (cnt != 6)
	return (0);
    if (y < 30)
	y += 2000;
    else if (y < 100)
	y += 1900;
    tm = 0;
    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
    return (tm);
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

    Prod_code = Prod_code_max = -1;
    Elev_min = Elev_max = -100;
    Warehoused = -1;
    Full_product_listing = 0;
    Max_n_records = 16;
    err = 0;
    Reverse_search = 0;
    Start_vol_time = End_vol_time = 0;

    while ((c = getopt (argc, argv, "p:fe:w:n:t:T:q:s:dh?")) != EOF) {
	switch (c) {
	    int t, t1, ret;

	    case 'p':
		if ((ret = sscanf (optarg, "%d%*c%d", &t, &t1)) < 1)
		    err = -1;
		else {
		    Prod_code = t;
		    if (ret == 2)
			Prod_code_max = t1;
		}
		break;

            case 'f':
                Full_product_listing = 1;
                break;

	    case 'e':
		if ((ret = sscanf (optarg, "%d%*c%d", &t, &t1)) < 1)
		    err = -1;
		else {
		    Elev_min = t;
		    if (ret == 2)
			Elev_max = t1;
		}
		break;

	    case 'w':
		if (sscanf (optarg, "%d", &t) != 1)
		    err = -1;
		Warehoused = t;
		break;

	    case 'n':
		if (sscanf (optarg, "%d", &Max_n_records) != 1)
		    err = -1;
		break;
	    
 	    case 't':
       	    case 'T':

		if (c == 'T')
		    Reverse_search = 1;
		End_vol_time = 0;
		if ((Start_vol_time = Parse_time_string (optarg)) > 0) {
		    char *p = optarg;
		    while (*p != '\0' && *p != ',')
			p++;
		    if (*p == ',') {
			End_vol_time = Parse_time_string (p + 1);
			if (End_vol_time == 0)
			    err = -1;
		    }
		}
		else
		    err = -1;
		if (err < 0)
		    printf ("Bad time spec (%s)\n", optarg);
		break;

	    case 'q':
		Sql_text = STR_copy (Sql_text, optarg);
		break;

	    case 's':
		Save_dir = STR_copy (Save_dir, optarg);
		if (Save_dir[strlen (Save_dir) - 1] == '/')
		    Save_dir[strlen (Save_dir) - 1] = '\0';
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

static void Print_usage (char *argv[])
{
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("    Queries the RPG product DB and prints the result. Retrieves\n");
    printf ("    products from the product DB and saves in files.\n");
    printf ("    Options:\n");
    printf ("    -p product_code[,product_code_max] (Search by product code.\n");
    printf ("       Default: All)\n");
    printf ("    -t start_vol_time[,end_vol_time] (Search by volume time. \n");
    printf ("       Format 'mm/dd/yyyy:hh:mm:ss'. Default: All time)\n");
    printf ("       e.g. -t 10/16/1994-08:11:51,10/16/1994-08:23:42\n");
    printf ("    -T (Same as -t except that the display is reverse in time)\n");
    printf ("       e.g. -T 10/16/1994-08:11:51,10/16/1994-08:23:42\n");
    printf ("    -e elev_min[,elev_max] (Search by elevation. In .1 degrees.\n");
    printf ("       Must > -100)\n");
    printf ("    -w warehoused prod ID (Search warehoused product by ID.\n");
    printf ("       Default: Non-warehoused products)\n");
    printf ("    -f (Prints detailed product info. Default: No)\n");
    printf ("    -n max_n_records (Max # of records printed. Default: 16)\n");
    printf ("    -q text (Search by SQL text)\n");
    printf ("       Keys: prod_code vol_t elev warehoused\n");
    printf ("       e.g. -q \"prod_code = 19 and elev >= 5 and elev <= 34\"\n");
    printf ("            -q \"prod_code = 19 and vol_t = 10/16/1994-08:11:51\"\n");
    printf ("    -s dir (Read the products from thr DB and save them in \"dir\")\n");
    printf ("    -h (Prints usage info)\n");
    exit (0);
}



