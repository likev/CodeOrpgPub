/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/19 20:46:35 $
 * $Id: gen_stat_msg.h,v 1.68 2014/08/19 20:46:35 steves Exp $
 * $Revision: 1.68 $
 * $State: Exp $
 */
/*****************************************************************
//
//     Contains the structure defintions for the volume-base
//     general status message information as well as the RPG
//     status of the general status message.  The rda_status.h
//     header file contains the RDA status portion of the general
//     status message.
//    
//     The Linear Buffer data store ID is ORPGDAT_GSM_DATA
//
//     The task table is also defined here and stored in the same data 
//     store.
//
//     Scan summary data used by legacy and new algorithms
//     is stored here.
//
//     The RDA status portion is updated by the CONTROL RDA 
//     function whenever new status data is received from the 
//     RDA.   Whenever the status changes, CONTROL RDA posts
//     the event ORPGEVT_RDA_STATUS_CHANGE. 
//     
//     The volume-based status is updated at beginning of volume
//     by PROCESS BASE DATA.  PROCESS BASE DATA posts the event
//     ORPGEVT_START_OF_VOLUME whenever this data has been 
//     updated.
//
//     The RPG status portion is TBD.
//
****************************************************************/

#ifndef GEN_STAT_MSG_H
#define GEN_STAT_MSG_H


