/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:12:11 $
 * $Id: dp_lt_accum_backup.c,v 1.2 2009/03/03 18:12:11 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

/***********************************************************************
   Filename: dp_lt_accum_backup.c

   Description
   ===========
      backup_lt_accum() backs up the lt_accum buffers to disk.

   Input: Circular_Queue_t* hourly_circq      - hourly circular queue
          Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
          Storm_Backup_t*   storm_backup      - storm backup
          Storm_Backup_t*   storm_diff_backup - storm diff backup

   Output: All 4 data stores backed up.

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int backup_lt_accum(Circular_Queue_t* hourly_circq,
                    Circular_Queue_t* hourly_diff_circq,
                    Storm_Backup_t*   storm_backup,
                    Storm_Backup_t*   storm_diff_backup)
{
   char  msg[200];         /* stderr message                  */
   int backup_OK = TRUE;   /* TRUE -> backed up 4 data stores */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning backup_lt_accum()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_circq, "backup_lt_accum", "hourly_circq"))
     return(NULL_POINTER);

   if(pointer_is_NULL(hourly_diff_circq, "backup_lt_accum", "hourly_diff_circq"))
     return(NULL_POINTER);

   if(pointer_is_NULL(storm_backup, "backup_lt_accum", "storm_backup"))
     return(NULL_POINTER);

   if(pointer_is_NULL(storm_diff_backup, "backup_lt_accum", "storm_diff_backup"))
     return(NULL_POINTER);

   /* We don't do anything if a backup fails.
    * Unlike restore, if one backup fails, we don't abort,
    * but try and continue with the backup. */

   backup_OK |= backup_hourly(hourly_circq);
   backup_OK |= backup_hourly_diff(hourly_diff_circq);
   backup_OK |= backup_storm(storm_backup);
   backup_OK |= backup_storm_diff(storm_diff_backup);

   if(backup_OK)
   {
      sprintf(msg, "Backup successful\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }
   else
   {
      sprintf(msg, "Backup failed\n");
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   return(FUNCTION_SUCCEEDED);

} /* end backup_lt_accum() =================================== */

/***********************************************************************
   Filename: dp_lt_accum_backup.c

   Description
   ===========
      backup_hourly() backs up the hourly circular queue to disk.

   Input: Circular_Queue_t* hourly_circq - hourly circular queue

   Output: The hourly circular queue backed up.

   Returns: FALSE (0), TRUE (1)

   We don't worry about writing the accum bufs to disk, as they are
   saved as we go along. (See CQ_Add_to_back in file dp_lt_accum_circ_q.c)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int backup_hourly(Circular_Queue_t* hourly_circq)
{
   int  bytes_written = 0; /* num bytes written */
   char msg[200];          /* stderr message    */

   static unsigned int circq_size = sizeof(Circular_Queue_t);

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning backup_hourly()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_circq, "backup_hourly",
                     "hourly_circq"))
   {
      /* Instead of returning NULL_POINTER, we return FALSE */

      return(FALSE);
   }

   bytes_written = RPGC_data_access_write(DP_HRLY_BACKUP,
                                          hourly_circq,
                                          circq_size,
                                          (LB_id_t) HOURLY_ID);
   if(bytes_written != circq_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_written %d\n",
              "backup_hourly:",
              "Failed to write HOURLY_ID to DP_HRLY_BACKUP",
               DP_HRLY_BACKUP, HOURLY_ID, bytes_written);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FALSE);
   }

   return(TRUE);

} /* end backup_hourly() =================================== */

/***********************************************************************
   Filename: dp_lt_accum_backup.c

   Description
   ===========
      backup_hourly_diff() backs up the hourly diff circular queue to disk.

   Input: Circular_Queue_t* hourly_diff_circq - hourly diff circular queue

   Output: The hourly diff circular queue backed up.

   Returns: FALSE (0), TRUE (1)

   We don't worry about writing the accum bufs to disk, as they are
   saved as we go along. (See CQ_Add_to_back in file dp_lt_accum_circ_q.c)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int backup_hourly_diff(Circular_Queue_t* hourly_diff_circq)
{
   int  bytes_written = 0; /* num bytes written */
   char msg[200];          /* stderr message    */

   static unsigned int circq_size = sizeof(Circular_Queue_t);

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning backup_hourly_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(hourly_diff_circq, "backup_hourly_diff",
                     "hourly_diff_circq"))
   {
      /* Instead of returning NULL_POINTER, we return FALSE */

      return(FALSE);
   }

   bytes_written = RPGC_data_access_write(DP_HRLY_BACKUP,
                                          hourly_diff_circq,
                                          circq_size,
                                          (LB_id_t) HOURLY_DIFF_ID);
   if(bytes_written != circq_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_written %d\n",
              "backup_hourly_diff:",
              "Failed to write HOURLY_DIFF_ID to DP_HRLY_BACKUP",
               DP_HRLY_BACKUP, HOURLY_DIFF_ID, bytes_written);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FALSE);
   }

   return(TRUE);

} /* end backup_hourly_diff() =================================== */

/***********************************************************************
   Filename: dp_lt_accum_backup.c

   Description
   ===========
      backup_storm() backs up the storm total to disk.

   Input: Storm_Backup_t* storm_backup - storm total backup

   Output: The storm total backed up.

   Returns: FALSE (0), TRUE (1)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int backup_storm(Storm_Backup_t* storm_backup)
{
   int  bytes_written = 0; /* num bytes written */
   char msg[200];          /* stderr message    */

   static unsigned int storm_size = sizeof(Storm_Backup_t);

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning backup_storm()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(storm_backup, "storm_backup",
                     "storm_backup"))
   {
      /* Instead of returning NULL_POINTER, we return FALSE */

      return(FALSE);
   }

   bytes_written = RPGC_data_access_write(DP_STORM_BACKUP,
                                          storm_backup,
                                          storm_size,
                                          (LB_id_t) STORM_ID);
   if(bytes_written != storm_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_written %d\n",
              "backup_storm:",
              "Failed to write STORM_ID to DP_STORM_BACKUP",
               DP_STORM_BACKUP, STORM_ID, bytes_written);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FALSE);
   }

   return(TRUE);

} /* end backup_storm() =================================== */

/***********************************************************************
   Filename: dp_lt_accum_backup.c

   Description
   ===========
      backup_storm_diff() backs up the storm total to disk.

   Input: Storm_Backup_t* storm_diff_backup - storm diff backup

   Output: The storm diff backed up.

   Returns: FALSE (0), TRUE (1)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   12/19/2007  0000       Ward               Initial version
************************************************************************/

int backup_storm_diff(Storm_Backup_t* storm_diff_backup)
{
   int  bytes_written = 0; /* num bytes written */
   char msg[200];          /* stderr message    */

   static unsigned int storm_size = sizeof(Storm_Backup_t);

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning backup_storm_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(storm_diff_backup, "backup_storm_diff",
                     "storm_diff_backup"))
   {
      /* Instead of returning NULL_POINTER, we return FALSE */

      return(FALSE);
   }

   bytes_written = RPGC_data_access_write(DP_STORM_BACKUP,
                                          storm_diff_backup,
                                          storm_size,
                                          (LB_id_t) STORM_DIFF_ID);
   if(bytes_written != storm_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_written %d\n",
              "backup_storm_diff:",
              "Failed to write STORM_DIFF_ID to DP_STORM_BACKUP",
               DP_STORM_BACKUP, STORM_DIFF_ID, bytes_written);
      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LT_ACCUM_DEBUG)
            fprintf(stderr, msg);
      }

      return(FALSE);
   }

   return(TRUE);

} /* end backup_storm_diff() =================================== */
