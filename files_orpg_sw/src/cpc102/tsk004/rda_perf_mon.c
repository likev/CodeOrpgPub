/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:20 $
 * $Id: rda_perf_mon.c,v 1.1 2011/05/22 16:48:20 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#define RDA_EVENTS
#include <perf_mon.h>

/* RDA events.  Note:  To avoid having to include RDA source files when 
   compiling, these events are hardcoded.   If the RDA event numbers 
   change, this file will need to be modified. */
#define START_OF_CUT		43
#define START_OF_VOLUME		45

/* Global Variables. */
Scan_info_t Scan_info;
Scan_info_t Prev_scan_info;
extern time_t Delta_time;
extern time_t Start_time;

/* Function Prototypes. */
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg );

#ifdef BYTE_SWAP
static char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len );
#endif

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Register the start of volume and start of elevation events (RDA).

   Returns:
      0 on success, -1 on failure.

/////////////////////////////////////////////////////////////////////////\*/
int RDA_register_events(){

   int ret;

   /* Register for start of elevation event. */
   ret = EN_register( START_OF_CUT, An_callback );
   if( ret < 0 ){

      fprintf( stderr, "EN_register START_OF_CUT event failed (ret %d)\n", ret );
      return -1;

   }

   ret = EN_register( START_OF_VOLUME, An_callback );
   if( ret < 0 ){

      fprintf( stderr, "EN_register START_OF_VOLUME event failed (ret %d)\n", ret );
      return -1;

   }

   /* Register the start of volume event. */

#ifdef BYTE_SWAP
   /* According to the RDA folks, I do not need to byte-swap the event 
      message.  If this turns out not to be the case, you'll have to compile 
      in the following code. */

#ifdef LITTLE_ENDIAN_MACHINE
   /* Set byte swapping function for event message. */
   EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_msg);
#endif
#endif

   /* Initialize the Scan_info_t data structure. */
   memset( &Scan_info, 0, sizeof(Scan_info_t) );
   memset( &Prev_scan_info, 0, sizeof(Scan_info_t) );

   return 0;

/* End of RDA_register_events(). */
}

/*\////////////////////////////////////////////////////////////////////////

   Description:
      The event callback function.  Checks if the event is an event
      that is recognized.  If it is, checks for Start of Elevation or
      Start of Volume.

   Input:      
      See LB man page.
        
////////////////////////////////////////////////////////////////////////\*/
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg ){

   static int vcp = -1;
   static int rda_cut_num = -1;

   /* Save previous scan info. */
   memcpy( &Prev_scan_info, &Scan_info, sizeof(Scan_info_t) );

   /* Verify the event ..... looking for ORPGEVT_SCAN_INFO. */
   if( evtcd == START_OF_VOLUME ){

      if( msg != NULL )
         vcp = *((int *) msg);

      else
         vcp = -1;

      Scan_info.scan_type = VOLUME_EVENT;
      rda_cut_num = 1;

   }
   else if( evtcd == START_OF_CUT ){

      if( msg != NULL )
         rda_cut_num = *((int *) msg); 

      else
         rda_cut_num = -1; 


      /* At the start of volume, we also get a start of cut.  
         However, the cut number will be one so we can ignore 
         this event.  Need to make sure the scan type is not 
         VOLUME_EVENT because if it is, then the volume event 
         has not been serviced yet. */
      if( (rda_cut_num == 1) && (Scan_info.scan_type != VOLUME_EVENT) )
         Scan_info.scan_type = NO_EVENT;

      else
         Scan_info.scan_type = ELEVATION_EVENT;
   }
   else
      Scan_info.scan_type = NO_EVENT;

   /* Fill in the Scan_info fields. */
   Scan_info.scan_time = time(NULL);
   Scan_info.rda_elev_num = rda_cut_num;
   Scan_info.vcp = vcp;

   /* Set the delta time (time between current and previous scan. */
   Delta_time = Scan_info.scan_time - Prev_scan_info.scan_time;
   if( Prev_scan_info.scan_time == 0 )
      Delta_time = 0;

   /* Set the start time if not already set. */
   if( Start_time == 0 )
      Start_time = Scan_info.scan_time;

/* End of An_callback() */
}

#ifdef BYTE_SWAP
#ifdef LITTLE_ENDIAN_MACHINE
/*\////////////////////////////////////////////////////////////////////////

   Description:
      Byte-swapping function for event.

   Note:
      This is required on LITTLE_ENDIAN_MACHINE since the message is
      passed Big Endian format.

////////////////////////////////////////////////////////////////////////\*/
static char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len ){

   static char swapped_msg[4];

   if( ((event == START_OF_VOLUME)
              ||
       (event == START_OF_CUT))
              && 
         (msg != NULL) ){

      MISC_bswap( sizeof(int), msg, 1, swapped_msg );
      memcpy( msg, swapped_msg, msg_len );
      return( msg );

   }

   return( msg );

/* End of Process_event_msg() */
}

#endif
#endif
