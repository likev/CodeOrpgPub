/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/12/03 20:47:39 $
 * $Id: mda2d_acl.c,v 1.10 2010/12/03 20:47:39 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda2d_acl.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        Oct. 30, 2002                                         *
 *      References:     WDSS MDA Fortran Source code                          *
 *			ORPG MDA AEL					      *
 *                                                                            *
 * 	Description:    This is a drive module which will call a few          *
 * 			functions to cobines the azimuthal shear pattern      *
 *                      vectors and form a 2D feature			      *
 *									      *
 *	notes:		none						      *
 ******************************************************************************/

#include<math.h>
#include<time.h>
#include<string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mda2d_main.h"
#include "mda2d_acl.h"
#include "mda2d_parameter.h"
#include <rpgcs.h>
#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"


/* Define symbolic names */
#define		ONE_ELEVATION	1	/* Boolean parameter */
#define		TRUE		1	/* Boolean parameter */
#define		FALSE		0	/* Boolean parameter */
#define		DEBUG		0	/* 1:print out debug information into log file */
#define		PRINT_TO_FILE	0	/* 1:print output into disk file */
#define 	DTR     	0.017453 /* factor to convert degree to radian */
#define 	MAX_INIT_VALUE  -999.0 /* the init value of MAX */
#define 	MIN_INIT_VALUE  999.0 /* the init value of MIN*/
#define 	TWENTY	        20 /* just a number 20 */
#define 	TEN		10 /* just a number 10 */
#define         BIN_SIZE        0.25 /* bin size */
#define         HALF_BIN_SIZE   0.125 /* half of the bin size */
#define         OUT_OF_RANGE	9000.0 /*a threshold to combine vectors */ 
#define         DEG_CIR	        360.0  /*a degree of a circle */
#define         HALF_DEG_CIR	180.0  /*half degree of a circle */
#define         THREE_HUNDRED   300.0  /* just number of 300.0 */
#define         SIXTY           60.0  /* just number of 60.0 */

/* declare global variables */
        extern float meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern float meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];

	Shear_vect_2d *shear_vect_2d; /* pointer to azimuthal shear pattern vectors */

        int list[MESO_MAX_VECT][MESO_MAX_NSV];
        int length[MESO_MAX_VECT];
        int overflow_flg;
