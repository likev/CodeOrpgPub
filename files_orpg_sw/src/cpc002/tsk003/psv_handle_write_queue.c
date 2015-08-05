
/******************************************************************

	file: psv_handle_write_queue.c

	This module contains functions that handle the write queue
	for each user.
	
******************************************************************/

/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/09/19 15:11:30 $
 * $Id: psv_handle_write_queue.c,v 1.103 2007/09/19 15:11:30 cmn Exp $
 * $Revision: 1.103 $
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

#define FTMD_NEED_TABLE
#include <a309.h>

#include <infr.h>
#include <orpg.h>
#include <orpgerr.h>
#include <orpgsite.h>
#include <prod_user_msg.h>
#include <prod_gen_msg.h>
#include <prod_distri_info.h>
#include <comm_manager.h>

#include "psv_def.h"

#define MAX_PRODUCT_SIZE	10000000
				/* max product size for protecting from
				   allocating too much memory */

#define INITIAL_WRITE_QUEUE_SIZE	64
				/* queue size for each user */

#define MAX_MSG_LENGTH		10240
				/* maximum message size that can be sent 
				   through RPG prod user link */

enum {SEARCH_FOR_SEND, SEARCH_FOR_SHED};
				/* values for argument "function" of
				   Search_message () */


struct queue_entry {		/* write queue entry */
    int type;			/* data type; Refer to Argument "type" of 
				   HWQ_put_in_queue () in psv_def.h */
    char *data;			/* pointer to the message;
				   NULL indicates unused entry */
    int distri_prio;		/* distribution priority - highest 
				   distri_prio is sent first */
    int inqueue_prio;		/* stay in queue priority - lowest inqueue_prio
				   is load shed first */
    unsigned int vnum;		/* product volume number, or, for other 
				   non-product messages, the volume 
				   number at which the message is put in 
				   the queue */
    int length;			/* size of data */
    int msg_code;		/* message code */
    time_t t_eq;		/* time product is put in the queue */

    Vv_record_t *vv;		/* V&V record */

    int next;			/* index of the next entry of the queue, -1 
				   indicates that there is no next entry */
};

typedef struct queue_entry Queue_entry;

typedef struct {		/* local part of the User_struct */
    int first;			/* the index of the first entry of the queue; 
				   -1 indicates that the queue is empty */
    Queue_entry *current;	/* the queue entry that is being written 
				   to the WAN */
    Queue_entry *alert;		/* the alert entry that is being written 
				   to the WAN */
    int nb_written;		/* number of bytes has been written */
    Queue_entry *queue;		/* the write queue */
    int qb_size;		/* size of the queue array */
    int n_bytes;		/* number of data bytes in the queue */
    int vv_fd;			/* V&V file descriptior */
} Hwq_local;

static int Vv_mode = 0;		/* in v&v mode */


/* local functions */
static int Remove_entry (User_struct *usr, int ind, int status);
static Queue_entry *Find_entry (User_struct *usr, int ind);
static int Get_next_message (User_struct *usr);
static int Search_message (User_struct *usr, int function);
static int Read_in_message (User_struct *usr, Queue_entry *entry);
static int Realloc_queue_buffer (User_struct *usr);
static void Print_new_item (User_struct *usr, int type, char *data);
static void Set_MHB_time (char *buf);
static int Shed_one_message (User_struct *usr, int need_report);
static void Set_vv_tag (User_struct *usr, Queue_entry *q, 
						int type, char *data);
static void Vv_send_vv (User_struct *usr, Queue_entry *entry, int status);
static void Vv_write (User_struct *usr, Vv_record_t *vv);


/**************************************************************************

    Description: Sets/resets the V&V mode.

    Inputs:	vv_mode - the new V&V mode.

**************************************************************************/

void HWQ_set_vv_mode (int vv_mode) {
    Vv_mode = vv_mode;
    if (Vv_mode)
	LE_send_msg (GL_INFO, "V&V mode turned on");
    else 
	LE_send_msg (GL_INFO, "V&V mode turned off");
}

/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int HWQ_initialize (int n_users, User_struct **users)
{
    int i;

    /* initialize the local data structure */
    for (i = 0; i < n_users; i++) {
	Hwq_local *hwq;

	users[i]->msg_queued = 0;
	users[i]->hwq = malloc (sizeof (Hwq_local));
	if (users[i]->hwq == NULL) {
	    LE_send_msg (GL_ERROR | 54,  "malloc failed");
	    return (-1);
	}

	hwq = (Hwq_local *)users[i]->hwq;
	hwq->qb_size = 0;
	hwq->n_bytes = 0;
	if (Realloc_queue_buffer (users[i]) < 0)
	    return (-1);
	hwq->first = -1;
	hwq->current = NULL;
	hwq->alert = NULL;
	hwq->vv_fd = -1;
    }

    return (0);
}

