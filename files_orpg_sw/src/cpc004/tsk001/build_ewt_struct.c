/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/09/15 12:50:54 $
 * $Id: build_ewt_struct.c,v 1.7 2008/09/15 12:50:54 cmn Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/**************************************************************************

   build_ewt_struct.c

   PURPOSE:

   This routine sets up access to the structure containing the ewt data.

   CALLED FROM:

   mpda_buf_cntrl.c
   
   INPUTS:

   None

   OUTPUTS:

   None.

   CALLS:
   
   None.

   RETURNS:

   None.

   NOTES:

   This module currently obtains the EWT data from the ITC data stores and
   copies it to a global variable for MPDA.  This copying allows MPDA to modify
   the values so that it can fill in any missing levels.

   HISTORY:

   R. May, 2/03         - changed to use native orpg ewt structure
   D. Zittel, 10/01     - adapted for orpg
   B. Conway, 10/00     - cleanup
   B. Conway, 5/96      - original development

****************************************************************************/

#include <memory.h>
#include <itc.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"


#include <veldeal.h>

void build_ewt_struct(){

   int i;

/* Get the EWT from the ITC data stores */

   if( !A304dg.sounding_avail || !use_sounding){
     sounding_for_use = FALSE;
     return;
     }

/* Fill in any missing levels in the EWT table with the value below the
   missing level */

     for(i=0;i<LEN_EWTAB;i++)
     {
       if(Ewt.newndtab[i][ECOMP] == MTTABLE)
         {
         if(i==0)
           Ewt.newndtab[0][ECOMP] = Ewt.newndtab[0][NCOMP] = 0;
         else
           {
           Ewt.newndtab[i][ECOMP] = Ewt.newndtab[i - 1][ECOMP];
           Ewt.newndtab[i][NCOMP] = Ewt.newndtab[i - 1][NCOMP];
           }
         }
     }

     sounding_for_use = TRUE;

}

