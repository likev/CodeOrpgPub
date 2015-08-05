/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 16:34:57 $
 * $Id: compute_area.c,v 1.2 2008/01/04 16:34:57 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        compute_area.c

Description:   Finds the area of the penultimate threshold in color tables and 
		returns it.
               
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
	08/04/2003	Initial implementation	David Zittel
	11/04/2004	SW CCR NA04-30810	Build8 changes and name changed
						from usp_comp_area to compute_area 
						and added to the saa library, saalib.
               
*******************************************************************************/

#include <stdio.h>
#include "saaprods_main.h"
#include "build_saa_color_tables.h"

int compute_area(short array[], int max_radials, int max_bins, int prod_id){

/*  find the maximum value and return it */

  register int i; 
  int azm, rng, accum_thresh;
  int debugit = FALSE;

  float PI = 3.14159;

  float arc_length;

  float bin_size = 0.0;
  float area_factor = 0.0;
  float fbin_range = 0.0;
  float twice_binsize = 0.0;
  float area_const = 0.0;
  float area;

  area = 0.0;
  /*  Case structure added for Build8 and to accomodate SSW/SSD products  */
  switch (prod_id)
  {
  case USWACCUM:
     {
     accum_thresh = LWEQV_THRES;
     break;
     }
  case USDACCUM:
     {
     accum_thresh = LDPTH_THRES;
     break;
     }
  case SSWACCUM:
     {
     accum_thresh = HWEQV_THRES;
     break;
     }
  case SSDACCUM:
     {
	 /* Scale the HDPTH_THRES by 10 to make the units compatible  */
     accum_thresh = HDPTH_THRES * 10;
     break;
     }
  default:
     return (0);
  }

  arc_length = PI/max_radials;
  bin_size = 1;  /*basedata.surv_bin_size/1000; */
  twice_binsize = 2.0 * bin_size;
  fbin_range = 0; /*basedata.surv_range * bin_size; */
  area_factor = arc_length * bin_size;
  area_const = 2.0 * fbin_range - bin_size;

  for(i = 0; i < max_radials*max_bins; ++i)
     if(array[i] > accum_thresh){
 	azm = i/max_bins;
	rng = i - (azm*max_bins);
	if(debugit){fprintf(stderr,"azm = %d, rng = %d, value = %d\n",azm,rng,array[i]);}
        area += area_factor * ((float)rng * twice_binsize - area_const);
        }
  if(debugit){fprintf(stderr,"Area = %f, prod_id = %d, accum_thresh = %d\n",
                     area,prod_id,accum_thresh);}

  return (int)area;
}          
     

