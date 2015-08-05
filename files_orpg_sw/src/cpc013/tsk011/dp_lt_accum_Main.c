/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:29:59 $
 * $Id: dp_lt_accum_Main.c,v 1.8 2014/07/29 22:29:59 dberkowitz Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

/******************************************************************************
   Filename: dp_lt_accum_Main.c

   Description
   ===========
      This module is the main function for the Dual Pol Long Term Accumulation
   task.

  Input:   none
  Output:  none
  Returns: none
  Globals: none
  Notes:   All input and output for this function are provided through
           ORPG API services calls.

   Change History
   ==============
   DATE        VERSION    PROGRAMMER   NOTES
   -------     -------    ----------   -----
   10/2007       0000     Cham, Stein  Initial implementation
   03/2008       0001     Ward         Made long term buffers global
   12/2011       0002     Ward         CCR NA11-00373:
                                       Add a difference product reset button
                                       to HCI. Don't reset on a QPE reset
                                       button push.
   03/06/2014    0003     Murnan       Added Top of Hour computation 
                                       (CCR NA12-00223)
******************************************************************************/

/* Global variable for callbacks. According to Chris Calvert of the ROC,
 * it is necessary for them to be global. */

int restart_dp_accum = FALSE;
int restart_dp_diff  = FALSE;

/* 03282008 Ward - Making all our (large) long term buffers global
 * (on the heap) instead of local to main() (on the stack). This will
 * avoid stack corruption and valgrind can be used to monitor the task. */

LT_Accum_Buf_t   lt_accum_buf;      /* our static data            */
Circular_Queue_t hourly_circq;      /* hourly circular queue      */
Circular_Queue_t hourly_diff_circq; /* hourly diff circular queue */
Storm_Backup_t   storm_backup;      /* storm total backup         */
Storm_Backup_t   storm_diff_backup; /* storm diff backup          */

