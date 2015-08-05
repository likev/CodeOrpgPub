
/******************************************************************

    This module queries the products.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 19:50:01 $
 * $Id: xpdt_query.c,v 1.3 2005/12/27 19:50:01 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <orpg.h> 
#include <prod_gen_msg.h> 
#include <infr.h> 
#include "xpdt_def.h"

#define LOCAL_NAME_SIZE 128		/* maximum name size */
#define LOCAL_FNAME_SIZE 32		/* maximum product file name size */

#define FILE_TABLE_UPDATE_TIME 2	/* file table update period (secs) */

extern int debug;

static char *Dir_name;			/* product file dir. Emtry string
					   indicates to use RPG product db */
static char Lb_name[LOCAL_NAME_SIZE];	/* RPG product DB LB name */
static int Lb_fd = -1;			/* RPG product DB LB fd */

typedef struct {
    char *file_name;			/* name of the file */
    unsigned int hash;			/* hash value of "file_name" */
    int prod_code;			/* product code */
    int elev;				/* elevation in .1 degrees */
    time_t vol_time;			/* volume time (UNIX time) */
    int msg_id;				/* an unique ID started with 2. <= 0 
					   indicates a deleted product. 1
					   indicates a non-prod file. */
    short params[6];
} Product_file_t;

static Product_file_t *Files = NULL;	/* current products (files) */
static int N_files = 0;			/* size of Files */

static int Msg_id = 2;			/* unique ID for each product file */

ORPGDBM_query_data_t Query_data[16];	/* DB query data */


static void Check_new_files ();
static int Get_prod_attributes (char *name, 
			int *prod_code, int *elev, time_t *vol_tm);
static char *Get_full_path (char *name);
static int Get_attr_from_prod (char *name, 
			int *prod_code, int *elev, time_t *vol_tm);
static int Parse_select_result (int ret, char *where, 
		void *query_result, RPG_prod_rec_t *db_info, int buf_size);
static unsigned int Get_hash (char *name);
static int Verify_produst (char *name, char *buf, int size,
			int *pprod_code, int *pelev, time_t *pvol_tm);


/**************************************************************************

    Initializes this module.

**************************************************************************/

int XQ_init (char *dir, char *prod_table) {

    Dir_name = dir;
    if (strlen (dir) > 0)
	XQ_routine ();		/* initialize the product file table */
    else {
	if (CS_entry_int_key (ORPGDAT_PRODUCTS, 
			ORPGSC_LBNAME_COL, LOCAL_NAME_SIZE, Lb_name) <= 0) {
	    fprintf (stderr, "product data base file name not found\n");
	    exit (1);
	}
	Lb_fd = LB_open (Lb_name, LB_READ, NULL);
	if (Lb_fd < 0) {
	    fprintf (stderr, "LB_open ORPGDAT_PRODUCTS (%s) failed (%d)\n", 
						Lb_name, Lb_fd);
	    exit (1);
	}
    }
    if (strlen (prod_table) == 0) {
	char *env = getenv ("CFG_DIR");
	if (env == NULL) {
	    fprintf (stderr, "Product table file name not specified\n");
	    exit (1);
	}
	strcpy (prod_table, env);
	strcat (prod_table, "/product_attr_table");
    }

    return (0);
}

/**************************************************************************

    Housekeeping function for this module.

**************************************************************************/

int XQ_routine () {
    static time_t last_time = 0;
    time_t tm;

    tm = time (NULL);
    if (tm >= last_time + FILE_TABLE_UPDATE_TIME) {
	Check_new_files ();
	last_time = tm;
    }

    return (0);
}

/**************************************************************************

    Read product of message ID "msg_id". The product is returned with 
    "data". Returns the product length on success of 0 on failure.

**************************************************************************/

