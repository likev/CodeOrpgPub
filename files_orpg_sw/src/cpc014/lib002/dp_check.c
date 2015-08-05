/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:14:23 $
 * $Id: dp_check.c,v 1.5 2014/09/02 20:14:23 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include "dp_lib_func_prototypes.h"

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_max_accum() checks max accum before sending to an output buffer

   Inputs: float max_accum

   Outputs: max_accum adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_max_accum(float max_accum)
{
  char msg[200];

  if(max_accum < MIN_MAX_ACCUM)
  {
     sprintf(msg, "%s: max_accum %f < MIN_MAX_ACCUM %f, %s\n",
             "check_max_accum",
             max_accum,
             MIN_MAX_ACCUM,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_MAX_ACCUM);
  }
  else if(max_accum > MAX_MAX_ACCUM)
  {
     sprintf(msg, "%s: max_accum %f > MAX_MAX_ACCUM %f, %s\n",
             "check_max_accum",
             max_accum,
             MAX_MAX_ACCUM,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MAX_ACCUM);
  }

  /* If we got here, we're OK */

  return(max_accum);

} /* end check_max_accum() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_dua_max_accum() checks DUA max accum before sending to an
   output buffer

   Inputs: float dua_max_accum

   Outputs: dua_max_accum adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_dua_max_accum(float dua_max_accum)
{
  char msg[200];

  if(dua_max_accum < MIN_MAX_ACCUM)
  {
     sprintf(msg, "%s: dua_max_accum %f < MIN_MAX_ACCUM %f, %s\n",
             "check_dua_max_accum",
             dua_max_accum,
             MIN_MAX_ACCUM,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_MAX_ACCUM);
  }
  else if(dua_max_accum > MAX_MAX_ACCUM_DUA)
  {
     sprintf(msg, "%s: dua_max_accum %f > MAX_MAX_ACCUM_DUA %f, %s\n",
             "check_dua_max_accum",
             dua_max_accum,
             MAX_MAX_ACCUM_DUA,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MAX_ACCUM_DUA);
  }

  /* If we got here, we're OK */

  return(dua_max_accum);

} /* end check_dua_max_accum() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_mean_field_bias() checks mean_field_bias before sending to an
   output buffer

   Inputs: float mean_field_bias

   Outputs: mean_field_bias adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_mean_field_bias(float mean_field_bias)
{
  char msg[200];

  if(mean_field_bias < MIN_MEAN_FIELD_BIAS)
  {
     sprintf(msg, "%s: mean_field_bias %f < MIN_MEAN_FIELD_BIAS %f, %s\n",
             "check_mean_field_bias",
             mean_field_bias,
             MIN_MEAN_FIELD_BIAS,
             "replacing it.");

     /* 20090127 Ward If the bias is 0.0 < MIN_MEAN_FIELD_BIAS (0.01),
      * not printing the ongoing GL_INFO message. We are returning 0.01
      * according to the specification.
      *
      * RPGC_log_msg(GL_INFO, msg);
      * if(DP_LIB002_DEBUG)
      *   fprintf(stderr, msg);
      */

     return(MIN_MEAN_FIELD_BIAS);
  }
  else if(mean_field_bias > MAX_MEAN_FIELD_BIAS)
  {
     sprintf(msg, "%s: mean_field_bias %f > MAX_MEAN_FIELD_BIAS %f, %s\n",
             "check_mean_field_bias",
             mean_field_bias,
             MAX_MEAN_FIELD_BIAS,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MEAN_FIELD_BIAS);
  }

  /* If we got here, we're OK */

  return(mean_field_bias);

} /* end check_mean_field_bias() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_sample_size() checks a sample_size before sending to an output buffer

   Inputs: float sample_size

   Outputs: sample_size adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_sample_size(float sample_size)
{
  char msg[200];

  if(sample_size < MIN_SAMPLE_SIZE)
  {
     sprintf(msg, "%s: sample_size %f < MIN_SAMPLE_SIZE %f, %s\n",
             "check_sample_size",
             sample_size,
             MIN_SAMPLE_SIZE,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_SAMPLE_SIZE);
  }
  else if(sample_size > MAX_SAMPLE_SIZE)
  {
     sprintf(msg, "%s: sample_size %f > MAX_SAMPLE_SIZE %f, %s\n",
             "check_sample_size",
             sample_size,
             MAX_SAMPLE_SIZE,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_SAMPLE_SIZE);
  }

  /* If we got here, we're OK */

  return(sample_size);

} /* end check_sample_size() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_date() checks a date before sending to an output buffer

   Inputs: short date

   Outputs: date adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

short check_date(short date)
{
  char msg[200];

  if(date < MIN_DATE)
  {
     sprintf(msg, "%s: date %hd < MIN_DATE %hd, %s\n",
             "check_date",
             date,
             MIN_DATE,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_DATE);
  }
  /* The compiler warns that this test is always FALSE because the
   * range of MAX_DATE is identical to SHORT_MAX. Commenting it out
   * in case the max is set differently in the future.
  else if(date > MAX_DATE)
  {
     sprintf(msg, "%s: date %hd > MAX_DATE %hd, %s\n",
             "check_date",
             date,
             MAX_DATE,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_DATE);
  }
  */

  /* If we got here, we're OK */

  return(date);

} /* end check_date () ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_time() checks a time before sending to an output buffer

   Inputs: short time

   Outputs: time adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

short check_time(short time)
{
  char msg[200];

  if(time < MIN_TIME)
  {
     sprintf(msg, "%s: time %hd < MIN_TIME %hd, %s\n",
             "check_time",
             time,
             MIN_TIME,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_TIME);
  }
  else if(time > MAX_TIME)
  {
     sprintf(msg, "%s: time %hd > MAX_TIME %hd, %s\n",
             "check_time",
             time,
             MAX_TIME,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_TIME);
  }

  /* If we got here, we're OK */

  return(time);

} /* end check_time() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_time_span() checks a time_span before sending to an output buffer

   Inputs: float time_span

   Outputs: time_span adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

short check_time_span(short time_span)
{
  char msg[200];

  if(time_span < MIN_TIME_SPAN)
  {
     sprintf(msg, "%s: time_span %hd < MIN_TIME_SPAN %hd, %s\n",
             "check_time_span",
             time_span,
             MIN_TIME_SPAN,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_TIME_SPAN);
  }
  else if(time_span > MAX_TIME_SPAN)
  {
     sprintf(msg, "%s: time_span %hd > MAX_TIME_SPAN %hd, %s\n",
             "check_time_span",
             time_span,
             MAX_TIME_SPAN,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_TIME_SPAN);
  }

  /* If we got here, we're OK */

  return(time_span);

} /* end check_time_span() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_min_accum_diff() checks a min_accum_diff before sending to an
   output buffer

   Inputs: float min_accum_diff

   Outputs: min_accum_diff adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_min_accum_diff(float min_accum_diff)
{
  char msg[200];

  if(min_accum_diff < MIN_MIN_ACCUM_DIFF)
  {
     sprintf(msg, "%s: min_accum_diff %f < MIN_MIN_ACCUM_DIFF %f, %s\n",
             "check_min_accum_diff",
             min_accum_diff,
             MIN_MIN_ACCUM_DIFF,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_MIN_ACCUM_DIFF);
  }
  else if(min_accum_diff > MAX_MIN_ACCUM_DIFF)
  {
     sprintf(msg, "%s: min_accum_diff %f > MAX_MIN_ACCUM_DIFF %f, %s\n",
             "check_min_accum_diff",
             min_accum_diff,
             MAX_MIN_ACCUM_DIFF,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MIN_ACCUM_DIFF);
  }

  /* If we got here, we're OK */

  return(min_accum_diff);

} /* end check_min_accum_diff() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_max_accum_diff() checks a max_accum_diff before sending to an
   output buffer

   Inputs: float max_accum_diff

   Outputs: max_accum_diff adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_max_accum_diff(float max_accum_diff)
{
  char msg[200];

  if(max_accum_diff < MIN_MAX_ACCUM_DIFF)
  {
     sprintf(msg, "%s: max_accum_diff %f < MIN_MAX_ACCUM_DIFF %f, %s\n",
             "check_max_accum_diff",
             max_accum_diff,
             MIN_MAX_ACCUM_DIFF,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_MAX_ACCUM_DIFF);
  }
  else if(max_accum_diff > MAX_MAX_ACCUM_DIFF)
  {
     sprintf(msg, "%s: max_accum_diff %f > MAX_MAX_ACCUM_DIFF %f, %s\n",
             "check_max_accum_diff",
             max_accum_diff,
             MAX_MAX_ACCUM_DIFF,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MAX_ACCUM_DIFF);
  }

  /* If we got here, we're OK */

  return(max_accum_diff);

} /* end check_max_accum_diff() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_max_inst_precip() checks a max_inst_precip before sending to an
   output buffer

   Inputs: unsigned float max_inst_precip

   Outputs: max_inst_precip adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

unsigned short check_max_inst_precip(unsigned short max_inst_precip)
{
  /* The compiler warns that these two tests are always FALSE because the
   * range of MIN_MAX_INST_PRECIP and MAX_MAX_INST_PRECIP are identical to
   * the range of an unsigned short (USHORT_MIN to USHORT_MAX). Commenting them
   * out in case the ranges are set differently in the future.

  char msg[200];

  if(max_inst_precip < MIN_MAX_INST_PRECIP)
  {
     sprintf(msg, "%s: max_inst_precip %hd < MIN_MAX_INST_PRECIP %hd, %s\n",
             "check_max_inst_precip",
             max_inst_precip,
             MIN_MAX_INST_PRECIP,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_MAX_INST_PRECIP);
  }
  else if(max_inst_precip > MAX_MAX_INST_PRECIP)
  {
     sprintf(msg, "%s: max_inst_precip %hd > MAX_MAX_INST_PRECIP %hd, %s\n",
             "check_max_inst_precip",
             max_inst_precip,
             MAX_MAX_INST_PRECIP,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MAX_INST_PRECIP);
  }
  */

  /* If we got here, we're OK */

  return(max_inst_precip);

} /* end check_max_inst_precip() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_hybr_rate_filled() checks a hybr_rate_filled before sending to an
   output buffer

   Inputs: float hybr_rate_filled

   Outputs: hybr_rate_filled adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_hybr_rate_filled(float hybr_rate_filled)
{
  char msg[200];

  if(hybr_rate_filled < MIN_HYBR_RATE_FILLED)
  {
     sprintf(msg, "%s: hybr_rate_filled %f < MIN_HYBR_RATE_FILLED %f, %s\n",
             "check_hybr_rate_filled",
             hybr_rate_filled,
             MIN_HYBR_RATE_FILLED,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_HYBR_RATE_FILLED);
  }
  else if(hybr_rate_filled > MAX_HYBR_RATE_FILLED)
  {
     sprintf(msg, "%s: hybr_rate_filled %f > MAX_HYBR_RATE_FILLED %f, %s\n",
             "check_hybr_rate_filled",
             hybr_rate_filled,
             MAX_HYBR_RATE_FILLED,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_HYBR_RATE_FILLED);
  }

  /* If we got here, we're OK */

  return(hybr_rate_filled);

} /* end check_hybr_rate_filled() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_highest_elev_used() checks a highest_elev_used before sending to an
   output buffer

   Inputs: float highest_elev_used

   Outputs: highest_elev_used adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

float check_highest_elev_used(float highest_elev_used)
{
  char msg[200];

  if(highest_elev_used < MIN_HIGHEST_ELEV_USED)
  {
     sprintf(msg, "%s: highest_elev_used %f < MIN_HIGHEST_ELEV_USED %f, %s\n",
             "check_highest_elev_used",
             highest_elev_used,
             MIN_HIGHEST_ELEV_USED,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_HIGHEST_ELEV_USED);
  }
  else if(highest_elev_used > MAX_HIGHEST_ELEV_USED)
  {
     sprintf(msg, "%s: highest_elev_used %f > MAX_HIGHEST_ELEV_USED %f, %s\n",
             "check_highest_elev_used",
             highest_elev_used,
             MAX_HIGHEST_ELEV_USED,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_HIGHEST_ELEV_USED);
  }

  /* If we got here, we're OK */

  return(highest_elev_used);

} /* end check_highest_elev_used() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_mode_filter_size() checks a mode_filter_size before sending to an
   output buffer

   Inputs: short mode_filter_size

   Outputs: mode_filter_size adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
   26 Aug 2014    0001       Murnan      	Removed function, min/max 
			 			values checked in DEA
******************************************************************************/

  /* Remove function since adaptable parameter "mode_filter_size"
   * is in the "dp_precip.alg" (i.e., DEA file) which already checks value against
   * valid ranges stored within the same DEA file.

short check_mode_filter_size(short mode_filter_size)
{
  char msg[200];

  if(mode_filter_size < MIN_MODE_FILTER_LEN)
  {
     sprintf(msg, "%s: mode_filter_size %hd < MIN_MODE_FILTER_LEN %hd, %s\n",
             "check_mode_filter_size",
             mode_filter_size,
             MIN_MODE_FILTER_LEN,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MIN_MODE_FILTER_LEN);
  }
  else if(mode_filter_size > MAX_MODE_FILTER_LEN)
  {
     sprintf(msg, "%s: mode_filter_size %hd > MAX_MODE_FILTER_LEN %hd, %s\n",
             "check_mode_filter_size",
             mode_filter_size,
             MAX_MODE_FILTER_LEN,
             "replacing it.");

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);

     return(MAX_MODE_FILTER_LEN);
  }

  return(mode_filter_size);

} */
  /* end check_mode_filter_size() ===================================== */



