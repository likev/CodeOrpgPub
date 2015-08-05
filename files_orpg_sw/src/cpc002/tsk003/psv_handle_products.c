
/******************************************************************

	file: psv_handle_products.c

	This module handles the one-time and routine product 
	distribution.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 14:55:27 $
 * $Id: psv_handle_products.c,v 1.135 2014/05/13 14:55:27 steves Exp $
 * $Revision: 1.135 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <prod_user_msg.h>
#include <prod_status.h>
#include <orpg.h>
#include <orpgdat.h>
#include <orpgevt.h>
#include <orpgerr.h>
#include <prod_gen_msg.h>
#include <a309.h>
#include <rpg_vcp.h>
#include <orpgprq.h>

#include "psv_def.h"

#define AFTER_VOLUME_PROCESSING_TIME	60
				/* time, in number of seconds after volume
				   scan ended, at which the product generation
				   is assumed to be failed */

enum {				/* values for Routine_prod_item.type */
    RPI_ELEV_PROD, 		/* elevation based product */
    RPI_VOL_PROD, 		/* volume based product */
};

typedef struct {
    prod_id_t prod_id;		/* buffer number of the product */
    short n_total;		/* total number distributed */
    short seq_number;		/* request sequence number */
    char wx_modes;		/* applicable weather modes: setting bit n 
				   (0, 1, 2, ... ; 0 is LSB) indicates that 
				   this entry is applied to weather mode n */ 
    char period;		/* distribution period in number of volumes */
    char number;		/* total number of products to be distributed. 
				   0 means infinity */
    char map_requested;		/* yes/no. maps are distributed with the 
				   product */
    char priority;		/* 1 (high) / 0 (low); distribution priority */
    char type;			/* product item type: RPI_ELEV_PROD or 
				   RPI_VOL_PROD */
    char aborted;		/* distribution aborted */
    short params[NUM_PROD_DEPENDENT_PARAMS];
				/* product dependent parameters */
    unsigned int pvnum;		/* current distri. volume number; We set this
				   to the current volume number when an 
				   item is put in the table. This will allow 
				   distribution started from this volume. */

    /* the following is used by elevation product only */
    int vcp;			/* vcp for the following info */
    unsigned int elev_vol;	/* volume number for the following info */
    short ep_ind;		/* elevation parameter index */	
    short n_elevs;		/* number of elevations requested */
    short elev_req;		/* elevation parameter in the request */
    short elev_ind;		/* current elevation index */
    short elevs[MAX_ELEVATION_CUTS];	/* elevation angles requested */
    char elev_inds[MAX_ELEVATION_CUTS];	/* elevation indexes requested */
    char distributed[MAX_ELEVATION_CUTS];
				/* product for this elevation and volume is 
				   distributed */
} Routine_prod_item;

typedef struct {		/* local part of the User_struct */
    int n_rt;			/* number of products in the routine 
				   distribution list */
    int n_buf;			/* buffer size for rt_list */
    Routine_prod_item *rt_list;	/* the routine distribution list */
    int pd_enabled;		/* product distribution enable flag. This is
				   used for disabling distribution of products
				   generated before connection. */
    unsigned int rps_vnum;	/* volume number when current RPS received */

    Prod_gen_status_header prev_hd;
				/* previously processed product status hd */
    Prod_gen_status *prev_stat;	/* previously processed product status list */
    int prev_stat_len;		/* length of prev_stat; -1 indicates that the
				   previously processed product status is not 
				   available */
    int ps_buf_size;		/* size of the buffer for prev_stat */
    short *changed;		/* the indices of changed status */
} Hp_local;


static int N_users;		/* number of users (links) */
static User_struct **Users;	/* the user structure list */
static int P_server_index;	/* instance index of this p_server */

static int Prod_status_updated = 1;
				/* product status msg has been updated */

enum {RPAP_NOT_FOUND, RPAP_DISTRI_ABORTED, RPAP_DISTRI_OK, RPAP_DISTRI_THROWN};
				/* Routine_process_a_product return values */

/* local functions */
static void Init_routine_prod_table (User_struct *usr);
static void Update_routine_prod_table (
			User_struct *usr, int n_reqs, char *umsg);
static int Parameters_match (User_struct *usr, short *req_params, 
				short *gen_params, int prod_id);
static int Get_updated_prod_gen_status (User_struct *usr, 
	Prod_gen_status *stat, int stat_len, Prod_gen_status_header *hd);
static void Print_RT_list (User_struct *usr, Routine_prod_item *list, int cnt);
static void Send_dd_for_scheduling (User_struct *usr);
static void Check_distribution_completion (User_struct *usr);
static int Search_for_first_entry (Prod_gen_status *stat, 
						int stat_len, int prod_id);
static int Allocate_list_buffer (User_struct *usr, int new_size);
static void Init_elev_fields (Routine_prod_item *entry);
static void Notify_prod_gen_failure (User_struct *usr, Routine_prod_item *prod,
				int seq_number, char *reason, int err_code);
static int Get_elevation (int prod_id, short *params);
static int Routine_process_a_product (Prod_gen_status *stat, int stat_len, 
	User_struct *usr, Routine_prod_item *list, int vind);
static int Retrieve_elevation_angles (Routine_prod_item *list, 
					int this_vnum, int this_vcp);
static int Is_product_already_distributed (User_struct *usr, 
						Routine_prod_item *item);
static void Process_unreported_prod (User_struct *usr, 
				Hp_local *hp, Routine_prod_item *list);


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.
		p_server_index - index of this p_server instance.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int HP_initialize (int n_users, User_struct **users, int p_server_index)
{
    int i;

    N_users = n_users;
    Users = users;
    P_server_index = p_server_index;

    /* allocate local data structure */
    for (i = 0; i < n_users; i++) {
	Hp_local *hp;

	hp = malloc (sizeof (Hp_local));
	if (hp == NULL) {
	    LE_send_msg (GL_ERROR | 21,  "malloc failed");
	    return (-1);
	}
	users[i]->hp = hp;
	hp->n_rt = hp->n_buf = 0;
	hp->rt_list = NULL;
	hp->pd_enabled = 1;
	hp->prev_stat = NULL;
	hp->prev_stat_len = -1;
	hp->ps_buf_size = 0;
	hp->changed = NULL;
    }

    ORPGDA_open (ORPGDAT_OT_RESPONSE + P_server_index, LB_READ);

    return (0);
}

/**************************************************************************

    Description: This function checks if product "prod_id" is requested
		for routine distribution by user "usr".

    Inputs:	usr - the user involved.
		prod_id - the product ID.

    Return:	The request sequence number if requested or -1 if not.

**************************************************************************/

int HP_routine_requested (User_struct *usr, int prod_id)
{
    Hp_local *hp;
    int k;

    if (usr->psv_state != ST_ROUTINE)
	return (-1);

    hp = (Hp_local *)usr->hp;
    for (k = 0; k < hp->n_rt; k++) {	/* for every product in the users 
					   routine prod distri list */
	Routine_prod_item *list;	

	list = hp->rt_list + k;
	if (list->prod_id == prod_id) {
	    return (list->seq_number);
	}
    }

    return (-1);
}

/**************************************************************************

    Description: This function handles new routine products. All new 
		products generated after the previous distribution are 
		checked against the routine product tables for all users. 
		If a match is found, the new product is sent to the user. 

**************************************************************************/

