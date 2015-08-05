/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/11/05 16:27:51 $
 * $Id: dp_dua_accum_func.c,v 1.11 2012/11/05 16:27:51 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <dp_lib_func_prototypes.h>
#include "dp_dua_accum_common.h"
#include "dp_dua_accum_consts.h"
#include "dp_dua_accum_func_prototypes.h"
#include "dp_dua_accum_types.h"

/******************************************************************************
   Function name: task_handler()

   Description:
   ============
      It reads user requests; then for every request, queries the database; then
      computes the accumulation during user-specified time period; then builds
      the output product.	

	  Return:
      1 - OK; -1 - error		
	
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/	

int task_handler()
{
    int num_requests = 0;
    int i = 0;
    int opstat = RPGC_NORMAL;
    S2S_Accum_Buf_t* inbuf = NULL;
    int vol_num = 0; /* volume scan number, which is used when building the
                      * final product */

    User_array_t * request_list = NULL;

    prod_dep_para_t prod_dep; /* structure of product dependent parameters */

    int dua_accum_grid[MAX_AZM][MAX_BINS]; /* accumulation during user-specified
                                              time period */

    /* start_time/end_time are user-specified start/end time */

    time_t start_time = 0L; /* set to avoid valgrind warning */
    time_t end_time   = 0L; /* set to avoid valgrind warning */

    int ret; /* hold return value of a function for program logic control */
    void *query_result = NULL; /* hold the database query result */
    int num_record = 0; /* number of records in the query result */

    dua_accum_buf_metadata_t metadata;  /* hold the metadata part of a message
                                           sent to the output buffer */

    /* year, month, day, hour, minute, second are only used once in converting
     * unix time to YMDHMS,of which, year, month, and day are passed to
     * user_time_to_unix_time() later on */

    int year   = 0; /* set to avoid valgrind warning */
    int month  = 0; /* set to avoid valgrind warning */
    int day    = 0; /* set to avoid valgrind warning */
    int hour   = 0; /* set to avoid valgrind warning */
    int minute = 0; /* set to avoid valgrind warning */
    int second = 0; /* set to avoid valgrind warning */

    /* get input data(172) from upstream task */

    inbuf = (S2S_Accum_Buf_t *) RPGC_get_inbuf_by_name((char *) IN_PROD_NAME,
                                                                &opstat);
    if (inbuf == NULL || opstat != RPGC_NORMAL)
    {
        RPGC_log_msg(GL_INFO, "RPGC_get_inbuf_by_name() error");

        if(inbuf != NULL)
            RPGC_rel_inbuf(inbuf);

        /* Replace RPGC_cleanup_and_abort() with RPGC_abort() until
         * ROC fixes bug.
         * RPGC_cleanup_and_abort(opstat); */

        RPGC_abort();
        return -1;
    }

    /* The end_time field in inbuf->dua_query could be less than or equal
       to 0, in such case, it would not be possible to get valid
       time stamp components (year, month, day) from input by using
       RPGCS_unix_time_to_ymdhms, so as to to constitute the end time
       later on. We need to abort. */

    if (inbuf->dua_query.end_time <= (time_t) 0)
    {
        RPGC_log_msg(GL_INFO, "end time in input message <=0, error");

        if(inbuf != NULL)
           RPGC_rel_inbuf(inbuf);

        /* Replace RPGC_cleanup_and_abort() with RPGC_abort() until
         * ROC fixes bug.
         * RPGC_cleanup_and_abort(PGM_INPUT_DATA_ERROR); */

        RPGC_abort();
        return -1;
    }
    else /* inbuf->dua_query.end_time > 0 */
    {
        /* Use the time info in inbuf to obtain year, month,day, hour, minute,
              second, and pass them to user_time_to_unix_time() later on */

        ret = RPGCS_unix_time_to_ymdhms(inbuf->dua_query.end_time, &year, &month,
                                        &day, &hour, &minute, &second);
        if(ret != 0)
        {
            RPGC_log_msg(GL_INFO, "error calling RPGCS_unix_time_to_ymdhms");

            if(inbuf != NULL)
               RPGC_rel_inbuf(inbuf);

            /* Replace RPGC_cleanup_and_abort() with RPGC_abort() until
             * ROC fixes bug.
             * RPGC_cleanup_and_abort(PGM_INPUT_DATA_ERROR); */

            RPGC_abort();
            return -1;
        }
    } /* end inbuf->dua_query.end_time > 0 */

    /* get volume scan number, whcih is used when building final products */

    vol_num = RPGC_get_buffer_vol_num((void *)inbuf);

    /* Read user requests */

    num_requests = get_user_requests(&request_list);

    if (num_requests <= 0)
    {
        RPGC_log_msg (GL_INFO,
                      "Get_user_requests() error, num_requests %d",
                      num_requests);

        RPGC_rel_inbuf(inbuf); /* newly added */
        /* RPGC_abort(); */
        RPGC_cleanup_and_abort(PGM_INVALID_REQUEST);
        return -1;
    }

    /* Process request loop */
    for (i = 0; i < num_requests; i++)
    {
        if(DP_DUA_ACCUM_DEBUG)
        {
           fprintf(stderr, "enter the request loop, total request = %d\n",
                            num_requests);
        }

        /* initialization for each request */

        memset((void *) dua_accum_grid, 0, INT_AZM_BINS);

        /* set members of the product-dependent-parameter structure to 0 */

        reset_prod_dep(&prod_dep);

        /*convert user request to start time and end time in unix-time format */

        ret = user_time_to_unix_time(request_list + i, &start_time, &end_time,
                           &prod_dep, year, month, day, hour, minute, second);
        if (ret == -1)
        {
            if(DP_DUA_ACCUM_DEBUG)
            {
               fprintf(stderr, "ret == -1\n");
            }
            RPGC_log_msg (GL_INFO, ">> user_time_to_unix_time() error");
            RPGC_rel_inbuf(inbuf); /* newly added */
            RPGC_abort_request(request_list + i, PGM_INVALID_REQUEST);
            return -1;
        }

        /* write start time and end time to metadata part of the message */

        metadata.start_time = start_time;
        metadata.end_time   = end_time;

        /* query the database */

        ret = query_DB(DUAUSERSEL, start_time, end_time, &query_result,
                      &num_record);

        if (ret == -1) /* make a null product */
        {
            RPGC_log_msg (GL_INFO, ">> query_DB() found no results");

            metadata.missing_period_flag = 0;
            metadata.null_product_flag   = NULL_REASON_3;
            metadata.bias                = 0.0;

            write_to_output_product((void*) dua_accum_grid, metadata,
                                     request_list + i, &prod_dep, vol_num,
                                     inbuf);
            continue;
        }

        /* Compute the accumulation during user-specified time period */

        ret = compute_dua_accum_grid(query_result, num_record, dua_accum_grid,
                                     &metadata);

        if (ret == -1) /* a null product */
        {
            metadata.missing_period_flag = 0;
            metadata.null_product_flag   = NULL_REASON_2;
            metadata.bias                = 0.0;

            write_to_output_product((void*) dua_accum_grid, metadata,
                                     request_list + i, &prod_dep, vol_num,
                                     inbuf);
            continue;
        }

        /* Write the result to the output product under normal circumstances */

        write_to_output_product((void*) dua_accum_grid, metadata,
                                 request_list + i, &prod_dep, vol_num,
                                 inbuf);

    } /* end of for (i = 0; i < num_requests; i++) */

    if(request_list != NULL)
       free(request_list);

    RPGC_rel_inbuf(inbuf); /* moved down here */

    return 1;

}  /* end of task_handler() */

