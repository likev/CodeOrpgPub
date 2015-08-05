/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:06 $
 * $Id: gauge_radar_hourly.c,v 1.3 2011/04/13 22:53:06 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <dp_lib_func_prototypes.h>
#include <gsl/gsl_statistics_double.h>
#include "gauge_radar_common.h"
#include "gauge_radar_consts.h"
#include "gauge_radar_proto.h"
#include "gauge_radar_types.h"

/******************************************************************************
   Function name: generate_product_hourly()

   Description:
   ============
      It determines whether to generate product or not based on whether it is in
      a different hour or not. If it is in the same hour, no product generated;
      otherwise, generate the product.

   Inputs:
      year, month, day, hour of product to be generated

   Outputs:
      None.

   Return:
     TRUE  - generate a product
     FALSE - don't generate a product

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Feb 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/

int generate_product_hourly(int year, int month, int day, int hour)
{
    static int first_time = TRUE;

    static int last_product_hour  = 0;
    static int last_product_day   = 0;
    static int last_product_month = 0;
    static int last_product_year  = 0;

    if(first_time == TRUE)
    {
        last_product_hour  = hour;
        last_product_day   = day;
        last_product_month = month;
        last_product_year  = year;

        first_time = FALSE;

        RPGC_log_msg(GL_INFO, "Hourly: first time, no product");

        return(FALSE);
    }
    else if ((hour  == last_product_hour)  &&
             (day   == last_product_day)   &&
             (month == last_product_month) &&
             (year  == last_product_year)) /* the hour is unchanged */
    {
        RPGC_log_msg(GL_INFO, "Hourly: same hour, no product");

        return(FALSE);
    }
    else /* the hour has changed */
    {
        last_product_hour  = hour;
        last_product_month = month;
        last_product_day   = day;
        last_product_year  = year;

        RPGC_log_msg(GL_INFO, "Hourly: new hour, product generated");

        return(TRUE);
    }

} /* end generate_product_hourly() ================================== */
