
/******************************************************************

	file: rpgdbm_products.c

	This is the rpgdbm module containing functions that handle ORPG
	product data base.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/02/01 16:01:00 $
 * $Id: rpgdbm_products.c,v 1.52 2008/02/01 16:01:00 jing Exp $
 * $Revision: 1.52 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <prod_gen_msg.h> 
#include <orpg.h> 
#include <infr.h> 
#include <orpgerr.h>
#include <sdq.h> 

#include "rpgdbm_def.h"

#define UTIL_REPORT_PERIOD 10	/* storage util report period in seconds */
#define MIN_PRODUCTS_LOW   5	/* minimum of N_products_low */


static int Maxn_products;	/* max number of products stored */
static int N_products_low = 0;	/* low water mark of num products */
static int N_products_high;	/* high water mark of num products */

static void *Pdb_id;		/* product DB ID */
static int N_products = 0;	/* num of products in the data base */

static int Plb_fd;		/* product LB fd */

static int Min_reten_time;	/* minimum product retention time */

typedef struct {		/* entry of the product retention table */
    short prod_code;		/* product code */
    short elev;			/* elevation angle or SDQM_UNDEF_SHORT */
    int pr_period;		/* retention period in number of seconds */
} Prod_retention_t;

static void *Pr_tid = NULL;	/* table ID of product retention table */
static Prod_retention_t *Prs;	/* pointer to the product retention table */
static int N_prs = 0;		/* size of the product retention table */    

static int Pr_updated = 0;	/* boolean: prod retention table updated */
static int Cmd_delete = 0;	/* boolean: commanded delete event received */

enum {GREATER_THAN, EQUAL_TO, LESS_THAN}; 	/* Compare_prod_code_elev 
						   return values */

typedef struct {			/* struct used for delayed product
					   deletion */
    time_t dtime;			/* DB deletion time */
    LB_id_t msg_id;			/* product LB message ID */
} Prod_deletion_t;

static void *Del_tid = NULL;		/* delayed deletion table id */
static int N_dels = 0;			/* size of delayed deletion table */
static Prod_deletion_t *Dels;		/* delayed deletion table */

static int Prod_db_initialized = 0;

typedef struct {			/* struct used for gen_t sort */
    unsigned int value;			/* sort value */
    int ind;
} Sort_t;

static int (*Sdqs_select_i) (char *db_name, int mode, int max_n, 
			int index_query, char *where, void **result);
static int (*Sdqm_insert) (void *db_id, void *rec);
static int (*Sdqm_delete) (void *db_id, int ind);


/* local functions */
static void Pr_upd_ntf (EN_id_t evtcd, char *msg, int msglen, void *arg);
static void Cmd_delete_ntf (EN_id_t evtcd, char *msg, int msglen, void *arg);
static void Delete_products (int delayed_delete);
static void Update_pr_table ();
static int Search_pr_table (int prod_code, int elev, int *found);
static int Compare_prod_code_elev (Prod_retention_t *pr, 
						int prod_code, int elev);
static void Delete_a_prod (time_t cr_time, int delayed_delete, 
				LB_id_t msg_id, int rec_ind);
static void Report_storage_util ();
static int Process_loadshed (time_t cr_time);
static void Sort (int n, Sort_t *ra);

int RPGDBM_house_keeping (int state, int n_recs, int lb_fd, void *db_id);


/***********************************************************************

    The following part implements functions that are shared by rpgdbm
    and sdqs.

***********************************************************************/

/******************************************************************

    House keeping function for the RPG product DB.

    state: SDQM_HK_INIT: Initialization; SDQM_HK_BUILD: Builds data
    base; SDQM_HK_NEW_RECORD: A record received after building;
    SDQM_HK_ROUTINE: Routine call. SDQM_HK_TERM: sdqs is going to
    terimnated. n_recs: the max number of records if state =
    SDQM_HK_INIT or the current number of records otherwise. lb_fd: LB
    fd. db_id: SDQM db id.

    Returns the updated value of "n_recs".

******************************************************************/

