/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 12:52:50 $
 * $Id: get_moments_status.c,v 1.6 2006/09/08 12:52:50 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/**************************************************************************

   get_moments_status.c

   PURPOSE:

   This gets the moments as set by the operator.  
      Bit one indicates status of reflectivity
      Bit two indicates status of velocity
      Bit three indicates status of spectrum width

   CALLED FROM:

   mpda_buf_cntrl.c

   INPUTS:

   None 

   CALLS:

   ORPGRDA_get_status

   OUTPUTS:

   None.

   RETURNS:

   *ref_stat, *vel_stat, *wid_stat - flags that represent
    the moments that are enabled

   HISTORY:

   W. Zittel   6/03      Original Development
               
               
*********************************************************************/

#include <rda_status.h>
#include <orpgrda.h>
#include <orpg.h>

void get_moments_status( int *ref_stat, int *vel_stat,
                         int *wid_stat )
{
   int moms_status;

   moms_status = ORPGRDA_get_status( RS_DATA_TRANS_ENABLED );
   if( moms_status == ORPGRDA_DATA_NOT_FOUND ){

      /* Assume all moments are enabled. */
      *ref_stat = *vel_stat = *wid_stat = 1;
      return;

   }

   *ref_stat = ((moms_status & BD_REFLECTIVITY ) > 0 ) ? 1 : 0;
   *vel_stat = ((moms_status & BD_VELOCITY ) > 0 ) ? 1 : 0;
   *wid_stat = ((moms_status & BD_WIDTH )> 0 ) ? 1 : 0;
   if ((*ref_stat & *vel_stat) == 0)
      LE_send_msg(GL_STATUS | LE_RPG_WARN_STATUS,"MPDA VCP But R or V Moment Disabled. Using VDA.\n");

   return;
}
