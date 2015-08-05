/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 20:11:03 $
 * $Id: mda3d_attributes.c,v 1.14 2014/05/13 20:11:03 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda3d_attributes.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        January 10, 2003                                      *
 *      References:     WDSS MDA Fortran Source code                          *
 *			ORPG MDA AEL					      *
 *                                                                            *
 *      Description:    This routine calculates attributes for 3D feature     *
 *                                                                            *
 *	Input:		none						      *
 *									      *
 *	Output:		none						      *
 * 	Return:		none						      *
 * 	Global:		none						      *
 *	notes:								      *
 *                      03/14/2008: pass the parameter "MESO_MIN_DEPTH" in all*
 *                      4 mda3d_rank() calls. The change made to fix the false*
 *                      alarm problem. by Yukuan                              *
 ******************************************************************************/


#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mda3d_main.h"
#include "mda3d_acl.h"
#include "mda3d_parameter.h"
#include "mda3d_attributes.h"

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

/* Define symbolic names */
#define		TRUE		1	/* Boolean parameter */
#define		FALSE		0	/* Boolean parameter */
#define		DEBUG		0	/* Boolean parameter */
#define 	DTR     0.017453 /* factor to convert degree to radian */
#define 	VIRTUAL_STRM_DEPTH -999.0 /* initialize storm depth */
#define         PRINT_TO_FILE   0       /* 1: yes, print it to file */
#define		CENTRAL_HEIGHT  6.0   /* 6.0 km */
#define		DEG_CIR		360.0 /* degree of circle */
#define 	HEIGHT_INIT	999.0 /* initial value of height */
#define 	THOUSAND	1000.0 /* just a number of 1000 */
#define		HUNDRED		100	/* just a number of 100 */