void HP_handle_routine_products ()
{
    int i;

    for (i = 0; i < N_users; i++) {/* send new routine products to the users */
	Prod_gen_status_header *hd;
 	Prod_gen_status *stat;
	int stat_len, prod_sent;
	unsigned int *vnum;
	User_struct *usr;
	Hp_local *hp;
	int k;

	usr = Users[i];
	hp = (Hp_local *)usr->hp;
	prod_sent = 0;
	if (usr->psv_state != ST_ROUTINE)
	    continue;	/* product distributed only in state ST_ROUTINE */

	/* read the product generation status */
	if (hp->pd_enabled == 0 && hp->prev_stat_len >= 0) {
	    stat_len = hp->prev_stat_len;
	    hd = &(hp->prev_hd);
	    stat = hp->prev_stat;
	}
	else {
	    if ((stat_len = HP_read_prod_gen_status (&stat, &hd)) < 0)
		return;
/*	    stat_len = Get_updated_prod_gen_status 
					    (usr, stat, stat_len, hd); */
	}
	vnum = hd->vnum;

	for (k = 0; k < hp->n_rt; k++) {/* for every product in the users 
					   routine prod distri list */
	    Routine_prod_item *list;	
	    int n, st, vind, ret;
	    time_t sys_vtime;
	    int prod_id, seq_number, this_vnum, vol_done;

	    list = hp->rt_list + k;

	    if (list->pvnum > vnum[0] ||	/* already distributed */
		(list->number > 0 && list->n_total >= list->number) ||
						/* distribution completed */
		hd->vdepth <= 0)		/* no stat info */
		continue;

	    /* which volume to work in */
	    for (n = 1; n < hd->vdepth; n++) {
		if (list->pvnum > vnum[n]) {
		    break;
		}
	    }
	    if (n >= hd->vdepth) {
		Process_unreported_prod (usr, hp, list);
		(list->pvnum)++;
		k--;
		continue;
	    }
	    vind = n - 1;
	    this_vnum = vnum[vind];
	    list->pvnum = this_vnum;
	    prod_id = list->prod_id;
	    seq_number = list->seq_number;

	    if (list->type == RPI_ELEV_PROD && list->elev_vol != this_vnum &&
			Retrieve_elevation_angles (list, 
					this_vnum, hd->vcpnum[vind]) < 0) {
		list->pvnum = this_vnum + 1;
		if (vind > 0)
		    k--;
		continue;
	    }

	    if (!(list->wx_modes & (1 << hd->wx_mode[vind]))) {
						/* wx mode doesn't match */
		list->pvnum = this_vnum + 1;
		if (vind > 0)
		    k--;
		continue;
	    }

	    if ((this_vnum % list->period) != 0) {/* we skip this volume */
		list->pvnum = this_vnum + 1;
		if (vind > 0)
		    k--;
		continue;
	    }

	    /* The volume is too old. We don't wait any longer */
	    if (this_vnum + 4 < vnum[0] ||	/* too many new volumes */
		(this_vnum < RRS_get_volume_number (&sys_vtime, NULL) && 
		sys_vtime + AFTER_VOLUME_PROCESSING_TIME < usr->time)) {

		Process_unreported_prod (usr, hp, list);
		list->pvnum = this_vnum + 1;
		if (vind > 0)
		    k--;
		continue;
	    }

	    st = Search_for_first_entry (stat, stat_len, prod_id);
			/* binary search for the first prod_id in stat */

	    vol_done = 0;
	    if (list->type == RPI_ELEV_PROD) {	/* elevation product */
		int cnt, ei;
	
		cnt = 0;
		for (ei = 0; ei < list->n_elevs; ei++) {
	    
		    if (list->distributed[ei])
			cnt++;
		    else {
			list->params[list->ep_ind] = list->elevs[ei];
			list->elev_ind = list->elev_inds[ei];
			ret = Routine_process_a_product (
					stat + st, stat_len - st, 
					usr, list, vind);

			if (ret == RPAP_DISTRI_OK)
			    prod_sent = 1;
			if (ret == RPAP_DISTRI_ABORTED)
			    list->aborted = 1;
			if (ret != RPAP_NOT_FOUND) {
			    list->distributed[ei] = 1;
			    cnt++;
			}
		    }
		}
		if (cnt >= list->n_elevs)
		    vol_done = 1;
	    }
	    else {				/* volume product */
		ret = Routine_process_a_product (stat + st, stat_len - st, 
						usr, list, vind);
		if (ret == RPAP_DISTRI_OK)
		    prod_sent = 1;
		if (ret == RPAP_DISTRI_ABORTED)
		    list->aborted = 1;
		else
		    list->aborted = 0;
		if (ret != RPAP_NOT_FOUND)
		    vol_done = 1;
	    }
	    if (vol_done) {
		if (!(list->aborted)) {
		    list->n_total++;
		}
		list->pvnum = this_vnum + 1;
		if (vind > 0)
		    k--;
	    }
	}			/* for each routine product in the list */

	if (prod_sent)
	    HWQ_comm_load_shed (usr); /* calculate util and do NB load shed */

	if (usr->up->cntl & UP_CD_DAC)
	    Check_distribution_completion (usr);
    }				/* for each user */

    return;
}

/**************************************************************************

    Processes the condition that the product genenration info of product 
    in "list" is not going to be available to the p_server.

**************************************************************************/

static void Process_unreported_prod (User_struct *usr, 
				Hp_local *hp, Routine_prod_item *list) {

    if (!hp->pd_enabled)
	return;
    if (hp->rps_vnum == list->pvnum)
	LE_send_msg (LE_VL1, 
	    "p_server times out (prod_id %d) in first volume", 
					    list->prod_id);
    else {
	if (list->type == RPI_ELEV_PROD) {
	    int ei;
	    for (ei = 0; ei < list->n_elevs; ei++) {
		if (!(list->distributed[ei])) {
		    list->params[list->ep_ind] = list->elevs[ei];
		    Notify_prod_gen_failure (
			usr, list, list->seq_number, 
			"p_server times out", RR_NOT_GENERATED);
		}
	    }
	}
	else
	    Notify_prod_gen_failure (usr, list, list->seq_number, 
		    "p_server times out", RR_NOT_GENERATED);
    }
}

/**************************************************************************

    Searches the new product status table for a routine distribution 
    product and processes the product.

    Inputs:	stat - First item in the product status list that matches
			the product id.
		stat_len - Total number of items after "stat".
		usr - The user involved.
		list - The routine distirbution entry involved.
		vind - The volume index (into array vnum).
		vol_num - Volume number involved.

    Return:	Returns RPAP_NOT_FOUND if no matched product ID is found 
		in the product generation status list, RPAP_DISTRI_ABORTED
		if the product is known to be not generated, RPAP_DISTRI_OK
		if the product is generated and distributed, 
		RPAP_DISTRI_THROWN if the product is generated but does not
		need to be distributed.

**************************************************************************/

static int Routine_process_a_product (Prod_gen_status *stat, int stat_len, 
	User_struct *usr, Routine_prod_item *list, int vind) {
    Hp_local *hp;
    int prod_id, seq_number;
    int n;

    hp = (Hp_local *)usr->hp;
    prod_id = list->prod_id;
    seq_number = list->seq_number;
    for (n = 0; n < stat_len; n++) {	/* go through the prod gen table */
	int msg_id;
	int msg_priority;
	Prod_gen_status *stn;

	stn = stat + n;
	if (stn->prod_id != prod_id)
	    return (RPAP_NOT_FOUND);
	if (stn->gen_pr == 0)
	    continue;
	msg_id = stn->msg_ids[vind];
	if (msg_id == PGS_SCHEDULED || msg_id == PGS_UNKNOWN ||
			msg_id == PGS_NOT_SCHEDULED)
	    continue;			/* wait */
	if (!(Parameters_match (usr, list->params, stn->params, prod_id)))
	    continue;
	if (list->type == RPI_ELEV_PROD && stn->elev_index >= 0 &&
			stn->elev_index != list->elev_ind)
	    continue;

	switch (msg_id) {

/*		    case PGS_NOT_SCHEDULED:	*//* abort waiting */
	    case PGS_REQED_NOT_SCHED:
		return (RPAP_DISTRI_ABORTED);

	    case PGS_VOLUME_ABORTED:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_VOLUME_ABORTED", RR_VOLUME_ABORTED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_MEMORY_LOADSHED:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_MEMORY_LOADSHED", RR_MEM_LOADSHED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_DISABLED_MOMENT:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_DISABLED_MOMENT", RR_MOMENT_DISABLED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_TIMED_OUT:
		if (hp->rps_vnum == list->pvnum)
		    LE_send_msg (LE_VL1, 
			"PGS_TIMED_OUT (prod_id %d) in first volume", 
							prod_id);
		else
		    Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_TIMED_OUT", RR_GEN_TIMED_OUT);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_TASK_NOT_RUNNING:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_TASK_NOT_RUNNING", RR_TASK_UNLOADED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_TASK_FAILED:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_TASK_FAILED", RR_TASK_FAILED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_INAPPR_WX_MODE:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_INAPPR_WX_MODE", RR_MOMENT_DISABLED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_SLOT_UNAVAILABLE:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_SLOT_UNAVAILABLE", RR_SLOT_FULL);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_INVALID_PARAMS:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_INVALID_PARAMS", RR_INVALID_PROD_PARAMS);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_DATA_SEQ_ERROR:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_DATA_SEQ_ERROR", RR_DATA_SEQUENCE_ERROR);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_TASK_SELF_TERM:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_TASK_SELF_TERM", RR_TASK_SELF_TERM);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_PRODUCT_DISABLED:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_PRODUCT_DISABLED", RR_NOT_GENERATED);
		return (RPAP_DISTRI_ABORTED);

	    case PGS_PRODUCT_NOT_GEN:
		Notify_prod_gen_failure (usr, list, seq_number,
			"PGS_PRODUCT_NOT_GEN", RR_NOT_GENERATED);
		return (RPAP_DISTRI_ABORTED);

	    default:
		if (list->priority != 0)
		   msg_priority = HWQ_TYPE_HIGH_PRIO;
		else
		   msg_priority = 0;
		   
		if (msg_id <= LB_MAX_ID) {	/* new product generated */

		    if (!hp->pd_enabled ||
			Is_product_already_distributed (usr, list))
			return (RPAP_DISTRI_THROWN);
				/* throw products generated before request or 
				   already distributed */
		    else {
			HP_send_a_product (usr, prod_id, 
				ORPGDAT_PRODUCTS, msg_id, list->pvnum,
				seq_number, msg_priority, 
				Get_elevation (prod_id, list->params),
				-1, 0);
			return (RPAP_DISTRI_OK);
		    }
		}
		else {
		    LE_send_msg (0, "product %d failure flag %x not notified", 
				prod_id, msg_id);
		    return (RPAP_DISTRI_ABORTED);
		}
		break;		/* here never reached */
	}
	break;			/* here never reached */
    }
    return (RPAP_NOT_FOUND);
}