/******************************************************************************
   Function name: get_user_requests()

   Description:
   ============
      It reads user request information, i.e., end time and time span in minutes.

   Inputs:	

   Outputs:	
      request list

   Return:
      number of request; -1 - error	
	
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
  13 Sep 2010    0001       James Ward         Limit number of product requests
                                               in ~/cfg/product_generation_tables
                                               to 6 (2, 3, 6, 12, 24 hrs). If
                                               num_reqs is passed in as 0, the
                                               default of 10 is used.
******************************************************************************/	

int get_user_requests(User_array_t * * request_list)
{
    int num_reqs = 6;

    /* Read user requests */

    *request_list = (User_array_t *) RPGC_get_customizing_data(-1, &num_reqs);

    if (request_list == NULL)
    {
        RPGC_log_msg(GL_INFO, ">>Get_user_requests(): no requests found\n");
        return -1;
    }

    return num_reqs;

}  /* end of Get_user_requests() */

/******************************************************************************
   Function name: user_time_to_unix_time()

   Description:
   ============
      It converts a user request to start time and end time in the format
      of unix-time.

   Inputs:
      user request; year, month, day, hour, minute, second,
      pointer of type prod_dep_para_t

   Outputs:	
      start time, end time

   Return:
      0 - OK, -1 - error		

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/	

int user_time_to_unix_time(User_array_t * user_request,
                           time_t *start_time, time_t *end_time,
                           prod_dep_para_t * prod_dep,
                           int year, int month, int day,
                           int hour, int minute, int second)
{
    int user_specified_end_time_in_second;
    int span = user_request->ua_dep_parm_1;

    int ret;

    int julian_date;

    /* Used ONLY for function that converts the format of start time from
     * Unix Time to YMDHMS, so as to fill in prod_dep->param_6 */

    int year_temp, month_temp, day_temp, hour_temp, minute_temp, second_temp;

    if (user_request->ua_dep_parm_0 == -1)
        user_specified_end_time_in_second = hour * MINS_PER_HOUR *
                          SECS_PER_MINUTE + minute * SECS_PER_MINUTE + second;
    else
        user_specified_end_time_in_second = user_request->ua_dep_parm_0 *
                                            SECS_PER_MINUTE;

    /* Compute the end_time by using YMD from the inbuf and
     * user_specified_end_time_in_second  */

    ret = RPGCS_ymdhms_to_unix_time(end_time, year, month, day,
           user_specified_end_time_in_second / SECS_PER_MINUTE / MINS_PER_HOUR,
           user_specified_end_time_in_second / SECS_PER_MINUTE % MINS_PER_HOUR,
           user_specified_end_time_in_second % SECS_PER_MINUTE);

    if (ret == -1)
    {
        RPGC_log_msg(GL_INFO,
                     ">>user specified time error, ret %d\n",
                     ret);
        return -1;
    }

    /* Compute the end_time in Unix_time format
     *
     * If user_specified_end_time_in_second > the end_time quantity
     * obtained from the triggering inbuf, which is represented using
     * hour-minute-second. In this case, we simply subtract 1 day from
     * the already computed end_time */

    if (hour * MINS_PER_HOUR * SECS_PER_MINUTE + minute * SECS_PER_MINUTE
             + second < user_specified_end_time_in_second)
    {
        *end_time -= SECONDS_PER_DAY;

        /* Recompute year_temp, month_temp, day_temp, which are used
         * by halfword 48. */

        RPGCS_unix_time_to_ymdhms(*end_time, &year_temp, &month_temp, &day_temp,
                                  &hour_temp, &minute_temp, &second_temp);
    }
    else /* use year, month, day passed in */
    {
        year_temp  = year;
        month_temp = month;
        day_temp   = day;
    }

    /* compute start_time */

    *start_time = *end_time - span * SECS_PER_MINUTE;

    /* put time related info into the product dependent structure */

    /* halfword 27 - End time in minutes */

    prod_dep->param_1 = check_time((short) (user_specified_end_time_in_second /
                                    SECS_PER_MINUTE));

    /* halfword 28 - Time span in minutes */

    prod_dep->param_2 = check_time_span((short) span);

    /* halfword 48 - End date (Julian date) */

    RPGCS_date_to_julian(year_temp, month_temp, day_temp, &julian_date);

    prod_dep->param_5 = check_date((short) julian_date);

    /* halfword 49 - Start time, in minutes */

    RPGCS_unix_time_to_ymdhms(*start_time, &year_temp, &month_temp, &day_temp,
                              &hour_temp, &minute_temp, &second_temp);

    prod_dep->param_6 = check_time((short) (minute_temp + (60 * hour_temp)));

    return 0;
}