int XQ_read_product (unsigned int msg_id, char **data) {
    Product_file_t *f;
    int i;

    if (strlen (Dir_name) == 0) {	/* read product DB */
	int size = LB_read (Lb_fd, data, LB_ALLOC_BUF, msg_id);
	if (size <= 0) {
	    fprintf (stderr, "LB_read product DB (msgid %d) failed (%d)\n",
			msg_id, size);
	    *data = NULL;
	    return (0);
	}
	return (size);
    }

    *data = NULL;
    for (i = 0; i < N_files; i++) {
	f = Files + i;
	if (f->msg_id <= 1)
	    continue;
	if (f->msg_id == msg_id)
	    break;
    }
    if (i >= N_files) {
	fprintf (stderr, "Product file (msgid %d) not found\n", msg_id);
	return (0);
    }
    return (XQ_read_product_by_name (Get_full_path (f->file_name), data));
}

/**************************************************************************

    Read product of name "name". The product is returned with 
    "data". Returns the product length on success of 0 on failure.

**************************************************************************/

int XQ_read_product_by_name (char *name, char **data) {
    int fd, size;
    char *buf;

    fd = open (name, O_RDONLY);
    if (fd < 0) {
	fprintf (stderr, "open file %s failed\n", name);
	return (0);
    }
    size = lseek (fd, 0, SEEK_END);
    lseek (fd, 0, SEEK_SET);
    if (size <= 0) {
	fprintf (stderr, "Bad file (%s) size (%d)\n", name, size);
	close (fd);
	return (0);
    }
    size += sizeof (Prod_header);
    buf = malloc (size);
    if (buf == NULL) {
	fprintf (stderr, "malloc (%d bytes) failed\n", size);
	close (fd);
	return (0);
    }
    memset (buf, 0, sizeof (Prod_header));
    if (read (fd, buf + sizeof (Prod_header), size) != 
					size - sizeof (Prod_header)) {
	fprintf (stderr, "read product file (%s) failed\n", name);
	close (fd);
	free (buf);
	return (0);
    }
    if (debug)
	printf ("    read product %d\n", size);
    close (fd);
    *data = buf;
    if (Verify_produst (name, buf + sizeof (Prod_header), 
		size - sizeof (Prod_header), NULL, NULL, NULL) > 0)
	return (size);
    return (0);
}

/**************************************************************************

    Query all vol times. The results are put in "Db_info" of size "size".
    Returns the number of records found.

**************************************************************************/

int XQ_get_all_vol_times (RPG_prod_rec_t *Db_info, int size) {
    int n_vols, done, i, ret;

    if (strlen (Dir_name) == 0) {
	void *query_result;
	char buf[128];
	SDQ_set_query_mode (SDQM_FULL_SEARCH | SDQM_HIGHEND_SEARCH |
			   SDQM_DISTINCT_FIELD_VALUES);
	SDQ_set_maximum_records (size);
	sprintf (buf, "vol_t >= 0");
	ret = SDQ_select (Lb_name, buf, (void **)&query_result);
	return (Parse_select_result (ret, buf, query_result, Db_info, size));
    }

    n_vols = 0;
    for (i = 0; i < N_files; i++) {
	int k;
	Product_file_t *f = Files + i;
	if (f->msg_id <= 1)
	    continue;
	for (k = 0; k < n_vols; k++) {
	    if (Db_info[k].vol_t == f->vol_time)
		break;
	}
	if (k < n_vols)
	    continue;
	if (n_vols >= size)
	    break;
	Db_info[n_vols].vol_t = f->vol_time;
	Db_info[n_vols].prod_code = f->prod_code;
	Db_info[n_vols].msg_id = f->msg_id;
	memcpy (Db_info[n_vols].params, f->params, 6 * sizeof (short));
	n_vols++;
    }
    done = 0;		/* sort */
    while (!done) {
	done = 1;
	for (i = 1; i < n_vols; i++) {
	    if (Db_info[i - 1].vol_t < Db_info[i].vol_t) {
		RPG_prod_rec_t t = Db_info[i - 1];
		Db_info[i - 1] = Db_info[i];
		Db_info[i] = t;
		done = 0;
	    }
	}
    }
    return (n_vols);
}

