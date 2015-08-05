/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 23:07:32 $
 * $Id: dp_lt_accum_circ_q.c,v 1.4 2009/10/27 23:07:32 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Initialize() initializes a circular queue. Sets first and last indexes
    to CQ_EMPTY. Sets all bias flags to FALSE.

    Inputs: Circular_Queue_t *circular_queue - the queue to initialize

    Outputs: The queue, initialized.

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    Called by: init_hourly(), init_hourly_diff()

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/23/2007     0000      Stein, Ward        Initial implementation
******************************************************************************/

int CQ_Initialize (Circular_Queue_t *circular_queue, int data_id, int max_queue)
{
   short i;
   char  msg[200];

   /* Check for NULL pointers */

   if(pointer_is_NULL(circular_queue, "CQ_Initialize", "circular_queue"))
      return(NULL_POINTER);

   circular_queue->data_id = data_id;

   circular_queue->first = CQ_EMPTY;
   circular_queue->last  = CQ_EMPTY;

   /* We can't have a queue larger than our allocated disk storage.
    *
    * DP_HRLY_ACCUM_MAXN_MSGS, defined in dp_lt_accum_Consts.h, is 30  */

   if((max_queue < 1) || (max_queue > DP_HRLY_ACCUM_MAXN_MSGS))
   {
      sprintf(msg, "%s max_queue %d %s %d\n",
                   "CQ_Initialize:",
                    max_queue,
                   "< 1 or > DP_HRLY_ACCUM_MAXN_MSGS",
                   DP_HRLY_ACCUM_MAXN_MSGS);

      RPGC_log_msg(GL_INFO, msg);
      if (DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      return (CQ_FAILURE);
   }
   else
      circular_queue->max_queue = max_queue;

   for(i=0; i<circular_queue->max_queue; i++)
   {
      circular_queue->begin_time[i]     = 0L;
      circular_queue->end_time[i]       = 0L;
      circular_queue->missing_period[i] = FALSE;
   }

   circular_queue->trim_time = 0L;

   return (CQ_SUCCESS);

} /* end CQ_Initialize() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Read_first() reads off the first accum buffer in the circular_queue and
    puts it in accum_buf; first index unchanged.

    Inputs: Circular_Queue_t* circular_queue - queue to read
            S2S_Accum_Buf_t*      accum_buf  - where to read it to

    Outputs: First accum buffer read into accum_buf

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/23/2007     0000      Stein, Ward        Initial implementation
******************************************************************************/

int CQ_Read_first (Circular_Queue_t *circular_queue, S2S_Accum_Buf_t *accum_buf)
{
   int ret   = READ_FAILED; /* return value             */
   int first = CQ_EMPTY;    /* makes referencing easier */

   /* Check for NULL pointers */

   if(pointer_is_NULL(circular_queue, "CQ_Read_first", "circular_queue"))
      return(NULL_POINTER);

   if(pointer_is_NULL(accum_buf, "CQ_Read_first", "accum_buf"))
      return(NULL_POINTER);

   first = circular_queue->first;

   if (first != CQ_EMPTY) /* queue not empty */
   {
      /* Read from the scan-to-scan data store. *
       * Note: data store rec_ids start at 1, so we add 1 to the value of
       * first.
       */

      ret = read_s2s_accum_data_store(circular_queue->data_id,
                                      first + 1,
                                      accum_buf);
      if(ret != READ_OK)
         return (CQ_FAILURE);

      /* Sync up missing period, for safety */

      circular_queue->missing_period[first] = accum_buf->supl.missing_period_flg;

      return (CQ_SUCCESS);
   }  /* end queue is not empty */

   /* If we got here, first == CQ_EMPTY -> CQ is empty; return an error */

   if ( DP_LT_ACCUM_DEBUG )
   {
      fprintf (stderr, "%s %s\n",
                       "CQ_Read_first:",
                       "attempt to read from an empty queue");
   }

   return (CQ_FAILURE);

} /* end CQ_Read_first() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Replace_first() replaces the first element of the queue.  Overwrites
    the first accum buffer in the circular_queue with a new one; dates and
    times are updated, but first index unchanged

    Inputs: Circular_Queue_t* circular_queue - the queue
            S2S_Accum_Buf_t*  accum_buf      - what to replace the first
                                               element with

    Outputs: Returns CQ_SUCCESS if successful, CQ_FAILURE if the CQ is empty.

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/23/2007     0000      Stein, Ward        Initial implementation
******************************************************************************/

int CQ_Replace_first (Circular_Queue_t *circular_queue,
                      S2S_Accum_Buf_t *accum_buf)
{
   int ret   = WRITE_FAILED; /* return value             */
   int first = CQ_EMPTY;     /* makes referencing easier */

   /* Check for NULL pointers */

   if(pointer_is_NULL(circular_queue, "CQ_Replace_first", "circular_queue"))
      return(NULL_POINTER);

   if(pointer_is_NULL(accum_buf, "CQ_Replace_first", "accum_buf"))
      return(NULL_POINTER);

   first = circular_queue->first;

   if (first != CQ_EMPTY) /* queue not empty */
   {
      /* Write to the scan-to-scan data store.
       * Note: data store rec_ids start at 1. */

      ret = write_s2s_accum_data_store(circular_queue->data_id,
                                       first + 1,
                                       accum_buf);
      if(ret != WRITE_OK)
         return (CQ_FAILURE);

      /* Copy the times from the accum_buf */

      circular_queue->begin_time[first] = accum_buf->supl.begin_time;
      circular_queue->end_time[first]   = accum_buf->supl.end_time;

      /* Sync up missing period, for safety */

      circular_queue->missing_period[first] = accum_buf->supl.missing_period_flg;

      return (CQ_SUCCESS);

   } /* end queue is not empty */

   /* If we got here, first == CQ_EMPTY -> CQ is empty; return an error */

   if ( DP_LT_ACCUM_DEBUG )
   {
      fprintf (stderr, "%s %s\n",
                       "CQ_Replace_first:",
                       "attempt to read from an empty queue");
   }

   return (CQ_FAILURE);

} /* end CQ_Replace_first() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Delete_first() increments first index to the next slot in the
    circular_queue, effectively "deleting" the first element in the queue.

    Inputs: Circular_Queue_t* circular_queue - the queue

    Outputs: The first element "deleted".

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/23/2007     0000      Stein, Ward        Initial implementation
******************************************************************************/

int CQ_Delete_first (Circular_Queue_t* circular_queue)
{
   int first = CQ_EMPTY; /* makes referencing easier */
   int last  = CQ_EMPTY; /* makes referencing easier */

   /* Check for NULL pointers */

   if(pointer_is_NULL(circular_queue, "CQ_Delete_first", "circular_queue"))
      return(NULL_POINTER);

   first = circular_queue->first;
   last  = circular_queue->last;

   if (first != CQ_EMPTY) /* queue not empty */
   {
      /* Reset the missing period flag, for safety */

      circular_queue->missing_period[first] = FALSE;

      if(first == last) /* was only one item in queue */
      {
         circular_queue->first = CQ_EMPTY; /* now no items in queue */
         circular_queue->last  = CQ_EMPTY; /* now no items in queue */
      }
      else /* delete the first by incrementing the first index */
      {
         circular_queue->first =(first + 1) % circular_queue->max_queue;
      }

      return (CQ_SUCCESS);

   } /* end queue is not empty */

   /* If we got here, first == CQ_EMPTY -> CQ is empty; return an error */

   if ( DP_LT_ACCUM_DEBUG )
   {
      fprintf (stderr, "%s %s\n",
                       "CQ_Delete_first:",
                       "attempt to delete from an empty queue.\n");
   }

   return (CQ_FAILURE);

} /* end CQ_Delete_first() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Get_time_span() returns the length of time covered by the circular_queue
    (last->end - first->begin) in seconds.

    Inputs: Circular_Queue_t *circular_queue - the queue

    Outputs: Time span in minutes if successful, CQ_FAILURE if the CQ is empty.

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    NOTE: Even though this function does not alter the contents of the circular
    queue, it receives a pointer to the queue.  Future programmers beware - do
    NOT inadvertently alter the queue! A pointer was passed to make the
    function more efficient by putting less data on the stack.

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/23/2007     0000      Stein, Ward        Initial implementation
******************************************************************************/

int CQ_Get_time_span (Circular_Queue_t* circular_queue, long* time_span)
{
   int  first      = CQ_EMPTY; /* makes referencing easier   */
   int  last       = CQ_EMPTY; /* makes referencing easier   */
   long queue_span = 0L;       /* span based on queue only   */
   long trim_span  = 0L;       /* span from when trim called */

   /* Check for NULL pointers */

   if(pointer_is_NULL(circular_queue, "CQ_Get_time_span", "circular_queue"))
      return(NULL_POINTER);

   if(pointer_is_NULL(time_span, "CQ_Get_time_span", "time_span"))
      return(NULL_POINTER);

   first = circular_queue->first;
   last  = circular_queue->last;

   if (first != CQ_EMPTY) /* queue is not empty */
   {
      /* queue_span = time span based on queue only */

      queue_span = circular_queue->end_time[last] -
                   circular_queue->begin_time[first];

      /* trim_span= time span based on when trim is called.
       *
       * CQ_Get_time_span() is called by CQ_Trim_To_Hour(). 
       * Check to see if the time when the CQ_Trim_To_Hour() was called 
       * to trim isn't longer. This will happen if there's accum bufs 
       * in the queue, and suddenly the precip stops. The queue will
       * remain the same, but time marches on, and we still want only 
       * the last hour. */

      trim_span = circular_queue->trim_time - 
                  circular_queue->begin_time[first];

      /* Take the longer */

      if(trim_span > queue_span)
         *time_span = trim_span;
      else
         *time_span = queue_span;

      return (CQ_SUCCESS);

   } /* end if queue is not empty */

   /* If we got here, first == CQ_EMPTY -> CQ is empty; return an error */

   if (DP_LT_ACCUM_DEBUG)
   {
      fprintf (stderr, "%s %s\n",
                       "CQ_get_time_span:",
                       "attempt to read from an empty queue");
   }

   return (CQ_FAILURE);

} /* end CQ_Get_time_span() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Add_to_back() adds an element to the back of a queue. Adds a new
    accum_buf after the last item in the circular_queue and increments the
    last index. Optional Biased and Unbiased arrays are passed in. If the
    queue was empty, they are initialized in preparating to the accum_buf
    being added to them outside this function.

    Inputs: Circular_Queue_t* circular_queue       - the queue
            S2S_Accum_Buf_t*  accum_buf            - the element to add
            int               Biased[][MAX_BINS]   - Biased array
            int               Unbiased[][MAX_BINS] - Unbiased array

    Outputs: The accum buffer added to the back of the queue

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    Called by: compute_hourly(), compute_hourly_diff().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/23/2007     0000      Stein, Ward        Initial implementation
******************************************************************************/

int CQ_Add_to_back (Circular_Queue_t *circular_queue,
                    S2S_Accum_Buf_t *accum_buf,
                    int Biased[][MAX_BINS], int Unbiased[][MAX_BINS])
{
   int  i, j;                 /* counters                 */
   int  ret   = WRITE_FAILED; /* return value             */
   int  first = CQ_EMPTY;     /* makes referencing easier */
   int  last  = CQ_EMPTY;     /* makes referencing easier */
   char msg[200];             /* stderr message           */

   /* Check for NULL pointers. Biased/Unbiased can be NULL. */

   if(pointer_is_NULL(circular_queue, "CQ_Add_to_back", "circular_queue"))
      return(NULL_POINTER);

   if(pointer_is_NULL(accum_buf, "CQ_Add_to_back", "accum_buf"))
      return(NULL_POINTER);

   first = circular_queue->first;
   last  = circular_queue->last;

   if(first != CQ_EMPTY) /* queue not empty */
   {
     /* Increment last */

     circular_queue->last = (last + 1) % circular_queue->max_queue;

     if (circular_queue->last == first) /* OVERFLOW!!! */
     {
        if ( DP_LT_ACCUM_DEBUG )
        {
           sprintf(msg, "%s %s\n",
                        "CQ_Add_to_back:",
                        "queue OVERFLOW!!!");

           RPGC_log_msg(GL_INFO, msg);
           if(DP_LT_ACCUM_DEBUG)
              fprintf(stderr, msg);
        }

        /* Reset last to its previous value before returning */

        circular_queue->last -= 1;
        if (circular_queue->last < 0) /* Correct for possible neg number */
           circular_queue->last = circular_queue->max_queue - 1;

        return (CQ_FAILURE);

     } /* end check if last == first */
   }  /* if(first != CQ_EMPTY) */
   else /* CQ was empty; we're adding the 1st item */
   {
      circular_queue->first = 0;
      circular_queue->last  = 0;

      if(Biased != NULL)
      {
         for(i = 0; i < MAX_AZM; i++)
           for(j = 0; j < MAX_BINS; j++)
              Biased[i][j] = QPE_NODATA;
      }

      if(Unbiased != NULL)
      {
         for(i = 0; i < MAX_AZM; i++)
           for(j = 0; j < MAX_BINS; j++)
              Unbiased[i][j] = QPE_NODATA;
      }
   }  /* else CQ was empty */

   /* Copy the dates and times from the accum_buf */

   last = circular_queue->last;

   circular_queue->begin_time[last] = accum_buf->supl.begin_time;
   circular_queue->end_time[last]   = accum_buf->supl.end_time;

   /* Copy metadata */

   circular_queue->missing_period[last] = accum_buf->supl.missing_period_flg;

   /* Write to the scan-to-scan data store. *
    * Note: data store rec_ids start at 1.  */

   ret = write_s2s_accum_data_store(circular_queue->data_id,
                                    last + 1,
                                    accum_buf);
   if(ret != WRITE_OK)
      return (CQ_FAILURE);

   return (CQ_SUCCESS);

} /* end CQ_Add_to_back() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_First_needs_interpolation() returns TRUE if the first entry
    will need interpolation, FALSE if it does not, or if there is no
    first entry.

    Inputs: Circular_Queue_t* circular_queue - the queue
            time_t*           interp_time    - the time to start interpolation

    Outputs: none

    Returns: FALSE (0), TRUE (1), NULL_POINTER (2)

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/30/2007     0000      Ward               Initial implementation
******************************************************************************/

int CQ_First_needs_interpolation (Circular_Queue_t* circular_queue,
                                  time_t* interp_time)
{
   int    first      = CQ_EMPTY; /* makes referencing easier    */
   int    last       = CQ_EMPTY; /* makes referencing easier    */
   time_t queue_time = 0L;       /* time based on queue only    */
   time_t trim_time  = 0L;       /* time based when trim called */

   /* Check for NULL pointers. */

   if(pointer_is_NULL(circular_queue, "CQ_First_needs_interpolation", "circular_queue"))
      return(NULL_POINTER);

   if(pointer_is_NULL(interp_time, "CQ_First_needs_interpolation", "interp_time"))
      return(NULL_POINTER);

   first = circular_queue->first;
   last  = circular_queue->last;

   if (first != CQ_EMPTY)  /* queue is not empty */
   {
      /* queue_time = one hour before queue end time */

      queue_time = circular_queue->end_time[last] - SECS_PER_HOUR;

      /* trim_time = one hour before trim was called.
       *
       * CQ_First_needs_interpolation() is called by CQ_Trim_To_Hour(). 
       * Check to see if the time when the CQ_Trim_To_Hour() was called 
       * to trim isn't longer. This will happen if there's accum bufs 
       * in the queue, and suddenly the precip stops. The queue will 
       * remain the same, but time marches on, and we still want only 
       * the last hour. */

      trim_time = circular_queue->trim_time - SECS_PER_HOUR;

      /* Take the longer time */

      if(trim_time > queue_time)
         *interp_time = trim_time;
      else
         *interp_time = queue_time;

      /* If the interpolation time falls inside the first accum grid,
       * return TRUE, otherwise return FALSE */

      if ((*interp_time > circular_queue->begin_time[first]) &&
          (*interp_time < circular_queue->end_time[first]))
      {
         return (TRUE);
      }
      else
      {
         return (FALSE);
      }
   } /* end queue is not empty */

   /* If we got here, first == CQ_EMPTY -> CQ is empty;
    * return FALSE (not an error) */

   if (DP_LT_ACCUM_DEBUG)
   {
      fprintf (stderr, "%s %s\n",
                       "CQ_First_needs_interpolation:",
                       "attempt to read from an empty queue");
   }

   return (FALSE);

} /* end CQ_First_needs_interpolation() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Trim_To_Hour() will trim the queue to only hold 1 hour's worth of
    accum buf. It will adjust 1 biased and 1 unbiased array as it trims,
    so the database and the arrays stay in sync.

    Inputs: Circular_Queue_t* circular_queue       - the queue
            int               Biased[][MAX_BINS]   - biased array to trim
            int               Unbiased[][MAX_BINS] - unbiased array to trim
            int               max_grid             - maximum grid value

            If Biased or Unbiased are NULL nothing will be done to them.

    Outputs: TRUE/FALSE, and the trimmed arrays

    Returns: CQ_SUCCESS (0), CQ_FAILURE (1), NULL_POINTER (2)

    Called by: compute_hourly(), compute_hourly_diff()

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/30/2007     0000      Ward               Initial implementation
******************************************************************************/

int CQ_Trim_To_Hour(Circular_Queue_t *circular_queue,
                    int    Biased[][MAX_BINS],
                    int    Unbiased[][MAX_BINS],
                    int    max_grid,
                    time_t trim_time)
{
   int             ret         = CQ_FAILURE; /* function return value */
   time_t          interp_time = 0L;
   long            time_span   = 0L;
   S2S_Accum_Buf_t first_s2s;

   /* Check for NULL pointers. Biased/Unbiased can be NULL. */

   if(pointer_is_NULL(circular_queue, "CQ_Trim_To_Hour", "circular_queue"))
      return(NULL_POINTER);

   /* Set the trim_time, when CQ_Trim_To_Hour() was called */ 

   circular_queue->trim_time = trim_time;

   /* Get the time span on every loop iteration */

   while(1)
   {
      ret = CQ_Get_time_span(circular_queue, &time_span);

      /* If the time span is less than one hour or if we didn't get a good
       * time span, exit the loop */

      if((time_span <= SECS_PER_HOUR) || (ret != CQ_SUCCESS))
         break;

      /* If we got here, then the queue is longer than an hour.
       * Get the first accum buf so we can subtract grids */

      if (CQ_Read_first(circular_queue, &first_s2s) != CQ_SUCCESS)
      {
         break; /* no first scan-to_scan accum_buf */
      }

      /* If we got here, we got an accum_buf.
       * Subtract the first grid from the biased array.  */

      if(Biased != NULL)
      {
         if (first_s2s.qpe_adapt.adj_adapt.bias_flag == TRUE)
         {
            subtract_biased_short_from_int(Biased,
                                           first_s2s.accum_grid,
                                           first_s2s.qpe_adapt.bias_info.bias);

            /* Because Dual Pol is NOT applying bias,
             * this should not happen, so print an error */

            if (DP_LT_ACCUM_DEBUG)
            {
               fprintf(stderr, "%s %s%s\n",
                               "CQ_Trim_To_Hour:",
                               "first_s2s.qpe_adapt.adj_adapt.bias_flag",
                               " == TRUE!!!");
            }
         }
         else /* the bias is off */
         {
            subtract_unbiased_short_from_int(Biased, first_s2s.accum_grid);
         }
      } /* end biased grid handling */

      /* Subtract the first grid from the unbiased array */

      if(Unbiased != NULL)
      {
         subtract_unbiased_short_from_int(Unbiased, first_s2s.accum_grid);

      } /* end unbiased grid handling */

      /* If the first accum buf has a missing period, delete it,          *
       * as we don't want to start on a missing period.                   *
       *                                                                  *
       * Else if removing the first accum buf would give us < 60 minutes, *
       * then the first grid needs to be interpolated and kept into       *
       * the arrays.                                                      *
       *                                                                  *
       * Else just delete the accum buf.                                  */

      if (first_s2s.supl.missing_period_flg)
      {
         if (CQ_Delete_first(circular_queue) != CQ_SUCCESS)
         {
            if (DP_LT_ACCUM_DEBUG)
            {
               fprintf (stderr, "%s %s %s\n",
                                "CQ_Trim_To_Hour:",
                                "missing period,",
                                "CQ_Delete_first() did not return CQ_SUCCESS");
            }
         }
      } /* end if first scan-to_scan had a missing period */
      else if (CQ_First_needs_interpolation(circular_queue, &interp_time) == TRUE)
      {
         /* What to do if interpolate_grid() returns FUNCTION_FAILED or
          * NULL_POINTER? */

         interpolate_grid(&first_s2s, interp_time, INTERP_FORWARD);

         if (Biased != NULL)
         {
            if (first_s2s.qpe_adapt.adj_adapt.bias_flag == TRUE)
            {
               add_biased_short_to_int(Biased,
                                       first_s2s.accum_grid,
                                       first_s2s.qpe_adapt.bias_info.bias,
                                       max_grid);

               /* Because Dual Pol is not applying bias,
                * this should not happen, so print an error
                */

               if (DP_LT_ACCUM_DEBUG)
               {
                  fprintf(stderr, "%s %s%s\n",
                                  "CQ_Trim_To_Hour:",
                                  "first_s2s.qpe_adapt.adj_adapt.bias_flag",
                                  " == TRUE!!!");
               }
            }
            else /* the bias is off */
            {
               add_unbiased_short_to_int(Biased, first_s2s.accum_grid, max_grid);
            }
         }

         if (Unbiased != NULL)
         {
             add_unbiased_short_to_int(Unbiased, first_s2s.accum_grid, max_grid);
         }

         /* Don't delete the first grid, just replace it. */

         if (CQ_Replace_first(circular_queue, &first_s2s) != CQ_SUCCESS)
         {
            if (DP_LT_ACCUM_DEBUG)
            {
               fprintf(stderr, "%s %s %s\n",
                               "CQ_Trim_To_Hour:",
                               "interpolation,",
                               "CQ_Replace_first() did not return CQ_SUCCESS");
            }
         }  /* if (CQ_Replace_first (..) */

         break; /* no need to look any more */

      } /* end else if first needed interpolation */
      else /* the first didn't need interpolation */
      {
         if (CQ_Delete_first(circular_queue) != CQ_SUCCESS)
         {
            if (DP_LT_ACCUM_DEBUG)
            {
               fprintf (stderr, "%s %s %s\n",
                                "CQ_Trim_To_Hour:",
                                "no interpolation,",
                                "CQ_Delete_first() did not return CQ_SUCCESS");
            }
         }

      } /* end else first didn't need interpolation */

   } /* end while loop over get time span */

   return (CQ_SUCCESS);

} /* end CQ_Trim_To_Hour() =================================== */

/******************************************************************************
    Filename: dp_lt_accum_circ_q.c

    Description:
    ============
    CQ_Get_Missing_Period() gets the missing period flag for the queue.
    If one of the buffers had a missing period flag set, it returns TRUE,
    otherwise it returns false.

    Inputs: Circular_Queue_t* circular_queue - the queue

    Outputs: The missing period flag.

    Returns: FALSE (0), TRUE (1)

    Called by: compute_hourly(), compute_hourly_diff(),
               restore_hourly(), restore_hourly_diff().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    10/30/2007     0000      Ward               Initial implementation
******************************************************************************/

int CQ_Get_Missing_Period(Circular_Queue_t *circular_queue)
{
   int i;               /* loop counter             */
   int ret  = FALSE;    /* return value             */
   int last = CQ_EMPTY; /* makes referencing easier */

   /* Check for NULL pointers. */

   if(pointer_is_NULL(circular_queue, "CQ_Get_Missing_Period", "circular_queue"))
   {
      /* Instead of returning NULL_POINTER, we return FALSE */

      return(FALSE);
   }

   i    = circular_queue->first;
   last = circular_queue->last;

   if(i == CQ_EMPTY) /* queue is empty */
     return(FALSE);

   /* If we got here, the queue is not empty.
    *
    * This while loop inspects all but the last */

   while (i != last)
   {
      ret |= circular_queue->missing_period[i];
      i = (i + 1) % circular_queue->max_queue;
   }

   /* Inspect the last */

   ret |= circular_queue->missing_period[last];

   return (ret);

} /* end CQ_Get_Missing_Period() =================================== */
