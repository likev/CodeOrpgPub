/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:06 $
 * $Id: gauge_radar_db.c,v 1.3 2011/04/13 22:53:06 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <dp_Consts.h>                /* FUNCTION_SUCCEEDED    */
#include <rpgc.h>                     /* RPGC_data_access_open */
#include <dp_lib_func_prototypes.h>   /* rpg_err_to_msg        */
#include <stdlib.h>                   /* free                  */
#include <gauge_radar_types.h>        /* Gauges_Buf_t          */

/* #define DP_GAUGES 30007 old database number */

/******************************************************************************
    Filename: gauge_radar_db.c

    Description:
    ============
       query_gauges_DB() queries the gauges database based on a user specified
       start time and end time.

    Input:  time_t start_time - start_time of query
            time_t end_time   - end_time   of query

    Output: Gauges_Buf_t* gauges_buf - gauges to be written to

    Returns: number of positive gauges

    Called by: <uncalled>, so commented out

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    --------    -------    ----------      ---------------
    20091202    0000       James Ward      Initial version
******************************************************************************/

/*
 * int query_gauges_DB(time_t start_time, time_t end_time,
 *                     Gauges_Buf_t* gauges_buf)
 * {
 *    char  where[300]; -* holds the where clause in the SQL-like API *-
 *    int   ret;        -* return value *-
 *    char  msg[200];   -* stderr message *-
 *    char  msg2[200];  -* stderr message *-
 *    int   i, j;
 *    char* record  = NULL;
 *    void* records = NULL;
 *
 *    int num_records     = 0;
 *    int positive_gauges = 0;
 *
 *    Gauges_Buf_t* new_gauges = NULL;
 *
 *    -* Check for NULL pointer *-
 *
 *    if(pointer_is_NULL(gauges_buf, "query_gauges_DB", "gauges_buf"))
 *       return(0);
 *
 *    -* Check that start_time <= end_time *-
 *
 *    if(start_time > end_time)
 *    {
 *       sprintf(msg, "start_time %ld > end_time %ld, can't query gauges\n",
 *               start_time, end_time);
 *
 *       RPGC_log_msg(GL_INFO, msg);
 *       if(GAUGE_RADAR_DEBUG)
 *          fprintf(stderr, msg);
 *
 *      return(0);
 *    }
 *
 *   -* Initialize gauges. If we were distinguishing between
 *    * 0.0 and "no data", we would initialize them to
 *    * "no data" here. *-
 *
 *   memset(gauges_buf, 0, gauges_buf_size);
 *
 *   gauges_buf->query.start_time = start_time;
 *   gauges_buf->query.end_time   = end_time;
 *
 *   -* Sample fixed values for for testing
 *    *
 *    * gauges_buf->gauges[  5 -* BLAC *-] = 0.25 * MM_TO_IN;
 *    * gauges_buf->gauges[ 44 -* NEWK *-] = 0.25 * MM_TO_IN;
 *    * gauges_buf->gauges[ 49 -* PAWN *-] = 0.51 * MM_TO_IN;
 *    * gauges_buf->gauges[ 52 -* REDR *-] = 1.27 * MM_TO_IN;
 *    * gauges_buf->gauges[ 78 -* ALV2 *-] = 0.25 * MM_TO_IN;
 *    * gauges_buf->gauges[ 78 -* ALV2 *-] = 0.25 * MM_TO_IN;
 *    *
 *    * return(6);
 *    *-
 *
 *    -*
 *    gauges_buf->gauges[ 20 /-   DURA -/] = 0.00984252;
 *    gauges_buf->gauges[ 36 /-   LANE -/] = 0.00984252;
 *    gauges_buf->gauges[ 47 /-   OKMU -/] = 0.00984252;
 *    gauges_buf->gauges[ 94 /-   HOLD -/] = 0.0299213;
 *    gauges_buf->gauges[132 /- KCB103 -/] = 0.00393701;
 *    gauges_buf->gauges[163 /- KSW110 -/] = 0.00393701;
 *
 *    return(6);
 *
 *    gauges_buf->gauges[ 20 /-   DURA -/] = 0.0299213;
 *    gauges_buf->gauges[ 36 /-   LANE -/] = 0.05;
 *    gauges_buf->gauges[ 47 /-   OKMU -/] = 0.00984252;
 *    gauges_buf->gauges[ 61 /-   STUA -/] = 0.0102362;
 *    gauges_buf->gauges[ 94 /-   HOLD -/] = 0.129921;
 *    gauges_buf->gauges[132 /- KCB103 -/] = 0.00393701;
 *    gauges_buf->gauges[154 /- KSE102 -/] = 0.00393701;
 *    gauges_buf->gauges[158 /- KSW104 -/] = 0.00393701;
 *    gauges_buf->gauges[160 /- KSW107 -/] = 0.00393701;
 *    gauges_buf->gauges[161 /- KSW108 -/] = 0.00393701;
 *    gauges_buf->gauges[162 /- KSW109 -/] = 0.00787402;
 *    gauges_buf->gauges[163 /- KSW110 -/] = 0.00787402;
 *
 *    return(12);
 *    *-
 *
 *   -* A gauge record covers [start_time, end_time) *-
 *
 *   sprintf(where, "start_time < %d and end_time > %d",
 *           (int) end_time, (int) start_time);
 *
 *   num_records = RPGC_DB_select(DP_GAUGES, where, &records);
 *
 *   if(num_records > 0) -* there are some records in the DB *-
 *   {
 *      sprintf(msg, "%ld <= %d gauge record", start_time, num_records);
 *
 *      if(num_records > 1)
 *         strcat(msg, "s");
 *
 *      sprintf(msg2, "%s < %ld\n", msg, end_time);
 *
 *      RPGC_log_msg(GL_INFO, msg2);
 *      if(GAUGE_RADAR_DEBUG)
 *         fprintf(stderr, msg2);
 *
 *      for(i = 0; i < num_records; i++)
 *      {
 *         ret = RPGC_DB_get_record(records, i, &record);
 *
 *         if((ret < 0) || (record == NULL)) -* error *-
 *         {
 *            sprintf(msg, "error: failed to get record %d, ret %d\n", i, ret);
 *
 *            RPGC_log_msg(GL_INFO, msg);
 *            if(GAUGE_RADAR_DEBUG)
 *               fprintf(stderr, msg);
 *         }
 *         else -* retrieved record i *-
 *         {
 *            new_gauges = (Gauges_Buf_t*) record;
 *
 *            sprintf(msg, "%ld <= reading gauge record %d < %ld\n",
 *                    new_gauges->query.start_time, i+1,
 *                    new_gauges->query.end_time);
 *
 *            RPGC_log_msg(GL_INFO, msg);
 *            if(GAUGE_RADAR_DEBUG)
 *               fprintf(stderr, msg);
 *
 *            for(j=0; j<NUM_GAUGES; j++)
 *            {
 *               if((new_gauges->gauges[j] <=   0.0) ||
 *                  (new_gauges->gauges[j] >= 990.0))
 *               {
 *                  -* This is a bad gauge, so don't add it in.
 *                   * According to Dave Kitzmiller, one bad gauge
 *                   * record should cause the total for that gauge
 *                   * to be discarded, so set flag gauges_buf->gauges[j]
 *                   * to QPE_NODATA and the total for that gauge will
 *                   * discarded after all the records have been read.
 *                   *
 *                   * Missing gauges are returned from the URL as < -990.0,
 *                   * BUT the python script doesn't keep track of individual
 *                   * missing values, it adds and subtracts the whole gauge
 *                   * array at once. For example:
 *                   *
 *                   * python ~/src/cpc102/tsk022/gauge_radar.py \
 *                   *        /tmp 20091109 2220 20091110 1400
 *                   *
 *                   * for the gauge A149, returns:
 *                   *
 *                   *  STID STNM TIME RAIN TS05 TS25 TS45 VW05 VW25 VW45
 *                   *  A149  521 1340 -996 -995 -995 -995 -995 -995 -995
 *                   *  A149  521 1435 0.00 -995 -995 -995 -995 -995 -995
 *                   *  A149  521 1340 -996 -995 -995 -995 -995 -995 -995
 *                   *  A149  521    0 0.00 18.9 17.0 16.5 0.18 0.20 0.18
 *                   *  A149  521  840 0.00 15.9 16.6 16.8 0.18 0.20 0.18
 *                   *
 *                   * This shouldn't be a problem if we store the gauge data
 *                   * in (the minimum) 5 minute intervals. Then the script
 *                   * would see only 1 gauge record, -996. Over longer
 *                   * intervals, * the script sums the values, including
 *                   * the bad ones.  * returning 996.0 above. The >= 990.0
 *                   * check is designed to catch this.
 *                   *
 *                   * A better solution is for the python script to
 *                   * return 'missing' if any of the 5 minute readings it
 *                   * collects are bad (-996).
 *                   *
 *                   * Because we are storing the gauge data in 5 minute
 *                   * intervals, we are not interpolating over the beginning
 *                   * and end of the time intervals. *-
 *
 *                  gauges_buf->gauges[j] = QPE_NODATA;
 *
 *                  -* debug printout
 *                   * fprintf(stderr,
 *                   *         "new_gauges[%d] %f < 0.0, %s[%d] %s\n",
 *                   *          j, new_gauges->gauges[j],
 *                   *          "init gauges_buf", j, "to QPE_NODATA");
 *                   *-
 *               }
 *               else if(gauges_buf->gauges[j] != QPE_NODATA)
 *               {
 *                  -* We have a good old value and a good new value *-
 *
 *                  -* debug printout
 *                   * fprintf(stderr,
 *                   *         "gauges_buf[%d] %f + new_gauges[%d] %f = ",
 *                   *         j, gauges_buf->gauges[j],
 *                   *         j, new_gauges->gauges[j]);
 *                   *-
 *
 *                  gauges_buf->gauges[j] += new_gauges->gauges[j];
 *
 *                  -* debug printout
 *                   * fprintf(stderr, "%f mm\n", gauges_buf->gauges[j]);
 *                   *-
 *               }
 *            }
 *
 *            -* Free memory allocated for record i by RPGC_DB_get_record() *-
 *
 *            free(record);
 *            record = NULL;
 *         }
 *      }
 *
 *      -* Free memory allocated for all records by RPGC_DB_select() *-
 *
 *      free(records);
 *      records = NULL;
 *
 *      -* Convert summed buffer from mm to inches.
 *       *
 *       * If a gauge value reads QPE_NODATA, one of the records for
 *       * that gauge is bad. Change the value to 0.0, so when the
 *       * gauge-radar pairs are looked at, it will be ignored,
 *       * for statistics are only calculated when the gauge and the
 *       * radar are both positive. A second approach would be to
 *       * return QPE_NODATA, to distinguish between 0.0 and "no data". *-
 *
 *      for(i=0; i<NUM_GAUGES; i++)
 *      {
 *          if(gauges_buf->gauges[i] == QPE_NODATA)
 *          {
 *             gauges_buf->gauges[i] = 0.0;
 *          }
 *          else if(gauges_buf->gauges[i] > 0.0)
 *          {
 *             gauges_buf->gauges[i] *= MM_TO_IN;
 *             positive_gauges++;
 *          }
 *      }
 *   } -* end num_records > 0 *-
 *   else if(num_records == 0) -* no records in DB *-
 *   {
 *      sprintf(msg, "%ld <= no gauge records < %ld\n",
 *                    start_time, end_time);
 *
 *      RPGC_log_msg(GL_INFO, msg);
 *      if(GAUGE_RADAR_DEBUG)
 *         fprintf(stderr, msg);
 *
 *      return(0);
 *   }
 *   else -* num_records < 0, DB error *-
 *   {
 *      sprintf(msg, "error: %d, %ld <= no gauge records < %ld\n",
 *                    num_records, start_time, end_time);
 *
 *      RPGC_log_msg(GL_INFO, msg);
 *      if(GAUGE_RADAR_DEBUG)
 *         fprintf(stderr, msg);
 *
 *      return(0);
 *   }
 *
 *   return(positive_gauges);
 *
 *} -* end query_gauges_DB() ===================================== *-
 */