/**************************************************************************

    Retrieves requested elevation angles and sets the elevation fields
    in the routine product request entry "list". It also resets the 
    distributed fields for every new elevation.

    Inputs:	list - the routine product distribution entry involved.
		this_vnum - the new volume number.
		this_vcp - the new vcp number.

    Returns 0 on success or -1 on failure.

**************************************************************************/

static int Retrieve_elevation_angles (Routine_prod_item *list, 
					int this_vnum, int this_vcp) {
    int i;

    {					/* reset elevation angles */
	short eind[MAX_ELEVATION_CUTS];

	list->n_elevs = ORPGPRQ_get_requested_elevations (this_vcp, 
			list->elev_req, MAX_ELEVATION_CUTS, this_vnum,
			list->elevs, eind);
	if (list->n_elevs < 0) {
	    LE_send_msg (GL_ERROR, "Failed in retrieving elevations\n");
	    return (-1);
	}
	list->vcp = this_vcp;
	for (i = 0; i < list->n_elevs; i++)
	    list->elev_inds[i] = eind[i];
    }

    for (i = 0; i < list->n_elevs; i++)
	list->distributed[i] = 0;
    list->aborted = 0;
    list->elev_vol = this_vnum;
    return (0);
}

/**************************************************************************

    Description: This function is called when a product scheduled for 
		distribution can not be generated. It sends an RR message 
		to the user and posts an LE message.

    Inputs:	usr - the user involved.
		prod - the product.
		seq_number - the request sequence number.
		reason - failure reason.
		err_code - error code for the RR message.

**************************************************************************/

static void Notify_prod_gen_failure (User_struct *usr, Routine_prod_item *prod,
				int seq_number, char *reason, int err_code)
{
    int prod_id;

    prod_id = prod->prod_id;
    if (!((Hp_local *)usr->hp)->pd_enabled ||
	Is_product_already_distributed (usr, prod))
	return;

    LE_send_msg (LE_VL1, "%s prod %d for volume %d", 
					reason, prod_id, prod->pvnum);
    HWQ_put_in_queue (usr, 0, 
	GUM_rr_message (usr, err_code, prod_id | THIS_IS_PROD_ID, 
			seq_number, 
			(ORPGMISC_vol_scan_num (prod->pvnum) << 16) | 
			Get_elevation (prod_id, prod->params)));
						/* send rr msg */
    return;
}

/**************************************************************************

    Checks if a product specified in an elevation wild card request has
    already been distributed. It returns 1 if the product has already 
    been distributed or 0 otherwise.

    Inputs:	usr - the user involved.
		item - the product request item.

**************************************************************************/

static int Is_product_already_distributed (User_struct *usr, 
						Routine_prod_item *item) {
    Hp_local *hp;
    int k;

    if (item->type != RPI_ELEV_PROD ||
	(item->elev_req & ORPGPRQ_ELEV_FLAG_BITS) == 0)
	return (0);			/* elevation wild card only */

    hp = (Hp_local *)usr->hp;
    for (k = 0; k < hp->n_rt; k++) {	/* for every item */
	Routine_prod_item *list;
	int i;
	short elev;

	list = hp->rt_list + k;
	if (list == item)		/* only check items in front of this */
	    break;

	if (list->prod_id != item->prod_id)
	    continue;

	elev = item->params[item->ep_ind];
	for (i = 0; i < list->n_elevs; i++) {
	    if (list->elevs[i] == elev && 
		(short)list->elev_inds[i] == item->elev_ind &&
		list->distributed[i])
		return (1);
	}
    }
    return (0);
}

/**************************************************************************

    Description: This function performs a binary search to find the first
		entry in the product status table for product "prod_id".

    Inputs:	stat - the product status table
		stat_len - the product status table size
		prod_id - the product ID to search for

    Return:	The index of the first entry of "prod_id" in the product 
		status table. It returns 0 if the table is empty. It 
		returns an index in the table if "prod_id" is not found.

**************************************************************************/

static int Search_for_first_entry (Prod_gen_status *stat, 
						int stat_len, int prod_id)
{
    int st;

    st = 0;
    if (stat_len > 0) {	
	int end, ind;

	end = stat_len - 1;
	ind = (st + end) / 2;
	while (ind != st) {
	    if (stat[ind].prod_id >= prod_id)
		end = ind;
	    else
		st = ind;
	    ind = (st + end) / 2;
	}
	if (stat[ind].prod_id < prod_id)
	    st = end;
    }
    return (st);
}

/**************************************************************************

    Description: This function checks whether the parameters of a 
		generated product match the parameters of a requested 
		product.

    Inputs:	usr - the user involved.
		req_params - the parameter array of product request.
		gen_params - the parameter array of generated product.
		prod_id - the product id.

    Return:	0 if the two parameter array do not match or non-zero
		if they match.

**************************************************************************/

static int Parameters_match (User_struct *usr, short *req_params, 
				short *gen_params, int prod_id) {
    int i;

    for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++) {
	int rp, gp;

	rp = req_params[i];
	gp = gen_params[i];

	if (rp == PARAM_UNUSED)
	    continue;
	else if (rp == PARAM_ALG_SET) {
	    if (gp != PARAM_ALG_SET)
		return (0);
	}
	else if (rp != gp)
	    return (0);
    }
    return (1);
}

/**************************************************************************

    Description: This function generates a product message and sends it
		to the user.

    Inputs:	usr - the user involved.
		prod_id - thr ORPG product id (buffer number);
		msg_id - msg ID as stored in LB;
		vnum - volume number;
		seq_number - request sequence number;
		high_prio - requested as high priority distribution 
			    if non-zero.
		elev - elevation angle in .1 degrees.
		dest_id - destination ID.
		one_time - if non-zero, it is an one-time product.

    Returns 0 on success or a negative error number.

**************************************************************************/

