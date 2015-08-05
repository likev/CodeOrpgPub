
/******************************************************************

	file: psv_rda_rpg_status.c

	This module contains functions that access the RDA/RPG status
	information.
	
******************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/31 17:18:13 $
 * $Id: psv_rda_rpg_status.c,v 1.95 2014/07/31 17:18:13 steves Exp $
 * $Revision: 1.95 $
 * $State: Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <orpg.h>
#include <mrpg.h>
#include <prod_user_msg.h>
#include <gen_stat_msg.h>
#include <orpgsum.h>

#include "psv_def.h"

#define MAX_N_VOL 			10
#define MAX_DELTA_DBZ0			8 	/* Corresponds to 2.0 in 1/4 dBZ increments */
#define MAX_DELTA_DBZ0_DELTA		4	/* Corresponds to 1.0 in 1/4 dBZ increments */
#define DUAL_POL_EXPECTED		0x20    
#define DUAL_POL_NOT_EXPECTED           0x1f
#define SECS_IN_HOUR			3600
#define MOMENT_MASK			DUAL_POL_NOT_EXPECTED

static int N_users;			/* number of users (links) */
static User_struct **Users;		/* user structure list */

static Pd_general_status_msg Cr_gsm;	/* current RDA/RPG status */
static int Version = 0;			/* version number of the current GSM */

static unsigned int Cr_volume_number;	/* current volume number */
static time_t Cr_volume_time;		/* current volume time */
static unsigned int Cr_hour;		/* current hour */
static unsigned int Pr_hour;			/* previous hour */
static time_t System_volume_time;	/* system time at which the volume 
					   started */
static int Cr_channel_num = 0;		/* current RDA channel number */

static int Cr_rpg_wideband_alarm = 0;	/* RPG wideband alarm active */

static int Pr_rpg_wideband_alarm = 0;	/* Previous RPG wideband alarm active */

static int Pr_h_ref_calib = 0;		/* Previous Horizontal Delta dBZ0 value */

static int Pr_v_ref_calib = 0;		/* Previous Vertical Delta dBZ0 value */

static int Cr_expected_vol_dur = 300;	/* expected duration time of current
					   volume scan */

static int Pv_status;			/* previous volume status (see 
					   gen_stat_msg.h) */

static int Pv_dual_pol = 0;		/* Dual Pol expected (see 
					   gen_stat_msg.h) */

static Mrpg_state_t Rpg_state;		/* current RPG state */

typedef struct {
    int version;			/* version sent to the user 
					   previously */
} Rrs_local;


static void Update_version ();
static int Update_rda_status ();
static int Update_orda_status ();
/* static void Sort_elev( short *elevs, int size ); */  /* Keep for possible
                                                           future use. */

/**************************************************************************

    Description: This function performs an initial read of the the RPG/RDA 
		status.

**************************************************************************/

void RRS_init_read () {

    Cr_gsm.mhb.msg_code = MSG_GEN_STATUS;
    Cr_gsm.mhb.length = sizeof (Pd_general_status_msg);
    Cr_gsm.mhb.dest_id = -1;
    Cr_gsm.mhb.n_blocks = 2;

    Cr_gsm.divider = -1;
    Cr_gsm.length = 178;

    /* Open GSM data LB for write permission. */
    ORPGDA_write_permission( ORPGDAT_GSM_DATA );

    RRS_update_rpg_state ();
    RRS_update_vol_status (NULL);
    RRS_update_rpg_status ();
    RRS_update_rda_status ();

    return;
}
 
/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int RRS_initialize (int n_users, User_struct **users)
{
    int i;

    N_users = n_users;
    Users = users;

    /* allocate local data structure */
    for (i = 0; i < n_users; i++) {
	Rrs_local *rrs;

	rrs = malloc (sizeof (Rrs_local));
	if (rrs == NULL) {
	    LE_send_msg (GL_ERROR | 182,  "malloc failed");
	    return (-1);
	}
	users[i]->rrs = rrs;
	rrs->version = -1;
    }

    return (0);
}

/**************************************************************************

    Description: This function updates the current time.   It is used for
		  purposes of sending a GSM.

**************************************************************************/
void RRS_update_time( time_t t ){

   Cr_hour = t / SECS_IN_HOUR;

   if((Cr_hour != Pr_hour) && Pr_hour != 0)
      Update_version ();
}

