/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:09:50 $
 * $Id: read_refl.c,v 1.6 2014/09/02 20:09:50 dberkowitz Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/******************************************************************************
  read_refl() reads header info. and Base Reflectivity data from BDE algorithm
              that are needed by EPRE algorithm.
    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    10/26/05      0001       C. Pham            CCR NA05-21401
    08/22/14	  0002	     Nicholas Cooper    CCR NA14-00268 Corrected to
						Surv. time instead of doppler
						time.
******************************************************************************/

/* Global and local include files ------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <rpg_globals.h>
#include <basedata_elev.h>
#include <epre_process.h>
#define SECS_PER_DAY 86400
#define MILSEC 1000


void read_refl(void *inbuf, int *elflag, int *elindx, int *tilts,
               double *elangle, int *volnum, int *vcpnum, int *radnum,
               int *beg_time, int *beg_date, int *end_time, int *end_date,
	       double azm_array[], short zrefl[][MAX_RNG], int *status,
	       int *bde_status)
{

  Compact_basedata_elev *bde = NULL; /* Pointer to an intermediate product
                                        inbuf                                 */
  int    offset, i, j;               /* Loop variables                        */
  char   msg[200];                   /* error message                         */
  short  got_good_time;              /* TRUE -> already got a time            */
  time_t max_time = 0L;	  	     /* maximum time of the sweep  	      */
  time_t min_time = 0L;		     /* minimum time of the sweep  	      */
  int    max_index = -1;	     /* index of max time of sweep 	      */
  int    min_index = -1;	     /* index of min time of sweep  	      */
  time_t radial_time = 0L;	     /* holder for each radial time	      */

  int debugit = FALSE;               /* Controls debug output in this file    */

   if (debugit) fprintf( stderr, "\nInside read_Refl function ...\n" );

/* Allocate memory to hold current intermediate product */
   bde = (Compact_basedata_elev *)inbuf;

   *status = 0;
   *elindx = (int)bde->elev_ind;
   *radnum = (int)bde->num_radials;

   if( *radnum >= MAX_RADIALS_ELEV ) {
        fprintf( stderr, "Read_Refl ERROR: num_radial correlation error\n" );
        *status = 0;
        return;
     }

   /* Get the elevation time from the start of the elevation/volume.
    * We prefer the radial marked as the start of the elevation/volume,
    * but will take the earliest one, in case that one is missing.
    */

   got_good_time = FALSE; /* start off pessimistic */

   for( i=0; i<*radnum; i++ )
   {

      *bde_status   = (int)bde->radial[i].bdh.status;

      if ((*bde_status == GOODBEL) || (*bde_status == GOODBVOL))
      {
         *vcpnum   = (int)bde->radial[i].bdh.vcp_num;
         *tilts    = (int)bde->radial[i].bdh.elev_num;
         *volnum   = (int)bde->radial[i].bdh.volume_scan_num;
         *elflag   = (int)bde->radial[i].bdh.last_ele_flag;
         *elangle  = (double)bde->radial[i].bdh.elevation;
      }

      /* Find the earlist and latest times within the surveillance cut */ 
      radial_time = (((int)bde->radial[i].bdh.surv_date - 1) * SECS_PER_DAY) +
                      ((int)bde->radial[i].bdh.surv_time / MILSEC);
      
      /* Check if the radial is the first good one */
      if(got_good_time == FALSE)
      {
	    min_time = radial_time;
	    max_time = radial_time;
	    max_index = i;
	    min_index = i;
            got_good_time = TRUE;
      }
      else   /* If it is not the first good radial */
      {
	 if (radial_time < min_time)
	 {
	     	/* min_time is the oldest time in the sweep */
	     	min_time = radial_time;
	     	min_index = i;
	 }
	 else if (radial_time >= max_time)
	 {
	    	/* max_time is the oldest time in the sweep */
	    	max_time = radial_time;
	    	max_index = i;
	 }
      }   /* end if first good radial */

      /* End of find the earlist and latest times within the surveillance cut */ 


      /* Load the azimuth angle into azimuth angle container (array) */

      azm_array[i] = (double)bde->radial[i].bdh.azimuth;

      /* Load the Base Reflectivity values into the refl container (array) */

      offset = bde->radial[i].bdh.ref_offset;
      for( j=0; j<MAX_RNG; j++ )
      {
            zrefl[i][j]=(short)bde->radial[i].radar_data[offset+j];
      } /* End of for loop to load reflectivity values */

   } /* End of radnum loop */

   *beg_date = (int)bde->radial[min_index].bdh.surv_date;
   *beg_time = (int)bde->radial[min_index].bdh.surv_time;
   *end_date = (int)bde->radial[max_index].bdh.surv_date;
   *end_time = (int)bde->radial[max_index].bdh.surv_time;

   #ifdef EPRE_DEBUG
    int year_start   = 0; 
    int month_start  = 0; 
    int day_start    = 0; 
    int hour_start   = 0; 
    int minute_start = 0; 
    int second_start = 0;

    int year_end   = 0; 
    int month_end  = 0; 
    int day_end    = 0; 
    int hour_end   = 0; 
    int minute_end = 0; 
    int second_end = 0; 
    
    /* Convert min/max times from s to m/d/y H:M:S */

    RPGCS_unix_time_to_ymdhms(min_time, &year_start, &month_start, &day_start,
			      &hour_start, &minute_start, &second_start);
    RPGCS_unix_time_to_ymdhms(max_time, &year_end, &month_end, &day_end,
			      &hour_end, &minute_end, &second_end);

    sprintf(msg, "%s Start(Az %6.2f): %.2d/%.2d/%d %.2d:%.2d:%.2d   End(Az %6.2f): "
                  "%.2d/%.2d/%d %.2d:%.2d:%.2d\n",
                "read_HeaderData:", azm_array[min_index], month_start, day_start,
		year_start, hour_start, minute_start, second_start,
		azm_array[max_index], month_end, day_end, year_end, hour_end, 
                minute_end, second_end);
   	fprintf(stderr, msg);

    sprintf(msg, "%s Start(i= %d): date: %d time (msec) %d   End(i= %d): date: %d time (msec) %d\n",
                "read_HeaderData:", min_index, *beg_date, *beg_time,
		max_index, *end_date, *end_time);
   	fprintf(stderr, msg);
   #endif


/* Print out some variables regarding refl. data */
   if ( debugit ) {
    fprintf( stderr,"===> Product 150 BDE ..........\n");
    fprintf( stderr, "current_vcp=%d\tcurrent_elev=%d\tvolumeNumber=%d\n",
                     *vcpnum,*tilts,*volnum);
    fprintf( stderr, "Elev_Index = %d  Elev_angle=%f\n",
                     *elindx,*elangle);
    fprintf( stderr, "passedDate=%d\tpassedTime=%d\tLelev_flag=%d\n",
                     *beg_date,*beg_time,*elflag);
    fprintf(stderr,"RADIAL NUMBER = %d\n",*radnum);

    for( i=0; i<*radnum; i++ ) {
      if(i == 0 || i == *radnum-1) {
        fprintf( stderr, "\n***Radial=%d\tAZIMUTH=%f\n",i,azm_array[i]);
        for( j=0; j<MAX_RNG; j++ ) {
          fprintf(stderr,"%d ",zrefl[i][j]);
        }
      fprintf( stderr, "\n");
      }
     }
   }  /*End of debugit */

/* refl.data and azimuth angle are sucessfully read, the status is returned */
   *status = 1;    
} 
