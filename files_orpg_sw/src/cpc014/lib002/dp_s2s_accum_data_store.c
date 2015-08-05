/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 23:05:26 $
 * $Id: dp_s2s_accum_data_store.c,v 1.4 2009/10/27 23:05:26 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include "cs.h"                     /* CS_KEY_NOT_FOUND */
#include "dp_lib_func_prototypes.h"
#include "lb.h"                     /* LB_NOT_FOUND     */

/******************************************************************************
    Filename: dp_s2s_accum_data_store.c

    Description:
    ============
       open_s2s_accum_data_store() will open a data store for reading or
    writing (only) scan-to-scan accum buffers.

    Input:  data_id - DUAUSERSEL           (300002)
                      DP_HOURLY_ACCUM      (300004)
                      DP_HOURLY_DIFF_ACCUM (300005)

    Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1)

    Called by: scan-to-scan main()

    data_id is set in ~/include/orpgdat.h

    We are storing S2S_Accum_Buf_t structures instead of accum grids because we
    need the grid times for interpolation.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    --------    -------    ----------      -----
    20081007    0000       Cham Pham       Initial implementation.
    20090406    0001       James Ward      Added logging of successful open
******************************************************************************/

int open_s2s_accum_data_store(int data_id)
{
   int  ret = FUNCTION_SUCCEEDED;
   char msg[200]; /* stderr message */

   ret = RPGC_data_access_open(data_id, LB_READ | LB_WRITE);

   if(ret < 0) /* open failed */
   {
      sprintf(msg, "%s %s, data_id %d\n",
              "open_s2s_accum_data_store:",
              "Failed to open s2s accum data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LIB002_DEBUG)
            fprintf(stderr, msg);
      }

      return (FUNCTION_FAILED);
   }
   else /* successful open */
   {
      sprintf(msg, "%s %s, data_id %d\n",
              "open_s2s_accum_data_store:",
              "Opened s2s accum data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return (FUNCTION_SUCCEEDED);
   }

} /* end open_s2s_accum_data_store() ===================================== */

/******************************************************************************
    Filename: dp_s2s_accum_data_store.c

    Description:
    ============
       read_s2s_accum_data_store() reads an S2S_Accum_Buf_t from the
    <data_id> data store

    Input:  int data_id - linear buffer id
            int rec_id  - message id associated with the data

    Output: S2S_Accum_Buf_t* accum - Structure filled with the data.

    Returns: READ_OK      (0)  - successful read
             READ_FAILED  (-1) - failed     read
             NULL_POINTER (2)  - null pointer, can't write to

    Called by: CQ_Read_first(), restore_hourly(), restore_hourly_diff(),

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    10/07       0000       Cham Pham       Initial implementation.
******************************************************************************/

int read_s2s_accum_data_store(int data_id, int rec_id, S2S_Accum_Buf_t* accum)
{
   char* buffer = NULL;  /* address of buffer    */
   int   bytes_read = 0; /* number of bytes read */
   char  msg[200];       /* stderr message       */

   static unsigned int s2s_size = sizeof(S2S_Accum_Buf_t);

   /* Check for NULL pointer */

   if(pointer_is_NULL(accum, "read_s2s_accum_data_store", "accum"))
      return(NULL_POINTER);

   /* LB_ALLOC_BUF reads the entire message */

   bytes_read = RPGC_data_access_read(data_id,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) rec_id);
   if(bytes_read != s2s_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "read_s2s_accum_data_store:",
              "Failed to read s2s accum from data store",
               data_id, rec_id, bytes_read);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      if(bytes_read < 0) /* RPG error */
      {
         if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
         {
            RPGC_log_msg(GL_INFO, msg);
            if(DP_LIB002_DEBUG)
               fprintf(stderr, msg);
         }
      }

      if(buffer != NULL)
         free(buffer);

      return(READ_FAILED);
   }
   else if(buffer == NULL) /* no buffer to read */
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "read_s2s_accum_data_store:",
              "buffer returned is NULL",
               data_id, rec_id, bytes_read);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(READ_FAILED);
   }
   else /* successful read */
   {
      memcpy(accum, buffer, s2s_size);

      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "%s %s, data_id %d\n",
              "read_s2s_accum_data_store:",
              "Read from s2s accum data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(READ_OK);
   }

} /* end read_s2s_accum_data_store() ===================================== */