int RPGDBM_house_keeping (int state, int n_recs, int lb_fd, void *db_id) {

    char *argv = "rpgdbm";

    Pdb_id = db_id;
    Plb_fd = lb_fd;
    N_products = n_recs;
    if (state == SDQM_HK_INIT) {		/* initializes */
	int max_prod_gen_rate;
	void **func_p;

	ORPGMISC_LE_init (1, &argv, 300, LB_SINGLE_WRITER, -1, 0);

	func_p = db_id;
	Sdqs_select_i = (int (*) (char *, int , int , 
					int , char *, void **))func_p[0];
	Sdqm_insert = (int (*) (void *, void *))func_p[1];
	Sdqm_delete = (int (*) (void *, int))func_p[2];

	Min_reten_time = 600;		/* minimum product retention time */
	max_prod_gen_rate = ORPGMISC_max_products ();
					/* number of products per minute */
	Maxn_products = N_products;
	N_products = 0;
	Update_pr_table ();
    
	max_prod_gen_rate = max_prod_gen_rate * 
		(HOUSEKEEPING_TIME + DELAYED_DEL_PERIOD) / DELAYED_DEL_PERIOD;
	N_products_high = Maxn_products - (Maxn_products * 2 / 100) - 4 - 
		max_prod_gen_rate;	/* reserve message slots for saving 
					   records and better efficiency */
	N_products_low = N_products_high - RANGE_BETWEEN_LOW_HIGH;
	if (N_products_low < MIN_PRODUCTS_LOW) {
	    LE_send_msg (GL_ERROR, "product DB too small\n");
	    exit (1);
	}
	LE_send_msg (LE_VL1, "at least %d products will be stored in DB\n", 
					N_products_low);
	return (N_products);
    }

    if (state == SDQM_HK_BUILD) {		/* builds data base */
	return (-1);
    }

    if (state == SDQM_HK_NEW_RECORD) {		/* A new record received */
	static int cnt = 0;

	cnt++;
	if ((cnt % 10) == 0)
	    LE_send_msg (LE_VL1, "%d new products added to DB\n", cnt);
	if (N_products > N_products_high)
	    Delete_products (1);
	return (N_products);
    }

    if (state == SDQM_HK_ROUTINE) {
	static time_t last_time = 0;
	static int task_ready_reported = 0;
	time_t cr_time;
	int i, ret, delay_time;

	Prod_db_initialized = 1;

	if (!task_ready_reported) {
	    if ((ret = EN_register (ORPGEVT_PROD_LIST, Pr_upd_ntf)) != 0 ||
		(ret = EN_register 
			    (ORPGEVT_PROD_DB_DELETE, Cmd_delete_ntf)) != 0) {
		LE_send_msg (GL_ERROR, "EN_register failed, ret %d\n", ret);
	    }
	    ORPGMGR_report_ready_for_operation ();
	    task_ready_reported = 1;
	}

	if (N_products_low < MIN_PRODUCTS_LOW)	/* not initialized */
	    return (N_products);
    	if (N_products > N_products_high)
	    Delete_products (1);

	if (Pr_updated) {
	    EN_control (EN_BLOCK);
	    Update_pr_table ();
	    Pr_updated = 0;
	    EN_control (EN_UNBLOCK);
	}
    
	cr_time = MISC_cr_time ();
	if (Cmd_delete) {
	    delay_time = 0;
	    Cmd_delete = 0;
	    LE_send_msg (GL_INFO, "Commanded product deletion\n");
	}
	else {
	    if (cr_time <= last_time + HOUSEKEEPING_TIME)
		return (N_products);
	    last_time = cr_time;
	    delay_time = DELAYED_DEL_PERIOD;
	}
    
	for (i = N_dels - 1; i >= 0; i--) {
	    if (cr_time - Dels[i].dtime >= delay_time) {
		if ((ret = LB_delete (Plb_fd, Dels[i].msg_id)) < 0)
		    LE_send_msg (GL_INFO | 20,  
			    "LB_delete product failed (ret %d)\n", ret);
		MISC_table_free_entry (Del_tid, i);
	    }
	}
    
	Report_storage_util ();
    }

    if (state == SDQM_HK_TERM) {	/* server is about to terminate */
	return (n_recs);
    }

    return (N_products);
}

