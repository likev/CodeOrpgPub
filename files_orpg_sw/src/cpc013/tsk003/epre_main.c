/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:01:33 $
 * $Id: epre_main.c,v 1.10 2014/09/02 20:01:33 dberkowitz Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*******************************************************************************
Description:
    epre_main.c is the main file for the Enhanced Preprecessing (EPRE) function. 
    The algorithm receives REFLDATA_ELEV and RECCLDIGREF data as inputs
    and produces a HYBRID SCAN (Reflectivity) Array and a HYBRID SCAN (Elevation
    Indices) Array for each volume.  The output is required by the PPS RATE
    ALGORITHM, the HYDROMET PRODUCTS task, the RADAR CODED MESSAGE and others.
    It is based on Enhanced Preprocessing (EPRE) prototype code from Tim 
    O'Bannon, January 2002.

Input:   none
Output:  none
Returns: none
Globals: none
Notes:   All input and output for this module are provided through
         ORPG API services calls.

Modified: Chris Calvert   11/03    - included callback function and new dea
                                     adaptation data format
          Cham Pham       12/05    - CCR NA05-21401 Changed for Linux - replaced 
                                     single to double precision floating point.
	  Nicholas Cooper 08/22/14 - CCR NA14-00268 Average time calculation 
	  Murnan		     modified to match DP QPE method (more
                                     accurate)
*******************************************************************************/

/* Global Include Files ---------------------------------------------------- */
#include "epre_process.h"
#include "epre_main.h"

