
/**************************************************************************

   Module:  rms_nb_status.c

   Description:  This is the module for getting narrowband status messages.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:25 $
 * $Id: rms_nb_status.c,v 1.15 2003/06/26 14:51:25 garyg Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <prod_status.h>
#include <rms_message.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/


/*
* Static Globals
*/

int          Num_users = 0;                   /* Number of narrowband users */
char         *User_info;                      /* Pointer to user information */
int          User_size;                       /* Size of user profile */
int          User_line_index [USER_TBL_SIZE]; /* Array of user indexes */
int          User_type [USER_TBL_SIZE];       /* Array of user types */
int          User_class [USER_TBL_SIZE];      /* Array of user classes */
int          User_info_ptr [USER_TBL_SIZE];   /* Array of user pointers */
int          User_id [USER_TBL_SIZE];         /* Array of user ids */
int          User_distri [USER_TBL_SIZE];     /* Array of user distribution methods */
short        User_max_time [USER_TBL_SIZE];   /* Array of user timeouts */
short        User_pms_list [USER_TBL_SIZE];   /* Array of user pms lists */
unsigned int User_defined [USER_TBL_SIZE];    /* Array of user distribution defines */
char         User_name [USER_TBL_SIZE][USER_NAME_LEN];    /* Array of user names */
char         User_password [USER_TBL_SIZE][PASSWORD_LEN]; /* Array of user passwords */


/*
* Static Function Prototypes
*/


/*************************************************************************
   Description:  This routine reads the narrowband user information and
   places it into an array of structures.

   Input: nb_ptr - A pointer to an array of Nb_status structures.

   Output:  Places the narrowband status in the nb_ptr array.

   Returns: n_lines - The number of narrowband lines in use.

   Notes:

   **************************************************************************/
int rms_get_nb_status(Nb_status *nb_ptr){

   Prod_user_status rms_nb_stat[US_STATUS_SIZE];  /* Product user status array */
   LB_id_t          line_index;
   Nb_status        *temp_ptr;  /* Temporary status pointer */
   int              ret, i, n_lines;
   int              wb_link;

   /* Get number of narrwoband lines */
   n_lines = ORPGNBC_n_lines();

   /* Determine which line is the wideband line */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* FAA serves only the first 32 lines.  If more are returned ignore them. */
   if (n_lines > MAX_FAA_LINES)
      n_lines = MAX_FAA_LINES;

   /* Set pointer to nb status structure */
   temp_ptr = nb_ptr;

   /* Zero out the array */
   for (i=0; i<= MAX_FAA_LINES; i++){
      temp_ptr->line_num = 0;
      temp_ptr->nb_status = 0;
      temp_ptr->nb_utilization = 0;
      temp_ptr++;
   } /* End loop */

   line_index = 0;

   for (i=1; i<=n_lines; i++){
      /* Determine if narrowband line selected is the wideband line */
      if ( line_index != wb_link){
         /* Read each line and extract the information */
         ret = ORPGDA_read (ORPGDAT_PROD_USER_STATUS, (char*)rms_nb_stat,
               US_STATUS_SIZE * sizeof(Prod_user_status), line_index);

         if (ret <=0 && ret != -56){
            LE_send_msg(RMS_LE_ERROR, 
                "Unable To Read ORPGDAT_PROD_USER_STATUS. %d Line index %d\n",
                ret, line_index );
         } /* End if */

         nb_ptr->line_num = (line_index + 1);

         /* Set up the information in the narrowband array for the status message */
         /* If line is enabled determine status */
         if (rms_nb_stat[0].enable == US_ENABLED ) {
            /* If line is connected determine status */
            if (rms_nb_stat[0].link == US_CONNECTED) {
               if(rms_nb_stat[0].line_stat == US_LINE_NORMAL){
                  nb_ptr->nb_status = 5;
               } /* End if */
               else if(rms_nb_stat[0].line_stat == US_LINE_NOISY){
                  nb_ptr->nb_status = 8;
               }/* End else if */

               if (rms_nb_stat[0].line_stat == US_LINE_FAILED )
                  nb_ptr->nb_status = 7;
            } /* End if */

            /* Line is connect pending */
            else if(rms_nb_stat[0].link == US_CONNECT_PENDING){
               nb_ptr->nb_status = 1;
            } /* End else if */

            /* If line is enabled and disconnected determine if solicited */
            if(rms_nb_stat[0].link == US_DISCONNECTED){
               /* Solicited disconnect */
               if(rms_nb_stat[0].discon_reason == US_SOLICITED){
                  nb_ptr->nb_status = 3;
               }/* End if */

               else{
                  /* Unsolicited disconnect */
                  nb_ptr->nb_status = 4;
               } /* End else */
            } /* End if */

            /* Line enables and failed not disconnected */
            else if(rms_nb_stat[0].line_stat == US_LINE_FAILED){
               nb_ptr->nb_status = 7;
            } /* End if */
         }/*End line enabled */

         /* If line is disabled determine status */
         if (rms_nb_stat[0].enable == US_DISABLED ) {

            /* If line is disabled and disconnected determine if solicited */
            if(rms_nb_stat[0].link == US_DISCONNECTED){
               /* Solicited disconnect */
               if(rms_nb_stat[0].discon_reason == US_SOLICITED){
                  nb_ptr->nb_status = 3;
               } /* End if */
               else{
                  /* Unsolicited disconnect */
                  nb_ptr->nb_status = 4;
               } /* End else */
            } /* End if */

            /* Line disabled and failed not disconnected */
            else if(rms_nb_stat[0].line_stat == US_LINE_FAILED){
               nb_ptr->nb_status = 7;
            } /* End else if */
         }/*End if */

         /* Narowband utilization as a percentage */
         nb_ptr->nb_utilization = rms_nb_stat[0].util;

         /* User id */
         nb_ptr->user_id = rms_nb_stat[0].uid;

         nb_ptr++;
      }/* End if */
      else {
         /* Wideband line selected zero out entry */
         nb_ptr->nb_status = 0;
         nb_ptr->nb_utilization = 0;
         nb_ptr->user_id = 0;
         nb_ptr++;
      } /* End else */

      line_index++;
   } /* End for loop */

   return(n_lines);

} /* End rms get nb status */