/******************************************************************

    Description: This function generates the RPG product DB record
		structure from a product header.

    Input:	phd - the ORPG product header.
		msgid - product LB message ID.

    Output:	rec - returns the product DB record.

******************************************************************/

int RPGDBM_get_record (char *phd_p, LB_id_t msgid, char *rec_p) {
    Prod_header *phd;
    RPG_prod_rec_t *rec;
    int prod_id, ind, warehoused_time, found, i;
    time_t cr_t;

    cr_t = MISC_cr_time ();
    phd = (Prod_header *)phd_p;
    rec = (RPG_prod_rec_t *)rec_p;
    prod_id = phd->g.prod_id;
    rec->prod_code = ORPGPAT_get_code (prod_id);
    rec->vol_t = phd->g.vol_t;
    rec->gen_t = cr_t;	/* we use current time instead of the gen_t in the 
			   product header so we will be fine in case the clock
			   goes back significantly */
    rec->wx_mode = phd->wx_mode;
    rec->elev_ind = phd->g.elev_ind;
    rec->msg_id = msgid;
    ind = ORPGPAT_elevation_based (prod_id);
    if (ind < 0)
	rec->elev = SDQM_UNDEF_SHORT;
    else
	rec->elev = phd->g.resp_params[ind];
    if( (warehoused_time = ORPGPAT_get_warehoused( prod_id )) > 0 )
	rec->warehoused = prod_id;
    else
	rec->warehoused = 0;
    for (i = 0; i < PGM_NUM_PARAMS; i++) {
	rec->params[i] = phd->g.resp_params[i];
	rec->req_params[i] = phd->g.req_params[i];
    }

    ind = Search_pr_table (rec->prod_code, rec->elev, &found);
    if (found)
	rec->reten_t = cr_t + Prs[ind].pr_period;
    else{

	if( rec->warehoused > 0 )
	   rec->reten_t = cr_t + warehoused_time;
	else
	   rec->reten_t = cr_t + Min_reten_time;
    }
    return (0);
}

/******************************************************************

    Description: This function generates and updates the product
		retention table. The table is sorted in terms of
		product code and elevation.

******************************************************************/

static void Update_pr_table () {
    int cnt, np, i;

    if (Pr_tid == NULL) {
	Pr_tid = MISC_open_table (
		sizeof (Prod_retention_t), 128, 1, &N_prs, (char **)&Prs);
	if (Pr_tid == NULL) {
	    LE_send_msg (GL_ERROR | 38,  "Malloc failed\n");
	    exit (1);
	}
    }

    np = ORPGPGT_get_tbl_num (ORPGPGT_CURRENT_TABLE);
    if (np < 0) {
	LE_send_msg (GL_ERROR, "reading product retention table failed\n");
	exit (1);
    }

    cnt = 0;
    for (i = 0; i < np; i++) {
	int prod_id, prod_code, ind, elev, k, found;
	Prod_retention_t *new;

	prod_id = ORPGPGT_get_prod_id (ORPGPGT_CURRENT_TABLE, i);
	prod_code = ORPGPAT_get_code (prod_id);
	if (prod_code <= 0)
	    continue;
	ind = ORPGPAT_elevation_based (prod_id);
	if (ind < 0)
	    elev = SDQM_UNDEF_SHORT;
	else
	    elev = ORPGPGT_get_parameter (ORPGPGT_CURRENT_TABLE, i, ind);
	k = Search_pr_table (prod_code, elev, &found);
	if (found) {
	    Prs[k].pr_period = 
		ORPGPGT_get_retention_period (ORPGPGT_CURRENT_TABLE, i) * 60;
	    continue;
	}

	if (k < 0)
	    k = 0;
	new = (Prod_retention_t *)MISC_table_new_entry (Pr_tid, NULL);
	if (new == NULL) {
	    LE_send_msg (GL_ERROR | 40,  "malloc failed\n");
	    exit (1);
	}
	if (k < N_prs - 1)
	    memmove (Prs + k + 1, Prs + k, 
				(N_prs -1 - k) * sizeof (Prod_retention_t));
	Prs[k].prod_code = prod_code;
	Prs[k].elev = elev;
	Prs[k].pr_period = 
		ORPGPGT_get_retention_period (ORPGPGT_CURRENT_TABLE, i) * 60;
    }
    LE_send_msg (LE_VL1, 
		"product retention time table updated (%d products)\n", N_prs);
}