/**************************************************************************

    Description: This function performs the NB load shed.

    Inputs:	usr - the user involved.

**************************************************************************/

void HWQ_comm_load_shed (User_struct *usr)
{
    int shed_len, max_qt, vol_time;
    int alarm, warning, load_shed_flag;
    Hwq_local *hwq;

    if (ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST, 
			    LOAD_SHED_WARNING_THRESHOLD, &warning) != 0 ||
	ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST, 
			    LOAD_SHED_ALARM_THRESHOLD, &alarm) != 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGLOAD_get_data LOAD_SHED_CATEGORY_PROD_DIST failed");
	alarm = warning = 100;
    }
    load_shed_flag = US_LOAD_NORMAL;
    if (usr->up == NULL ||
	    !(usr->up->cntl & UP_CD_COMM_LOAD_SHED)) /* load shed disabled */
	load_shed_flag = US_LOAD_SHED_DISABLED;
    if ((vol_time = RRS_get_volume_duration ()) <= 0)
	LE_send_msg (GL_ERROR, "bad volume duration (%d)", vol_time);

    hwq = (Hwq_local *)usr->hwq;
    shed_len = max_qt = 0;
    while (1) {
	int shed, ind;

	if (vol_time <= 0)
	    break;
	max_qt = 0;
	ind = hwq->first;
	while (ind >= 0) {
	    Queue_entry *entry;
	    entry = hwq->queue + ind;
	    if (entry->data != NULL && entry != hwq->current) {
		max_qt = usr->time - entry->t_eq;
		break;
	    }
	    ind = entry->next;
	}
	max_qt = (int)((float)max_qt * 100.f / (float)vol_time + .5f);
						/* in percent */

	if ((shed_len == 0 && max_qt < alarm) ||
	    (shed_len > 0 && max_qt < warning))
	    break;

	/* perform load shed */
	if (load_shed_flag == US_LOAD_SHED_DISABLED) /* NB load shed */
	    break;
	shed = Shed_one_message (usr, 1);
	LE_send_msg (LE_VL1, "  NB rate %d, max qt %d (%d bytes), shed %d",
			usr->line_tbl->baud_rate, max_qt, hwq->n_bytes, shed);
	if (shed > 0)
	   shed_len += shed;
	else
	    break;
    }

    if (shed_len > 0) {
	char *msg;

	LE_send_msg (GL_STATUS | GL_ERROR | LE_RPG_AL_LS | 58, 
		"Narrow Band line %d, Comm. OVERLOAD CRITICAL, PRODUCTS SHED",
				usr->line_ind);
/*      we dont send anymore since we send after each message shed
	HWQ_put_in_queue (usr, 0,      send rr msg 
	    	GUM_rr_message (usr, RR_NB_LOADSHED, 0, 0, 0));
*/
	msg = RRS_gsm_message (usr);		/* get a general status msg */
	if (msg != NULL) {
	    Pd_general_status_msg *gs;
	    gs = (Pd_general_status_msg *)msg;
	    gs->rpg_nb_status |= HALFWORD_SHIFT(14);	/* NB loadshed */
	    HWQ_put_in_queue (usr, 0, msg);	/* send GSM to the user */
	}
	SUS_loadshed_changed (usr, US_LOAD_SHED, max_qt);

        /* now clear */
	msg = RRS_gsm_message (usr);		/* get a general status msg */
	if (msg != NULL) {
	    Pd_general_status_msg *gs;
	    gs = (Pd_general_status_msg *)msg;
	    gs->rpg_nb_status &= ~(HALFWORD_SHIFT(14));	/* NB loadshed cleared */
	    HWQ_put_in_queue (usr, 0, msg);	/* send GSM to the user */
	}
    }
    else
	SUS_loadshed_changed (usr, load_shed_flag, max_qt);

    return;
}

/**************************************************************************

    Description: This function extends the buffer for the queue.

    Inputs:	usr - the user involved;

    Return:	0 on success or -1 on failure.

**************************************************************************/

static int Realloc_queue_buffer (User_struct *usr)
{
    char *qb;
    int new_size;
    Hwq_local *hwq;
    Queue_entry *q;
    int k;

    hwq = (Hwq_local *)usr->hwq;

    if (hwq->qb_size == 0)
	new_size = INITIAL_WRITE_QUEUE_SIZE;
    else
	new_size = 2 * hwq->qb_size;

    qb = malloc (new_size * sizeof (Queue_entry));
    if (qb == NULL) {
	LE_send_msg (GL_ERROR | 59,  "malloc failed");
	return (-1);
    }
    if (hwq->qb_size > 0) {
	memcpy (qb, hwq->queue, hwq->qb_size * sizeof (Queue_entry));
	if (hwq->current != NULL)		/* recover hwq->current */
	    hwq->current = (Queue_entry *)((char *)hwq->current +
			(qb - (char *)hwq->queue));
	if (hwq->alert != NULL)			/* recover hwq->alert */
	    hwq->alert = (Queue_entry *)((char *)hwq->alert +
			(qb - (char *)hwq->queue));
	free (hwq->queue);
    }

    q = (Queue_entry *)(qb + hwq->qb_size * sizeof (Queue_entry));
    for (k = 0; k < new_size - hwq->qb_size; k++) {
	q[k].data = NULL;
	q[k].vv = NULL;
    }

    hwq->qb_size = new_size;
    hwq->queue = (Queue_entry *)qb;
    return (0);
}