int HP_send_a_product (User_struct *usr, int prod_id, int lb_id, 
		int msg_id, unsigned int vnum, int seq_number, int prio, 
			int elev, int dest_id, time_t one_time)
{
    char *buf;
    Indirect_rpg_msg *indirect;
    int read_size, malloc_size, ret;
    Pd_prod_header *hd;
    Prod_header *orpg_phd;
    int pid;

    read_size = sizeof (Pd_prod_header);
    if (one_time)
	read_size = (WTMODOFF + 2) * sizeof (short);
		/* to get part of the prod descript block for weather mode */
    malloc_size = sizeof (Pd_prod_header) + sizeof (Indirect_rpg_msg);
		/* for Indirect_rpg_msg and prod_descrip_block_size */
    if (read_size > malloc_size)
	malloc_size = read_size;
    buf = WAN_usr_msg_malloc (malloc_size);
    if (buf == NULL)
	return (0);

    /* read the message header block */
    ret = ORPGDA_read (lb_id, buf - sizeof (Prod_header), 
			sizeof (Prod_header) + read_size, msg_id);
    if (ret <= 0 && ret != LB_BUF_TOO_SMALL) {
	LE_send_msg (GL_ERROR | 24,  
	"ORPGDA_read (prod_id %d, msg_id %d) failed (ret %d), L%d", 
					prod_id, msg_id, ret, usr->line_ind);
	WAN_free_usr_msg (buf);
	return (PSV_PRODUCT_LOST);
    }

    if (one_time != 0) {		/* check wx_mode permission */
	short *spt;
	short prod_wx_mode;
	int perm_wx_modes;

	spt = (short *)buf;
	prod_wx_mode = spt[WTMODOFF];
        /* For little endian machines we need to swap the value.  Note - this
           macro function will not swap if not necessary. */
        prod_wx_mode = SHORT_BSWAP_L(prod_wx_mode);
	HP_prod_permission_ok (usr, prod_id, PD_PMS_ONETIME, &perm_wx_modes);
	if (!(perm_wx_modes & (1 << prod_wx_mode))) {
	    WAN_free_usr_msg (buf);
	    LE_send_msg (GL_INFO, 
		"product %d rejected due to wx permission", prod_id);
	    return (PSV_FAILED);
	}
    }

    indirect = (Indirect_rpg_msg *)(buf + sizeof (Pd_prod_header));
    indirect->orpgdat = lb_id;
    indirect->msg_id = msg_id;
    indirect->vol_number = vnum;
    indirect->seq_number = seq_number;
    indirect->elev_angle = elev;
    indirect->req_time = one_time;

    hd = (Pd_prod_header *)buf;
    UMC_message_header_block_swap (hd);
    hd->dest_id = dest_id;

    /* verify the product message */
    pid = ORPGPAT_get_prod_id_from_code (hd->msg_code);
    orpg_phd = (Prod_header *)(buf - sizeof (Prod_header));
    if (prod_id != orpg_phd->g.prod_id || prod_id != pid) {
	LE_send_msg (GL_ERROR | 25,  "product IDs do not match (%d %d %d)\n", 
				prod_id, orpg_phd->g.prod_id, pid);
	WAN_free_usr_msg (buf);
	return (PSV_PRODUCT_LOST);
    }

    HP_set_to_use_db_read (orpg_phd, indirect);
    HWQ_put_in_queue (usr, HWQ_TYPE_INDIRECT | prio, buf);
 
    return (0);
}

/**************************************************************************

    Sets the "indirect" so we will read directly from data base instead of 
    product queue when we send the product to WAN.

    Input:	phd - ORPG product header.

    Input/Output:	indirect - the Indirect_rpg_msg struct.

**************************************************************************/

void HP_set_to_use_db_read (Prod_header *phd, Indirect_rpg_msg *indirect) {

    if (indirect->orpgdat < ORPGDAT_BASE && phd->g.len >= 0) {
						/* see orpgda.c */
	indirect->orpgdat = ORPGDAT_PRODUCTS;
	indirect->msg_id = phd->g.id;
    }
}

/**************************************************************************

    Description: This function handles new one-time requested products.
		Any new one-time requested product or failure info is 
		sent to the corresponding user. If request time is old 
		than the connect time, the product is discarded.

**************************************************************************/

void HP_handle_one_time_products ()
{
   
    while (1) {
	One_time_prod_req_response resp;
	int dest_id;
	int ret, ind;
	int msg_priority;
	int err_bit;

	ret = ORPGDA_read (ORPGDAT_OT_RESPONSE + P_server_index, (char *)&resp, 
				sizeof (One_time_prod_req_response), LB_NEXT);
	if (ret == LB_TO_COME)
	    return;
	if (ret == LB_EXPIRED) {
	    LE_send_msg (GL_ERROR, 
		"ORPGDA_read OT_RESPONSE (p_server %d) - message lost",
						P_server_index);
	    continue;
	}	
	if (ret != sizeof (One_time_prod_req_response)) {
	    LE_send_msg (GL_ERROR | 26,  
		"ORPGDA_read ORPGDAT_OT_RESPONSE failed (ret %d)", ret);
	    return;
	}
	LE_send_msg (LE_VL1 | 27,  "OT resp: error %d, L%d\n", 
					resp.error, resp.line_ind);

	/* find the user */
	for (ind = 0; ind < N_users; ind++) {
	    if (Users[ind]->line_ind == resp.line_ind)
		break;
	}
	if (ind >= N_users) {
	    LE_send_msg (GL_ERROR | 28,  
		"Bad line index in one-time response (%d)", resp.line_ind);
	    continue;
	}

	if (Users[ind]->psv_state != ST_ROUTINE)
			/* product distributed only in state ST_ROUTINE */
	    continue;	

	if (resp.req_time < Users[ind]->session_time) {	/* check time */
	    LE_send_msg (0, "old one-time response - discarded");
	    continue;
	}

	if (Users[ind]->up->cntl & UP_CD_MULTI_SRC)
	    dest_id = resp.src_id;
	else
	    dest_id = -1;
	if (resp.priority == OTR_ALERT_PRIORITY)
	   msg_priority = HWQ_TYPE_ALERT_PRIO |HWQ_TYPE_ONETIME;
	else if (resp.priority == OTR_HIGH_PRIORITY)
	   msg_priority = HWQ_TYPE_HIGH_PRIO | HWQ_TYPE_ONETIME;
	else 
	   msg_priority = HWQ_TYPE_ONETIME; 

	err_bit = -1;
	if (resp.error == OTR_PRODUCT_READY) {	/* send product */
	    ret = HP_send_a_product (Users[ind], (int)resp.prod_id, 
			(int)resp.lb_id, 
			(int)resp.msg_id, resp.vol_number, 
			(int)resp.seq_number, msg_priority, 
			(int)resp.elev, dest_id, resp.req_time);
	    if (ret >= 0)
		HWQ_comm_load_shed (Users[ind]); 
				/* calculate util and do NB load shed */
	    if (ret == PSV_FAILED)		/* send an RR msg */
		err_bit = RR_ILLEGAL_REQ;
	    else if (ret == PSV_PRODUCT_LOST)
		err_bit = RR_PROD_EXPIRED;
	}
	else {					/* send an RR msg */
	    switch (resp.error) {
		case OTR_CPU_LOAD_SHED:
		    err_bit = RR_CPU_LOADSHED;
		    break;

		case OTR_TASK_NOT_RUNNING:
		    err_bit = RR_TASK_UNLOADED;
		    break;

		case OTR_NEXT_VOLUME:
		    err_bit = RR_AVAIL_NEXT_VOL;
		    break;
		
		case OTR_PRODUCT_NOT_AVAILABLE:
                case OTR_PRODUCT_NOT_GEN:
		    err_bit = RR_NOT_GENERATED;
		    break;

		case OTR_TASK_NOT_STARTED:
		    err_bit = RR_TASK_UNLOADED;
		    break;

		case OTR_TASK_FAILED:
		    err_bit = RR_TASK_FAILED;
		    break;

		case OTR_TASK_SELF_TERM:
		    err_bit = RR_TASK_SELF_TERM;
		    break;

		case OTR_DISABLED_MOMENT:
		    err_bit = RR_MOMENT_DISABLED;
		    break;

		case OTR_MEM_LOADSHED:
		    err_bit = RR_MEM_LOADSHED;
		    break;

		case OTR_SLOT_FULL:
		    err_bit = RR_SLOT_FULL;
		    break;

		case OTR_INVALID_PARAMS:
		    err_bit = RR_INVALID_PROD_PARAMS;
		    break;

		case OTR_DATA_SEQ_ERROR:
		    err_bit = RR_DATA_SEQUENCE_ERROR;
		    break;

		default:
		    err_bit = RR_ONETIME_GEN_FAILED;
		    break;
	    }
	}
	if (err_bit >= 0) {
	    char *rr_msg;

	    rr_msg = GUM_rr_message (Users[ind], err_bit, 
			    (int)resp.prod_id | THIS_IS_PROD_ID, 
			    (int)resp.seq_number, 
			    (ORPGMISC_vol_scan_num (resp.vol_number) << 16) | 
							(int)resp.elev);
	    if (rr_msg != NULL) {		/* set the destination id */
		Pd_request_response_msg *rr;

		rr = (Pd_request_response_msg *)rr_msg;
		rr->mhb.dest_id = dest_id;
		HWQ_put_in_queue (Users[ind], 0, rr_msg);
	    }
	}
    }
}