/**************************************************************************

    Description: This function updates the RDA status. It is called 
		when the RDA status changes.

**************************************************************************/

void RRS_update_rpg_state () {
    int ret;

    ret = ORPGDA_read (ORPGDAT_TASK_STATUS, (char *)&Rpg_state, 
			sizeof (Mrpg_state_t), MRPG_RPG_STATE_MSGID);
    if (ret != sizeof (Mrpg_state_t)) {
	LE_send_msg (GL_ERROR,  
  "ORPGDA_read failed (ORPGDAT_TASK_STATUS, MRPG_RPG_STATE_MSGID) (ret %d)",
									ret);
	ORPGTASK_exit (1);
    }
}

/**************************************************************************

    Description: This function checks the RDA configuration and calls the 
                 appropriate function to update the RDA status. It is
                 called when the RDA status changes.

**************************************************************************/

void RRS_update_rda_status ()
{
    int err = 0;
    int ret = -1;

    err = ORPGRDA_get_rda_config( NULL );
    if ( err == ORPGRDA_LEGACY_CONFIG )
    {
       ret = Update_rda_status();
    }
    else if ( err == ORPGRDA_ORDA_CONFIG )
    {
       ret = Update_orda_status();
    }
    else
    {
       LE_send_msg (GL_ERROR,
          "ORPGRDA_get_rda_config returned invalid value (err %d)", err);
       ORPGTASK_exit (1);
    }

    if ( ret < 0 )
    {
       LE_send_msg (GL_ERROR | 183,  
	  "ORPGDA_read failed (ORPGDAT_GSM_DATA, RDA_STATUS_ID) (ret %d)",
									ret);
	ORPGTASK_exit (1);
    }

    return;
}

/**************************************************************************

    Description: This function updates the RPG status. It is called 
		when the RPG status changes.

**************************************************************************/

void RRS_update_rpg_status ()
{
    unsigned int bitflags ;
    int ret;

    bitflags = 0;
    ret = ORPGINFO_statefl_get_rpgopst(&bitflags) ;
    if (ret != 0) {
	LE_send_msg (GL_ERROR | 184, 
	"ORPGINFO_statefl_get_rpgopst() failed (ret %d)", ret);
	ORPGTASK_exit (1);
    }
    Cr_gsm.rpg_op_status = (short) bitflags ;

    bitflags = 0;
    ret = ORPGINFO_statefl_get_rpgalrm(&bitflags) ;
    if (ret != 0) {
	LE_send_msg (GL_ERROR | 185, 
	"ORPGINFO_statefl_get_rpgalrm() failed (ret %d)", ret);
	ORPGTASK_exit (1);
    }
    Cr_gsm.rpg_alarms = (short) bitflags ;

    /* There are new RPG alarm bits (in the upper two bytes of rpg_alarms) that
       can cause RPGOPST_MAR or RPGOPST_MAM bit to set. In this case, we set
       RPGALRM_RPGCTLFL RPG alarm bit to indicate there is a alarm. It will 
       need to update the ICD to send the new RPG alarm bits to the user. */
    if ((Cr_gsm.rpg_op_status & 
	 (ORPGINFO_STATEFL_RPGOPST_MAR | ORPGINFO_STATEFL_RPGOPST_MAM)) &&
		!(Cr_gsm.rpg_alarms & (~ORPGINFO_STATEFL_RPGALRM_NONE)))
	Cr_gsm.rpg_alarms |= ORPGINFO_STATEFL_RPGALRM_RPGCTLFL;

    Pr_rpg_wideband_alarm = Cr_rpg_wideband_alarm;
    Cr_rpg_wideband_alarm = 0;
    if( bitflags & ORPGINFO_STATEFL_RPGALRM_WBFAILRE )
	Cr_rpg_wideband_alarm = 1;

    bitflags = 0;
    ret = ORPGINFO_statefl_get_rpgstat(&bitflags) ;
    if (ret != 0) {
	LE_send_msg (GL_ERROR | 186, 
	"ORPGINFO_statefl_get_rpgstat() failed (ret %d)", ret);
	ORPGTASK_exit (1);
    }
    Cr_gsm.rpg_status = (short) bitflags ;

    Cr_gsm.rpg_nb_status = 0;

    Update_version ();

    return;

}
 
