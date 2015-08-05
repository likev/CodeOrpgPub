/******************************************************************************
 *      Module:         mda3d_msi.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        January 10, 2003                                      *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This routine calculates MSI for 3D feature            *
 *                                                                            *
 *      notes:          none                                                  *
 ******************************************************************************/
/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 20:11:03 $
 * $Id: mda3d_msi.c,v 1.8 2014/05/13 20:11:03 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include<math.h>
#include<time.h>
#include<string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mda3d_parameter.h"
#include "mda3d_acl.h"
#include "mda3d_attributes.h"

#include "siteadp.h"

#define 	DEBUG 0 /* boolean variable */
#define 	BIN_SIZE 0.25 /* bin size */
#define 	HALF_BIN_SIZE 0.125 /* half of the bin size */
#define		HEIGHT_SIXTEEN_KM 16.0 /* height threshold of 16 */
#define		HEIGHT_ELEVEN_KM 11.0 /* height threshold of 11 */
#define 	ELEVEN	11 /* Just a number of 11 */
#define 	FOURTYFIVE	45 /* Just a number of 45 */
#define         FOOT2METER      0.3048 /*convert foot to meter */
#define         THOUSAND        1000.0 /* just anumber of 1000.0*/

/*declare global variables */
	extern float meso_vd_thr[BASEDATA_DOP_SIZE][99];
        extern float meso_shr_thr[BASEDATA_DOP_SIZE][99];
	
	extern cplt_t mda_sf[MESO_MAX_FEAT][MESO_NUM_ELEVS];
        extern cplt_t cplt[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
        extern int mda_num_sf[MESO_NUM_ELEVS];
        extern int current_length; /* current_length of array mda_sf[][] */
        extern int nbr_cplt;
        extern mda_th_xs_t mda_th_xs[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
        extern int ncf[MESO_MAX_NCPLT];

        extern int num_elev;
        extern float elev_val[MESO_NUM_ELEVS];

        extern int MDA_2D_MAX_RANK;

	extern Siteadp_adpt_t site_adapt;


void mda3d_msi(int num_elev, float elev_val[], int i, int n, new_cplt_t new_cplt[])
 {

	/* declare local variables */
	int j, k; 
	int el_idx = 0;
	int bin_index;
	float feat_ht, cr, elev_avg, thickness;
	float msi, int_rotv, int_shr, int_gtg;
	float prev_ht, prev_rank;
	float prev_rotv, prev_shr, prev_gtg;
	float bot_ht = 0.0; 
	float air_den, adj_den, avg_den;
	float distance, gradient, value;
	float next_ht; 
	float top_ht = 0.0; 
	float integ_depth;

	/* initialize */
	msi = 0.0;
	int_rotv = 0.0;
	int_shr = 0.0;
	int_gtg = 0.0;

        if(DEBUG) fprintf(stderr, "\n**In MSI...elev=%f\n",elev_val[num_elev-1]);
	/* loop through all 2D features in the 3D couplet */
	for (j = 0; j < ncf[i]; j++)
         {
	  feat_ht = fabs(cplt[i][j].ht);
          cr = cplt[i][j].cr;
	  elev_avg = 0.0;
   if(DEBUG) fprintf(stderr, "feature#=%d, ncf=%d, ca=%f, cr=%f\n",i,ncf[i],cplt[i][j].ca,cr);

	  for (k = 0; k < MESO_NUM_ELEVS; k++)
           {
	    if ( cplt[i][j].elevation == elev_val[k])
	    {
   if(DEBUG) fprintf(stderr, "el_idx=%d\n",k);
             el_idx = k;
             break;
            }
           } 

	  /* Start integration at the lowest 2D feature in the couplet */
	  /* Only integrate the portion of the 3D feature that is      */
	  /* below MESO_MAX_DEPTH.                                     */

	  if ( j == 0)
           {
	    if (cplt[i][j].elevation == elev_val[0] )
             {

             /* This logic assumes that if the feature is detected on the  */
             /* lowest elevation angle then it extends to the ground.      */

              if ( feat_ht  <= MESO_MAX_DEPTH)
               thickness = feat_ht;
              else
               thickness = MESO_MAX_DEPTH;
              prev_ht = 0.0;
	     }
            else
             {
             
              /* This is an elevated feature.                              */
              /* Assume the base is really halfway between this elevation  */
              /* and the one below it.                                     */
              
	      elev_avg = (elev_val[el_idx - 1] + elev_val[el_idx]) / 2.0;
	      prev_ht = cr * sin(elev_avg * DTR) + (cr * cr) / (2.0 * IR * RE);
              if (prev_ht > MESO_MAX_DEPTH)
               {
                thickness = 0.0;
                prev_ht = MESO_MAX_DEPTH;        /**  Not in NSSL code **/
               }
              else if (feat_ht > MESO_MAX_DEPTH)
                thickness = MESO_MAX_DEPTH - prev_ht;
              else
                thickness = feat_ht - prev_ht;
	     }
	    prev_rank = 0.0;
	    prev_rotv = 0.0;
	    prev_shr = 0.0;
	    prev_gtg = 0.0;
	    bot_ht = prev_ht;
           }
          else /* Continue integration with the next 2D feature up in the couplet */
           {
	    prev_rank =cplt[i][j-1].rank;
	    prev_rotv =cplt[i][j-1].rot_vel;
	    prev_shr =cplt[i][j-1].shr;
	    prev_gtg =cplt[i][j-1].gtgmax;
	    prev_ht = fabs(cplt[i][j-1].ht);
	    if (prev_ht > MESO_MAX_DEPTH)
	     thickness = 0.0;
            else if (feat_ht > MESO_MAX_DEPTH)
             thickness = MESO_MAX_DEPTH - prev_ht;
            else
             thickness = feat_ht - prev_ht;
	   }

          /* Weight rank values with various air density adjustments */
	  air_den = mda3d_density(feat_ht);
	  adj_den = mda3d_density(prev_ht);
	  avg_den = (air_den + adj_den) / 2.0;
	  distance = feat_ht - prev_ht;

	  if( distance != 0.0)
           {
	    gradient = (cplt[i][j].rank - prev_rank) / distance;
	   }
          else
           {
	    gradient = 0.0;
	   }

	  value = prev_rank + gradient * thickness / 2.0;
          msi = msi + thickness * value * avg_den;
   if(DEBUG) fprintf(stderr, "msi1=%f\n",msi);
	  /* Integrate information using trapezoid rule with information from
	   * lower elevation scans */
	  if ( distance != 0.0) 
           {
	    gradient = (cplt[i][j].rot_vel - prev_rotv) / distance;
            value = prev_rotv + gradient * thickness / 2.0;
	    int_rotv = int_rotv + thickness * value;
	    
	    gradient = (cplt[i][j].shr - prev_shr ) / distance;
            value = prev_shr + gradient * thickness / 2.0;
	    int_shr = int_shr + thickness * value;

	    gradient = (cplt[i][j].gtgmax - prev_gtg) / distance;
            value = prev_gtg + gradient * thickness / 2.0;
	    int_gtg = int_gtg + thickness * value;
	   }
          else
           {
	    int_rotv = 0.0;
	    int_shr = 0.0;
	    int_gtg = 0.0;
           }

	  /* Finish the integration at the top of 3D couplet */
	  if ( j == (ncf[i] -1) )
           {
            if (DEBUG) fprintf(stderr, "At top of 3D couplet\n");
            if ( feat_ht < MESO_MAX_DEPTH )
             {
              if (DEBUG) {
               fprintf(stderr, "j=%d, feat_ht=%f num_elev=%d, el_idx=%d, ncf=%d\n",j, feat_ht,num_elev, el_idx,ncf[i]);
              }
/*** The commented out code incorrectly compares a zero relative loop index  **/
/*** (j) with non-zero-relative counters (num_elev and MESO_NUM_ELEVS).      **/
/*** Regardless, testing showed that the first two if checks were not needed.**/
/*** The lone check for a zero value in the next elev_val array was enough   **/
/*** to produce the same result.                                             **/
/****	      if ( j < num_elev && el_idx < MESO_NUM_ELEVS &&              ****/
/****	           elev_val[el_idx + 1] != 0.0)                            ****/
              if (elev_val[el_idx + 1] != 0.0)
               {
                /* Here, we are at the top of the 3D feature but not at */
                /* the highest elevation of the VCP.                    */
	        elev_avg = (elev_val[el_idx] + elev_val[el_idx +1] ) / 2.0;
		next_ht = cr * sin(elev_avg * DTR) + (cr * cr) / (2.0 * IR * RE);

                if ( next_ht < MESO_MAX_DEPTH)
                 thickness = next_ht - feat_ht;
                else
                 {
                  thickness = MESO_MAX_DEPTH - feat_ht;
                  next_ht = MESO_MAX_DEPTH;           /** Not in NSSL code **/
                 }
               }
              else 
               {
                /* Here, we are at the top of the 3D feature but do not */
                /* know the next angle in the VCP.  Set the next_ht to  */
                /* the height of this 2D feature and go on.             */
                next_ht = feat_ht;                    /** Not in NSSL code **/
                thickness = 0.0;                      /** Not in NSSL code **/
/**		next_ht = MESO_MAX_DEPTH;**//**Removed. Assumes we are at 8km*/
    if (DEBUG) fprintf(stderr, "Assuming were are at or above 8 km\n");
                if ((feat_ht + thickness) > MESO_MAX_DEPTH)
                 thickness = MESO_MAX_DEPTH - feat_ht;
	       }
	      top_ht = MIN(next_ht, MESO_MAX_DEPTH);
    if (DEBUG) fprintf(stderr, "next_ht=%f, top_ht=%f, thickness=%f\n",next_ht,top_ht,thickness);

	      /* Weight rank values with various air density adjustmentd */
	      air_den = mda3d_density(feat_ht);
              adj_den = mda3d_density(next_ht);
	      avg_den = (air_den + adj_den) / 2.0;
              distance = next_ht - feat_ht;
              if (distance != 0.0)
               gradient = cplt[i][j].rank /distance;
              else
               gradient = 0.0;

	      value = gradient * thickness / 2.0;
              msi = msi + thickness * value * avg_den;
   if(DEBUG) fprintf(stderr, "msi2=%f\n",msi);

              /* Integrate information using trapezoid rule with information from
               * lower elevation scans */
	      if ( distance != 0.0)
               {
                gradient = cplt[i][j].rot_vel / distance;
                value = gradient * thickness / 2.0;
                int_rotv = int_rotv + thickness * value;

                gradient = cplt[i][j].shr / distance;
                value = gradient * thickness / 2.0;
                int_shr = int_shr + thickness * value;

                gradient = cplt[i][j].gtgmax / distance;
                value = gradient * thickness / 2.0;
                int_gtg = int_gtg + thickness * value;
               }
              else
               {
                int_rotv = 0.0;
                int_shr = 0.0;
                int_gtg = 0.0;
               }
              
	     }
            else
             {
	      top_ht = MESO_MAX_DEPTH;
           if (DEBUG) fprintf(stderr, "Defaulting top_ht\n");
	     }	    

           } /* END of if ( j == ncf[i] ) */
  if(DEBUG) fprintf(stderr,"top_ht = %f, bot_ht = %f\n",top_ht, bot_ht);
         } /* END of for (j = 0, j < ncf[i]; j++) */
	
 	/* Compute depth of integration */
	integ_depth = top_ht - bot_ht;

	/* Divide MSI values by depth of integration and assign to new_cplt array */
        if (integ_depth != 0.0)
         new_cplt[n].msi = msi / integ_depth;
        else
         new_cplt[n].msi = 0.0;
  if(DEBUG) fprintf(stderr,"integ_depth=%f, MSI=%f\n",integ_depth,new_cplt[n].msi );

	/* Divide other values by depth of integration and assign to new_cplt array */
 	if (integ_depth != 0.0)
         {
	  new_cplt[n].vert_integ_rot_vel = int_rotv / integ_depth;
	  new_cplt[n].vert_integ_shear = int_shr / integ_depth;
          new_cplt[n].vert_integ_gtg_vel_diff = int_gtg / integ_depth;
         }
	else
         {
	  new_cplt[n].vert_integ_rot_vel = 0.0;
          new_cplt[n].vert_integ_shear = 0.0;
          new_cplt[n].vert_integ_gtg_vel_diff = 0.0;
	 } 

	/* Assign rank to 3D circulation using integrated values ("MSI Rank") */
	new_cplt[n].msi_rank = 0;

        /* Find the bin index */
	bin_index = (int) ((cplt[i][0].cr - HALF_BIN_SIZE) / BIN_SIZE);

        for ( k = MESO_MIN_RANK; k <= MESO_MAX_RANK; k++)
         {
	  if ( ((new_cplt[n].vert_integ_rot_vel * 2.0 >= meso_vd_thr[bin_index][k]) &&
	       (new_cplt[n].vert_integ_shear >= meso_shr_thr[bin_index][k])) ||
	       (new_cplt[n].vert_integ_gtg_vel_diff >= meso_vd_thr[bin_index][k]) )
           {
	    new_cplt[n].msi_rank = k;
	   }
         } 

 }



float mda3d_density(float input_height)
{
	/* Declare the local variables */
	float attitude; /* Radar attitude */ 

        int i, level;
        int k = 0;
	float air_den;
        float cent_ht, thickness, height;
	float d1[45] = {1.159, 1.135, 1.112, 1.089, 1.066, 1.041, 1.016, 0.9920,
			0.9683, 0.9448, 0.9218, 0.8992, 0.8771, 0.8555, 0.8343,
			0.8135, 0.7931, 0.7730, 0.7533, 0.7340, 0.7151, 0.6966,
			0.6784, 0.6707, 0.6433, 0.6271, 0.6112, 0.5957, 0.5804,
			0.5654, 0.5506, 0.5362, 0.5442, 0.5082, 0.4946, 0.4812,
			0.4681, 0.4553, 0.4428, 0.4305, 0.4185, 0.4067, 0.3951,
			0.3838, 0.3728};
	float d2[11] = {0.3728, 0.3513, 0.3308, 0.3112, 0.2925, 0.2746, 0.2576,
			0.2413, 0.2258, 0.2076, 0.1909};

	attitude = site_adapt.rda_elev * FOOT2METER /THOUSAND;

	cent_ht = input_height + attitude;
        if (cent_ht < 0.0) {
          cent_ht = 0.0;
          if (DEBUG)
           fprintf(stderr, "The Cent height is below the sea level!\n");
        }

	if (cent_ht >= HEIGHT_SIXTEEN_KM)
         {
	  air_den = d2[10];
          return air_den;
	 }

	if ( cent_ht >= HEIGHT_ELEVEN_KM)
         {
	  thickness = 0.5;
	  level = ELEVEN;
	  for (i = 1; i <= level; i++) {
	   height = 11.0 + (float)i * thickness - thickness;
           if ( cent_ht < height){
            k = i -2;
            break; 
           }
	  } /*END of for (i = 1; i <= 11; i++)  */ 
         air_den = d2[k] + (d2[k] - d2[k+1]) * (cent_ht -height) / thickness;
	 }
        else
         {
	  thickness = 0.25;
	  level = FOURTYFIVE;
          for ( i = 1; i <= level; i++){
	   height = (float)i *thickness - thickness;
           if(cent_ht < height) {
	    k = i - 2;
            break; 
           }
          }
         /* calculate air density */
         air_den = d1[k] + (d1[k] - d1[k+1])*(cent_ht -height) / thickness;
	 }
   	
	return air_den;

	
}