/**************************************************************************
   Description:  This routine reads the narrowband user information and
   gets the user id for that line.

   Input: nb_ptr - A pointer to an array of Nb_status structures.

   Output:

   Returns: usr_id - Id of this user.

   Notes:

   **************************************************************************/

short rms_get_user_id(int line_num){

   Prod_user_status rms_nb_stat[US_STATUS_SIZE];  /* Product user status array */
   LB_id_t          line_index;
   short            uid;
   int              ret;

   /* Set line index to input line number */
   line_index = (LB_id_t)line_num;

   /* Read line status of input line */
   ret = ORPGDA_read (ORPGDAT_PROD_USER_STATUS, (char*)rms_nb_stat,
            US_STATUS_SIZE * sizeof(Prod_user_status), line_index);

   /* If return is less than zero and not equal to LB_NOT_FOUND print 
      error and return */
   if (ret <=0 && ret != -56){
      LE_send_msg(RMS_LE_ERROR, 
          "Unable To Read ORPGDAT_PROD_USER_STATUS. %d Line index %d\n",
          ret, line_index );
      return (-1);
   } /* End if */

   /* Set user id and return it */
   uid = rms_nb_stat[0].uid;

   return (uid);
}

/**************************************************************************
   Description:  This routine reads the narrowband user information and
   gets the line index for that line.

   Input: nb_ptr - A pointer to an array of Nb_status structures.

   Output:

   Returns: usr_id - Line index of this user.

   Notes:

   **************************************************************************/