/**************************************************************************

    Description: This function updates the volume status. It is called 
		when a volume scan starts.

**************************************************************************/

void RRS_update_vol_status (void *avset_ss)
{
    Vol_stat_gsm_t vol;
    int vcp_changed, n_elev, ind;
    int avset_term_ang, ret, i;
    short elev_angle[MAX_GS_N_ELEV + MAX_GS_ADD_ELEV];

    static int p_n_sails_cuts = 0;
    static int p_n_elev = 0;
    static int p_vcp = 0;

    ret = ORPGDA_read (ORPGDAT_GSM_DATA, (char *)&vol, 
			sizeof (Vol_stat_gsm_t), VOL_STAT_GSM_ID);
    if (ret != sizeof (Vol_stat_gsm_t)) {
	LE_send_msg (GL_ERROR | 187,  
	      "ORPGDA_read failed (ORPGDAT_GSM_DATA, VOL_STAT_GSM_ID) (ret %d)",
									ret);
	ORPGTASK_exit (1);
    }

    Cr_volume_number = vol.volume_number;
    Cr_volume_time = UNIX_SECONDS_FROM_RPG_DATE_TIME 
				(vol.cv_julian_date, vol.cv_time);
    System_volume_time = MISC_systime (NULL);
    Pv_status = vol.pv_status;
    Cr_expected_vol_dur = vol.expected_vol_dur;

    Cr_gsm.wx_mode = vol.mode_operation;
    Cr_gsm.vcp = vol.vol_cov_patt;

    /* Set the previous volume VCP number. */
    if (p_vcp == 0)
       p_vcp = vol.vol_cov_patt;

    /* VCP number change? */
    vcp_changed = 0;
    if (vol.vol_cov_patt != p_vcp)
       vcp_changed = 1;

    /* Set the number of elevations this VCP. */
    if ((vcp_changed) || (p_n_elev == 0))
       p_n_elev = vol.num_elev_cuts; 

    /* Save the VCP number and AVSET termination angle. */
    p_vcp = vol.vol_cov_patt;
    avset_term_ang = vol.avset_term_ang;
    
    Cr_gsm.super_res = vol.super_res_cuts;
    Pv_dual_pol = vol.dual_pol_expected;
    Cr_gsm.vcp_supp_data = vol.vcp_supp_data;

    if( Pv_dual_pol )
       Cr_gsm.data_trans_enable |= DUAL_POL_EXPECTED;

    else
       Cr_gsm.data_trans_enable &= DUAL_POL_NOT_EXPECTED;

    /* In the event all moments are disabled, clear the dual pol expected bit. */
    if( (Cr_gsm.data_trans_enable & MOMENT_MASK) == BD_ENABLED_NONE )
       Cr_gsm.data_trans_enable = BD_ENABLED_NONE;

    /* If AVSET is not enabled, set the number of cuts as defined
       in volume status. */
    if ( ((Cr_gsm.vcp_supp_data & VSS_AVSET_ENABLED) == 0)
                             ||
        (p_n_sails_cuts != vol.n_sails_cuts) )
       p_n_elev = vol.num_elev_cuts;

    /* Save the number of SAILS cuts. */
    p_n_sails_cuts = vol.n_sails_cuts;

    /* If Avset is enabled and Scan Summary data is defined,
       set the number of cuts and the Avset termination angle 
       as defined by the scan summary data. */
    if (avset_ss != NULL) {
	Scan_Summary *ss = (Scan_Summary *)avset_ss;
	if (ss->avset_status == 2 && ss->last_rda_cut != 0xff){
	    p_n_elev = ss->last_rda_cut;
            avset_term_ang = ss->avset_term_ang;
        }
    }

    ind = -1;
    n_elev = 0;
    for (i = 0; i < p_n_elev; i++) {

        /* Matching RPG index ... skip. */
	if (ind == vol.elev_index[i])
	    continue;
	if (n_elev >= MAX_GS_N_ELEV + MAX_GS_ADD_ELEV) {
	    LE_send_msg (GL_ERROR | 188,  
		"Too many elevations (%d) found in VOL_STAT_GSM_ID", n_elev);
	    break;
	}

        /* Save the elevation angle. */
	ind = vol.elev_index[i];
	elev_angle[n_elev] = vol.elevations[i];
	n_elev++;

        /* If AVSET is enabled, the elevation angle matches the last 
           Avset termination angle and the VCP has not changed, break 
           out of loop. */
        if( (!vcp_changed) 
                 && 
            (vol.elevations[i] == avset_term_ang)
                 &&
            (Cr_gsm.vcp_supp_data & VSS_AVSET_ENABLED))
           break;
    }
    Cr_gsm.n_elev = n_elev;

    for (i = n_elev; i < MAX_GS_N_ELEV + MAX_GS_ADD_ELEV; i++)
	elev_angle[i] = 0;

    memcpy (Cr_gsm.elev_angle, elev_angle, MAX_GS_N_ELEV * sizeof (short));
    memcpy (Cr_gsm.add_elev_angle, elev_angle + MAX_GS_N_ELEV,
					MAX_GS_ADD_ELEV * sizeof (short));

    Update_version ();

    return;
}

