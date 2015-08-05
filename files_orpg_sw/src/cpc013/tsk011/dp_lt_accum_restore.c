/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/10 20:42:20 $
 * $Id: dp_lt_accum_restore.c,v 1.6 2012/01/10 20:42:20 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      restore_lt_accum() restores the lt_accum from data store. If this fails,
      it initializes.

   Input: Circular_Queue_t* hourly_circq  - hourly circular queue
          Storm_Backup_t*   storm_backup  - storm backup
          LT_Accum_Buf_t*   lt_accum_buf  - our static data
          S2S_Accum_Buf_t*  s2s_accum_buf - scan-to-scan accum buffer
                                            that triggered our restore
   Output: All data stores restored

   Called by: LT accum main()

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------    -----------------------------
   12/19/2007   0000       Ward         Initial version
   12/08/2011   0001       Ward         CCR NA11-00373:
                                        split restore_lt_diff() out from
                                        restore_lt_accum()
************************************************************************/

int restore_lt_accum(Circular_Queue_t* hourly_circq,
                     Storm_Backup_t*   storm_backup,
                     LT_Accum_Buf_t*   lt_accum_buf,
                     S2S_Accum_Buf_t*  s2s_accum_buf)
{
   char  msg[200];                   /* stderr message */
   int restore_OK = FUNCTION_FAILED; /* FUNCTION_SUCCEEDED ->
                                      * restored 4 data stores */
   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning restore_lt_accum()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_circq, "restore_lt_accum", "hourly_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_backup, "restore_lt_accum", "storm_backup"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "restore_lt_accum", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "restore_lt_accum", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* We assume the restore is OK until proven otherwise.    *
    * Unlike backup, if one restore fails, we abort the rest *
    * and reinitialize.                                      */

   restore_OK = restore_hourly(hourly_circq,
                               lt_accum_buf,
                               s2s_accum_buf);

   if(restore_OK == FUNCTION_SUCCEEDED)
   {
      restore_OK = restore_storm(storm_backup,
                                 lt_accum_buf,
                                 s2s_accum_buf);
   }

   if(restore_OK == FUNCTION_SUCCEEDED)
   {
      sprintf(msg, "Restore successful\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else /* failed to get a buffer, or a NULL pointer */
   {
      sprintf(msg, "Restore failed, initializing LT accum\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      /* Restoration failed for 1 of the 4 linear buffers, *
       * so initialize all of them.                        */

      init_lt_accum(hourly_circq,
                    storm_backup,
                    lt_accum_buf,
                    s2s_accum_buf);
   }

   return(FUNCTION_SUCCEEDED);

} /* end restore_lt_accum() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      restore_lt_diff() restores the lt_diff from data store. If this fails,
      it initializes.

   Input: Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
          Storm_Backup_t    storm_diff_backup - storm diff backup
          LT_Accum_Buf_t*   lt_accum_buf      - our static data
          S2S_Accum_Buf_t*  s2s_accum_buf     - scan-to-scan accum buffer
                                                that triggered our restore
   Output: All data stores restored

   Called by: LT accum main()

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------    -----------------------------
   12/19/2007   0000       Ward         Initial version
   12/08/2011   0001       Ward         CCR NA11-00373:
                                        split restore_lt_diff() out from
                                        restore_lt_accum()
************************************************************************/

int restore_lt_diff(Circular_Queue_t* hourly_diff_circq,
                    Storm_Backup_t*   storm_diff_backup,
                    LT_Accum_Buf_t*   lt_accum_buf,
                    S2S_Accum_Buf_t*  s2s_accum_buf)
{
   char  msg[200];                   /* stderr message */
   int restore_OK = FUNCTION_FAILED; /* FUNCTION_SUCCEEDED ->
                                      * restored 4 data stores */
   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning restore_lt_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_diff_circq, "restore_lt_diff", "hourly_diff_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_diff_backup, "restore_lt_diff", "storm_diff_backup"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "restore_lt_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "restore_lt_diff", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* We assume the restore is OK until proven otherwise.    *
    * Unlike backup, if one restore fails, we abort the rest *
    * and reinitialize.                                      */

   restore_OK = restore_hourly_diff(hourly_diff_circq,
                                    lt_accum_buf,
                                    s2s_accum_buf);

   if(restore_OK == FUNCTION_SUCCEEDED)
   {
      restore_OK = restore_storm_diff(storm_diff_backup,
                                      lt_accum_buf,
                                      s2s_accum_buf);
   }

   if(restore_OK == FUNCTION_SUCCEEDED)
   {
      sprintf(msg, "Restore successful\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else /* failed to get a buffer, or a NULL pointer */
   {
      sprintf(msg, "Restore failed, initializing LT diff\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      /* Restoration failed for 1 of the 4 linear buffers, *
       * so initialize all of them.                        */

      init_lt_diff(hourly_diff_circq,
                   storm_diff_backup,
                   lt_accum_buf,
                   s2s_accum_buf);

   }

   return(FUNCTION_SUCCEEDED);

} /* end restore_lt_diff() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      restore_hourly() restores the hourly from data store.

   Input: Circular_Queue_t* hourly_circq  - hourly circular queue
          LT_Accum_Buf_t*   lt_accum_buf  - our static data
          S2S_Accum_Buf_t*  s2s_accum_buf - scan-to-scan accum buffer

   Output: Static data store restored

   Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

   Called by: restore_lt_accum()

   DP_HRLY_BACKUP is set in ~/include/orpgdat.h

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int restore_hourly(Circular_Queue_t* hourly_circq,
                   LT_Accum_Buf_t*   lt_accum_buf,
                   S2S_Accum_Buf_t*  s2s_accum_buf)
{
   int              i;
   int              ret = READ_FAILED;      /* return value                   */
   int              max_grid = 0;           /* maximum value of grid (inches) */
   int              bytes_read = 0;         /* number of bytes read           */
   char*            buffer = NULL;          /* pointer to disk buffer         */
   S2S_Accum_Buf_t  temp_s2s_accum;         /* temporary accum buffer         */
   short            missing_period = FALSE; /* TRUE -> a period is missing    */
   char             msg[200];               /* stderr message                 */
   long             time_diff = 0L;         /* time difference                */
   int              max_queue = 0;          /* max in queue                   */

   static unsigned int circq_size = sizeof(Circular_Queue_t);

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning restore_hourly()\n");

   /* Check for NULL pointers */

   if(pointer_is_NULL(hourly_circq, "restore_hourly", "hourly_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "restore_hourly", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "restore_hourly", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* Restore the hourly circular queue metadata.
    * LB_ALLOC_BUF reads the entire message.
    * NOTE: The hourly and the hourly difference are both in the same
    * linear buffer.
    */

   bytes_read = RPGC_data_access_read(DP_HRLY_BACKUP,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) HOURLY_ID);
   if(bytes_read == circq_size)
   {
      memcpy(hourly_circq, buffer, bytes_read);

      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "Restored HOURLY_ID from DP_HRLY_BACKUP");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else
   {
      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "%s %s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "restore_hourly:",
              "Failed to restore HOURLY_ID from DP_HRLY_BACKUP.",
              "Is it empty?",
               DP_HRLY_BACKUP, HOURLY_ID, bytes_read);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }

   /* Default Max_vols_per_hour: 30 */

   max_queue = s2s_accum_buf->qpe_adapt.dp_adapt.Max_vols_per_hour;

   i = hourly_circq->first;

   if (i == CQ_EMPTY) /* queue is empty, init */
   {
      init_hourly(lt_accum_buf, hourly_circq, max_queue);
      return(FUNCTION_SUCCEEDED);
   }

   time_diff = s2s_accum_buf->supl.end_time -
               hourly_circq->end_time[hourly_circq->last];

   /* If queue is too old, init */

   if((time_diff <= 0) || (time_diff > SECS_PER_HOUR))
   {
      init_hourly(lt_accum_buf, hourly_circq, max_queue);
      return(FUNCTION_SUCCEEDED);
   }

   /* If we got here, the queue is not empty, and the time is OK.
    * Restore the accum bufs.
    *
    * Default max_hourly_acc: 800 mm
    *
    * max_grid is in thousandths of inches, the same as the
    * One_Hr_biased/One_Hr_unbiased.
    */

   max_grid = s2s_accum_buf->qpe_adapt.accum_adapt.max_hourly_acc
              * MM_TO_IN * 1000;
   do
   {
      /* Note: data store rec_ids start at 1. */

      ret = read_s2s_accum_data_store(DP_HRLY_ACCUM,
                                      i + 1,
                                      &temp_s2s_accum);
      if(ret == READ_OK)
      {
         /* Note: We don't have to worry about the accum
          * buf being too old as CQ_Trim_To_Hour() will
          * trim us back to an hour later. */

         if (temp_s2s_accum.qpe_adapt.adj_adapt.bias_flag == TRUE)
         {
            add_biased_short_to_int(lt_accum_buf->One_Hr_biased,
                                    temp_s2s_accum.accum_grid,
                                    temp_s2s_accum.qpe_adapt.bias_info.bias,
                                    max_grid);
         }
         else
         {
            add_unbiased_short_to_int(lt_accum_buf->One_Hr_biased,
                                      temp_s2s_accum.accum_grid,
                                      max_grid);
         }

         add_unbiased_short_to_int(lt_accum_buf->One_Hr_unbiased,
                                   temp_s2s_accum.accum_grid,
                                   max_grid);
      }
      else
      {
         sprintf(msg, "%s %s %s, data_id %d, rec_id %d\n",
                 "restore_hourly:",
                 "Failed to read from DP_HRLY_ACCUM.",
                 "Is it empty?",
                  DP_HRLY_ACCUM, i);
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);

         return(FUNCTION_FAILED);
      }

      i = (i + 1) % hourly_circq->max_queue;

   } while (i != hourly_circq->last);


   lt_accum_buf->supl.hrly_begtime =
                      hourly_circq->begin_time[hourly_circq->first];
   lt_accum_buf->supl.hrly_endtime =
                      hourly_circq->end_time[hourly_circq->last];

   missing_period = CQ_Get_Missing_Period(hourly_circq);

   lt_accum_buf->supl.missing_period_One_Hr_biased   = missing_period;
   lt_accum_buf->supl.missing_period_One_Hr_unbiased = missing_period;

   return(FUNCTION_SUCCEEDED);

} /* end restore_hourly() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      restore_hourly_diff() restores the hourly diff from data store.

   Input: Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
          LT_Accum_Buf_t*   lt_accum_buf      - our static data
          S2S_Accum_Buf_t*  s2s_accum_buf     - scan-to-scan accum buffer

   Output: Static data store restored

   Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

   Called by: restore_lt_diff()

   DP_HRLY_BACKUP is set in ~/include/orpgdat.h

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int restore_hourly_diff(Circular_Queue_t* hourly_diff_circq,
                        LT_Accum_Buf_t*   lt_accum_buf,
                        S2S_Accum_Buf_t*  s2s_accum_buf)
{
   int              i;
   int              ret = READ_FAILED;      /* return value                */
   int              bytes_read = 0;         /* number of bytes read        */
   char*            buffer = NULL;          /* pointer to disk buffer      */
   S2S_Accum_Buf_t  temp_s2s_accum;         /* temporary accum buffer      */
   short            missing_period = FALSE; /* TRUE -> a period is missing */
   char             msg[200];               /* stderr message              */
   long             time_diff = 0L;         /* time difference             */
   int              max_queue = 0;          /* max in queue                */

   static unsigned int circq_size = sizeof(Circular_Queue_t);

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning restore_hourly_diff()\n");

   /* Check for NULL pointers */

   if(pointer_is_NULL(hourly_diff_circq, "restore_hourly_diff", "hourly_diff_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "restore_hourly_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "restore_hourly_diff", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* Restore the hourly circular queue metadata.
    * LB_ALLOC_BUF reads the entire message.
    * NOTE: The hourly and the hourly difference are both in the same
    * linear buffer.
    */

   bytes_read = RPGC_data_access_read(DP_HRLY_BACKUP,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) HOURLY_DIFF_ID);
   if(bytes_read == circq_size)
   {
      memcpy(hourly_diff_circq, buffer, bytes_read);

      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "Restored HOURLY_DIFF_ID from DP_HRLY_BACKUP");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else
   {
      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "%s %s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "restore_hourly_diff:",
              "Failed to restore HOURLY_DIFF_ID from DP_HRLY_BACKUP.",
              "Is it empty?",
               DP_HRLY_BACKUP, HOURLY_DIFF_ID, bytes_read);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }

   /* Default Max_vols_per_hour: 30 */

   max_queue = s2s_accum_buf->qpe_adapt.dp_adapt.Max_vols_per_hour;

   i = hourly_diff_circq->first;

   if(i == CQ_EMPTY) /* queue is empty, init */
   {
      init_hourly_diff(lt_accum_buf, hourly_diff_circq, max_queue);
      return(FUNCTION_SUCCEEDED);
   }

   time_diff = s2s_accum_buf->supl.end_time -
               hourly_diff_circq->end_time[hourly_diff_circq->last];

   /* If queue is too old, init */

   if((time_diff <= 0) || (time_diff > SECS_PER_HOUR))
   {
      init_hourly_diff(lt_accum_buf, hourly_diff_circq, max_queue);
      return(FUNCTION_SUCCEEDED);
   }

   /* If we got here, the queue is not empty,
    * and the time is OK. Restore the accum bufs.
    */

   do
   {  /* loop over all diff accum bufs */
      /* Note: data store rec_ids start at 1, but i is an array index. */

      ret = read_s2s_accum_data_store(DP_DIFF_ACCUM,
                                      i + 1,
                                      &temp_s2s_accum);
      if(ret == READ_OK)
      {
         /* Update our running hourly diff. Unlike the hourly accumulation,
          * there is no max_hourly_acc threshold for a difference grid.
          *
          * We don't have to worry about the accum buf being too old
          * as CQ_Trim_To_Hour() will trim us back to an hour later.
          *
          * The difference grid is always unbiased. */

         add_unbiased_short_to_int(lt_accum_buf->One_Hr_diff,
                                   temp_s2s_accum.accum_grid,
                                   INT_MAX);
      }
      else
      {
         sprintf(msg, "%s %s %s, data_id %d, rec_id %d\n",
                 "restore_hourly_diff:",
                 "Failed to read from DP_DIFF_ACCUM.",
                 "Is it empty?",
                  DP_DIFF_ACCUM, i);
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);

         return(FUNCTION_FAILED);
      }

      i = (i + 1) %  hourly_diff_circq->max_queue;

   } while (i != hourly_diff_circq->last);
   /* end loop over all diff accum bufs */

   lt_accum_buf->supl.hrlydiff_begtime =
     hourly_diff_circq->begin_time[hourly_diff_circq->first];
   lt_accum_buf->supl.hrlydiff_endtime =
     hourly_diff_circq->end_time[hourly_diff_circq->last];

   missing_period = CQ_Get_Missing_Period(hourly_diff_circq);

   lt_accum_buf->supl.missing_period_One_hr_diff = missing_period;

   return(FUNCTION_SUCCEEDED);

} /* end restore_hourly_diff() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      restore_storm() restores the storm from data store.

   Input: Storm_Backup_t*  storm_backup  - storm backup
          LT_Accum_Buf_t*  lt_accum_buf  - our static data
          S2S_Accum_Buf_t* s2s_accum_buf - scan-to-scan accum buffer

   Output: Static data store restored

   Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

   Called by: restore_lt_accum()

   DP_STORM_BACKUP is set in ~/include/orpgdat.h

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int restore_storm(Storm_Backup_t*  storm_backup,
                  LT_Accum_Buf_t*  lt_accum_buf,
                  S2S_Accum_Buf_t* s2s_accum_buf)
{
   int   bytes_read = 0; /* number of bytes read         */
   char* buffer = NULL;  /* pointer to disk buffer       */
   char  msg[200];       /* stderr message               */
   long  time_diff;      /* time difference              */
   int   restart_secs;   /* restart threshold in seconds */

   static unsigned int storm_size = sizeof(Storm_Backup_t);

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning restore_storm()\n");

   /* Check for NULL pointers */

   if(pointer_is_NULL(storm_backup, "restore_storm", "storm_backup"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "restore_storm", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "restore_storm", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* Restore the hourly circular queue metadata.
    * LB_ALLOC_BUF reads the entire message.
    * NOTE: The storm total and the storm-total difference are both in the same
    * linear buffer.
    */

   bytes_read = RPGC_data_access_read(DP_STORM_BACKUP,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) STORM_ID);
   if(bytes_read == storm_size)
   {
      memcpy(storm_backup, buffer, bytes_read);

      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "Restored STORM_ID from DP_STORM_BACKUP");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else
   {
      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "%s %s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "restore_storm:",
              "Failed to restore STORM_ID from DP_STORM_BACKUP.",
              "Is it empty?",
               DP_STORM_BACKUP, STORM_ID, bytes_read);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }

   /* Copy Storm to static
    *
    * Default restart_time: 60 mins
    */

   restart_secs = s2s_accum_buf->qpe_adapt.accum_adapt.restart_time *
                  SECS_PER_MIN;

   time_diff = s2s_accum_buf->supl.end_time - storm_backup->storm_endtime;

   if ((storm_backup->num_bufs <= 0) ||
       (time_diff <= 0)              ||
       (time_diff > restart_secs))
   {
      init_storm(lt_accum_buf, storm_backup);
   }
   else /* backup is not empty */
   {
      memcpy(lt_accum_buf->Storm_Total,
             storm_backup->StormTotal,
             INT_AZM_BINS);

      lt_accum_buf->supl.stmtot_begtime = storm_backup->storm_begtime;
      lt_accum_buf->supl.stmtot_endtime = storm_backup->storm_endtime;

      lt_accum_buf->supl.missing_period_Storm_Total = storm_backup->missing_period;
   }

   return(FUNCTION_SUCCEEDED);

} /* end restore_storm() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      restore_storm_diff() restores the storm diff from data store.

   Input: Storm_Backup_t   storm_diff_backup - storm diff backup
          LT_Accum_Buf_t*  lt_accum_buf      - our static data
          S2S_Accum_Buf_t* s2s_accum_buf     - scan-to-scan accum buffer

   Called by: restore_lt_diff()

   Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

   Called by:

   DP_STORM_BACKUP is set in ~/include/orpgdat.h

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int restore_storm_diff(Storm_Backup_t*  storm_diff_backup,
                       LT_Accum_Buf_t*  lt_accum_buf,
                       S2S_Accum_Buf_t* s2s_accum_buf)
{
   int   bytes_read = 0; /* number of bytes read         */
   char* buffer = NULL;  /* pointer to disk buffer       */
   char  msg[200];       /* stderr message               */
   long  time_diff;      /* time difference              */
   int   restart_secs;   /* restart threshold in seconds */

   static unsigned int storm_size = sizeof(Storm_Backup_t);

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning restore_storm_diff()\n");

   /* Check for NULL pointers */

   if(pointer_is_NULL(storm_diff_backup, "restore_storm_diff", "storm_diff_backup"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "restore_storm_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "restore_storm_diff", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* Restore the hourly circular queue metadata.
    * LB_ALLOC_BUF reads the entire message.
    * NOTE: The storm total and the storm-total difference are both in the same
    * linear buffer.
    */

   bytes_read = RPGC_data_access_read(DP_STORM_BACKUP,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) STORM_DIFF_ID);
   if(bytes_read == storm_size)
   {
      memcpy(storm_diff_backup, buffer, bytes_read);

      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "Restored STORM_DIFF_ID from DP_STORM_BACKUP");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else
   {
      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "%s %s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "restore_storm_diff:",
              "Failed to restore STORM_DIFF_ID from DP_STORM_BACKUP.",
              "Is it empty?",
               DP_STORM_BACKUP, STORM_DIFF_ID, bytes_read);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }

   /* Copy Storm Diff to static
    *
    * Default restart_time: 60 mins
    */

   restart_secs = s2s_accum_buf->qpe_adapt.accum_adapt.restart_time * SECS_PER_MIN;

   time_diff = s2s_accum_buf->supl.end_time - storm_diff_backup->storm_endtime;

   if ((storm_diff_backup->num_bufs <= 0) ||
       (time_diff <= 0)                   ||
       (time_diff > restart_secs))
   {
      init_storm_diff(s2s_accum_buf, lt_accum_buf, storm_diff_backup);
   }
   else /* backup is not empty */
   {
      memcpy(lt_accum_buf->Storm_Total_diff,
             storm_diff_backup->StormTotal,
             INT_AZM_BINS);

      lt_accum_buf->supl.stmdiff_begtime = storm_diff_backup->storm_begtime;
      lt_accum_buf->supl.stmdiff_endtime = storm_diff_backup->storm_endtime;

      lt_accum_buf->supl.missing_period_Storm_Total_diff =
                    storm_diff_backup->missing_period;
   }

   return(FUNCTION_SUCCEEDED);

} /* end restore_storm_diff() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_restore.c

   Description
   ===========
      open_lt_accum_buffers() opens all the lt_accum buffers we need.

   Input: void

   Output: Buffers opened

   Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1)

   Called by: LT accum main()

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   20080318    0000       Ward               Initial version
************************************************************************/

