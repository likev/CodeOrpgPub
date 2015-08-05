/**************************************************************************
   
   Module:  rms_rec_free_text_command.c   
   
   Description:  This module receives a free text message from RMMS.  Upon
   receipt of this command the message is placed in the console message lb.
   

   Assumptions:

**************************************************************************/
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/13 22:13:20 $
 * $Id: rms_rec_free_text.c,v 1.20 2012/11/13 22:13:20 ccalvert Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <rms_message.h>
#include <rda_rpg_console_message.h>
#include <rpg.h>
#include <orpgedlock.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define MAX_FREE_TEXT_SIZE       400

/*   Masks for message sources/destinations         */

#define ALL_MESSAGE              0xffff
#define RDA_MESSAGE              0x0001
#define HCI_MESSAGE              0x0002
#define APUP_MESSAGE             0x0004
#define PUES_MESSAGE             0x0008
#define LINE_MESSAGE             0x0020
#define MTYP_RDACON              10

#define MILLISECONDS_PER_SECOND  1000
#define MAX_NARROWBAND_LINES     48
#define MAX_PARAMS               12


/*
* Static Globals
*/

static short params [12] = {0};
extern int   Num_users;                       /* Number of narrowband users */
extern int   User_class [USER_TBL_SIZE];      /* Array of user classes */
extern int   User_line_index [USER_TBL_SIZE]; /* Array of user line indexes */
extern int   User_type [USER_TBL_SIZE];       /* Array of user record types */


/*
* Static Function Prototypes
*/

static int rms_build_text_msg(char *string);
static int rms_set_users (int class);

/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: free_text_buf - Pointer to the message buffer.

   Output: Send free text message to users.

   Returns: 0 = Successful command

   Notes:

   **************************************************************************/

int rms_rec_free_text_command (UNSIGNED_BYTE *free_text_buf) {

   UNSIGNED_BYTE *free_text_buf_ptr;
   int           ret, i;
   ushort        sent_to;
   ushort        message_size;
   char          temp_string [MAX_FREE_TEXT_SIZE];
   time_t        tm;
   RDA_RPG_console_message_t Msg;

   /* Set pointer to beginning of buffer */
   free_text_buf_ptr = free_text_buf;

   /* Set the pointer past the header */
   free_text_buf_ptr += MESSAGE_START;

   /* Get recievers of message */
   sent_to = conv_ushrt(free_text_buf_ptr);
   free_text_buf_ptr += PLUS_SHORT;

   /* Get message size in bytes */
   message_size = conv_ushrt(free_text_buf_ptr);
   free_text_buf_ptr += PLUS_SHORT;

   /* Add one for rounding */
   message_size = ((message_size * 2) + 1);

   /* Put user message in string */
   for (i=0; i<=message_size; i++){
      conv_char(free_text_buf_ptr, &temp_string[i], PLUS_BYTE);
      free_text_buf_ptr += PLUS_BYTE;
   } /* End loop */

   /* Clear out the parameters for the next message */
   for (i=0; i <= MAX_PARAMS; i++){
      params[i] = 0;
   }/* End loop */

   if (sent_to == 0)
      sent_to = ALL_MESSAGE;

   if (sent_to & RDA_MESSAGE) {

      /* Check to see if another process has locked RDA commands. */
      ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

      /* If command buffer locked cancel command and return error code */
      if ( ret == ORPGEDLOCK_EDIT_LOCKED){
         LE_send_msg(RMS_LE_ERROR,
            "Unable to send RDA text message RDA commands are locked");
         return (24);
      } /* End if */

      LE_send_msg (RMS_LE_LOG_MSG, "Message sent to RDA\n");

      /* Send the command to RDA. */
      ORPGRDA_send_cmd (COM4_WBMSG, (int) RMS_INITIATED_RDA_CTRL_CMD,
                        MTYP_RDACON, (int) strlen (temp_string),
                        (int) 0, (int) 0, (int) 0, temp_string,
                        strlen(temp_string)+1);

   } /* End if */

   if (sent_to & HCI_MESSAGE) {

       /* Get Julian date and time past midnight in seconds*/
      tm = time((time_t*)NULL);

      Msg.msg_hdr.julian_date = RPG_JULIAN_DATE(tm);
      Msg.msg_hdr.milliseconds = (RPG_TIME_IN_SECONDS(tm) * MILLISECONDS_PER_SECOND);
      Msg.size = message_size;
      strcpy (Msg.message, temp_string);

      /* Write the message to the HCI console message buffer */
      if((ret = ORPGDA_write (ORPGDAT_RDA_CONSOLE_MSG, (char *) &Msg, 
                              sizeof (Msg), LB_ANY)) <0) {
         LE_send_msg(RMS_LE_ERROR, 
            "Failed to write RDA console message (%d).\n", ret );
         return (-1);
      } /* End if */
   } /* End if */

   /* Set parameter to send message to APUPs */
   if (sent_to & APUP_MESSAGE) {
      ret = rms_set_users(CLASS_I);
      if(ret != 0){
         LE_send_msg(RMS_LE_ERROR, "Failed to send to CLASS_I users (%d).\n", ret );
         return(-1);
      }/* End if */

      ret = rms_set_users(RPGOP_CLASS);

      if(ret != 0){
         LE_send_msg(RMS_LE_ERROR, "Failed to send to RPGOP users (%d).\n", ret );
         return(-1);
      }/* End if */

      ret = rms_set_users(RPGOP_CLASS98);

      if(ret != 0){
         LE_send_msg(RMS_LE_ERROR, "Failed to send to RPGOP98 users (%d).\n", ret );
         return(-1);
      }/* End if */
   } /* End if */

   /* Set parameter to send message to PEUSs */
   if (sent_to & PUES_MESSAGE) {
      ret = rms_set_users( CLASS_III);

      if(ret != 0){
         LE_send_msg(RMS_LE_ERROR, "Failed to send to CLASS_III users (%d).\n", ret );
         return(-1);
      }/* End if */
   } /* End if */

   /* If a user is found for the narrowband type send the message */
   if((params[0] != 0) || (params[1] !=0) || (params[2] !=0)){

      /* Send the message out as a product */
      ret = rms_build_text_msg(temp_string);

      if (ret != 0){
         LE_send_msg (RMS_LE_ERROR, "Unable to send free text message");
         return (-1);
      }else {

         if (sent_to & APUP_MESSAGE) {
            LE_send_msg (RMS_LE_LOG_MSG, "Message sent to APUP\n");
         } /* End if */

         if (sent_to & PUES_MESSAGE) {
            LE_send_msg (RMS_LE_LOG_MSG, "Message sent to PUES\n");
         } /* End if */

      }/* End else */
   }/* End if */

   return (0);

} /*End rms rec free text command msg */

