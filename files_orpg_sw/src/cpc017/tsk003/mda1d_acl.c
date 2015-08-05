/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 19:56:13 $
 * $Id: mda1d_acl.c,v 1.13 2014/05/13 19:56:13 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_acl.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *			ORPG MDA AEL					      *
 *                                                                            *
 * 	Description:    This module reads COMBBASE data (reflectivity and     *
 *			velocity), and call modules to (1) Adjust the velocity*
 *  			with reflectivity; (2) Find the shear section	      *
 *                      (3) Find the core shear vector, (4) Finish the shear  *
 *                      vectors that are not completed  at the end of an      *
 *                      elevation.
 *									      *
 *	notes:		none						      *
 ******************************************************************************/

#include<math.h>

#include "mda1d_main.h"
#include "mda1d_parameter.h"
#include "mda1d_acl.h"
#include <rpgcs.h>
#include <mda_adapt.h>

#define EXTERN extern /* Prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

/* Define symbolic names */
#define		ONE_ELEVATION	1	/* Boolean parameter */
#define		ONE_VOLUME	1	/* Boolean parameter */
#define		TRUE		1	/* Boolean parameter */
#define		PRINT_TO_FILE	0	/* 1: yes, print output to file */
#define         BEG__OF_ELEV    0       /* begining of elevation */
#define         BEG_OF_VOL      3       /* begining of volume */
#define         PSU_END_OF_ELEV 8       /* pseudo end of elevation */
#define         PSU_END_OF_VOL  9       /* pseudo end of volume */
#define         END_OF_ELEV     2       /* end of elevation */
#define         END_OF_VOL      4       /* end of volume */
#define 	MAX_RANK_RANGE  30      /* Range of the rank*/
#define         FOUR            4       /* just the number 4 */
#define		THOUSAND        1000    /* just the number 1000 */
#define		INVALID_VEL     999.0   /* an invalid value */ 
#define		INVALID_REF     -25     /* an invalid value */ 
#define         SHEAR_MAX_HGT_TH 15.    /* Maximum height for shear vectors */

/* acknowledge the global variable */
        extern int YES_DO_IT; /* use to control initializing range array */


