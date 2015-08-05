/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 23:08:34 $
 * $Id: dp_elev_main.c,v 1.5 2009/10/27 23:08:34 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include "dp_elev_func_prototypes.h"

/******************************************************************************
   Filename: dp_elev_main.c

   Description:
   ============
   The main function for the Dual-Pol Elevation Algorithm.

   Inputs: DP_BASE_HC_AND_ML buffer, which contains the moments.
           See dp_elev_Consts.h for a list of them.

   Outputs: none

   Change History
   ==============
   DATE        VERSION    PROGRAMMERS        NOTES
   ----------  -------    -----------------  ---------------------------
   20071005     0000      Liu, Stein, Ward   Initial implementation
   20080115     0001      Ward               Move counters out of global
*****************************************************************************/

int main(int argc, char* argv[])
{
   char* inbuf_name  = "DP_BASE_HC_AND_ML";
   char* outbuf_name = "DP_MOMENTS_ELEV";

   int   opstat = RPGC_NORMAL;   /* buffer status                   */
   short status = 0;             /* radial status                   */
   int   vsnum, elnum;           /* volume and elevation numbers    */
   int   vcp;                    /* volume coverage pattern         */
   int   elev_angle_tenths;      /* elevation angle in 10ths        */
   char  msg[200];               /* stderr message                  */
   char  msg2[200];              /* stderr message, part 2          */
   int   num_out_of_range = 0;   /* number of radials outside 0-359 */
   int   num_duplicates   = 0;   /* number of duplicated radials    */
   int   ret              = 0;   /* callback return code            */
   dp_precip_adapt_t dp_adapt;   /* callback data                   */

   unsigned int compact_dp_size = sizeof(Compact_dp_basedata_elev);

   Base_data_header*         inbuf  = NULL; /* input buffer pointer  */
   Compact_dp_basedata_elev* outbuf = NULL; /* output buffer pointer */

   /* Initialize log_error services. */

   RPGC_init_log_services( argc, argv );

   /* Register inputs and outputs. */

   RPGC_reg_io( argc, argv );

   /* Register adaptation data callback. We only collect mode filter size. */

   ret = RPGC_reg_ade_callback(dp_precip_callback_fx,
                               &dp_adapt,
                               DP_PRECIP_ADAPT_DEA_NAME,
                               ADPT_UPDATE_BOV);
   if(ret < 0)
   {
      sprintf(msg, "Cannot register %s adaptation data callback function\n",
                   "DP_PRECIP_ADAPT");

      RPGC_log_msg(GL_ERROR, msg);
      if(DP_ELEV_PROD_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_ERROR, msg);
         if(DP_ELEV_PROD_DEBUG)
            fprintf(stderr, msg);
      }

      RPGC_abort_task();

   } /* end if ret < 0 */

   /* Initialize this task. */

   RPGC_task_init(ELEVATION_BASED, argc, argv);

   /* Register a Termination Handler. Code cribbed from:
    * ~/src/cpc001/tsk024/hci_agent.c
    *
    * 20080213 Chris Calvert says not to use termination
    * handlers. Keeping the code here in case we ever get
    * permission to turn it back on.
    *
    * if(ORPGTASK_reg_term_handler(dp_elev_prod_terminate) < 0)
    * {
    *    RPGC_log_msg(GL_ERROR, "Could not register for termination signals");
    *    exit(GL_EXIT_FAILURE);
    *
    * } -* end termination handler called *-
    */

   /* Print startup message right after RPGC calls */

   sprintf(msg, "BEGIN DP_ELEV_PROD, CPC004/TSK012\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_ELEV_PROD_DEBUG)
      fprintf(stderr, msg);

   /* Main processing loop. */

   while ( 1 ) /* suspend until driving input available. */
   {
      RPGC_wait_act(WAIT_DRIVING_INPUT);

      /* Get an output buffer to store our collection of radials. */

      outbuf = (Compact_dp_basedata_elev *)
                RPGC_get_outbuf_by_name(outbuf_name,
                                        compact_dp_size,
                                        &opstat);

      /* If unable to acquire the output buffer,
       * we have nowhere to collect our radials.
       * Abort processing and return to waiting for activation. */

      if((opstat != RPGC_NORMAL) || (outbuf == NULL))
      {
         sprintf(msg, "RPGC_get_outbuf_by_name %s failed %p, opstat %d\n",
                  outbuf_name,
                  (void*) outbuf,
                  opstat);

          RPGC_log_msg(GL_ERROR, msg);
          if(DP_ELEV_PROD_DEBUG)
             fprintf(stderr, msg);

          if(outbuf != NULL)
          {
             RPGC_rel_outbuf((void*) outbuf, DESTROY);
             outbuf = NULL;

             RPGC_abort_because(opstat);

             continue; /* waiting for input buffer */
          }

      } /* end if outbuf == NULL */

      /* Initialize the elevation */

      init_elevation(outbuf, &num_out_of_range, &num_duplicates);

      /* Collect all the radials of this elevation scan. */

      while(1)
      {
         /* Get the driving input, the HCA buffer */

         inbuf = (Base_data_header*) RPGC_get_inbuf_by_name(inbuf_name, &opstat);

         /* If acquisition of the input buffer fails,
          * release the input buffer, and wait for activation. */

         if((inbuf == NULL) || (opstat != RPGC_NORMAL))
         {
             sprintf(msg, "RPGC_get_inbuf_by_name %s failed %p, opstat %d\n",
                     inbuf_name,
                     (void*) inbuf,
                     opstat);

             RPGC_log_msg(GL_ERROR, msg);
             if(DP_ELEV_PROD_DEBUG)
                fprintf(stderr, msg);

             if (inbuf != NULL)
             {
                RPGC_rel_inbuf ((void*) inbuf);
                inbuf = NULL;
             }

             RPGC_cleanup_and_abort(opstat);

             break; /* while waiting for the next radial in elevation */

         } /* end didn't get input buffer */

         /* Check for Dual Pol data */

         else if((inbuf->msg_type & PREPROCESSED_DUALPOL_TYPE) == FALSE)
         {
            /* This is not dual pol data - dpprep should've cut off
             * our data stream before we get here. */

            sprintf(msg, "inbuf->msg_type %d & %s %d %s\n",
                          inbuf->msg_type,
                          "PREPROCESSED_DUALPOL_TYPE",
                          PREPROCESSED_DUALPOL_TYPE,
                          "== FALSE");

            /* Make exception to the "RPGC abort -> writes to GL_ERROR" 
             * rule because in the early stages of Dual Pol this may 
             * be a frequent occurance. */

            RPGC_log_msg(GL_INFO, msg);
            if(DP_ELEV_PROD_DEBUG)
               fprintf(stderr, msg);

            if(inbuf != NULL)
            {
               RPGC_rel_inbuf ((void*) inbuf);
               inbuf = NULL;
            }

            if(outbuf != NULL)
            {
               RPGC_rel_outbuf((void*) outbuf, DESTROY);
               outbuf = NULL;
            }

            RPGC_abort_because(PGM_DISABLED_MOMENT);

            break; /* while waiting for the next radial in elevation */
         }

         /* Get the radial status. According to CODE Volume 3,
          * Section II, Part A, the radial status flag is in the low byte.
          * The high byte can be set to indicate a bad radial. */

         status = inbuf->status & 0xF;

         vsnum             = RPGC_get_buffer_vol_num((void*) inbuf);
         elnum             = RPGC_get_buffer_elev_index((void*) inbuf);
         vcp               = RPGC_get_buffer_vcp_num((void*) inbuf);
         elev_angle_tenths = RPGCS_get_target_elev_ang(vcp, elnum);

         /* If we started a new elevation/volume, but still have radials left
          * to be written out, write them out and get a fresh output buffer */

         if ( (((outbuf->vol_ind > 0) && (outbuf->vol_ind  != vsnum))  ||
              ((outbuf->elev_ind > 0) && (outbuf->elev_ind != elnum))) &&
              (outbuf->num_radials != 0) )
         {
            /* We've got a new elevation, but we still have radials in the
	     * output buffer left over from the last scan, so we never got a
	     * proper GENDEL or GENDVOL. Write the output buffer and get a
	     * fresh one. */

            sprintf(msg,  "(outbuf->vol_ind %d != vsnum %d) OR",
                           outbuf->vol_ind, vsnum);
            sprintf(msg2, "%s (outbuf->elev_ind %d != elnum %d) AND",
                           msg, outbuf->elev_ind, elnum);
            sprintf(msg, "%s outbuf->num_radials = %d, releasing old "
		         " output buffer.\n",
                          msg2, outbuf->num_radials);

            RPGC_log_msg(GL_INFO, msg);
            if(DP_ELEV_PROD_DEBUG)
               fprintf(stderr, msg);

            RPGC_rel_outbuf((void*) outbuf, FORWARD);

            /* Get a new output buffer. */

            outbuf = (Compact_dp_basedata_elev *)
                      RPGC_get_outbuf_by_name(outbuf_name,
                                              compact_dp_size,
                                              &opstat);

            /* If unable to acquire the output buffer, we have nowhere to put
	     * our elevation. Abort processing and continue. */

            if(outbuf == NULL)
            {
               sprintf(msg, "RPGC_get_outbuf_by_name %s failed %p (2), opstat %d\n",
                        outbuf_name,
                        (void*) outbuf,
                        opstat);

               RPGC_log_msg(GL_ERROR, msg);
               if(DP_ELEV_PROD_DEBUG)
                  fprintf(stderr, msg);

               RPGC_abort_because(opstat);
               continue; /* waiting for the next radial */

            } /* end if outbuf == NULL */

            /* Initialize the elevation */

            init_elevation(outbuf, &num_out_of_range, &num_duplicates);

         } /* end if started a new elevation, but radials left
            * from previous elevation */

         /* If beginning a new elevation/volume, say so. GOODBEL/GOODBVOL
          * are set in the first radial in the elevation. */

         if((status == GOODBEL) || (status == GOODBVOL))
         {
            sprintf(msg, "---------- vol %d elev %d elev ang %2.1f vcp %d ----------\n",
                         vsnum, elnum, elev_angle_tenths / 10.0, vcp);
            RPGC_log_msg(GL_INFO, msg);
            if(DP_ELEV_PROD_DEBUG)
               fprintf(stderr, msg);

         } /* end start of elevation */

         /* Copy metadata. The mode filter will be applied in Add_radial().
          * Default Mode_filter_len: 9 */

         outbuf->type               = inbuf->msg_type;
         outbuf->vol_ind            = vsnum;
         outbuf->elev_ind           = elnum;
         outbuf->mode_filter_length = dp_adapt.Mode_filter_len;

         /* Add the radial to the elevation product. (our work horse) */

         Add_radial((char*) inbuf, outbuf, &num_out_of_range, &num_duplicates);

         /* Release the input buffer. */

         RPGC_rel_inbuf((void *) inbuf);

         /* If end of elevation/volume, release the output buffer,
          * and go back to waiting for activation. GENDEL/GENDVOL
          * are set in the last radial of the elevation. */

         if ((status == GENDEL) || (status == GENDVOL))
         {
           if((num_out_of_range > 0) || (num_duplicates > 0))
           {
              sprintf(msg, "%d radials processed. %s %d out of range, %d ",
                      outbuf->num_radials,
                      "radials ignored:", num_out_of_range,
                       num_duplicates);

              if(num_duplicates != 1)
                 strcat(msg, "duplicates\n");
              else
                 strcat(msg, "duplicate\n");

           } /* end had radials out of range or duplicates */
           else /* got exactly 360 radials */
           {
              sprintf(msg, "%d radials processed.\n", outbuf->num_radials);
           }

           RPGC_log_msg(GL_INFO, msg);
           if(DP_ELEV_PROD_DEBUG)
              fprintf(stderr, msg);

           RPGC_rel_outbuf((void*) outbuf, FORWARD);

           break; /* out of the get next radial while loop */

         } /* end if end of elevation */

      } /* end infinite while loop waiting for radials */

   } /* end infinite while loop waiting for input */

   return 0;

} /* end main() ========================================= */

