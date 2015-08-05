
/******************************************************************

	file: psv_process_state_routine.c

	This module processes the state of ST_ROUTINE.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/05/09 20:47:16 $
 * $Id: psv_process_state_routine.c,v 1.83 2012/05/09 20:47:16 jing Exp $
 * $Revision: 1.83 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

#define FTMD_NEED_TABLE		/* get A309_ftmd_line_spec */
#include <a309.h>

#include <infr.h>
#include <orpg.h>
#include <orpgerr.h>
#include <orpgevt.h>
#include <prod_status.h>
#include <prod_user_msg.h>
#include <prod_distri_info.h>
#include <prod_gen_msg.h>

#include "psv_def.h"

typedef struct {		/* local part of the User_struct */
    char *ppdl;			/* The previous prod distri list */
    int n_ppdl;			/* num of products in ppdl */
    int ppdl_size;		/* buffer size for ppdl */
} Psr_local;

static int N_users;		/* number of users (links) */
static User_struct **Users;	/* user structure list */

/* local functions */
static void Process_user_msg (User_struct *usr, char *umsg);
static void Rps_timer_expiration (User_struct *usr);
static void Send_text_msg (User_struct *usr, char *data);
static void Process_pup_text_msg (User_struct *usr, char *umsg);
static void Process_environmental_data_msg (User_struct *usr, char *umsg);
static void Process_external_data_msg (User_struct *usr, char *umsg);
static void Set_all_user_bit (int value, char *umsg);
static void Set_user_bit (int line_ind, char *umsg);


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int PSR_initialize (int n_users, User_struct **users)
{
    int i;

    N_users = n_users;
    Users = users;

    /* allocate local data structure */
    for (i = 0; i < n_users; i++) {
	Psr_local *psr;

	psr = malloc (sizeof (Psr_local));
	if (psr == NULL) {
	    LE_send_msg (GL_ERROR | 152,  "malloc failed");
	    return (-1);
	}
	users[i]->psr = psr;
	psr->ppdl = NULL;
	psr->n_ppdl = 0;
	psr->ppdl_size = 0;
    }

    return (0);
}

/**************************************************************************

    Description: This function resets this module when a new user is 
		connected.

    Inputs:	usr - the user involved.

**************************************************************************/

void PSR_new_user (User_struct *usr)
{
    Psr_local *psr;

    psr = (Psr_local *)usr->psr;
    psr->n_ppdl = 0;

    return;
}

/**************************************************************************

    Description: This function processes the state ST_ROUTINE.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PSR_routine (int ev, User_struct *usr, void *ev_data)
{

    if (RRS_rpg_test_mode () && !(usr->up->cntl & UP_CD_RPGOP)) {
	PSR_enter_test_mode (usr);
	return;
    }
    if (RRS_rpg_inactive ()) {
	PSR_enter_inactive_mode (usr);
	return;
    }

    switch (ev) {

	case EV_USER_DATA:
	    Process_user_msg (usr, ev_data);
	    break;

	case EV_NEXT_STATE:
	    /* to start distribution immediately */
	    EN_post (ORPGEVT_PROD_STATUS, NULL, 0, 0);
	    break;

	case EV_WRITE_COMPLETED:
	    HWQ_wan_write (usr);
	    break;

	case EV_NEW_VOL_SCAN:
	    break;

	case EV_WX_ALERT:
	    break;

	case EV_SESSION_TIMER:
	    PSR_session_expiration (usr);
	    break;

	case EV_RPS_TIMER:
	    Rps_timer_expiration (usr);
	    break;

	case EV_TEXT_MSG:		/* text message to be distributed */
	    Send_text_msg (usr, ev_data);
	    break;

	case EV_ENTER_TEST:
	case EV_ENTER_INACTIVE:
	    break;

	default:
	    LE_send_msg (0, "event (%d) ignored - PSR_routine", ev);
	    break;
    }
    return;
}

/**************************************************************************

    Description: This function processes the procedure of session
		expiration.

    Inputs:	usr - the user involved.

**************************************************************************/

void PSR_session_expiration (User_struct *usr)
{
    char *msg;

    LE_send_msg (GL_INFO | 155, 
		"session timer expired (line %d)", usr->line_ind);
    HWQ_shed_all (usr);			/* remove msgs in the queue */
    msg = RRS_gsm_message (usr);	/* get a general status msg */
    if (msg != NULL) {
	Pd_general_status_msg *gs;
	gs = (Pd_general_status_msg *)msg;
	gs->rpg_nb_status |= HALFWORD_SHIFT(15);/* commanded disconnect */
	gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
	HWQ_put_in_queue (usr, 0, msg);		/* send to the user */
    }

    usr->discon_reason = US_SESSION_EXPIRED;
    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
    usr->psv_state = ST_DISCONNECTING;
    usr->next_state = 1;

    return;
}