/******************************************************************************
    Filename: dp_s2s_accum_data_store.c

    Description:
    ============
       write_s2s_accum_data_store() writes an S2S_Accum_Buf_t to the
    <data_id> data store.

    20071025 Ning Shen says that for DUAUSERSEL, we should use a rec_id
    of LB_ANY, to get a system assigned next id, instead of a user specified
    msg id (1 to LB_MAX_ID).

    Input:  int data_id - linear buffer id
            int rec_id  - message id associated with the data.

    Output: S2S_Accum_Buf_t* accum - data to be written

    Returns: WRITE_OK     (0)  - successful write
             WRITE_FAILED (-1) - failed     write
             NULL_POINTER (2)  - null pointer, nothing to write

    Called by: scan-to-scan main(), CQ_Replace_first(), CQ_Add_to_back()

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    10/07       0000       Cham Pham       Initial implementation.
******************************************************************************/

int write_s2s_accum_data_store(int data_id, int rec_id, S2S_Accum_Buf_t* accum)
{
   int  bytes_written = 0; /* number of  bytes written */
   char msg[200];          /* stderr message           */

   static unsigned int s2s_size = sizeof(S2S_Accum_Buf_t);

   /* Check for NULL pointer */

   if(pointer_is_NULL(accum, "write_s2s_accum_data_store", "accum"))
      return(NULL_POINTER);

   bytes_written = RPGC_data_access_write(data_id,
                                          accum,
                                          s2s_size,
                                          (LB_id_t) rec_id);
   if(bytes_written != s2s_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_written %d\n",
              "write_s2s_accum_data_store:",
              "Failed to write s2s accum to data store",
               data_id, rec_id, bytes_written);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      if(bytes_written < 0) /* RPG error */
      {
         if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
         {
            RPGC_log_msg(GL_INFO, msg);
            if(DP_LIB002_DEBUG)
               fprintf(stderr, msg);
         }
      }

      return(WRITE_FAILED);
   }
   else /* successful write */
   {
      sprintf(msg, "%s %s, data_id %d\n",
              "write_s2s_accum_data_store:",
              "Wrote to s2s accum data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(WRITE_OK);
   }

} /* end write_s2s_accum_data_store() ===================================== */

/******************************************************************************
    Filename: dp_s2s_accum_data_store.c

    Description:
    ============
       init_s2s_accum_data_store() initializes an S2S_Accum_Buf_t data store.

    20071025 Ning Shen says that for DUAUSERSEL, if we specify the LB msg_size
    to be 0 in the snippet, we don't need to call init_s2s_accum_data_store()
    because the RPG will handle the allocation of space.

    Input:  data_id - linear buffer id

    Output: S2S_Accum_Buf_t - Structure to contain data.

    Return:  0 - successful write data to linear buffer
            -1 - failed to write data to linear buffer

    Called by: None

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    10/07       0000       Cham Pham       Initial implementation.
****************************************************************************/

/* int init_s2s_accum_data_store(int data_id, int num_grids_in_lb)
 * {
 *    int             i, ret = 0;
 *    S2S_Accum_Buf_t accum;
 *
 *    -* Initialize a sample S2S_Accum_Buf_t to ZERO *-
 *
 *    memset(&accum, 0, sizeof(S2S_Accum_Buf_t));
 *
 *    for (i = 0; i < num_grids_in_lb; i++)
 *    {
 *       ret = write_s2s_accum_data_store(data_id, i, &accum);
 *       if (ret == WRITE_FAILED)
 *          return ret;
 *    }
 *
 *    return ret;
 *
 * } -* end init_s2s_accum_data_store() ===================================== *-
 */

