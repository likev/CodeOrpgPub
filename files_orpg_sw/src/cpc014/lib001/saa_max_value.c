/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:25:08 $
 * $Id: saa_max_value.c,v 1.1 2004/01/21 20:25:08 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saa_max_value.c

Description:   Finds the largest positive value in an integer array and returns it.
               
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 09/04/2003 - Zittel
               
*******************************************************************************/

#include <stdio.h>
#include "saa_arrays.h"

int saa_max_value(short array[], int* azm, int* rng){

/*  find the maximum value and return it */

  int max_value = 0;
  register int i;
  int isave;
  int DEBUG = 0;
  
  isave = 0;

  for(i = 0; i < MAX_SAA_RADIALS*MAX_SAA_BINS; ++i)
       if(array[i] > max_value){
          max_value = array[i];
          isave = i;
       }
  *azm = isave/MAX_SAA_BINS;
  *rng = isave - (*azm * MAX_SAA_BINS); 
  if(DEBUG){
     fprintf(stderr,"Maximum Accumulation = %d\n",max_value);
     fprintf(stderr,"Isave = %d, Azimuth = %d, Range = %d\n",isave,*azm,*rng);
  }
  return max_value;
}          
     