/**************************************************************************

    Description: This function returns non-zero, if RPG currently is in
		test mode, or 0 otherwise.

**************************************************************************/

int RRS_rpg_test_mode ()
{
    if (Rpg_state.test_mode != MRPG_TM_NONE)
	return (1);
    else
	return (0);
}

/**************************************************************************

    Description: This function returns non-zero, if RPG currently is in
		inactive mode, or 0 otherwise.

**************************************************************************/

int RRS_rpg_inactive ()
{
    if (Rpg_state.active == MRPG_ST_ACTIVE)
	return (0);
    return (1);
}

/**************************************************************************

    Description: This function returns the current RDA operability status.

    Return:	Returns the current RDA operability status.

**************************************************************************/

int RRS_get_RDA_op_status ()
{

    return (Cr_gsm.rda_op_status);
}

/**************************************************************************

    Description: This function returns the expected current volume duration
		time in seconds.

    Return:	Returns the expected current volume duration time.

**************************************************************************/

int RRS_get_volume_duration ()
{

    return (Cr_expected_vol_dur);
}

/**************************************************************************

    Description: This function returns the current volume number, the 
		clock time when the volume started and the volume time.

    Output:	clock - the clock time when the volume started if not NULL.
		vtime - the current volume time if not NULL.

    Return:	Returns the current volume number.

**************************************************************************/

unsigned int RRS_get_volume_number (time_t *clock, time_t *vtime)
{

    if (clock != NULL)
	*clock = System_volume_time;
    if (vtime != NULL) 
	*vtime = Cr_volume_time;
    return (Cr_volume_number);
}

/**************************************************************************

    Description: This function returns the General Status Message.

    Return:	Returns the General Status Message on success or NULL on
		failure.

**************************************************************************/

char *RRS_gsm_message (User_struct *usr)
{
    Pd_general_status_msg *gsm;
    char *buf;

    buf = WAN_usr_msg_malloc (sizeof (Pd_general_status_msg));
    if (buf == NULL)
	return (NULL);

    memcpy (buf, (char *)&Cr_gsm, sizeof (Pd_general_status_msg));

    gsm = (Pd_general_status_msg *)(buf);
    GUM_date_time (usr->time, &(gsm->mhb.date), &(gsm->mhb.time));
    gsm->max_connect_time = PSAI_max_connect_time (usr);
    gsm->mhb.dest_id = -1;
    gsm->distri_method = usr->up->distri_method;
    gsm->RDA_channel_num = Cr_channel_num;
    gsm->build_version = ORPGMISC_RPG_build_number();

    ((Rrs_local *)usr->rrs)->version = Version;

    return (buf);
}

/**************************************************************************

    Description: This function sends the updated gms to all users.

**************************************************************************/

void RRS_send_gms ()
{
    int i;

    for (i = 0; i < N_users; i++) {
	User_struct *usr;
	char *buf;

	usr = Users[i];
	if (usr->psv_state == ST_ROUTINE &&
	    (usr->up->cntl & UP_CD_STATUS) &&
	    ((Rrs_local *)usr->rrs)->version != Version &&
	    (buf = RRS_gsm_message (usr)) != NULL) {
	    LE_send_msg (LE_VL1 | 189,  "sending general status msg, L%d\n", 
							usr->line_ind);
	    HWQ_put_in_queue (usr, 0, buf);
	}
    }
}

/**************************************************************************

    Description: This function checks whether the General Status Message
		is updated and updates the version number (Version) if
		it is updated.

**************************************************************************/