/******************************************************************************
   Filename: dp_s2s_accum_data_store.c

   Description:
   ============
   rpg_err_to_msg() converts an RPG error number to an output string.
   RPG error numbers are cryptic.

   Inputs: int error - the RPG error number

   Outputs: char* str - the output string

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   `man infr` is used to convert the 1000 or so messages into strings.
    Look for error message #defines in ~/lib/include

   Called by: restore_hourly(), restore_hourly_diff(), restore_storm(),
              restore_storm_diff(), open_lt_accum_buffers(), backup_hourly(),
              backup_hourly_diff(), backup_storm(), backup_storm_diff(),
              open_s2s_accum_data_store(), read_s2s_accum_data_store(),
              write_s2s_accum_data_store().

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   -----------   -------    -----------------  ----------------------
   17 Mar 2008   0000       Ward               Initial implementation
*****************************************************************************/

int rpg_err_to_msg(int rpg_err, char* str)
{
   /* Check for NULL pointers */

   if(pointer_is_NULL(str, "rpg_err_to_msg", "str"))
      return(NULL_POINTER);

   switch(rpg_err)
   {
      case LB_BAD_ARGUMENT: /* -41 */
        sprintf(str, "%d %s\n",
                rpg_err,
                "LB_BAD_ARGUMENT: One of the calling parameters is not valid.");
        break;

      case LB_LB_ERROR: /* -48 */
        sprintf(str, "%d %s %s\n",
                rpg_err,
                "LB_LB_ERROR: An error is found in the LB internal control data structure.",
                "The LB is likely to be corrupted.");
        break;

      case LB_NOT_FOUND: /* -56 */
        sprintf(str, "%d %s\n",
                rpg_err,
                "LB_NOT_FOUND: The message to access was not found in the LB.");
        break;

      case CS_KEY_NOT_FOUND: /* -780 */
        sprintf(str, "%d %s\n",
                rpg_err,
                "CS_KEY_NOT_FOUND: The key to search was not found.");
        break;

      default:
        sprintf(str, "%d %s\n",
                rpg_err,
                "UNKNOWN RPG error");
        break;
   }

   return(FUNCTION_SUCCEEDED);

} /* end rpg_err_to_msg() ===================================== */

/******************************************************************************
    Filename: dp_s2s_rate_data_store.c

    Description:
    ============
       open_s2s_rate_data_store() will open a data store for reading or
    writing (only) scan-to-scan rate buffers.

    Input:  data_id - DP_OLD_RATE (300001)

    Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1)

    Called by: scan-to-scan main()

    data_id is set in ~/include/orpgdat.h

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    10/07       0000       Cham Pham       Initial implementation.
******************************************************************************/

