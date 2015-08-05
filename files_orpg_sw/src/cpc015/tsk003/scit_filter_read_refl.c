/*
 * RCS info
 * $Author: steves $
 * $Date: 2006/09/18 16:37:30 $
 * $Locker:  $
 * $Id: scit_filter_read_refl.c,v 1.2 2006/09/18 16:37:30 steves Exp $
 * $revision$
 * $state$
 * $Logs$
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         scit_filter_read_refl.c                               *
 *      Author:         Yukuan Song                                           *
 *      Created:        Janurary 28, 2005                                     *
 *      References:     NSSL SCIT Fortran/C++ Source code                     *
 *                                                                            *
 *      Description:    This module reads the elevation-based reflectivity    * 
 *                      data, and then put them in a     		      *
 *                      two dimentional array. The function scit_filter()     *
 *                      is called to filter the data			      *
 *                                                                            *
 *      Input:          elevation_based reflectivity data                     *
 *      Output:         filtered reflectivity                                 *
 *                                                                            *
 *      notes:          none                                                  *
 ******************************************************************************/
#ifdef SUNOS
#define scit_filter_read_refl scit_filter_read_refl_ 
#endif
#ifdef LINUX
#define scit_filter_read_refl scit_filter_read_refl__
#endif


/* Global and local include files ------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <rpg_globals.h>
#include <basedata_elev.h>
#include <rpgc.h>

#define HEAD_SIZE       13
#define DEBUG           0
#define BYTE_MASK       255
#define MAXBINS         460
#define MAXRADS         400

/* Declare local function */
void scit_filter(int num_bin, int num_rad, short refl[MAXRADS][MAXBINS],
                 short refl_ff[MAXRADS][MAXBINS], float beamwidth,
                 int gatewidth, float, float);

int scit_filter_read_refl(void *inbuf, Compact_basedata_elev *out_buf, 
                short zrefl_f[MAXRADS][MAXBINS], short zrefl[MAXRADS][MAXBINS],
                float header[HEAD_SIZE][MAXRADS],
		int *read_status, int *outlb_status, int *filter_status, 
        	float *fraction_required, float *kernel_size)
{

   Compact_basedata_elev *bde = NULL; /* Pointer to an intermediate product */
   int bin_num, max_bin_num,  gatewidth;
   float beamwidth, sum_beam;

   int i, j;                  /* Loop variables                        */
   int radnum, offset;

   /* initialize max_bin_num to be zero */
   max_bin_num =0;
   *read_status = 1;

   if (DEBUG) fprintf( stderr, "\nInside read_Refl function ...\n" );

   /* Allocate memory to hold current intermediate product */
   bde = (Compact_basedata_elev *)inbuf;

   /* Test to see if the required moment (reflectivity) is enabled */
   if (!(bde->type & REF_ENABLED_BIT)) {
      *read_status = -1;
      return 0;
   }

   /* Make sure the reflectivity radials are NOT(!) mapped to the
      Doppler radials on this cut */

   if ((bde->type & BYTE_MASK) & REF_INSERT_BIT) {
      *read_status = -1;
      return 0;
   } 

   /* Fill out the head field of the output linear buffer */
   if ( *outlb_status) {
      out_buf->num_radials = bde->num_radials;
      out_buf->type = bde->type;
      out_buf->elev_ind = bde->elev_ind;
   }


   /* Get the number of radial, Gatewidth */
   radnum = (int)bde->num_radials;
   gatewidth = (int)bde->radial[0].bdh.surv_bin_size;

   /* Check the radial number in this elevation */
   if( radnum >= MAX_RADIALS_ELEV ) {
      *read_status = -1;
       return 0;
   }

   /* Initialize beamwidth, sum_beam */
   beamwidth = 0;
   sum_beam =0;

   for( i=0; i<radnum; i++ ){

      /* Record some of the head fields to be use */
      header[0][i] = (float)bde->radial[i].bdh.status;
      header[1][i] = (float)bde->radial[i].bdh.time;
      header[2][i] = (float)bde->radial[i].bdh.azimuth;
      header[3][i] = (float)bde->radial[i].bdh.azi_num;
      header[4][i] = (float)bde->radial[i].bdh.elevation;
      header[5][i] = (float)bde->radial[i].bdh.surv_bin_size;
      header[6][i] = (float)bde->radial[i].bdh.surv_range;
      header[7][i] = (float)bde->radial[i].bdh.n_surv_bins;
      header[8][i] = (float)bde->radial[i].bdh.delta_angle;
      header[9][i] = (float)bde->radial[i].bdh.sin_azi;
      header[10][i] = (float)bde->radial[i].bdh.cos_azi;
      header[11][i] = (float)bde->radial[i].bdh.rpg_elev_ind;
      header[12][i] = (float)bde->radial[i].bdh.volume_scan_num;

      sum_beam += (float) ((bde->radial[i].bdh.delta_angle)/10.0);

      /* get the number of bins for this radial */
      bin_num = (int)bde->radial[i].bdh.n_surv_bins;
      if ( bin_num > max_bin_num)
         max_bin_num = bin_num;

      /* Save reflectivity data in the zrefl[][] */
      offset = bde->radial[i].bdh.ref_offset;
      for (j=0; j < bin_num; j++ ) {
         zrefl[i][j]=(short)bde->radial[i].radar_data[offset+j];

      }/*END of for(j=0; j < bin_num; j++ ) */

   }/* END of or( i=0; i<radnum; i++ ) */

   /* Calculate the beamwidth */
   beamwidth = sum_beam / radnum;


   /* call scit_filter() */
   if(*filter_status) {
    scit_filter(max_bin_num, radnum, zrefl, zrefl_f, beamwidth, gatewidth,
                             *fraction_required, *kernel_size);
   }

   /* Save the filtered refl in the output linear buffer */
   if ( *outlb_status) {

      for( i=0; i<radnum; i++ ) {

         out_buf->radial[i].bdh = bde->radial[i].bdh;

         offset = bde->radial[i].bdh.ref_offset;
         for( j=0; j<max_bin_num; j++ ) {
            out_buf->radial[i].radar_data[offset+j] = (char)zrefl_f[i][j];
         }

         /* Assign zero values to bins beyond bin bin_num */
         for (j=max_bin_num; j < MAXBINS; j++) {
            out_buf->radial[i].radar_data[offset+j] = 0;
         } /* END of for (j=bin_num; j < MAXBINS; j++) */

      }

   } /* END of if ( outlb_status)  */

  return 0;
} 
