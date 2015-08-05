/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:29:59 $
 * $Id: dp_lt_accum_init.c,v 1.6 2014/07/29 22:29:59 dberkowitz Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

time_t start_time_storm_diff; /* last time storm diff was initialized */

/***********************************************************************
   Filename: dp_lt_accum_init.c

   Description
   ===========
      init_lt_accum() initializes the lt_accum data stores.

   Input: Circular_Queue_t* hourly_circq  - hourly circular queue
          Storm_Backup_t*   storm_backup  - storm backup
          LT_Accum_Buf_t*   lt_accum_buf  - our static data
          S2S_Accum_Buf_t*  s2s_accum_buf - latest scan-to-scan

   Output: All data stores initialized

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER    NOTES
   ----------  -------    ----------    -------------------------------
   12/19/2007   0000       Ward         Initial version
   12/08/2011   0001       Ward         CCR NA11-00373:
                                        split init_lt_diff() out from
                                        init_lt_accum()
************************************************************************/

int init_lt_accum(Circular_Queue_t* hourly_circq,
                  Storm_Backup_t*   storm_backup,
                  LT_Accum_Buf_t*   lt_accum_buf,
                  S2S_Accum_Buf_t*  s2s_accum_buf)
{
   int  max_queue = 0; /* maximum accum grids tracked */
   char msg[200];      /* stderr message              */

   static unsigned int qpe_adapt_size = sizeof(QPE_Adapt_t);
   static unsigned int lt_supl_size   = sizeof(LT_Accum_Supl_t);

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning init_lt_accum()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_circq, "init_lt_accum", "hourly_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_backup, "init_lt_accum", "storm_backup"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "init_lt_accum", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "init_lt_accum", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* Initialize all of our metadata. */

   memset(&(lt_accum_buf->qpe_adapt), 0, qpe_adapt_size);
   memset(&(lt_accum_buf->supl),      0, lt_supl_size);

   /* Initialize our data stores. We initialize them separately just
    * in case one of them requires a future custom initialization.
    *
    * Default Max_vols_per_hour: 30
    */

   max_queue = s2s_accum_buf->qpe_adapt.dp_adapt.Max_vols_per_hour;

   init_hourly(lt_accum_buf, hourly_circq, max_queue);

   init_storm(lt_accum_buf, storm_backup);

   sprintf(msg, "Initialized LT accum\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, msg);

   return(FUNCTION_SUCCEEDED);

} /* end init_lt_accum() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_init.c

   Description
   ===========
      init_lt_diff() initializes the lt_diff data stores.

   Input: Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
          Storm_Backup_t    storm_diff_backup - storm diff backup
          LT_Accum_Buf_t*   lt_accum_buf      - our static data
          S2S_Accum_Buf_t*  s2s_accum_buf     - latest scan-to-scan

   Output: All data stores initialized

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER    NOTES
   ----------  -------    ----------    --------------------------------
   12/19/2007   0000       Ward         Initial version
   12/08/2011   0001       Ward         CCR NA11-00373:
                                        split init_lt_diff() out from
                                        init_lt_accum()
************************************************************************/

int init_lt_diff(Circular_Queue_t* hourly_diff_circq,
                 Storm_Backup_t*   storm_diff_backup,
                 LT_Accum_Buf_t*   lt_accum_buf,
                 S2S_Accum_Buf_t*  s2s_accum_buf)
{
   int  max_queue = 0; /* maximum accum grids tracked */
   char msg[200];      /* stderr message              */

   /* 12/08/2011 Ward CCR NA11-00373 - This is already done
    *                                  by init_lt_accum()
    * static unsigned int qpe_adapt_size = sizeof(QPE_Adapt_t);
    * static unsigned int lt_supl_size   = sizeof(LT_Accum_Supl_t);
    */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning init_lt_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_diff_circq, "init_lt_diff", "hourly_diff_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_diff_backup, "init_lt_diff", "storm_diff_backup"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "init_lt_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(s2s_accum_buf, "init_lt_diff", "s2s_accum_buf"))
      return(NULL_POINTER);

   /* Initialize all of our metadata.
    *
    * 12/08/2011 Ward CCR NA11-00373 - This is already done
    *                                  by init_lt_accum()
    *
    * memset(&(lt_accum_buf->qpe_adapt), 0, qpe_adapt_size);
    * memset(&(lt_accum_buf->supl),      0, lt_supl_size);
    */

   /* Initialize our data stores. We initialize them separately just
    * in case one of them requires a future custom initialization.
    *
    * Default Max_vols_per_hour: 30
    */

   max_queue = s2s_accum_buf->qpe_adapt.dp_adapt.Max_vols_per_hour;

   init_hourly_diff(lt_accum_buf, hourly_diff_circq, max_queue);

   init_storm_diff(s2s_accum_buf, lt_accum_buf, storm_diff_backup);

   sprintf(msg, "Initialized LT diff\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, msg);

   return(FUNCTION_SUCCEEDED);

} /* end init_lt_diff() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_init.c

   Description
   ===========
      init_hourly() - initializes the hourly component.

   Input: LT_Accum_Buf_t*   lt_accum_buf - our static data
          Circular_Queue_t* hourly_circq - hourly circular queue
          int               max_queue    - maximum allowed in the queue

   Output: hourly_circq, One_Hr_biased, and One_Hr_unbiased initialized

   Called by: init_lt_accum()

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
   03/21/2014  0001       BerkowitzMurnan    Top of Hour initialized
************************************************************************/

int init_hourly(LT_Accum_Buf_t* lt_accum_buf,
                Circular_Queue_t* hourly_circq,
                int max_queue)
{
   int  i, j;     /* counters       */
   char msg[200]; /* stderr message */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning init_hourly()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(lt_accum_buf, "init_hourly", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(hourly_circq, "init_hourly", "hourly_circq"))
      return(NULL_POINTER);

   /* Init our static data */

   for(i = 0; i < MAX_AZM; i++)
   {
     for(j = 0; j < MAX_BINS; j++)
     {
        lt_accum_buf->TOH_unbiased[i][j]    = QPE_NODATA;  /* added for TOH */ 
        lt_accum_buf->One_Hr_biased[i][j]   = QPE_NODATA;
        lt_accum_buf->One_Hr_unbiased[i][j] = QPE_NODATA;
     }
   }

   /* Init the hourly circular queue. */

   if(CQ_Initialize (hourly_circq, DP_HRLY_ACCUM, max_queue) != CQ_SUCCESS)
   {
      sprintf(msg, "%s %s\n",
                   "init_hourly:",
                   "CQ_Initialize() DP_HRLY_ACCUM != CQ_SUCCESS!!!");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   /* We don't need to init the DP_HRLY_ACCUM data store that
    * tracks the hourly_circq. It will either be read in on
    * a restore and hourly_circq will be updated to match
    * it, or it will be overwritten by the first backup. */

   return(FUNCTION_SUCCEEDED);

} /* end init_hourly() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_init.c

   Description
   ===========
      init_hourly_diff() - initializes the hourly diff component.

   Input: LT_Accum_Buf_t*   lt_accum_buf      - our static data
          Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
          int               max_queue         - maximum allowed in the queue

   Output: hourly_diff_circq and One_Hr_diff initialized

   Called by: init_lt_diff()

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int init_hourly_diff(LT_Accum_Buf_t* lt_accum_buf,
                     Circular_Queue_t* hourly_diff_circq,
                     int max_queue)
{
   int  i, j;     /* counters       */
   char msg[200]; /* stderr message */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning init_hourly_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(lt_accum_buf, "init_hourly_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(hourly_diff_circq, "init_hourly_diff", "hourly_diff_circq"))
      return(NULL_POINTER);

   /* Init our static data */

   for(i = 0; i < MAX_AZM; i++)
   {
     for(j = 0; j < MAX_BINS; j++)
     {
        lt_accum_buf->One_Hr_diff[i][j] = QPE_NODATA;
     }
   }

   /* Init the hourly diff circular queue. */

   if(CQ_Initialize (hourly_diff_circq, DP_DIFF_ACCUM, max_queue) !=
                     CQ_SUCCESS)
   {
      sprintf(msg, "%s %s\n",
                   "init_hourly_diff:",
                   "CQ_Initialize() DP_DIFF_ACCUM != CQ_SUCCESS!!!");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   /* We don't need to init the DP_DIFF_ACCUM data store that
    * tracks the hourly_diff_circq. It will either be read in on
    * a restore and hourly_diff_circq will be updated to match
    * it, or it will be overwritten by the first backup. */

   return(FUNCTION_SUCCEEDED);

} /* end init_hourly_diff() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_init.c

   Description
   ===========
      init_storm() - initializes the storm components.

   Input: LT_Accum_Buf_t* lt_accum_buf - our static data
          Storm_Backup_t* storm_backup - our disk data store

   Output: storm_backup and Storm_Total initialized

   Called by: init_lt_accum()

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int init_storm(LT_Accum_Buf_t* lt_accum_buf,
               Storm_Backup_t* storm_backup)
{
   int  i, j; /* counters */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning init_storm()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(lt_accum_buf, "init_storm", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_backup, "init_storm", "storm_backup"))
      return(NULL_POINTER);

   /* Init our static and backup data */

   for(i = 0; i < MAX_AZM; i++)
   {
     for(j = 0; j < MAX_BINS; j++)
     {
        lt_accum_buf->Storm_Total[i][j] = QPE_NODATA;
        storm_backup->StormTotal[i][j]  = QPE_NODATA;
     }
   }

   storm_backup->storm_begtime  = 0L;
   storm_backup->storm_endtime  = 0L;
   storm_backup->missing_period = FALSE;
   storm_backup->num_bufs       = 0;

   return(FUNCTION_SUCCEEDED);

} /* end init_storm() ============================================ */

/***********************************************************************
   Filename: dp_lt_accum_init.c

   Description
   ===========
      init_storm_diff() - initializes the storm diff components.

   Input: LT_Accum_Buf_t* lt_accum_buf      - our static data
          Storm_Backup_t* storm_diff_backup - our disk data store

   Output: storm_diff_backup and Storm_Total_diff initialized

   Called by: init_lt_diff()

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER   NOTES
   ----------  -------    ----------   ----------------------
   12/19/2007   0000       Ward        Initial version
   12/08/2011   0001       Ward        CCR NA11-00373:
                                       Initialize storm diff
                                       start time
************************************************************************/

int init_storm_diff(S2S_Accum_Buf_t* s2s_accum_buf,
                    LT_Accum_Buf_t*  lt_accum_buf,
                    Storm_Backup_t*  storm_diff_backup)
{
   int  i, j; /* counters */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning init_storm_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(lt_accum_buf, "init_storm_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_diff_backup, "init_storm_diff", "storm_diff_backup"))
      return(NULL_POINTER);

   /* Init our static and backup data */

   for(i = 0; i < MAX_AZM; i++)
   {
     for(j = 0; j < MAX_BINS; j++)
     {
        lt_accum_buf->Storm_Total_diff[i][j] = QPE_NODATA;
        storm_diff_backup->StormTotal[i][j]  = QPE_NODATA;
     }
   }

   storm_diff_backup->storm_begtime  = 0L;
   storm_diff_backup->storm_endtime  = 0L;
   storm_diff_backup->missing_period = FALSE;
   storm_diff_backup->num_bufs       = 0;

   start_time_storm_diff = s2s_accum_buf->supl.end_time;

   return(FUNCTION_SUCCEEDED);

} /* end init_storm_diff() ============================================ */