/* declare global variables */
	extern cplt_t mda_sf[MESO_MAX_FEAT][MESO_NUM_ELEVS];
	extern cplt_t cplt[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
	extern int mda_num_sf[MESO_NUM_ELEVS];
	extern int current_length; /* current_length of array mda_sf[][] */
        extern int nbr_cplt;
        extern mda_th_xs_t mda_th_xs[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
        extern int ncf[MESO_MAX_NCPLT];

	extern int ncf[MESO_MAX_NCPLT];

	extern float mda_strm_depth;

	extern int MDA_2D_MAX_RANK;

void mda3d_attributes(int num_elev, float elev_val[], 
		int *nbr_new_cplts, new_cplt_t new_cplt[]) 
{
	/* declare the local variables */

	int i, j, k, n, core_rank; /* loop index */

	float rng, azm; /* range and azimuth */
	float sx, sy;
	float sum_ll_vd, sum_ml_vd; 
        int num_ll_vd, num_ml_vd;
	float core_base, core_top, core_depth; 
        int strength_rank=0;
        int sh_rank;
        float nssl_base, nssl_top, nssl_depth, rel_depth;
	float user_base, user_top, user_depth;
	float dist, avg_dia;

	int low_core_rank;
	
	int rank_set_flag;
	int SHALLOW; /* boolean variable */
  	
	FILE *fp3 =NULL;

        /* Loop for all 3D couplets */

	/* print out a header in this file */
        if (PRINT_TO_FILE) {
	fp3 = fopen("output3d", "a");
	fprintf(fp3, "BEGINING -------------------\n");
	fprintf(fp3, " ID    SR      class_type      NSSL_BASE       NSSL_DEPTH      LOWT\n");
        fprintf(fp3, "--- ---- ----- ---- ---- ----- ----- ----- ----- ----- ---- ---- ----\n");

	}
        if(DEBUG)fprintf(stderr,"\nENTERING mda3d_attributes. elev_val=%4.1f..\n",elev_val[num_elev-1]);
        if(DEBUG) {
          fprintf(stderr, "mda_adapt.beam_width=%f\n", mda_adapt.beam_width);
          fprintf(stderr, "mda_adapt.meso_ll_conv_ht=%d\n", mda_adapt.meso_ll_conv_ht);
          fprintf(stderr, "mda_adapt.meso_ml_conv_ht1=%d\n", mda_adapt.meso_ml_conv_ht1);
          fprintf(stderr, "mda_adapt.meso_ml_conv_ht2=%d\n", mda_adapt.meso_ml_conv_ht2);
          fprintf(stderr, "mda_adapt.mda_no_shallow=%d\n", mda_adapt.mda_no_shallow);
          fprintf(stderr, "mda_adapt.meso_min_depth_shal=%f\n", mda_adapt.meso_min_depth_shal);
          fprintf(stderr, "mda_adapt.meso_max_top_shal=%d\n", mda_adapt.meso_max_top_shal);
          fprintf(stderr, "mda_adapt.meso_min_rank_shal=%d\n", mda_adapt.meso_min_rank_shal);
          fprintf(stderr, "mda_adapt.overlap_filter_on=%d\n", mda_adapt.overlap_filter_on);
          fprintf(stderr, "mda_adapt.min_filter_rank=%d\n", mda_adapt.min_filter_rank);
        }

        for (i = 0; i < nbr_cplt; i++)
         {
	  n = *nbr_new_cplts; /* index for new cplt array */

	  /* calculate attributes for 3D couplets */ 

	  /* Check to see if the feature is topped */
	  if ((elev_val[num_elev-1] - cplt[i][ncf[i]-1].elevation) > 0.1) {
	   new_cplt[n].topped = 1;
	  }
	 else {
	  new_cplt[n].topped = 0;
	 }
         
	  /* Initialize 3D strength rank numbers */
          new_cplt[n].strength_rank = (int)cplt[i][0].rank;

	  /* Set 3D couplet ID */
          new_cplt[n].meso_id = i;

	  /* Calculate low-level centroid(x,y) using lowest-elevation 2D feature */
          new_cplt[n].ll_center_x = cplt[i][0].cx;
          new_cplt[n].ll_center_y = cplt[i][0].cy;
	  rng = sqrt(new_cplt[n].ll_center_x * new_cplt[n].ll_center_x +
                     new_cplt[n].ll_center_y * new_cplt[n].ll_center_y );
          azm = atan2(new_cplt[n].ll_center_x, new_cplt[n].ll_center_y) / DTR;
	  if (azm < 0.0)
            azm = DEG_CIR + azm;
	  new_cplt[n].ll_center_azm = azm;
          new_cplt[n].ll_center_rng = rng;

	  /* Set low-level elevation sweep number */
	  new_cplt[n].ll_elev_sweep = cplt[i][0].sweep_num;

          /* Calculate 3D average centroid (x, y) of all 2D features below 6km */
	  k = 0;
          sx = 0.0;
          sy = 0.0;

          for (j = 0; j< ncf[i]; j++)
           {
	    if (cplt[i][j].ht <= CENTRAL_HEIGHT)
             {
	       k +=1;
               sx += cplt[i][j].cx;
               sy += cplt[i][j].cy;
             }  
           }

	  if ( k > 0)
           {
	    sx /=(float)k;
	    sy /=(float)k;
           }
          else
           {
	    sx = cplt[i][0].cx;
	    sy = cplt[i][0].cy;
           }
	  
	  rng = sqrt(sx * sx + sy * sy);
	  azm = atan2(sx, sy) / DTR;
          if (azm < 0.0)
            azm = DEG_CIR + azm;

	  new_cplt[n].center_azm = azm;
	  new_cplt[n].center_rng = rng;

	  /* Calculate base, top, and depth of couplet */
	  new_cplt[n].base = cplt[i][0].ht;
	  new_cplt[n].top = fabs(cplt[i][ncf[i] - 1].ht);
	  new_cplt[n].depth = new_cplt[n].top - fabs(new_cplt[n].base) + 
				cplt[i][0].cr * sin(mda_adapt.beam_width * DTR);          


	  /* Calculate diameter, maximum shear, rotational velocity,
	   * and gate to gate velocity difference, and their heights (uses
           * only information below the MESO_MAX_MAG_HT level */
	  new_cplt[n].low_level_dia = cplt[i][0].dia;
	  new_cplt[n].max_level_dia = cplt[i][0].dia;
	  new_cplt[n].low_level_rot_vel = cplt[i][0].rot_vel;
	  new_cplt[n].max_level_rot_vel = cplt[i][0].rot_vel;
	  new_cplt[n].low_level_shear = cplt[i][0].shr;
	  new_cplt[n].max_level_shear = cplt[i][0].shr;
	  new_cplt[n].low_level_gtg_vel_diff = cplt[i][0].gtgmax;
	  new_cplt[n].max_level_gtg_vel_diff = cplt[i][0].gtgmax;
	  new_cplt[n].height_max_level_dia = fabs(cplt[i][0].ht);
	  new_cplt[n].height_max_level_rot_vel = fabs(cplt[i][0].ht);
	  new_cplt[n].height_max_level_shear = fabs(cplt[i][0].ht);
	  new_cplt[n].height_max_level_gtg_vel_diff = fabs(cplt[i][0].ht);

	  for ( j = 0; j < ncf[i]; j++)
           {
	    if ( cplt[i][j].ht <= MESO_MAX_MAG_HT)
             {
              if ( cplt[i][j].dia > new_cplt[n].max_level_dia )
               {
		new_cplt[n].max_level_dia = cplt[i][j].dia;
                new_cplt[n].height_max_level_dia = cplt[i][j].ht;
	       }

              if (cplt[i][j].rot_vel > new_cplt[n].max_level_rot_vel )
               {
                new_cplt[n].max_level_rot_vel = cplt[i][j].rot_vel;
                new_cplt[n].height_max_level_rot_vel = cplt[i][j].ht;
               }

              if (cplt[i][j].shr > new_cplt[n].max_level_shear )
               {
                new_cplt[n].max_level_shear = cplt[i][j].shr;
                new_cplt[n].height_max_level_shear = cplt[i][j].ht;
               }

              if ( cplt[i][j].gtgmax > new_cplt[n].max_level_gtg_vel_diff )
               {
                new_cplt[n].max_level_gtg_vel_diff = cplt[i][j].gtgmax;
                new_cplt[n].height_max_level_gtg_vel_diff = cplt[i][j].ht;
               }

             } /* END of if ( cplt[i][j].ht <= MESO_MAX_MAG_HT) */
	   } /* END of for ( j = 0; j < ncf[i]; j++) */

	  /* Calculate the slope of the delta-V (trends used for computing
           * ascending versus descending mesocyclones. */
	  if ((new_cplt[n].height_max_level_rot_vel - fabs(new_cplt[n].base)) != 0.0)
           new_cplt[n].delt_v_slope = THOUSAND * (new_cplt[n].max_level_rot_vel - 
	                                       new_cplt[n].low_level_rot_vel) /
                                               (new_cplt[n].height_max_level_rot_vel - 
                                               fabs(new_cplt[n].base));
          else
	   new_cplt[n].delt_v_slope = NX_MISSING_DATA;

	 /* Calculate Mesocyclone Strength Index (MSI):
          * (Density-weighted vertically-integrated strength rank) */
	 mda3d_msi(num_elev, elev_val, i, n, new_cplt); 

	 /* Compute low- and mid-level convergence from radial shear information */
	 sum_ll_vd = 0.0;
	 sum_ml_vd = 0.0;
	 num_ll_vd = 0;
	 num_ml_vd = 0;

	for (j = 0; j < ncf[i]; j++)
        {
	  if ( fabs(cplt[i][j].ht) <= mda_adapt.meso_ll_conv_ht)
          {
	    sum_ll_vd += cplt[i][j].avg_conv;
            num_ll_vd += 1;
          }

	  if ( fabs(cplt[i][j].ht) >= mda_adapt.meso_ml_conv_ht1 &&
               fabs(cplt[i][j].ht) <= mda_adapt.meso_ml_conv_ht2 )
          {
	    sum_ml_vd += cplt[i][j].avg_conv;
            num_ml_vd += 1;
          } 
        }/* END of for (j = 0; j < ncf[i]; j++) */

        if (num_ll_vd != 0)
         new_cplt[n].low_level_convergence = sum_ll_vd / num_ll_vd;
        else if (new_cplt[n].base > mda_adapt.meso_ll_conv_ht)
         new_cplt[n].low_level_convergence = (float)NX_MISSING_DATA;
        else
         new_cplt[n].low_level_convergence = 0.0;
         	
	if (num_ml_vd != 0)     
         new_cplt[n].mid_level_convergence = sum_ml_vd / num_ml_vd;    
        else if (new_cplt[n].base > mda_adapt.meso_ml_conv_ht2)   
         new_cplt[n].mid_level_convergence = (float)NX_MISSING_DATA;
        else
         new_cplt[n].mid_level_convergence = 0.0;

	/* Classify 3D Couplets */
	/* Find base, top, and depth of 3-km continuous vertical core of couplet */
	SHALLOW   = FALSE;
  	sh_rank   = 0;
  	rel_depth = NX_MISSING_DATA;

        /* assign rank_set_flag to 0 */
        rank_set_flag = 0;

	for( core_rank = MDA_2D_MAX_RANK; core_rank >= MESO_MIN_RANK; core_rank--)
        {
	  mda3d_rank(i, n, core_rank, MESO_MIN_DEPTH, &core_base, &core_top, &core_depth, new_cplt);

	  if (core_base == HEIGHT_INIT && core_top == HEIGHT_INIT)
           continue;

	  /* Assign 3D couplet strength based on vertical core if core meets 3 km
	   * depth and 5 km base threshold. If not, loop down 1 rank and try again */
	  if ( fabs(core_base) <= MESO_MAX_BASE &&
	            core_depth >= MESO_MIN_DEPTH )
           {
	    strength_rank = core_rank;
            rank_set_flag = 1;
            if(DEBUG)fprintf(stderr,"CORE, az/ran=%6.1f/%6.1f rank=%d\n",
              new_cplt[n].ll_center_azm, new_cplt[n].ll_center_rng, core_rank);
            break;
	   }
        }/*END of for( core_rank = MESO_2D_MAX_RANK; ... */

        if (rank_set_flag != 1)
           strength_rank = 0.0;
          
	/* Assign core base, top and depth to 3D couplet */
	new_cplt[n].core_base  = core_base;
	new_cplt[n].core_top   = core_top;
	new_cplt[n].core_depth = core_depth;

        /* If enabled, search for a *potential* shallow circulation */
        if ( !(mda_adapt.mda_no_shallow) )
        {
	  for( core_rank = MDA_2D_MAX_RANK; core_rank >= MESO_MIN_RANK; core_rank--)
          {
	    /*mda3d_rank(i, n, core_rank, mda_adapt.meso_min_depth_shal, &core_base, &core_top, &core_depth, new_cplt); commented out on 03/14/2008*/
	    mda3d_rank(i, n, core_rank, MESO_MIN_DEPTH, &core_base, &core_top, &core_depth, new_cplt);

            if (core_base == HEIGHT_INIT && core_top == HEIGHT_INIT)
              continue;

            /* Determine if circulation is shallow (boundary-layer type ) */
	    if (core_top   <  mda_adapt.meso_max_top_shal  &&
    	        core_rank  >= mda_adapt.meso_min_rank_shal &&
                core_depth >= mda_adapt.meso_min_depth_shal )
	    {
  	      SHALLOW = TRUE;
              sh_rank = core_rank;
              break;
            }
          }/*END of for( core_rank = MESO_2D_MAX_RANK; ... */
        }/*END of for( !(mda_adapt.mda_no_shallow) */
	
	/* Find base, top, and depth of rank low_core_rank core 
	 * This is used to determine the relative depth for low-topped mesocyclones */
	new_cplt[n].low_toped_meso = 0;
        for (low_core_rank = MDA_2D_MAX_RANK; low_core_rank >= MESO_MIN_RANK; low_core_rank--) 
        { 
	  /*mda3d_rank(i, n, low_core_rank, 0.0 , &nssl_base, &nssl_top, &nssl_depth, new_cplt); commented out on 03/14/2008 */
	  mda3d_rank(i, n, low_core_rank, MESO_MIN_DEPTH , &nssl_base, &nssl_top, &nssl_depth, new_cplt);

          /* Determine if this circulation meets the low-topped supercell classification,
	   * if the relative depth is some percentage of the storm depth. MDA_STRM_DEPTH is
           * either the avg depth of all SCIT cells from the previous volume scan, 
	   * or the sounding or NSE-computed estimate storm-top level. 
	   * If it meets the criteria, set the strength rank to at least MESO_MIN_RANK_LT. */

          /* The NSE storm depth is calculated (future improvement), mda_strm_depth will
           * be assigned the "NSE storm depth". */

          if(mda_strm_depth == VIRTUAL_STRM_DEPTH ||
             nssl_depth     == VIRTUAL_STRM_DEPTH ||
             mda_strm_depth == 0.0)
              rel_depth = NX_MISSING_DATA;
          else
              rel_depth = MIN(1.0, (nssl_depth / mda_strm_depth));

          /* storm relative depth */
	  new_cplt[n].storm_relative_depth = rel_depth;

          if ( nssl_depth < MESO_MIN_DEPTH_LT &&
               nssl_base  < MESO_MIN_BASE_LT  &&
	       rel_depth  > MESO_MIN_RELDEPTH)
          {
            /* This feature meets low core criteria at this rank. */
            /* Select the method that produces the stongest rank. */
            
            if (low_core_rank > strength_rank)
            {
               new_cplt[n].low_toped_meso = 1;
               strength_rank = low_core_rank;
               if(DEBUG)fprintf(stderr,"LOW CORE, az/ran=%6.1f/%6.1f rank=%d\n",
                   new_cplt[n].ll_center_azm, new_cplt[n].ll_center_rng, low_core_rank);
               break;
            }/* END of if (low_core_rank > strength_rank) */
          }/* END of if */ 
        } /* END of for (low_core_rank = MESO_2D_MAX_RANK; ..) */
	     
	 /* If the nssl_depth is less than the core_depth, then read just 
          * nssl_depth to be between the nssl_base and core_top. */

	 if (nssl_base != HEIGHT_INIT &&
             nssl_depth < new_cplt[n].core_depth )
          {
	   nssl_top   = new_cplt[n].core_top;
	   nssl_depth = new_cplt[n].core_top - fabs(nssl_base) + 
	                cplt[i][0].cr * sin(mda_adapt.beam_width * DTR);
	  }

	 /* assign nssl base, nssl top and nssl depth */
         new_cplt[n].nssl_base  = nssl_base;
         new_cplt[n].nssl_top   = nssl_top;
         new_cplt[n].nssl_depth = nssl_depth;
	 
	 /* assign each circulation type 2 */
          new_cplt[n].cir_type = 2;

	 /* check to see if the circulation meet "Mesocyclone" criteria (rank=5) */
	 if ( strength_rank >= 5)
         {
          new_cplt[n].user_meso = 1;
         }
	 else
          {
	   new_cplt[n].user_meso = 0;
          }

	/* Find base, top, and depth of the "user-defined" core. Also assign
	 * 3D couplet strength based on vertical core if core meets the
	 * "user-defined" depth and base thresholds, and classify circulation
         * as a mesocyclone. */
	if ( MDA_USER_CLASS == 1)
        {
	  /*mda3d_rank(i, n, MESO_MIN_RANK_USER, MESO_MIN_DEPTH_USER, &user_base, &user_top, &user_depth, new_cplt); commented out on 03/14/2008 */
	  mda3d_rank(i, n, MESO_MIN_RANK_USER, MESO_MIN_DEPTH, &user_base, &user_top, &user_depth, new_cplt);
	  new_cplt[n].user_defined_base  = user_base;
	  new_cplt[n].user_defined_top   = user_top;
	  new_cplt[n].user_defined_depth = user_depth;

	  if (fabs(new_cplt[n].user_defined_base <= MESO_MAX_BASE_USER) &&
	      (new_cplt[n].user_defined_depth >= MESO_MIN_DEPTH_USER))
           {
	    new_cplt[n].user_meso = 1;
	    new_cplt[n].cir_type  = 2;
	   }
	  else
           {
	    new_cplt[n].user_meso = 0;
	   }
	 }/* END of if ( MDA_USER_CLASS == 1) */

         /* Assign 3D strength rank */
         new_cplt[n].strength_rank = strength_rank; /*DO IT */

         /* classify "shallow" circulations (those which exist below 3 km) */
         if (new_cplt[n].strength_rank == 0   &&
             new_cplt[n].base          <  0.0 &&
 	     SHALLOW                   == TRUE)
         {
	    new_cplt[n].cir_type = 1;
	    new_cplt[n].strength_rank = sh_rank; /* DO IT */
            if(DEBUG)fprintf(stderr,"SHALLOW, az/ran=%6.1f/%6.1f rank=%d\n",
                   new_cplt[n].ll_center_azm, new_cplt[n].ll_center_rng, sh_rank);
         }

         /* update the circulation type based on low_topped status */
         if ( new_cplt[n].low_toped_meso == 1)
            new_cplt[n].cir_type = 3; 

         /* write 3D feature output */
         if ( rel_depth == NX_MISSING_DATA)
	   rel_depth = VIRTUAL_STRM_DEPTH;
         else
           rel_depth = new_cplt[n].storm_relative_depth*HUNDRED;

	  if (PRINT_TO_FILE)
           {
	   fprintf(fp3, "%3d	%2d 	%2d		%6.1f		%6.1f		%5d\n", 
	                n, new_cplt[n].strength_rank, new_cplt[n].cir_type, 
			new_cplt[n].nssl_base, new_cplt[n].depth, new_cplt[n].low_toped_meso);
              /*
	  fprintf(fp3, "    Lowest-Tilt           Total Core  Rel   Circ\n");
	  fprintf(fp3, " ID Azmth Range Base Top  Depth Depth Depth Type MSIa MSIr NSSL LOWT\n");
	  fprintf(fp3, "--- ---- ----- ---- ---- ----- ----- ----- ----- ----- ---- ---- ----\n");
          fprintf(fp3, "%3d%6.1f%6.1f%5.1f%5.1f%6.1f%6.1f%6.1f%2d %2d%5d%5d%5d%5d\n", 
	         n, cplt[i][0].ca, cplt[i][0].cr, fabs(new_cplt[n].base), fabs(new_cplt[n].top), 
	         new_cplt[n].depth, new_cplt[n].core_depth, rel_depth, new_cplt[n].cir_type,
		 new_cplt[n].strength_rank, (int)(new_cplt[n].msi*THOUSAND + 0.5), new_cplt[n].msi_rank,
	         new_cplt[n].user_meso, new_cplt[n].low_toped_meso); 

	  fprintf(fp3, "  2D feat info:  Elev  Azmth Range  Hgt  Dia  Rot_V Shear GTGVD Rank\n");
	  fprintf(fp3, "---- ---- ----- ----- ----- ----- ----- ----- -----\n");
	  for ( j =0; j< ncf[i]; j++)
           {
	    fprintf(fp3, "                 %6.1f%6.1f%6.1f%6.1f%6.1f%6.1f%6.1f%6.1f%5.1f\n",
	           cplt[i][j].elevation, cplt[i][j].ca, cplt[i][j].cr, fabs(cplt[i][j].ht),
	 	   cplt[i][j].dia, cplt[i][j].rot_vel, cplt[i][j].shr, cplt[i][j].gtgmax, cplt[i][j].rank); 
	   }
			*/

	  } /* END of if (PRINT_TO_FILE) */

	  /* increment num of new cplt by 1 */
          (*nbr_new_cplts)++;

         }/* END of for (i = 0; i < nbr_cplt; i++) */


	  /* Filter overlapping detections, and remove those that overlap
	   * another below it. Filter detection with higher LL elevation
	   * sweep number. If the same, filter detection with lower
           * strength rank. "Filtering" means to not display the detection.
           * It is still saved for processing. */
            for ( i = 0; i < (*nbr_new_cplts) -1; i++)
            { 
	    if (new_cplt[i].strength_rank  != 0 &&
		new_cplt[i].ll_elev_sweep != -1)
             {
	      for (j = i+1; j < (*nbr_new_cplts); j++)
               {
	        if (new_cplt[j].strength_rank != 0 &&
                new_cplt[j].ll_elev_sweep != -1)
                 {
	   	  dist = sqrt((new_cplt[i].ll_center_x - new_cplt[j].ll_center_x) * 
			      (new_cplt[i].ll_center_x - new_cplt[j].ll_center_x) + 
			      (new_cplt[i].ll_center_y - new_cplt[j].ll_center_y) *
			      (new_cplt[i].ll_center_y - new_cplt[j].ll_center_y) );
		  avg_dia = (new_cplt[i].low_level_dia + new_cplt[j].low_level_dia) / 2.0;

	          if( dist < avg_dia)
		   {
		    if ( new_cplt[i].ll_elev_sweep < new_cplt[j].ll_elev_sweep)
                     new_cplt[j].ll_elev_sweep = -1;
                    else if ( new_cplt[i].ll_elev_sweep > new_cplt[j].ll_elev_sweep)
                     new_cplt[i].ll_elev_sweep = -1;
                    else
                     {
		      if (new_cplt[i].ll_elev_sweep > 1 && new_cplt[j].ll_elev_sweep > 1 )
                       {
			if ( new_cplt[i].strength_rank < new_cplt[j].strength_rank)
			 new_cplt[i].ll_elev_sweep = -1;
                        else
                         new_cplt[j].ll_elev_sweep = -1;
	               }
	             }
		   }/* END of if( dist < avg_dia) */
 
                 } /* END of if */
               }/* END of for (j = i+1; j < *nbr_new_cplts; j++) */ 

	     } /* END of if (new_cplt[i].cir_type != 2 || new_cplt[i].str ... */
            } /* END of for ( i = 0; i < *nbr_new_cplts -1; i++) */

	  /* Filter detections below minimum display rank, and set new_cplt[i].ll_elev_sweep
	   * to zero if it will not be displayed because of the rank and overlap filters */
	  for (i = 0; i < (*nbr_new_cplts); i++)
           {
            /* These two lines are comment out to remove the influence of strength rank * 
	    if (new_cplt[i].strength_rank < mda_adapt.min_filter_rank)
	      new_cplt[i].ll_elev_sweep = -1;
             ***************************************************************************/

	    if ( new_cplt[i].ll_elev_sweep != -1)
             new_cplt[i].ll_elev_sweep = 1;
            else
             new_cplt[i].ll_elev_sweep = 0;
             
            if(DEBUG && new_cplt[i].ll_elev_sweep == 0) fprintf(stderr,"OL Feature at %6.1f/%6.1f\n",
                  new_cplt[i].ll_center_azm, new_cplt[i].ll_center_rng);
	   }/* END of for (i = 0; i < *nbr_new_cplts; i++) */

	/* Close the file pointer "fp3" */
          if ( fp3 != NULL )
           fclose(fp3);
}

/*============================================================================ *
Description: This routine is used to find the base, top, and depth of a 
             "3D continuous core" of 2D features of the input strength rank
             within a couplet.
Note:
=============================================================================== */


void mda3d_rank(int i, int n, int core_rank, float depth_thresh,
		float *base, float *top, float *depth, new_cplt_t new_cplt[])
{
	/* declare local variables */
	int   j, k; /* loop index */
        int   find_base;
        int   find_top;
        float half_beam_width;
	
	/* initialize */
	*base     = HEIGHT_INIT;
	*top      = HEIGHT_INIT;
        find_base = 0;
        find_top  = 0;

        /* Compute the half-beam width at the range of this feature */
        half_beam_width = cplt[i][0].cr * 0.5 * sin (mda_adapt.beam_width * DTR);       
        
	/* Find the base of the "3D core" */
        for (j = 0; j < ncf[i]; j++)
        {
          if ( cplt[i][j].rank >= core_rank)
          {
             /* If we are at the top of the feature */ 
             if (j == (ncf[i] - 1))
             {
               /* We are at the feature top.  See if the beam width is    */
               /* enough to pass the depth threshhold with a single elev. */
               
               if ((half_beam_width * 2.) >= depth_thresh)
               {
                 *base = cplt[i][j].ht;
                 find_base = 1;
                 break;
               }
             }
             else
             {
               /* else if the next 2D component also meets this rank       */
               /* threshold or the feature is at a distance where the beam */
               /* width is large enough to meet the depth threshold with a */
               /* single elevation, then set the base height.              */
             
               if ( (cplt[i][j+1].rank     >= core_rank)    || 
                   ((half_beam_width * 2.) >= depth_thresh) )
               {
                 *base = cplt[i][j].ht;
                 find_base = 1;
                 break;
               }
             }/* END if j == (ncf[i] - 1) */
          } /* END if ( cplt[i][j].rank >= core_rank) */
	} /* END of for (j = 0; j < ncf[i]; j++) */	

        /* Find the top of the "3D core" */
        if ( find_base == 1)
        {
	  for ( k = j +1; k < ncf[i]; k++)
          {
	    if (cplt[i][k].rank < core_rank)
            {
              /* Set the core top to the previous level.        */
	      *top = fabs(cplt[i][k-1].ht);
              find_top = 1;
              break;
            }
            else if (k == (ncf[i] - 1) && cplt[i][k].rank >= core_rank)
            {
              /* Set the core top to the current level.         */
              /* Because this level is still within the core    */
              /* don't set the find_top flag.                   */
	      *top = fabs(cplt[i][k].ht);
	    }
            else
            {
              ;
            }
          } /* END of for ( k = j +1; k < ncf[i]; k++) */
           
          /* If we didn't find the actual core top, set it to   */
          /* the top of the entire feature.                     */ 
          if ( !find_top)
           *top = new_cplt[n].top;
           
        }/* END of if( find_base == 1) */

	/* Find the depth of the "3D core" */
	if ( *base != HEIGHT_INIT)
        {
	  if ( *top > MESO_MAX_DEPTH)
          {
            /* Cap the core top height at the maximum value.    */
            /* Compute the depth and add a half beam width to   */
            /* the base only.                                   */
	    *top = MESO_MAX_DEPTH;
            *depth = *top - fabs(*base) + half_beam_width;
          }
          else
          {
            /* Compute the depth and add a half beam width for   */
            /* the base and top.                                 */
	    *depth = *top - fabs(*base) + (half_beam_width * 2.);
          }
          if ( *depth < 0.0)
           *depth = 0.0;
        }
        else
        {
	  *depth = VIRTUAL_STRM_DEPTH;
        }
}