/******************************************************************************
    Filename: gauge_radar_db.c

    Description:
    ============
       insert_gauges_DB() writes gauges data to the database

    Input:  Gauges_Buf_t* gauges - data to be written

    Output: data written

    Returns: WRITE_OK      (0) - successful write
             WRITE_FAILED (-1) - failed     write
             NULL_POINTER  (2) - null pointer, nothing to write

    Called by: <uncalled>, so commented out

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    --------    -------    ----------      ---------------
    20091202    0000       James Ward      Initial version
******************************************************************************/

/* int insert_gauges_DB(Gauges_Buf_t* gauges_buf)
 *{
 *   int  bytes_written = 0; -* number of  bytes written *-
 *   char msg[200];          -* stderr message           *-
 *
 *   -* Check for NULL pointer *-
 *
 *   if(pointer_is_NULL(gauges_buf, "insert_gauges_DB", "gauges_buf"))
 *      return(NULL_POINTER);
 *
 *   -* Insert into the database *-
 *
 *   bytes_written = RPGC_DB_insert(DP_GAUGES, gauges_buf, gauges_buf_size);
 *
 *   if(bytes_written != gauges_buf_size)
 *   {
 *      sprintf(msg, "%s %s, data_id %d, bytes_written %d\n",
 *              "insert_gauges_DB:",
 *              "Failed to write gauges to data store",
 *               DP_GAUGES, bytes_written);
 *
 *      RPGC_log_msg(GL_INFO, msg);
 *      if(GAUGE_RADAR_DEBUG)
 *         fprintf(stderr, msg);
 *
 *      if(bytes_written < 0) -* RPG error *-
 *      {
 *         if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
 *         {
 *            RPGC_log_msg(GL_INFO, msg);
 *            if(GAUGE_RADAR_DEBUG)
 *               fprintf(stderr, msg);
 *         }
 *      }
 *
 *      return(WRITE_FAILED);
 *   }
 *   else -* successful write *-
 *   {
 *      sprintf(msg, "%s %ld <= 1 record < %ld\n",
 *              "Wrote to gauges data store:",
 *               gauges_buf->query.start_time,
 *               gauges_buf->query.end_time);
 *
 *      RPGC_log_msg(GL_INFO, msg);
 *      if(GAUGE_RADAR_DEBUG)
 *         fprintf(stderr, msg);
 *
 *      return(WRITE_OK);
 *   }
 *
 *} -* end insert_gauges_DB() ===================================== *-
 */

