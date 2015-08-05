/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/30 19:42:36 $
 * $Id: dp_lt_accum_Compute_TOH.c,v 1.1 2014/07/30 19:42:36 dberkowitz Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


#include "dp_Consts.h"              /* MINS_PER_HOUR  */
#include "qperate_types.h"          /* added for test */

#include "dp_s2s_accum_types.h"     /* added for test */
#include "dp_lt_accum_types.h"      /* added for test */

#include "dp_lt_accum_func_prototypes.h"
#include <limits.h>                 /* INT_MAX        */
#include "dp_lt_accum_Consts.h"     /* INTERP_FORWARD */
#include "qperate_Consts.h"         /* QPE_NODATA     */


/*****************************************************************************
   Filename: dp_lt_accum_Compute_TOH.c

   Description
   ===========
   compute_Top_of_Hour() computes the grids for the Top of Hour products:

        DAA (170) Digital Accumulation Array (always unbiased)

    When a new accumulation grid is a check is made to see if a Top of Hour
    exists within the time boundaries of the accumulation grid. If so the
    appropiate HH:00:00 is calculated in seconds for the end time and an
    one hour of time is subtracted to generate the begin time is seconds.

    These top of hour times are used as a database query of all available
    accumulation periods stored in DUAUSERSEL scan to scan data store.  The
    result produces a query result with all records meeting our SQL request.

    The records are summed into a final accumulation grid that represents the
    one hour window we are intereste in. The exception would be for those
    record the have either the Top of Hour end time and begin time. These records
    will require interpolation.

    In some events, not enough time with accumulations occurred during the hour to 
    justify a Top of Hour (TOH) accumulation.  If this is the case, a variable
    null_TOH_unbiased is set equal to TRUE.  This variable will be critical when
    it is time to insert the TOH_unbiased accumulation grid in place of the 
    normal accumulation within the DAA product.  If null_TOH_unbiased is set 
    to FALSE, the TOH_unbiased accumulation grid in used.  

    Note: A null product means that the product could have been
    generated, but was not, due to a condition. It does not indicate
    some sort of system failure. When we make a null product, we don't
    initialize the grid because it is not displayed.

   Inputs: S2S_Accum_Buf_t*  s2s_accum_buf - latest scan to scan accum
           LT_Accum_Buf_t*   lt_accum_buf  - our saved data
           
   Outputs: lt_accum_buf.TOH_unbiased  - with latest s2s accum added

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------

    14 Feb 2014     0000       Murnan/Berkowitz  Initial implementation
                                                 (CCR NA-00264)


******************************************************************************/