/*************************************************************
			
    Description: This function performs a binary search to find
		the first entry in the product retention table
		that has equal or greater prod_code and elevation. 
		If the table is empty it returns -1. It may return
		an index beyond the last table element.

    Input:	prod_code - product code;
		elev - elevation angle.

    Output:	found - boolean: An exact match has been found.

    Returns:	The table index found.

**************************************************************/
 
static int Search_pr_table (int prod_code, int elev, int *found) {
    int st, end;

    *found = 0;
    if (N_prs <= 0)
	return (-1);
    st = 0;
    end = N_prs - 1;

    while (1) {
	int ind, ret;

	ind = (st + end) >> 1;
	if (st == ind) {

	    ret = Compare_prod_code_elev (Prs + st, prod_code, elev);
	    if (ret == EQUAL_TO)
		*found = 1;
	    if (ret != LESS_THAN)
		return (st);
	    ret = Compare_prod_code_elev (Prs + end, prod_code, elev);
	    if (ret == EQUAL_TO)
		*found = 1;
	    if (ret == LESS_THAN)
		return (end + 1);
	    return (end);
	}
	if (Prs[ind].prod_code > prod_code ||
		(Prs[ind].prod_code == prod_code && 
			elev != SDQM_UNDEF_SHORT && Prs[ind].elev > elev))
	    end = ind;
	else
	    st = ind;
    }
}

/*************************************************************
			
    Description: This function compares the product code and elev
		in product retention time table "pr" with 
		"prod_code" and "elev".

    Input:	pr- product retention table entry.
		prod_code - product code;
		elev - elevation angle.

    Returns:	GREATER_THAN, EQUAL_TO or LESS_THAN.

**************************************************************/

static int Compare_prod_code_elev (Prod_retention_t *pr, 
						int prod_code, int elev) {

    if (pr->prod_code > prod_code)
	return (GREATER_THAN);
    else if (pr->prod_code == prod_code) {
	return (EQUAL_TO);
/*
	if (elev == SDQM_UNDEF_SHORT)
	    return (EQUAL_TO);
	else {
	    if (pr->elev > elev)
		return (GREATER_THAN);
	    else if (pr->elev == elev)
		return (EQUAL_TO);
	    else
		return (LESS_THAN);
	}
*/
    }
    else
	return (LESS_THAN);
}

/******************************************************************

    Description: This function deletes products until the product
		number is below N_products_low.

    Input:	delayed_delete - boolean: whether actual product 
			removal from the product LB need to be 
			delayed.

******************************************************************/

static void Delete_products (int delayed_delete) {
    SDQM_index_results *qr;	/* query results */
    time_t cr_time;
    int del_cnt, ret, i;
    char where[128];

    if (N_products <= N_products_low)
	return;
    if (N_products > N_products_high + (Maxn_products - N_products_high) / 2)
	delayed_delete = 0;

    cr_time = MISC_cr_time ();
    Process_loadshed (cr_time);
 
    del_cnt = 0;

    /* query expired products. The results are sorted on retention_time 
       because the way we specify the product DB */
    sprintf (where, "reten_t >= %d and reten_t <= %d", 
						-0x7fffffff, 0x7fffffff);
    ret = Sdqs_select_i ("rpg_prod_db", 0, N_products - N_products_low, 
				1, where, (void **)(&qr));
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 29,  
		    "Sdqm_execute_query_index failed, ret %d\n", ret);
	exit (1);
    }

    for (i = 0; i < qr->n_recs; i++) {
	char *rec;

	rec = (char *)qr->rec_add + qr->list[i] * qr->rec_size;
	Delete_a_prod (cr_time, delayed_delete, 
				*((LB_id_t *)rec), qr->list[i]);
	del_cnt++;
	if (N_products <= N_products_low)
	    break;
    }
    if (qr != NULL)
	free (qr);

    if (del_cnt > 0) {
	LE_send_msg (LE_VL1 | 31,  "%d products deleted\n", del_cnt);
    }
}

