/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 12:52:48 $
 * $Id: get_ewt_value.c,v 1.3 2006/09/08 12:52:48 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**************************************************************************
 
   get_ewt_value.c
 
   PURPOSE:
 
   This routine gets an EWT value that corresponds to the gate in question.

   CALLED FROM:

   first_triplet_attempt
   second_triplet_attempt
   pairs_and_trips_attempts
   final_unf_attempts
   replace_orig_vals
 
   INPUTS:
 
   float  rng    - range to the gate (km)
   int    azindx - index value to the radial within the trig tables 

   CALLS:

   None.

   OUTPUTS:
 
   None.
 
   RETURNS:

   EWT value or MISSING if none found
 
   NOTES:
 
   HISTORY:

   R. May,    02/03     - cleanup, change to use ORPG native structure
   B. Conway, 10/00     - cleanup
   B. Conway, 5/95      - original development
 
****************************************************************************/

#include <a309.h>
#include <basedata.h>
#include <itc.h>
#include "mpda_constants.h"
#include "mpda_trig_arrays.h"


/*  Following constant defined as follows: 1/(2*1.21*6371*1000) m^-1 */
#define INV_TWOEARTHRADIUS  0.00000006486

Base_data_header rpg_hdr;
struct trig_arrays radscan;
extern A3cd97 Ewt;  /* Global copy of the EWT for MPDA use */

float get_ewt_value(float rng, int azindx){

   float bin_hgt;
   int   guess;

/* 
   Find the bin height in Kfeet 
*/

   bin_hgt = M_TO_KFT * rng * (rpg_hdr.sin_ele + rng * INV_TWOEARTHRADIUS);

/* 
   Find the table entry closest to the bin height 
*/
/*   if( bin_hgt <= 0.0 )
      guess = 0; */

   if( bin_hgt >= (float) LEN_EWTAB )
      guess = LEN_EWTAB - 1;

   else
      guess = (int) (bin_hgt + 0.5);

/*
   If table entry is missing, return MISSING_F.  Otherwise compute radial
   component from ECOMP and NCOMP.
*/
   if(Ewt.newndtab[guess][ECOMP] == MTTABLE) return (MISSING_F);

   return ( (Ewt.newndtab[guess][ECOMP] * radscan.sinazim[azindx] + 
             Ewt.newndtab[guess][NCOMP] * radscan.cosazim[azindx] )
             * rpg_hdr.cos_ele );

}  
