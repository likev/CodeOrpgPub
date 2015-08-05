/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:20:00 $
 * $Id: prcprate_lfm4_map.c,v 1.2 2006/02/09 18:20:00 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_lfm4_map.c

   Description
   ===========
      This function maps rate scan bins onto 1/4 lfm grid, performing 
   area-weighted averaging of bins that map onto a grid box.

   Change History
   =============
   08/29/88      0000      Greg Umstead         spr # 80390
   04/10/90      0001      Dave Hozlock         spr # 90697
   02/22/91      0002      Paul Jendrowski      spr # 91254
   02/15/91      0002      John Dephilip        spr # 91762
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      Toolset              spr 91895
   10/05/92      0006      Joseph/Wheeler       ccr na90-93082
   10/13/92      0006      Bradley Sutker       ccr na92-28001
   01/05/93      0007      PJ Yelshin           ccr na93-00401
   01/26/93      0007      Mary Lou Eckerle     ccr na93-02002
   03/25/93      0008      Toolset              spr na93-06801
   01/28/94      0009      Toolset              spr na94-01101
   03/03/94      0010      Toolset              spr na94-05501
   04/11/96      0011      Toolset              ccr na95-11802
   12/23/96      0012      Toolset              ccr na95-11807
   06/10/97      0013      Melanie Taylor       ccr na94-23504
   03/16/99      0014      Toolset              ccr na98-23803
   12/20/04      0015      Cham Pham            ccr NA05-01303
   10/26/05      0016      Cham Pham            ccr NA05-21401
****************************************************************************/
/* Global include files */
#include <rpgc.h>
#include <prcprtac_main.h>
#include <a313hbuf.h>
#include <a313h.h>
#include <a3146.h>

/* Local include file */
#include "prcprtac_Constants.h"

void lfm4_map( )
{
 double numer[HYZ_LFM4*HYZ_LFM4],
       denom[HYZ_LFM4*HYZ_LFM4];
 int   i, j;

/* If tables in image are based on site lat and lon ... do process*/
  if ((a314c1.grid_lat==sadpt.rda_lat)&&(a314c1.grid_lon==sadpt.rda_lon)) 
  {

/* Initialize numerator and denominator to zero, and flag boxes
   as beyond range...
 */
   memset( numer, 0, HYZ_LFM4*HYZ_LFM4*sizeof(double) );
   memset( denom, 0, HYZ_LFM4*HYZ_LFM4*sizeof(double) );

/* if precipitation exists...*/
    if ( a313hgen.flag_zero == FLAG_CLEAR ) 
    {

/* Do for each azimuth...*/
      for ( j=0; j<MAX_AZMTHS; j++ ) 
      {

/* Do until the end of the radial...*/
        for ( i=0; i<MAX_RABINS; i++ ) 
        {

/* Check for missing value*/
          if ( RateScan[j][i] == FLG_MISSNG ) 
          {
           /* Set missing bin to zero*/
            RateScan[j][i]=0;
          } 
          else 
          {
/* Note C conversion:  All arrays considers with which lfm4grid uses at each 
   grid point should be subtract to 1 because C array starts with zero. 
 */
/* Check for the lfm grid within bounds*/
            if ( a314c1.lfm4grid[j][i] != BEYOND_GRID ) 
            {
/* Increment numerator by product of bin area and precip rate...*/
              numer[a314c1.lfm4grid[j][i]-1] = numer[a314c1.lfm4grid[j][i]-1]+
                                           a313hlfm.bin_area[i]*RateScan[j][i];

/* Increment denominator by bin area...*/
              denom[a314c1.lfm4grid[j][i]-1] =
                          denom[a314c1.lfm4grid[j][i]-1]+a313hlfm.bin_area[i];
            }
          }

        }/* end loop MAX_RABINS*/

      }/* end loop MAX_AZMTHS*/

/* Debug writes for numerator and denominator...do for each lfm bin...*/ 
      a314c1.max82val = 0;

      for ( i=0; i<HYZ_LFM4*HYZ_LFM4; i++ ) 
      {

/* If this box is within range ... go ahead and calculate*/
        if ( a314c1.lfm4flag[FLAG_RNG][i] == WITHIN_RANGE ) 
        {

/* Calculate value for numerator...*/
          if ( denom[i] != 0. ) 
          {
/* Note: Changed for LINUX - Used RPGC_NINT library function instead of 
         adding 0.5 for rounding to the nearest integer.
 */
            
            lfm4Grd[i] = RPGC_NINT(numer[i]/denom[i]);

/* Capture the maximum value of lfm4Grd */
            if ( lfm4Grd[i] > a314c1.max82val )
              a314c1.max82val = (lfm4Grd[i]-1)*MM_TO_IN;
          }
        }

/* Beyond range */
        else if ( a314c1.lfm4flag[FLAG_RNG][i] == BEYOND_RANGE )
        {
           lfm4Grd[i] = BEYOND_RANGE;
        }
/* A hole ... use nearest bin data for hole*/
        else
        {
           lfm4Grd[i]=
            RateScan[a314c1.lfm4flag[FLAG_AZ][i]][a314c1.lfm4flag[FLAG_RNG][i]];
        }

      }/* End loop HYZ_LFM4*HYZ_LFM4 */

    }
/* If precip. does not exist*/
    else 
    {
/* Do for each lfm bin... */
      for ( i=0; i<HYZ_LFM4*HYZ_LFM4; i++ ) 
      {

/* Set lfm grid to zero */
        if ( a314c1.lfm4flag[FLAG_RNG][i] == BEYOND_RANGE )
        {
           lfm4Grd[i] = BEYOND_RANGE;
        }
        else
        {
           lfm4Grd[i] = 0;
        }

      }/* End for loop HYZ_LFM4*HYZ_LFM4 */

    }/* end a313hgen.flag_zero==FLAG_CLEAR*/

  }/* table is bad... set output to beyond range */
  else 
  {
    for ( i=0; i<HYZ_LFM4*HYZ_LFM4; i++ )
    {
      lfm4Grd[i] = BEYOND_RANGE;
    }
  }
}