/**************************************************************************

    Description: This function empties the write queue.

    Inputs:	usr - the user involved;

**************************************************************************/

void HWQ_empty_queue (User_struct *usr)
{
    Hwq_local *hwq;

    hwq = (Hwq_local *)usr->hwq;

    while (Remove_entry (usr, 0, VV_DISCARDED) == 0);
    hwq->current = NULL;
    hwq->alert = NULL;
    hwq->n_bytes = 0;
    HWQ_comm_load_shed (usr);
    usr->msg_queued = 0;

    return;
}

/**************************************************************************

    Description: This function adds a new message at the end of the queue.

    Inputs:	usr - the user involved;
		type - data type. Refer to Argument "type" of 
			HWQ_put_in_queue () in psv_def.h;
		data - pointer to the message;

    Return:	0 on success or -1 if the queue is full.

**************************************************************************/

int HWQ_put_in_queue (User_struct *usr, int type, char *data)
{
    Hwq_local *hwq;
    Queue_entry *q, *last;
    int unused;

    if (data == NULL) {
	LE_send_msg (0, "NULL data in HWQ_put_in_queue, L%d", usr->line_ind);
	return (0);
    }

    hwq = (Hwq_local *)usr->hwq;
    q = hwq->queue;

    /* find an unused entry */
    for (unused = 0; unused < hwq->qb_size; unused++)
	if (q[unused].data == NULL)
	    break;
    if (unused >= hwq->qb_size) {	/* extend the queue array */
	unused = hwq->qb_size;
	if (Realloc_queue_buffer (usr) < 0)
	    return (-1);
	q = hwq->queue;
    }

    if (LE_local_vl (-1) >= 1)		/* print a message about the new item */
	Print_new_item (usr, type, data);

    {				/* evaluate the priorities */
    	Pd_msg_header *hd;
	int categ, prio, ot, prod_priority;

	hd = (Pd_msg_header *)data;
	q[unused].msg_code = hd->msg_code;
	q[unused].length = UMC_get_product_length (hd);

	hd->src_id = ORPGSITE_get_int_prop (ORPGSITE_RPG_ID);
	if (hd->dest_id < 0 && usr->up != NULL)
	    hd->dest_id = usr->up->user_id;

	if (hd->msg_code == MSG_ALERT)	/* alert message */
	    categ = 3;
	else if (hd->msg_code == MSG_GEN_STATUS || 
		 hd->msg_code == MSG_REQ_RESPONSE ||
		 hd->msg_code == MSG_FREE_TEXT_MSG ||
		 hd->msg_code == MSG_ALERT_PARAMETER)
	    categ = 2;
	else if (type & HWQ_TYPE_ALERT_PRIO)	/* alert products */
	    categ = 1;  
	else 
	    categ = 0;

	if (type & HWQ_TYPE_HIGH_PRIO)	/* request priority */
	    prio = 1;
	else
	    prio = 0;

	if ( (type & HWQ_TYPE_ONETIME) && !(type & HWQ_TYPE_ALERT_PRIO) )	/* one-time product */
	    ot = 1;
	else
	    ot = 0;

	prod_priority = ORPGPAT_get_priority (
				ORPGPAT_get_prod_id_from_code ((int)hd->msg_code),
				ORPGVST_get_mode());
	if (prod_priority < 0)
	    prod_priority = 0;
	q[unused].distri_prio = categ * 10000000 + prio * 10000 + ot * 1000;
	q[unused].inqueue_prio = q[unused].distri_prio + prod_priority;
    }

    /* volume number */
    if (type & HWQ_TYPE_INDIRECT){
	Indirect_rpg_msg *indirect;

	indirect = (Indirect_rpg_msg *)(data + sizeof (Pd_msg_header));
	q[unused].vnum = indirect->vol_number;
    }
    else 
	q[unused].vnum = RRS_get_volume_number (NULL, NULL);
    q[unused].t_eq = usr->time;

    if (Vv_mode)
	Set_vv_tag (usr, q + unused, type, data);

    if (!(type & HWQ_TYPE_INDIRECT) && 
			((Pd_msg_header *)data)->msg_code != MSG_ALERT) {
	int icd_len;
	char *icd;

	icd_len = UMC_to_ICD (data, ((Pd_msg_header *)data)->length, 
				sizeof (CM_req_struct), (void *)&icd);
	if (icd_len < 0) {
	    LE_send_msg (GL_ERROR | 60,  "UMC_to_ICD failed (ret %d)", icd_len);
	    return (-1);
	}
	if (!(type & HWQ_TYPE_STATIC))
	    WAN_free_usr_msg (data);
	type &= ~HWQ_TYPE_STATIC;
	q[unused].data	= icd + sizeof (CM_req_struct);
	q[unused].length = icd_len;
	type |= HWQ_TYPE_CMHD_FREE;
    }
    else{
        if( ((Pd_msg_header *)data)->msg_code == MSG_ALERT )
           UMC_message_header_block_swap (data);
	q[unused].data = data;
    }

    q[unused].type = type;
    q[unused].next = -1;

    /* find the end entry */
    if (hwq->first < 0)
	hwq->first = unused;
    else {
	last = q + hwq->first;
	while (last->data != NULL && last->next >= 0)
	    last = q + last->next;
	if (last->data == NULL) {
	    LE_send_msg (GL_ERROR | 61,  "fatal error in HWQ_put_in_queue");
	    ORPGTASK_exit (1);
	}
	else
	    last->next = unused;
    }
    hwq->n_bytes += q[unused].length;

    usr->msg_queued = 1;
    return (0);
}