int main(int argc, char* argv[])
{
  char* inbuf_name  = "DP_S2S_ACCUM";
  char* outbuf_name = "DP_LT_ACCUM";

  S2S_Accum_Buf_t* s2s_accum_buf = NULL; /* our latest scan-to-scan */
  char* outbuf = NULL;                   /* ouput buffer            */
  int   opstat = RPGC_NORMAL;            /* buffer status           */
  int   ret = 0;                         /* RPGC return code        */
  short vsnum = 0;                       /* volume number           */
  char  msg[200];                        /* stderr message          */
  short first_time = TRUE;               /* first time flag         */

  unsigned int qpe_adapt_size = sizeof(QPE_Adapt_t);
  unsigned int lt_size        = sizeof(LT_Accum_Buf_t);

  /* Initialize log_error services. */

  RPGC_init_log_services( argc, argv );

  /* Register inputs and outputs. */

  RPGC_reg_io ( argc, argv );

  /* Initialize this task. */

  RPGC_task_init( VOLUME_BASED, argc, argv );

  /* Register a Termination Handler. Code cribbed from:
   * ~/src/cpc001/tsk024/hci_agent.c
   *
   * 20080213 Chris Calvert says not to use termination
   * handlers. Keeping the code here in case we ever get
   * permission to turn it back on.
   *
   * if(ORPGTASK_reg_term_handler(dp_lt_accum_terminate) < 0)
   * {
   *    RPGC_log_msg(GL_ERROR, "Could not register for termination signals");
   *    exit(GL_EXIT_FAILURE);
   * }
   */

  /* Register restart accum callback.
   * ORPGEVT_RESTART_LT_ACCUM is set in ~/include/orpgevt.h */

  ret = RPGC_reg_for_external_event(ORPGEVT_RESTART_LT_ACCUM,
                                    restart_dp_accum_cb,
                                    ORPGEVT_RESTART_LT_ACCUM);
  if(ret < 0)
  {
     sprintf(msg,
             "ORPGEVT_RESTART_LT_ACCUM register failed, ret = %d",
             ret);

     RPGC_log_msg(GL_ERROR, msg);
     if(DP_LT_ACCUM_DEBUG)
        fprintf(stderr, msg);

     exit(GL_EXIT_FAILURE);
  }

  /* Register restart diff callback.
   * ORPGEVT_RESTART_LT_DIFF is set in ~/include/orpgevt.h */

  ret = RPGC_reg_for_external_event(ORPGEVT_RESTART_LT_DIFF,
                                    restart_dp_diff_cb,
                                    ORPGEVT_RESTART_LT_DIFF);
  if(ret < 0)
  {
     sprintf(msg,
             "ORPGEVT_RESTART_LT_DIFF register failed, ret = %d",
             ret);

     RPGC_log_msg(GL_ERROR, msg);
     if(DP_LT_ACCUM_DEBUG)
        fprintf(stderr, msg);

     exit(GL_EXIT_FAILURE);
  }

  /* Print startup message (after all RPGC calls) */

  sprintf(msg, "BEGIN DP_LT_ACCUM, CPC013/TSK011\n");
  RPGC_log_msg(GL_INFO, msg);
  if(DP_LT_ACCUM_DEBUG)
    fprintf(stderr, msg);

  /* Open our 4 linear buffers. */

  if(open_lt_accum_buffers() == FUNCTION_FAILED)
  {
     sprintf(msg, "Could not open all 4 linear buffers, exiting.\n");
     RPGC_log_msg(GL_INFO, msg);
     if(DP_LT_ACCUM_DEBUG)
       fprintf(stderr, msg);
     exit(GL_EXIT_FAILURE);
  }

  /* The main loop, which will never end. */

  while ( 1 )
  {
    RPGC_wait_act( WAIT_DRIVING_INPUT );

    /* If we got here, we got the driving input.
     * Input buffer is from dp_s2s_accum task. */

    s2s_accum_buf = (S2S_Accum_Buf_t*)
                     RPGC_get_inbuf_by_name(inbuf_name, &opstat);

    if(s2s_accum_buf == NULL || opstat != RPGC_NORMAL)
    {
       sprintf(msg, "RPGC_get_inbuf_by_name %s failed %p, opstat %d\n",
               inbuf_name,
               (void*) s2s_accum_buf,
               opstat);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
          fprintf(stderr, msg);

       if(s2s_accum_buf != NULL)
       {
          RPGC_rel_inbuf ((void*) s2s_accum_buf);
          s2s_accum_buf = NULL;
       }

       RPGC_cleanup_and_abort(opstat);
       continue;
    }

    /* Get current volume scan number */

    vsnum = RPGC_get_buffer_vol_num( s2s_accum_buf );

    sprintf(msg, "---------- volume: %d ---------- \n", vsnum);
    RPGC_log_msg(GL_INFO, msg);
    if ( DP_LT_ACCUM_DEBUG )
       fprintf(stderr, msg);

    if(first_time)
    {
       /* Initialize everything. We have to wait until we get
        * our first s2s_accum_buf to get the Max_vols_per_hour */

       init_lt_accum(&hourly_circq,
                     &storm_backup,
                     &lt_accum_buf,
                     s2s_accum_buf);

       init_lt_diff(&hourly_diff_circq,
                    &storm_diff_backup,
                    &lt_accum_buf,
                    s2s_accum_buf);

       /* See if we can restore our static storage from disk.
        * If we can't restore from disk, or the restore only
        * works partially (it restores the one-hour, but not
        * the storm), then restore_lt_accum() will call init_lt_accum()
        * again, so init_lt_accum() might be called twice during
        * a run.
        *
        * We have wait until we get our first s2s_accum_buf to
        * get the storm restart threshold. */

       restore_lt_accum(&hourly_circq,
                        &storm_backup,
                        &lt_accum_buf,
                        s2s_accum_buf);

       restore_lt_diff(&hourly_diff_circq,
                       &storm_diff_backup,
                       &lt_accum_buf,
                       s2s_accum_buf);

       first_time = FALSE;
    } /* if(first_time) */
    else /* two flags to check */
    {
       if(restart_dp_accum == TRUE)
       {
          sprintf(msg, "Got %s event, initializing DP accumulations\n",
                  "ORPGEVT_RESTART_LT_ACCUM");
          RPGC_log_msg(GL_INFO, msg);
          if(DP_LT_ACCUM_DEBUG)
             fprintf(stderr, msg);

          init_lt_accum(&hourly_circq,
                        &storm_backup,
                        &lt_accum_buf,
                        s2s_accum_buf);

          restart_dp_accum = FALSE;
       }

       if(restart_dp_diff == TRUE)
       {
          sprintf(msg, "Got %s event, initializing DP-PPS differences\n",
                  "ORPGEVT_RESTART_LT_DIFF");
          RPGC_log_msg(GL_INFO, msg);
          if(DP_LT_ACCUM_DEBUG)
             fprintf(stderr, msg);

          init_lt_diff(&hourly_diff_circq,
                       &storm_diff_backup,
                       &lt_accum_buf,
                       s2s_accum_buf);

          restart_dp_diff = FALSE;
       }
    }

    /* Copy QPE adaptable parameters and supplemental data.
     * Do this after a restore/restart, because the
     * restore/restart might init our buffers. */

    memcpy(&(lt_accum_buf.qpe_adapt),
           &(s2s_accum_buf->qpe_adapt),
           qpe_adapt_size);

    ret = copy_supplemental(&(s2s_accum_buf->supl), &(lt_accum_buf.supl));

    if(ret != FUNCTION_SUCCEEDED)
    {
       sprintf(msg, "copy_supplemental() failed, ret: %d\n", ret);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
          fprintf(stderr, msg);
    }


    /* Compute Hourly to represent the Top of Hour - uses DUAUSERSEL data and
     * SQL like requests - added for Top of Hour (TOH)*/

    ret = compute_Top_of_Hour(s2s_accum_buf, &lt_accum_buf);  /* added for TOH */

    if(ret != FUNCTION_SUCCEEDED)
    {
       sprintf(msg, "compute_Top_of_Hour() failed, ret: %d\n", ret);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
          fprintf(stderr, msg);
    }

    /* Compute Hourly */

    ret = compute_hourly(s2s_accum_buf, &lt_accum_buf, &hourly_circq);

    if(ret != FUNCTION_SUCCEEDED)
    {
       sprintf(msg, "compute_hourly() failed, ret: %d\n", ret);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
          fprintf(stderr, msg);
    }

    /* Compute Storm */

    ret = compute_storm(s2s_accum_buf, &lt_accum_buf, &storm_backup);

    if(ret != FUNCTION_SUCCEEDED)
    {
       sprintf(msg, "compute_storm() failed, ret: %d\n", ret);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
          fprintf(stderr, msg);
    }

    /* Compute Difference */

    ret = compute_diff(s2s_accum_buf, &lt_accum_buf, &hourly_diff_circq,
                 &storm_diff_backup);

    if(ret != FUNCTION_SUCCEEDED)
    {
       sprintf(msg, "compute_diff() failed, ret: %d\n", ret);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
          fprintf(stderr, msg);
    }


    /* We no longer need the s2s_accum_buf input buffer, so release it */

    RPGC_rel_inbuf((void *) s2s_accum_buf);

    /* For testing purposes - adjust i,j to fprintf the bin you want
     *
     * int i = 356;
     * int j = 150;
     *
     * fprintf(stderr, "%d [%d][%d] One_Hr_biased %d, One_Hr_unbiased %d\n",
     *         vsnum,
     *         i, j,
     *         lt_accum_buf.One_Hr_biased[i][j],
     *         lt_accum_buf.One_Hr_unbiased[i][j]);
     * fprintf(stderr, "%d [%d][%d] Storm_Total %d, One_Hr_diff %d,
     *         Storm_Total_diff %d\n",
     *         vsnum,
     *         i, j,
     *         lt_accum_buf.Storm_Total[i][j],
     *         lt_accum_buf.One_Hr_diff[i][j],
     *         lt_accum_buf.Storm_Total_diff[i][j]);
     */

    /* Backup our static storage to the data stores */

    backup_lt_accum(&hourly_circq,
                    &hourly_diff_circq,
                    &storm_backup,
                    &storm_diff_backup);

    /* Get an output buffer */

    outbuf = (char*) RPGC_get_outbuf_by_name(outbuf_name,
                                              lt_size,
                                              &opstat);

    if((outbuf == NULL) || (opstat != RPGC_NORMAL))
    {
        sprintf(msg, "RPGC_get_outbuf_by_name %s failed %p, opstat %d\n",
                outbuf_name,
                (void*) outbuf,
                opstat);

        RPGC_log_msg(GL_INFO, msg);
       if(DP_LT_ACCUM_DEBUG)
           fprintf(stderr, msg);

       RPGC_rel_outbuf((void*) outbuf, DESTROY);
       RPGC_abort_because(opstat);
    }
    else
    {
       memcpy((char*) outbuf, &lt_accum_buf, lt_size);
       RPGC_rel_outbuf((void*) outbuf, FORWARD);
    }

  } /* end while (1) */

  return 0;

} /* end main() ============================================ */