/**************************************************************************

    Returns the LE_send_msg code for status messages that needs to be
    supressed in case of bad line.

    Inputs:	usr - the user involved.

**************************************************************************/

int PSR_status_log_code (User_struct *usr) {
    if (usr->status_msg_cnt < MAX_STATUS_MSGS)
	return ( (GL_STATUS | LE_RPG_COMMS) );
    else
	return (GL_INFO);
}

/**************************************************************************

    Processes status message supression in case of bad line.

    Inputs:	usr - the user involved.

**************************************************************************/

void PSR_process_status_supression (User_struct *usr) {
    if (usr->status_msg_cnt < MAX_STATUS_MSGS) {
	usr->status_msg_cnt++;
	if (usr->status_msg_cnt == MAX_STATUS_MSGS)
	    LE_send_msg (GL_STATUS | LE_RPG_COMMS, 
		"Status Message suppression starts due to errors for line %d",
							    usr->line_ind);
    }
}

/**************************************************************************

    Description: This function processes the procedure of RPS timer
		expiration.

    Inputs:	usr - the user involved.

**************************************************************************/

static void Rps_timer_expiration (User_struct *usr)
{
    char *msg;

    LE_send_msg (PSR_status_log_code (usr),
			"RPS timer expired on line %d", usr->line_ind);
    PSR_process_status_supression (usr);

    HWQ_shed_all (usr);			/* remove msgs in the queue */
    msg = RRS_gsm_message (usr);	/* get a general status msg */
    if (msg != NULL) {
	Pd_general_status_msg *gs;
	gs = (Pd_general_status_msg *)msg;
	gs->rpg_nb_status |= HALFWORD_SHIFT(15);/* commanded disconnect */
	gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
	HWQ_put_in_queue (usr, 0, msg);		/* send to the user */
    }

    usr->discon_reason = US_SESSION_EXPIRED;
    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
    usr->psv_state = ST_DISCONNECTING;
    usr->next_state = 1;

    return;
}


/**************************************************************************

    Description: This function sends a general status message indicating 
		test mode and terminates the connection.

    Inputs:	usr - the user involved.

**************************************************************************/

void PSR_enter_test_mode (User_struct *usr)
{
    char *msg;

    HWQ_shed_all (usr);		/* remove msgs in the queue */
    msg = RRS_gsm_message (usr);	/* get a general status msg */
    if (msg != NULL) {
	Pd_general_status_msg *gs;
	gs = (Pd_general_status_msg *)msg;
	gs->rpg_nb_status |= HALFWORD_SHIFT(15);/* commanded disconnect */
	gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
	gs->rpg_status |= HALFWORD_SHIFT(11);	/* test mode */
	HWQ_put_in_queue (usr, 0, msg);		/* send to the user */
    }

    usr->discon_reason = US_IN_TEST_MODE;
    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
    usr->psv_state = ST_DISCONNECTING;
    usr->next_state = 1;

    return;

}

/**************************************************************************

    Description: This function sends a general status message indicating 
		switching to inactive mode and terminates the connection.

    Inputs:	usr - the user involved.

**************************************************************************/

void PSR_enter_inactive_mode (User_struct *usr)
{
#ifdef OLD_CODE
    Pd_general_status_msg *gs;

    HWQ_shed_all (usr);		/* remove msgs in the queue */
    gs = (Pd_general_status_msg *)RRS_gsm_message (usr);
				/* get a general status msg */
    if (gs != NULL) {
	gs->rpg_nb_status |= HALFWORD_SHIFT(15); /* switch to inactive */
	gs->prod_avail = GSM_PA_PROD_NOT_AVAIL; 
	HWQ_put_in_queue (usr, 0, (char *)gs);
    }

    usr->discon_reason = US_IN_TEST_MODE;
    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
    usr->psv_state = ST_DISCONNECTING;
    usr->next_state = 1;
#endif

    WAN_disconnect (usr);
    usr->discon_reason = US_IN_TEST_MODE;
    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
    usr->psv_state = ST_DISCONNECTING;

    return;
}

/**************************************************************************

    Description: This function processes a user message.

    Inputs:	usr - the user involved.
		umsg - pointer to the user message.

**************************************************************************/