/**************************************************************************

    Description: This function prints info about the new item to put in 
		the queue.

    Inputs:	usr - the user involved;
		type - data type. Refer to Argument "type" of 
			HWQ_put_in_queue () in psv_def.h;
		data - pointer to the message;

**************************************************************************/

static void Print_new_item (User_struct *usr, int type, char *data)
{
    Pd_msg_header *hd;
    char buf[256];
    int id;

    hd = (Pd_msg_header *)data;
    id = ORPGPAT_get_prod_id_from_code ((int)hd->msg_code);
    if (id >= 0)
	sprintf (buf, "queue msg (code %d, id %d, len %d)", 
			hd->msg_code, id, UMC_get_product_length (hd));
    else
	sprintf (buf, "queue msg (code %d, len %d)", 
			hd->msg_code, UMC_get_product_length (hd));

    switch (hd->msg_code) {
	Pd_request_response_msg *rr;
	Pd_prod_list_msg *pl;
	Pd_general_status_msg *gsm;

	case MSG_REQ_RESPONSE:
	    rr = (Pd_request_response_msg *)data;
	    sprintf (buf + strlen (buf), ", RR err %x", 
					(unsigned int)rr->error_code);
	break;

	case MSG_PROD_LIST:
	    pl = (Pd_prod_list_msg *)data;
	    sprintf (buf + strlen (buf), ", PROD_DISTRI_LIST N prods %d", 
					pl->num_products);
	break;

	case MSG_GEN_STATUS:
	    gsm = (Pd_general_status_msg *)data;
	    sprintf (buf + strlen (buf), 
			", GSM (OP, AL, ST) RPG %x %x %x RDA %x %x %x", 
			(unsigned int)gsm->rpg_op_status, 
			(unsigned int)gsm->rpg_alarms, 
			(unsigned int)gsm->rpg_status,
			(unsigned int)gsm->rda_op_status, 
			(unsigned int)gsm->rda_alarms, 
			(unsigned int)gsm->rda_status);
	break;

	case MSG_ALERT_PARAMETER:
	    sprintf (buf + strlen (buf), ", ALERT_PARAMETER");
	break;

	default:
	break;
    }

    if (type & HWQ_TYPE_INDIRECT) {
	Indirect_rpg_msg *indirect;

	indirect = (Indirect_rpg_msg *)(data + sizeof (Pd_msg_header));
	sprintf (buf + strlen (buf), ", indirect (msgid %d)", 
						indirect->msg_id);
    }

    sprintf (buf + strlen (buf), ", L%d", usr->line_ind);
    LE_send_msg (0, buf);

    return;
}

/**************************************************************************

    Description: This function writes the next message to the WAN according
		to the message priorities. We assume that the high priority
		circuit messages are all shorter than 10k such that no
		segmentation is needed.

    Inputs:	usr - the user involved;

    Return:	Number of bytes written, 0 if there is no data to write 
		or -1 in error conditions.

**************************************************************************/