int compute_Top_of_Hour(S2S_Accum_Buf_t*  s2s_accum_buf,
                   LT_Accum_Buf_t*   lt_accum_buf)
{
  int    max_hourly     = 0;      /* maximum hourly value in inches         */
  char   msg[200];                /* stderr msg                             */
  int    ret;                     /* return code                            */

  int year, month, day, hour, minute, second;
  int i,j;                        /* counters                               */
  int zero_minute = 0;            /* TOH minute value                       */
  int zero_second = 0;            /* TOH second value                       */
  time_t hour_end_time = 0L;      /* TOH end time                           */
  time_t hour_begin_time = 0L;    /* TOH begin time                         */
  time_t thres_min_accum_time_in_secs; /* (default 54)mins * 60 sec/min */

  void *query_result = NULL;      /* holds the database query results       */
  int num_record = 0;             /* number of records in the query results */


  /* Check for NULL pointers. */

  if(pointer_is_NULL(s2s_accum_buf, "compute_TOH", "s2s_accum_buf"))
     return(NULL_POINTER);

  if(pointer_is_NULL(lt_accum_buf, "compute_TOH", "lt_accum_buf"))
     return(NULL_POINTER);


  /* Determine if the current volume scan to volume scan (i.e., scan to scan or s2s)
   * accumulation includes the current Top of Hour time (e.g., 10:00:00Z in seconds 
   * since 1/1/1970).
   */

  ret = RPGCS_unix_time_to_ymdhms(s2s_accum_buf->supl.end_time, &year, &month, &day,
                                  &hour, &minute, &second);
  if(ret != 0)
  {
     sprintf(msg, "%s %s %d\n",
        "compute_Top_of_Hour:", "RPGCS_unix_time_to_ymdhms error, ret:", ret);
     RPGC_log_msg(GL_INFO,msg);
     if(DP_LT_ACCUM_DEBUG)
        fprintf(stderr, msg);
  }

      /* The resulting HH:MM:SS are reduced to current Top of Hour (HH:00:00), then
       * converted back from YMDHMS to seconds.
       */
 
 
  ret = RPGCS_ymdhms_to_unix_time(&hour_end_time, year, month, day,
                                  hour, zero_minute, zero_second);
  if(ret != 0)
  {
     sprintf(msg, "%s %s %d\n",
        "compute_Top_of_Hour:", "RPGCS_ymdhms_to_unix_time error, ret:", ret);
     RPGC_log_msg(GL_INFO,msg);
     if(DP_LT_ACCUM_DEBUG)
        fprintf(stderr, msg);
  }

      /* Check if the current volume scan to volume scan accumulation contains the
       * current Top of Hour.
       */

  if(hour_end_time > s2s_accum_buf->supl.begin_time)
  {
      /* This volume scan to scan accumulation DOES include the currrent
       * Top of Hour and an hourly accumulation will be calculated and inserted
       * into the DAA product in place of its normal accumulation.
       */

     hour_begin_time = hour_end_time - SECS_PER_HOUR;
     lt_accum_buf->supl.TOH_begtime  = hour_begin_time;
     lt_accum_buf->supl.TOH_endtime = hour_end_time;
     lt_accum_buf->supl.total_TOH_missing_time = 0L;
     lt_accum_buf->supl.total_TOH_accum_time = 0L;
     lt_accum_buf->supl.null_TOH_unbiased = FALSE;
     lt_accum_buf->supl.missing_period_TOH_unbiased = FALSE;  /* generate a product */

  }
  else
  {
      /* This volume scan to volume scan accumulation DOES NOT include the current
       * Top of Hour and no hourly accumulation will be calculated.
       */
 
     lt_accum_buf->supl.TOH_begtime = 0L;
     lt_accum_buf->supl.TOH_endtime = 0L;
     lt_accum_buf->supl.total_TOH_missing_time = 0L;
     lt_accum_buf->supl.total_TOH_accum_time = 0L;
     lt_accum_buf->supl.null_TOH_unbiased = NULL_REASON_5;
     lt_accum_buf->supl.missing_period_TOH_unbiased = FALSE;

     for (i = 0; i < MAX_AZM; i++)
     {
        for (j = 0; j < MAX_BINS; j++)
        {
           lt_accum_buf->TOH_unbiased[i][j] = QPE_NODATA;
        }
     }

     return(FUNCTION_SUCCEEDED);
  }

  /* Default max_hourly_acc: 800 mm
   *
   * max_hourly is in thousandths of inches, and is the same for all the
   * hourly accumulation grids (TOH_unbiased/One_Hr_biased/One_Hr_unbiased).
   */


  max_hourly = s2s_accum_buf->qpe_adapt.accum_adapt.max_hourly_acc *
               MM_TO_IN * 1000;

  /* query the database */

  ret = query_DB_TOH(DUAUSERSEL, hour_begin_time, hour_end_time, &query_result,
                      &num_record);

  if(ret <= 0) /* bad or empty query - make a null product */
  {
     lt_accum_buf->supl.TOH_begtime = 0L;
     lt_accum_buf->supl.TOH_endtime = 0L;
     lt_accum_buf->supl.total_TOH_missing_time = 0L;
     lt_accum_buf->supl.total_TOH_accum_time = 0L;
     lt_accum_buf->supl.null_TOH_unbiased = NULL_REASON_6;
     lt_accum_buf->supl.missing_period_TOH_unbiased = FALSE;

     for (i = 0; i < MAX_AZM; i++)
     {
        for (j = 0; j < MAX_BINS; j++)
        {
           lt_accum_buf->TOH_unbiased[i][j] = QPE_NODATA;
        }
     }
     return(FUNCTION_SUCCEEDED);    
  }

  /* Compute the accumulation during TOH time period */

  ret = compute_TOH_accum_grid(query_result, num_record, lt_accum_buf);

  if(ret != 0) /* make a null product */
  {
     /* no accumulation generated since there was a problem pulling records
      * from the DUAUSERSEL data base (holds volume scan to volume scan
      * accumulations).
      */
     lt_accum_buf->supl.TOH_begtime = 0L;
     lt_accum_buf->supl.TOH_endtime = 0L;
     lt_accum_buf->supl.total_TOH_missing_time = 0L;
     lt_accum_buf->supl.total_TOH_accum_time = 0L;
     lt_accum_buf->supl.null_TOH_unbiased = NULL_REASON_6;
     lt_accum_buf->supl.missing_period_TOH_unbiased = FALSE;

     for(i = 0; i < MAX_AZM; i++)
     {
        for(j = 0; j < MAX_BINS; j++)
        {
           lt_accum_buf->TOH_unbiased[i][j] = QPE_NODATA;
        }
     }
  }
  else
  {
    lt_accum_buf->supl.TOH_min_time_period =
           s2s_accum_buf->qpe_adapt.accum_adapt.min_time_period;
    thres_min_accum_time_in_secs =
           s2s_accum_buf->qpe_adapt.accum_adapt.min_time_period *
           SECS_PER_MIN;
    lt_accum_buf->supl.total_TOH_accum_time = 
           SECS_PER_HOUR - lt_accum_buf->supl.total_TOH_missing_time;
              

    /* When too much missing time occurs in the Top of Hour accumulation, we
     * reset the null_TOH_unbiased a number greater than zero.  Here the
     * null_TOH_unbiased = 7 meaning there was too much time missing during
     * the one hour accumulation.  Currently, the TOH is the only one hour
     * accumulation that has this criteria.
     */ 

    if(lt_accum_buf->supl.total_TOH_accum_time < thres_min_accum_time_in_secs)
    {

       lt_accum_buf->supl.null_TOH_unbiased = NULL_REASON_7;


       for(i = 0; i < MAX_AZM; i++)
       {
          for(j = 0; j < MAX_BINS; j++)
          {
             lt_accum_buf->TOH_unbiased[i][j] = QPE_NODATA;
          }
       }
    }
  }
 
  return(FUNCTION_SUCCEEDED);

} /* end compute_Top of Hour() =================================== */