static void Process_user_msg (User_struct *usr, char *umsg)
{
    Pd_msg_header *hd;

    hd = (Pd_msg_header *)umsg;

    /* check source ID */
    if (!(usr->up->cntl & UP_CD_MULTI_SRC)) {
	if (hd->src_id != usr->up->user_id) {
	    LE_send_msg (GL_ERROR | 157,  
		"bad user msg source ID (%d, L%d) discarded", 
					hd->src_id, usr->line_ind);
	    usr->bad_msg_cnt++;
	    return;
	}
    }

    switch (hd->msg_code) {
	char *msg;

	case MSG_PROD_REQUEST:
	case MSG_PROD_REQ_CANCEL:
	    HP_process_prod_request (usr, umsg);
	    break;

	case MSG_MAX_CON_DISABLE:
	    PSAI_process_max_connect_time_disable_request_msg (usr, umsg);
	    break;

	case MSG_EXTERNAL_DATA:	/* External Data message */
	    Process_external_data_msg (usr, umsg);
	    break;

	case MSG_ALERT_REQUEST:
	    PWA_alert_request (usr, umsg);
	    break;

	case MSG_ENVIRONMENTAL_DATA:	/* Environmental Data message */
	    Process_environmental_data_msg (usr, umsg);
	    break;

	case 75:			/* PUP text message */
	    Process_pup_text_msg (usr, umsg);
	    break;

	case MSG_PROD_LIST:		/* request for GSM */
	    LE_send_msg (LE_VL1, 
		    "GSM requested, L%d\n", usr->line_ind);
	    msg = RRS_gsm_message (usr);  /* get a general status msg */
	    if (msg != NULL)
		HWQ_put_in_queue (usr, 0, msg);
	    break;

	default:
	    LE_send_msg (GL_INFO | 161,  
		"user (line %d) message (%d) ignored - Process_user_msg", 
					usr->line_ind, hd->msg_code);
	    break;
    }
    return;
}

/**************************************************************************

    Description: Sends a text message to a user.

    Inputs:	usr - the user involved.
		data - RPG text message info.

**************************************************************************/

static void Send_text_msg (User_struct *usr, char *data)
{
    Send_text_msg_t *stm;
    Prod_header *phd;
    Indirect_rpg_msg *indirect;
    Pd_prod_header *mhb;
    int need_to_send;
    char *buf;

    stm = (Send_text_msg_t *)data;

    need_to_send = 0;
    if (stm->ftmd_type_spec & (FTMD_ALL | FTMD_ALL_NB_USERS))
	need_to_send = 1;

    if (usr->up->defined & UP_DEFINED_CLASS) {	/* match class */
	int class;

	class = usr->up->class;
	if (class >= 1 && class <= 5 &&
		(stm->ftmd_type_spec & (1 << class)))
	    need_to_send = 1;
    }

    {						/* match line index */
	int ind, bit;

	ind = usr->line_ind / 16;
	bit = usr->line_ind % 16;
	if (ind < FTMD_LINE_SPEC_SIZE &&
	    (stm->ftmd_line_spec[ind] & (1 << bit)))
	    need_to_send = 1;
    }
    if (!need_to_send)
	return;

    buf = WAN_usr_msg_malloc (sizeof (Pd_prod_header) + 
					sizeof (Indirect_rpg_msg));
    if (buf == NULL)
	return;

    memcpy (buf - sizeof (Prod_header), stm->hd, 
			sizeof (Prod_header) + sizeof (Pd_prod_header));

    phd = (Prod_header *)stm->hd;
    indirect = (Indirect_rpg_msg *)(buf + sizeof (Pd_prod_header));
    indirect->orpgdat = stm->data_type;
    indirect->msg_id = stm->msgid;
    indirect->vol_number = phd->g.vol_num;
    indirect->seq_number = phd->g.req_num;
    indirect->elev_angle = 0;

    mhb = (Pd_prod_header *)buf;
    mhb->dest_id = -1;

    LE_send_msg (LE_VL1 | 163,  "sending RPG text msg to L%d\n", 
							usr->line_ind);
    HP_set_to_use_db_read (phd, indirect);
    HWQ_put_in_queue (usr, 
	    HWQ_TYPE_ONETIME | HWQ_TYPE_INDIRECT, buf);
    return;
}

/**************************************************************************

    Description: This converts a PUP text message to the RPG text message
		format.

    Inputs:	usr - the user involved.

    Input/Output:	umsg - the text message.

**************************************************************************/