/******************************************************************************
   Function name: query_DB()

   Description:
   ============
      It queries the scan-to-scan accumulation database based on
      user specified start time and end time.

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
******************************************************************************/	

int query_DB(int data_id, time_t start_time, time_t end_time,
             void **query_result, int *number_record_p)
{
    char where[300]; /* hold the clause in the SQL-like API */

    int num_record = 0; /* number of records */

    sprintf(where, "end_time > %d and begin_time < %d", (int) start_time,
                                                        (int) end_time);

    num_record = RPGC_DB_select(data_id, where, query_result);

    if (num_record <= 0)
    {
        RPGC_log_msg(GL_INFO,
                     ">> DB query found no results, num_record %d\n",
                     num_record);
        return -1;
    }

    *number_record_p = num_record;

    return num_record;
}

/******************************************************************************
   Function name: compute_dua_accum_grid ()

   Description:
   ============
      It computes accumulation during the user-specified time period

   Inputs:
      query result, number of records, pointer of type dua_accum_buf_metadata_t

   Outputs:
      dua_accum_grid

   Return:
      0 - OK, -1 - error	

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/	

int compute_dua_accum_grid(void * query_result, int num_record,
                           int dua_accum_grid[][MAX_BINS],
                           dua_accum_buf_metadata_t * metadata_p)
{
    int    i;
    double apply_bias = 0.0;/* A flag to tell us whether or not to apply bias */

    int ret; /* hold the return value of a function for program logic control */
    char * record; /* hold one record in the query result */
    S2S_Accum_Buf_t * accum_data; /* hold one record in query results in format
                                   * of structure S2S_Accum_Buf_t */
    /* the total sum of bias from the grids in query result */

    float bias_sum = 0.0;

    /* the total number of grids in the query result, excluding those with
       ST_active flag == 0 ; and the first / last grid with
       the missing_period_flag == 1.0 */

    int num_of_record_with_precip = 0;

    /* Get the adaptation data */

    if(RPGC_ade_get_values(HYDRO_DEA_FILE, "bias_flag", &apply_bias) != 0)
    {
        RPGC_log_msg(GL_INFO, ">> RPGC_ade_get_values() error");
        RPGC_log_msg(GL_INFO, ">>  No bias applied");
        apply_bias = 0.0;
    }

    /* For dual-pol, since there is no reliable bias yet, set the
     * apply_bias flag to FALSE. This line may be taken out when there is a
     * reliable bias in dual-pol data.
     */

    apply_bias = 0.0; /* apply_bias = FALSE */

    /* Based on our group discussion today, the apply_bias should come from
     * the header. We will add that mechanism once we get a new CCR.
     * This comment is added on 1/28/2010.
     */

    /* Some initializations */

    metadata_p->missing_period_flag = FALSE;

    /* Read input data and add it to final grid. */

    for (i = 0; i < num_record; i++)
    {
        /* Read a record from the query result */

        ret = RPGC_DB_get_record(query_result, i, &record);
        if(ret < 0)
        {
            RPGC_log_msg(GL_INFO, ">> compute_dua_accum(): read data error");

            if(query_result != NULL)
               free(query_result);

            return -1;
        }
        else
        {
            accum_data = (S2S_Accum_Buf_t *)record;
        }

        if(accum_data->supl.ST_active_flg == FALSE)
        {
            if(record != NULL)
               free(record);

            continue;
        }

        /* Interpolation for the first and the last record
         *
         * What to do if interpolate_grid() returns FUNCTION_FAILED or
         * NULL_POINTER? */

        if (i == 0) /* if it is 1st record */
        {
            if (accum_data->supl.missing_period_flg == FALSE)
                interpolate_grid(accum_data, metadata_p->start_time,
                                                   INTERP_FORWARD);
            else
            {
               if(record != NULL)
                  free(record);

               continue;
            }
        }
        else if (i == num_record-1) /* if it is the last record */
        {
            if(accum_data->supl.missing_period_flg == FALSE)
            {
               interpolate_grid(accum_data, metadata_p->end_time,
                                INTERP_BACKWARD);
            }
            else
            {
               if(record != NULL)
                  free(record);

               continue;
            }
        }

        /* if not 1st and last record, simply add in record */
        /* add the data to dua_accum_grid[][] */

        if (apply_bias != 0.0)
        {
            add_biased_short_to_int(dua_accum_grid, accum_data->accum_grid,
                                accum_data->qpe_adapt.bias_info.bias, INT_MAX);
        }
        else
        {
            add_unbiased_short_to_int(dua_accum_grid, accum_data->accum_grid,
                                      INT_MAX);
        }

        /* deal with the bias */

        bias_sum += accum_data->qpe_adapt.bias_info.bias;
        num_of_record_with_precip ++;

        /* deal with the missing period flag by OR them */

        metadata_p->missing_period_flag |=
                                   (short)accum_data->supl.missing_period_flg;
        if(record != NULL)
           free(record);

    }  /* end of for (i = 0; i < num_record; i++)*/

    /* set values of bias and null_product flag*/

    if (num_of_record_with_precip == 0) /* no record has precipitation */
    {
        metadata_p->null_product_flag = NULL_REASON_2;
        metadata_p->bias = 1.0;
    }
    else
    {
        metadata_p->null_product_flag = FALSE;
        if (apply_bias != 0.0)
            metadata_p->bias = bias_sum / num_of_record_with_precip;
        else
            metadata_p->bias = 1.0;
    }

    if(query_result != NULL)
       free(query_result);

    return 0;
}