/**************************************************************************

    Description: This function processes a user product request message.

    Inputs:	usr - the user involved.
		umsg - pointer to the user product request message.

**************************************************************************/

void HP_process_prod_request (User_struct *usr, char *umsg)
{
    Pd_msg_header *hd;
    Pd_request_products *req;
    int ret;
    int rtp;
    int k, cnt;
    Pd_request_products *r;
    int bad_msg;

    hd = (Pd_msg_header *)umsg;
    if (hd->n_blocks < 1)		/* unexpected request */
	return;
    hd->line_ind = usr->line_ind;	/* set the line index */
    hd->time = MISC_systime (NULL);
					/* reset the time for later response
					   verification */
    req = (Pd_request_products *)(umsg + ALIGNED_SIZE (sizeof (Pd_msg_header)));

    if (hd->n_blocks == 1 || req->num_products == -1) 
	rtp = 1;			/* a routine product request */
    else
	rtp = 0;			/* a one-time product request */

    if (rtp) {				/* cancel RPS timer */
	MT_cancel_timer (usr, EV_RPS_TIMER);
	if (PSR_status_log_code (usr) == GL_INFO)
	    LE_send_msg (GL_STATUS | LE_RPG_COMMS, 
	"Status Message suppression ended for line %d; line %d is CONNECTED",
				usr->line_ind, usr->line_ind);
	usr->status_msg_cnt = 0;
    }

    /* verify if the products are allowed for request */
    r = req;
    cnt = 0;
    bad_msg = 0;
    for (k = 0; k < hd->n_blocks - 1; k++) {
	int allowed, prod_id;

	if (rtp && r->num_products != -1) {	/* products must be either all
						   one-time or all routine */
	    LE_send_msg (LE_VL1,  
		    "req of mixed one-time and routine prods not allowed, L%d",
							usr->line_ind);
	    HWQ_put_in_queue (usr, 0, GUM_rr_message (usr, RR_ILLEGAL_REQ, 0, 
			(int)r->seq_number, 0));	/* send rr msg */
	    usr->bad_msg_cnt++;
	    return;
	}

	prod_id = ORPGPAT_get_prod_id_from_code (r->prod_id);
					/* convert to prod ID */

	if (prod_id < 0) {
	    LE_send_msg (LE_VL1, 
		"Product not permitted - unknown product code %d, L%d", 
					r->prod_id, usr->line_ind);
	    allowed = 0;
	}
	else {
	    int type;
	    r->prod_id = prod_id;
	    if (rtp)
		type = PD_PMS_ROUTINE;
	    else
		type = PD_PMS_ONETIME;
	    allowed = HP_prod_permission_ok (usr, prod_id, type, NULL);
	    if (!allowed)
		LE_send_msg (LE_VL1, "Product (%d) not permitted, L%d", 
					prod_id, usr->line_ind);
	}
	if (r->req_interval <= 0) {
	    allowed = 0;
	    LE_send_msg (LE_VL1, "Invalid request interval (%d), L%d", 
					r->req_interval, usr->line_ind);
	}

	if (allowed) {		/* check number of requested products */
	    int n_eles, ep_i;

	    ep_i = ORPGPAT_elevation_based (prod_id);
	    if (ep_i >= 0 && ep_i < NUM_PROD_DEPENDENT_PARAMS && 
				r->params[ep_i] != PARAM_UNUSED) {
		short eles[MAX_ELEVATION_CUTS];
		n_eles = ORPGPRQ_get_requested_elevations (-1, 
			r->params[ep_i], MAX_ELEVATION_CUTS, 0, eles, NULL);
		if (n_eles < 0) {
		    LE_send_msg (LE_VL1, 
		"Product (%d) not permitted - bad elevation spec (%x), L%d",
			prod_id, (unsigned int)r->params[ep_i], usr->line_ind);
		    allowed = 0;
		}
		else
		    cnt += n_eles;
	    }
	    else	
		cnt++;
	    if (rtp && cnt > usr->up->n_req_prods) {
		LE_send_msg (LE_VL1, 
	"Product (%d) not permitted - too many (%d > %d) requested, L%d", 
			prod_id, cnt, usr->up->n_req_prods, usr->line_ind);
		allowed = 0;
	    }
	}

	if (!allowed) {
	    if (prod_id < 0) {
	        HWQ_put_in_queue (usr, 0,	/* send rr msg */
			GUM_rr_message (usr, RR_NO_SUCH_PROD, 
				(int)r->prod_id, 
				(int)r->seq_number, 
				Get_elevation (r->prod_id, r->params)));
	    }
	    else {
	        HWQ_put_in_queue (usr, 0,	/* send rr msg */
			GUM_rr_message (usr, RR_ILLEGAL_REQ, 
				(int)r->prod_id | THIS_IS_PROD_ID, 
				(int)r->seq_number, 
				Get_elevation (r->prod_id, r->params)));
	    }
	    r->num_products = 0;	/* void this request */
	    bad_msg = 1;
	}
	else {
	    /* set NON_SCHEDULING_REQUEST_BIT flag */
	    r->flag_bits &= ~(NON_SCHEDULING_REQUEST_BIT | 
						ALERT_SCHEDULING_BIT);
	    if ((usr->up->cntl & UP_CD_NO_SCHEDULE) &&
		prod_id != CFCPROD)
		r->flag_bits |= NON_SCHEDULING_REQUEST_BIT;
	}
	r++;
    }
    if (bad_msg)
	usr->bad_msg_cnt++;
    else
	usr->bad_msg_cnt = 0;

    LE_send_msg (LE_VL2 | 33,
		"prod req msg received: len %d, n_prods %d, L%d\n", 
				UMC_product_length(hd), cnt, usr->line_ind);
    /* send to the appropriate scheduler */
    if (rtp) {
	Update_routine_prod_table (usr, cnt, umsg);
	ret = ORPGDA_write (ORPGDAT_RT_REQUEST, umsg, 
		ALIGNED_SIZE (sizeof (Pd_msg_header)) + 
		(hd->n_blocks - 1) * sizeof (Pd_request_products), 
		P_server_index);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR | 34,  
		"ORPGDA_write ORPGDAT_RT_REQUEST failed (ret = %d)", ret);
	    ORPGTASK_exit (1);
	}
	EN_post (ORPGEVT_RT_REQUEST, NULL, 0, EN_POST_FLAG_LOCAL);
    }
    else {
	ret = ORPGDA_write (ORPGDAT_OT_REQUEST, umsg, 
		ALIGNED_SIZE (sizeof (Pd_msg_header)) + 
		(hd->n_blocks - 1) * sizeof (Pd_request_products), 
		P_server_index);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR | 35,  
		"ORPGDA_write ORPGDAT_OT_REQUEST failed (ret = %d)", ret);
	    ORPGTASK_exit (1);
	}
	EN_post (ORPGEVT_OT_REQUEST, NULL, 0, EN_POST_FLAG_LOCAL);
    }

    return;
}

/**************************************************************************

    Description: This function initializes the routine product distribution
		table in terms of the default product distribution list. 
		Duplicated products in the list are not checked. They 
		will cause duplicated distribution. This function has not
		been updated to support the elevation wild card. This is
		deemed not necessary because class IV users are going away.

    Inputs:	usr - the user involved.

**************************************************************************/