int HWQ_wan_write (User_struct *usr)
{
    Hwq_local *hwq;
    int len, i;

    usr->msg_queued = 0;
    hwq = (Hwq_local *)usr->hwq;

    /* write a high priority message */
    if (WAN_circuit_ready (usr, HIGH_PRIORITY)) {
	if (hwq->alert != NULL) {
	    for (i = 0; i < hwq->qb_size; i++) {
		if (Find_entry (usr, i) == hwq->alert) {
		    Remove_entry (usr, i, VV_SUCCESS);
		    break;
		}
	    }
	    hwq->alert = NULL;
	}
	for (i = 0; i < hwq->qb_size; i++) {
	    Queue_entry *entry;
    
	    entry = Find_entry (usr, i);
	    if (entry == NULL)
		break;
    
	    if (entry->msg_code == MSG_ALERT) {
		Set_MHB_time (entry->data);
		if (Vv_mode && entry->vv != NULL)
		    entry->vv->t_dq = usr->time;
		hwq->alert = entry;
		WAN_write (usr, HIGH_PRIORITY, entry->data, entry->length);
		return (entry->length);
	    }
	}
    }

    /* write a normal priority message */
    if (WAN_circuit_ready (usr, NORMAL_PRIORITY) &&
	(len = Get_next_message (usr)) > 0) {
	if (hwq->nb_written == 0) {
	    Set_MHB_time (hwq->current->data);
	    if (Vv_mode && hwq->current->vv != NULL)
		hwq->current->vv->t_dq = usr->time;
	}
	if (WAN_write (usr, NORMAL_PRIORITY, 
			hwq->current->data + hwq->nb_written, len) == 0) {
	    hwq->nb_written += len;
	    return (len);
	}
    }

    return (0);
}

/**************************************************************************

    Description: This function sets the date/time fields in the Message
		Header Block. The time is considered as transmission time.

    Input:	buf - The message to send to the user.

**************************************************************************/

static void Set_MHB_time (char *buf)
{
    Pd_msg_header *mhb;
    unsigned short date;
    int tm;

    mhb = (Pd_msg_header *)buf;
    GUM_date_time (time (NULL), (short *)&date, &tm);
#ifdef LITTLE_ENDIAN_MACHINE
    mhb->date = SHORT_BSWAP (date);
    mhb->time = INT_BSWAP (tm);
#else
    mhb->date = date;
    mhb->time = tm;
#endif
    return;
}

/**************************************************************************

    Description: This function sheds all messages in the queue.

    Inputs:	usr - the user involved;

**************************************************************************/

void HWQ_shed_all (User_struct *usr)
{

    while (Shed_one_message (usr, 0) > 0); 
    HWQ_comm_load_shed (usr);
}

/**************************************************************************

    Description: This function removes a message with the lowest priority 
		in the write queue. The message being sent to the user
		is excluded.

		We may need to return further info about the removed 
		message.

    Inputs:	usr - the user involved;
		need_report - need to send an RR msg to the user.

    Return:	Number of bytes loadshed from the write queue.

**************************************************************************/

static int Shed_one_message (User_struct *usr, int need_report)
{
    int ind;
    Queue_entry *entry;

    /* search for the lowest priority entry */
    if ((ind = Search_message (usr, SEARCH_FOR_SHED)) < 0)
	return (0);

    entry = Find_entry (usr, ind);
    if (need_report && entry->type & HWQ_TYPE_INDIRECT) {
						/* send rr msg */
	Indirect_rpg_msg *indirect;
	indirect = (Indirect_rpg_msg *)(entry->data + sizeof (Pd_prod_header));
	HWQ_put_in_queue (usr, 0, 
	    	GUM_rr_message (usr, RR_NB_LOADSHED, entry->msg_code, 
			indirect->seq_number, 
			(ORPGMISC_vol_scan_num (indirect->vol_number) << 16) | 
					indirect->elev_angle));
    }

    Remove_entry (usr, ind, VV_LOADSHED);

    return (entry->length);
}

/**************************************************************************

    Description: This function calculates and returns the total number of
		data bytes in the write queue.

    Inputs:	usr - the user involved;

    Return:	Number of bytes in the write queue.

**************************************************************************/

int HWQ_get_wq_size (User_struct *usr)
{
    Hwq_local *hwq;
    int size;
    int i;

    hwq = (Hwq_local *)usr->hwq;
    size = 0;
    for (i = 0; i < hwq->qb_size; i++) {
	Queue_entry *entry;

	entry = Find_entry (usr, i);
	if (entry == NULL)
	    break;
	size += entry->length;
	if (entry == hwq->current)
	    size -= hwq->nb_written;
    }

    return (size);
}

/**************************************************************************

    Description: This function returns the "ind"-th entry in the write queue.

    Inputs:	usr - the user involved;
		ind - entry index;

    Return:	Pointer to the entry on success or NULL if the entry is not 
		found.

**************************************************************************/

static Queue_entry *Find_entry (User_struct *usr, int ind)
{
    Hwq_local *hwq;
    Queue_entry *q, *entry;
    int i;

    hwq = (Hwq_local *)usr->hwq;
    q = hwq->queue;

    if (hwq->first < 0)			/* empty queue */
	return (NULL);

    entry = q + hwq->first;
    for (i = 0; i < ind; i++) {
	if (entry->data != NULL && entry->next >= 0) {
	    entry = q + entry->next;
	}
	else
	    return (NULL);
    }
    if (entry->data != NULL)
	return (entry);
    else
	return (NULL);
}