/******************************************************************************
   Function name: query_DB_TOH()

   Description:
   ============
      It queries the scan-to-scan accumulation database based on
      hourly start time and end time.

   Inputs:
      databse ID, start time, end time

   Outputs:	
      query result, number of records

   Return:
      number of records; otherwise -1 if found no records or failed.	

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
   20140306      0001       Murnan             Modified for hourly accum,
                                               specifically TOH
******************************************************************************/	

int query_DB_TOH(int data_id, time_t start_time, time_t end_time,
             void **query_result, int *number_record_p)
{
    char where[300]; /* hold the clause in the SQL-like API */

    int num_record = 0; /* number of records */
    char   msg[200];    /* stderr msg        */


    sprintf(where, "end_time > %d and begin_time < %d", (int) start_time,
                                                        (int) end_time);

    num_record = RPGC_DB_select(data_id, where, query_result);

    if (num_record <= 0)
    {
      if(DP_LT_ACCUM_DEBUG)
      {
         sprintf(msg, "%s %s %d\n",
                   "TOH query_DB:",
                   "found no results, num_record =",
                   num_record);

         fprintf(stderr, msg);
      }
      return -1;
    }

    if(DP_LT_ACCUM_DEBUG)
    {
      sprintf(msg, "%s %s %d\n",
                   "TOH query_DB:",
                   "found records to use, num_record =",
                   num_record);
      fprintf(stderr, msg);
    }

    *number_record_p = num_record;

    return num_record;
} /* end query_DB_TOH() =================================== */

