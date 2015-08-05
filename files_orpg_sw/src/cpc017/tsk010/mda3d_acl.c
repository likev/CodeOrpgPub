/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 20:11:03 $
 * $Id: mda3d_acl.c,v 1.18 2014/05/13 20:11:03 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         mda3d_acl.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        January 10, 2003                                      *
 *      References:     WDSS MDA Fortran Source code                          *
 *			ORPG MDA AEL					      *
 *                                                                            *
 *      Description:    This routine takes in 2D features and vertically      *
 *                      associates them into 3D couplets                      *
 *                      Finally will compute the attributes of all 3D couplets*
 *                                                                            *
 *	Input:		none						      *
 *									      *
 *	Output:		none						      *
 * 	Return:		none						      *
 * 	Global:		none						      *
 *	notes:		none						      *
 ******************************************************************************/

#include<math.h>
#include<time.h>
#include<string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mda3d_main.h"
#include "mda3d_acl.h"
#include "mda3d_parameter.h"

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

#include "a315.h"            /* for SCIT interface */

/* Define symbolic names */
#define		PROCESS 	1	/* Boolean parameter */
#define		TRUE		1	/* Boolean parameter */
#define		FALSE		0	/* Boolean parameter */
#define		DEBUG		0	/* Boolean parameter */
#define 	DIST_INIT	999.0   /* just number 999.0 */
#define 	THOUSAND	1000    /* just number 1000 */
#define 	ROUNDOFF	0.5     /* used to round off a number */
#define		END_OF_VOLUME   4       /* end of volume index */
#define        PSEUDO_END_OF_VOLUME   9  /* pseudo end of volume */  