/**************************************************************************

    Query all product code values. The results are put in "Db_info" of size 
    "size". Returns the number of records found.

**************************************************************************/

int XQ_get_all_product_code (RPG_prod_rec_t *Db_info, int size) {
    int n_codes, done, i, ret;

    if (strlen (Dir_name) == 0) {
	void *query_result;
	char buf[128];
	SDQ_set_query_mode (SDQM_FULL_SEARCH | SDQM_HIGHEND_SEARCH |
			   SDQM_DISTINCT_FIELD_VALUES);
	SDQ_set_maximum_records (size);
	sprintf (buf, "prod_code >= 0");
	ret = SDQ_select (Lb_name, buf, (void **)&query_result);
	return (Parse_select_result (ret, buf, query_result, Db_info, size));
    }

    n_codes = 0;
    for (i = 0; i < N_files; i++) {
	int k;
	Product_file_t *f = Files + i;
	if (f->msg_id <= 1)
	    continue;
	for (k = 0; k < n_codes; k++) {
	    if (Db_info[k].prod_code == f->prod_code)
		break;
	}
	if (k < n_codes)
	    continue;
	if (n_codes >= size)
	    break;
	Db_info[n_codes].vol_t = f->vol_time;
	Db_info[n_codes].prod_code = f->prod_code;
	Db_info[n_codes].msg_id = f->msg_id;
	memcpy (Db_info[n_codes].params, f->params, 6 * sizeof (short));
	n_codes++;
    }
    done = 0;		/* sort */
    while (!done) {
	done = 1;
	for (i = 1; i < n_codes; i++) {
	    if (Db_info[i - 1].prod_code < Db_info[i].prod_code) {
		RPG_prod_rec_t t = Db_info[i - 1];
		Db_info[i - 1] = Db_info[i];
		Db_info[i] = t;
		done = 0;
	    }
	}
    }
    return (n_codes);
}

/**************************************************************************

    Query all products by product code "prod_code" and volume time 
    "vol_time". The results are put in "Db_info" of size "size". Returns 
    the number of records found.

**************************************************************************/

int XQ_query_prod_by_code_and_vol_time (int prod_code, time_t vol_time,
					RPG_prod_rec_t *Db_info, int size) {
    int n_prods, done, i, ret;

    if (strlen (Dir_name) == 0) {
	void *query_result;
	char buf[128];
	SDQ_set_query_mode (0);
	SDQ_set_maximum_records (size);
	sprintf (buf, "prod_code = %d and vol_t = %lu", prod_code, vol_time);
	ret = SDQ_select (Lb_name, buf, (void **)&query_result);
	return (Parse_select_result (ret, buf, query_result, Db_info, size));
    }

    n_prods = 0;
    for (i = 0; i < N_files; i++) {
	Product_file_t *f = Files + i;
	if (f->msg_id <= 1)
	    continue;
	if (f->prod_code != prod_code || f->vol_time != vol_time)
	    continue;
	if (n_prods >= size)
	    break;
	Db_info[n_prods].vol_t = f->vol_time;
	Db_info[n_prods].prod_code = f->prod_code;
	Db_info[n_prods].msg_id = f->msg_id;
	Db_info[n_prods].elev = f->elev;
	memcpy (Db_info[n_prods].params, f->params, 6 * sizeof (short));
	n_prods++;
    }
    done = 0;		/* sort */
    while (!done) {
	done = 1;
	for (i = 1; i < n_prods; i++) {
	    if (Db_info[i - 1].elev > Db_info[i].elev) {
		RPG_prod_rec_t t = Db_info[i - 1];
		Db_info[i - 1] = Db_info[i];
		Db_info[i] = t;
		done = 0;
	    }
	}
    }
    return (n_prods);
}