/**************************************************************************

    Description: This function removes the entry "ind" in the write queue.
		If the HWQ_TYPE_NEED_FREE flag is set, data buffer is
		freed.

    Inputs:	usr - the user involved;
		ind - entry index;
		status - V&V status.

    Return:	0 on success or -1 if the entry is not found.

**************************************************************************/

static int Remove_entry (User_struct *usr, int ind, int status)
{
    Hwq_local *hwq;
    Queue_entry *q, *entry, *prev;
    int i;

    hwq = (Hwq_local *)usr->hwq;
    q = hwq->queue;

    if (hwq->first < 0)			/* empty queue */
	return (-1);

    prev = NULL;
    entry = q + hwq->first;
    for (i = 0; i < ind; i++) {
	if (entry->data != NULL && entry->next >= 0) {
	    prev = entry;
	    entry = q + entry->next;
	}
	else
	    return (-1);
    }
    if (entry->data == NULL)
	return (-1);

    if (Vv_mode)
	Vv_send_vv (usr, entry, status);

    /* free the buffer */
    if (!(entry->type & HWQ_TYPE_STATIC)) {
	if (entry->type & HWQ_TYPE_CMHD_FREE)
	    free (entry->data - sizeof (CM_req_struct));
	else
	    WAN_free_usr_msg (entry->data);
    }

    /* remove this entry */
    if (prev == NULL)
	hwq->first = entry->next;
    else
	prev->next = entry->next;
    entry->data = NULL;
    hwq->n_bytes -= entry->length;

    return (0);
}

/**************************************************************************

    Description: This function searches for the next message to be sent to
		the user in terms of the priorities. If such a message
		is found it sets current and nb_written fields in Hwq_local.
		This function also removes the entry that has been sent to
		the user. Messages for the high priority circuit are assumed
		to have been processed before this function is called. If 
		an error is found in reading a stored message, we discard
		the queue entry and repeat the search.

    Inputs:	usr - the user involved;

    Return:	Number of bytes to write, 0 if there is no data to write 
		or -1 in error conditions.

**************************************************************************/

static int Get_next_message (User_struct *usr)
{
    Hwq_local *hwq;
    int len;

    hwq = (Hwq_local *)usr->hwq;

    if (hwq->current != NULL) {
	if (hwq->nb_written < hwq->current->length) {/* more bytes to be sent */
	    len = hwq->current->length - hwq->nb_written;
	    if (len >= MAX_MSG_LENGTH)
		len = MAX_MSG_LENGTH;
	    	return (len);
	}
	else {			/* entire msg has been sent to the user */
	    int i;

	    for (i = 0; i < hwq->qb_size; i++) {
		if (Find_entry (usr, i) == hwq->current) {
		    Remove_entry (usr, i, VV_SUCCESS);
		    HWQ_comm_load_shed (usr);
		    break;
		}
	    }
	    hwq->current = NULL;
	}
    }

    while (1) {
	int ind, ret;

	/* search for the highest priority entry */
	if ((ind = Search_message (usr, SEARCH_FOR_SEND)) < 0)
	    return (0);
    
	/* if the data is of type HWQ_TYPE_INDIRECT, we read in the message */
	hwq->current = Find_entry (usr, ind);
	if ((hwq->current->type & HWQ_TYPE_INDIRECT) &&
	    (ret = Read_in_message (usr, hwq->current)) < 0) {
	    hwq->current = NULL;
	    if (ret == -2)
		return (-1);
	    Remove_entry (usr, ind, VV_LOST);
	}
	else
	    break;
    }

    hwq->nb_written = 0;
    len = hwq->current->length;
    if (len >= MAX_MSG_LENGTH)
	len = MAX_MSG_LENGTH;

    LE_send_msg (LE_VL1 | 62,  
	"    next msg to send, len %d, msg_code %d, L%d\n", 
	hwq->current->length, hwq->current->msg_code, usr->line_ind);

    return (len);
}

/**************************************************************************

    Description: This function reads in the complete message for a 
		HWQ_TYPE_INDIRECT type message. The original message is 
		replaced by the new one. If the message is lost,
		a request response message is put in the queue.

    Inputs:	usr - the user involved;
		entry - the queue entry in processing.

    Return:	0 on success, -2 on malloc error or -1 on other failure
		conditions.

**************************************************************************/