short rms_get_line_index(int line_num){

   Pd_line_entry  *p_line_entry;  /* Product user entry line entry pointer */
   Pd_distri_info *p_tbl;         /* Product distribution information pointer */

   /* Get the product distribution information */
   p_tbl = ORPGGST_get_prod_distribution_info();

   /* If pointer equal NULL return */
   if (p_tbl == NULL)
      return (-1);

   /* Set pointer to beginning of line list */
   p_line_entry = (Pd_line_entry*) (p_tbl + 1);

   /* Set pointer to line entry */
   p_line_entry = (p_line_entry + line_num);

   /* Release product distribution table memory */
   free(p_tbl);

   /* Return the line index */
   return (p_line_entry->line_ind);

}

/*******************************************************************
 Description:  This routine get the user profile information and stores it
                in an array for use by the function caller.

   Input: None

   Output: An array of user profile information.

   Returns: 0 = Successful read.

   Notes:
******************************************************************/
int rms_read_user_info (){

   int           ret;
   int           i;
   int           offset;
   Pd_user_entry *user_info;  /* Product user information entry */
   LB_info       lb_info; /* LB information structure */

   Num_users = 0;

   /* Initialize the user profile arrays */
   for (i=0;i<USER_TBL_SIZE;i++) {
      User_info_ptr [i] = -1;
      User_class [i] = -1;
      User_type [i] = -1;
      User_line_index [i] = -1;
   } /* End loop */

   /* Get the size of the user info message so we can allocate 
      enough memory for it. */
   ret = ORPGDA_info (ORPGDAT_PROD_INFO, PD_USER_INFO_MSG_ID, &lb_info);

   /* If unable to get LB info print error message and return */
   if (ret != LB_SUCCESS) {
      LE_send_msg (RMS_LE_ERROR,
         "ORPGDA_info (ORPGDAT_PROD_INFO) failed: %d\n",ret);
      return (-1);
   } /* End if */

   /* Set size to the LB info size */
   User_size = lb_info.size;

   /* Since we have good info, allocate memory to store it */
   User_info = (char *) calloc (lb_info.size, 1);

   if (User_info == NULL) {
      LE_send_msg (RMS_LE_ERROR,
          "calloc for User_info failed (size %d\n", lb_info.size);
      return (-1);
   } /* End if */

   /* Now read the user profile data. */
   ret = ORPGDA_read (ORPGDAT_PROD_INFO, (char *) User_info,
                 lb_info.size, PD_USER_INFO_MSG_ID);

   /* If error reading LB print error message and return */
   if (ret < 0) {
      if (ret != LB_BUF_TOO_SMALL) {
         LE_send_msg (RMS_LE_ERROR,
             "ERROR: reading PD_USER_INFO_MSG_ID: %d\n", ret);
         return (-1);
      } /* End if */
   } /* End if */
   else {
      /* If LB read was successful then place user profile data in 
         array for each user */
      offset = 0;
      Num_users = 0;

      while (offset < ret) {

         /* Put offset of this user entry in array */
         User_info_ptr [Num_users] = offset;

         /* Point to user entry */
         user_info  = (Pd_user_entry *) (User_info + offset);

         /* Put user class in array */
         User_class [Num_users] = user_info->class;

         /* Put user type in array */
         User_type [Num_users] = user_info->up_type;

         /* Put user line index in array */
         User_line_index [Num_users] = user_info->line_ind;

         /* Put user id in array */
         User_id [Num_users] = user_info->user_id;

         /* Put user distribution method in array */
         User_distri [Num_users] = user_info->distri_method;

         /* Put user connect time in array */
         User_max_time [Num_users] = user_info->max_connect_time;

         /* Put user defined in array */
         User_defined [Num_users] = user_info->defined;

         /* Put user pms list in array */
         User_pms_list[Num_users] = user_info->pms_list;

         /* Put user name in array */
         memcpy (&User_name [Num_users][0], user_info->user_name, USER_NAME_LEN);

         /* Put user password in array */
         memcpy (&User_password [Num_users][0], user_info->user_password, PASSWORD_LEN);

          /* Increment number of users */
          Num_users++;

          /* Increment the offset by the size of this user entry */
          offset = offset + user_info->entry_size;

      } /* End loop */
   } /* End else */

   return (0);

} /* End rms read user info */