/******************************************************************************
    Filename: gauge_radar_db.c

    Description:
    ============
       read_new_gauges() reads new gauges by calling the python script

    Input: char* gauges_start_day   - start YYYYMMDD of read
           char* gauges_start_time  - start HHSS     of read
           char* gauges_end_day     - end   YYYYMMDD of read
           char* gauges_end_time    - end   YYYYMMDD of read
           gauge gauges[MAX_GAUGES] - list of gauges
           short     num_gauges         - number of gauges
           Gauges_Buf_t* gauges_buf - buffer to be filled

    Times are given in YYYYMMDD HHSS instead of UNIX time, because
    that's what the python script prefers. It assumes that the
    start and end times are already on 5 minute boundaries, because
    the python script requires it.

    Output: Gauges_Buf_t* gauges_buf - buffer filled

    Returns: READ_OK      (0) - successful read
             READ_FAILED (-1) - failed     read
             NULL_POINTER (2) - null pointer, nothing to read to

    Called by: query_gauges_web()

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    --------    -------    ----------      ---------------
    20091202    0000       James Ward      Initial version
******************************************************************************/

int read_new_gauges(char* gauges_start_day, char* gauges_start_time,
                    char* gauges_end_day,   char* gauges_end_time,
                    Gauge gauges[MAX_GAUGES],
                    short num_gauges,
                    Gauges_Buf_t* gauges_buf)
{
   char  command[200];     /* python command   */
   char  gauges_file[200]; /* python out file  */
   FILE* fp;               /* file pointer     */
   char  line[40];         /* one line in file */
   char  msg[200];         /* stderr message   */

   static int  first_time = TRUE;
   static char pydir[40];   /* pydir   */
   static char datadir[40]; /* datadir */

   int lines_read;  /* used for debugging                  */
   int gauges_read; /* used to make sure we get all gauges */
   int i;

   if(pointer_is_NULL(gauges_buf, "read_new_gauges", "gauges_buf"))
      return(NULL_POINTER);

   /* Init all the gauges to missing */

   for(i=0; i<MAX_GAUGES; i++)
       gauges_buf->gauges[i] = GAUGE_MISSING;

   /* Call the python script to collect the gauge data.
    * $WORK_DIR/gauges must exist for the script to work. */

   if(first_time == TRUE)
   {
      sprintf(pydir,   "~/src/cpc102/tsk022");
      sprintf(datadir, "%s/gauges", getenv("WORK_DIR"));
      first_time = FALSE;
   }

   sprintf(command,
           "python %s/gauge_radar.py %s %8s %4s %8s %4s\n",
           pydir,
           datadir,
           gauges_start_day, gauges_start_time,
           gauges_end_day,   gauges_end_time);

   sprintf(msg, "%s\n", command);

   RPGC_log_msg(GL_INFO, msg);
   if(GAUGE_RADAR_DEBUG)
      fprintf(stderr, msg);

   system(command);

   /* Python output file should be dropped off in datadir */

   sprintf(gauges_file,
           "%s/%8s%4s_to_%8s%4s_precips.csv",
           datadir,
           gauges_start_day, gauges_start_time,
           gauges_end_day,   gauges_end_time);

   if((fp = fopen(gauges_file, "r")) == NULL)
   {
      /* sprintf(msg,
       *         "Couldn't open gauges file %s\n", gauges_file);
       */

      sprintf(msg,
              "No gauge data from %8s %4s to %8s %4s\n",
              gauges_start_day, gauges_start_time,
              gauges_end_day,   gauges_end_time);

      RPGC_log_msg(GL_INFO, msg);
      if(GAUGE_RADAR_DEBUG)
         fprintf(stderr, msg);

      return(READ_FAILED);
   }
   else /* we opened the data file */
   {
      gauges_read = 0;
      lines_read  = 0;

      while(fgets(line, 40, fp) != NULL)
      {
         lines_read++;

         /* A gauge line returned by the Python script looks like:
          *
          *     REDR ,  1.27
          *
          * where 1.27 (mm) is the gauge reading at the
          * GAUGE_DATA_START position.
          *
          * Locate the gauge in the gauge array.
          * A gauge might be missing. */

         for(i=0; i<num_gauges; i++)
         {
            if(strstr(line, gauges[i].id) != NULL)
            {
               gauges_buf->gauges[i] =
                  atof(&(line[GAUGE_DATA_START]));

               gauges_read++;
               break;
            }
         }

         /* Commented out, we're not reading all gauges
          *
          * if(i == num_gauges)
          * {
          *    sprintf(msg,
          *            "line %4d  %-6.6s %s\n",
          *            lines_read, &(line[2]), "- unknown gauge");
          *
          *    RPGC_log_msg(GL_INFO, msg);
          *    if(GAUGE_RADAR_DEBUG)
          *       fprintf(stderr, msg);
          * }
          */
      }

      if(gauges_read != num_gauges) /* should get exactly num_gauges */
      {
         sprintf(msg,
                 "Expected %d gauges, collected %d\n",
                 num_gauges, gauges_read);

         RPGC_log_msg(GL_INFO, msg);
         if(GAUGE_RADAR_DEBUG)
            fprintf(stderr, msg);

         for(i=0; i<num_gauges; i++)
         {
            if(gauges_buf->gauges[i] == GAUGE_MISSING)
            {
               sprintf(msg,
                       "gauge %s is missing\n",
                       gauges[i].id);

               RPGC_log_msg(GL_INFO, msg);
               if(GAUGE_RADAR_DEBUG)
                  fprintf(stderr, msg);
            }
         }
      }

      fclose(fp);

      /* Remove (unlink) the output file. Comment out unlink
       * if you want the file to hang around for gauge debugging. */

      /*
      unlink(gauges_file);
      */
   }

   return(READ_OK);

} /* end read_new_gauges() ===================================== */

