/**************************************************************************
   
   Module:  rms_narrowband_interface.c   
   
   Description:  This module builds authorized user message to be sent to RMMS.  Upon
   receipt of this command the specified user message is sent to RMMS for 
   editing.
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:22 $
 * $Id: rms_narrowband_interface.c,v 1.13 2003/06/26 14:51:22 garyg Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define MAX_NAME_SIZE   256
#define SINGLE_LINE   1
/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

static int rms_change_nb_interface(short start, short end, ushort command);

/**************************************************************************
   Description:  This function reads the command from the message buffer.
      
   Input: nb_inter_buf - Pointer to the message buffer.
      
   Output: Sends command to enable/disable narrwoband lines.

   Returns: 0 = Successful command.

   Notes:

   **************************************************************************/

int rms_rec_nb_inter_command (UNSIGNED_BYTE *nb_inter_buf) {

   UNSIGNED_BYTE *nb_inter_buf_ptr;
   int           ret;
   short         first, last, cmd_flag;

   /* Set pointer to beginning of buffer */
   nb_inter_buf_ptr = nb_inter_buf;

   /* Place pointer past header */
   nb_inter_buf_ptr += MESSAGE_START;

   /* Get command to be executed */
   cmd_flag = conv_shrt(nb_inter_buf_ptr);
   nb_inter_buf_ptr += PLUS_SHORT;

   /* Get number of first narrowband line to connect/disconnect */
   first = conv_shrt(nb_inter_buf_ptr);
   nb_inter_buf_ptr += PLUS_SHORT;

   /* Get number of last narrowband line to connect/disconnect. If zero
      then only one line is affected if not zero indicates the last line in
      the range of lines to be connected/disconnected*/
   last = conv_shrt(nb_inter_buf_ptr);
   nb_inter_buf_ptr += PLUS_SHORT;

   /* Command must be either 1 or 2 */
   if (cmd_flag > 0 && cmd_flag < 3){
      ret = rms_change_nb_interface(first, last, cmd_flag);
      if (ret != 1)
         return (ret);
      return (0);
   } /* call send clutter */

   return (24);

} /* End rms rec nb inter command */


/**************************************************************************
   Description:  This function connects or disconnects the narrowband
   interface(s) based on line numbers sent from RMMS.

   Input: start - The beginning line number to be changed.
           end - The ending line number to be changed.
           command - The command retrieved from the RMMS message.

   Output:

   Returns: Command successful = 1, Command not successful = -1

   Notes:

   **************************************************************************/
static int rms_change_nb_interface(short start, short end, ushort command){

   int i, j;
   int start_int, end_int, n_lines;
   int ret;
   int *line_index;
   int wb_link;

   start_int = (int)start;
   end_int = (int)end;

   /* Get the line number of the wideband connection */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* Validation checks */
   if ( start_int < 1 || start_int > 47 ) {
      LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid starting line number", start_int);
      return (3);
   } /* End if */

   if ( end_int < 2 && end_int != 0) {
      LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid ending line number", end_int);
      return (3);
   } /* End if */

   if ( end_int > 47) {
      LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid ending line number", end_int);
      return (3);
   } /* End if */

   if ( start_int > end_int && end_int != 0){
      LE_send_msg(RMS_LE_LOG_MSG,
         "Start line number (%d) is greater than end line number (%d)", 
         start_int,end_int);
      return (24);
   } /* End if */

   /* Ending line number is zero indicating a single line */
   if (end_int == 0){
      /* Check to see if wideband line selected */
      if ( wb_link != (start_int -1)){
         n_lines = SINGLE_LINE;
         start_int --;
         /* Place only single line number in line index */
         line_index = &start_int;
      }else{
          LE_send_msg(RMS_LE_ERROR,"Wideband line selected (line %d)", wb_link);
          return (24);
      }/* End else */
   }else{
      /* Set number of lines to ending line number */
      n_lines = end_int;

      /* Allocate memory to store line numbers */
      line_index = (int*)malloc((n_lines * (sizeof(int))));

      j = 0;

      /* If not a single line build a line index of the lines to 
         be connected/disconnected */
      for(i=(start_int - 1); i<= (end_int -1 ); i++){

         if(i < 0){
            LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid line number",i);
            /* Free memory */
            if ( n_lines != SINGLE_LINE){
               free (line_index);
            } /* End if */

            return (3);
         } /* End if */

         /* Check for wideband line and ignore it if found */
         if ( wb_link != i ){
            line_index[j] = i;
            j++;
         }else{
             LE_send_msg(RMS_LE_ERROR,
                "Wideband line selected (line %d)", wb_link);
         }  /* End else */
      }
   }

   /* Disable command */
   if(command == 1){

      /* Send the command to disable the lines in the line index */
      ret = ORPGNBC_enable_disable_NB_links(NBC_DISABLE_LINK, n_lines, line_index);

      if (ret < 0){
         LE_send_msg(RMS_LE_LOG_MSG,
           "Unable to disable narrowband lines (ret %d)", ret);
         /* Free memory */
         if ( n_lines != SINGLE_LINE){
            free (line_index);
         } /* End if */

         return (ret);
      } /* End if */

   } /* End if */

   /* Enable command */
   if(command == 2){

      /* Send the command to enable the lines in the line index */
      ret = ORPGNBC_enable_disable_NB_links(NBC_ENABLE_LINK, n_lines, line_index);

      if ( ret < 0){
         LE_send_msg(RMS_LE_LOG_MSG,
           "Unable to enable narrowband lines (ret %d)", ret);
         /* Free memory */
         if ( n_lines != SINGLE_LINE){
            free (line_index);
         } /* End if */

         return (ret);
      } /* End if */

   } /* End if */

   /* Free memory */
   if ( n_lines != SINGLE_LINE){
      free (line_index);
   } /* End if */
      
   return (0);   
      
} /*End rms change nb interface */
