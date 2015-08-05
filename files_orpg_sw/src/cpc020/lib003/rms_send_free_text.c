/**************************************************************************
   
   Module:  rms_send_free_text_msg.c   
   
   Description:  This module sends a free text message to RMMS.  
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:36 $
 * $Id: rms_send_free_text.c,v 1.12 2003/06/26 14:51:36 garyg Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <rms_message.h>
#include <rda_rpg_console_message.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define FREE_TYPE      13
#define TEXT_PAD_SIZE  422


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This function write the string to the message buffer.
      
   Input: free_text_buf - Pointer to the message buffer.
      
   Output: Free text message sent to FAA/RMMS

   Returns: None

   Notes:

   **************************************************************************/

void rms_send_free_text_msg () {

   UNSIGNED_BYTE free_text_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE *free_text_buf_ptr;
   char          *Msg;
   int           ret, i, msg_size;
   LB_info       info;
   ushort        message_size_halfwords, num_halfwords;

   /* Search for the latest free text message. */
   if((ret = ORPGDA_seek(ORPGDAT_RMS_TEXT_MSG, 0, LB_LATEST, &info)) < 0) {
       LE_send_msg(RMS_LE_ERROR, "Failed seek RMMS free text message (%d).\n", ret );
   }else {

      /* Allocate memory for the message. */
      Msg = (char*)malloc(info.size);

      /* Read the latest message. */
      ret = ORPGDA_read (ORPGDAT_RMS_TEXT_MSG, (char *) Msg, info.size, info.id);

      if(ret <0 ){
         LE_send_msg(RMS_LE_ERROR, "Failed read RMMS free text message (%d).\n", ret );
      }else {
         /* Set the pointer to beginning of buffer */
         free_text_buf_ptr = free_text_buf;

         /* Set the pointer past the header */
         free_text_buf_ptr += MESSAGE_START;

         /* Compute the size of the free text string in halfwords */
         message_size_halfwords = (info.size/2);

         /* Adjust for rounding */
         message_size_halfwords += (info.size%2);

         /* Put free text string size in output buffer */
          conv_ushort_unsigned(free_text_buf_ptr,&message_size_halfwords);
         free_text_buf_ptr += PLUS_SHORT;

         /* Put free text string in output buffer */
         for (i=0; i< info.size; i++){
            conv_char_unsigned(free_text_buf_ptr, &Msg[i], PLUS_BYTE);
            free_text_buf_ptr += PLUS_BYTE;
         } /* End loop */

         /* Compute the size of the message */
         msg_size = (free_text_buf_ptr - free_text_buf);

         /* Pad the message */
         pad_message (free_text_buf_ptr, msg_size, TEXT_PAD_SIZE);
         free_text_buf_ptr += (TEXT_PAD_SIZE - msg_size);

         /* Add the terminator to the message */
         add_terminator(free_text_buf_ptr);
         free_text_buf_ptr += PLUS_INT;

         /* Compute the size of the message in halfwords */
         num_halfwords = ((free_text_buf_ptr - free_text_buf) / 2);

         /* Add header to message */
         ret = build_header(&num_halfwords, FREE_TYPE, free_text_buf, 0);

         if (ret != 1){
            LE_send_msg (RMS_LE_ERROR,
            "RMS build header failed for rms send free text message");
         } /* End if */

         /* Send message to the FAA/RMMs */
         ret = send_message(free_text_buf,FREE_TYPE,RMS_STANDARD);

         if (ret != 1){
            LE_send_msg (RMS_LE_ERROR,
            "Send message failed (ret %d) for rms send free text message", ret);
         }  /* end if */
      } /* End else */

      free(Msg);

   } /* End else */

} /*End rms send free text msg */

