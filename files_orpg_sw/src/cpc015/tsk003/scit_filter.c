/*
 * RCS info
 * $Author: ryans $
 * $Date: 2005/02/23 22:30:32 $
 * $Locker:  $
 * $Id: scit_filter.c,v 1.1 2005/02/23 22:30:32 ryans Exp $
 * $revision$
 * $state$
 * $Logs$
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         scit_filter.c                                         *
 *      Author:         Yukuan Song                                           *
 *      Created:        Janurary 28, 2005                                     *
 *      References:     NSSL SCIT Fortran/C++ Source code                     *
 *                                                                            *
 *      Description:    This module does filtering on the input refl data     *
 *  			by using a kenel which adjusts for different ranges   *
 *									      *
 *      Input: 		unfiltered reflectivity data			      * 			
 *      Output:		filtered reflectivity data			      *
 *                                                                            *
 *      notes:          none                                                  *
 ******************************************************************************/
/* system includes ---------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* local include */
#include "scit_filter.h"

#define DEBUG	0

/* declare the local function */
short quick_select(short arr[], int n);

void scit_filter(int num_bin, int num_rad, short refl[MAXRADS][MAXBINS], 
		 short refl_ff[MAXRADS][MAXBINS], float beamwidth,
		 int gatewidth, float fraction_required, 
                 float kernel_size)
{
        /* declare a static variable array to store information of 
         * kernel size in radials. The array will be filled out the first time
         * the function scit_filter() is called. */
        static int kernel_size_in_radials[MAXBINS];
        static int firstTime =1; /* A on/off kind of flag */

	/* Local variables */
	int i, j, ii, iii, jj;

	int ctr; /* index of the array kernel_points[] */
	short *kernel_points; /* Array containing valid samples within a kernel */
        int num_missing; /* number of the missing in a kernel */
	int num_samples; /* number of samples within a kernel */

	int kernel_size_in_bins; /* Kernel size in bins */
	int startazmnum, endazmnum; /* starting and ending 
			   	       azimuth numbers     */

        if(DEBUG) {
	 fprintf(stderr, "firstTime=%d\n", firstTime);
	 fprintf(stderr, "num_rad=%d, num_bin=%d\n", num_rad, num_bin);
	 fprintf(stderr, "beamwidth=%f, gatewidth=%d\n", beamwidth, gatewidth);
	 fprintf(stderr, "Starting scit_filter function \n");
	 fprintf(stderr, "fraction_required in filter=%f\n", fraction_required);
         fprintf(stderr, "kernel_size=%f\n", kernel_size);
        }

        /* make unit convertion for "kernel_size" */
        kernel_size *= THOUSAND; /* convert KM to M */

	/* Calculate the starting and ending azimuth numbers */
	startazmnum = 0;
	endazmnum = num_rad - 1; 

	/* Convert kernel_size in km to bins */
	kernel_size_in_bins = (int) ((float)(kernel_size)/(float)(gatewidth));
	if (kernel_size_in_bins % 2 == 0)
	  kernel_size_in_bins++; /* Must be odd number */

	if(DEBUG) fprintf(stderr, "kernel_size_in_bin=%d, gatewidth=%d\n", 
			kernel_size_in_bins, gatewidth);

	/* Convert Kernel Size in KM to Radials */
        /* loop from kernel_size_in_bins/2 to 
         * num_bin - kernel_size_in_bins/2 */
        if(firstTime == 1) {
         /* set the firstTime to be 0 so that this block won't be
          * accessed anymore  */
         firstTime = 0;

         for ( i = (int)(kernel_size_in_bins/2); 
                i < (int)(MAXBINS - kernel_size_in_bins/2);
                  i++){
         kernel_size_in_radials[i] = 
			(int) (kernel_size / (((float)i*gatewidth)*
                              tan(beamwidth*PI/PI_DEGREE)));

         if ( kernel_size_in_radials[i] < 1)
                kernel_size_in_radials[i] = 1;
         if ( kernel_size_in_radials[i] > num_rad)
                kernel_size_in_radials[i] = num_rad;
         if ( kernel_size_in_radials[i] % 2 == 0)
                kernel_size_in_radials[i] +=1;
         if(DEBUG)
          fprintf(stderr,"i=%d, kernel_size_in_radials=%d\n",
                                  i,  kernel_size_in_radials[i]);
         } /* END of for ( i = (int)(kernel_size_in_bins/2); */
   
         /* Assign values to points at near range to radar */
         for (i = 0; i < (int)(kernel_size_in_bins/2);i++){
	  kernel_size_in_radials[i] = 
		kernel_size_in_radials[(int)(kernel_size_in_bins/2)];
          if(DEBUG)
           fprintf(stderr,"i=%d, kernel_size_in_radials=%d\n",
				i,  kernel_size_in_radials[i]);
         }

         /* Assign values to points at far range edge of data */
         for(i = (int)(MAXBINS - kernel_size_in_bins/2); i < MAXBINS; i++) {
	  kernel_size_in_radials[i] = 
            kernel_size_in_radials[(int)(MAXBINS - kernel_size_in_bins/2)-1];
          if(DEBUG) 
           fprintf(stderr,"i=%d, kernel_size_in_radials=%d\n",
                i,  kernel_size_in_radials[i]);
         } 
        }/* END of if(firstTime == 1) */

	/* Call the scale filter with 
         * range-dependent kernel */
	for ( j = 0; j < num_bin; j++) {

         /* allocate memory for kernel_points[] */
         num_samples = kernel_size_in_bins * kernel_size_in_radials[j];
         kernel_points = (short *)calloc(num_samples, sizeof(short));

         if(DEBUG) 
	  fprintf(stderr, "num_samples=%d\n", num_samples);

	 for ( i=startazmnum; i<=endazmnum; i++) {

          /* Initialize variables */
	  ctr = 0;
	  num_missing = 0;
 
	  for (ii = (i-(int)(kernel_size_in_radials[j]/2)); 
		ii<=(i+(int)(kernel_size_in_radials[j]/2)); ii++) {
	   if(ii<0) { /* Check for 360-degree crossing */
            iii = endazmnum + ii +1; 
           }
           else
           if (ii > endazmnum) {
	    iii = ii - endazmnum -1;
           }
           else {
            iii = ii;
           }

           for ( jj=(j-(int)(kernel_size_in_bins/2)); 
			jj<=(j+(int)(kernel_size_in_bins/2)); jj++) {

	    if (jj < 0 || jj > (num_bin -1)) continue;

	    /* collect valid data into an array */
	    if (refl[iii][jj] == MISSING ||
	        refl[iii][jj] == RANGEFOLDED){
             num_missing++;
             }
            else {
	     kernel_points[ctr++] = refl[iii][jj];
	    }
	   } /* END of for ( jj=(j-(kernel_size_in_bins[ ... */ 
	  } /* END of for (ii = (i-(kernel_size_i ... */

	  if(DEBUG)
	   fprintf(stderr, "num_misssing=%d\n", num_missing);

	  /* We need at least fraction_required of the points to do filter */
	  if ((float)num_missing/((float)ctr+(float)num_missing) >
	   (1 - fraction_required)) {
	   refl_ff[i][j] = FILTER_MISSING;
	   }
           else { /* identify the middle value */
            refl_ff[i][j]=quick_select(kernel_points, ctr);
	  }
         } /* END of for ( i=(int)startazmnum; i,=(int)endazmnum; i++)  */  
	 /* free memory allocated to kernel_points[] */
         free(kernel_points);
	} /* END of for ( j = 0; j < ((int)num_bin-(kernel */

	 if(DEBUG)fprintf(stderr, "Filter process has been done\n");
	
	return;
}