/******************************************************************************
    Filename: gauge_radar_db.c

    Description:
    ============
       query_gauges_web() queries the web directly instead of using
       the 24 hour gauge database. It is intended to be used for storm
       queries that span more than 24 hours.

    Input:  time_t start_time - start_time of query
            time_t end_time   - end_time   of query

    Output: Gauges_Buf_t* gauges_buf - gauges to be written to

    Returns: number of positive gauges

    Called by: write_to_output_product() of dp_evaluation_with_gage_data
               (cpc102/tsk022)

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    --------    -------    ----------      ---------------
    20100203    0000       James Ward      Initial version
******************************************************************************/

int query_gauges_web(time_t start_time, time_t end_time,
                     Gauge gauges[MAX_GAUGES],
                     short num_gauges,
                     Gauges_Buf_t* gauges_buf)
{
   int  ret;                 /* return value */
   int  i;
   int  positive_gauges = 0;
   char msg[200];            /* stderr message */

   time_t secs;

   int year   = 0;
   int month  = 0;
   int day    = 0;
   int hour   = 0;
   int minute = 0;
   int second = 0;

   char gauges_start_day[YYYYMMDD_LEN + 1];
   char gauges_start_time[HHMM_LEN    + 1];
   char gauges_end_day[YYYYMMDD_LEN   + 1];
   char gauges_end_time[HHMM_LEN      + 1];

   static unsigned int gauges_buf_size = sizeof(Gauges_Buf_t);

   /* Check for NULL pointer */

   if(pointer_is_NULL(gauges_buf, "query_gauges_web", "gauges_buf"))
      return(0);

   /* Check that start_time <= end_time */

   if(start_time > end_time)
   {
      sprintf(msg, "start_time %ld > end_time %ld, can't query gauges\n",
              start_time, end_time);

      RPGC_log_msg(GL_INFO, msg);
      if(GAUGE_RADAR_DEBUG)
         fprintf(stderr, msg);

      return(0);
   }

   /* Initialize gauges. If we were distinguishing between
    * 0.0 and "no data", we would initialize them to
    * "no data" here. */

   memset(gauges_buf, 0, gauges_buf_size);

   memset(gauges_start_day,  0, YYYYMMDD_LEN + 1);
   memset(gauges_start_time, 0, HHMM_LEN     + 1);
   memset(gauges_end_day,    0, YYYYMMDD_LEN + 1);
   memset(gauges_end_time,   0, HHMM_LEN     + 1);

   /* Convert the UNIX start_time to YYYYMMDD HHMM.
    * The python script prefers YYYYMMDD HHMM to UNIX time. */

   ret = RPGCS_unix_time_to_ymdhms(start_time,
                                   &year, &month, &day,
                                   &hour, &minute, &second);

   sprintf(msg, "start_time: %04d%02d%02d %02d%02d%02d\n",
           year, month, day, hour, minute, second);

   RPGC_log_msg(GL_INFO, msg);
   if(GAUGE_RADAR_DEBUG)
      fprintf(stderr, msg);

   sprintf(gauges_start_day, "%04d%02d%02d", year, month, day);

   if(start_time % SECS_IN_5_MINS == 0) /* start_time is on a 5 min boundary */
   {
      sprintf(gauges_start_time, "%02d%02d", hour, minute);

      gauges_buf->query.start_time = start_time;
   }
   else /* start_time is not on a 5 minute boundary */
   {
      /* Find a 5 minute boundary before the start_time.
       * Example: If the minute is 47, then 47 / 5 = 9 and 9 * 5 = 45 */

      minute /= 5;
      minute *= 5;

      sprintf(gauges_start_time, "%02d%02d", hour, minute);

      RPGCS_ymdhms_to_unix_time(&(gauges_buf->query.start_time),
                                year, month, day,
                                hour, minute, 0);
   }

   /* Find a 5 minute boundary after the end_time.
    * We do our calculations in UNIX time to avoid
    * minute/hour/day/month/year rollover logic. */

   ret = RPGCS_unix_time_to_ymdhms(end_time,
                                   &year, &month, &day,
                                   &hour, &minute, &second);

   sprintf(msg, "  end_time: %04d%02d%02d %02d%02d%02d\n",
           year, month, day, hour, minute, second);

   RPGC_log_msg(GL_INFO, msg);
   if(GAUGE_RADAR_DEBUG)
      fprintf(stderr, msg);

   if(end_time % SECS_IN_5_MINS == 0) /* end_time is on a 5 min boundary */
   {
      gauges_buf->query.end_time = end_time;
   }
   else /* end_time is not on a 5 minute boundary */
   {
      secs = end_time - gauges_buf->query.start_time;

      secs /= SECS_IN_5_MINS;
      secs *= SECS_IN_5_MINS; /* now at 5 minutes before end_time */
      secs += SECS_IN_5_MINS; /* now at 5 minutes after  end_time */

      gauges_buf->query.end_time = gauges_buf->query.start_time + secs;
   }

   /* Convert the UNIX end_time to YYYYMMDD HHMM.
    * The python script prefers YYYYMMDD HHMM to UNIX time. */

   ret = RPGCS_unix_time_to_ymdhms(gauges_buf->query.end_time,
                                   &year, &month, &day,
                                   &hour, &minute, &second);

   sprintf(gauges_end_day, "%04d%02d%02d", year, month, day);

   sprintf(gauges_end_time, "%02d%02d", hour, minute);

   /* Sample fixed values for for testing
    *
    * gauges_buf->gauges[  5 -* BLAC *-] = 0.25 * MM_TO_IN;
    * gauges_buf->gauges[ 44 -* NEWK *-] = 0.25 * MM_TO_IN;
    * gauges_buf->gauges[ 49 -* PAWN *-] = 0.51 * MM_TO_IN;
    * gauges_buf->gauges[ 52 -* REDR *-] = 1.27 * MM_TO_IN;
    * gauges_buf->gauges[ 78 -* ALV2 *-] = 0.25 * MM_TO_IN;
    * gauges_buf->gauges[ 78 -* ALV2 *-] = 0.25 * MM_TO_IN;
    *
    * return(6);

    * gauges_buf->gauges[ 20 /-   DURA -/] = 0.0299213;
    * gauges_buf->gauges[ 36 /-   LANE -/] = 0.05;
    * gauges_buf->gauges[ 47 /-   OKMU -/] = 0.00984252;
    * gauges_buf->gauges[ 61 /-   STUA -/] = 0.0102362;
    * gauges_buf->gauges[ 94 /-   HOLD -/] = 0.129921;
    * gauges_buf->gauges[132 /- KCB103 -/] = 0.00393701;
    * gauges_buf->gauges[154 /- KSE102 -/] = 0.00393701;
    * gauges_buf->gauges[158 /- KSW104 -/] = 0.00393701;
    * gauges_buf->gauges[160 /- KSW107 -/] = 0.00393701;
    * gauges_buf->gauges[161 /- KSW108 -/] = 0.00393701;
    * gauges_buf->gauges[162 /- KSW109 -/] = 0.00787402;
    * gauges_buf->gauges[163 /- KSW110 -/] = 0.00787402;

    * return(12);
    */

   ret = read_new_gauges(gauges_start_day, gauges_start_time,
                         gauges_end_day,   gauges_end_time,
                         gauges,           num_gauges,
                         gauges_buf);

   /* Check output values. Replace GAUGE_MISSING = -99999 with 0,
    * which is handled better by the gauge-radar pair comparisons. */

   positive_gauges = 0;

   for(i=0; i<num_gauges; i++)
   {
       if((gauges_buf->gauges[i] <=   0.0) ||
          (gauges_buf->gauges[i] >= 990.0))
       {
          gauges_buf->gauges[i] = 0.0;
       }
       else /* gauges_buf->gauges[i] > 0.0 */
       {
          gauges_buf->gauges[i] *= MM_TO_IN;
          positive_gauges++;
       }
   }

   return(positive_gauges);

} /* end query_gauges_web() ===================================== */