/******************************************************************************
   Filename: dp_elev_main.c

   Description:
   ============
   save_radial() writes one radial to disk for later comparison.
   This is just a debugging routine.

   Inputs: Base_data_header* inbuf - the radial to save.

   Outputs: none

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       James Ward         Initial implementation
*****************************************************************************/

void save_radial(Base_data_header* inbuf)
{
  char  filename[200];
  FILE* fp;
  int   vsnum = 0;
  int   elnum = 0;
  short radnum = 0;
  int   inbuf_bytes = 0;

  vsnum = RPGC_get_buffer_vol_num((void*) inbuf);

  if ( vsnum < 0 )
  {
    if ( DP_ELEV_PROD_DEBUG )
       fprintf(stderr, "vsnum %d < 0, can't save radial\n", vsnum);
    return;
  }

  elnum = RPGC_get_buffer_elev_index((void*) inbuf);

  if ( elnum < 0 )
  {
     if ( DP_ELEV_PROD_DEBUG )
        fprintf(stderr, "elnum %d < 0, can't save radial\n", elnum);
    return;
  }

  /* 20071009 Jim Ward - Cham says that it's better to use inbuf->azi_num
   * than inbuf->azimuth. azi_num seems to start at 1. */

  radnum = inbuf->azi_num - 1;

  if ( radnum < 0 )
  {
     if ( DP_ELEV_PROD_DEBUG )
      fprintf(stderr, "radnum %d < 0, can't save radial\n", radnum);
     return;
  }

  sprintf(filename, "/home/wardj/tmp/radials/%d/%d/radial.%d.%d.%3.3d",
          vsnum, elnum, vsnum, elnum, radnum);

  inbuf_bytes = RPGC_get_inbuf_len((void*) inbuf);

  if ( (fp = fopen(filename, "w+")) != NULL )
  {
     fwrite((void*) inbuf, inbuf_bytes, 1, fp );

     fclose( fp );
  }
  else if ( DP_ELEV_PROD_DEBUG )
  {
     fprintf(stderr, "couldn't fopen() to write %d bytes to %s\n",
                      inbuf_bytes, filename);
  }

} /* end save_radial( ) ===================================== */