static void Process_pup_text_msg (User_struct *usr, char *umsg)
{
    int ret, len;
    int user_desig;
    unsigned short *spt;
    char *buf;
    Prod_gen_msg *pgm;

    if (usr->up->cntl & UP_CD_FREE_TEXTS)
	LE_send_msg (LE_VL1 | 164, 
			"PUP text msg received, L%d\n", usr->line_ind);
    else {
	LE_send_msg (LE_VL1, 
			"PUP text msg rejected, L%d\n", usr->line_ind);
	return;
    }

    spt = (unsigned short *)umsg;
    len = (spt[LGMSWOFF] << 16) | spt[LGLSWOFF];
    if (len < PHEADLNG * (int)sizeof (short)) {
	LE_send_msg (GL_ERROR | 165,  "text msg too short (%d)", len);
	return;
    }

    /* convert to RPG text message format */
    Set_all_user_bit (0, umsg);
    user_desig = ((unsigned short *)umsg)[48];	/* see ICD */
    if (user_desig != 0)
	Set_user_bit (user_desig - 1, umsg);
    else {
	Pd_user_entry *up;

	up = usr->up;
	if (up->cntl & UP_CD_RPGOP)	/* send to all users */
	    Set_all_user_bit (0xffff, umsg);
	else {				/* send to all dedicated users */
	    int i;

	    for (i = 0; i < N_users; i++) {
		if (Users[i]->line_type == DEDICATED)
		    Set_user_bit (Users[i]->line_ind, umsg);
	    }
	}
    }

    /* add ORPG product header */
    buf = malloc (len + sizeof (Prod_header));
    if (buf == NULL) {
	LE_send_msg (GL_ERROR | 166,  "malloc failed (size %d)", 
					len + sizeof (Prod_header));
	return;
    }
    memset (buf, 0, sizeof (Prod_header));
    memcpy (buf + sizeof (Prod_header), umsg, len);
    pgm = (Prod_gen_msg *)buf;
    pgm->len = len + sizeof (Prod_header);
    pgm->prod_id = FTXTMSG;
    pgm->gen_t = time (NULL);
    pgm->vol_t = pgm->gen_t;

    ret = UMC_product_back_to_icd (buf + sizeof (Prod_header), len);
				/* convert to ICD format */

    if (ret != len) {
	LE_send_msg (GL_ERROR | 167,  
		"UMC_product_back_to_icd (FTXTMSG) failed (ret %d)", ret);
	free (buf);
	return;
    }

    /* send for distirbution */
    ret = ORPGDA_write (FTXTMSG, buf, len + sizeof (Prod_header), LB_ANY);
    if (ret < 0) 
	LE_send_msg (GL_ERROR | 168,  
		    "ORPGDA_write FTXTMSG failed (ret %d)\n", ret);
    EN_post (ORPGEVT_FREE_TXT_MSG, NULL, 0, 0);

    /* send product generation message */
    ret = ORPGDA_write (ORPGDAT_PROD_GEN_MSGS, buf, 
				sizeof (Prod_gen_msg), LB_ANY);
    if (ret < 0)
	LE_send_msg (GL_ERROR | 169,  
		"ORPGDA_write ORPGDAT_PROD_GEN_MSGS failed (ret %d)", ret);    

    /* send for RPG display */
    ret = ORPGDA_write (ORPGDAT_PUP_TXT_MSG, buf, 
				len + sizeof (Prod_header), LB_ANY);
    if (ret < 0) 
	LE_send_msg (GL_ERROR | 170,  
		"ORPGDA_write ORPGDAT_PUP_TXT_MSG failed (ret %d)\n", ret);
    EN_post (ORPGEVT_PUP_TXT_MSG, NULL, 0, 0);

    free (buf);
    return;
}

/**************************************************************************

    Description: Sets a user bit in an RPG text message.

    Inputs:	line_ind - line number.

    Input/Output:	umsg - the text message.

**************************************************************************/

static void Set_user_bit (int line_ind, char *umsg)
{
    unsigned short *spt;
    int word, bit;

    word = line_ind / 16;
    if (line_ind < 0 || word >= FTMD_LINE_SPEC_SIZE) {
	LE_send_msg (GL_ERROR | 171,  
		"bad user designation in text msg (%d)\n", line_ind);
	return;
    }
    bit = line_ind % 16;

    spt = (unsigned short *)umsg;
    spt[A309_ftmd_line_spec[word]] |= (1 << bit);
    return;
}