#ifdef __cplusplus
extern "C"
{
#endif

#include<rda_status.h>
#include<orpgctype.h>
#include<lb.h>
#include<vcp.h>
#include<mode_select.h>


/* Message IDs in the ORPGDAT_GSM_DATA LB */
enum { RDA_STATUS_ID, VOL_STAT_GSM_ID, PREV_RDA_STAT_ID,
       SAILS_STATUS_ID, SAILS_REQUEST_ID, WX_STATUS_ID };

#define MAX_CUTS           VCP_MAXN_CUTS 


/* Bit fields for vcp_supp_data in Volume Scan Status. */
#define VSS_AVSET_ENABLED	0x01
#define VSS_SAILS_ACTIVE	0x02
#define VSS_SITE_SPECIFIC_VCP	0x04
#define VSS_RXRN_ENABLED     	0x08
#define VSS_CBT_ENABLED     	0x10

/*    Volume Scan Status            */
/*
    Note:   This structure is also defined in a309.inc, a3cd06.  If 
            Vol_stat_gsm_t changes, the include file also needs to 
            change.  Also the fields that can be stored as chars
            need to remain ints or short ints until all RPG code
            is ported to FORTRAN. */
typedef struct volume_status {

   unsigned long volume_number;     /* Current volume scan sequence 
                                       number.  Monotonically 
                                       increases.  Initial value 0. */

   unsigned long cv_time;           /* Current volume scan time in
                                       milliseconds past midnight. */

   int cv_julian_date;              /* Current volume scan Julian date */

   int initial_vol;                 /* Flag, if set, indicates the 
                                       volume associated with volume
                                       volume is the initial volume.  
                                       It is assumed for an initial
                                       volume, no radar data-derived
                                       products will be available. */ 

   int pv_status;                   /* Previous volume scan status:
                                          1 - completed successfully,
                                          0 - aborted. */


   int expected_vol_dur;            /* Expected volume scan duration,
                                       in seconds. */

   int volume_scan;                 /* The volume scan number [0, 80]. */

   int mode_operation;              /* Mode of operation:
                                          0 - Maintenance Mode
                                          1 - Clear Air Mode        
                                          2 - Precipitation Mode */

   int dual_pol_expected;           /* Flag, if set, indicates the
                                       volume scan is expected to 
                                       have Dual Pol fields transmitted
                                       to the RPG from the RDA. */

   int vol_cov_patt;                /* Volume coverage pattern */

   int rpgvcpid;                    /* slot in vcp_table containing VCP
                                       data associated with vol_cov_patt. */

   int num_elev_cuts;               /* Number of elevations in VCP. */

   short elevations[MAX_CUTS];      /* Elevation angles (deg*10). */
   
   short elev_index[MAX_CUTS];      /* RPG elev index associated with 
                                       each elevation angle. */
   
   int super_res_cuts;		    /* Bit map indicating which RPG cuts
                                       are expected to have super res data. */
   
   short vcp_supp_data;    	    /* Bit map indicating supplemental VCP
                                       information. */

   unsigned char n_sails_cuts;      /* Number of SAILS cuts. */

   unsigned char spare;             /* Spare. */

   unsigned char sails_cut_seq[VCP_MAXN_CUTS];
                                    /* For SAILS cuts, contains the SAILS cut 
                                       sequence number. */

   unsigned char spare0;

   unsigned short avset_term_ang;   /* AVSET termination angle (deg*10), if AVSET active.  
                                       Otherwise, set to 0. */
 
   unsigned short spare1;	    /* Spare. */

   Vcp_struct current_vcp_table;    /* The current VCP data. */
         
} Vol_stat_gsm_t;


/* RDA-RPG communications status */
typedef struct {
   
   int wblnstat;    /* Wideband line status */
   int rda_display_blanking;    /* Status fields to NOT blank. 
                                      0 = None Blanked.
             >0 = Field to Blank. */
   int wb_failed;    /* If set (non 0) then the WB *
           is considered failed.  If 0,
           then not considered failed. */


} RDA_RPG_comms_status_t;

/* RDA status structure ... include comms status and RDA Status Message.
   The RDA_status_msg_t structure is defined in rda_status.h. */
typedef struct {

   RDA_RPG_comms_status_t wb_comms;
   RDA_status_msg_t       status_msg;

} RDA_status_t;
   

/* ORDA status structure ... include comms status and ORDA Status Message.
   The ORDA_status_msg_t structure is defined in rda_status.h. */
typedef struct {

   RDA_RPG_comms_status_t wb_comms;
   ORDA_status_msg_t       status_msg;

} ORDA_status_t;


/*
  Stores RDA state variables to be used to return the RDA to a previous
  state after wideband failures, restarts, etc.
*/
typedef struct previous_state {

   short rda_status;
   unsigned short data_trans_enbld;
   short rda_control_auth;
   short int_suppr_unit;
   short op_mode;
   short spot_blanking_status;
   short current_vcp_number;
   short channel_control;
   Vcp_struct current_vcp_table;  
   
}  Previous_state_t;


#define MAX_VSCANS          81
#define MAX_SCAN_SUM_VOLS   (MAX_VSCANS-1)


#define WX_STATUS_UNDEFINED   -1

/* Mode Selection Supplemental Data. */
typedef struct{

   time_t curr_time;       /* The UNIX time the detection algorithm 
                              was last executed. */
   time_t last_time;       /* UNIX time precipitation was last 
                              detected. */
   time_t time_to_cla;     /* End time for category 1 precipitation
                              detected. */
   int pcpctgry;           /* The current precipitation 
                              category.
                                 0 - No Precipitation
                                 1 - Precipitation */
   int prectgry;           /* The previous precipitation  
                              category. */
} A3052t;


typedef struct{

   int current_wxstatus;
   int current_vcp;
   int recommended_wxstatus;
   int wxstatus_deselect;
   time_t recommended_wxstatus_start_time;
   int recommended_wxstatus_default_vcp;
   time_t conflict_start_time;
   time_t current_wxstatus_time;
   float precip_area;
   Mode_select_entry_t mode_select_adapt;
   A3052t a3052t;

} Wx_status_t;


/* SAILS information. */

typedef struct {

   int n_sails_cuts;	        /* Number of SAILS cuts (as defined by 
                                   adaptation data) in use.  Must be less 
                                   than the MIN( max_sails_cuts, 
                                                 site_max_sails_cuts) */
} SAILS_status_t;

typedef struct {

   int n_req_sails_cuts;        /* Number of SAILS cuts requested to be used.
                                   Must be less than the MIN( max_sails_cuts, 
                                   max_sails_cuts_this_vcp, site_max_sails_cuts) */
} SAILS_request_t;

#ifdef __cplusplus
}
#endif

#endif