/******************************************************************

    Description: This function deletes a product from the DB.

    Input:	cr_time - current time.
		delayed_delete - boolean: whether actual product 
			removal from the product LB need to be 
			delayed.
		msg_id - product LB message ID.
		rec_ind - record index in DB.

******************************************************************/

static void Delete_a_prod (time_t cr_time, int delayed_delete, 
				LB_id_t msg_id, int rec_ind) {
    int ret;

    if (Del_tid == NULL) {
	Del_tid = MISC_open_table (
		sizeof (Prod_deletion_t), 64, 0, &N_dels, (char **)&Dels);
	if (Del_tid == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed\n");
	    exit (1);
	}
    }

    if ((ret = Sdqm_delete (Pdb_id, rec_ind)) < 0) {
        LE_send_msg (GL_ERROR, 
			"Sdqm_delete product failed (ret %d)\n", ret);
    }

    if (!delayed_delete) {
	if ((ret = LB_delete (Plb_fd, msg_id)) < 0)
	    LE_send_msg (GL_ERROR,  
		"LB_delete product failed (msgid %d, ret %d)\n", msg_id, ret);
    }
    else {		/* add to the deletion table */
	Prod_deletion_t *new;

	new = (Prod_deletion_t *)MISC_table_new_entry (Del_tid, NULL);
	if (new == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed\n");
	    exit (1);
	}
	new->dtime = cr_time;
	new->msg_id = msg_id;
    }
    N_products--;
}

/******************************************************************

    Description: This is the EN callback function.

    Inputs:	See EN man-page.

******************************************************************/

static void Pr_upd_ntf (EN_id_t evtcd, char *msg, int msglen, void *arg) {
    int *ipt;

    ipt = (int *)msg;
    if (msglen == 4 && *ipt == ORPGPGT_CURRENT_TABLE)
	Pr_updated = 1;
}

/******************************************************************

    Description: This is the EN callback function.

    Inputs:	See EN man-page.

******************************************************************/

static void Cmd_delete_ntf (EN_id_t evtcd, char *msg, int msglen, void *arg) {
    Cmd_delete = 1;
}

/******************************************************************

    Description: Reports the storage utilization periodically.

******************************************************************/

static void Report_storage_util () {
    static time_t last_report_time = 0;
    int util, n_unexpired, ret;
    time_t cr_time;

    cr_time = MISC_cr_time ();
    if (cr_time <= last_report_time + UTIL_REPORT_PERIOD)
	return;
    last_report_time = cr_time;

    n_unexpired = Process_loadshed (cr_time);
    util = n_unexpired * 100 / N_products_low;
    if (util > 100)
	util = 100;

    ret = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE, 
					LOAD_SHED_CURRENT_VALUE, util);
		/* this function processes loadshed warning and alarms */
    if (ret < 0)
	LE_send_msg (GL_ERROR | 43, "ORPGLOAD_set_data failed (ret %d)", ret);
    return;
}

/******************************************************************

    Processes product storage loadshed. Loadshed products are marked
    as expired in the database. Since we don't actually change 
    products in the product data store, we will have a load shed 
    activity if we restart rpgdbm. To eliminate this problem, we
    don't report load shed before we receive any new product.

    Input:	cr_time - current time.

    Return:	The number of unexpired products.

******************************************************************/