static void Init_routine_prod_table (User_struct *usr)
{
    Hp_local *hp;
    Routine_prod_item *list;
    int i, vnum;
    Pd_prod_item *dd;

    if (usr->up->dd_len <= 0)
	return;

    if (Allocate_list_buffer (usr, usr->up->dd_len) < 0)
	return;

    hp = (Hp_local *)usr->hp;
    list = hp->rt_list;

    /* create the table from the default product distribution list */
    dd = (Pd_prod_item *)((char *)usr->up + usr->up->dd_list);
    vnum = RRS_get_volume_number (NULL, NULL);
    for (i = 0; i < usr->up->dd_len; i++) {
	list->prod_id = dd->prod_id;
	list->wx_modes = dd->wx_modes;
	list->period = dd->period;
	list->number = dd->number;
	list->map_requested = dd->map_requested;
	list->priority = dd->priority;
	list->seq_number = 0;
	memcpy ((char *)list->params, (char *)dd->params, 
				NUM_PROD_DEPENDENT_PARAMS * sizeof (short));
	list->n_total = 0;
	list->pvnum = vnum;
	Init_elev_fields (list);
	dd++;
	list++;
    }
    hp->n_rt = usr->up->dd_len;
    hp->rps_vnum = vnum;

    if (LE_local_vl (-1) >= 2)
	Print_RT_list (usr, hp->rt_list, hp->n_rt);

    return;
}

/**************************************************************************

    Description: This function initializes elevation fields of the routine 
	product distribution list entry.

    Inputs:	entry - the routine product distribution list entry.

**************************************************************************/

static void Init_elev_fields (Routine_prod_item *entry) {

    entry->ep_ind = ORPGPAT_elevation_based (entry->prod_id);
    if (entry->ep_ind >= 0 && entry->ep_ind < NUM_PROD_DEPENDENT_PARAMS && 
			entry->params[entry->ep_ind] != PARAM_UNUSED) {
	entry->type = RPI_ELEV_PROD;
	entry->elev_req = entry->params[entry->ep_ind];
	entry->n_elevs = 0;
    }
    else
	entry->type = RPI_VOL_PROD;
    entry->vcp = -1;
    entry->elev_vol = 0xffffffff;	/* impossible volume number */

    return;
}

/**************************************************************************

    Description: This function updates the routine product distribution
		table according a new user routine product request. 
		Duplicated products in the list are not checked. They 
		will cause duplicated distribution. Products already 
		distributed are checked and will not be re-distributed.

    Inputs:	usr - the user involved.
		n_reqs - number of requested products in message "umsg".
		umsg - pointer to the user message. NULL indicates that
			there is no user message.

**************************************************************************/

static void Update_routine_prod_table (
				User_struct *usr, int n_reqs, char *umsg)
{
    Hp_local *hp;
    int cnt, old_n_rt;
    Routine_prod_item *list, *old_rt_list;
    Pd_msg_header *hd;
    Pd_request_products *req;
    int i;

    hp = (Hp_local *)usr->hp;
    old_rt_list = hp->rt_list;
    old_n_rt = hp->n_rt;
    hp->rt_list = NULL;		/* save the list by removing from HP */
    hp->n_rt = hp->n_buf = 0;

    if (n_reqs < 0 || umsg == NULL ||
	Allocate_list_buffer (usr, n_reqs) < 0) {
	if (old_rt_list != NULL)
	    free ((char *)old_rt_list);
	return;
    }
    list = hp->rt_list;

    /* set up the table */
    cnt = 0;
    hd = (Pd_msg_header *)umsg;
    req = (Pd_request_products *)(umsg + 
				ALIGNED_SIZE (sizeof (Pd_msg_header)));
    for (i = 0; i < hd->n_blocks - 1; i++) {
	if (req->num_products != 0) {
	    unsigned int prev_voln;

	    if (cnt >= n_reqs) {
		LE_send_msg (GL_ERROR | 36,  
			"fatal error in Update_routine_prod_table");
		ORPGTASK_exit (1);
	    }
	    list->prod_id = req->prod_id;
	    list->wx_modes = 0xff;
	    list->period = req->req_interval;
	    list->number = 0; /* req->num_products is always -1 */
	    if (req->flag_bits & MAP_FLAG_BIT)
		list->map_requested = 1;
	    else
		list->map_requested = 0;
	    if (req->flag_bits & PRIORITY_FLAG_BIT)
		list->priority = 1;
	    else
		list->priority = 0;
	    list->seq_number = req->seq_number;
	    memcpy ((char *)list->params, (char *)req->params, 
				NUM_PROD_DEPENDENT_PARAMS * sizeof (short));
	    list->n_total = 0;
	    Init_elev_fields (list);

	    prev_voln = 0xffffffff;	/* find previous processing volume # */
	    if (old_n_rt > 0) {
	        int k;
		    
	        for (k = 0; k < old_n_rt; k++) {
		    Routine_prod_item *old;

		    old = old_rt_list + k;
		    if (old->prod_id == list->prod_id) {
		        if (old->type == RPI_ELEV_PROD)
			    old->params[old->ep_ind] = 
						list->params[old->ep_ind];
			if (memcmp (old->params, list->params, 
			    sizeof (short) * NUM_PROD_DEPENDENT_PARAMS) == 0 &&
			    old->pvnum < prev_voln)
			    prev_voln = old->pvnum;
		    }
		}
	    }
	    if (prev_voln == 0xffffffff)
	        list->pvnum = RRS_get_volume_number (NULL, NULL);
	    else
	        list->pvnum = prev_voln;

	    cnt++;
	    list++;
	}
	req++;
    }
    hp->n_rt = cnt;

    list = hp->rt_list;
    for (i = 0; i < cnt; i++) {
	int k;

	if (list[i].type != RPI_ELEV_PROD ||
	    (list[i].elev_req & ORPGPRQ_ELEV_FLAG_BITS) == 0)
	    continue;
	for (k = i + 1; k < cnt; k++) {
	    if (list[k].prod_id == list[i].prod_id &&
		(list[k].elev_req & ORPGPRQ_ELEV_FLAG_BITS) == 0) {
		Routine_prod_item t;

		memcpy (&t, list + i, sizeof (Routine_prod_item));
		memcpy (list + i, list + k, sizeof (Routine_prod_item));
		memcpy (list + k, &t, sizeof (Routine_prod_item));
		break;
	    }
	}
    }

    if (LE_local_vl (-1) >= 2)
	Print_RT_list (usr, hp->rt_list, cnt);

    if (old_rt_list != NULL)
	free ((char *)old_rt_list);

    /* we call HP_handle_routine_products to disable distribution of newly
       RPS requested products that are already generated */
    hp->pd_enabled = 0;
    HP_handle_routine_products ();
    hp->pd_enabled = 1;

    hp->rps_vnum = RRS_get_volume_number (NULL, NULL);

    return;
}

/**************************************************************************

    Description: This function generates a routine product request message
		based on the default distribution list and sends the message
		to the ps_routine for scheduling.

    Inputs:	usr - the user involved.

**************************************************************************/

static void Send_dd_for_scheduling (User_struct *usr)
{
    int size;
    int i, ret;
    char *msg;
    Pd_msg_header *hd;
    Pd_request_products *req;
    Pd_prod_item *dd;
    time_t tm;

    if (usr->up->dd_len <= 0)
	return;

    /* the space for the message */
    size = ALIGNED_SIZE (sizeof (Pd_msg_header)) + 
			usr->up->dd_len * sizeof (Pd_request_products);
    msg = malloc (size);
    if (msg == NULL) {
	LE_send_msg (GL_ERROR | 37,  "malloc failed");
	return;
    }

    hd = (Pd_msg_header *)msg;
    req = (Pd_request_products *)(msg + 
				ALIGNED_SIZE (sizeof (Pd_msg_header)));
    hd->msg_code = MSG_PROD_REQUEST;
    tm = time (NULL);
    hd->date = RPG_JULIAN_DATE (tm);
    hd->time = RPG_TIME_IN_SECONDS (tm);
    hd->length = size;
    hd->src_id = 0;
    hd->dest_id = 0;
    hd->n_blocks = usr->up->dd_len + 1;
    hd->line_ind = usr->line_ind;

    dd = (Pd_prod_item *)((char *)usr->up + usr->up->dd_list);
    for (i = 0; i < usr->up->dd_len; i++) {

	req->divider = -1;
	req->length = 32;
	req->prod_id = dd->prod_id;
	req->flag_bits = 0;		/* low priority */
	if (dd->map_requested)
	    req->flag_bits |= MAP_FLAG_BIT;
	req->seq_number = 0;
	req->num_products = -1;		/* for routine products */
	req->req_interval = dd->period;
	req->VS_date = 0;		/* not used */
	req->VS_start_time = 0;		/* not used */
	memcpy ((char *)req->params, (char *)dd->params, 
				NUM_PROD_DEPENDENT_PARAMS * sizeof (short));
	req++;
	dd++;
    }

    /* send to the routine scheduler */
    ret = ORPGDA_write (ORPGDAT_RT_REQUEST, msg, size, P_server_index);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 38,  
		"ORPGDA_write ORPGDAT_RT_REQUEST failed (ret = %d)", ret);
	ORPGTASK_exit (1);
    }
    EN_post (ORPGEVT_RT_REQUEST, NULL, 0, EN_POST_FLAG_LOCAL);

    free (msg);
    return;
}