void mda2d_acl()
{
	
        /* declare local variables */
	Shear_vect_2d feature[MESO_MAX_NSV];
	int length_feature;

	char *elev_data; /* pointer to the linear buffer of input data */
	Shear_vect *shear_vect; /* pointer to azimuthal shear pattern vectors */
	Conv_vect *conv_vect; /* pointer to radial convergent vectors */
	int num_shear_vect; /* number of shear vectors */
	int num_conv_vect; /* number of convergent vectors */

	float bazm_this, eazm_this, bazm_other, eazm_other;
	int overlap;

	int mda_v_index[MESO_MAX_FEAT][MESO_MAX_NSV];
	int mda_v_index_com[MESO_MAX_FEAT][MESO_MAX_NSV];
        int nbr_mda_feat, nbr_mda_feat_com;
        int nbr_mda_rank;
        Feature_2D mda_sf[MESO_MAX_FEAT], mda_sf_com[MESO_MAX_FEAT];

	int opstatus;

	int max_rank, min_rank;

	int kk, loop_start, loop_end;

	float elevation;

	NODEPTR L = NULL; 
	NODEPTR prev = NULL; /* the pointer to the previous node */ 
	NODEPTR tmp = NULL; /* the pointer to the node to be deleted */ 
	NODEPTR feature_ord = NULL; /* the pointer to the feature list */ 
        NODEPTR vmin_ptr = NULL; /* a pointer to a note with the minimum vel */
        NODEPTR vmax_ptr = NULL; /* a pointer to a note with the maximum vel */
        NODEPTR last_node = NULL;/* a poniter to a note at the end of list   */

	int length_feature_original;

	float radim; 
	float new_beg_azm, new_beg_vel; /* new begining azm and vel */ 
	float new_end_azm, new_end_vel; /* new end azm and vel */ 

	float azm_diff; /* azimuth difference */
	float azm_dist; /* distance between the begining and end azimuth */

	int bin_index;  /* index of a bin */
	int index_v;  /* index of vectors in a feature */

	float x1, x2, y1, y2, length_tmp;
	float ht, ratio, lr, hr;

	float vv, vmin, vmax, max_vd, max_shr, gtgmax;
	float rot_vel, shr;

	float azmin, azmax, rmin, rmax, dia, cx, cy, cr, ca;
	int i, j, k, m; /* loop indexes */

	int num_reduced; /* the number of reduced vectors */

	float avg_rng, sum_delv, num_delv, max_delv, dist, radius;

	char  *output_ptr; /* pointer to output buffer */
        int  *buffer; /* pointer to output buffer */
        int  outbuf_size; /* actual size of output buffer, in bytes */

	int index_elev; /* index of elevation angle */
	int radial_status; /* radial status index */
	int stime; /* time information */
	int sdate;  /* date information */

	int off_set; /* offset of the input bufferr */

        int elev_index;

        /* create a file pointer and open a file */
	FILE *output_2d;

	LE_send_msg( GL_INFO, "We are at the begining of Control loop\n" ); 

	/* open a file if PRINT_TO_FILE is true */
        if(PRINT_TO_FILE)
           output_2d = fopen("output_2d", "a");

 	/* read data from the registered intermedial product */
	elev_data = (char *)RPGC_get_inbuf(MDA1D,&opstatus);
	
	/* check radial ingest status before continuing */
        if(opstatus!=NORMAL){
        RPGC_abort();
        return;           /* go to the next volume */
        }
	
        /*get the elevation index */
        elev_index = RPGC_get_buffer_elev_index(elev_data);

	/* read number of shear vectors */
	memcpy(&num_shear_vect, elev_data, sizeof(int));
        off_set = sizeof(int);

	/* read the elevation angle from the linear buffer */
	memcpy(&elevation, elev_data+off_set, sizeof(float));
	off_set += sizeof(float);
       
	/* Read elevation index */
	memcpy(&index_elev, elev_data+off_set, sizeof(int));
        off_set += sizeof(int);

	/* Read radial status */
	memcpy(&radial_status, elev_data+off_set, sizeof(int));
        off_set += sizeof(int);

	/* Read scan time */
	memcpy(&stime, elev_data+off_set, sizeof(int));
	off_set += sizeof(int);

	/* Read scan date */
	memcpy(&sdate, elev_data+off_set, sizeof(int));
	off_set += sizeof(int);

        /* Read overflow flag */
        memcpy(&overflow_flg, elev_data+off_set, sizeof(int));
	off_set += sizeof(int);

	/* allocate memory to shear_vect */
	shear_vect = (Shear_vect *)calloc(num_shear_vect, sizeof(Shear_vect));
        if(DEBUG) {
          fprintf(stderr, "shear_vect=%d\n", num_shear_vect);
          fprintf(stderr, "mda_adapt.meso_2d_dist=%f\n", mda_adapt.meso_2d_dist);
          fprintf(stderr, "mda_adapt.meso_min_nsv=%d\n", mda_adapt.meso_min_nsv);
          fprintf(stderr, "mda_adapt.meso_min_radim=%f\n", mda_adapt.meso_min_radim);
          fprintf(stderr, "mda_adapt.meso_max_vect_len=%d\n", mda_adapt.meso_max_vect_len);
          fprintf(stderr, "mda_adapt.meso_max_dia=%d\n", mda_adapt.meso_max_dia);
          fprintf(stderr, "mda_adapt.meso_max_ratio=%f\n", mda_adapt.meso_max_ratio);
          fprintf(stderr, "mda_adapt.meso_min_ratio=%f\n", mda_adapt.meso_min_ratio);
        }

	/* allocate memory to shear_vect_2d */
	shear_vect_2d = (Shear_vect_2d *)calloc(num_shear_vect, sizeof(Shear_vect_2d));

	/* copy all shear vectors into shear_vect */
	memcpy(shear_vect, elev_data+off_set, 
		num_shear_vect*sizeof(Shear_vect));
        off_set += num_shear_vect*sizeof(Shear_vect);
	
        /* read the value for number of convergent vectors */
	memcpy(&num_conv_vect, elev_data + off_set, sizeof(int)); 
	off_set += sizeof(int);
	
	/* allocate memory area to shear_vect */
        conv_vect = (Conv_vect *)calloc(num_conv_vect, sizeof(Conv_vect));
        if(DEBUG)
        fprintf(stderr, "conv_vect=%d\n", num_conv_vect);

	/* read the value for number of convergent vectors */
        memcpy(conv_vect, elev_data + off_set, 
		num_conv_vect*sizeof(Conv_vect));
        
	/* since we have ingested information from the linear buffer *
         * we are ready to release the input buffer */
        RPGC_rel_inbuf((void*)elev_data);

        /* find the max rank value among the shear vectors */
	max_rank = MAX_INIT_VALUE; 
        min_rank = MIN_INIT_VALUE;

	/* transform shear_vect into shear_vect_2d */
	for (j = 0; j < num_shear_vect; j++)
         {
	  shear_vect_2d[j].range = shear_vect[j].range;
	  shear_vect_2d[j].beg_azm = shear_vect[j].beg_azm;
	  shear_vect_2d[j].end_azm = shear_vect[j].end_azm;
	  shear_vect_2d[j].beg_vel = shear_vect[j].beg_vel;
	  shear_vect_2d[j].end_vel = shear_vect[j].end_vel;
	  shear_vect_2d[j].vel_diff = shear_vect[j].vel_diff;
	  shear_vect_2d[j].shear = shear_vect[j].shear;
	  shear_vect_2d[j].maxgtgvd = shear_vect[j].maxgtgvd;
	  shear_vect_2d[j].gtg_azm = shear_vect[j].gtg_azm;
	  shear_vect_2d[j].rank = shear_vect[j].rank;
	  shear_vect_2d[j].cs_vec_flag = shear_vect[j].cs_vec_flag;
	  shear_vect_2d[j].id = j;
	  shear_vect_2d[j].marked = 0.0;

	  if (min_rank > shear_vect[j].rank)
	   min_rank = shear_vect[j].rank;

	  if (max_rank < shear_vect[j].rank)
	   max_rank = shear_vect[j].rank;
 	 }

	/* free the memory allocated to shear_vect */
        free(shear_vect);
 
	if(DEBUG)
	fprintf(stderr, "max_rank = %d min_rank = %d\n", max_rank, min_rank);

	/* initialize parameters */
	nbr_mda_feat = 0;
	nbr_mda_feat_com = 0;

	/* loop through each vector strength rank */
	for (i = max_rank; i >= min_rank; i--)
	 {

	  /* initialize variable */
          nbr_mda_rank = 0;

	  /* create a link list for each 1D vector. the link list
	   * comprises 1D vectors which overlap radially and
	   * overlap azimuthally, compared with the considered 
           * 1D vector.
 	   */
	  for (j = 0; j < num_shear_vect; j++)
           {
            /* initialize length[] */
            length[j] = 0;

	    if ( shear_vect_2d[j].rank >= (float)i)
             {
              list[j][0] = shear_vect_2d[j].id;
              length[j] = 1;

	      /* search all other vectors and see if they 
	       * overlap radially or azimuthally with this one
	       * What we did here is a trick, we don't need 
               * to search all vectors; we only need to search 
               * vectors that are a few bins away from vector j because
               * the vectors array has been sorted. 
               */
              
	      /* find the start index of the loop */
              kk = j - TEN;
              if (kk > 0)
               {
                while (fabs( shear_vect_2d[kk].range - shear_vect_2d[j].range) <= mda_adapt.meso_2d_dist)
                 {
                 kk -=TWENTY;
                 if ( kk <= 0)
                  break;
                 }
/*                loop_start = max(0,kk); */
                loop_start = 0 < kk ? kk : 0;
	       }
              else
               {
		loop_start = 0;
	       }

              /* the find the end index of the loop */
              kk = j + TEN;
              if ( kk < num_shear_vect-1)
               {
	        while (fabs( shear_vect_2d[kk].range - shear_vect_2d[j].range) <= mda_adapt.meso_2d_dist)
                 {
		  kk +=TWENTY;
                  if (kk >= num_shear_vect-1)
                   break;
		 }	
/*                loop_end = min(kk, num_shear_vect-1); */
                loop_end = kk < num_shear_vect-1 ? kk : num_shear_vect-1;
	       }
	      else
               {
		loop_end = num_shear_vect-1;
	       }

	      /* search vectors between loop_start, loop_end */
	      for (k = loop_start; k <= loop_end; k++)
	       {
		if ( shear_vect_2d[k].rank >= (float)i && k != j)
		 { 
		  /* check to see if the vectors overlap radially */
 		  if (fabs( shear_vect_2d[k].range - shear_vect_2d[j].range) <= mda_adapt.meso_2d_dist)
		   {
                    /* check to see if the vectors overlap azimuthally */
	            overlap = 0; 

	   	    bazm_this = shear_vect_2d[j].beg_azm;
		    eazm_this = shear_vect_2d[j].end_azm;	    
	   	    bazm_other = shear_vect_2d[k].beg_azm;
		    eazm_other = shear_vect_2d[k].end_azm;	    

		    if (bazm_this > THREE_HUNDRED && eazm_this < SIXTY)
                     {
		      if (bazm_other >= bazm_this || bazm_other <= eazm_this ||
		          eazm_other >= bazm_this || eazm_other <= eazm_this)
		       overlap = 1;
 		     }
		    else
                     {
		      if ((bazm_other >= bazm_this && bazm_other <= eazm_this) ||
			   (eazm_other >= bazm_this && eazm_other <= eazm_this))
		       overlap = 1;
		     }

		    if (bazm_other > THREE_HUNDRED && eazm_other < SIXTY)
                     {
		      if (bazm_this >= bazm_other || bazm_this <= eazm_other ||
			  eazm_this >= bazm_other || eazm_this <= eazm_other)
		       overlap = 1;
                     }
                    else
		     {
		      if ((bazm_this >= bazm_other && bazm_this <= eazm_other) ||
			   (eazm_this >= bazm_other && eazm_this <= eazm_other))
		       overlap = 1;
		     }

		    /* if overlap, add this vector to vectors list */
		    if (overlap) 
                     {
                      list[j][length[j]] = shear_vect_2d[k].id;
                      length[j] +=1;
 		     } /* END of if (overlap) */

		   } /* END of if (fabs( shear_vect_2d[k].range ... */ 
		 } /* END of if ( shear_vect_2d[k].rank >= (float)i) */
	       } /* END of for (k = loop_start; k <= loop_end; k++) */

	     } /* END of if ( shear_vect_2d[j].rank >= (float)i) */
           } /* END of for (j = 0; j < num_shear_vect; j++) */

/*=================================second part======================================= */
	  /* merge the related lists to form one 2D feature */
	  for (j = 0; j < num_shear_vect; j++)
	   {
            if (length[j] > 1  && shear_vect_2d[j].marked != 1) 
             {
              feature[0] = shear_vect_2d[list[j][0]];
              length_feature =1;
              shear_vect_2d[j].marked = 1;

              for (k = 1; k < length[j]; k++)
               {
                /* if the node is not marked, make this node
                 * marked to indicate it has been used.
                 * if marked, the node will not be used again. 
		 */
                m = list[j][k];
                if (shear_vect_2d[m].marked != 1)
                 {
                  if (length_feature != (MESO_MAX_NSV-1))
                   {
                   feature[length_feature] = shear_vect_2d[list[j][k]];
                   length_feature +=1;
                   shear_vect_2d[m].marked = 1;

                   if (length[m] > 1 && length_feature != (MESO_MAX_NSV-1))
                   search_list(m, feature, &length_feature);
                   }
                  else
                   {
		    break;
		   }
                 }
               } /* END of for (k = 1; k < length[j]; k++) */

	      /* print out the feature */
	      if (DEBUG) {
	      fprintf(stderr,"number of length = %d\n", nbr_mda_feat);
              fprintf(stderr, "feature %d: ", length_feature);
	      fprintf(stderr, "i=%d\n", i);
              for (k = 0; k < length_feature; k++)
                fprintf(stderr, "%f->", feature[k].range);
              fprintf(stderr,"\n");
              }

	      /* sort the features in decreasing range */
              for (k = 0; k < length_feature; k++) 
               {
                insert_list_in_order(feature[k], &feature_ord);
               }

	      /* assign values to mda_v_index */
              L = feature_ord;
              index_v = 0;
              while (L != NULL)
               {
                mda_v_index[nbr_mda_feat][index_v] = L->data.id;
                L = L->next;
                index_v +=1;
               }/* END of while (L != NULL) */

              /* save the length of feature before compressed */
                  length_feature_original = length_feature;
	
	      /* handle the case where two vectors overlap 
               * each other at the same range 
               */

	      /* initialize the number of reduced vectors */
              num_reduced = 0;


	      /* check to see if the feature has minimum num of pattern vectors */
              if (length_feature >= mda_adapt.meso_min_nsv)
               {
                L = feature_ord;
                while (L->next != NULL)
	         L = L->next; /* find the last node */
   
		/* check to see if a feature extends at least meso_min_radim radially */
	        radim = fabs(feature_ord->data.range - L->data.range);

	        if ( radim >= mda_adapt.meso_min_radim)
		 {
		  /* check for vectors having the same range */
		  L = feature_ord;

		  for (; L != NULL && L->next != NULL; L = L->next)
                   {
		    if (L->data.range == (L->next)->data.range)
                     {

		     /* define the new begin azm and vel based on old beg_vel */
		      if ((L->next)->data.beg_vel < L->data.beg_vel)
                       {
                        new_beg_azm = (L->next)->data.beg_azm;
			new_beg_vel = (L->next)->data.beg_vel;
                       }
                      else
                       {
			new_beg_azm = L->data.beg_azm;
			new_beg_vel = L->data.beg_vel;
		       }

		    /* find the new end azimuth and velocity */

                     if ((L->next)->data.end_vel > L->data.end_vel)
                      {
                        new_end_azm = (L->next)->data.end_azm;
                        new_end_vel = (L->next)->data.end_vel;
                       }
                      else
                       {
                        new_end_azm = L->data.end_azm;
                        new_end_vel = L->data.end_vel;
                       }

                      /* Check to make sure we haven't created an anticyclonic
                       * shear.  This could happen in one of of four chances
                       * when the end velocity of the shear with a smaller azimuth
                       * has a greater outbound velocity AND the beginning
                       * velocity of the other has the more inbound velocity.
                       * In this case, take the shear with the highest rank.
                       */
                      if (new_beg_azm > new_end_azm &&
                          new_beg_vel < new_end_vel   )
                      {
                        float difference;
                        difference = new_beg_azm - new_end_azm;

                      /* See if we may just be crossing zero degrees and not
                       * really making an anticyclonic shear.
                       */
                        if (difference < 100.)
                        {
                          if(DEBUG) fprintf(stderr,"!!!ANTICYCLONIC SHEAR range=%f ba/bv=%f/%f ea/ev = %f/%f\n",
                            L->data.range, new_beg_azm,
                            new_beg_vel, new_end_azm, new_end_vel);
                          if (L->data.rank >= (L->next)->data.rank)
                          {
                             new_beg_vel = L->data.beg_vel;
                             new_beg_azm = L->data.beg_azm;
                             new_end_vel = L->data.end_vel;
                             new_end_azm = L->data.end_azm;
                          }
                          else
                          {
                             new_beg_vel = (L->next)->data.beg_vel;
                             new_beg_azm = (L->next)->data.beg_azm;
                             new_end_vel = (L->next)->data.end_vel;
                             new_end_azm = (L->next)->data.end_azm;
                          }
                        }/*end if difference < 100 */
                      }
		      /* if combined vector is longer than MESO_VECT_LEN, 
	               * then keep the stronger of two vectors. If they are
                       * both in the same strength, discard both of them.
                       */

		      x1 = L->data.range * sin(new_beg_azm * DTR);
		      y1 = L->data.range * cos(new_beg_azm * DTR);
		      x2 = L->data.range * sin(new_end_azm * DTR);
		      y2 = L->data.range * cos(new_end_azm * DTR);
		      length_tmp = sqrt( (x1 - x2) * (x1 - x2) + (y1 -y2) * (y1 - y2));

		      if ( length_tmp > mda_adapt.meso_max_vect_len) 
		       {
			if ( L->data.rank == (L->next)->data.rank)
                         {
                          num_reduced +=2;
                          L->data.range += OUT_OF_RANGE;
                          (L->next)->data.range += OUT_OF_RANGE;
                         }
			else if (L->data.rank < (L->next)->data.rank)
                         {
                          num_reduced +=1;
		 	  L->data.range += OUT_OF_RANGE;
                         }
			else
 			 {
 			  num_reduced +=1;
                          (L->next)->data.range += OUT_OF_RANGE;
 			 }
		       } /* END of if ( length_tmp > mda_adapt.meso_max_vect_len) */
                      else
                       {
			/* combine the two vectors */
			num_reduced +=1;
			(L->next)->data.range += OUT_OF_RANGE;

			L->data.beg_azm = new_beg_azm;
			L->data.end_azm = new_end_azm;
			L->data.beg_vel = new_beg_vel;
			L->data.end_vel = new_end_vel;

			azm_diff = L->data.end_azm - L->data.beg_azm;
			if (azm_diff > HALF_DEG_CIR)
			 azm_diff = DEG_CIR - azm_diff;
                        else if (azm_diff < 0.0)
                         azm_diff = DEG_CIR + azm_diff;

			azm_dist = azm_diff * DTR * L->data.range;
			if (azm_dist < 0.0)
                         {
			  L->data.vel_diff = 0.0;
			  L->data.shear = 0.0;
          		 }
                        else
                         {
			  L->data.vel_diff = L->data.end_vel - L->data.beg_vel;
			  L->data.shear = L->data.vel_diff / azm_dist;
			 }

			/* update the max gate-to-gate velocity difference, and its azm */
			if ((L->next)->data.maxgtgvd > L->data.maxgtgvd)
			 {
			  L->data.maxgtgvd = (L->next)->data.maxgtgvd;
			  L->data.gtg_azm = (L->next)->data.gtg_azm;
			 }

		        /* find the bin index according to its range */
			bin_index = (int) ((L->data.range - HALF_BIN_SIZE) / BIN_SIZE);	
			
 			for ( k = MESO_MIN_RANK; k <= MESO_MAX_RANK; k++)
			 {
        		  if ((L->data.vel_diff >= meso_vd_thr[bin_index][k] &&
			       L->data.shear >= meso_shr_thr[bin_index][k]) ||
			       L->data.maxgtgvd >= meso_vd_thr[bin_index][k])
			   L->data.rank = (float)k; 
			 }  
		       } /* END of else of "if ( length_tmp > mda_adapt.meso_max_vect_len)"*/
		     } /* END of if (L->data.range == (L->next)->data.range) */ 
                   } /* END of for (; L != NULL; L = L->next) */

		  /* if duplicate vectors found, compress feature */
		  if (num_reduced != 0)
		   {
		    L = feature_ord;
		    prev = L;
	            while(L != NULL)
		     {
		      if (L->data.range > OUT_OF_RANGE)
                       {
			if (L == feature_ord ) /* this is the first node */
                         {
			  feature_ord = feature_ord->next;
			  tmp = L;
                          L = L->next;
                          free(tmp);
                          length_feature -=1;
                          prev = L;
			 }	
                        else
			 {
                          prev->next = L->next;	  
			  tmp = L;
                          L = L->next;
			  free(tmp);
			  length_feature -=1;
                         }
		       } /* END of if (L->data.range > OUT_OF_RANGE) */
                      else
                       {
  			prev = L;
                        L = L->next; 
		       } 
		     } /* END of while */
		    if (DEBUG) {
                    fprintf(stderr, "length = %d\n", length_feature);
		    print_list(feature_ord);
	            }	
		   } /* END of if (num_reduced != 0) */


		  /* check again to see if length of feature is not
                   * smaller than THREHOLD
                   */
                  if (length_feature >= mda_adapt.meso_min_nsv)
                   {
		    /* Smooth vector parameters using a MESO_SMOOTH_VECT
		     * point running average and calculate maximum (or
		     * minimum) values.
		     */
                    vmin_ptr = NULL;
                    vmax_ptr = NULL;
		    vmin = MIN_INIT_VALUE;
		    vmax = MAX_INIT_VALUE;
		    max_vd = MAX_INIT_VALUE;
		    max_shr = MAX_INIT_VALUE;
		    gtgmax = MAX_INIT_VALUE;

		    if (YES_DO_SMOOTH == 1)
                     {
                      prev = feature_ord;
		      L = feature_ord->next;
		      while (L->next != NULL)
		       {
			vv = (prev->data.beg_vel + L->data.beg_vel + 
						(L->next)->data.beg_vel) / 3.0;
			if ( vv < vmin) 
			 {
			  vmin = vv;
			  vmin_ptr = L;
			 }
			
			/* find the max velocity */
			vv = (prev->data.end_vel + L->data.end_vel + 
                                                (L->next)->data.end_vel) / 3.0;
			if (vv > vmax)
			 {
			  vmax = vv;
			  vmax_ptr =L;
			 }

			/* find max velocity difference */
			vv = (prev->data.vel_diff + L->data.vel_diff + 
                                                (L->next)->data.vel_diff) / 3.0;
			if (vv > max_vd)
			  max_vd = vv;

			/* find max shear */
			vv = (prev->data.shear + L->data.shear + 
                                                (L->next)->data.shear) / 3.0;
			if (vv > max_shr)
			  max_shr = vv;

	    		/* find the max gate-to-gate velocity */
			vv = (prev->data.maxgtgvd + L->data.maxgtgvd + 
                                                (L->next)->data.maxgtgvd) / 3.0;
			if ( vv > gtgmax)
			  gtgmax = vv;
                        
  			/* update L and prev */
			prev = L;
			L = L->next;

		       }/* END of While(L->next != NULL) */
                      /* set the last_node to be L */
		      last_node = L;
		     } /* END of if (YES_DO_SMOOTH) */
                    else 
                     {
		      L = feature_ord;
  		      last_node = L;
		      while (L != NULL)
		       {
		 	if ( L->data.beg_vel < vmin)
			 {
			  vmin = L->data.beg_vel;
			  vmin_ptr = L;
			 }

			if (L->data.end_vel > vmax)
			 {
			  vmax = L->data.end_vel;
			  vmax_ptr = L;
                         }	

			if (L->data.vel_diff > max_vd)
			  max_vd = L->data.vel_diff;

			if (L->data.shear > max_shr)
			  max_shr = L->data.shear;

			if (L->data.maxgtgvd > gtgmax)
			  gtgmax = L->data.maxgtgvd;

			/* update L and last_node */
                        last_node = L;
			L = L->next;

		       } /* END of while (L->next != NULL) */ 
		     } /* END of else of "if (YES_DO_SMOOTH)" */

		    /* calculate the center of the feature: mid-point between 
		     * the average maxmimum and minumum velocity location.
		     * Also calculate the azimuth and range of the locations
		     * of the locations of the velocity minimum and maximum, 
		     * the radial diameter.
		     */
		    azmin = vmin_ptr->data.beg_azm;
		    azmax = vmax_ptr->data.end_azm;
		    rmin = vmin_ptr->data.range;
		    rmax = vmax_ptr->data.range;
		    x1 = rmin * sin(azmin * DTR);
		    y1 = rmin * cos(azmin * DTR);
		    x2 = rmax * sin(azmax * DTR);
		    y2 = rmax *cos(azmax * DTR); 
                    dia = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
		    cx = 0.5 * (x1 + x2);
		    cy = 0.5 * (y1 + y2);
		    cr = sqrt(cx * cx + cy * cy);
		    ca = atan2(cx, cy) / DTR;
		    if ( ca < 0.0 )
			ca = DEG_CIR +ca;

		    /* calculate range/azimuthal distance aspect ratio */
                    hr = feature_ord->data.range;
		    lr = last_node->data.range;
		    ratio = (hr - lr) / dia;

		    /* calculate the feature's height */
		    ht = cr * sin(elevation * DTR) + (cr * cr) / (2.0 * IR * RE);

		   /* Check to see if the feature is not larger than meso_max_dia.
		    * Delete features whoes azmin is larger than azmax (will 
		    * happen for weak azimuth shear features that don't have 
		    * closed circulations).
		    * Delete features with a range/azimuthal distance
		    * aspect ratio which is too large or too small.
		    * Delete features above MESO_MAX_FHT km.
		    */

		   if (DEBUG)
		     fprintf(stderr, "ht = %f, MESO_MAX_FHT = %f\n", ht, MESO_MAX_FHT);

		   if (dia < mda_adapt.meso_max_dia &&
		       ( !( (azmin <= azmax && fabs(azmax - azmin) >= HALF_DEG_CIR) ||
 			  (azmin >= azmax && fabs(azmax - azmin) < HALF_DEG_CIR) ) ) &&
		       ( !(ratio > mda_adapt.meso_max_ratio || ratio < mda_adapt.meso_min_ratio) ) &&
		       (ht <= MESO_MAX_FHT ) )
		     {
		      /* calculate overall feature rotational velocity and shear */
		      rot_vel = fabs(0.5 * (vmax - vmin));
		      shr = fabs((vmax - vmin) / dia);
                      
                      /* save feature parameters */
                     if (nbr_mda_feat < MESO_MAX_FEAT)
                      {
		      mda_sf[nbr_mda_feat].ca = ca;
		      mda_sf[nbr_mda_feat].cr = cr;
		      mda_sf[nbr_mda_feat].cx = cx;
		      mda_sf[nbr_mda_feat].cy = cy;
		      mda_sf[nbr_mda_feat].ht = ht;
		      mda_sf[nbr_mda_feat].dia = dia;
		      mda_sf[nbr_mda_feat].rot_vel = rot_vel;
		      mda_sf[nbr_mda_feat].shr = shr;
		      mda_sf[nbr_mda_feat].max_vd = max_vd*0.5;
		      mda_sf[nbr_mda_feat].max_shr = max_shr;
		      mda_sf[nbr_mda_feat].gtgmax = gtgmax;
		      mda_sf[nbr_mda_feat].rank = (float)i;
		      mda_sf[nbr_mda_feat].avg_conv = 0.0;
		      mda_sf[nbr_mda_feat].max_conv = 0.0;
		      mda_sf[nbr_mda_feat].length = (float)length_feature;
		      mda_sf[nbr_mda_feat].length_org = (float)length_feature_original;
		      mda_sf[nbr_mda_feat].ratio = ratio;
		      mda_sf[nbr_mda_feat].azmin = azmin;
		      mda_sf[nbr_mda_feat].azmax = azmax;
		      mda_sf[nbr_mda_feat].lr = lr;
		      mda_sf[nbr_mda_feat].hr = hr;

		      /* update the num of the features */
		      nbr_mda_feat +=1;
		      nbr_mda_rank +=1;

		      /* write feature information to output file */
	              if (PRINT_TO_FILE) {	
		      fprintf(output_2d, "\nFeat  No.    Center     Ht  Dia. Rot_vel ");
		      fprintf(output_2d, "  Shear    Maximums      Aspect Rank\n");
		      fprintf(output_2d, " ID  Vect  AZM   RAN   (KM)  (KM)  (m/s)  ");
		      fprintf(output_2d, "(m/s/km) R_Vel Shear GTGDV Ratio  old \n");
		      fprintf(output_2d, "--- --- --- --- --- --- ------"); 
		      fprintf(output_2d, "------- --- --- --- --- --- ---\n"); 
                      fprintf(output_2d, "%3d  %3d  %5.1f %5.1f %4.1f %4.1f", 
			  nbr_mda_feat, length_feature, ca, cr, ht, dia);
		      fprintf(output_2d, "  %5.1f  %6.1f   %5.1f %5.1f %5.1f %5.1f %4.1f\n",
			rot_vel, shr, max_vd*0.5, max_shr, gtgmax, ratio, (float)i);

		      /* write feature's vector information to output file */
		      fprintf(output_2d, "\n  Pattern Vectors for above feature:\n");
		      fprintf(output_2d, "  Range      Azimuth        Velocity     Delta");
		      fprintf(output_2d, "   Shear    Max     Azm     Rank\n");
		      fprintf(output_2d, "   (km)   Begin    End    Min    Max       V ");
		      fprintf(output_2d, "            GTGVD   GTGVD   25=hi\n");
		      fprintf(output_2d, " -------  ----   ----  ----  ---- ----");
		      fprintf(output_2d, "  ----   ----   ----   ----\n");

		      for (L= feature_ord; L != NULL; L = L->next)
		      {
		      fprintf(output_2d, " %7.3f  %5.1f   %5.1f   %5.1f   ",
			  L->data.range, L->data.beg_azm, L->data.end_azm, L->data.beg_vel);
                      fprintf(output_2d,"%5.1f   %5.1f   %5.1f   %5.1f   %5.1f    %5.1f\n", L->data.end_vel,
		      L->data.vel_diff, L->data.shear, L->data.maxgtgvd, L->data.gtg_azm, L->data.rank);
		      } 
		      } /* END of if (PRINT_TO_FILE) */	
		      } /* END of if (nbr_mda_feat <= MESO_MAX_FEAT) */
		     } /* END of if (dia < mda_adapt.meso_max_dia && ... */
                   } /* END of if (length_feature >= mda_adapt.meso_min_nsv) */
		 } /* if ( radim >= mda_adapt.meso_min_radim) */
               }/* END of if (length_feature >= mda_adapt.meso_min_nsv) */

	      /* free the memory allocated to feature and feature_org */
              free_list(&feature_ord);
              L = NULL;
              prev = NULL;
              tmp = NULL;
              vmin_ptr = NULL;
              vmax_ptr = NULL;
              last_node = NULL;
             } /* END of if (list[j] != NULL && ...) */
            /* set the node unmarked again */
            shear_vect_2d[j].marked = 0;

            /* If the max num of 2d features have been reached, stop */
            /* Least significant bit of overflow_flg is set by mda1d if too many shear vectors */
            /* Second bit of overflow_flg is set by mda2d if too many 2D components */
            if (nbr_mda_feat >= MESO_MAX_FEAT){
             if(overflow_flg < 2) {
              overflow_flg += 2;
             }
             break;
            }
	   } /* END of for (j = 0; j < num_shear_vect; j++) */          

         /* Set features created from different strength thresholds into
           * a complete meso component array
           */
          if ( nbr_mda_feat > 0 && nbr_mda_feat_com < MESO_MAX_FEAT)
	  mda_features_compress(mda_sf, mda_sf_com, mda_v_index, 
	      mda_v_index_com, nbr_mda_feat, &nbr_mda_feat_com, nbr_mda_rank);
	  
	 if (DEBUG) {
	  for (j = 0; j < nbr_mda_feat_com; j++)
           {
            fprintf(stderr, "\n**************************************\n");
            for (k = 0; k < mda_sf_com[j].length; k++)
             fprintf(stderr,"index= %d", mda_v_index_com[j][k]);
           }
         } /* END of if (DEBUG) */ 
				
	 } /* END of for (i = MDA_1D_MAX_RANK; i <= MDA_1D_MIN_RANK; i++) */

	/* Reassign rank to feature based on maximum values */
	for (i = 0; i < nbr_mda_feat_com; i++)
	 {
	  /* find the bin index according to its range */
          bin_index = (int) ((mda_sf_com[i].cr - HALF_BIN_SIZE) / BIN_SIZE);
	  for (k = MESO_MIN_RANK; k <= MESO_MAX_RANK; k++)
	   {
	    if ( ( (mda_sf_com[i].max_vd) * 2 >= meso_vd_thr[bin_index][k] &&
		mda_sf_com[i].max_shr >= meso_shr_thr[bin_index][k]) ||
		mda_sf_com[i].gtgmax >= meso_vd_thr[bin_index][k])
	     mda_sf_com[i].rank = (float) k; 		
	   }
	 } /* END of for (i = 0; i < nbr_mda_feat_com; i++) */

	/* Sort the feature array by strength */
	mda2d_sort_byrank(mda_sf_com, nbr_mda_feat_com);

	/* Compute a few more attributes of each 2D feature */
        for (i = 0; i < nbr_mda_feat_com; i++)
         {
          /* Compute convergence (based on radial convergent features)
           * for the 2D MDA feature
           */
          sum_delv = 0.0;
          num_delv = 0.0;
          max_delv = MAX_INIT_VALUE;

          x2 = mda_sf_com[i].cr * sin(mda_sf_com[i].ca * DTR);
          y2 = mda_sf_com[i].cr * cos(mda_sf_com[i].ca * DTR);
          radius = (fabs(mda_sf_com[i].dia) / 2.0) + mda_adapt.meso_conv_buff;

          /* Determine which convergence vectors are close to the 2D feature */
          for (j = 0; j < num_conv_vect; j++)
           {
            avg_rng = (conv_vect[j].range_max + conv_vect[j].range_min) / 2.0;
            x1 = avg_rng * sin(conv_vect[j].azm * DTR);
            y1 = avg_rng * cos(conv_vect[j].azm * DTR);

            dist = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));

            if (dist < radius)
             {
              sum_delv += conv_vect[j].vel_diff;
              num_delv +=1;
/*              max_delv = max(max_delv, conv_vect[j].vel_diff); */
              max_delv = max_delv < conv_vect[j].vel_diff ? conv_vect[j].vel_diff : max_delv;
             }
           } /* END of for (j = 0; j < nbr_conv_vect; j++) */

        /* Compute average and maximum velocity differences */
        if (num_delv != 0)
         {
          mda_sf_com[i].avg_conv = sum_delv / num_delv;
          mda_sf_com[i].max_conv = max_delv;
         }
        else
         {
          mda_sf_com[i].avg_conv = 0.0;
          mda_sf_com[i].max_conv = 0.0;
         }

        } /* END of for (i = 0, i < nbr_mda_feat_com; i++) */

	/* release the allocated memory to "shear_vect" and "shear_vect_2d" */
        free(shear_vect_2d);
        free(conv_vect);

	/* Write mda_sf_com header information to output file */
        if (PRINT_TO_FILE)
        {
	if (nbr_mda_feat_com > 0)
         {
	  fprintf(output_2d, "\n=========================\n");
    	  fprintf(output_2d, "Compressed 2D feature array:\n");
	  fprintf(output_2d, "\nFeat  No.     Center    Ht   Dia. Rot Vel");
	  fprintf(output_2d, "  Shear     Maximums      Elev  Rank\n");
	  fprintf(output_2d, " ID  Vect  AZM   RAN  (km)  (km)  (m/s)  ");
	  fprintf(output_2d, "(m/s/km) R_Vel Shear GTGDV (deg)  new \n");
	  for (i = 0; i < nbr_mda_feat_com; i++)
	   {
	    fprintf(output_2d, "%3d  %3d  %5.1f %5.1f %4.1f %4.1f",
                      (i+1), (int)mda_sf_com[i].length, mda_sf_com[i].ca, 
		      mda_sf_com[i].cr, mda_sf_com[i].ht, mda_sf_com[i].dia);
                      fprintf(output_2d, "  %5.1f  %6.1f   %5.1f %5.1f %5.1f %5.1f %4.1f\n",
                      mda_sf_com[i].rot_vel, mda_sf_com[i].shr, mda_sf_com[i].max_vd, 
		      mda_sf_com[i].max_shr, mda_sf_com[i].gtgmax, elevation, 
		      (float)mda_sf_com[i].rank);
  	   }
	 } /* END of if (nbr_mda_feat_com > 0) */
       }/* END of if(PRINT_TO_FILE) */
	
	/* close the file */
        if(PRINT_TO_FILE)
         fclose(output_2d);

	/* Get output buffer and write MDA 2D features into linear buffer */
	/* Get an output linear buffer */
          buffer=(int *)RPGC_get_outbuf(MDA2D,BUFSIZE,&opstatus);
	  
        /* check error condition of buffer allocation. abort if abnormal      */
          if(opstatus!=NORMAL)
           {
	    RPGC_log_msg(GL_ERROR, "MDA1D: cannot get the output buffer\n");
            if(opstatus==NO_MEM)
             RPGC_abort_because(PROD_MEM_SHED);
            else
             RPGC_abort();
            return;
           }

          /* assign out_put_ptr to point to buffer */
          output_ptr = (char *) buffer;
	  
	/* Copy number of MDA 2D features into linear buffer */
        if(DEBUG)
 	fprintf(stderr, "NUM =%d\n", nbr_mda_feat_com);

	memcpy(output_ptr, &nbr_mda_feat_com, sizeof(int));
        output_ptr += sizeof(int);

	/* Copy number of tilts into linear buffer */
	memcpy(output_ptr, &index_elev, sizeof(int));
        output_ptr += sizeof(int);

	/* Copy elevation angle into linear buffer */
	memcpy(output_ptr, &elevation, sizeof(float));
        output_ptr += sizeof(float);

	/* Copy Radial status into the linear buffer */
	memcpy(output_ptr, &radial_status, sizeof(int));
        output_ptr += sizeof(int);

	/* Copy time into linear buffer */
	memcpy(output_ptr, &stime, sizeof(int));
        output_ptr += sizeof(int);

	/* Copy date into linear buffer */
	memcpy(output_ptr, &sdate, sizeof(int));
        output_ptr += sizeof(int);

        /* Copy overflow_flg into linear buffer */
        memcpy(output_ptr, &overflow_flg, sizeof(int));
        output_ptr += sizeof(int);

	/* Copy 2D features into linear buffer */
	for (i = 0; i < nbr_mda_feat_com; i++)
         {
	  memcpy(output_ptr, &(mda_sf_com[i].ca), sizeof(float));
          output_ptr += sizeof(float);
	  memcpy(output_ptr, &(mda_sf_com[i].cr), sizeof(float));
          output_ptr += sizeof(float);
	  memcpy(output_ptr, &(mda_sf_com[i].cx), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].cy), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].ht), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].dia), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].rot_vel), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].shr), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].gtgmax), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].rank), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].avg_conv), sizeof(float));
          output_ptr += sizeof(float);
          memcpy(output_ptr, &(mda_sf_com[i].max_conv), sizeof(float));
          output_ptr += sizeof(float);
	  
	 } 

	/* Forward and release the product linear buffer */
        outbuf_size = (int) (output_ptr - (char *) buffer);
	RPGC_rel_outbuf(buffer,FORWARD|RPGC_EXTEND_ARGS,outbuf_size);

	return;
}

