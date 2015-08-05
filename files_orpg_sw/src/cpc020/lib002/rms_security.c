/**************************************************************************
   
   Module:  rms_security.c   
   
   Description:  This module verifies passwords and locks control for rms.
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:33 $
 * $Id: rms_security.c,v 1.7 2003/06/26 14:51:33 garyg Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <rms_message.h>
#include <infr.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define MAX_PATHNAME_SIZE   128


/*
* Static Globals
*/

static int stat_lbfd;  /* Id of the RMS status LB */


/*
* Static Function Prototypes
*/

static int get_lbfd();

/**************************************************************************
   Description:  This function verifies the password.
      
   Input: Password.
      
   Output: None

   Returns: Security level

   Notes:

   **************************************************************************/
int rms_validate_password (char* password) {

   /* Compare the input password with the three hardcoded paswords.  
      The passwords will be hardcoded until the FAA includes an ICD
      message that will allow the ORPG RMS to change them */
   if (!strncmp (password, RMS_PASSWORD1, strlen(RMS_PASSWORD1)) || 
       !strncmp (password, RMS_PASSWORD2, strlen(RMS_PASSWORD2)) ||
       !strncmp (password, RMS_PASSWORD3, strlen(RMS_PASSWORD3)) )
      /* Good password */
      return SECURITY_LEVEL_RMS;
    else /* Bad Password */
       return SECURITY_LEVEL_ZERO;

} /*End rms validate password */

/**************************************************************************
   Description:  This function locks the rda and rpg commands.

   Input: Command = RDA or ORPG lock, State = lock or unlock

   Output: Lock or unlock the rda and orpg command buffers

   Returns: 0 = Successful command

   Notes:

   **************************************************************************/

int rms_rda_rpg_lock(int command, int state){

   int        ret;
   Rms_status *status_ptr;     /* Pointer to the RMS status structure */
   char       status_buf[256]; /* Character buffer to use for reading 
                                  the RMS status buffer */

   /* Check to see if the RMS status buffer id is defined.  If not get the id */
   if(stat_lbfd == 0)
      stat_lbfd = get_lbfd();

   /* If RMS unable to get status buffer id then print error message and return */
   if (stat_lbfd < 0){
      LE_send_msg(RMS_LE_ERROR, "Failed to find status buffer (%d).\n", stat_lbfd);
      return (stat_lbfd);
   } /* End if */

   /* Read the RMS status buffer */
   ret = LB_read(stat_lbfd, &status_buf, sizeof(status_buf), RMS_STATUS);

   /* If unable to read the RMS status buffer print error message and return */
   if ( ret < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to to read rms status msg (%d).\n", ret );
      return (ret);
   } /* End if */

   /* Cast the character buffer to the RMS status structure pointer */
   status_ptr = (Rms_status*) status_buf;

   /* Determine if the lock command is for the RDA or RPG */
   if (command == RMS_RPG_LOCK){
      /* Determine if the command is to lock or unlock the RPG */
      if(state == RMS_SET_LOCK){
         status_ptr->rms_rpg_locked_hci = 1;
      } /* End if */
      else if (state == RMS_CLEAR_LOCK){
         status_ptr->rms_rpg_locked_hci = 0;
      } /* End  else if */
   }/* End if */
   else if (command == RMS_RDA_LOCK){
      /* Determine if the command is to lock or unlock the RDA */
      if(state == RMS_SET_LOCK){
         status_ptr->rms_rda_locked_hci = 1;
      } /* End if */
      else if (state == RMS_CLEAR_LOCK){
         status_ptr->rms_rda_locked_hci = 0;
      } /* End else if */
   }/* End if */

   /* Write the new status to the LB */
   ret = LB_write(stat_lbfd, status_buf, sizeof(status_buf), RMS_STATUS);

   /* If the write fails print error message and return */
   if ( ret < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to to write rms status msg (%d).\n", ret );
      return (ret);
   } /* End if */

   return (1);

 } /* End rms rda rpg lock */

 /**************************************************************************
   Description:  This function gets the rda or rpg lock status.

   Input: Command = RDA or ORPG lock

   Output: None

   Returns: Status of the RDA or ORPG lock

   Notes:

   **************************************************************************/
int rms_get_lock_status( int command){

   int        ret;
   Rms_status *status_ptr;     /* Pointer to the RMS status structure */
   char       status_buf[256]; /* Character buffer to use for reading 
                                  the RMS status buffer */

   /* Check to see if the RMS status buffer id is defined.  If not get the id */
   if(stat_lbfd == 0)
      stat_lbfd = get_lbfd();

   /* If RMS unable to get status buffer id then print error message and return */
   if (stat_lbfd < 0){
      LE_send_msg(RMS_LE_ERROR, "Failed to find status buffer (%d).\n", stat_lbfd);
      return (stat_lbfd);
   }/* End if */

   /* Read the RMS status buffer */
   ret = LB_read(stat_lbfd, &status_buf, sizeof(status_buf), RMS_STATUS);

   /* If unable to read the RMS status buffer print error message and return */
   if ( ret < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to to read rms status msg (%d).\n", ret );
      return (ret);
   }/* End if */

   /* Caste the character buffer to the RMS status structure pointer */
   status_ptr = (Rms_status*) status_buf;

   /* Determine if the lock command is for the RDA or RPG */
   if (command == RMS_RPG_LOCK){
      /* Determine if the RPG commands are locked or unlocked and return the status */
      if(status_ptr->rms_rpg_locked_hci == 1){
         return (RMS_COMMAND_LOCKED);
      }/* End if */
      else if (status_ptr->rms_rpg_locked_hci == 0){
         return (RMS_COMMAND_UNLOCKED);
      }/* End else if */
   }/* End if */
   else if (command == RMS_RDA_LOCK){
      /* Determine if the RDA commands are locked or unlocked and return the status */
      if(status_ptr->rms_rda_locked_hci == 1){
         return (RMS_COMMAND_LOCKED);
      }/* End if */
      else if (status_ptr->rms_rda_locked_hci == 0){
         return (RMS_COMMAND_UNLOCKED);
      }/* End else if */
   }/* End if */

   return (-1);

}


 /**************************************************************************
   Description:  This function gets the LB id of the rms status buffer.

   Input: None

   Output: None

   Returns: LB id of the rms status buffer

   Notes:

   **************************************************************************/
static int get_lbfd(){

   int  ret, status_lbfd;
   char status_name[MAX_PATHNAME_SIZE];  /* Name string */

   /* Get the path for the misc LB directory */
   ret = MISC_get_work_dir (status_name, MAX_PATHNAME_SIZE - 16);

   if (ret < 0) {
      LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed in util (ret %d)", ret);
      return (-1);
   } /* End if */

   /* Add the name for the status LB */
   strcat (status_name, "/rms_status.lb");

   /* Get the id for the RMS status LB */
   status_lbfd = LB_open ( status_name, LB_WRITE, NULL);

   /* Unable to get LB id print error message */
   if (status_lbfd <0){
      LE_send_msg(RMS_LE_ERROR, 
          "Unable to open RMS status linear buffer (%d).\n", status_lbfd );
   }/* End if */

   /* Return the LB id */
   return (status_lbfd);

}
