/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/26 21:46:10 $
 * $Id: pcipdalg.c,v 1.22 2012/10/26 21:46:10 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */
#define PCIPDALG_C
#include <pcipdalg.h>
#include <gagedata.h>

/* Global Variables. */
pid_t Pid;

/* Static Global Variables. */
static int Read_wx_status_set = 0;

/* Function Prototypes. */
void LB_notify_fx( int fd, LB_id_t msg_id, int msg_info, void *arg );

/*\/////////////////////////////////////////////////////////////////////////////

   Description:
      Main routine for Precipitation Detection Function.

   Inputs:
      argc - number of command line arguments.
      argv[] - the command line arguments.

   Returns:
      Always returns 0.

   Notes:
      This function is also referred to as the Auto Mode Selection algorithm.
      At present it detects weather mode changes and the VCP that is executed  
      is the default for the new weather mode.

/////////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int retval;

   /* Specify inputs. */
   RPGC_reg_io( argc, argv );

   /* Specify "PRFSEL" as optional.  If this data is not being generated,
      we still want the Mode Selection Function to execute. */
   RPGC_in_opt_by_name( "PRFSEL", 20 );

   /* Specify outputs. */
   RPGC_out_data_by_name_wevent( "PRCIPMSG", ORPGEVT_PRECIP_CAT_CHNG );

   /* Register the scan summary array. */
   RPGC_reg_scan_summary();

   /* Register for adaptation data. */
   RPGC_reg_ade_callback( hydromet_prep_callback_fx, &Hydromet_prep,
                          HYDROMET_PREP_DEA_NAME, BEGIN_VOLUME );
   Msf_ade_id = RPGC_reg_ade_callback( mode_select_callback_fx, &Mode_select,
                                       MODE_SELECT_DEA_NAME, BEGIN_VOLUME );

   /* Register for external event (ORPGEVT_START_OF_VOLUME_DATA) */
   RPGC_reg_for_external_event( ORPGEVT_START_OF_VOLUME_DATA,
                                A3052C_start_of_volume,
                                ORPGEVT_START_OF_VOLUME_DATA );

   /* Register the internal event (EVT_ANY_INPUT_AVAILABLE) */
   RPGC_reg_for_internal_event( EVT_ANY_INPUT_AVAILABLE,
                                A3052A_buffer_control,
                                INPUT_AVAILABLE );

   /* Register for LB Notification.  Since this process also writes to this LB,
      make sure we open with "LB_WRITE" access before we register for UN 
      callbacks ... we do this to ensure the LB won't need to be re-opened
      because of access privelege conflict. */
   if( ((retval = RPGC_data_access_open( ORPGDAT_GSM_DATA, LB_WRITE )) < 0)
                                  ||
       ((retval = RPGC_data_access_UN_register( ORPGDAT_GSM_DATA, WX_STATUS_ID, 
                                                LB_notify_fx )) < 0) )
      LE_send_msg( GL_INFO, "Unable to Register for UN Notification: %d\n", retval );

   /* Ready to go ...... */
   RPGC_task_init( VOLUME_BASED, argc, argv );

   /* Get this processes PID.   This will be checked whenever we get UN Notification. */
   Pid = getpid();

   /* Read the previous Weather Mode Status. */
   Read_wx_status();

   /* Get the product IDs of the inputs/outputs this task produces. */
   Hybrscan_id = RPGC_get_id_from_name( "HYBRSCAN" );
   Prcipmsg_id = RPGC_get_id_from_name( "PRCIPMSG" );
   Prfsel_id = RPGC_get_id_from_name( "PRFSEL" );
   if( (Hybrscan_id == - 1)
            ||
       (Prcipmsg_id == - 1)
            ||
       (Prfsel_id == - 1) ){

      RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name Failed.  Committing Hari Kiri.\n" );
      RPGC_hari_kiri();

   }

   /* Wait for events. */
   while(1){

      /* When an event occurs, the event service routine is called, followed by 
         return from RPGC_wait_for_event().   Since we have nothing to do outside 
         the event service routine, wait for next event. */
      RPGC_wait_for_event();

      /* If need to read weather status, read it now. */
      if( Read_wx_status_set ){

         RPGC_log_msg( GL_INFO, "Wx Status Updated!  Update Local Status\n" );
         Read_wx_status_set = 0;
         Read_wx_status();

      }

   } /* End of "while(1)" loop. */

   return 0;

} /* End of main() */

/*\/////////////////////////////////////////////////////////////////////////////

   Description:
      Service routine for LB notification.

   Inputs:
      see LB man page for details.

/////////////////////////////////////////////////////////////////////////////\*/
void LB_notify_fx( int fd, LB_id_t msg_id, int msg_info, void *arg ){

   pid_t sender_id = EN_sender_id();

   RPGC_log_msg( GL_INFO, "PID: %d.  Process PID %d Updated Wx Status\n",
                 Pid, sender_id );
   if( sender_id != Pid ){

      LE_send_msg( GL_INFO, "Read Wx Status With Next Available Input\n" );
      Read_wx_status_set = 1;

   }

/* End of LB_notify_fx() */
}
