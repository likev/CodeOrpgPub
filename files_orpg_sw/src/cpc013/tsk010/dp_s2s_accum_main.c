/*
 * RCS info
 * $Author:
 * $Locker:
 * $Date:
 * $Id:
 * $Revision:
 * $State:
 */

#include "dp_s2s_accum_func_prototypes.h"

/******************************************************************************
   Filename: dp_s2s_accum_main.c

   Description:
   ============
      The main function for the Dual-Pol Accumulation Algorithm.
   The algorithm receives QPE rate data as input and produces a
   scan-to-scan accumulation.

   Inputs: none

   Outputs: none

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Pham, Stein, Ward  Initial implementation
******************************************************************************/

int main( int argc, char* argv[] )
{
   char* inbuf_name  = "QPERATE";
   char* outbuf_name = "DP_S2S_ACCUM";

   int          i, j;                     /* counters                  */
   int          opstat = RPGC_NORMAL;     /* buffer status             */
   Rate_Buf_t   qperate_old;              /* last rate grid we've seen */
   Rate_Buf_t*  qperate_new = NULL;       /* new rate grid             */
   S2S_Accum_Buf_t* accum_buf = NULL;     /* accum buffer              */
   int          vsnum = 0;                /* volume number             */
   int          ret = 0;                  /* return value              */
   short        first_time = TRUE;        /* first time flag           */
   char         msg[200];                 /* stderr message            */

   unsigned int rate_size      = sizeof(Rate_Buf_t);
   unsigned int s2s_size       = sizeof(S2S_Accum_Buf_t);
   unsigned int dua_query_size = sizeof(dua_query_t);
   unsigned int qpe_adapt_size = sizeof(QPE_Adapt_t);
   unsigned int s2s_supl_size  = sizeof(S2S_Accum_Supl_t);

   /* Initialize log_error services. */

   RPGC_init_log_services( argc, argv );

   /* Specify inputs and outputs */

   RPGC_reg_io( argc, argv );

   /* Initialize this task */

   RPGC_task_init( VOLUME_BASED, argc, argv );

   /* Open the DUAUSERSEL.DAT data store. This data store is used
    * by DUA to place accum grids into its 24 hour database. */

   ret = open_s2s_accum_data_store( DUAUSERSEL );

   if(ret == FUNCTION_FAILED)
      RPGC_log_msg(GL_INFO, "CPC013/TSK010_Main: Error opening DUAUSERSEL.DAT\n");

   /* Open the DP_OLD_RATE.DAT data store. This data store saves
    * the old rate grid, in case dp_s2s_accum is interrupted then restarted.
    * The file is named in ~/cfg/data_attr_table */

   ret = open_s2s_rate_data_store( DP_OLD_RATE );

   if(ret == FUNCTION_FAILED)
      RPGC_log_msg(GL_INFO, "CPC013/TSK010_Main: Error opening DP_OLD_RATE.DAT\n");

   /* Register a Termination Handler. Code cribbed from:
    * ~/src/cpc001/tsk024/hci_agent.c
    *
    * 20080213 Chris Calvert says not to use termination
    * handlers. Keeping the code here in case we ever get
    * permission to turn it back on.
    *
    * if(ORPGTASK_reg_term_handler(dp_s2s_accum_terminate) < 0)
    * {
    *    RPGC_log_msg(GL_ERROR, "Could not register for termination signals");
    *    exit(GL_EXIT_FAILURE);
    * }
    */

   /* Print startup message right after RPGC calls */

   sprintf(msg, "BEGIN DP_S2S_ACCUM, CPC013/TSK010\n");
   RPGC_log_msg( GL_INFO, msg );
   if(DP_S2S_ACCUM_DEBUG)
      fprintf(stderr, msg);

   while( 1 ) /* wait until the input buffer is available */
   {
      RPGC_wait_act( WAIT_DRIVING_INPUT );

     /* Get the input buffer */

     qperate_new = (Rate_Buf_t*) RPGC_get_inbuf_by_name (inbuf_name, &opstat);

     if((opstat != RPGC_NORMAL) || (qperate_new == NULL))
     {
        sprintf(msg, "RPGC_get_inbuf_by_name %s failed %p, opstat %d\n",
                      inbuf_name,
                      (void*) qperate_new,
                      opstat);

        RPGC_log_msg(GL_INFO, msg);
        if(DP_S2S_ACCUM_DEBUG)
          fprintf(stderr, msg);

        if ( qperate_new != NULL )
        {
           RPGC_rel_inbuf ((void*) qperate_new);
           qperate_new = NULL;
        }

        RPGC_cleanup_and_abort(opstat);
        continue;
     }

     /* If we got here, we have an input buffer. Get the volume number */

     vsnum = RPGC_get_buffer_vol_num((void*) qperate_new);

     sprintf(msg, "---------- volume: %d ---------- \n", vsnum);
     RPGC_log_msg(GL_INFO, msg);
     if(DP_S2S_ACCUM_DEBUG)
        fprintf(stderr, msg);

     if(first_time) /* try to read the old rate grid from disk */
     {
        first_time = FALSE;

        /* Read the old rate grid from the DP_OLD_RATE.DAT data store */

        ret = read_s2s_rate_data_store(DP_OLD_RATE, OLD_RATE_ID, &qperate_old);

        if(ret != READ_OK) /* we have no old rate grid */
        {
           /* Save the new rate grid as the old one */

            memcpy(&qperate_old, qperate_new, rate_size);

            /* Write the new rate grid to the DP_OLD_RATE.DAT data store */

            ret = write_s2s_rate_data_store(DP_OLD_RATE, OLD_RATE_ID, &qperate_old);
            if(ret == WRITE_OK)
            {
               sprintf(msg, "Wrote new rate grid to DP_OLD_RATE\n");
               RPGC_log_msg(GL_INFO, msg);
               if(DP_S2S_ACCUM_DEBUG)
                  fprintf(stderr, msg);
            }

            /* Release the input buffer */

            if(qperate_new != NULL)
            {
               RPGC_rel_inbuf ((void*) qperate_new);
               qperate_new = NULL;
            }

            /* 20090728 Jim Ward added a call to RPGC_abort_because() at
             * Steve Smith's request because product generators are required
             * to either make a product or abort so an RPM can be sent to
             * the product requestor. */

            RPGC_abort_because(PGM_PROD_NOT_GENERATED);

            continue; /* go to next volume */

        } /* end old rate grid not read from disk */

     } /* end if first_time */

     /* If we got here, we have an old and a new rate grid.
      * Get an output buffer. */

     accum_buf = (S2S_Accum_Buf_t*)
                  RPGC_get_outbuf_by_name(outbuf_name,
                                          s2s_size,
                                          &opstat);

     if((opstat != RPGC_NORMAL) || (accum_buf == NULL))
     {
        sprintf(msg, "RPGC_get_outbuf_by_name %s failed %p, opstat %d\n",
                  outbuf_name,
                  (void*) accum_buf,
                  opstat);

        RPGC_log_msg(GL_INFO, msg);
        if(DP_S2S_ACCUM_DEBUG)
           fprintf(stderr, msg);

        /* Save the new rate grid as the old one */

        memcpy(&qperate_old, qperate_new, rate_size);

        /* Write the new rate grid to the DP_OLD_RATE.DAT data store */

        ret = write_s2s_rate_data_store(DP_OLD_RATE, OLD_RATE_ID, &qperate_old);

        if(ret == WRITE_OK)
        {
           sprintf(msg, "Wrote new rate grid to DP_OLD_RATE\n");
           RPGC_log_msg(GL_INFO, msg);
           if(DP_S2S_ACCUM_DEBUG)
              fprintf(stderr, msg);
        }

        if(qperate_new != NULL)
        {
           RPGC_rel_inbuf((void*) qperate_new);
           qperate_new = NULL;
        }

        if (accum_buf != NULL)
        {
           RPGC_rel_outbuf((void*) accum_buf, DESTROY);
           accum_buf = NULL;
        }

        RPGC_abort_because(opstat);

        continue;

     } /* end get_outbuf failed */

     /* If we got here, we have an old rate grid, a new rate grid,
      * and an output buffer. Initialize it. */

     memset(&(accum_buf->dua_query), 0, dua_query_size);
     memset(&(accum_buf->qpe_adapt), 0, qpe_adapt_size);
     memset(&(accum_buf->supl),      0, s2s_supl_size);

     for(i = 0; i < MAX_AZM; i++)
       for(j = 0; j < MAX_BINS; j++)
          accum_buf->accum_grid[i][j] = QPE_NODATA;

     /* Copy new rate adapt to accum adapt */

     memcpy(&(accum_buf->qpe_adapt),
            &(qperate_new->qpe_adapt),
            qpe_adapt_size);

     /* Copy the supplemental data. This is a little trickier than a straight
      * memcpy() because we pick fields from both the old/new rate grids.
      */

     copy_supplemental(&qperate_old, qperate_new, accum_buf);

     /* Compute an accumulation and release the output buffer.
      * There are 3 types of accums:
      *
      * 1. (Full data) - no missing period,
      * 2. (Incomplete data) - has a missing period in the middle,
      * 3. (No data) - NO_S2S_ACCUM = no scan-to-scan accumulation could be
      *                generated because the rate grid times are too far apart.
      *
      *    In case 2, we want to be sure and pass the accum grid to the
      *    DUA database, so the missing period flag is set correctly when
      *    a DUA query is done over a period that includes the accum.
      */

     ret = dp_compute_accum(&qperate_old, qperate_new, accum_buf);

     if(ret == NO_S2S_ACCUM) /* null accum */
     {
       accum_buf->supl.null_accum = NULL_REASON_1;
     }
     else /* got a non-null (good) accum */
     {
       accum_buf->supl.null_accum = FALSE;

       /* Write the accum grid to the DUAUSERSEL.DAT data store.
        * This data store is used by DUA to place accum grids into
        * its 24 hour database.
        *
        * 20071025 Ning Shen says when writing to use LB_ANY to get
        * a system assigned unique id instead of a user specified
        * msg id (1 to LB_MAX_ID). */

       ret = write_s2s_accum_data_store(DUAUSERSEL, LB_ANY, accum_buf);

       if(ret == WRITE_OK)
       {
          sprintf(msg, "Wrote accum_buf to DUAUSERSEL\n");
          RPGC_log_msg(GL_INFO, msg);
          if(DP_S2S_ACCUM_DEBUG)
             fprintf(stderr, msg);
       }

     } /* end got a non-null accum */

     /* The new grid is now the old grid */

     memcpy(&qperate_old, qperate_new, rate_size);

     /* Write the old rate grid to the DP_OLD_RATE.DAT data store
      * for re-read upon task restart */

     ret = write_s2s_rate_data_store(DP_OLD_RATE, OLD_RATE_ID, &qperate_old);

     if(ret == WRITE_OK)
     {
        sprintf(msg, "Wrote new rate grid to DP_OLD_RATE\n");
        RPGC_log_msg(GL_INFO, msg);
        if(DP_S2S_ACCUM_DEBUG)
           fprintf(stderr, msg);
     }

     /* Release the input buffer */

     if(qperate_new != NULL)
     {
        RPGC_rel_inbuf ((void*) qperate_new);
        qperate_new = NULL;
     }

     /* Release the output buffer. We always want to FORWARD
      * the accum grid even if there's a null accum, so we can pass
      * the meta information forward to the STA tab/DSA layer 2
      */

     if(accum_buf != NULL)
     {
        RPGC_rel_outbuf((void*) accum_buf, FORWARD);
        accum_buf = NULL;
     }

  } /* end while waiting for input buffer */

  return (0);

} /* end main() ============================================================ */