/**************************************************************************

    Description: Sets all user bits in an RPG text message.

    Inputs:	value - the value to set.

    Input/Output:	umsg - the text message.

**************************************************************************/

static void Set_all_user_bit (int value, char *umsg)
{
    unsigned short *spt;
    int i;

    spt = (unsigned short *)umsg;
    for (i = 0; i < FTMD_LINE_SPEC_SIZE; i++)
	spt[A309_ftmd_line_spec[i]] = value;
}

/**************************************************************************

    Description: Writes the environmental data to data store.

    Inputs:	usr - the user involved.  (Currently not used, added for
                      possible future use.)

    Input/Output:	umsg - the environmental data message.

**************************************************************************/

static void Process_environmental_data_msg (User_struct *usr, char *umsg)
{
    int ret, type, len;
    unsigned short *spt;
    unsigned short *block_spt;

    LE_send_msg (LE_VL1, "Environmental Data msg received, L%d\n", usr->line_ind);

    spt = (unsigned short *)umsg;
    len = (spt[LGMSWOFF] << 16) | spt[LGLSWOFF];

    /* Get to the start of the block. */
    block_spt = spt + ALIGNED_SIZE((int)sizeof(Pd_msg_header))/sizeof(short);
    type = block_spt[1];

    switch( type ){

       case BIAS_TABLE_BLOCK_ID:
       {

          Pd_bias_table_msg *bias_tab = (Pd_bias_table_msg *) umsg;
          LE_send_msg( LE_VL1, "Bias Table Data (%d) Received\n", type );  

          /* Strip off the product header length from total length. */
          len -= ALIGNED_SIZE((int) sizeof(Pd_msg_header));

          /* write to data store */
          ret = ORPGDA_write (ORPGDAT_ENVIRON_DATA_MSG, (void *) &bias_tab->block, len, 
                              ORPGDAT_BIAS_TABLE_MSG_ID);
          if (ret < 0) 
	     LE_send_msg (GL_ERROR,  
		    "ORPGDA_write ORPGDAT_ENVIRON_DATA_MSG failed (ret %d)\n", ret);
          break;
       }

       default:
       {
          LE_send_msg( LE_VL1, "Unknown Block ID (%d) For Environmental Data\n", type );
          break;

       }

    }
    return;
}

/**************************************************************************

    Description: Writes external data messages to the proper data store.

    Inputs:	usr - the user involved.  (Currently not used, added for
                      possible future use.)

    Input/Output:	umsg - the external data message.

**************************************************************************/

static void Process_external_data_msg (User_struct *usr, char *umsg)
{
    int ret, type, len;
    unsigned short *spt;
    unsigned short *block_spt;


    LE_send_msg (LE_VL1, "External Data msg received, L%d\n", usr->line_ind);

    spt = (unsigned short *)umsg;
    len = (spt[LGMSWOFF] << 16) | spt[LGLSWOFF];

    /* Get to the start of the block. */
    block_spt = spt + ALIGNED_SIZE((int)sizeof(Pd_msg_header))/sizeof(short);

    /* NOTE: up to this point, the msg hdr has been the only thing that has 
       been byte swapped.  If necessary, we need to swap before using data. */
    type = block_spt[1];

    #ifdef LITTLE_ENDIAN_MACHINE 
       type = SHORT_BSWAP(block_spt[1]);
    #endif

    switch( type ){

       case RUC_MODEL_DATA_BLOCK_ID:
       {
	  Pd_msg_header *msg_hd = (Pd_msg_header *)umsg;
          LE_send_msg( LE_VL1, "RUC Model Data (Type %d) Received\n", type );  
          LE_send_msg( GL_STATUS,
	      "Model Data Message Received from Narrowband Line %d (User %d)",
	      msg_hd->line_ind, msg_hd->src_id);  

          /* Strip off the product header length from total length. */
          len -= ALIGNED_SIZE((int) sizeof(Pd_msg_header));

          /* write to data store */
          ret = ORPGDA_write (ORPGDAT_ENVIRON_DATA_MSG,
             (void *)(umsg + ALIGNED_SIZE((int)sizeof(Pd_msg_header))), len,
             ORPGDAT_RUC_DATA_MSG_ID);
          if (ret < 0) 
	     LE_send_msg (GL_ERROR,  
		    "ORPGDA_write ORPGDAT_ENVIRON_DATA_MSG failed (ret %d)\n", ret);
          break;
       }

       default:
       {
          LE_send_msg( LE_VL1, "Unknown Block ID (%d) For External Data\n", type );
          break;
       }
    }

    return;
}

