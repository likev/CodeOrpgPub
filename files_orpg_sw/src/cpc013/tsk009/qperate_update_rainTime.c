/*
 * RCS info
 * $Author:
 * $Locker:
 * $Date:
 * $Id:
 * $Revision:
 * $State:
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/*****************************************************************************
  Filename: qperate_update_rainTime.c

  Description:
  ============
  update_supl_rainTime() returns the accumulation beginning time and
  latest rain time based on the value of prcp_begin_flg and prcp_detected_flg
  respectively. Start_ST_time records the first volume date/time that
  starts to rain for the accumulation period. The start_ST_time for the
  remaining volumes will still keep the first volume date/time until it is
  not raining for an hour (i.e. storm total accumulation is not active).
  Lasted time_prcp will be updated based on the rain detected flag.

  Inputs:  time_t average_elev_time   - average elevation time
                                        for the current volume

  Outputs: Rate_Buf_t* rate_out       - rate output buffer, in particular:

           rate_supl->start_date_time - If the prcp_begin_flg is set to be
                                        true, the rate_supl->start_date_prcp
                                        value will be assigned by the average
                                        date value for the current volume,
                                        Otherwise it will still use the average
                                        time for the volume that starts to
                                        rain.

           rate_supl->start_date_time - same as the above

           rate_supl->last_date_prcp  - If the prcp_detected_flg is set to be
                                        true, rate_supl->last_date_prcp value
                                        will be updated to the average
                                        date/time for the current volume,
                                        otherwise it will use its old value
                                        if ST_active_flg is not activated.

           rate_supl->last_time_prcp  - Same as the above

  Change History
  ==============
  DATE          VERSION    PROGRAMMER         NOTES
  ----------    -------    ---------------    ------------------
  06/20/07      0001       Jihong Liu         Accumulation beginning time and
                                              latest rain time are based on
                                              average date/time for the current
                                              volume instead of the current
                                              elevation when the rain is
                                              detected.
  09/20/11      0002       James Ward         Commented out as folded into
                                              precip_accum_init().
******************************************************************************/

/*
* void update_supl_rainTime(time_t average_elev_time, Rate_Buf_t* rate_out)
* {
*   -* AEL 3.1.1 *-
*
*   static time_t lasTimeRain   = 0L; -* Init latest rain time             *-
*   static time_t start_ST_time = 0L; -* Init start time for ST rain accum *-
*
*   if(rate_out->rate_supl.prcp_begin_flg == TRUE)
*   {
*      -* Update precip start time, AEL 3.1.3 *-
*
*      start_ST_time = average_elev_time;
*   }
*
*   rate_out->rate_supl.start_time_prcp = start_ST_time;
*
*   -* Update the latest rain time *-
*
*   if(rate_out->rate_supl.prcp_detected_flg == TRUE) -* AEL 3.1.1 *-
*      lasTimeRain = average_elev_time;
*
*   -* Update time for last precip. *-
*
*   rate_out->rate_supl.last_time_prcp = lasTimeRain;
*
*   -* Update rate grid time *-
*
*   rate_out->rate_supl.time = average_elev_time;
*
*} -* end update_supl_rainTime() =============================== */