static int Read_in_message (User_struct *usr, Queue_entry *entry)
{
    Pd_prod_header *hd, *entry_hd;
    Indirect_rpg_msg *indirect;
    int size, ret, tm;
    char *buf;
    short *spt;

    entry_hd = (Pd_prod_header *)entry->data;
    size = UMC_get_product_length (entry_hd);
    if (size > MAX_PRODUCT_SIZE) {
	LE_send_msg (GL_ERROR | 63,  "unexpected large product (%d), L%d", 
					size, usr->line_ind);
	return (-1);
    }
    buf = WAN_usr_msg_malloc (size);
    if (buf == NULL) {
	LE_send_msg (GL_ERROR | 64,  "Malloc failed, L%d", usr->line_ind);
	return (-2);
    }

    indirect = (Indirect_rpg_msg *)(entry->data + sizeof (Pd_prod_header));
    ret = ORPGDA_read (indirect->orpgdat, buf - sizeof (Prod_header), 
			size + sizeof (Prod_header), indirect->msg_id);
/* {
static int cnt = 0, next = -1;
if (next < 0)
    next = 8 + (rand () % 10);
if (cnt == next) {
    LE_send_msg (0, "*****    simulate product lost");
    ret = -56;
    next = -1;
    cnt = 0;
}
cnt++;
} */

    if (ret != size + sizeof (Prod_header)) {
	LE_send_msg (GL_ERROR | 65,  "product in store %d, id = %d, lost (ret = %d)",
			indirect->orpgdat, indirect->msg_id, ret);
	WAN_free_usr_msg (buf);
	buf = GUM_rr_message (usr, RR_PROD_EXPIRED, (int)entry_hd->msg_code, 
			(int)indirect->seq_number, 
			(ORPGMISC_vol_scan_num (indirect->vol_number) << 16) |
					(int)indirect->elev_angle);
						/* send rr msg */
	HWQ_put_in_queue (usr, 0, buf);
	return (-1);
    }

    hd = (Pd_prod_header *)(buf);
    UMC_message_header_block_swap (hd);
    hd->src_id = entry_hd->src_id;
    hd->dest_id = entry_hd->dest_id;
    spt = (short *)buf;
    if (size >= 60 * 2)	{	/* MHB + PDB. see ICD */
	spt[18] = indirect->seq_number;
#ifdef LITTLE_ENDIAN_MACHINE
	spt[18] = SHORT_BSWAP (spt[18]);
#endif
	if (hd->msg_code == 75) {	/* free text message */
	    int i;
	    for (i = 0; i < FTMD_LINE_SPEC_SIZE; i++)
		spt[A309_ftmd_line_spec[i]] = 0;	/* pup expects 0 */
	    spt[FTMD_TYPE_SPEC] = 0;
	    spt[46] = ORPGSITE_get_int_prop (ORPGSITE_RPG_ID);
#ifdef LITTLE_ENDIAN_MACHINE
	    spt[46] = SHORT_BSWAP (spt[46]);
#endif
	}
    }
    else
	LE_send_msg (GL_ERROR | 66,  
		"prod %d does not have a prod description block", 
							indirect->orpgdat);
    /* the following 4 lines may be removed - the time will be reset later */
    hd->date = RPG_JULIAN_DATE (usr->time);
    tm = RPG_TIME_IN_SECONDS (usr->time);
    hd->timem = tm >> 16;
    hd->timel = tm & 0xffff;
    UMC_message_header_block_swap (hd);

    if (!(entry->type & HWQ_TYPE_STATIC))
	WAN_free_usr_msg (entry->data);

    entry->data	= buf;

    return (0);	
}

/**************************************************************************

    Description: This function searches for the next message to be sent to 
		the user or the next message to be shed from the queue.
		The message being sent to the user is excluded.

    Inputs:	usr - the user involved;
		function - SEARCH_FOR_SEND or SEARCH_FOR_SHED.

    Return:	index of the message in the queue, or -1 if there is no
		message in the queue.

**************************************************************************/

static int Search_message (User_struct *usr, int function) {
    Hwq_local *hwq;
    int ind;
    int i;
    Queue_entry *current;

    hwq = (Hwq_local *)usr->hwq;
    current = hwq->current;
    ind = -1;
    for (i = 0; i < hwq->qb_size; i++) {
	Queue_entry *entry;
	int distrip, found_distrip;
	int min_t_eq, found_min_t_eq;

	entry = Find_entry (usr, i);
	if (entry == NULL)
	    break;
	if (entry->msg_code == MSG_ALERT)
	    continue;
	if (entry == current)
	    continue;

	if (function == SEARCH_FOR_SEND) {
	    distrip = entry->distri_prio;
	    if (ind < 0 || distrip > found_distrip) {
		ind = i;
		found_distrip = distrip;
	    }
	}
	else {
	    /* search for a product to NB loadshed */
	    min_t_eq = entry->t_eq;
	    if (ind < 0 || min_t_eq < found_min_t_eq) {
		ind = i;
		found_min_t_eq = min_t_eq;
	    }
	}
    }

    return (ind);
}

/**************************************************************************

    Description: Sets the V&V message tag when a message is put in the queue.

    Inputs:	usr - the user involved;
		q - the queue entry;
		type - data type. Refer to Argument "type" of 
			HWQ_put_in_queue () in psv_def.h;
		data - pointer to the message;

**************************************************************************/