/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_true_false_flag() checks a TRUE/FALSE flag before sending to an
   output buffer

   Inputs: int   flag - the flag
           char* name - the name of the flag, for message purposes.

   Outputs: flag adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
******************************************************************************/

int check_true_false_flag(int flag, char* name)
{
  char msg[200];

  if((flag != TRUE) && (flag != FALSE))
  {
     sprintf(msg, "%s: %s %d != TRUE %d or FALSE %d, ",
             "check_true_false_flag",
             name,
             flag,
             TRUE,
             FALSE);

     if(flag > 0)
     {
        strcat(msg, "replacing it with TRUE.\n");
        return(TRUE);
     }
     else
     {
        strcat(msg, "replacing it with FALSE.\n");
        return(FALSE);
     }

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);
  }

  /* If we got here, we're OK */

  return(flag);

} /* end check_true_false_flag() ===================================== */

/******************************************************************************
   Filename: dp_check.c

   Description:
   ============
   check_null_product() checks a null product before sending to an
   output buffer

   Inputs: int   null_indicator
           char* name - the name of the null_indicator, for message purposes.

   Outputs: null_indicator adjusted

   Change History
   ==============
   DATE           VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
   11 Apr 2008    0000       Ward               Initial implementation
   31 Mar 2014    0001       Murnan             Added new NULL definitions
                                                for TOH addtion
******************************************************************************/

int check_null_product(int null_indicator, char* name)
{
  char msg[200];

  if((null_indicator < FALSE) || (null_indicator > NULL_REASON_7))
  {
     sprintf(msg, "%s: %s (%d < FALSE %d) or (%d > NULL_REASON_7 %d), ",
             "check_null_product",
             name,
             null_indicator,
             FALSE,
             null_indicator,
             NULL_REASON_7);

     if(null_indicator < FALSE)
     {
        strcat(msg, "replacing it with FALSE.\n");
        return(FALSE);
     }
     else
     {
        strcat(msg, "replacing it with NULL_REASON_7.\n");
        return(NULL_REASON_7);
     }

     RPGC_log_msg(GL_INFO, msg);
     if(DP_LIB002_DEBUG)
       fprintf(stderr, msg);
  }

  /* If we got here, we're OK */

  return(null_indicator);

} /* end check_null_product() ===================================== */