static int Process_loadshed (time_t cr_time) {
    SDQM_index_results *qr;	/* query results */
    int alarm, warning;
    int n_unexpired, cnt,ret;
    char where[128];

    if (ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_STORAGE, 
			    LOAD_SHED_WARNING_THRESHOLD, &warning) != 0 ||
	ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_STORAGE, 
			    LOAD_SHED_ALARM_THRESHOLD, &alarm) != 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGLOAD_get_data LOAD_SHED_CATEGORY_PROD_STORAGE failed");
	alarm = warning = 100;
    }

    /* query number of nonexpired products */
    sprintf (where, "reten_t >= %d and reten_t <= %d", 
					(int)cr_time, 0x7fffffff);
    ret = Sdqs_select_i ("rpg_prod_db", 0, Maxn_products, 
				1, where, (void **)&qr);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 42, 
		"Sdqm_execute_query_index failed, ret %d\n", ret);
	return (0);
    }

    n_unexpired = qr->n_recs_found;

    cnt = 0;
    if (n_unexpired >= alarm * N_products_low / 100) {	/* load shed */
	int i, n_warning;
	Sort_t *gs;

	gs = malloc (qr->n_recs * sizeof (Sort_t));
	if (gs == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed\n");
	    return (0);
	}
	for (i = 0; i < qr->n_recs; i++) {
	    RPG_prod_rec_t *rec;
	    rec = (RPG_prod_rec_t *)
			((char *)qr->rec_add + qr->list[i] * qr->rec_size + 
					sizeof (LB_id_t));
	    gs[i].value = rec->gen_t;
	    gs[i].ind = qr->list[i];
	}
	Sort (qr->n_recs, gs);

	n_warning = warning * N_products_low / 100;

	for (i = 0; i < qr->n_recs; i++) {
	    RPG_prod_rec_t *r;
	    char *rec, buf[sizeof (RPG_prod_rec_t) + sizeof (LB_id_t)];

	    rec = (char *)(qr->rec_add) + gs[i].ind * qr->rec_size;
	    memcpy (buf, rec, qr->rec_size);
	    r = (RPG_prod_rec_t *)(buf + sizeof (LB_id_t));

	    r->reten_t = cr_time - 1;
	    if ((ret = Sdqm_delete (Pdb_id, gs[i].ind)) < 0) {
		LE_send_msg (GL_ERROR | 34,  
				"Sdqm_delete product failed (ret %d)\n", ret);
		free (gs);
		return (0);
	    }
	    if ((ret = Sdqm_insert (Pdb_id, buf)) < 0) {
		LE_send_msg (GL_ERROR | 28,  
				"Sdqm_insert product failed (ret %d)\n", ret);
		free (gs);
		return (0);
	    }

	    n_unexpired--;
	    cnt++;
	    if (n_unexpired < n_warning)
		break;
	}
	free (gs);
    }
    if (qr != NULL)
	free (qr);

    if (Prod_db_initialized && cnt > 0) {
	LE_send_msg (GL_STATUS | LE_RPG_AL_LS | 32,  
		"On-line Storage OVERLOAD CRITICAL, PRODUCTS SHED");
    }
    if (cnt > 0)
	LE_send_msg (LE_VL1, "  Storage load shed %d products", cnt);

    return (n_unexpired);
}

/********************************************************************
			
    Description: This is the Heapsort algorithm from "Numerical recipes
		in C". Refer to the book.

    Input:	n - array size.
		ra - array to sort into ascent order.

********************************************************************/

static void Sort (int n, Sort_t *ra) {
    int l, j, ir, i;
    Sort_t rra;				/* type dependent */

    if (n <= 1)
	return;
    ra--;
    l = (n >> 1) + 1;
    ir = n;
    for (;;) {
	if (l > 1)
	    rra = ra[--l];
	else {
	    rra = ra[ir];
	    ra[ir] = ra[1];
	    if (--ir == 1) {
		ra[1] = rra;
		return;
	    }
	}
	i = l;
	j = l << 1;
	while (j <= ir) {
	    if (j < ir && ra[j].value < ra[j+1].value)	/* type dependent */
		++j;
	    if (rra.value < ra[j].value) {		/* type dependent */
		ra[i] = ra[j];
		j += (i = j);
	    }
	    else
		j = ir + 1;
	}
	ra[i] = rra;
    }
}