/******************************************************************************
   Filename: dp_elev_main.c

   Description:
   ============
   init_elevation() does start of elevation processing.

   Inputs: Compact_dp_basedata_elev* out - the array of radials
           int* num_out_of_range         - number of radials out of range
           int* num_duplicates           - number of duplicated radials

   Outputs: none

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   -----------   -------    -----------------  ----------------------
    5 Oct 2007    0000       James Ward         Initial implementation
   19 Mar 2009    0001       James Ward         Deleted init_bad_radial()
                                                to speed up processing
*****************************************************************************/

void init_elevation(Compact_dp_basedata_elev* out,
                    int* num_out_of_range, int* num_duplicates)
{
   unsigned int compact_dp_size = sizeof(Compact_dp_basedata_elev);

   /* Init everything to 0. This initializes the header to all 0s,
    * and the generic moments to the generic moment no data value
    * (unsigned char) 0. When a moment is read by qperate
    * get_moment_value(), the 0 will be converted to QPE_NODATA, a short.
    *
    * Note: For HCA this assumes a hydroclass of 0 indicates no data, which
    * is OK because HCA no longer considers 0, 1 as valid hydroclasses. */

   memset((void*) out, 0, compact_dp_size);

   *num_out_of_range = 0;
   *num_duplicates   = 0;

} /* end init_elevation() ================================== */

/******************************************************************************
    Filename: dp_elev_main.c

    Description:
    ============
      dp_elev_prod_terminate() is the Termination Signal Handler.

      Code cribbed from ~/src/cpc001/tsk024/hci_agent.c

    Inputs:
       int signal - the signal sent (SIGTERM = 15)
       int flag   - signal type (3)?

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----------  -------    ----------      -----------------------
    02/12/2008    0000     Ward            Initial implementation
******************************************************************************/

int dp_elev_prod_terminate( int signal, int flag )
{
   RPGC_log_msg(GL_INFO, "Termination Signal Handler Called\n");

   return 0;

} /* end dp_elev_prod_terminate() ========================= */