/*==================================================================
   Description:
        merge all related lists to form one feature;
   Inputs:
         list- A pointer to a list of list
	 j   - index of current list to which lists will be merged
	 k   - index of a list merged

   Outputs:

   Returns:
        none
   Notes:
=================================================================*/

void search_list (int k, 
		 Shear_vect_2d feature[MESO_MAX_NSV], 
		 int *length_feature)
{
	/* declare local variables */
	int i, j; /* loop index */
	
              for (i = 1; i < length[k]; i++)
               {
                /* if the node is not marked, make this node
                 * marked to indicate it has been used.
                 * if marked, the node will not be used again. */
                j = list[k][i];

                if (shear_vect_2d[j].marked != 1)
                 {
		  if (*length_feature != (MESO_MAX_NSV-1))
                   {
                    feature[*length_feature] = shear_vect_2d[list[k][i]];
                    *length_feature +=1;
                    shear_vect_2d[j].marked = 1;

                    if (length[j] > 1 && *length_feature != (MESO_MAX_NSV-1))
                     search_list(j, feature , length_feature);
                   }
                  else
                   {
		    return;
	           }
                 }
               } /* END of for (i = 1; i < length[k]; i++) */
	return;

}


/*==================================================================
   Description:
        compress 2D features into a compressed array;
   Input: mda_sf[]
          mda_v_index[][]
          nbr_mda_feat
          nbr_feat_rank
   Output: mda_sf_com[]
           nbr_mda_feat_com
   Returns:
        none
   Notes:
=================================================================*/
void mda_features_compress(Feature_2D mda_sf[], Feature_2D mda_sf_com[],
	int mda_v_index[MESO_MAX_FEAT][MESO_MAX_NSV], 
	int mda_v_index_com[MESO_MAX_FEAT][MESO_MAX_NSV], 
	int nbr_mda_feat, int *nbr_mda_feat_com, int nbr_feat_rank) 
{
	/* declare the local variables */
	int i, j, k, m;
	int over_lap, num_ovlp;
	int ov_index[MESO_MAX_FEAT];
	float la, ha, lr, hr, ca, cr;
	int ovlp_two;
	int new_comp;

	if ((*nbr_mda_feat_com) == 0)
	 {
	  (*nbr_mda_feat_com) = nbr_mda_feat;
	  for ( i = 0; i < nbr_mda_feat; i++)
	   {
 	    mda_sf_com[i] = mda_sf[i];
	    for (j = 0; j < mda_sf[i].length_org; j++)
	     mda_v_index_com[i][j] = mda_v_index[i][j];
	   } /* END of for ( i = 0; i < nbr_mda_feat; i++) */
         } /* END of if (*nbr_mda_feat_com == 0) */
        else
         {
	  /* check to see if features overlap with existing meso components */

	  new_comp = (*nbr_mda_feat_com);
	  
	  /* loop all unchecked features at this strength rank*/
	  for (i = nbr_mda_feat - nbr_feat_rank; i < nbr_mda_feat; i++)
	   {
	    over_lap = 0;
            ov_index[i] = -1;
	    num_ovlp = 0;

	    la = mda_sf[i].azmin;
	    ha = mda_sf[i].azmax;
	    lr = mda_sf[i].lr;
	    hr = mda_sf[i].hr;

	    /* Loop all compressed features */
	    for (j = 0; j < (*nbr_mda_feat_com); j++)
	     {
	       for (k = 0; k < mda_sf_com[j].length_org; k++)
 		{
		 for (m = 0; m < mda_sf[i].length_org; m++)
                  {
                   if (mda_v_index_com[j][k] == mda_v_index[i][m])
                    {
		     over_lap = 1;
                     ov_index[i] = j;
		     num_ovlp += 1;
		     break;
		    }
		  } /* END of for (m = 0; m < mda_sf[i].length; m++) */
	         if (over_lap == 1)
		  break;
		}/* END of for (k = 0; k < mda_sf_com[j].length; k++) */ 

	      /* do double-check of overlap by comparing centroids */
	      if (over_lap != 1)
               {
		ca = mda_sf_com[j].ca;
		cr = mda_sf_com[j].cr;

		if (la < ha )
                 {
		  if (ca >= la && ca <= ha && cr >= lr && cr <= hr)
		   {
		    over_lap = 1;
                    ov_index[i] = j;
		    num_ovlp += 1;
	      	   }
		 } /* END of if (la < ha ) */
                else
                 {
		  if ((ca >= la || ca <= ha) && cr >= lr && cr <= hr)
                   {    
                    over_lap = 1;
                    ov_index[i] = j;
                    num_ovlp += 1;
                   }    
		 } /* END of else in "if (la < ha )" */
	       } /* END of if (over_lap ! = 1) */

	     } /* END of for (j = 0; j < *nbr_mda_feat_com; j++) */ 

	    /* if feature overlaps with more than one compressed feature,
             * assign its pointer to zero.
             */
	    if (num_ovlp != 1)
	     ov_index[i] = -1;

	   /* if feature does not overlap with one of higher rank, add it to the 
	    * meso component array.
            */
	   if (over_lap != 1)
            {
	     mda_sf_com[new_comp] = mda_sf[i];

	     /* update mda_v_index_com */
	     for (k = 0; k < mda_sf[i].length_org; k++)
	      mda_v_index_com[new_comp][k] = mda_v_index[i][k];
	     
	     new_comp +=1;

	    } /* END of if (overlap != 1) */
	     
	   } /* END of for (i = nbr_mda_feat - nbr_feat_rank; ... */

	  (*nbr_mda_feat_com) = new_comp;

	  /* Loop through all features of current rank again for inclusion into
	   * compressed feature table.
           */
	  for (i = nbr_mda_feat - nbr_feat_rank; i < nbr_mda_feat; i++)
           {
	    /* Build table of overlapping features. Only one feature of
	     * lower rank can overlap with a single feature of high rank,
	     * and vice versa. Implicitly, features that overlap with 
             * two or more of higher rank are never saved.
             */
	    if ( ov_index[i] != -1)
             {
	      ovlp_two = 0;
              for (j = i+1; j < nbr_mda_feat; j++)
	       {
		if (ov_index[i] == ov_index[j])
                 {
		  ov_index[j] = -1;
                  ovlp_two = 1;
		 }
	       }
	      
	      /* if ther are two unchecked features overlap with one compressed
	       * feature, ignore this unchecked feature
               */
	      if (ovlp_two == 1)
               {
        	ov_index[i] = -1;
                continue; 
               }

	      /* if feature overlap with one of the higher rank,
	       * replace it with the one of lower rank. */
              mda_sf_com[ov_index[i]] = mda_sf[i];
	      for (j = 0; j < mda_sf[i].length_org; j++) 
		mda_v_index_com[ov_index[i]][j] = mda_v_index[i][j];

	     } /* END of if ( ov_index[i] != -1) */
	   } /* END of for (i = nbr_mda_feat - nbr_feat_rank ... */
    
	 } /* END of else in "if (*nbr_mda_feat_com == 0)" */


} /* END of function */

/*==================================================================
   Description:
        Use the bubble sort to sort the 2D features by rank;
   Inputs:
	mda_sf_com[]
        nbr_mda_feat_com
   Outputs:
	mda_sf_com[]
        nbr_mda_feat_com
   Returns:
        none
   Notes:
=================================================================*/


void mda2d_sort_byrank(Feature_2D mda_sf_com[], int nbr_mda_feat_com)
{
	/* declare the local variables */
	int i, j, k;
	Feature_2D temp;

	/* Sort the 2D features by decreasing strength rank */
	for (i = 0; i < nbr_mda_feat_com-1; i++)
	 for (j = 0; j < (nbr_mda_feat_com -i-1); j++)
	  {
	   k = j + 1;
	   
	   if (mda_sf_com[j].rank < mda_sf_com[k].rank)
            {
	     temp = mda_sf_com[k];
             mda_sf_com[k] = mda_sf_com[j];
             mda_sf_com[j] = temp;
            }
	  } /* END of for (i = 0; i < nbr_mda_feat_com-1; i++) */

}