/******************************************************************************
    Function: interpolate_grid_TOH.c

    Description:
    ============
    interpolate_grid_TOH() adjusts an S2S_Accum_Buf_t to a fraction of its
    time span. This version is used by compute_TOH_accum_grid only.  Another
    version is used by DUA and to trim other hourly products back to an hour.

    If INTERP_FORWARD is set,

       accum->accum_grid[][] will be replaced by a grid that interpolates
       FORWARD from interp_time to accum->supl.end_time.

       accum->supl.begin_time will be replaced with interp_time.

    If INTERP_BACKWARD is set,

       accum->accum_grid[][] will be replaced by a grid that interpolates
       BACKWARD from interp_time to accum->supl.begin_time.

       accum->supl.end_time will be replaced with interp_time.

    Inputs: S2S_Accum_Buf_t* accum       - buffer you want to interpolate
            time_t           interp_time - interpolation time
            int              direction   - INTERP_FORWARD or INTERP_BACKWARD

    Outputs: S2S_Accum_Buf_t* accum modified

    Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

    Called by: compute_TOH_accum_grid().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ------------------
    20140306      0001       Berkowitz/Murnan   Initial implementation, 
                                                modification for handling
                                                when interp_time= accum begin
                                                time for INTERP_FORWARD
                                                to include entire accum grid.
                                                Smiliar situation for 
                                                INTERP_BACKWARD and accum end
                                                time. 
******************************************************************************/

