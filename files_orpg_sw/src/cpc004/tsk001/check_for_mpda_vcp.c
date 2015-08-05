/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 12:52:44 $
 * $Id: check_for_mpda_vcp.c,v 1.3 2006/09/08 12:52:44 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**************************************************************************

   check_for_mpda_VCP.c

   PURPOSE:

   This routine checks to see if the current VCP is an MPDA VCP.

   CALLED FROM:

   veldeal.c

   INPUTS:

   int vcp       - number of the current VCP
   int *mpda_flg - pointer to flag to indicate MPDA VCP

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   1 if the VCP is an MPDA number, 0 otherwise 

   NOTES:

   HISTORY:

   C. Calvert, 06/03     - determined mpda vcp flag by searching a range of vcps
   D. Zittel, 10/01     - adapted for orpg
   B. Conway, 9/00      - cleanup 
   B. Conway, 5/96      - original development

****************************************************************************/

#define GLOBAL_VCP_LIST

#include "mpda_constants.h"
#include "rdacnt.h"
#include "mpda_vcp_info.h"


void check_for_mpda_vcp( int vcp, int *mpda_flg ){

    *mpda_flg = 0;
    if( (vcp >= VCP_MIN_MPDA) && (vcp <= VCP_MAX_MPDA) ) 
        *mpda_flg = 1;

   return;

}