/**************************************************************************

    Description: This function prints the routine product distribution list.

    Inputs:	usr - the user involved.
		list - the routine distribution list.
		cnt - list length.

**************************************************************************/

static void Print_RT_list (User_struct *usr, Routine_prod_item *list, int cnt)
{
    int i, cde;
    char *mne = NULL, *mnemonic = "   ";

    LE_send_msg (LE_VL2 | 39,  "Routine prod distri list (len %d): L%d", 
						cnt, usr->line_ind);
    if (cnt > 0)
	LE_send_msg (LE_VL2 | 40,  "    pid cde mne wxm prd num  map prio seq   p1  p2  p3  p4  p5  p6  n_distrd\n");	
    for (i = 0; i < cnt; i++){
	int ep_ind;

        mne = ORPGPAT_get_mnemonic( (int) list[i].prod_id );
        if( mne == NULL ) mne = mnemonic;
        cde = ORPGPAT_get_code( (int) list[i].prod_id );
        if( cde < 0 ) cde = 0;

	ep_ind = -1;
	if (list->type == RPI_ELEV_PROD)
	    ep_ind = list->ep_ind;
	LE_send_msg (LE_VL2 | 41,  "    %3d %3d %3s %3d %3d %3d  %3d %4d %3d  %s  %8d\n",	
		list[i].prod_id, cde, mne, list[i].wx_modes, list[i].period, 
		list[i].number, 
		list[i].map_requested, list[i].priority, list[i].seq_number,
		HP_print_parameters (list[i].params, list[i].prod_id), 
							list[i].n_total);

    }
    return;
}

/**************************************************************************

    Description: This function prints the product parameters.

    Inputs:	params - the product parameters.

    Return:	A pointer to the buffer of the printed text.

**************************************************************************/

char *HP_print_parameters (short *params, int prod_id) {
    static char buf[64];	/* buffer for the parameter text */
    char *pt;
    int ep_ind, i;

    ep_ind = ORPGPAT_elevation_based (prod_id);
    pt = buf;
    for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++) {
	int p;

	p = params[i];
	switch (p) {
	    case PARAM_UNUSED:
		strcpy (pt, "UNU ");
		pt += 4;
		break;
	    case PARAM_ALG_SET:
		strcpy (pt, "ALG ");
		pt += 4;
		break;
	    default:
		if (i == ep_ind) {		/* elevation */
		    sprintf (pt, "%d%d ", p >> 13, p & 0x1ff);
		    while (strlen (pt) < 4)
			strcat (pt, " ");
		    pt += strlen (pt);
		}
		else {				/* others */
		    sprintf (pt, "%3d ", p);
		    pt += strlen (pt);
		}
		break;
	}
    }
    return (buf);
}

/**************************************************************************

    Description: This function initializes the product table for a new user.

    Inputs:	usr - the user involved.

**************************************************************************/

void HP_init_prod_table (User_struct *usr)
{

    ((Hp_local *)usr->hp)->pd_enabled = 1;
    Get_updated_prod_gen_status (usr, NULL, 0, NULL);
    Send_dd_for_scheduling (usr);
    Init_routine_prod_table (usr);
    if (usr->up->cntl & UP_CD_DAC)
	Check_distribution_completion (usr);
    return;
}

/**************************************************************************

    Description: This function removes the current product table.

    Inputs:	usr - the user involved.

**************************************************************************/

void HP_clear_prod_table (User_struct *usr)
{
    Hp_local *hp;
    Pd_msg_header hd;
    int ret;

    hp = (Hp_local *)usr->hp;
    if (hp->rt_list != NULL)
	free ((char *)hp->rt_list);
    hp->rt_list = NULL;
    hp->n_rt = hp->n_buf = 0;

    /* do not cancel requests for dial-in and WAN users. */
    if( (usr->line_type == DIAL_IN)
                   ||
        (usr->line_type == WAN_LINE) )
       return;

    /* send request canceling messages to the product schedulers */
    hd.msg_code = MSG_PROD_REQUEST;
    hd.date = RPG_JULIAN_DATE (usr->time);
    hd.time = usr->time;
    hd.length = sizeof (Pd_msg_header);
    hd.src_id = 0;
    hd.dest_id = 0;
    hd.n_blocks = 1;
    hd.line_ind = usr->line_ind;

    /* send to the schedulers */
    ret = ORPGDA_write (ORPGDAT_OT_REQUEST, (char *)&hd, 
				sizeof (Pd_msg_header), P_server_index);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 42,  
		"ORPGDA_write ORPGDAT_OT_REQUEST failed (ret = %d)", ret);
	ORPGTASK_exit (1);
    }
    EN_post (ORPGEVT_OT_REQUEST, NULL, 0, EN_POST_FLAG_LOCAL);
    ret = ORPGDA_write (ORPGDAT_RT_REQUEST, (char *)&hd, 
				sizeof (Pd_msg_header), P_server_index);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 43,  
		"ORPGDA_write ORPGDAT_RT_REQUEST failed (ret = %d)", ret);
	ORPGTASK_exit (1);
    }
    EN_post (ORPGEVT_RT_REQUEST, NULL, 0, EN_POST_FLAG_LOCAL);

    return;
}

/**************************************************************************

    Sets flag Prod_status_updated. This function is called when the update
    event is received.

**************************************************************************/

void HP_prod_status_updated () {
    Prod_status_updated = 1;
}

/**************************************************************************

    Description: This function reads and returns the product generation 
		status message. If the message is not updated it returns
		a saved version.

    Output:	stat - pointer to the product list (Prod_gen_status array).
		pgs_hd - pointer to the product generation status header.

    Return:	length of the product list on success or -1 on failure.

**************************************************************************/

int HP_read_prod_gen_status (Prod_gen_status **stat, 
				Prod_gen_status_header **pgs_hd) {
    static char *buf = NULL, *saved_buf;
    static int msg_size = 0;
    Prod_gen_status_header *hd;

    if (!Prod_status_updated) {
	memcpy (buf, saved_buf, msg_size);
	hd = (Prod_gen_status_header *)buf;
    }
    else {
	static int buf_len = 0;
	int len;

	/* read the product generation status */
	msg_size = 0;
	while (1) {
    
	    if (buf == NULL) {	/* malloc the buffer */
		if (buf_len == 0)
		    buf_len = sizeof (Prod_gen_status_header) + 
					200 * sizeof (Prod_gen_status);
		if ((buf = malloc (buf_len * 2)) == NULL) {
		    LE_send_msg (GL_ERROR | 45,  "malloc failed");
		    return (-1);
		}
		saved_buf = buf + buf_len;
	    }
	    Prod_status_updated = 0;
	    len = ORPGDA_read (ORPGDAT_PROD_STATUS, 
					buf, buf_len, PROD_STATUS_MSG);
	    if (len == LB_BUF_TOO_SMALL) {
		free (buf);
		buf = NULL;
		buf_len *= 2;
		continue;
	    }
	    if (len < 0) {
		LE_send_msg (GL_ERROR | 46,  
		    "ORPGDA_read ORPGDAT_PROD_STATUS failed (ret = %d)", len);
		return (-1);
	    }
	    break;
	}
    
	/* message verification */
	if (len < (int)sizeof (Prod_gen_status_header)) {
	    LE_send_msg (GL_ERROR | 47,  
		    "bad product status message (size %d)", len);
	    return (-1);
	}
	hd = (Prod_gen_status_header *)buf;
	if (len != sizeof (Prod_gen_status_header) + 
				    hd->length * sizeof (Prod_gen_status) || 
	    hd->length > MAX_PRODS_IN_STATUS_MAG) {
	    LE_send_msg (GL_ERROR | 48,  
		    "bad product status message (len %d)", hd->length);
	    return (-1);
	}
	msg_size = len;
	memcpy (saved_buf, buf, msg_size);
    }

    if (msg_size == 0)
	return (-1);

    if (stat != NULL)
	*stat = (Prod_gen_status *)(buf + hd->list);
    if (pgs_hd != NULL)
	*pgs_hd = hd;

    return (hd->length);
}