static void Update_version ()
{
    static Pd_general_status_msg prev_gsm;	/* previous RDA/RPG status */
    int changed, diff;

    changed = 0;
    if ((Version == 0) || (Cr_hour != Pr_hour)) {

        if (Cr_hour != Pr_hour) {

           LE_send_msg( GL_INFO, "Sending GSM At Top of Hour\n" );
           Pr_hour = Cr_hour;

        }
	changed = 1;

    }
    else {
	if (prev_gsm.rda_op_status != Cr_gsm.rda_op_status ||
	    prev_gsm.rda_status != Cr_gsm.rda_status ||
	    prev_gsm.rda_alarms != Cr_gsm.rda_alarms ||
	    prev_gsm.data_trans_enable != Cr_gsm.data_trans_enable ||
	    prev_gsm.rpg_op_status != Cr_gsm.rpg_op_status ||
	    prev_gsm.rpg_alarms != Cr_gsm.rpg_alarms ||
	    prev_gsm.rpg_status != Cr_gsm.rpg_status ||
	    prev_gsm.rpg_nb_status != Cr_gsm.rpg_nb_status ||
	    prev_gsm.wx_mode != Cr_gsm.wx_mode ||
	    prev_gsm.vcp != Cr_gsm.vcp ||
	    Pr_rpg_wideband_alarm != Cr_rpg_wideband_alarm ||
	    prev_gsm.n_elev != Cr_gsm.n_elev ||
	    prev_gsm.super_res != Cr_gsm.super_res ||
	    prev_gsm.vcp_supp_data != Cr_gsm.vcp_supp_data ||
	    (prev_gsm.cmd & 0x1) != (Cr_gsm.cmd & 0x1) )
	    changed = 1;
	else {
	    int i;

            diff = abs((int) (Pr_h_ref_calib - Cr_gsm.ref_calib));
            if( (diff >= MAX_DELTA_DBZ0_DELTA) ||
                ((abs((int) Cr_gsm.ref_calib) >= MAX_DELTA_DBZ0) && (diff != 0)) )
                changed = 1;

            diff = abs((int) (Pr_v_ref_calib - Cr_gsm.v_ref_calib));
            if( (diff >= MAX_DELTA_DBZ0_DELTA) ||
                ((abs((int) Cr_gsm.v_ref_calib) >= MAX_DELTA_DBZ0) && (diff != 0)) )
                changed = 1;

	    for (i = 0; i < MAX_GS_N_ELEV; i++)
		if (prev_gsm.elev_angle[i] != Cr_gsm.elev_angle[i])
		    changed = 1;
	    for (i = 0; i < MAX_GS_ADD_ELEV; i++)
		if (prev_gsm.add_elev_angle[i] != Cr_gsm.add_elev_angle[i])
		    changed = 1;
	}
    }

    if (changed) {

        unsigned short data_trans_enable;

	/* Set the Product Availability word of the GSM. */
	Cr_gsm.prod_avail = GSM_PA_PROD_AVAIL;

        data_trans_enable = Cr_gsm.data_trans_enable & MOMENT_MASK;

	/* The following criteria are for Degraded Product Availability. */
	if( ((data_trans_enable != BD_ENABLED_NONE) &&
	     (data_trans_enable != BD_ENABLED_ALL)) ||
	    (Cr_gsm.rpg_alarms & ORPGINFO_STATEFL_RPGALRM_DBFL) ||
	    (Cr_gsm.rpg_alarms & ORPGINFO_STATEFL_RPGALRM_RPGCTLFL) ||
	    (Cr_gsm.rpg_alarms & ORPGINFO_STATEFL_RPGALRM_RPGTSKFL) ||
	    (Cr_gsm.rpg_alarms & ORPGINFO_STATEFL_RPGALRM_NODE_CON) ||
	    (Cr_gsm.rpg_alarms & ORPGINFO_STATEFL_RPGALRM_MEDIAFL) )
	    Cr_gsm.prod_avail = GSM_PA_DEGRADED_AVAIL;

	/* The following criteria are for Products Unavailable. */
	if( (data_trans_enable == BD_ENABLED_NONE) ||
	    (Cr_rpg_wideband_alarm) ||
	    (Cr_gsm.rda_status == RS_STANDBY) ||
	    (Cr_gsm.rda_status == RS_RESTART) ||
	    (Cr_gsm.rda_status == RS_OFFOPER) ||
	    (Cr_gsm.rda_op_status & OS_INOPERABLE) ||
	    (Cr_gsm.rda_op_status & OS_INDETERMINATE) ||
	    (Cr_gsm.rda_op_status & OS_COMMANDED_SHUTDOWN) ||
	    (Cr_gsm.rda_op_status & OS_WIDEBAND_DISCONNECT) ||
	    (Cr_gsm.rpg_status & ORPGINFO_STATEFL_RPGSTAT_STANDBY) ||
	    (Cr_gsm.rpg_status & ORPGINFO_STATEFL_RPGSTAT_RESTART) ||
	    (Cr_gsm.rpg_op_status & ORPGINFO_STATEFL_RPGOPST_CMDSHDN) )
	    Cr_gsm.prod_avail = GSM_PA_PROD_NOT_AVAIL;

	memcpy ((char *)&prev_gsm, (char *)&Cr_gsm, 
				sizeof (Pd_general_status_msg));

        Pr_rpg_wideband_alarm = Cr_rpg_wideband_alarm;

	Version++;
    }
    return;
}