int interpolate_grid_TOH(S2S_Accum_Buf_t *accum, time_t interp_time, int direction)
{
   int    i, j; /* indices */
   time_t interp_secs     = 0L;
   time_t accum_secs      = 0L;
   float  interp_fraction = 0.0;
   float  temp_val        = 0.0;
   char   msg[200];                /* stderr msg */


   /* Check for NULL pointer */

   if(pointer_is_NULL(accum, "interpolate_grid_TOH", "accum"))
      return(NULL_POINTER);

   /* Don't interpolate over a missing period */

   if(DP_LT_ACCUM_DEBUG)
   {
      sprintf(msg,"%s\n",
           "interpolate_grid_TOH: Starting");
      fprintf(stderr,msg);
   } 

   if (accum->supl.missing_period_flg == TRUE)
      return (FUNCTION_FAILED);

   /* Do we have a time span to interpolate over? */

   accum_secs = accum->supl.end_time - accum->supl.begin_time;

   if(DP_LT_ACCUM_DEBUG)
   {
      sprintf(msg,"%s %ld\n",
           "interpolate_grid_TOH: calculate accum_secs =",
           accum_secs);
      fprintf(stderr,msg);
   } 


   if (accum_secs == 0) /* no time span */
      return (FUNCTION_FAILED);

   /* How long to interpolate? */

   switch (direction)
   {
      case INTERP_FORWARD:  /* interp_time to end_time */
           interp_secs = accum->supl.end_time - interp_time ;
           break;
      case INTERP_BACKWARD: /* begin_time to interp_time */
           interp_secs = interp_time - accum->supl.begin_time;
           break;
      default: /* unknown direction */
           return (FUNCTION_FAILED);
           break;
   }

   if(DP_LT_ACCUM_DEBUG)
   {
       sprintf(msg,"%s %d %s %d %s %d\n",
            "interpolate_grid_TOH: direction =",
            direction, "INTERP_FORWARD =",
            INTERP_FORWARD, "INTERP_BACKWARD =",
            INTERP_BACKWARD);
       fprintf(stderr,msg);
   }
 
   if(interp_secs <= 0)
   {
      /* Note the condition of "interp_secs = 0" is handled in
       * the DB_Query_TOH function by the SQL query of:
       * "end_time > TOH_begtime" and "begin_time < TOH_endtime"
       * Using this SQL query would produce accumulation periods that have:
       * TOH_begtime = accum->supl.begin_time and/or
       * TOH_endtime = accum->supl.end_time.
       */

      if(DP_LT_ACCUM_DEBUG)
      {
         sprintf(msg,"%s %d %s %ld %s %ld\n",
               "interpolate_grid_TOH: DIRECTION (FORWD=1, BACKWD=-1):",
               direction, "bad condition: interp_secs <= 0 :",
               interp_secs, "<=", accum_secs);
         fprintf(stderr,msg);
      } 
      return (FUNCTION_FAILED);
   }
   else if(interp_secs >= accum_secs) 
   {
      /* Note the condition of "interp_secs == accum_secs" is handled in
       * the DB_Query_TOH function by the SQL query of:
       * "end_time > TOH_begtime" and "begin_time < TOH_endtime"
       * Using this SQL query could produce either accumulation periods that have:
       * TOH_begtime = accum->supl.begin_time or
       * TOH_endtime = accum->supl.end_time.
       * The condition where interp_secs > accum_secs should never happen.
       * In this case this entire accumulation grid would be added to the
       * TOH total unbiased accumulation grid (i.e., TOH_unbiased (AZM)(RAN)) 
       */

 
      if(DP_LT_ACCUM_DEBUG)
      {
         sprintf(msg,"%s %d %s %ld %s %ld\n",
               "interpolate_grid_TOH: DIRECTION (FORWD=1, BACKWD=-1):",
               direction, "bad condition: interp_secs >= accum_secs :", 
               interp_secs, ">=", accum_secs);
         fprintf(stderr,msg);
      } 
      return (FUNCTION_SUCCEEDED);
   }
   else
   {
   

   /* Interpolation should result with amounts less than the original grid.
    * interp_fraction should always be less than 1.0 */

      interp_fraction = ((float) interp_secs) / accum_secs;

      if(DP_LT_ACCUM_DEBUG)
      {
         sprintf(msg,"%s %d %s %ld %s %ld\n",
               "interpolate_grid_TOH: DIRECTION (FORWD=1, BACKWD=-1):",
               direction, "interpolate condition: interp_secs < accum_secs :", 
               interp_secs, "<", accum_secs);
         fprintf(stderr,msg);
      } 

      /* Do the interpolation */

      for (i = 0; i < MAX_AZM; ++i)
      {
         for (j = 0; j < MAX_BINS; ++j)
         {
            /* Don't interpolate a QPE_NODATA */

            if (accum->accum_grid[i][j] == QPE_NODATA)
               continue;

            temp_val = (float) accum->accum_grid[i][j] * interp_fraction;

            if (temp_val > SHRT_MAX)
            {
               accum->accum_grid[i][j] = SHRT_MAX;

               if(DP_LT_ACCUM_DEBUG)
               {
                  sprintf(msg,"%s temp_val: %f, accum_grid[%d][%d] capped at "
                        "SHRT_MAX (%d)\n",
                        "interpolate_grid_TOH", temp_val, i, j, SHRT_MAX);
                  fprintf(stderr,msg);
               }
            }
            else if (temp_val <= SHRT_MIN)
            {
               /* Add 1 since QPE_NODATA is SHRT_MIN */

               accum->accum_grid[i][j] = SHRT_MIN + 1;

               if (DP_LT_ACCUM_DEBUG)
               {
                  sprintf(msg,"%s temp_val: %f, accum_grid[%d][%d] capped at "
	                "SHRT_MIN + 1(%d)\n",
                        "interpolate_grid_TOH:", temp_val, i, j, SHRT_MIN + 1);
                  fprintf(stderr,msg);
               }
            }
            else /* a good value */
            {
               accum->accum_grid[i][j] = (short) RPGC_NINT (temp_val);
               if(accum->accum_grid[i][j] == QPE_NODATA) /* accidentally landed */
                  accum->accum_grid[i][j] += 1;          /* move up 1 */
            }

         } /* end loop over all bins */

      } /* end loop over all radials */

      /* Adjust accum buffer starting/ending time */

      if ( direction == INTERP_FORWARD )
         accum->supl.begin_time = interp_time;
      else /* direction == INTERP_BACKWARD */
         accum->supl.end_time = interp_time;
   }
   return (FUNCTION_SUCCEEDED);

} /* end interpolate_gird_TOH() =================================== */