/**************************************************************************

    Description: This function checks if the product (pid) is 
		permitted for request, in weather mode (wx_mask), for
		one time or routine (check_type) distribution. The latter
		entries in the list will override any previous entries.

    Inputs:	usr - the user involved.
		pid - product id.
		check_type - PD_PMS_ONETIME or PD_PMS_ROUTINE

    Output:	wx_modes - returns the permitted weather mode.

    Return:	1 if the permission is accepted or 0 if the permission is
		rejected.

**************************************************************************/

int HP_prod_permission_ok (User_struct *usr, int pid, 
				int check_type, int *wx_modes)
{
    Pd_pms_entry *pms_list;
    int pms_len;
    int allowed, k;

    pms_len = usr->up->pms_len ;
    pms_list = (Pd_pms_entry *)((char *)usr->up + usr->up->pms_list);

    allowed = 0;
    for (k = 0; k < pms_len; k++) {
	Pd_pms_entry *pms;

	pms = pms_list + k;
	if (pms->prod_id == pid ||
	    pms->prod_id == PD_PMS_ALL) {
	    if (pms->types & check_type) {
		allowed = 1;
		if (wx_modes != NULL)
		    *wx_modes = pms->wx_modes;
	    }
	    else
		allowed = 0;
	}
	else if (pms->prod_id == PD_PMS_NONE)
	    allowed = 0;
    }
    return (allowed);
}

/**************************************************************************

    This function compares the product list in the product generation
    status message, "stat", with the previous saved one and returns, in
    the same array, those products which status are different. This
    function is designed for increasing routine product search
    efficiency in HP_handle_routine_products. The product generation
    status message header, "hd" is also saved for used by other
    functions.

    Inputs:	usr - the user involved.
		stat - the original product generation status. If NULL,
			a new update check session is started.

		stat_len - length of the "stat" array.
		hd - product generation status message header.

    Outputs:	stat - the suppressed product generation status list 
			containing only the products which status has 
			updated.

    Returns:	The array length of the updated status.

**************************************************************************/

static int Get_updated_prod_gen_status (User_struct *usr, 
	Prod_gen_status *stat, int stat_len, Prod_gen_status_header *hd) {
    Hp_local *hp;
    int i, oi, ni, new_buf_size;
    char *buf;
    int v_changed;

    hp = (Hp_local *)usr->hp;
    if (stat == NULL) {
	hp->prev_stat_len = -1;		/* discard the previous list */
	return (0);
    }

    /* malloc the static space */
    buf = NULL;
    if (hp->ps_buf_size < stat_len) {
	char *tmp;

	new_buf_size = stat_len + 50;
	tmp = malloc (new_buf_size * sizeof (short));
	buf = malloc (new_buf_size * sizeof (Prod_gen_status));
	if (tmp == NULL || buf == NULL) {
	    if (tmp != NULL)
		free (tmp);
	    if (buf != NULL)
		free (buf);
	    LE_send_msg (GL_ERROR | 49,  "malloc failed");
	    hp->prev_stat_len = -1;
	    return (stat_len);
	}
	if (hp->changed != NULL)
	    free ((char *)(hp->changed));
	hp->changed = (short *)tmp;
    }

    /* go through the list and mark those that have not changed */
    for (i = 0; i < stat_len; i++)
	hp->changed[i] = 1;
    oi = 0;
    for (i = 0; i < stat_len; i++) {
	int n;

	if (oi >= hp->prev_stat_len)
	    break;
	if (stat[i].prod_id < hp->prev_stat[oi].prod_id)
	    continue;
	while (stat[i].prod_id > hp->prev_stat[oi].prod_id && 
						oi < hp->prev_stat_len)
	    oi++;
	if (oi >= hp->prev_stat_len)
	    break;
	for (n = oi; n < hp->prev_stat_len; n++) {
					/* go through the old table */
	    if (hp->prev_stat[n].prod_id != stat[i].prod_id)
		break;
	    if (memcmp (hp->prev_stat + n, stat + i, 
					sizeof (Prod_gen_status)) == 0) {
		hp->changed[i] = 0;
		break;
	    }
	}
    }

    /* check whether volume list changed */
    if (hp->prev_stat_len >= 0 &&
	memcmp (hp->prev_hd.vnum, hd->vnum, 
				PGS_LIST_LENGTH * sizeof (int)) != 0) {
	v_changed = 1;
    }
    else
	v_changed = 0;

    /* store the status */
    if (buf != NULL) {
	if (hp->prev_stat != NULL)
	    free ((char *)(hp->prev_stat));
	hp->prev_stat = (Prod_gen_status *)buf;
	hp->ps_buf_size = new_buf_size;
    }
    memcpy (hp->prev_stat, stat, stat_len * sizeof (Prod_gen_status));
    hp->prev_stat_len = stat_len;
    memcpy (&(hp->prev_hd), hd, sizeof (Prod_gen_status_header));

    if (!v_changed) {		/* remove the unchanged */
	ni = 0;
	for (i = 0; i < stat_len; i++) {
	    if (!(hp->changed[i]))
		continue;
	    if (ni != i) 
		memcpy (stat + ni, stat + i, sizeof (Prod_gen_status));
	    ni++;
	}
	return (ni);
    }
    else
	return (stat_len);
}

/**************************************************************************

    Description: This function checks whether all routine products are 
		distributed. It disconnects the user if product distribution
		is completed.

    Inputs:	usr - the user involved.

**************************************************************************/

static void Check_distribution_completion (User_struct *usr)
{
    Hp_local *hp;
    int done, k;

    hp = (Hp_local *)usr->hp;
    done = 1;
    for (k = 0; k < hp->n_rt; k++) { 
	Routine_prod_item *list;	

	list = hp->rt_list + k;
	if (list->number == 0 || list->n_total < list->number)
	    done = 0;
    }

    if (done) {
	char *msg;

	LE_send_msg (GL_ERROR | 51,  "session completed, L%d", usr->line_ind);
	msg = RRS_gsm_message (usr);	/* get a general status msg */
	if (msg != NULL) {
	    Pd_general_status_msg *gs;
	    gs = (Pd_general_status_msg *)msg;
	    gs->rpg_nb_status |= HALFWORD_SHIFT(15);/* commanded disconnect */
	    gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
	    HWQ_put_in_queue (usr, 0, msg);		/* send to the user */
	}

	usr->discon_reason = US_SOLICITED;
	MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	usr->psv_state = ST_DISCONNECTING;
	usr->next_state = 1;
    }

    return;
}

/**************************************************************************

    Description: This function allocates/reallocates the routine product
		distribution list buffer.

    Inputs:	usr - the user involved.
		new_size - the new list size.

    Return:	0 on success or -1 on failure, in which case the original
		buffer is freed.

**************************************************************************/

static int Allocate_list_buffer (User_struct *usr, int new_size)
{
    Hp_local *hp;
    Routine_prod_item *list;

    hp = (Hp_local *)usr->hp;
    if (hp->rt_list != NULL)
	free ((char *)hp->rt_list);
    hp->n_rt = hp->n_buf = 0;
    hp->rt_list = NULL;

    list = (Routine_prod_item *)malloc (new_size * sizeof (Routine_prod_item));
    if (list == NULL) {
	LE_send_msg (GL_ERROR | 52,  "malloc failed");
	return (-1);
    }

    hp->rt_list = list;
    hp->n_buf = new_size;

    return (0);
}

/***********************************************************************

    Returns the elevation value of product request of "prod_id" with 
    parameters "params". If the product is not elevation based, 0 is
    returned.

***********************************************************************/

static int Get_elevation (int prod_id, short *params) {
    int ep_ind;

    ep_ind = ORPGPAT_elevation_based (prod_id);
    if (ep_ind >= 0 && ep_ind < NUM_PROD_DEPENDENT_PARAMS &&
					params[ep_ind] != PARAM_UNUSED)
	return (params[ep_ind]);
    else
	return (0);
}