/**************************************************************************

    Parses SDQ_select results, query_result, and put it in db_info of 
    array_sizse "buf_size". "ret" is the SDQ_select return value and 
    "where" is the SDQ_select input. Returns the number of records found.

**************************************************************************/

static int Parse_select_result (int ret, char *where, 
		void *query_result, RPG_prod_rec_t *db_info, int buf_size) {
    int n_recs, i;

    if (ret < 0) {
	fprintf (stderr, "SDQ_select (%s) failed (%d)\n", where, ret);
    }
    else {
	ret = SDQ_get_query_error (query_result);
	if (ret < 0)
	    fprintf (stderr, "query (%s) error: %d\n", where, ret);
    }
    n_recs = 0;
    if (ret >= 0) {
	int records_found, records_returned;
	records_found = SDQ_get_n_records_found (query_result);
	records_returned = SDQ_get_n_records_returned (query_result);
	if (records_found > records_returned)
	    fprintf (stderr, 
		"Query matched more records (%d) than returned (%d)\n",
				records_found, records_returned);
	if (records_returned > 0)
	    n_recs = records_returned;
    }

    if (n_recs > buf_size)
	n_recs = buf_size;
    for (i = 0; i < n_recs; i++) {
	RPG_prod_rec_t *rec;
	int j;

	if (SDQ_get_query_record (query_result, i, (void **)&rec)) {
	    db_info[i].msg_id = rec->msg_id;
	    db_info[i].reten_t = rec->reten_t;
	    db_info[i].vol_t = rec->vol_t;
	    db_info[i].prod_code = rec->prod_code;
	    db_info[i].elev = rec->elev;
	    db_info[i].warehoused = rec->warehoused;

	    for (j = 0; j < 6; j++) {
		db_info[i].params[j] = rec->params[j];
		db_info[i].req_params[j] = rec->req_params[j];
	    }
	} else {
	    fprintf (stderr, "SDQ_get_query_record (%d) failed\n", i);
	    n_recs = 0;
	    break;
	}
    }

    if (query_result != NULL)
	free (query_result);
    return (n_recs);
}

/**************************************************************************

    Goes through the directory and finds any new files and deleted files
    and update the file database.

**************************************************************************/

static void Check_new_files () {
    static DIR *Dir = NULL;		/* directory struct */
    struct dirent *dp;
    Product_file_t *f;
    int i;

    if (strlen (Dir_name) == 0)
	return;
    if (Dir == NULL) {
	Dir = opendir (Dir_name);
	if (Dir == NULL) {
	    fprintf (stderr, "opendir (%s) failed, errno %d\n", 
						Dir_name, errno);
	    exit (1);
	}
    }

    for (i = 0; i < N_files; i++) {
	f = Files + i;
	if (f->msg_id > 0)		/* to be check if still exists */
	    f->msg_id = -f->msg_id;
	else
	    f->msg_id = 0;		/* delete this product */
    }

    rewinddir (Dir);	/* rewind and refresh */
    while ((dp = readdir (Dir)) != NULL) {
	time_t vol_tm;
	int is_prod, prod_code, elev;
	unsigned int hash;

	hash = Get_hash (dp->d_name);
	for (i = 0; i < N_files; i++) {
	    f = Files + i;
	    if (f->msg_id < 0 && f->hash == hash &&
		strcmp (f->file_name, dp->d_name) == 0) {
		f->msg_id = -f->msg_id;
		break;
	    }
	}
	if (i < N_files)	/* existing file */
	    continue;

	is_prod = Get_prod_attributes (dp->d_name, &prod_code, &elev, &vol_tm);

	for (i = 0; i < N_files; i++) {		/* find an empty slot */
	    f = Files + i;
	    if (f->msg_id == 0)
		break;
	}
	if (i >= N_files) {	/* create a new slot */
	    Product_file_t pf;
	    pf.file_name = NULL;
	    Files = (Product_file_t *)STR_append ((char *)Files, 
				(char *)&pf, sizeof (Product_file_t));
	    N_files++;
	}
	f = Files + i;		/* add the new product to the table */
	memset (f->params, 0, 6 * sizeof (short));
	f->params[2] = elev;
	f->prod_code = prod_code;
	f->elev = elev;
	f->vol_time = vol_tm;
	if (is_prod)
	    f->msg_id = Msg_id++;
	else
	    f->msg_id = 1;
	f->hash = hash;
	if (f->file_name != NULL)
	    free (f->file_name);
	f->file_name = malloc (strlen (dp->d_name) + 1);
	if (f->file_name == NULL) {
	    fprintf (stderr, "malloc failed\n");
	    exit (1);
	}
	strcpy (f->file_name, dp->d_name);
    }
}