static void Set_vv_tag (User_struct *usr, Queue_entry *q, 
						int type, char *data) {
    Vv_record_t *vv;

    if (q->vv == NULL &&
	(q->vv = (Vv_record_t *)malloc (sizeof (Vv_record_t))) == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return;
    }
    vv = q->vv;
    vv->type = VV_UNUSED;
    vv->vol_num = q->vnum;
    vv->line_ind = usr->line_ind;
    vv->t_eq = usr->time;
    vv->priority = q->distri_prio;
    vv->ls_priority = q->inqueue_prio;

    if (type & HWQ_TYPE_INDIRECT) {	/* product */
	Indirect_rpg_msg *indirect;
	Pd_msg_header *hd;
	Prod_header *orpg_phd;
	int i;
	short *spt, *ipt;

	orpg_phd = (Prod_header *)(data - sizeof (Prod_header));
	hd = (Pd_msg_header *)data;
	indirect = (Indirect_rpg_msg *)(data + sizeof (Pd_msg_header));

	vv->t.prod.id = hd->msg_code;
	spt = vv->t.prod.params;
	ipt = orpg_phd->g.req_params;
	for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++)
	    spt[i] = ipt[i];
	vv->seq_num = indirect->seq_number;
	vv->t_req = indirect->req_time;
	vv->t_gen = orpg_phd->g.gen_t;
	vv->type = VV_PRODUCT;
    }
    else {
	Pd_msg_header *hd;

	hd = (Pd_msg_header *)data;
	if (hd->msg_code == MSG_REQ_RESPONSE) {
	    Pd_request_response_msg *rr;

	    rr = (Pd_request_response_msg *)data;
	    vv->t.rr.code = hd->msg_code;
	    vv->t.rr.prod_code = rr->msg_code;
	    vv->t.rr.elev = rr->elev_angle;
	    vv->t.rr.er_code = rr->error_code;
	    vv->seq_num = rr->seq_number;
	    vv->t_gen = hd->time;
	    vv->type = VV_RR;
	}
	if (hd->msg_code == MSG_ALERT) {	/* alert */
	    short *spt;
	    Prod_header *orpg_phd;

	    orpg_phd = (Prod_header *)(data - sizeof (Prod_header));
	    spt = (short *)hd;
	    vv->t.alert.code = hd->msg_code;
	    vv->t.alert.area_number = spt[11];
	    vv->t.alert.category = spt[12];
	    vv->t.alert.status = spt[10];
	    vv->t.alert.t_det = orpg_phd->g.gen_t;
	    vv->seq_num = 0;			/* to be set */
	    vv->t_gen = orpg_phd->g.gen_t;
	    vv->type = VV_ALERT;
	}
    }
}

/**************************************************************************

    Description: Outputs a V&V record before dequeue an entry.

    Inputs:	usr - the user involved;
		entry - the queue entry to be processed;
		status - V&V status.

**************************************************************************/

static void Vv_send_vv (User_struct *usr, Queue_entry *entry, int status) {
    Vv_record_t *vv;

    vv = entry->vv;
    if (vv == NULL || vv->type == VV_UNUSED)
	return;
    if (status == VV_SUCCESS)
	vv->t_sent = usr->time;
    else
	vv->t_dq = usr->time;
    vv->status = status;
    vv->size = entry->length;

    Vv_write (usr, vv);
    vv->type = VV_UNUSED;
}

/**************************************************************************

    Description: Writes a V&V record to the per user V&V file.

    Inputs:	usr - the user involved;
		vv - the V&V record.

**************************************************************************/

#define VV_NAME_SIZE 	128

static void Vv_write (User_struct *usr, Vv_record_t *vv) {
    Hwq_local *hwq;

    hwq = (Hwq_local *)usr->hwq;
    if (hwq->vv_fd < 0) {		/* open V&V file */
	int len;
	char path[VV_NAME_SIZE];

	if ((len = MISC_get_work_dir (path, VV_NAME_SIZE)) <= 0 ||
	    len + 24 >= VV_NAME_SIZE) {	/* 24 is the max file name len */
	    LE_send_msg (GL_ERROR, 
		"MISC_get_work_dir (V&V) failed (ret %d)", len);
	    return;
	}
	sprintf (path + strlen (path), "/pdvv.%d", usr->line_ind);
	hwq->vv_fd = open (path, O_RDWR | O_CREAT, 0660);
	if (hwq->vv_fd < 0) {
	    LE_send_msg (GL_ERROR, "open V&V file (%s) failed", path);
	    return;
	}
	ftruncate (hwq->vv_fd, 0);
    }
    if (write (hwq->vv_fd, vv, sizeof (Vv_record_t)) != sizeof (Vv_record_t))
	LE_send_msg (GL_ERROR, "write V&V failed");
}