int open_lt_accum_buffers(void)
{
   int   ret = 0;  /* return value   */
   char  msg[200]; /* stderr message */

   /* Open DP_HRLY_BACKUP, the hourly circular queue, for reading/writing. */

   ret = RPGC_data_access_open(DP_HRLY_BACKUP, LB_READ | LB_WRITE);

   if(ret < 0)
   {
      sprintf(msg, "%s %s, data_id %d, ret %d\n",
              "open_lt_accum_buffers:",
              "Failed to open DP_HRLY_BACKUP",
               DP_HRLY_BACKUP, ret);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }
   else
   {
      sprintf(msg, "Opened DP_HRLY_BACKUP\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   /* Open DP_HRLY_ACCUM, the scan-to-scan accums, for reading/writing. */

   ret = RPGC_data_access_open(DP_HRLY_ACCUM, LB_READ | LB_WRITE);

   if(ret < 0)
   {
      sprintf(msg, "%s %s, data_id %d, ret %d\n",
              "open_lt_accum_buffers:",
              "Failed to open ",
               DP_HRLY_ACCUM, ret);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }
   else
   {
      sprintf(msg, "Opened DP_HRLY_ACCUM\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   /* Open DP_DIFF_ACCUM, the scan-to-scan diff accums, for reading/writing. */

   ret = RPGC_data_access_open(DP_DIFF_ACCUM, LB_READ | LB_WRITE);

   if(ret < 0)
   {
      sprintf(msg, "%s %s, data_id %d, ret %d\n",
              "open_lt_accum_buffers:",
              "Failed to open ",
               DP_DIFF_ACCUM, ret);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }
   else
   {
      sprintf(msg, "Opened DP_DIFF_ACCUM\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   /* Open DP_STORM_BACKUP for reading/writing. */

   ret = RPGC_data_access_open(DP_STORM_BACKUP, LB_READ | LB_WRITE);

   if(ret < 0)
   {
      sprintf(msg, "%s %s, data_id %d, ret %d\n",
              "open_lt_accum_buffers:",
              "Failed to open DP_STORM_BACKUP",
               DP_STORM_BACKUP, ret);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FUNCTION_FAILED);
   }
   else
   {
      sprintf(msg, "Opened DP_STORM_BACKUP\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   return(FUNCTION_SUCCEEDED);

} /* end open_lt_accum_buffers() ============================================ */