/******************************************************************************
    Filename: dp_lt_accum_Main.c

    Description:
    ============
      dp_lt_accum_terminate() is the Termination Signal Handler. Currently
      unused.

      Code cribbed from ~/src/cpc001/tsk024/hci_agent.c

    Inputs:
       int signal - the signal sent (SIGTERM = 15)
       int flag   - signal type (3)?

    Currently uncalled.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----------  -------    ----------      -----------------------
    02/12/2008    0000     Ward            Initial implementation
******************************************************************************/

/* int dp_lt_accum_terminate( int signal, int flag )
 * {
 *   RPGC_log_msg(GL_INFO, "Termination Signal Handler Called\n" );
 *
 *   return 0;
 *
 *} -* end dp_lt_accum_terminate() =================================== */

/*****************************************************************************
   Filename: dp_lt_accum_Main.c

   Description
   ===========
   restart_dp_accum_cb() is the Dual Pol long term accum callback function

   Input: int fx_parm - the event that caused the call.

   Output: none

   As per Chris Calvert guidance, the callback just sets a flag and exits.
   To test, try something like this in an unrelated task:

   if(vsnum % 3 == 0)
   {
      EN_post(ORPGEVT_RESTART_LT_ACCUM, "Hello", strlen("Hello") + 1, 0);
      RPGC_log_msg(GL_INFO, "ORPGEVT_RESTART_LT_ACCUM event posted");
   }

   Change History
   ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    29 Oct 2007    0000       Ward               Initial implementation
    07 Dec 2011    0001       Ward               CCR NA11-00373:
                                                 Don't reset on a QPE reset
                                                 button push.
******************************************************************************/

void restart_dp_accum_cb(int fx_parm)
{
   restart_dp_accum = TRUE;

} /* end restart_dp_accum_cb() =================================== */

/*****************************************************************************
   Filename: dp_lt_accum_Main.c

   Description
   ===========
   restart_dp_diff_cb() is the Dual Pol difference callback function

   Input: int fx_parm - the event that caused the call.

   Output: none

   As per Chris Calvert guidance, the callback just sets a flag and exits.
   To test, try something like this in an unrelated task:

   if(vsnum % 3 == 0)
   {
      EN_post(ORPGEVT_RESTART_LT_DIFF, "Hello", strlen("Hello") + 1, 0);
      RPGC_log_msg(GL_INFO, "ORPGEVT_RESTART_LT_DIFF event posted");
   }

   Change History
   ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    07 Dec 2011    0000       Ward               CCR NA11-00373:
                                                 Reset diff on a DIFF reset
                                                 button push.
******************************************************************************/

void restart_dp_diff_cb(int fx_parm)
{
   restart_dp_diff = TRUE;

} /* end restart_dp_diff_cb() =================================== */