/* declare global variables */
	extern cplt_t mda_sf[MESO_MAX_FEAT][MESO_NUM_ELEVS];
	extern cplt_t cplt[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
	extern int mda_num_sf[MESO_NUM_ELEVS];
	extern int current_length; /* current_length of array mda_sf[][] */
        extern int nbr_cplt;
        extern mda_th_xs_t mda_th_xs[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
        extern int ncf[MESO_MAX_NCPLT];

	extern int ttime[MESO_NUM_ELEVS], tdate[MESO_NUM_ELEVS];

	extern int num_elev;
	extern float elev_val[MESO_NUM_ELEVS];

	extern int MDA_2D_MAX_RANK;
        extern float mda_strm_depth;

void mda3d_acl()
{
	
        /* declare local variables */
	char *elev_data; /* pointer to the linear buffer of input data */
	int opstatus;
	int i, j, k, l, m; /* loop index */
        int id;

	int end_volume;

        int nbr_new_cplts; 
	new_cplt_t new_cplt[MESO_MAX_NCPLT];


	int back_check;

	int stime, sdate;
        int overflow_flg;
  
	char  *output_ptr; /* pointer to output buffer */
	int  *buffer; /* pointer to output buffer */
	int   outbuf_size; /* Actual size of output buffer. */

	int n2o_order[MESO_MAX_FEAT][MESO_MAX_FEAT];  /* The "table" containing the best order
 						       * of candidate features to be paired with
						       * feature from lower tilt. */
	float n2o_dist[MESO_MAX_FEAT][MESO_MAX_FEAT]; /* The corresponding distances between 
                    				       * the feature pairs in the above table */
	int meso_v_d_th; /* Distance threshold used for vertical association
			  * loop from v_d_th_lo to v_d_th_hi. */
	float dist; /* distance between features from the consecutive tilts */


	feature2d_t *feature_2d;

	centattr_t *scit_data;   /* Structure for CENTATTR linear buffer */

	float elevation; /* Elevation angle */
        int num_sf, elev_index;/*num of 2d features, elevation index in int*/ 

	LE_send_msg( GL_INFO, "We are at the begining of Control loop\n" ); 


	while(PROCESS)
        {

	/* Initialize variables */
	nbr_new_cplts = 0;


 	/* read data from the registered intermedial product */
	elev_data = (char *)RPGC_get_inbuf(MDA2D,&opstatus);
	
	/* check radial ingest status before continuing */
        if(opstatus!=NORMAL){
	RPGC_log_msg(GL_INFO, "MDA3D: Didn't get the input buffer\n"); 
        RPGC_abort();
        return;           /* go to the next volume */
        }

	if (DEBUG)
	fprintf(stderr, "Have successfully got the a pointer to input data \n"); 

	/* read the number of 2D features */
	memcpy(&num_sf, elev_data, sizeof(int));
        
        if (DEBUG)
        fprintf(stderr,"num_sf = %d\n", num_sf);

	/* read the elevation index from the linear buffer */
	memcpy(&elev_index, elev_data+sizeof(int), sizeof(int));

	/* reduce elev_index by 1 to make it fit C-array starting with 0 */
        elev_index = elev_index -1;
	if (DEBUG)
         fprintf(stderr, "elev_index = %d\n", elev_index);
   
        /* Initialize if this is the first elevation in the volume */
        if (elev_index == 0)
        {
           /* reset elevation table and num_elev */
           for (i = 0; i < num_elev; i++) {
            elev_val[i] = 0.0;
            ttime[i] = 0;
            tdate[i]  = 0;
           }

           num_elev = 0;
           nbr_cplt = 0;
           MDA_2D_MAX_RANK = 0;
        }
     
	/* read the elevation angle from the linear buffer */
        memcpy(&elevation, elev_data+sizeof(int)+sizeof(int), sizeof(float));

        /* update num of tilts and elevation table */
        elev_val[num_elev] = elevation;
	num_elev++;

	if (DEBUG)
	 fprintf(stderr, "elevation=%f\n", elevation);

	/* fill in the array "mda_num_sf[]" */
        mda_num_sf[elev_index] = num_sf;

	/* read end of volume index */
        memcpy(&end_volume, elev_data+sizeof(int)+sizeof(int)+sizeof(float), sizeof(int));

	/* read the time information */
	memcpy(&stime, elev_data+sizeof(int)+sizeof(int)+sizeof(float)+sizeof(int), sizeof(int));
	memcpy(&sdate, elev_data+sizeof(int)+sizeof(int)+sizeof(float)+sizeof(int)+sizeof(int),
					sizeof(int));

        /* Set the time table */
	ttime[elev_index] = stime;
	tdate[elev_index] = sdate;

        /* read the overflow flag */
        memcpy(&overflow_flg, elev_data+sizeof(int)+sizeof(int)+sizeof(float)+sizeof(int)+sizeof(int)+sizeof(int), sizeof(int));

	if(DEBUG)
	 fprintf(stderr, "ttime=%d, tdate=%d\n", ttime[elev_index], tdate[elev_index]);

	/* read 2D features from input linear buffer */
	/* allocate memory to shear_vect_2d */
        feature_2d = (feature2d_t *)calloc(num_sf, sizeof(feature2d_t));
	
        /* copy all shear vectors into shear_vect */
        memcpy(feature_2d, elev_data+sizeof(int)+sizeof(int)+sizeof(float)+sizeof(int)+
			sizeof(int)+sizeof(int)+sizeof(int), num_sf*sizeof(feature2d_t));

	/* release the input buffer */
        RPGC_rel_inbuf((void*)elev_data);

	/* fill in the array "mda_sf[][]" */
	if(DEBUG)
         fprintf(stderr, "center-azimuth center-range height diameter\n");
        for(i = 0; i < num_sf; i++)
         {
          mda_sf[i][elev_index].ca = feature_2d[i].ca;
          mda_sf[i][elev_index].cr = feature_2d[i].cr;
          mda_sf[i][elev_index].cx = feature_2d[i].cx;
          mda_sf[i][elev_index].cy = feature_2d[i].cy;
          mda_sf[i][elev_index].ht = feature_2d[i].ht;
          mda_sf[i][elev_index].dia = feature_2d[i].dia;
          mda_sf[i][elev_index].rot_vel = feature_2d[i].rot_vel;
          mda_sf[i][elev_index].shr = feature_2d[i].shr;
          mda_sf[i][elev_index].gtgmax = feature_2d[i].gtgmax;
          mda_sf[i][elev_index].rank = feature_2d[i].rank;
          mda_sf[i][elev_index].avg_conv = feature_2d[i].avg_conv;
          mda_sf[i][elev_index].max_conv = feature_2d[i].max_conv;
          mda_sf[i][elev_index].couplet_id = -1;
          mda_sf[i][elev_index].sweep_num = elev_index;
          mda_sf[i][elev_index].elevation = elevation;

	 /* Find the max rank of 2D feature */
         MDA_2D_MAX_RANK = MAX(MDA_2D_MAX_RANK, feature_2d[i].rank);
	  
          if(DEBUG)
           fprintf(stderr,"%8.4f %8.4f %7.4f %7.4f\n", mda_sf[i][elev_index].ca,
			mda_sf[i][elev_index].cr, mda_sf[i][elev_index].ht,
			mda_sf[i][elev_index].dia);
         }/* END of for(i = 0; i < num_sf; i++) */

/*---------------------------------------------------------------------------------*/
/* This is corresponding to the subroutine MDA_3D_ASSOC() in Fortran code 
 * this routine vertically associates 2D shear features into 3D shear features */

        if(DEBUG)
	 fprintf(stderr, "At begining of associate function. MDA_2D_MAX_RANK=%d\n",MDA_2D_MAX_RANK);
	if (elev_index == 0)
         {
	  /* this is the first elevation, send out the 2D features */
          /* Get an output linear buffer */
          buffer=(int *)RPGC_get_outbuf(MDA3D,BUFSIZE,&opstatus);

          /* check error condition of buffer allocation. abort if abnormal      */
          if(opstatus!=NORMAL)
           {
            RPGC_log_msg(GL_INFO, "MDA3D: cannot get the output buffer\n");
            if(opstatus==NO_MEM)
             RPGC_abort_because(PROD_MEM_SHED);
            else
             RPGC_abort();
            return;
           }

          /* assign out_put_ptr to point to buffer */
          output_ptr = (char *) buffer;

          /* copy number of 2D features into linear buffer */
          memcpy(output_ptr, &num_sf, sizeof(int));
          output_ptr += sizeof(int);

          /* copy the overflow flag into the linear buffer */
          memcpy(output_ptr, &overflow_flg, sizeof(int));
          output_ptr += sizeof(int);

	  /* copy the elevation time array into linear buffer */
          memcpy(output_ptr, &ttime, (MESO_NUM_ELEVS*sizeof(int)));
          output_ptr += (MESO_NUM_ELEVS * sizeof(int));

          memcpy(output_ptr, feature_2d, num_sf*sizeof(feature2d_t));
          output_ptr += num_sf*sizeof(feature2d_t);

	  /* Forward and release output buffer */
          outbuf_size = (int) (output_ptr - (char*) buffer);
          RPGC_rel_outbuf(buffer,FORWARD|RPGC_EXTEND_ARGS, outbuf_size);

	  /* free the memory allocated to feature_2d */
          free(feature_2d);

          continue;
         }

	 /* free the memory allocated to feature_2d */
          free(feature_2d);

	/* initialize the feature selection arrays */
	for (i = 0; i < MESO_MAX_FEAT; i++)
         {
         for (j = 0; j< MESO_MAX_FEAT; j++)
	  {
	   n2o_dist[i][j] = DIST_INIT;
           n2o_order[i][j] = -1; 
	  }
         }

	/* Search for vertical association using an increasing distance
         * threshold. Do not associate any feature whose range is less than 
         * meso_min_rng (useful for removing clutter-induced false alarms). */
	for (i = 0; i < mda_num_sf[elev_index - 1]; i++)
 	 {
	  if (mda_sf[i][elev_index -1].cr < mda_adapt.meso_min_rng)
	   continue;

	  l = -1;
	
          for (meso_v_d_th = mda_adapt.v_d_th_lo; meso_v_d_th <= mda_adapt.v_d_th_hi; meso_v_d_th++)
	   {
	    for (j = 0; j < mda_num_sf[elev_index]; j++)
             {
	      if (mda_sf[j][elev_index].cr < mda_adapt.meso_min_rng || n2o_dist[i][j] != DIST_INIT)
	       continue;
 
	      /* compute distance between features from consecutive tilts */
	      dist = sqrt((mda_sf[i][elev_index -1].cx - mda_sf[j][elev_index].cx) *
			  (mda_sf[i][elev_index -1].cx - mda_sf[j][elev_index].cx) +
			  (mda_sf[i][elev_index -1].cy - mda_sf[j][elev_index].cy) *
			  (mda_sf[i][elev_index -1].cy - mda_sf[j][elev_index].cy) );

	      /* vertically associate by comparing distance between features to 
	       * distance threshold. make sure that features from upper tilts
	       * have a larger height value than those from lower tilts. */
              if (dist <= (float)meso_v_d_th)
                {
		 if (mda_sf[i][elev_index - 1].ht <= mda_sf[j][elev_index].ht)
                  {
		   n2o_dist[i][j] = dist;
                   l++;
		   n2o_order[i][l] = j;
                  }
                }
             }/* END of for (j = 0; j < mda_num_sf[elev_index]; j++) */
           } /* END of for (meso_v_d_th = mda_adapt.v_d_th_lo;... */         
	 } /* END of for (i = 0; i < mda_num_sf[elev_index - 1]; i++) */
	
	/* Check to see if any upper features are associated with more than one
	 * lower feature. If so, take closest lower feature, and make the other
	 * lower feature use their second closest associated upper feature. Repeat
	 * the process until all lower features are associated with best upper feature. */
        for (i = 0; i < mda_num_sf[elev_index - 1]; i++)
         {

      	  line60:

	  if ( n2o_order[i][0] == -1)
           continue;
	  
          back_check = FALSE;

          for (j = 0; j < mda_num_sf[elev_index - 1]; j++)
           {
	    if (i == j || n2o_order[j][0] == -1)
             continue;

            if (n2o_order[i][0] == n2o_order[j][0])
             {
	      if (n2o_dist[i][n2o_order[i][0]] <=
                  n2o_dist[j][n2o_order[j][0]])
               {
		k = j;
                if (i > j)
                 back_check = TRUE;
               }
              else
               {
		k = i;
               }

              n2o_dist[k][n2o_order[k][0]] = DIST_INIT;

	      /*Do IT HERE NEEDs optimizing !! */
              for (m = 0; m < MESO_MAX_FEAT-1; m++)
               n2o_order[k][m] = n2o_order[k][m+1];

	      n2o_order[k][MESO_MAX_FEAT-1] = -1;

              if (back_check == TRUE)
		i = j;

              goto line60;

             } /* END of if (n2o_oder[i][0] == n2o_oder[j][0]) */
	   } /* END of for (j = 0; j < mda_num_sf[elev_index - 1]; j++) */
	 } /* END of for (i = 0; i < mda_num_sf[elev_index - 1]; i++) */ 

        /* Find the upper feature that matches the lower feature */
        for (i = 0; i < mda_num_sf[elev_index -1]; i++)
         {
	  j = n2o_order[i][0];
	  if ( j == -1)
           continue;

	/* If feature is not already part of a 3D feature, create a new 3D feature */
          if ( mda_sf[i][elev_index - 1].couplet_id == -1 )
           {
	    if (nbr_cplt >= MESO_MAX_NCPLT)
             {
              overflow_flg += 4; /* Third bit is for mda3d overflow */
              break; /* stop processing new couplets */
             }

	    /* set number of 2D features in cplt to 2,
             * which means there are 2 2D features in this cplt */
            ncf[nbr_cplt] = 2;

	    /* Start filling in cplt array */
	    cplt[nbr_cplt][0] = mda_sf[i][elev_index - 1];
	    cplt[nbr_cplt][1] = mda_sf[j][elev_index];

 	    /* Make the height a negative value if it is the lowest elevation angle */
            if ( (elev_index -1 ) == 0)
              cplt[nbr_cplt][0].ht = -cplt[nbr_cplt][0].ht;

	    /* set 3D couplet ID */
            mda_sf[i][elev_index - 1].couplet_id = nbr_cplt;
            mda_sf[j][elev_index].couplet_id = nbr_cplt;

	    /* Set low-level elevation sweep number */
	    cplt[nbr_cplt][0].sweep_num = elev_index - 1;

	    /* Start filling in time-height cross-section array */
	    mda_th_xs[nbr_cplt][0].tilt_num = elev_index -1;
	    mda_th_xs[nbr_cplt][0].couplet_id = nbr_cplt;
	    mda_th_xs[nbr_cplt][0].ht = (int)(mda_sf[i][elev_index - 1].ht * THOUSAND + ROUNDOFF);
	    mda_th_xs[nbr_cplt][0].dia = (int)(mda_sf[i][elev_index - 1].dia + ROUNDOFF);
	    mda_th_xs[nbr_cplt][0].rot_vel = (int)(mda_sf[i][elev_index - 1].rot_vel + ROUNDOFF);
	    mda_th_xs[nbr_cplt][0].shr = (int)(mda_sf[i][elev_index - 1].shr + ROUNDOFF);
	    mda_th_xs[nbr_cplt][0].gtgmax = (int)(mda_sf[i][elev_index - 1].gtgmax + ROUNDOFF);
	    mda_th_xs[nbr_cplt][0].rank = (int)(mda_sf[i][elev_index - 1].rank + ROUNDOFF);
	    mda_th_xs[nbr_cplt][0].cr = mda_sf[i][elev_index - 1].cr;
	    mda_th_xs[nbr_cplt][0].ca = mda_sf[i][elev_index - 1].ca;

	    mda_th_xs[nbr_cplt][1].tilt_num = elev_index; 
            mda_th_xs[nbr_cplt][1].couplet_id = nbr_cplt;
            mda_th_xs[nbr_cplt][1].ht = (int)(mda_sf[j][elev_index].ht * THOUSAND + ROUNDOFF);
            mda_th_xs[nbr_cplt][1].dia = (int)(mda_sf[j][elev_index].dia + ROUNDOFF);
            mda_th_xs[nbr_cplt][1].rot_vel = (int)(mda_sf[j][elev_index].rot_vel + ROUNDOFF);
            mda_th_xs[nbr_cplt][1].shr = (int)(mda_sf[j][elev_index].shr + ROUNDOFF);
            mda_th_xs[nbr_cplt][1].gtgmax = (int)(mda_sf[j][elev_index].gtgmax + ROUNDOFF);
            mda_th_xs[nbr_cplt][1].rank = (int)(mda_sf[j][elev_index].rank + ROUNDOFF);
	    mda_th_xs[nbr_cplt][1].cr = mda_sf[j][elev_index].cr;
	    mda_th_xs[nbr_cplt][1].ca = mda_sf[j][elev_index].ca;

	    /* increment nbr_cplt by 1 */
             nbr_cplt++;

             if (DEBUG)
              fprintf(stderr, "nbr_cplt=%d\n", nbr_cplt);

	
           }/* END of if ( mda_sf[i][elev_index - 1].couplet_id == 0 ) */
          else
           {
	    /* Feature is already associated with an existing 3D feature, so 
             * add the 2D feature to its cplt array */
            id = mda_sf[i][elev_index - 1].couplet_id;

            if ( cplt[id][ncf[id]-1].elevation == mda_sf[j][elev_index].elevation)
	     continue; 

            /* Continue filling in cplt array */
            cplt[id][ncf[id]] = mda_sf[j][elev_index]; 	

	    /* Set 3D couplet ID */
	    mda_sf[j][elev_index].couplet_id = id;

            /* Fill time-height cross-section array */
	    mda_th_xs[id][ncf[id]].tilt_num = elev_index;
            mda_th_xs[id][ncf[id]].couplet_id = nbr_cplt;
            mda_th_xs[id][ncf[id]].ht = (int)(mda_sf[j][elev_index].ht * THOUSAND + ROUNDOFF);
            mda_th_xs[id][ncf[id]].dia = (int)(mda_sf[j][elev_index].dia + ROUNDOFF);
            mda_th_xs[id][ncf[id]].rot_vel = (int)(mda_sf[j][elev_index].rot_vel + ROUNDOFF);
            mda_th_xs[id][ncf[id]].shr = (int)(mda_sf[j][elev_index].shr + ROUNDOFF);
            mda_th_xs[id][ncf[id]].gtgmax = (int)(mda_sf[j][elev_index].gtgmax + ROUNDOFF);
            mda_th_xs[id][ncf[id]].rank = (int)(mda_sf[j][elev_index].rank + ROUNDOFF);
	    mda_th_xs[id][ncf[id]].cr = mda_sf[j][elev_index].cr;
	    mda_th_xs[id][ncf[id]].ca = mda_sf[j][elev_index].ca;

	    /* increment number of 2D features in cplt */
            ncf[id] +=1;


           } /* END of else of "if ( mda_sf[i][elev_index - 1].couplet_id == -1)"*/
         } /* END of for (i = 0; i < mda_num_sf[elev_index -1]; i++) */ 

	if (DEBUG)
	 fprintf(stderr, "At the end of the associate function\n");
        
	if(DEBUG)
         fprintf(stderr, "Number of cplt is: %d\n", nbr_cplt);
/*---------------------------------------------------------------------------------*/

/* call a function to calculates attributes of all 3D couplets per elevation */
	  /* Get the SCIT data used to compute the average */
	  /* storm relative depth.                         */

	  if (end_volume == END_OF_VOLUME || end_volume == PSEUDO_END_OF_VOLUME) {
  	    scit_data = (centattr_t*)RPGC_get_inbuf(CENTATTR,&opstatus);
            mda_strm_depth = 0.0;
            /* mda_strm_depth is inialized each volume and used to sum     */
            /* the SCIT storm cell depths when computing the average depth */
	    if (opstatus == NORMAL) {
	      if (scit_data->hdr.nstorms != 0) {
	 	  
                /* Sum the depths */
                for (i = 0; i < MIN(10,scit_data->hdr.nstorms); i++){
	          mda_strm_depth += (scit_data->stm[i].top - scit_data->stm[i].bas);
               if(DEBUG)
                 fprintf(stderr, "SCIT top = %f base = %f\n", scit_data->stm[i].top, scit_data->stm[i].bas);
	         } 
	  
	        /* Compute the average depth */
	        mda_strm_depth = mda_strm_depth / MIN(10,scit_data->hdr.nstorms); 
	      } /* end if */
	    
  	      /* release the input buffer */
              RPGC_rel_inbuf((void*)scit_data);	    

         }
         else {
           RPGC_log_msg(GL_INFO,"MDA3D did not receive optional SCIT input");
           
	    } /*end if */
	  }

	  /* Compute the attributes of all 3D couplets. */
	  mda3d_attributes(num_elev, elev_val, &nbr_new_cplts, new_cplt);

	  /* Get an output linear buffer */
	  buffer=(int *)RPGC_get_outbuf(MDA3D,BUFSIZE,&opstatus);

          /* check error condition of buffer allocation. abort if abnormal      */
          if(opstatus!=NORMAL)
           {
	    RPGC_log_msg(GL_INFO, "MDA3D: cannot get the output buffer\n");
            if(opstatus==NO_MEM)
             RPGC_abort_because(PROD_MEM_SHED);
            else
             RPGC_abort();
            return;
           }
	  /* assign out_put_ptr to point to buffer */
          output_ptr = (char *) buffer;

	  /* copy number of new couplets into linear buffer */
          memcpy(output_ptr, &nbr_new_cplts, sizeof(int));
          output_ptr += sizeof(int);

	  /* copy the overflow flag into linear buffer */
          memcpy(output_ptr, &overflow_flg, sizeof(int));
          output_ptr += sizeof(int);

	  /* copy the elevation time array into linear buffer */
          memcpy(output_ptr, &ttime, (MESO_NUM_ELEVS*sizeof(int)));
          output_ptr += (MESO_NUM_ELEVS * sizeof(int));

        if(DEBUG)
	fprintf(stderr, "num of couplets = %d, %d\n", nbr_cplt, nbr_new_cplts);

	 /* initialize */
          for (i =0; i < nbr_cplt; i++)
           {
            /* copy number of shear vectors in one given couplet into linear buffer */
            memcpy(output_ptr, &ncf[i], sizeof(int));
            output_ptr += sizeof(int);

            /* Copy time-height X-sections into linear buffer */
            for ( j = 0; j < ncf[i]; j++)
             {
              memcpy(output_ptr, &(mda_th_xs[i][j]), sizeof(mda_th_xs_t));
              output_ptr += sizeof(mda_th_xs_t);
             }

            /* Copy 3D couplets into linear buffer */
            memcpy(output_ptr, &(new_cplt[i]), sizeof(new_cplt_t));
            output_ptr += sizeof(new_cplt_t);

          } /* END of for (i =0; i < nbr_cplt; i++) */

	/* Forward and release output buffer */
	outbuf_size = (int) (output_ptr - (char*) buffer);
        RPGC_rel_outbuf(buffer,FORWARD|RPGC_EXTEND_ARGS, outbuf_size);

 	if (end_volume == END_OF_VOLUME || end_volume == PSEUDO_END_OF_VOLUME)
        {
	 /* reset elevation table and num_elev */
         for (i = 0; i < num_elev; i++) {
            elev_val[i] = 0.0;
            ttime[i] = 0;
            tdate[i]  = 0;
         }

         num_elev = 0;
         nbr_cplt = 0;
         MDA_2D_MAX_RANK = 0;

	 break;
        }
       
	} /* END of while(PROCESS) */

	return;
}