/************************************************************************

    Extracts and returns the volume time, product code and elevation from
    file name "name". It returns 1 on success or 0 if "name" is not a 
    product file. It first tries to use the file name to identify the
    product file. The name format is yyyymmdd_hh:mm:ss:_ppp_eeee where
    ppp and eeee are product code and elevation in degrees. If name 
    does not tell it calls another funciton to open the file and check.

************************************************************************/

static int Get_prod_attributes (char *name, 
		int *prod_code, int *elev, time_t *vol_tm) {
    char buf[LOCAL_NAME_SIZE];
    struct stat st;
    int ret, ok, len;

    ret = stat (Get_full_path (name), &st);
    if (ret < 0) {
	fprintf (stderr, "stat (%s) failed, errno %d\n", name, errno);
	return (0);
    }
    if (!(st.st_mode & S_IFREG))	/* not a regular file */
	return (0);

    len = strlen (name);
    ok = 0;
    if (/*len >= LOCAL_FNAME_SIZE && len < 24*/ len < 0) { /* commented */
	int y, mon, d, h, m, s, pcode;
	float ele;
	char c;

	ok = 1;
	strncpy (buf, name, 4);
	buf[4] = '\0';
	if (sscanf (buf, "%d%c", &y, &c) != 1)
	    ok = 0;
	strncpy (buf, name + 4, 2);
	buf[2] = '\0';
	if (ok && sscanf (buf, "%d%c", &mon, &c) != 1)
	    ok = 0;
	strncpy (buf, name + 6, 2);
	buf[2] = '\0';
	if (ok && sscanf (buf, "%d%c", &d, &c) != 1)
	    ok = 0;
	strncpy (buf, name + 9, 2);
	buf[2] = '\0';
	if (ok && sscanf (buf, "%d%c", &h, &c) != 1)
	    ok = 0;
	strncpy (buf, name + 12, 2);
	buf[2] = '\0';
	if (ok && sscanf (buf, "%d%c", &m, &c) != 1)
	    ok = 0;
	strncpy (buf, name + 15, 2);
	buf[2] = '\0';
	if (ok && sscanf (buf, "%d%c", &s, &c) != 1)
	    ok = 0;
	strncpy (buf, name + 18, 3);
	buf[3] = '\0';
	if (ok && sscanf (buf, "%d%c", &pcode, &c) != 1)
	    ok = 0;
	strcpy (buf, name + 22);
	if (ok && sscanf (buf, "%f%c", &ele, &c) != 1)
	    ok = 0;
	if (ok) {
	    time_t t = 0;
	    unix_time (&t, &y, &mon, &d, &h, &m, &s);
	    *vol_tm = t;
	    *prod_code = pcode;
	    *elev = (int)(ele * 10.);
	}
    }
    if (!ok && 
	Get_attr_from_prod (name, prod_code, elev, vol_tm) == 0) {
	if (debug)
	    fprintf (stderr, "File %s is not a product file\n", name);
	return (0);
    }
    return (1);
}

