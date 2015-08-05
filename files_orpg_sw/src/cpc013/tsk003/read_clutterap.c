/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 17:09:32 $
 * $Id: read_clutterap.c,v 1.4 2008/01/04 17:09:32 aamirn Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*****************************************************************************
   read_clutterap() reads header info. and Clutter/AP from REC algorithm that
                    are needed by EPRE algorithm.
*****************************************************************************/

/* Global and Local include ------------------------------------------------ */
#include <stdio.h>
#include <recclalg_arrays.h>
#include <recclalg_adapt.h>
#include <basedata.h>
#include "epreConstants.h"
#include <infr.h>

void read_clutterap(char *inbuf, int *volnum, int *elindx,
                    int *spotblk, short ap[MAX_RADIALS][MAX_RNG], int *status)
{
  Rec_prod_header_t *phdr=NULL; /* Pointer to an intermediate product header  */
  Rec_rad_header_t  *rhdr=NULL; /* Pointer to an intermediate product radial  */
  int   offset = 0,             /* Holds calculated length of product         */
        i, j, k;                /* Loop variables                             */
  short *radialAP;              /* Array of pointers to access inbuf          */
  int debugit = FALSE;            /* Controls debug output in this file         */
  int num_radials = 0;          /* Variable holds number of radials           */

  if ( debugit )
   fprintf( stderr, "\n==============> PRODUCT 298 REC  ..............\n");

/* Allocate memory to hold current intermediate product ***/

   phdr = (Rec_prod_header_t *)inbuf;
   rhdr = (Rec_rad_header_t *)(inbuf + sizeof(Rec_prod_header_t));

/* Get REC header information to support EPRE algorithm */

   *volnum       = (int)phdr->volume_scan_num;
   *elindx       = (int)phdr->rpg_elev_ind;
   if( SPOT_BLANK_VOLUME & (int)rhdr->spot_blank_flag )
      *spotblk   = 1;
   else
      *spotblk   = 0;

   num_radials   = (int)phdr->num_radials;

/* Calculate the offset into the input buffer where the AP/clutter data begin */
   offset =  sizeof(Rec_prod_header_t) + sizeof(Rec_rad_header_t)*MAX_RAD;

   if ( num_radials >= MAX_RADIALS ) {
      fprintf( stderr, "Prod 298 radials=%d\n",phdr->num_radials);
      *status = 0;
      return;
    }

/* Load AP/Clutter values into radialAP container (array) from the calculated
   offset position assigned by pointer radialAP. ***/

   radialAP = (short *)( inbuf+offset );
   k = 0;

   for( i=0; i<num_radials; i++ ){
     for( j=0; j<MAX_RNG; j++ ) {
         ap[i][j]=(short)radialAP[k];
         k++;
      } /* end of the inner loop for each bin */
    } /* end of the outer loop for each radial */

/* print out some variable information on ap/clutter */
   if ( debugit ) {
      fprintf( stderr,"===> Product 298 REC ..........\n");
      fprintf( stderr, "volumeNumber=%d\n",*volnum);
      fprintf( stderr, "Elev_Index = %d Spot_Blank = %d\n",
               *elindx,*spotblk);
      fprintf(stderr," .............. PASSED PRODUCT 298 REC <==========\n");
    }

/* AP/Clutter data is  successfully read; the status is returned */
   *status = 1;

}
