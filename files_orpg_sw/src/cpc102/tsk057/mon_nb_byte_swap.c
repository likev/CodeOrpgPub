/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/05 15:20:57 $
 * $Id: mon_nb_byte_swap.c,v 1.4 2012/10/05 15:20:57 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/


#include <prod_user_msg.h>
#include <legacy_prod.h>
#include <orpg.h>
#include <infr.h>

#define MSG_HDR_SIZE_SHORTS   (sizeof(Prod_msg_header_icd)/sizeof(short))
#define MSG_HDR_SIZE_BYTES    sizeof(Prod_msg_header_icd)

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Byte-swaps the message received from RDA.  
//
//   Inputs:
//      msg_data - pointer to the ICD message.
//      size - size of the ICD message, in bytes.
//
//   Returns:
//      0 - on success, -1 on failure.
//
//   Notes:
//      No message validation is done.
//
////////////////////////////////////////////////////////////////////////\*/
int From_ICD( short *msg_data, int size ){

#ifdef LITTLE_ENDIAN_MACHINE

   /* Byte-swap the message header. */ 
   MISC_swap_shorts( MSG_HDR_SIZE_SHORTS, msg_data );

   /* Process according to message type. */
   switch( msg_data[0] ){

      case MSG_MAX_CON_DISABLE:
      {
         Prod_max_conn_time_disable_msg_icd *array =
                       (Prod_max_conn_time_disable_msg_icd *) msg_data;
         MISC_swap_shorts( 5, &(array->divider) );  

         break;
      }

      case MSG_SIGN_ON:
      {
         Prod_sign_on_msg_icd *array = (Prod_sign_on_msg_icd *) msg_data;
         MISC_swap_shorts( 2, &(array->divider) );
         MISC_swap_shorts( 2, &(array->disconn_override_flag) );
   
         break;
      }

      case MSG_ALERT_REQUEST:
      {
         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size/sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      case MSG_ENVIRONMENTAL_DATA:
      {
         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size / sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      case MSG_EXTERNAL_DATA:
      {
         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size / sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      case MSG_PROD_REQUEST:
      case MSG_PROD_REQ_CANCEL:
      {
         /* We don't process this message here .... we use the UMC library instead. */
         break;
      }

      case MSG_ALERT_PARAMETER:
      {

         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size / sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      case MSG_PROD_LIST:
      {

         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size / sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      case MSG_ALERT:
      {
         Prod_alert_msg_icd *array = (Prod_alert_msg_icd *) msg_data;

         MISC_swap_shorts( 11, &(array->divider) );
         MISC_swap_shorts( 4, &(array->vol_num) );

         break;
      }

      case MSG_GEN_STATUS:
      {
         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size / sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      case MSG_REQ_RESPONSE:
      {
         unsigned short *array = (unsigned short *) msg_data;

         array += MSG_HDR_SIZE_SHORTS;
         MISC_swap_shorts( (size / sizeof(short)) - MSG_HDR_SIZE_SHORTS, (short *) array );

         break;
      }

      default:
      {
         fprintf( stderr, "Unknown Message Type in From_ICD() (%d)\n", msg_data[0] );
         return(-1);
      }

   /* End of "switch" */
   }

#endif

   return 0;

}