/************************************************************************

    Extracts and returns the volume time, product code and elevation from
    product file "name". It returns 1 on success or 0 if "name" is not a 
    product file.

************************************************************************/

static int Get_attr_from_prod (char *name, 
			int *pprod_code, int *pelev, time_t *pvol_tm) {
    int fd, file_size;
    char buf[sizeof (Graphic_product)];

    fd = open (Get_full_path (name), O_RDONLY);
    if (fd < 0) {
	fprintf (stderr, "open file %s failed\n", Get_full_path (name));
	return (0);
    }
    file_size = lseek (fd, 0, SEEK_END);
    lseek (fd, 0, SEEK_SET);
    if (read (fd, buf, sizeof (Graphic_product)) != sizeof (Graphic_product)) {
	if (debug)
	    fprintf (stderr, "File %s is not a product - Too short\n", 
						Get_full_path (name));
	close (fd);
	return (0);
    }
    close (fd);

    return (Verify_produst (Get_full_path (name), buf, file_size, 
					pprod_code, pelev, pvol_tm));
}

/************************************************************************

    Checks if "buf" of "size" bytes is an ICD product. "buf" may contains 
    only the header. It returns 1 if true or 0 if false. If it is an ICD 
    product, the product code, elevation angle and volume time is returned.

************************************************************************/

static int Verify_produst (char *name, char *buf, int size,
			int *pprod_code, int *pelev, time_t *pvol_tm) {
    Graphic_product *hd;
    short prod_code, elev, divider;
    unsigned short vt_ms, vt_ls, vol_date;
    unsigned int t, msg_len;

    hd = (Graphic_product *)buf;
    msg_len = hd->msg_len;
    prod_code = hd->prod_code;
    elev = hd->param_3;
    vol_date = hd->vol_date;
    vt_ms = hd->vol_time_ms;
    vt_ls = hd->vol_time_ls;
    divider = hd->divider;
#ifdef LITTLE_ENDIAN_MACHINE
    msg_len = INT_BSWAP (msg_len); 
    prod_code = SHORT_BSWAP (prod_code); 
    elev = SHORT_BSWAP (elev); 
    vol_date = SHORT_BSWAP (vol_date); 
    vt_ms = SHORT_BSWAP (vt_ms); 
    vt_ls = SHORT_BSWAP (vt_ls); 
    divider = SHORT_BSWAP (divider); 
#endif

printf (" %d  %d  %d   %d %d   %d %d\n", size, msg_len, divider, hd->msg_code, hd->prod_code, prod_code, prod_code);
    if (size != msg_len || divider != -1 || 
		hd->msg_code != hd->prod_code || 
		prod_code < 16 || prod_code > 300) {
	fprintf (stderr, "File %s is not a NEXRAD product\n", name);
	return (0);
    }
    t = vt_ms;
    t = (t << 16) | vt_ls;
    if (pprod_code != NULL)
	*pprod_code = prod_code;
    if (pelev != NULL)
	*pelev = elev;
    if (pvol_tm != NULL)
	*pvol_tm = (time_t)(vol_date - 1) * 86400 + t;
    return (1);
}

/************************************************************************

    Returns the full path of file name "name".

************************************************************************/

static char *Get_full_path (char *name) {
    static char fname[LOCAL_NAME_SIZE * 2 + 4];
    sprintf (fname, "%s/%s", Dir_name, name);
    return (fname);
}

/************************************************************************

    Returns the full path of file name "name".

************************************************************************/

static unsigned int Get_hash (char *name) {
    unsigned char *cpt;
    unsigned int h, cnt;

    if (name == NULL)
	return (0);
    h = 0;
    cpt = name;
    cnt = 0;
    while (*cpt != '\0') {
	unsigned int t;
	t = *cpt;
	h += t << (cnt % 24);
	cnt++;
	cpt++;
    }
    return (h);
}

