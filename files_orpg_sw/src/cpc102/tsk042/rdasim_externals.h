/********************************************************************************

    Description: process level global variables for the RDA simulator

 ********************************************************************************/


/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/31 17:59:05 $
 * $Id: rdasim_externals.h,v 1.10 2014/07/31 17:59:05 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

#include <rdasim_simulator.h>
#include <rda_status.h>

#ifndef  RDASIM_EXTERNALS_H
#define RDASIM_EXTERNALS_H


#ifdef RDASIM_MAIN
#define EXTERN
#else
#define EXTERN extern
#endif


EXTERN short  REstart_elevation_cut;     /* specifies the elevation cut to restart */
EXTERN short  VErbose_mode;              /* user set verbosity level */
EXTERN short  RDA_channel_number;        /* RDA Channel number */
EXTERN short  DATa_trans_enbld;          /* The moments enabled for the current scan. */
EXTERN link_t LINk;                      /* the comm link structure */
EXTERN ORDA_status_msg_t RDA_status_msg; /* RDA status message */
EXTERN int    COmpress_radials;		 /* Compress Message 31 data. */


   /* The following is the list of exceptions that can be sent 
      to the simulator via Event Notification by tool rdasim_tst */

EXTERN int FAT_forward;                      /* Send a forward FAT radial */
EXTERN int FAT_backward;                     /* Send a backward FAT radial */
EXTERN int NEG_start_angle;                  /* Send a negative azimuth at start of elevation. */
EXTERN int BAD_elevation_cut;                /* Send a bad elevation number */
EXTERN int MAX_radial_exceeded;              /* Send more than 400 radials */
EXTERN int UNExpected_elevation;             /* Send an unexpected elevation start */
EXTERN int UNExpected_volume;                /* Send an unexpected volume start */
EXTERN int BAD_segment_bypass;               /* Send a bad segment for the bypass map */
EXTERN int BAD_segment_notch;                /* Send a bad segment for the notchwidth map */
EXTERN int BAD_start_vcp;                    /* Send a bad vcp on volume start */
EXTERN int IGNore_volume_elevation_restart;  /* Ignore a volume restart */
EXTERN int BEFore_volume_start_data;         /* Send data before the start of volume */
EXTERN int SKIp_start_of_vol;                /* Skip start of volume message */
EXTERN int LOOpback_timeout;                 /* Cause loopback timeout */
EXTERN int LOOpback_scramble;                /* Cause loopback scramble */
EXTERN int RDA_alarm_test;                   /* RDA Alarm test */
EXTERN int RDA_alarm_code;                   /* RDA Alarm Code */

#endif