/**************************************************************************

    Description: This function resets this module when a new user is 
		connected.

    Inputs:	usr - the user involved.

*************************************************************************/

void RRS_new_user (User_struct *usr)
{

    ((Rrs_local *)usr->rrs)->version = -1;

    return;
}

/**************************************************************************

    Description: This function updates the Legacy RDA status. It is called 
		when the Legacy RDA status changes.

**************************************************************************/

static int Update_rda_status ()
{
   int err;
   int ret_val = 0; /* 0 = OK, Neg = Failure */
   RDA_status_t rda;

   err = ORPGDA_read (ORPGDAT_GSM_DATA, (char *)&rda, 
      sizeof (RDA_status_t), RDA_STATUS_ID);
   if (err != sizeof (RDA_status_t))
   {
      LE_send_msg (GL_ERROR | 183,  
         "ORPGDA_read failed (ORPGDAT_GSM_DATA, RDA_STATUS_ID) (err %d)",
         err);
      ret_val = -1; 
   }
   else
   {  
      Cr_gsm.rda_op_status = rda.status_msg.op_status;
      Cr_gsm.rda_status = rda.status_msg.rda_status;
      Cr_gsm.rda_alarms = rda.status_msg.rda_alarm;
      Cr_gsm.data_trans_enable = rda.status_msg.data_trans_enbld;
      Cr_gsm.ref_calib = rda.status_msg.ref_calib_corr;
      Cr_gsm.v_ref_calib = 0;

      /* If the Operability Status is Commanded Shutdown, then set 
         the RDA alarm field to Indeterminate. */
      if ((Cr_gsm.rda_op_status & 0xfffe) == OS_COMMANDED_SHUTDOWN )
         Cr_gsm.rda_alarms = 1;

      /* NOTE: there is no RDA Build Num in the Legacy RDA Status
         message.  So, we make sure this value is set to 0. */
      Cr_gsm.RDA_build_num = 0;
      Cr_channel_num = rda.status_msg.msg_hdr.rda_channel;

      if (rda.wb_comms.rda_display_blanking == RS_OPERABILITY_STATUS)
         Cr_gsm.rda_status = 0;
      if (rda.wb_comms.rda_display_blanking == RS_RDA_STATUS)
         Cr_gsm.rda_op_status = 0;
      if (rda.wb_comms.rda_display_blanking != 0){

         Cr_gsm.ref_calib = 0;
         Cr_gsm.data_trans_enable = BD_ENABLED_NONE;

      }
      
      Update_version ();
   }

   return ret_val;

} /* end Update_rda_status */

/**************************************************************************

    Description: This function updates the Open RDA status. It is called 
		when the Open RDA status changes.

**************************************************************************/

