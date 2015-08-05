/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:47 $
 * $Id: prcprate_find_holes.c,v 1.1 2005/03/09 15:43:47 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_find_holes.c 

   Description
   ===========
    This function initializes polar to lfm conversion tables.

   Change History
   ============
   10/13/92      0000      bradley sutker      ccr# na92-28001
   03/25/93      0001      toolset              spr na93-06801
   01/28/94      0002      toolset              spr na94-01101
   03/03/94      0003      toolset              spr na94-05501
   04/11/96      0004      toolset              ccr na95-11802
   12/23/96      0005      toolset              ccr na95-11807
   03/16/99      0006      toolset              ccr na98-23803 
   11/25/04      0007      Cham Pham            CCR NA05-01303
*****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a3146.h>

/* Local include file */
#include "prcprtac_Constants.h"

#define AZ_RND 1.05         /* Factor used to convert azimuth angle to azimuth
                               index of bin closet to grid hole.             */
#define RG_RND 2.0          /* Factor used to convert range to range to range
                               bin index of closet to grid hole.             */
#define ONE    1            /* Constant representing the value 1             */

void find_holes( double ls, double lamdas )
{
double  az_ij=0.;           /* Azimuth angle to the center of an LFM box     */
double  rg_ij=0.;           /* Range to the center of an LFM box             */ 
int     i,                  /* Loop control for absolute LFM box number      */
        lfm_i,              /* LFM I coordinate (i increases to the right)   */ 
        lfm_j;              /* LFM J coordinate (j increases downward)       */

   if (DEBUG) {fprintf(stderr,"FIND_HOLES....\n");}

/* Search for holes in the 1/4 lfm lookup table and identify the
   az/ran of the data to be used to fill in the hole. 
 */
   for ( i=0; i<NUM_LFM4; i++ )
   {

/* Holes exist where lfm4flag[FLAG_RNG][i] contains the inital value*/

     if ( a314c1.lfm4flag[FLAG_RNG][i] == BEYOND_RANGE ) 
     {

/* Compute i/j coordinates of the hole*/
       lfm_j = (i/HYZ_LFM4+1)-1;
       lfm_i = i-lfm_j*HYZ_LFM4;

/* Compute the az/ran of the hole*/
       lfm_to_azran( ls, lamdas, lfm4_idx, lfm_i+1, lfm_j+1, &az_ij, &rg_ij );

/* If the range is within the product coverage area requirement then
   store azimuth and range index into the lfm4flag lookup table
 */
       if ( rg_ij <= LFM4_RNG ) 
       {
         a314c1.lfm4flag[FLAG_AZ][i] = (int)(az_ij+AZ_RND);
         a314c1.lfm4flag[FLAG_RNG][i]= (int)(rg_ij/RG_RND)+ONE;
       }

     }/* End if block flm4flag equals to BEYOND_RANGE */

   }/* End loop NUM_LFM4 */

/* Search for holes in the 1/16 lfm lookup table and identify the
   az/ran of the data to be used to fill in the hole 
 */
   for ( i=0; i<NUM_LFM16; i++ ) 
   {

/* Holes exist where lfm16flag[FLAG_RNG][i] contains the inital value*/
     if ( a314c1.lfm16flag[FLAG_RNG][i] == BEYOND_RANGE ) 
     {
/* Compute i/j coordinates of the hole*/
        lfm_j = (i/HYZ_LFM16+1)-1;  
        lfm_i = i-lfm_j*HYZ_LFM16; 

/* Compute the az/ran of the hole*/
        lfm_to_azran( ls, lamdas, lfm16_idx, lfm_i+1, lfm_j+1, &az_ij, &rg_ij );

/* If the range is within the product coverage area requirement then
   store azimuth and range index into the lfm16flag lookup table*/
       if ( rg_ij <= LFM16_RNG ) 
       {
         a314c1.lfm16flag[FLAG_AZ][i] = (int)(az_ij+AZ_RND);
         a314c1.lfm16flag[FLAG_RNG][i]= (int)rg_ij+ONE;
       }

     }/* End if block lfm16flag equals to BEYOND_RANGE */

   }/* End loop NUM_LFM16 */

/* Search for holes in the 1/40 lfm lookup table and identify the
   az/ran of the data to be used to fill in the hole 
 */
   for (i=0; i<NUM_LFM40; i++) 
   {

/* Holes exist where lfm40flag[FLAG_RNG][i] contains the inital value*/
     if ( a314c1.lfm40flag[FLAG_RNG][i] == BEYOND_RANGE ) 
     {

/* Compute i/j coordinates of the hole*/
       lfm_j = (i/HYZ_LFM40+1)-1;
       lfm_i = i-lfm_j*HYZ_LFM40;

/* Compute the az/ran of the hole*/
       lfm_to_azran( ls, lamdas, lfm40_idx, lfm_i+1, lfm_j+1, &az_ij, &rg_ij );

/* If the range is within the product coverage area requirement then
   store azimuth and range index into the lfm40flag lookup table
 */
       if ( rg_ij <= LFM40_RNG ) 
       {
         a314c1.lfm40flag[FLAG_AZ][i] =(int)(az_ij+AZ_RND);
         a314c1.lfm40flag[FLAG_RNG][i]=(int)(rg_ij/RG_RND)+ONE;
       }

     }/* End if block lfm40flag equals to BEYOND_RANGE */

   }/* End loop NUM_LFM40 */
}