/******************************************************************************
   Function name: write_to_output_product ()

   Description:
   ============
      It builds the output product (173)

   Inputs:
      dua_accum_grid, metadata of type dua_accum_buf_metadata_t, request_list,
      pointer of type prod_dep_para_t, vol_num

   Outputs:	

   Return:
      0 - OK, -1 - error	

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
   7 Oct 2010    0001       James Ward         At suggestion of Steve Smith,
                                               removed copy of packet16_data
                                               to output buffer. CCR NA10-00355
******************************************************************************/	

int write_to_output_product(void * dua_accum_grid,
                            dua_accum_buf_metadata_t metadata,
                            User_array_t * request_list,
                            prod_dep_para_t * prod_dep,
                            int vol_num,
                            S2S_Accum_Buf_t* inbuf)
{
    int   iostat   = 0;
    int   i4word   = 0;
    int   ret      = FUNCTION_SUCCEEDED; /* return value */
    void* outbuf_p = NULL; /* pointer to the output buffer */

    /* hold scaled value of dua_accum_grid to conform to the final product
     * standard */

    unsigned char digital_grid[MAX_AZM][MAX_BINS];

    Packet16_hdr_t  packet16_hdr;   /* packet16 header        */
    Packet16_data_t packet16_data;  /* packet16 data          */
    Graphic_product *hdr;           /* graphic product header */
    Symbology_block* sym;           /* symbology block        */

    float product_scale  = 1.0;
    float product_offset = 0.0;

    int min_value;
    int max_value; /* we use func convert_int_to_256_char() to get the
                    * max_value. Be alert that the data of scan-to-scan
                    * accumulation is in 1000ths of inches, after using
                    * this function, the max_value is in 1000ths of inches.
                    */

    unsigned int block_len = 0; /* length of the entire symbology block */
    int rad; /* looping counter */
    int offset; /* control the pointer offset when writing packet data
                 * radial by radial */


    unsigned char * temp_p; /* temporary pointer to facilitate writing of
                             * the code segment that writes missing period
                             * flag & null product flag into the final product */

    /* Get the output buffer
     *
     * 20101007 Ward - Downsized output buffer request to SIZE_P173.
     *                 It mimics the DSA size. CCR NA10-00355. */

    outbuf_p = (void *) RPGC_get_outbuf_by_name_for_req((char*) OUTPUT_PRODUCT_NAME,
                        SIZE_P173, request_list, &iostat);

    if (outbuf_p == NULL || iostat != RPGC_NORMAL )
    {
        RPGC_log_msg (GL_INFO, ">> writing to output buffer failed");
        RPGC_abort_request(request_list, iostat);
        return -1;
    }

    sym = (Symbology_block *) ((char *) outbuf_p + sizeof(Graphic_product));

    if(metadata.null_product_flag) /* make a null product */
    {
       make_null_symbology_block((char *) outbuf_p,
                                  metadata.null_product_flag,
                                  OUTPUT_PRODUCT_NAME,
                                  inbuf->qpe_adapt.accum_adapt.restart_time,
                                  inbuf->supl.last_time_prcp,
                                  inbuf->qpe_adapt.prep_adapt.rain_time_thresh);

       prod_dep->param_4 = 0; /* when null_product_flag == 1, do not write to
                               * symbology, just set the max_value component in
                               * prod_dep structure to 0 for later on writing of
                               * product description block.
                               */
    }
    else /* make a non-null product */
    {
        /* scale values in dua_accu_grid to range 1 to 255 and store it in array
         * digital_grid */

        /* Be alert that the data of scan-to-scan accumulation is in 1000th
         * of inches, after using this function, the max_value is ALSO in 1000th
         * of inches */

        ret = convert_int_to_256_char(dua_accum_grid,
                                      &min_value,
                                      &max_value,
                                      &product_scale,
                                      &product_offset,
                                      DUA_PRODUCT_ID,
                                      digital_grid);

         if(ret == FUNCTION_SUCCEEDED)
         {
            prod_dep->param_4 = (short) RPGC_NINT(SCALE_MAX_ACCUM *
                                        check_dua_max_accum(max_value / 1000.0));

            /* Fill in symbology block header. Symbology block header is 10 bytes.
             * Symbology block can wrap several layers; in our case, only one layer
             * is wraped. Each layer wraps a data packet, and each layer begins with
             * a 6-byte layer header, followed by the data packet.
             *
             * For simplicity, we put the layer header inside symbology block header
             * since we only have 1 layer.
             */

            sym->divider  = (short) -1;
            sym->block_id = (short)  1;
            block_len     = (int) (30 + ((MAX_BINS + 6) * MAX_AZM) );

            RPGC_set_product_int( (void *) &(sym->block_len), block_len );

            sym->n_layers      = (short)  1;
            sym->layer_divider = (short) -1;

            /* layer length: Block length - (SYMB_SIZE + LYR_HDR_SIZE) */

            RPGC_set_product_int( (void *) &(sym->data_len), (int) (block_len-16));

            /* Fill in packet16 data packet header.
             * Each type of data packet has different format.
             * We use packet16 data packet.
             * This type of data packet begins with a 14-byte packet header followed
             * by packet data.  */

            packet16_hdr.pcode           = PACKET_16;  /* packet pcode 16 */
            packet16_hdr.first_range_bin = 0; /* distance to the first range bin  */
            packet16_hdr.num_bins        = MAX_BINS; /* number of bins in each radial */
            packet16_hdr.icenter         = ICENTER;   /* i center of display */
            packet16_hdr.jcenter         = JCENTER;   /* j center of display */
            packet16_hdr.scale_factor    = RANGE_SCALE_FACT; /* scale factor */
            packet16_hdr.num_radials     = MAX_AZM;   /* number of radials included */

            /* Copy the packet header into the output buffer */

            memcpy ((void *)((unsigned char *) outbuf_p + SYMB_OFFSET + 16),
                                               &packet16_hdr, 14);
            /* Fill in packet16 packet data.
             * Packet data is arranged in a radial-by-radial fashion. Each radial
             * data begines with 6-byte specification, followed by raw data.
             */

           /* First Set offset to beginning of packe data 150 bytes = 120 + 16 + 14
            * then wtite each radial, and update the offset value in the loop
            */

           offset = 150;
           for(rad = 0; rad < MAX_AZM; rad++ )
           {
              /* Fill in the 6-byte specification for each radial
               * start_angle and delta_angle are scaled by 10 */

              packet16_data.num_bytes   = (short) MAX_BINS;
              packet16_data.start_angle = (short) (rad * SCALE_FACTOR);
              packet16_data.delta_angle = (short) DELTA_ANGLE;

              /* 20201007 Ward - Removed copy of data. CCR NA10-00355
               *
               * for(bin = 0; bin < MAX_BINS; bin++)
               *
               *  packet16_data.data[bin] = (unsigned char) digital_grid[rad][bin];
               */

              /* Write each radial into packet data location of output buffer */

              RPGP_set_packet_16_radial((unsigned char *) outbuf_p + offset,
                                        (short) packet16_data.start_angle,
                                        (short) packet16_data.delta_angle,
                                        /* packet16_data.data, */
                                        &(digital_grid[rad][0]),
                                        (int) MAX_BINS );

              /* Increment the pointer offset (926 bytes per radial struct) */

              offset += (MAX_BINS + 6);

            } /* end rad loop */

         } /* end could do the conversion */
         else /* couldn't do the convert, make a null product */
         {
            make_null_symbology_block((char *) outbuf_p,
                                        metadata.null_product_flag,
                                        OUTPUT_PRODUCT_NAME,
                                        inbuf->qpe_adapt.accum_adapt.restart_time,
                                        inbuf->supl.last_time_prcp,
                                        inbuf->qpe_adapt.prep_adapt.rain_time_thresh);

             prod_dep->param_4 = 0; /* when null_product_flag == 1, do not write to
                                     * symbology, just set the max_value component in
                                     * prod_dep structure to 0 for later on writing of
                                     * product description block.
                                     */
         } /* end couldn't do the conversion */

    } /* end make a non-null product */

    /************* start filling in PDB of Graphic Product Header *************/

    hdr = (Graphic_product *) outbuf_p; /* cast the output buffer pointer to
                                         * a graphic product header pointer */

    /* Initialize graphic product header to NULL */

    memset((void *) hdr, 0, sizeof(Graphic_product));

    /* Initialize some fields in the product description block */

    RPGC_prod_desc_block( (void *) outbuf_p, DUA_PRODUCT_ID, vol_num);

    /* This is a digital, 8-bit, 256-level product */

    RPGC_set_product_float((void*) &(hdr->level_1), product_scale);
    RPGC_set_product_float((void*) &(hdr->level_3), product_offset);

    /* According to Brian Klein, level_5 = hw 35 is reserved
     * by the FAA to store a logarithmic scale */

    hdr->level_6 = UCHAR_MAX; /* hw 36, 255 = max data level */
    hdr->level_7 =         1; /* hw 37 */
    hdr->level_8 =         0; /* hw 38 */

    /* Set halfword 29 - elevation number to 0 for volume based product */

    hdr->elev_ind = 0;

    /* Get values of some unset product dependent parameters */

    /*  We'll need this later when bias gets implemented for dual pol
     *
     *  prod_dep->param_7 =  (short) RPGC_NINT(SCALE_MEAN_FIELD_BIAS *
     *                               check_mean_field_bias(metadata.bias));
     */

    /* WARNING - This needs to be changed once bias is actually being applied
     *   to the products
     */

    prod_dep->param_7 =  (short) 100;

    /* The low byte of param_3 is the null product flag, the high byte
     * is the missing period flag. */

    /* temp_p is a temporary pointer to facilitate code writing */

    temp_p = (unsigned char *) (&(prod_dep->param_3));

    if(metadata.null_product_flag)
        *temp_p = (unsigned char) metadata.null_product_flag;
    else
        *temp_p = (unsigned char) 0;

    temp_p++; /* move up 1 byte */

    *temp_p = (unsigned char) metadata.missing_period_flag;

    /* Fill in product dependent parameters in product description block */

    RPGC_set_dep_params((void *) outbuf_p, (short *) prod_dep);

    /* Set three WORDs that holds offsets (in half words) of three blocks, ie,
     * Symbology block, GAB, and TAB, respectively, from begining of the product.
     *
     * Note 60 = sizeof(Graphic_product)/sizeof(short) */

    RPGC_set_prod_block_offsets((void *) outbuf_p, 60, 0, 0);

    /************** end filling in PDB of Graphic Product Header **********/

    /* begin filling in product message header and release the output buffer */

    /* set the length that will be passed as an input to function
     * RPGC_prod_hdr() so as to build the message header block. As an input,
     * it is the total length of all optional parts of the product
     * (the Symbology Block, the GAB, and the TAB).
     * When RPGC_prod_hdr returns, length will be increased by 120 bytes to
     * reflect the total product size.  */

    RPGC_get_product_int(&(sym->block_len), &i4word);

    RPGC_prod_hdr((void*) outbuf_p, DUA_PRODUCT_ID, &i4word);

    RPGC_rel_outbuf((void *) outbuf_p, FORWARD);

    return 0;

    /* end filling in product message header and release the output buffer */

} /* end write_to_output_product() */

/******************************************************************************
   Function name: reset_prod_dep ()

   Description:
   ============
      It reset each member of a product-dependent-parameter structure to 0

   Inputs:
      pointer to a product-dependent-parameter structure

   Outputs:
      product-dependent-parameter structure that has been reset	

   Return:
      0 - OK, -1 - error	

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/	

void reset_prod_dep(prod_dep_para_t * prod_dep)
{
    prod_dep->param_1  = 0;
    prod_dep->param_2  = 0;
    prod_dep->param_3  = 0;
    prod_dep->param_4  = 0;
    prod_dep->param_5  = 0;
    prod_dep->param_6  = 0;
    prod_dep->param_7  = 0;
    prod_dep->param_8  = 0;
    prod_dep->param_9  = 0;
    prod_dep->param_10 = 0;

    return;
}