/* declare global variables */
	double mda_vect_vel[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
	double mda_vect_azm[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
 	int meso_vect_in[BASEDATA_DOP_SIZE];
	int length_vect[BASEDATA_DOP_SIZE];	
	int num_look_ahead[BASEDATA_DOP_SIZE];	
	double meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
	double meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        int meso_max_ahd[BASEDATA_DOP_SIZE];

	int have_open_vector;
	int call_from_finish;
        int overflow_flg; /* set when too many shear vectors are detected */

	int num_conv_vect;
        Conv_vect mda_conv_vect[CONV_MAX_VECT];

	double save_azm[MESO_AZOVLP_THR_SR]; 
	double save_vel[MESO_AZOVLP_THR_SR][BASEDATA_DOP_SIZE]; 

        int num_mda_vect;
        Shear_vect mda_shr_vect[MESO_MAX_VECT];


void mda1d_acl()
{

	int count;	/* the number of the radials at an elevation */
	
   	Base_data_radial
      	*base_data=NULL;/* a pointer to a base data radial structure*/

  	int ref_enable; /* variables: used to hold the status of */
  	int vel_enable; /* the individual moments of base data. */
  	int spw_enable; /* TESTed after reading first radial */

  	int opstatus;   /* variable:  used to hold return values */

	double vel[BASEDATA_DOP_SIZE], range_vel[BASEDATA_DOP_SIZE]; 
	double old_vel[BASEDATA_DOP_SIZE];
	double old_azm, azm, elevation;
	float sum_elev, mean_elev;

        double first_azm;
        int across_azm;

        double hc; /* for use of height calculation */
	double conv_max_rng_th;
	double shear_max_rng_th;
	int swp_nbr, ng_ref, ng_vel, naz;

	int  *buffer; /* pointer to output buffer */
	char  *output_ptr; /* pointer to output buffer */
        int outbuf_size;  /* Actual size of the output buffer */

	int radial_status;
	int scan_time, radial_time;
	int scan_date, radial_date;
        int azm_reso;

        int i,j,k; /* loop index */

	FILE *fp2;
	FILE *mda1d_out;

        /* the following variable is used to conver double into float */
	float shear_range, shear_beg_azm, shear_end_azm, shear_beg_vel;
        float shear_end_vel, shear_vel_diff, shear_shear, shear_maxgtgvd;
        float shear_gtg_azm, shear_rank, shear_cs_vec_flag;
	
	float conv_azm, conv_range_max, conv_range_min, conv_max_vel;
        float conv_min_vel, conv_vel_diff;

	/*================================================================*/
        /* Loop over the whole volume if everything is normal */

        /*initialize array overflow flag */
        overflow_flg = 0;

	while(ONE_VOLUME) 
        {
  	/* LABEL:READ_FIRST_RAD Read first radial of the elevation and    * 
  	 * accomplish the required moments check 			  * 
  	 * ingest one radial of data from the BASEDATA linear buffer. The * 
  	 * data will be accessed via the base_data pointer 		  */
         base_data = RPGC_get_inbuf_by_name( "SR_COMBBASE", &opstatus );

  	 /* check radial ingest status before continuing */
  	 if(opstatus!=NORMAL){
      	   RPGC_abort();        
           return;           /* go to the next volume */
    	 }


  	 /* test to see if the required moment (velocity) is enabled   */
  	 RPGC_what_moments((Base_data_header*)base_data,&ref_enable,
                     &vel_enable,&spw_enable);

  	 if(vel_enable!=TRUE || ref_enable != TRUE){
            RPGC_log_msg(GL_INFO,
              "mda1d_acl.c: required moments are not available\n");

            RPGC_rel_inbuf((void*)base_data);      
      	    RPGC_abort_because(PROD_DISABLED_MOMENT);    
            break;
    	 }

         /* pick up information we want from the radaial data*/
         mda1d_data_preprocessing(base_data, &old_azm, &elevation, &swp_nbr,
		&ng_ref, &ng_vel, old_vel,
		&radial_status, &radial_time, &radial_date,  &azm_reso);

	 /* check to see if we read the first radial of the elevation *
          * if not, release inbuf and goto read next radial           */
         /* read the radial status flag */
         if (!(radial_status == BEG__OF_ELEV || radial_status == BEG_OF_VOL))
         {
           RPGC_rel_inbuf((void*)base_data);
           continue; /* goto while(ONE_VOLUME) */
         }


	 /* accumulate elevations */
	 sum_elev = elevation;
	 count = 1;

         /* get the azimuth of the first radial at the elevation */
	 /* pick up the elevation scan time and date */
         first_azm = old_azm;
	 scan_time = radial_time;
	 scan_date = radial_date;

         /* initialize some of varaibles */
         naz = 0;
	 num_mda_vect = 0;
	 num_conv_vect = 0;
         call_from_finish = 0;
	 across_azm = 0;
         save_azm[naz] = old_azm; /* save the azimuth */
	
         for (k = 0; k< ng_vel; k++)
         {
            meso_vect_in[k] = 0; /* initilize the array to 0 */
            save_vel[naz][k] = old_vel[k]; /* save the velocity */
         }
     
         /* calculate range threshold for shear and convergence vectors */
         hc = IR*RE;
         conv_max_rng_th = sqrt( (hc * sin (elevation * DEGREE_TO_RADIAN)) *
                                 (hc * sin (elevation * DEGREE_TO_RADIAN)) +
                              2 * hc * CONV_MAX_HGT_TH) -
                                  hc * sin (elevation * DEGREE_TO_RADIAN);

         shear_max_rng_th = sqrt( (hc * sin (elevation * DEGREE_TO_RADIAN)) *
                                  (hc * sin (elevation * DEGREE_TO_RADIAN)) +
                               2 * hc * SHEAR_MAX_HGT_TH) -
                                   hc * sin (elevation * DEGREE_TO_RADIAN);

         /* initialize the range array for velocity *
          * do it only once per run */
         if(YES_DO_IT)
	 {
           YES_DO_IT = 0; /* turn it to off */
           for (i=0; i < BASEDATA_DOP_SIZE; i++)
           {
             range_vel[i] = (i)*DOP_BIN_SIZE + (DOP_BIN_SIZE / 2.0);
           } 
         
           /* set up tables for Strength Rank and look-ahead mode */
           mda1d_set_table(range_vel);
         }

         /* release the input buffer */
	 RPGC_rel_inbuf((void*)base_data);

	 /* find convergence vectors */
         mda1d_conv_vector(old_vel, old_azm, range_vel, ng_vel, conv_max_rng_th);

    	 /* ELEVATION PROCESSING SEGMENT. continue to ingest and process     * 
    	  * individual base radials until either a failure to read valid     * 
    	  * input data (a radial in correct sequence) or until after reading * 
    	  * and processing the last radial in the elevation 		    */
    	 while(ONE_ELEVATION) {
   
           /* LABEL:READ_NEXT_RAD Read the next radial of the elevation      * 
            * ingest one radial of data from the BASEDATA linear buffer. The * 
            * data will be accessed via the base_data pointer                */
           base_data = RPGC_get_inbuf_by_name( "SR_COMBBASE", &opstatus );         

           /* check radial ingest status before continuing                   */
           if(opstatus!=NORMAL)
           {
	      RPGC_log_msg(GL_INFO, "MDA1D: cannot get the input buffer\n");
              RPGC_abort();
              return;
           }

           /* process the input data */	
	   mda1d_data_preprocessing(base_data, &azm, &elevation, &swp_nbr,
                  &ng_ref, &ng_vel, vel,
                  &radial_status, &radial_time, &radial_date,  &azm_reso);

           /* release the input buffer */
           RPGC_rel_inbuf((void*)base_data);

	   /* accumulate elevation */
           sum_elev += elevation;
	   count +=1;

           /* increment naz: number of radials */
           naz++;

	   /* Processing the data */
           if (across_azm != 1)
           {
              /* set "call_from_finish" to false (0) */
	      call_from_finish = 0;

              /* find shear vectors */
              mda1d_find_shear(azm, vel, old_azm, old_vel, range_vel, ng_vel, naz,azm_reso);
       
              /* find convergence vectors */
              mda1d_conv_vector(vel, azm, range_vel, ng_vel, conv_max_rng_th);

	      /* write the current azimuth value into old azimuth */
	      old_azm = azm;

              /* test to see if the first and last azm overlap */
              if (radial_status == PSU_END_OF_ELEV || 
                  radial_status == PSU_END_OF_VOL  ||
                  radial_status == END_OF_ELEV     ||
                  radial_status == END_OF_VOL)
              {
                across_azm = 1;
                break;
              }
           } /* END of if (across_azm != 1) */

         } /* END of while(ONE_ELEVATION) */

	 /* calculate mean of elevation in this sweep */
	 if (count != 0)
	    mean_elev = sum_elev / count;

	 /* since we reach the end of a elevation,              *
	  * we need to complete those vectors that have	       *
	  * not closed yet upon reaching the end of a elevation */
         if (across_azm == 1)
           mda1d_finish_vectors(ng_vel, old_vel, old_azm, range_vel, naz, azm_reso);

	 /* sort the vectors in decreasing range */
	 mda1d_vect_sort();
     
	 /* find the duplicate vectors */
	 mda1d_clean_duplicate();

	if (PRINT_TO_FILE)
        {
        /* write vector information to output file */
        mda1d_out = fopen("mda1d_out.data", "a");
        fp2 = fopen("fort.61tmp","a");

	if (num_mda_vect > 0)
         {
          fprintf(mda1d_out, "\nelevation = %f\n", elevation);
          fprintf(mda1d_out, "Range Beg_Azm End_Azm Beg_Vel End_Vel");
          fprintf(mda1d_out, " Vel_Dif Shear   MxGtgVD Gtg_Azm Rank Flg\n");
          fprintf(mda1d_out, " (km)   (deg)   (deg)   (m/s)   (m/s)   (m/s)");
          fprintf(mda1d_out, " (m/s/k)   (m/s)   (deg)\n");
          fprintf(mda1d_out, " ----- ------- ------- ------- ------- -------");
          fprintf(mda1d_out, " ------- ------- ------- ---- ---\n");
          for (j = 0; j < num_mda_vect; j++)
           {
            fprintf(mda1d_out, "%5.1f%8.1f%8.1f%8.1f%8.1f%8.1f%8.1f%8.1f%8.1f",
 	            mda_shr_vect[j].range, mda_shr_vect[j].beg_azm, mda_shr_vect[j].end_azm,
		    mda_shr_vect[j].beg_vel, mda_shr_vect[j].end_vel, mda_shr_vect[j].vel_diff,
                    mda_shr_vect[j].shear, mda_shr_vect[j].maxgtgvd, mda_shr_vect[j].gtg_azm);
            fprintf(mda1d_out, "%5.1f%5.1f\n", mda_shr_vect[j].rank, mda_shr_vect[j].cs_vec_flag);
           }
         }         
        else
         {
          fprintf(mda1d_out, "No patern vectors found on this tilt\n");
         }
        
	/* print out convergence vector */
	if (num_conv_vect >0)
         {
          for (j = 0; j < num_conv_vect; j++)
           {
            fprintf(fp2, "%10.1f%10.1f%10.1f%10.1f%10.1f%10.1f\n",
		   mda_conv_vect[j].azm, mda_conv_vect[j].range_max,
		   mda_conv_vect[j].range_min, mda_conv_vect[j].max_vel,
		   mda_conv_vect[j].min_vel, mda_conv_vect[j].vel_diff);
           }
          fprintf(fp2, "---------------------------------------------\n");
         }
        else
         {
          fprintf(fp2, "No convergence vectors found on this tilt\n");
         }
	/* close the file mda1d_out */
	fclose(mda1d_out);
	fclose(fp2);
        }

	/* allocate a partition (accessed by the pointer, buffer) within the  */
        /* MDA1D linear buffer. error return in opstatus                      */
        buffer=(int *)RPGC_get_outbuf(MDA1D,BUFSIZE,&opstatus);
        /* check error condition of buffer allocation. abort if abnormal      */
        if(opstatus!=NORMAL)
         {
	  RPGC_log_msg(GL_INFO, "MDA1D: cannot get the output buffer\n");
          if(opstatus==NO_MEM)
          RPGC_abort_because(PROD_MEM_SHED);
          else
          RPGC_abort();
          break;
         }

	/* assign out_put_ptr to point to buffer */
	output_ptr = (char *) buffer;
        
        /* copy number of shear vectors into linear buffer */
        memcpy(output_ptr, &num_mda_vect, sizeof(int));
        output_ptr += sizeof(int);

	/* copy mean elevation angle into linear buffer */
	memcpy(output_ptr, &mean_elev, sizeof(float));
	output_ptr += sizeof(float);

	/* load elevation index */
	memcpy(output_ptr, &swp_nbr, sizeof(int));
        output_ptr += sizeof(int);

	/* load radial status */
	memcpy(output_ptr, &radial_status, sizeof(int));
        output_ptr += sizeof(int);

	/* load elevation scan time */
	memcpy(output_ptr, &scan_time, sizeof(int));
        output_ptr += sizeof(int);

	/* load elevation scan date */
        memcpy(output_ptr, &scan_date, sizeof(int));
        output_ptr += sizeof(int);

        /* load array overflow flag */
        memcpy(output_ptr, &overflow_flg, sizeof(int));
        output_ptr += sizeof(int);

        /* copy shear vectors into linear buffer */
        for (i = 0; i < num_mda_vect; i++)
         {
	  /* convert the double to float to save space */
          shear_range = mda_shr_vect[i].range;
          shear_beg_azm = mda_shr_vect[i].beg_azm;
          shear_end_azm = mda_shr_vect[i].end_azm;
          shear_beg_vel = mda_shr_vect[i].beg_vel;
          shear_end_vel = mda_shr_vect[i].end_vel;
          shear_vel_diff = mda_shr_vect[i].vel_diff;
          shear_shear = mda_shr_vect[i].shear;
          shear_maxgtgvd =mda_shr_vect[i].maxgtgvd;
          shear_gtg_azm = mda_shr_vect[i].gtg_azm;
          shear_rank = mda_shr_vect[i].rank;
          shear_cs_vec_flag = mda_shr_vect[i].cs_vec_flag;

          memcpy(output_ptr, &shear_range, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_beg_azm, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_end_azm, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_beg_vel, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_end_vel, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_vel_diff, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_shear, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_maxgtgvd, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_gtg_azm, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_rank, FOUR);
          output_ptr +=FOUR;
          memcpy(output_ptr, &shear_cs_vec_flag, FOUR);
	  output_ptr +=FOUR;
         }

	/* copy number of convergence vectors into linear buffer */
        memcpy(output_ptr, &num_conv_vect, sizeof(int));
        output_ptr += sizeof(int);

        /* write the convergence vectors into buffer */
	for (j = 0; j < num_conv_vect; j++)
         {
          /* convert double to float to save on space */
          conv_azm = mda_conv_vect[j].azm;
          conv_range_max = mda_conv_vect[j].range_max;
          conv_range_min = mda_conv_vect[j].range_min;
          conv_max_vel = mda_conv_vect[j].max_vel;
          conv_min_vel = mda_conv_vect[j].min_vel;
          conv_vel_diff = mda_conv_vect[j].vel_diff;

          memcpy(output_ptr, &conv_azm, FOUR);
	  output_ptr +=FOUR;
	  memcpy(output_ptr, &conv_range_max, FOUR);
          output_ptr +=FOUR;
	  memcpy(output_ptr, &conv_range_min, FOUR);
          output_ptr +=FOUR;
	  memcpy(output_ptr, &conv_max_vel, FOUR);
          output_ptr +=FOUR;
	  memcpy(output_ptr, &conv_min_vel, FOUR);
          output_ptr +=FOUR;
	  memcpy(output_ptr, &conv_vel_diff, FOUR);
          output_ptr +=FOUR;
         }
        /* release output buffer */
        outbuf_size = (int) (output_ptr - (char *) buffer);
	RPGC_rel_outbuf(buffer,FORWARD|RPGC_EXTEND_ARGS,outbuf_size);

	/* Test to see if we are at the end of a volume */
	if(radial_status == END_OF_VOL ||
           radial_status == PSU_END_OF_VOL)
         {
	 /* Go out of the volume loop */
	 break;
         } 
	} /* END of while(ONE_VOLUME) */
   
       return;

} /* END of the function */

/*****************************************************************/
/******************************************************************

   Description:
	Read expected header information from Base_data_radial;
	Convert vel and ref in ICD to coresponding real values
 	Adjust velocity with reflectivity
   Inputs:
	base_data - A pointer to one radial of base data 

   Outputs:
	azimuth - azimuth angle of the radial
	elevation - elevation angle
	swp_nbr - sweep number 
	ndate - date in mm/dd/yy
	ntime - time in hh/mm/ss
	ng_ref - number of gates for reflectivity data
	ng_vel - number of gates for velocity data
	end_vol - end of volume scan indicator (1, yes)
	gs_ref - gate spacing of reflectivity data
	gs_vel - gate spacing of velocity data
    
     	vel[920] - the velocity adjusted by reflectivity
	
   Returns:
	none
   Notes:

******************************************************************/

void mda1d_data_preprocessing(Base_data_radial *base_data, double *azimuth,
	double *elevation, int *swp_nbr, int *ng_ref, int *ng_vel,
	double vel[],
	int *radial_status, int *radial_time, int *radial_date,int *azm_reso)
{
	/* local variables */
	int i; /* loop index */
        int j; /* bin index of reflectivity */    
        short refl[MAX_BASEDATA_REF_SIZE]; /* reflectivity in ICD format */

        static int elev_index = -1;

        /* get azimuth, elevation, swp_nbr, ng_ref, ng_vel *
	 * from the Base_data_radial *base_data   */
	*azimuth = base_data->hdr.azimuth;
	*elevation = base_data->hdr.elevation;

        /* Account for any supplemental scans. */
        if( (elev_index < 0)
                 ||
            (base_data->hdr.status == GOODBVOL)
                 ||
            (base_data->hdr.status == GOODBEL)
                 || 
            (base_data->hdr.status == GOODBELLC) )
        {
           
           /* MDA should not process supplemental scans.  Therefore we
              need to adjust for this if a SAILS VCP is in progress. */
           elev_index = RPGCS_remap_rpg_elev_index( base_data->hdr.vcp_num,
                                                    base_data->hdr.rpg_elev_ind );
        }

        *swp_nbr = elev_index;

	*ng_vel = base_data->hdr.n_dop_bins;
	*ng_ref = base_data->hdr.n_surv_bins;

	*radial_status =  base_data->hdr.status;
	*radial_time = (base_data->hdr.time)/THOUSAND;
	*radial_date = base_data->hdr.date;
        *azm_reso = base_data->hdr.azm_reso;

        /* convert reflectivity data writen in ICD into DBZ *
	 * use the value INVALID_REF to represent missing data(0)   *
 	 * and range-folding data (1)			    */
 	for (i = 0; i < *ng_ref; i++)
  	{
          if(base_data->ref[i] == 0 || base_data->ref[i] == 1)
	    refl[i] = INVALID_REF;
	  else
            refl[i] = RPGCS_reflectivity_to_dBZ(base_data->ref[i]);
	}
	
/****** The following code was added to handle spot blanked radials that ****/
/*      do not have any gates of data.  It also handles radials that have   */
/*      too few gates to work with.  In order to take advantage of MDA's    */
/*      look ahead capability, a dummy, empty radial is made so that        */
/*      features crossing the spot blanked radial(s) have a chance to       */
/*      continue detection as they move through the empty radial(s).        */
/*      By setting the reflectivity data to below threshold, the velocity   */
/*      data will also be flaged in the loop below.                         */
	if ((*ng_ref <= mda_adapt.conv_max_lookahd) || (*ng_vel <= mda_adapt.conv_max_lookahd)){
	   *ng_ref = MAX_BASEDATA_REF_SIZE;
	   *ng_vel = BASEDATA_DOP_SIZE;
	   for (i = 0; i < *ng_ref; i++)
              refl[i] = INVALID_REF;
        }
/********         End of special spot blank code                   **********/

        /* convert the velocity writen in ICD into real value in m/s *
         * use INVALID_VEL to represent the missing or invalid data*
         * adjust the velocity with reflectivity threshold           */

        RPGCS_set_velocity_reso(base_data->hdr.dop_resolution);
        for (i = 0; i < *ng_vel; i++)
	{
	 /* find the bin index of the corresponding reflectivity data *
	  * for example, vel(0,1)<->refl(0), vel(2,3,4,5)<->ref(1)... */ 
         if(*azm_reso == 1) /* half-degree azimuth interval */
         j=i;
        else              /* one-degree azimuth interval */
         j = (i+2)/4;
         
         if(base_data->vel[i] ==0 || base_data->vel[i] ==1 ||refl[j] < mda_adapt.min_refl)
           vel[i] = INVALID_VEL;
         else
           vel[i] = RPGCS_velocity_to_ms(base_data->vel[i]); 
	}
 		
} /* END of the function mda1d_preprocessing(..) */
