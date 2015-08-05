/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:20 $
 * $Id: rpg_perf_mon.c,v 1.1 2011/05/22 16:48:20 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#define RPG_EVENTS
#include <perf_mon.h>
#include <dlfcn.h>

/* Global Variables. */
Scan_info_t Scan_info;
Scan_info_t Prev_scan_info;
extern time_t Delta_time;
extern time_t Start_time;

/* For dynamic loading of library liborpg.so. */
void *ORPG_library = NULL;
extern int Library_link_failed;
int (*ORPGDA_read_fp)() = NULL;

/* Function Prototypes. */
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg );
static char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len );

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Register the scan information event (RPG).

   Returns:
      0 on success, -1 onf failure.

/////////////////////////////////////////////////////////////////////////\*/
int RPG_register_events(){

   int ret;

   /* Register for Scan Info Event. */
   ret = EN_register( ORPGEVT_SCAN_INFO, An_callback );
   if( ret < 0 ){

      fprintf( stderr, "EN_register scan info event failed (ret %d)\n", ret );
      return -1;

   }

#ifdef LITTLE_ENDIAN_MACHINE
   /* Set byte swapping function for event message. */
   EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_msg);
#endif

   /* Initialize the Scan_info_t data structure. */
   memset( &Scan_info, 0, sizeof(Scan_info_t) );
   memset( &Prev_scan_info, 0, sizeof(Scan_info_t) );

   /* Open the liborpg library ... needed for calls to ORPGDA_read(). */
   ORPG_library = dlopen( "liborpg.so", RTLD_LAZY | RTLD_GLOBAL );
   if( ORPG_library == NULL )
      return -1;

   /* Get addresss of ORPGDA_read function. */
   if( ORPGDA_read_fp == NULL ){

      *(void **)(&ORPGDA_read_fp) = dlsym( ORPG_library, "ORPGDA_read" );
      if( ORPGDA_read_fp == NULL ){

         Library_link_failed = 1;
         return -1;

      }

   }

   /* Assume the library link was successful. */
   Library_link_failed = 0;

   return 0;

/* End of RPG_register_events(). */
}

/*\////////////////////////////////////////////////////////////////////////

   Description:
      The RPG event callback function.  Checks if the event is a
      Scan Info event.  If it is, checks for Start of Elevation or
      Start of Volume.

   Input:      
      See LB man page.
        
////////////////////////////////////////////////////////////////////////\*/
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg ){

   Orpgevt_scan_info_t *scan_info;

   /* Verify the event ..... looking for ORPGEVT_SCAN_INFO. */
   if( evtcd == ORPGEVT_SCAN_INFO ){

      scan_info = (orpgevt_scan_info_t *) msg;

      /* If end of volume or end of elevation, just return. */
      if( (scan_info->key == ORPGEVT_END_ELEV) 
                          ||
          (scan_info->key == ORPGEVT_END_VOL) ) 
         return;

      /* Copy the previous scan information data. */
      memcpy( &Prev_scan_info, &Scan_info, sizeof(Scan_info_t) );

      /* Start of elevation .... */
      if( scan_info->key == ORPGEVT_BEGIN_ELEV )
         Scan_info.scan_type = ELEVATION_EVENT;

      /* Start of volume .... */
      else if (scan_info->key == ORPGEVT_BEGIN_VOL)
         Scan_info.scan_type = VOLUME_EVENT;

      Scan_info.scan_time = scan_info->data.time/1000 + (scan_info->data.date-1)*86400;
      Scan_info.rda_elev_num = scan_info->data.elev_cut_number;
      Scan_info.vcp = scan_info->data.vcp_number;

      /* Set the delta time (time between current and previous scan). */
      Delta_time = Scan_info.scan_time - Prev_scan_info.scan_time;
      if( Prev_scan_info.scan_time == 0 )
         Delta_time = 0;

      /* Set the start time if not already set. */
      if( Start_time == 0 )
         Start_time = Scan_info.scan_time;

   }

/* End of An_callback() */
}

#ifdef LITTLE_ENDIAN_MACHINE
/*\////////////////////////////////////////////////////////////////////////

   Description:
      Byte-swapping function for scan info event.

   Note:
      This is required on LITTLE_ENDIAN_MACHINE since the message is
      passed Big Endian format.

////////////////////////////////////////////////////////////////////////\*/
static char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len ){

   static char swapped_msg[ ORPGEVT_SCAN_INFO_DATA_LEN ];
   int num_ints = msg_len / sizeof( int );

   if( (event == ORPGEVT_SCAN_INFO) && (msg != NULL) ){

      MISC_bswap( sizeof(int), msg, num_ints, swapped_msg );
      memcpy( msg, swapped_msg, msg_len );
      return( msg );

   }

   return( msg );

/* End of Process_event_msg() */
}

#endif