/******************************************************************************
   Filename: dp_s2s_accum_main.c

   Description:
   ============
   copy_supplemental() copies the supplemental data from rate buffers to an
   accum buffer.

   Inputs: Rate_Buf_t*  old   - one rate buffer to get data from
           Rate_Buf_t*  new   - another rate buffer to get data from
           S2S_Accum_Buf_t* accum - the accum buf to be filled

    For the prcp_begin_flg, we will use this logic:

    ----------------------------------------------------------------------------
     1st time? |  Old prcp_begin_flg | New prcp_begin_flg | Accum prcp_begin_flg
    ----------------------------------------------------------------------------
       Y       |               either one T               |           T
    -----------|---------------------|--------------------|---------------------
       Y       |          F          |         F          |           F
    -----------|---------------------|--------------------|---------------------
       N       |          F          |         T          |           T
    -----------|---------------------|--------------------|---------------------
       N       |          T          |         T          |           T, error
    -----------|---------------------|--------------------|---------------------

   The N, T, T case should never happen due to logic in QPE rate. If it does,
   print an error and continue.

   Outputs: none

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       James Ward         Initial implementation
*******************************************************************************/

int copy_supplemental(Rate_Buf_t* old, Rate_Buf_t* new,
                      S2S_Accum_Buf_t* accum)
{
   char  msg[200]; /* stderr message   */
   static short first_time = TRUE;

   /* Check for NULL pointers */

   if(pointer_is_NULL(old, "copy_supplemental", "old"))
      return(NULL_POINTER);

   if(pointer_is_NULL(new, "copy_supplemental", "new"))
      return(NULL_POINTER);

   if(pointer_is_NULL(accum, "copy_supplemental", "accum"))
      return(NULL_POINTER);

   /* Copy start/end date/times */

   accum->supl.begin_time = old->rate_supl.time;
   accum->supl.end_time   = new->rate_supl.time;

   /* Fill in some data from the new rate_supl. Note that QPERATE sets the
    * ST_Active flag, so we don't worry about the start/end of storms. */

   accum->supl.ST_active_flg      = new->rate_supl.ST_active_flg;
   accum->supl.pct_hybrate_filled = new->rate_supl.pct_hybrate_filled;
   accum->supl.highest_elev_used  = new->rate_supl.highest_elang;
   accum->supl.sum_area           = new->rate_supl.sum_area;
   accum->supl.vol_sb             = new->rate_supl.vol_sb;

   if (first_time == TRUE)
   {
      /* Check for precip. begin */

      if ( old->rate_supl.prcp_begin_flg || new->rate_supl.prcp_begin_flg )
         accum->supl.prcp_begin_flg = TRUE;
      else
         accum->supl.prcp_begin_flg = FALSE;

      first_time = FALSE;

   } /* end if first time */
   else if ( new->rate_supl.prcp_begin_flg == TRUE )
   {
      /* Not first time, but precip has begun */

      accum->supl.prcp_begin_flg = TRUE;

      if (old->rate_supl.prcp_begin_flg)
      {
         sprintf(msg, "copy_supplemental: %s is TRUE, %s is TRUE\n",
                      "old->rate_supl.prcp_begin_flg",
                      "new->rate_supl.prcp_begin_flg");

         RPGC_log_msg(GL_INFO, msg);
         if(DP_S2S_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

   } /* end not first time, but precip begin */
   else /* not first time and precip not begun */
      accum->supl.prcp_begin_flg = FALSE;

   /* Set times based on when precip detected */

   accum->supl.prcp_detected_flg = new->rate_supl.prcp_detected_flg;

   if ( (old->rate_supl.prcp_detected_flg == FALSE) &&
        (new->rate_supl.prcp_detected_flg == TRUE) )
   {
      /* The old rate_supl had no precip. and the new rate_supl has precip. *
       * so use the new rate_supl's precip. dates.                      */

      accum->supl.last_time_prcp  = new->rate_supl.time;
      accum->supl.start_time_prcp = new->rate_supl.time;
   }
   else /* use the old rate_supl's precip. dates */
   {
      accum->supl.last_time_prcp  = old->rate_supl.last_time_prcp;
      accum->supl.start_time_prcp = old->rate_supl.start_time_prcp;
   }

   return(FUNCTION_SUCCEEDED);

} /* end copy_supplemental() =========================== */

/******************************************************************************
    Filename: dp_s2s_accum_main.c

    Description:
    ============
      dp_s2s_accum_terminate() is the Termination Signal Handler.

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

int dp_s2s_accum_terminate( int signal, int flag )
{
   RPGC_log_msg( GL_INFO, "Termination Signal Handler Called\n" );

   return 0;

} /* end dp_s2s_accum_terminate() =================================== */