int open_s2s_rate_data_store(int data_id)
{
   int  ret = FUNCTION_SUCCEEDED;
   char msg[200]; /* stderr message */

   ret = RPGC_data_access_open(data_id, LB_READ | LB_WRITE);

   if(ret < 0) /* open failed */
   {
      sprintf(msg, "%s %s, data_id %d\n",
              "open_s2s_rate_data_store:",
              "Failed to open s2s rate data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         if(DP_LIB002_DEBUG)
            fprintf(stderr, msg);
      }

      return (FUNCTION_FAILED);
   }
   else /* successful open */
   {
      sprintf(msg, "%s %s, data_id %d\n",
              "open_s2s_rate_data_store:",
              "Opened s2s rate data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return (FUNCTION_SUCCEEDED);
   }

} /* end open_s2s_rate_data_store() ===================================== */

/******************************************************************************
    Filename: dp_s2s_rate_data_store.c

    Description:
    ============
       read_s2s_rate_data_store() reads an Rate_Buf_t from the
    <data_id> data store

    Input:  int data_id - linear buffer id
            int rec_id  - message id associated with the data

    Output: Rate_Buf_t* rate - Structure filled with the data.

    Returns: READ_OK      (0)  - successful read
             READ_FAILED  (-1) - failed     read
             NULL_POINTER (2)  - null pointer, can't write to

    Called by: scan-to-scan main()

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    10/07       0000       Cham Pham       Initial implementation.
******************************************************************************/

int read_s2s_rate_data_store(int data_id, int rec_id, Rate_Buf_t* rate)
{
   char* buffer = NULL;  /* address of buffer    */
   int   bytes_read = 0; /* number of bytes read */
   char  msg[200];       /* stderr message       */

   static unsigned int rate_size = sizeof(Rate_Buf_t);

   /* Check for NULL pointer */

   if(pointer_is_NULL(rate, "read_s2s_rate_data_store", "rate"))
      return(NULL_POINTER);

   /* LB_ALLOC_BUF reads the entire message */

   bytes_read = RPGC_data_access_read(data_id,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) rec_id);
   if(bytes_read != rate_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "read_s2s_rate_data_store:",
              "Failed to read s2s rate from data store",
               data_id, rec_id, bytes_read);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      if(bytes_read < 0) /* RPG error */
      {
         if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
         {
            RPGC_log_msg(GL_INFO, msg);
            if(DP_LIB002_DEBUG)
               fprintf(stderr, msg);
         }
      }

      if(buffer != NULL)
         free(buffer);

      return(READ_FAILED);
   }
   else if(buffer == NULL) /* no buffer to read */
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_read %d\n",
              "read_s2s_rate_data_store:",
              "buffer returned is NULL",
               data_id, rec_id, bytes_read);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(READ_FAILED);
   }
   else /* successful read */
   {
      memcpy(rate, buffer, rate_size);

      if(buffer != NULL)
         free(buffer);

      sprintf(msg, "%s %s, data_id %d\n",
              "read_s2s_rate_data_store:",
              "Read from s2s rate data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(READ_OK);
   }

} /* end read_s2s_rate_data_store() ===================================== */

/******************************************************************************
    Filename: dp_s2s_rate_data_store.c

    Description:
    ============
       write_s2s_rate_data_store() writes an Rate_Buf_t to the
    <data_id> data store.

    20071025 Ning Shen says that for DUAUSERSEL, we should use a rec_id
    of LB_ANY, to get a system assigned next id, instead of a user specified
    msg id (1 to LB_MAX_ID).

    Input:  int data_id - linear buffer id
            int rec_id  - message id associated with the data.

    Output: Rate_Buf_t* rate - data to be written

    Returns: WRITE_OK     (0)  - successful write
             WRITE_FAILED (-1) - failed     write
             NULL_POINTER (2)  - null pointer, nothing to write

    Called by: scan-to-scan main()

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    10/07       0000       Cham Pham       Initial implementation.
******************************************************************************/

int write_s2s_rate_data_store(int data_id, int rec_id, Rate_Buf_t* rate)
{
   int  bytes_written = 0; /* number of  bytes written */
   char msg[200];          /* stderr message           */

   static unsigned int rate_size = sizeof(Rate_Buf_t);

   /* Check for NULL pointer */

   if(pointer_is_NULL(rate, "write_s2s_rate_data_store", "rate"))
      return(NULL_POINTER);

   bytes_written = RPGC_data_access_write(data_id,
                                          rate,
                                          rate_size,
                                          (LB_id_t) rec_id);
   if(bytes_written != rate_size)
   {
      sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_written %d\n",
              "write_s2s_rate_data_store:",
              "Failed to write s2s rate to data store",
               data_id, rec_id, bytes_written);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      if(bytes_written < 0) /* RPG error */
      {
         if(rpg_err_to_msg(bytes_written, msg) == FUNCTION_SUCCEEDED)
         {
            RPGC_log_msg(GL_INFO, msg);
            if(DP_LIB002_DEBUG)
               fprintf(stderr, msg);
         }
      }

      return(WRITE_FAILED);
   }
   else /* successful write */
   {
      sprintf(msg, "%s %s, data_id %d\n",
              "write_s2s_rate_data_store:",
              "Wrote to s2s rate data store",
               data_id);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(WRITE_OK);
   }

} /* end write_s2s_rate_data_store() ===================================== */