static int Update_orda_status ()
{
   int err;
   int ret_val = 0; /* 0 = OK, Neg = Failure */
   ORDA_status_t rda;
   float temp_val;
   int alarm_mask = 0x00ff;
   char chan_mask = 0x03;     /* Mask for masking out upper 6 bits of rda
                                channel field. */

   err = ORPGDA_read (ORPGDAT_GSM_DATA, (char *)&rda, 
      sizeof (ORDA_status_t), RDA_STATUS_ID);
   if (err != sizeof (ORDA_status_t))
   {
      LE_send_msg (GL_ERROR | 183,  
         "ORPGDA_read failed (ORPGDAT_GSM_DATA, RDA_STATUS_ID) (err %d)",
         err);
      ret_val = -1; 
   }
   else
   {  
      Cr_gsm.rda_op_status = rda.status_msg.op_status;
      Cr_gsm.rda_status = rda.status_msg.rda_status;
      Cr_gsm.rda_alarms = rda.status_msg.rda_alarm & alarm_mask;

      /* If the Operability Status is Commanded Shutdown, then set 
         the RDA alarm field to Indeterminate. */
      if ((Cr_gsm.rda_op_status & 0xfffe) == OS_COMMANDED_SHUTDOWN )
         Cr_gsm.rda_alarms = 1;

      Cr_gsm.data_trans_enable = rda.status_msg.data_trans_enbld;

      if( Pv_dual_pol )
          Cr_gsm.data_trans_enable |= DUAL_POL_EXPECTED;

      else
          Cr_gsm.data_trans_enable &= DUAL_POL_NOT_EXPECTED;

    /* In the event all moments are disabled, clear the dual pol expected bit. */
    if( (Cr_gsm.data_trans_enable & MOMENT_MASK) == BD_ENABLED_NONE )
       Cr_gsm.data_trans_enable = BD_ENABLED_NONE;

      /* Horizontal and Vertical Channel dBZ0 values. */
      Pr_h_ref_calib = Cr_gsm.ref_calib;
      temp_val = ((float) rda.status_msg.ref_calib_corr)/100.0;
      if( temp_val < 0.0 )
         Cr_gsm.ref_calib = (short) ((temp_val-0.125)*4.0);
      else 
         Cr_gsm.ref_calib = (short) ((temp_val+0.125)*4.0);

      Pr_v_ref_calib = Cr_gsm.v_ref_calib;
      temp_val = ((float) rda.status_msg.vc_ref_calib_corr)/100.0;
      if( temp_val < 0.0 )
         Cr_gsm.v_ref_calib = (short) ((temp_val-0.125)*4.0);
      else 
         Cr_gsm.v_ref_calib = (short) ((temp_val+0.125)*4.0);

      /* Starting in RDA Build 11.2, the build number is scaled by
         100.  In order to not change the GSM, we scale by 10. */
      if( (rda.status_msg.rda_build_num / 100) > 2 )
         Cr_gsm.RDA_build_num = rda.status_msg.rda_build_num / 10;
      else
         Cr_gsm.RDA_build_num = rda.status_msg.rda_build_num;
      Cr_gsm.cmd = rda.status_msg.cmd;

      /* 
         For ORDA, the upper 6 bits of the RDA Channel field in the message
         header are used to describe the configuration (ORDA vs Legacy, etc.).
         For the purposes of the GSM message to the users, we can mask these
         values out and just tell them the RDA Channel value.
      */

      Cr_channel_num = (rda.status_msg.msg_hdr.rda_channel & chan_mask);

      if (rda.wb_comms.rda_display_blanking == RS_OPERABILITY_STATUS)
         Cr_gsm.rda_status = 0;
      if (rda.wb_comms.rda_display_blanking == RS_RDA_STATUS)
         Cr_gsm.rda_op_status = 0;
      if (rda.wb_comms.rda_display_blanking != 0){

         Cr_gsm.ref_calib = 0;
         Cr_gsm.v_ref_calib = 0;
         Cr_gsm.data_trans_enable = BD_ENABLED_NONE;

      }

      Update_version ();
   }

   return ret_val;

} /* end Update_orda_status */


/**************************************************************************

    Description: Sorts the list of elevation angles in ascending order.

**************************************************************************/
/*
static void Sort_elev ( short *elevs, int n_elevs )
{

   int fu;
   int place;
   short temp;

   for( fu = 1; fu < n_elevs; fu++ ){

      if( elevs[fu] < elevs[fu-1] ){
         temp = elevs[fu];

         for( place = fu-1; place >= 0; place-- ){
            elevs[place+1] = elevs[place];
            if( (place == 0) || (elevs[place-1] <= temp) )
               break;
         }
         elevs[place] = temp;
      }

   }

}*/ /* end Sort_elev() */