int main(int argc, char* argv[])
{
   
/* Variable Declarations and Definitions */
   short algProcess = TRUE,     /* Loop control for entire algorithm         */
         eleProcess = TRUE;     /* Loop control for radial processing        */
   int   rc = 0;		/* return code from function call */

   Compact_basedata_elev *elev_prod = NULL; /*Pointer to input buffer of
                                              base elevation data            */

   char *outbuf=NULL;           /* Pointer to to hybrid scan intermediate
                                   product output buffer                     */
   char *recref=NULL;           /* Pointer input buffer of AP/Clutter data   */

   int  iostat;                 /* Status from API calls                     */

/* Test Variables ****/
   int elindxF=0;		/* Elev. index of base elevation             */
   int elindxR=0;               /* Elev. index of clutter reflectivity       */
   int volnumF=0;               /* Volume of base elevation                  */
   int volnumR=0;               /* Volume of clutter reflectivity            */
   int cur_vol=0;               /* Current volume scan                       */

   double azimuth_array[MAX_RAD],
          elev_angle;

/* Epre variables declaration ***/

   int reflstat,
       apstat,
       bde_stat,
       elev_flag,
       cur_vcp,
       cur_elev,
       nvcp,
       max_ntilts,
       radial_num,
       spotblank,
       start_Zdate,
       start_Ztime,             /* Milsec from midnight */
       end_Zdate,
       end_Ztime,               /* Milsec from midnight */
       ret_date,
       ret_time,
       new_hys,
       full_hys,
       rain_detected,
       eleangle,
       elev_ind,
       new_vol;

   time_t b_time, e_time;

   short Zrefl[MAX_RAD][MAX_RNG],
         APclutter[MAX_RAD][MAX_RNG];

/* Flag Variables Declaration ***/
   const static short BYTE_MASK = 255; /* Mask to obtain right byte infomation*/
   int debugit = FALSE;                  /* Controls debug output in this file  */
   int startup = TRUE;                 /* Indicates starting process EPRE     */

/* Initialize Variables for EPRE Algorithm ***/
   b_time = 0;
   e_time = 0;
   eleangle = -999;
   elev_ind = 0;
   max_ntilts=0;
   full_hys = 0;
/* ----------------------------------------------------------------------- */

   fprintf( stderr,"\nBegin CP013 Task 3: EPRE Algorithm\n" );

/* LABEL:REG_INIT  Algorithm Registration and Initialization Section
 * Initialize log_error services. */
   RPGC_init_log_services( argc, argv );

/* Register inputs and output */
   RPGC_reg_io ( argc, argv );

/* Register moments of interest */
   RPGC_reg_moments(REF_MOMENT);

/* Register adaptation data */
    rc = RPGC_reg_ade_callback( hydromet_acc_callback_fx,
                                &hyd_acc,
                                HYDROMET_ACC_DEA_NAME,
                                BEGIN_VOLUME );
    if( rc < 0 )
      RPGC_log_msg( GL_ERROR, "HYDROMET ACC: cannot register adaptation data callback function\n");

    rc = RPGC_reg_ade_callback( hydromet_adj_callback_fx,
                                &hyd_adj,
                                HYDROMET_ADJ_DEA_NAME,
                                BEGIN_VOLUME );
    if( rc < 0 )
      RPGC_log_msg( GL_ERROR, "HYDROMET ADJ: cannot register adaptation data callback function\n");

    rc = RPGC_reg_ade_callback( hydromet_prep_callback_fx,
                                &hyd_epre,
                                HYDROMET_PREP_DEA_NAME,
                                BEGIN_VOLUME );
    if( rc < 0 )
      RPGC_log_msg( GL_ERROR, "HYDROMET PREP: cannot register adaptation data callback function\n");

    rc = RPGC_reg_ade_callback( hydromet_rate_callback_fx,
                                &hyd_rate,
                                HYDROMET_RATE_DEA_NAME,
                                BEGIN_VOLUME );
    if( rc < 0 )
      RPGC_log_msg( GL_ERROR, "HYDROMET RATE: cannot register adaptation data callback function\n");

/* ORPG task initialization routine. Input parameters argc/argv are
 * not used in this algorithm */
   RPGC_task_init( VOLUME_BASED, argc, argv );

/* While loop that controls how long the task will execute. As long as
 * PROCESS remains TRUE, the task will continue. */

   while( algProcess )
   {
   /* Suspend until driving input available. */
    RPGC_wait_act(WAIT_DRIVING_INPUT);

   /* Initialize new volume scan */
    new_vol = 1;

   /* Open output buffer */
    outbuf = (char*)RPGC_get_outbuf_by_name("HYBRSCAN", LRGSIZ_HYBRID, &iostat);

    if (outbuf == NULL || iostat != NORMAL)
     {
       RPGC_log_msg( GL_INFO, "RPGC_get_outbuf HYBRSCAN (%d)\n",iostat);
       RPGC_abort_because( iostat );
       continue;
     }
   
   /* Copy Precip Status Mesg into output buffer */
    copy_precip_status_msg (epre_buf.HydroMesg);

   /* Copy adatpation data into output buffer, and initialize supplemental 
    * data portion to zero */
    copy_adapt_supl (epre_buf.HydroAdapt, epre_buf.HydroSupl);

    if (debugit) fprintf(stderr,"--> Successfully obtained OUTPUT BUFFER <--\n" );

    while (eleProcess)
    {

     /* Get Base Reflectivity Data Elevation  */
     elev_prod = (Compact_basedata_elev *)RPGC_get_inbuf_by_name("REFLDATA_ELEV",&iostat);

     if (iostat != NORMAL){
         RPGC_log_msg( GL_INFO, "RPGC_get_inbuf REFLDATA_ELEV (%d)\n",iostat);
         RPGC_cleanup_and_abort(iostat);
         break;
      }

     if(debugit) fprintf(stderr,"MESSAGE_TYPE = %d\n\n",elev_prod->type);

     /* Test to see if the required moment (reflectivity) is enabled */
     if (!(elev_prod->type & REF_ENABLED_BIT)) {
         fprintf(stderr,"Aborted for Disabled Moments\n");
         RPGC_rel_inbuf((void*)elev_prod);
         RPGC_abort_because(PROD_DISABLED_MOMENT);
         RPGC_log_msg( GL_INFO, "ERROR: Aborted for Disabled Moments\n");
         continue;
        }

     /* Make sure the reflectivity radials are NOT(!) mapped to the
        Doppler radials on this cut */

     if ((elev_prod->type & BYTE_MASK) & REF_INSERT_BIT) {
         RPGC_rel_inbuf((void*)elev_prod);
         if(TRUE) fprintf(stderr,"Skip the second scan of a split cut....\n");
         continue;
        }

     /* Build the reflectivity output product now */
     read_refl ((void *)elev_prod, &elev_flag, &elindxF, &cur_elev,
                &elev_angle, &volnumF, &cur_vcp, &radial_num,
                &start_Ztime, &start_Zdate, &end_Ztime, &end_Zdate, 
		azimuth_array, Zrefl, &reflstat, &bde_stat);

     cur_vol=RPGC_get_buffer_vol_num((void*)elev_prod);
     nvcp = RPGC_get_buffer_vcp_num((void*)elev_prod);
     elev_ind = RPGC_get_buffer_elev_index((void*)elev_prod);

     /* Determine the maximum tilt number for this VCP...used to stop
        EPRE processing at the end of the next to last tilt. */
     max_ntilts = RPGCS_get_last_elev_index(nvcp) - 1;
     eleangle = RPGCS_get_target_elev_ang(nvcp,elev_ind);

     if (debugit) {
        fprintf( stderr,"NEXT HIGHEST ELEVATION = %d\n",max_ntilts );
        fprintf( stderr,"ELE_IND= %d\tELEV_ANGLE = %d\telev_angle =%f\n",
                  elev_ind,eleangle,elev_angle);
       }

     /* Get AP/Clutter Likelihood Reflectivity data ====================*/
     recref = RPGC_get_inbuf_by_name("RECCLDIGREF",&iostat);
     if (iostat != NORMAL) {
         RPGC_log_msg( GL_INFO, "RPGC_get_inbuf RECCLDIGREF (%d)\n",iostat);
         RPGC_cleanup_and_abort(iostat);
         break;
       }

     /* Release REFLDATA_ELEV input buffer */
     RPGC_rel_inbuf((void*)elev_prod);

     /* Build the reflectivity output product now */
     read_clutterap ((char *)recref, &volnumR, &elindxR,
                     &spotblank, APclutter, &apstat);

     /* Release RECCLDIGREF input buffer */
     RPGC_rel_inbuf((void*)recref);


     /* Verify 2 inputs are the same volume scan and same elevation scan */
     if ((elindxF == elindxR) && (volnumF == volnumR) &&
        ((reflstat == 1) && (apstat == 1)))
       {
         if (new_vol) {
            /* Initialize Hybrid Scan flag */
            full_hys = 0;
            new_hys = 1;
            new_vol = 0;

            /* Get time at the begining of volume */
            b_time = get_elev_time (start_Zdate, start_Ztime);

           }/* end of new_vol */

         /* Check that inputs of different types are from the same volume scan
            and same elevation cut */
         if (full_hys == 0)
          {
            /* Run Enhanced Processing algorithm (EPRE) */
            full_hys = c_epre (&startup, &new_hys, cur_vcp, cur_elev,
                               max_ntilts, volnumF, b_time, eleangle,
                               elev_ind, radial_num, &rain_detected,
                               Zrefl, APclutter, azimuth_array,
                               epre_buf.HyScanZ, epre_buf.HyScanE);

            /* Complete the PPS processing when the Hybrid Scan is full or
             * volume is at the next to highest tilt. */
            if (full_hys || (elindxF == max_ntilts) || elev_flag)
              {
               /* Get ending date and time for Hybrid Scan */
               e_time = get_elev_time (end_Zdate, end_Ztime);

               /* Compute average date/time for Hybrid Scan */
               compute_avg_datetime (b_time, e_time, &ret_date, &ret_time);

               a3133c7.avgdate = ret_date;
               a3133c7.avgtime = ret_time;

               /* Only if it is detected raining, last_date_rain &
                  last_time_rain have been updated */

               if (a3133c7.rain_detec_flg == TRUE) {
                  a3133c7.last_date_rain = ret_date;
                  a3133c7.last_time_rain = ret_time;
                 }

               /* If no rain was detected this scan, set flag, else clear it */

               if (a3133c7.reset_stp_flg == FLAG_SET)
                  a3133c7.zerohybrd = FLAG_SET;
               else
                  a3133c7.zerohybrd = FLAG_CLEAR;

               /* Copy volume scan spot blank status */
               a3133c7.vol_sb = spotblank;

               /* Copy supplemental data to the output buffer */
               memcpy(epre_buf.HydroSupl, &a3133c7, sizeof(EPRE_supl_t));

               /* Copy the EPRE info and Hybrid Scan to the outbuf buffer */
               memcpy((char*)outbuf,&epre_buf,sizeof(EPRE_buf_t));

               /* Release and forward the Hybrid Scan output buffer to the
                * next algorithm. */
               RPGC_rel_outbuf ((void*) outbuf, FORWARD);

               /* If product successfully output, then abort the remaining
                  elevations of the volume scan. */
               RPGC_abort_remaining_volscan();
               break;

             } /* end of full_hys || cur_elev == max_ntilts */
          }/* end of full_hys == 0 */

        } /*end verify 2 inputs are the same volume and elevation scan */

      }/* end while(eleProcess) */

   } /* end while(algProcess) */

   fprintf( stderr,"\nEPRE Program Terminated\n" );
   return (0);

} /* END of  MAIN */