/******************************************************************************
   Function name: compute_TOH_accum_grid ()

   Description:
   ============
      It computes accumulation for each Top of Hour period

   Inputs:
      query result, number of records, pointer of type LT_Accum_Buf_t

   Outputs:
      lt_accum_buf

   Return:
      0 - OK, -1 - error	

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation for DUA

   20140306      0001       Murnan             Reworked code to handle Top of
                                               Hour accumulation
******************************************************************************/	

int compute_TOH_accum_grid(void * query_result, int num_record,
                           LT_Accum_Buf_t*   lt_accum_buf)
{
    int    i;
    int ret;                           /* hold the return value of a function for 
                                        * program logic control */
    char * record;                     /* hold one record in the query result */
    S2S_Accum_Buf_t * accum_data;      /* hold one record in query results in
                                        * format of structure S2S_Accum_Buf_t */
    int num_of_record_with_precip = 0; /* the number of actual records used */

    time_t total_missing_time = 0L;    /* sum of all missing time during
                                        * TOH accumulation */
    char   msg[200];                   /* stderr msg */


    /* Other initializations */
    lt_accum_buf->supl.missing_period_TOH_unbiased = FALSE;

    /* Read input data and add it to final grid. First record is assumed to include
     * the TOH_begtime.  Similar, the last record is assumed to include the
     * TOH_endtime. */

    for (i = 0; i < num_record; i++)
    {

        if(DP_LT_ACCUM_DEBUG)
        {
           sprintf(msg, "%s %s %d %s %d\n",
                 "compute_TOH_accum_grid:",
                 "reading record, record number =",i,
                 "total records to read =", num_record);
           fprintf(stderr, msg);
        }

        /* Read a record from the query result.  Put result in accum_data */

        ret = RPGC_DB_get_record(query_result, i, &record);
        if(ret < 0)
        {
            sprintf(msg, "%s %s\n",
                  "compute_TOH_accum_grid:",
                  "read data error");

            RPGC_log_msg(GL_INFO, msg);
            if(DP_LT_ACCUM_DEBUG)
              fprintf(stderr, msg);
   
            if(query_result != NULL)
              free(query_result);

            return -1;
        }
        else
        {
            accum_data = (S2S_Accum_Buf_t *)record;

            if(DP_LT_ACCUM_DEBUG)
            {
               sprintf(msg, "%s %s %s%ld %s%ld %s%ld %s%ld\n",
                     "compute_TOH_accum_grid:",
                     "filled in accum_data",
                     "begin_time = ", accum_data->supl.begin_time,
                     "end_time = ", accum_data->supl.end_time,
                     "TOH_begtime = ", lt_accum_buf->supl.TOH_begtime,
                     "TOH_endtime = ", lt_accum_buf->supl.TOH_endtime);
               fprintf(stderr, msg);
            }

            if(DP_LT_ACCUM_DEBUG)
            {
               sprintf(msg, "%s %s%d\n",
                     "compute_TOH_accum_grid:",
                     "ST_active_flg = ", accum_data->supl.ST_active_flg);
               fprintf(stderr, msg);
            }


        }

        if(accum_data->supl.ST_active_flg == FALSE)
        {
            if(record != NULL)
               free(record);

            continue;
        }

        if(DP_LT_ACCUM_DEBUG)
        {
           sprintf(msg, "%s %s%ld %s%ld %s%ld %s%ld\n",
                 "compute_TOH_accum_grid: s2s",
                 "period begins at:", accum_data->supl.begin_time,
                 "period ends at:", accum_data->supl.end_time,
                 "TOH_begtime =", lt_accum_buf->supl.TOH_begtime,
                 "TOH_endtime =", lt_accum_buf->supl.TOH_endtime);

           fprintf(stderr, msg);
        }


        /* Interpolation for the first and the last record 
         * Note: condition where accum_data->supl.begin_time and
         * lt_accum_buff->supl.TOH_begtime are equal, we do not 
         * need to interpolate because in this situation we want 
         * the entire accumulation period anyway.*/


        if (accum_data->supl.begin_time < lt_accum_buf->supl.TOH_begtime  &&
            accum_data->supl.end_time >= lt_accum_buf->supl.TOH_begtime) /* 1st record */
        {

           if(DP_LT_ACCUM_DEBUG)
           {
              sprintf(msg, "%s %s%ld %s%ld %s\n",
                    "compute_TOH_accum_grid: s2s",
                    "period begins at:", accum_data->supl.begin_time,
                    "period ends at:", accum_data->supl.end_time,
                    "interpolate forward");

              fprintf(stderr, msg);
           }

           ret = interpolate_grid_TOH(accum_data, lt_accum_buf->supl.TOH_begtime,
                                        INTERP_FORWARD);
           if (ret == FUNCTION_FAILED)
           {

              if(DP_LT_ACCUM_DEBUG)
              {
                 sprintf(msg, "%s %s%ld %s%ld %s%d %s%ld\n",
                       "compute_TOH_accum_grid: s2s",
                       "begins at:", accum_data->supl.begin_time,
                       "ends at:", accum_data->supl.end_time,
                       "index #:", i, 
                       "TOH begin is:", lt_accum_buf->supl.TOH_begtime);

                 fprintf(stderr, msg);
              }

              continue;
           }
        }  /* completed 1st record */

        else if (accum_data->supl.begin_time < lt_accum_buf->supl.TOH_endtime  &&
            accum_data->supl.end_time >= lt_accum_buf->supl.TOH_endtime) /* last record */
        {

           if(DP_LT_ACCUM_DEBUG)
           {
              sprintf(msg, "%s %s%ld %s%ld %s\n",
                    "compute_TOH_accum_grid: s2s",
                    "period begins at:", accum_data->supl.begin_time,
                    "period ends at:", accum_data->supl.end_time,
                    "interpolate backward");

              fprintf(stderr, msg);
           }

           ret = interpolate_grid_TOH(accum_data, lt_accum_buf->supl.TOH_endtime,
                                                   INTERP_BACKWARD);

           if (ret == FUNCTION_FAILED)
           {

              if(DP_LT_ACCUM_DEBUG)
              {
                sprintf(msg, "%s %s%ld %s%ld %s%d %s%d %s%ld\n",
                      "compute_TOH_accum_grid: s2s",
                      "begins at:",accum_data->supl.begin_time,
                      "ends at:",accum_data->supl.end_time,
                      "index (", num_record, ")-1 compared to actual #:", i,
                      "TOH end is:",lt_accum_buf->supl.TOH_endtime);
                fprintf(stderr, msg);
              }

              continue;
           }
        }  /* completed last record */


        add_unbiased_short_to_int(lt_accum_buf->TOH_unbiased, accum_data->accum_grid,
                                      INT_MAX);
        num_of_record_with_precip ++;


        if(accum_data->supl.missing_period_flg == TRUE)
        {
           total_missing_time += accum_data->supl.missing_period_time;
           lt_accum_buf->supl.missing_period_TOH_unbiased = TRUE; 
        }

        if(record != NULL)
           free(record);

    }  /* end of for (i = 0; i < num_record; i++)*/


    if(DP_LT_ACCUM_DEBUG)
    {
       sprintf(msg, "%s %s\n",
             "compute_TOH_accum_grid:",
             "finished interpolations");
       fprintf(stderr, msg);
    }

    /* set values of bias and null_product flag*/

    if (num_of_record_with_precip == 0) /* no record has precipitation */
    {
        lt_accum_buf->supl.null_TOH_unbiased = NULL_REASON_2;
    }
    else
    {
        lt_accum_buf->supl.null_TOH_unbiased = FALSE;  /* generate a TOH product */
    }

    lt_accum_buf->supl.total_TOH_missing_time = total_missing_time;

    if(query_result != NULL)
       free(query_result);

    return 0;
} /* end compute_Top_of_Hour() =================================== */