/**************************************************************************
   Description:  This function builds the text message and sends it to
   pserver to be distributed to the correct users.

   Input: free_text_buf - Pointer to the message buffer.
           parameters - Parameters for p server to use.

   Output: Free text message to users.

   Returns: 0 = Successful send message.

   Notes:

   **************************************************************************/

static int rms_build_text_msg(char *string) {

   int        vol_num, length, status;
   short      *ptr;
   int        prod_id = FTXTMSG;
   static int initialized = 0;
   int        argc = 2;
   char       *argv[2] = { "rms_interface","-v" };

   /* One-time initialization.*/
   if( !initialized ){
      RPGC_out_data_wevent( FTXTMSG, ORPGEVT_FREE_TXT_MSG );
      RPGC_task_init( VOLUME_BASED, argc, argv );
      initialized = 1;
   }/* End if */

   /* Get current volume scan number. */
   vol_num = RPGC_get_current_vol_num();

   /*Acquire output buffer for Free Text Message.  Return -1 on failure.*/
   ptr = (short *) RPGC_get_outbuf( FTXTMSG, 5000, &status );

   if( ptr == NULL ){
      LE_send_msg(RMS_LE_ERROR, "NULL pointer returned from RPGC_get_outbuf\n" );
      return (-1);
   }/* End if */

   /* Fill in the product description block.  Return -1 on failure.*/
   status = RPGC_prod_desc_block( (void *) ptr, prod_id, vol_num );

   if( status < 0 ){

      LE_send_msg(RMS_LE_ERROR, "RPGC_prod_desc_block Failed\n" );
      return (-1);
   }/* End if */

   /* Set the product dependent parameters in the product description block.*/
   RPGC_set_dep_params ( (void *) ptr, params );

   /* Build the standalone alphanumeric data.  Return -1 on failure. */
   status = RPGC_stand_alone_prod( (void *) ptr, string, &length );
   if( status < 0 ){
      LE_send_msg(RMS_LE_ERROR, "RPGC_stand_alone_prod Failed\n" );
      return (-1);
   }/* End if */

   /* Build product header.  Return -1 on failure.  */
   status = RPGC_prod_hdr( (void*) ptr, prod_id, &length );
   if( status < 0 ){
      LE_send_msg(RMS_LE_ERROR, "RPGC_prod_hdr Failed\n" );
      return (-1);
   }/* End if */

   /* Release the output buffer. */
   RPGC_rel_outbuf( ptr, FORWARD );

   /* Post an event so p_server will be notified of the 
      new message to be sent.*/
   EN_post (ORPGEVT_FREE_TXT_MSG, NULL, 0, 0);

   return (0);
} /* End rms build text message */

/**************************************************************************
   Description:  This function checks the user profiles to find narrowband users that
   fit the class. It then sets the bits for those users.

   Input: class - Type of narrowband user to recieve the message.

   Output: Parameter bits set.

   Returns: 0 = Successful send message.

   Notes:

   **************************************************************************/

static int rms_set_users (int class) {

   int temp_param, temp_bit;
   int ret, i;
   int wb_link;
   int line_ind;

   /* Get the user profiles for the narrowband lines*/
   ret = rms_read_user_info();

   if (ret !=0){
      LE_send_msg(RMS_LE_ERROR, "Unable to get user profiles (ret %d)", ret );
      return (ret);
   }

   /* Determine which line is the wideband line */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* Check for the correct user type and set the bits in the parameters */
   for (i=0; i <=Num_users; i++){

      if( (i != wb_link) && (User_class[i] == class) && (User_type[i] == UP_LINE_USER) ){

         line_ind = User_line_index[i];
         temp_param = line_ind/16;
         temp_bit = line_ind%16;
         params[temp_param] |= (1<< temp_bit);
      }/*End if */
   }/* End loop */
   return (0);

}/*End set users */
