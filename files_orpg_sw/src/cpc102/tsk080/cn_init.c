/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 15:01:27 $
 * $Id: cn_init.c,v 1.1 2014/05/13 15:01:27 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 *
 */  


/* cldm_init.c - This file contains the primary initialization routines 
                 for convert_ldm */


#include "change_nyquist.h"
#include <string.h>


/********************************************************************************

    Description: Initialize misc RPG parameters used by convert_ldm

          Input: 

         Output: *site_id         - The site ICAO/ID

         Return: 0 on success; -1 on error
   
 ********************************************************************************/

int INIT_RPG_parameters (char *site_id) {
   int  ret;
   char *ptr;

      /* Get site ICAO */

   if ((ret = DEAU_get_string_values( "site_info.rpg_name", &ptr)) < 0) {
      LE_send_msg (GL_ERROR, 
                   "Failure getting site_info.rpg_name (%d)", ret);
      return (-1);
   }

   strncpy (site_id, ptr, 4);
   site_id[4] = '\0';

   return (0);
}
