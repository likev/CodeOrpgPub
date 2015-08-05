/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 12:52:51 $
 * $Id: initialize_data_arrays.c,v 1.4 2006/09/08 12:52:51 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/**************************************************************************

   initialize_data_arrays.c

   PURPOSE:

   This routine initializes the data arrays and num_prfs.

   CALLED FROM:
 
   mpda_buf_cntrl.c

   INPUTS:

   None.

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   HISTORY:

   R. May,    02/03      - add save.az1 to list of arrays to initialize
   R. May,    11/02      - modified to use faster memset call
   B. Conway, 10/00      - original development

****************************************************************************/

#include <memory.h>
#include "mpda_constants.h"
#include "mpda_structs.h"


void initialize_data_arrys()
{
   memset(save.ref_buff, UIF_MISSING, sizeof(save.ref_buff));
   memset(save.az1, 0, sizeof(save.az1));
   memset(save.az_flag2, FALSE, sizeof(save.az_flag2));
   memset(save.az_flag3, FALSE, sizeof(save.az_flag3)); 
   memset(save.vel_limit, 0, sizeof(save.vel_limit));
   memset(save.vel1, MISSING_BYTE, sizeof(save.vel1));
   memset(save.final_vel, MISSING_BYTE, sizeof(save.final_vel));
   memset(save.vel2, MISSING_BYTE, sizeof(save.vel2));
   memset(save.vel3, MISSING_BYTE, sizeof(save.vel3));
   memset(save.sw1, UIF_MISSING, sizeof(save.sw1));
   memset(save.sw2, UIF_MISSING, sizeof(save.sw2));
   memset(save.sw3, UIF_MISSING, sizeof(save.sw3));
   memset(save.final_sw, UIF_MISSING, sizeof(save.final_sw));
   num_prfs = 0;
   memset(init_prf_arry, FALSE, sizeof(init_prf_arry));
}
