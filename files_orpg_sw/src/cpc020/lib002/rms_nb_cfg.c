/**************************************************************************

   Module:  rms_cfg_ptr.c

   Description:  This module retrives the information needed for the narrow-
   band reconfiguration commands.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:24 $
 * $Id: rms_nb_cfg.c,v 1.13 2003/06/26 14:51:24 garyg Exp $
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

#define PASSWORD_SIZE   4


/*
* Static Globals
*/

extern int   Num_users;                     /* Number of narrowband users */
extern char   *User_info;                   /* Pointer to user profiles */
extern int   User_type [USER_TBL_SIZE];     /* Array of user types */
extern int   User_class [USER_TBL_SIZE];    /* Array of user classes */
extern int   User_info_ptr [USER_TBL_SIZE]; /* Array of user pointers */
extern short User_max_time [USER_TBL_SIZE]; /* Array of user timeouts */
extern short User_pms_list [USER_TBL_SIZE]; /* Array of user pms lists */
extern int   User_line_index [USER_TBL_SIZE]; /* Array of user indexes */


/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: nb_inter_buf - Pointer to the message buffer.

   Output:

   Returns:

   Notes:

   **************************************************************************/

int rms_get_cfg_ptr (Nb_cfg* cfg_ptr,short line_num, short uid) {

   Pd_distri_info *p_tbl;        /* Pointer to product distribution table */
   Pd_line_entry  *p_line_entry; /* Pointer to line entry in product distribution table */
   Pd_pms_entry   *user_entry;   /* Pointer to user entry in product distribution table */
   int            i, num_lines, ret;
   int            user_num = 0;

   /* Get narrowband information */
   p_tbl = ORPGGST_get_prod_distribution_info();

   /* If the pointer is NULL print error message and return */
   if (p_tbl == NULL)
      return (-1);

   /* Set pointer to beginning of line list */
   p_line_entry = (Pd_line_entry*) (p_tbl + 1);

   /* Get the number of narrowband lines */
   num_lines = ORPGNBC_n_lines();

   /* Search for line to retrive configuration */
   for ( i = 0; i <= num_lines; i++) {
      /* Get each line entry */
      /* If the requested line number is the same as this entry 
         get baud rate and password */
      if ( p_line_entry->line_ind == (line_num)) {
         memcpy(cfg_ptr->password, p_line_entry->port_password, PASSWORD_SIZE);
         cfg_ptr->rate = p_line_entry->baud_rate;
         break;
      } /*end if */

      p_line_entry = (p_line_entry + 1);

      /* If the line is not found set up the pointer for error control */
      if (i == num_lines)
         p_line_entry = NULL;

   } /* end loop */

   /* If the line is not found print error message and return */
   if(p_line_entry == NULL){
      LE_send_msg(RMS_LE_ERROR,"Unable to find line (%d)",line_num);
      return (8);
   }

   /* Get user array information */
   ret = rms_read_user_info();

   if (ret < 0){
      LE_send_msg(RMS_LE_ERROR,"Unable to read user profiles");
      return (ret);
   }

   /* Check to see if user in the array */
   for ( i=0; i < Num_users; i++){
      /* If the index equals the line number set user number 
         and exit loop */
      if(User_line_index[i] == line_num){
         user_num = i;
         break;
      }
      else /* If user not found set user number for error control */
        user_num = -1;
   }

   /* If user found continue */
   if (user_num >= 0){
      /* Place user information into configuration structure */
      cfg_ptr->line_type = User_type[user_num];
      cfg_ptr->line_class = User_class[user_num];
      cfg_ptr->connect_time = User_max_time[user_num];

      /* Place pointer at distribution information for this user */
      user_entry = (Pd_pms_entry *) ((User_info + User_info_ptr[user_num]) 
                   + User_pms_list[user_num]);

      if (User_pms_list[user_num] == 0){
         /* If distribution information not available print error and return */
         LE_send_msg(RMS_LE_ERROR,
             "Unable to get distribution for %d", User_line_index[user_num]);
         cfg_ptr->distri_type = 0;
      }
      else {
         /* Put distribution information in configuration structure */
         cfg_ptr->distri_type = user_entry->types;
      }

      return (1);
   }
   else
      /* If user not found return error code */
      return (-1);
} /*End rms get nb cfg */
