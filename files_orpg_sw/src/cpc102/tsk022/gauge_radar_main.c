/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:06 $
 * $Id: gauge_radar_main.c,v 1.3 2011/04/13 22:53:06 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include "gauge_radar_common.h"
#include "gauge_radar_consts.h"
#include "gauge_radar_types.h"
#include "gauge_radar_proto.h"
#include "siteadp.h"      /* Siteadp_adpt_t */

#include <unistd.h>       /* getopt()            */
#include <stdlib.h>       /* atoi()              */
#include <prod_gen_msg.h> /* PGM_REALTIME_STREAM */

/*****************************************************************************
   Filename: gauge_radar_main.c

   Description:
   ============
      It does all the RPG registrations and initializations, then enters into
      a loop control of request awaiting. If there are user requests, it will
      retrieve the requests and generate products accordingly. 		

      When checking in changes to the ROC, use the tools CCR, NA10-00028.

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   1 Dec 2009    0000       Zhan Zhang         Initial implementation
******************************************************************************/

int data_stream = PGM_REALTIME_STREAM; /* else is PGM_REPLAY_STREAM */

/* Make our large buffers global, so they're on the heap */

int pps_hourly[MAX_AZM][MAX_2KM_RESOLUTION];
int pps_storm[MAX_AZM][MAX_2KM_RESOLUTION];

/* Since we don't know a DP storm is over until the ST_active flag
 * is FALSE, and lt_accum_buf->Storm_Totaxl is zeroed out, we have
 * to keep track of the Storm_Total locally so we can do a comparison
 * at the storm end. */

int Storm_Total[MAX_AZM][MAX_BINS];

/* old_* buffers keep track of old values, for quality control */

float old_gauge_hourly[MAX_GAUGES];
float old_dp_hourly[MAX_GAUGES];
float old_pps_hourly[MAX_GAUGES];

float old_gauge_storm[MAX_GAUGES];
float old_dp_storm[MAX_GAUGES];
float old_pps_storm[MAX_GAUGES];

short zero_scan       = TRUE; /* TRUE -> zero legacy scan-to_scan    */
short zero_hourly     = TRUE; /* TRUE -> zero legacy hourly accum    */
short no_hourly       = TRUE; /* TRUE -> no   legacy hourly accum    */
short reset_pps_storm = TRUE; /* TRUE -> no   legacy storm is active */

/* See the Read_options() file header for a discussion of the four
 * command line arguments. */

char   storm_print_chars[20];
time_t storm_print_time   = 0L; /* storm_print_chars as a Unix time */
int    storm_print_volume = 0;
int    stats_everywhere   = FALSE;
int    use_only_nonzero   = FALSE;

Siteadp_adpt_t site_adapt; /* site adaptation data */

Gauge gauges[MAX_GAUGES];
short num_gauges = 0;

int main(int argc, char * argv[])
{
    /* Read command line arguments
     * Read_options(argc, argv);
     */

    /* Register a volume based accum prod as an input */

    RPGC_reg_inputs(argc, argv);

    /* Register output */

    RPGC_reg_outputs(argc, argv);

    /* Register for Scan Summary */

    RPGC_reg_scan_summary();

    /* Get site adaptation data */

    RPGC_reg_site_info(&site_adapt);

    /* Task timing */

    RPGC_task_init(VOLUME_BASED, argc, argv);

    /* Record if replay or not */

    data_stream = RPGC_get_input_stream(argc, argv);

    RPGC_log_msg(GL_INFO, "BEGIN GAUGE_RADAR, CPC102/TSK022");

    /* Get the adaptable parameters */

    get_adapt();

    read_gauges();

    /* Check gauges for debug ...
     * read_gauges() uses RPGCS_latlon_to_xy() to calculate distances
     * to the radar. Use: change_radar -r KOUN
     * to set the radar so distances are calculated correctly.
     *
     * print_gauges();
     * exit(0);
     */
    print_gauges();

    /* Algorithm Control Loop  */

    while(1)
    {
       RPGC_wait_act(WAIT_DRIVING_INPUT);
       task_handler();
    }

    return 0;

} /* end main() ================================== */

/******************************************************************************
   Function name: Read_options()

   Description:
   ============
     Reads the command line arguments, passed through the
     ~/cfg/extensions/task_attr_table.gauge_radar. The command line arguments
     control product timing and output:

     1. -t storm_print_time

        If -t is set, the first volume after the storm_print_time
        will also produce a storm product. The default is to generate
        a storm product only after the end of a storm.

         Format: -t YYYY_MM_DD_HH_MM_SS
        Example: -t 2010_03_21_03_24_47

     2. -v storm_print_volume

        If -v is set, a storm product will be generated every
        storm_print_volume. The default is to generate a storm product
        only after the end of a storm. In the example below, a storm
        product will be generated every 10th volume.

         Format: -v X
        Example: -v 10

     3. -s

        If -s is set, the statistics set will be added to
        the product TAB, the hourly .txt file and the storm .txt file.
        The default is to print statistics only to the product TAB.

         Format: -s
        Example: -s

     4. -n

        If -n is set, only gauge-DP and gauge-PPS pairs where at
        least one of the gauge, DP, or PPS is non-zero will be added
        to the product, the hourly .txt file and the storm .txt file.
        The default is to add all pairs, even when the gauge, DP, and PPS
        are all zero. Independent of the -n flag, only gauge-DP and
        gauge-PPS pairs will be added the .csv files.

         Format: -n
        Example: -n

     The arguments are set in ~/cfg/extensions/task_attr_table.gauge_radar

     Default setting:

        args
                   0 ""

     All args on:

        args
                   0 "-t 2010_03_20_00_00_00 -v 10 -s -n"

   Inputs:

     int argc    - the argument count
     char **argv - the arguments

   Outputs:

     Four global variables set:

        time_t storm_print_time
        int    storm_print_volume
        int    stats_everywhere
        int    use_only_nonzero

   Return:

      None.

   Called by: <uncalled>, so commented out

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   31 Mar 2010    0000      James Ward         Initial implementation
******************************************************************************/

/* #define ARG_T_SIZE 20
 *
 * void Read_options (int argc, char **argv)
 * {
 *     extern char* optarg;    -* used by getopt *-
 *     extern int   optind;
 *     int c;                  -* used by getopt *-
 *
 *     char year[5], mon[3], day[3];
 *     char hour[3], min[3], sec[3];
 *
 *     while ((c = getopt (argc, argv, "nst:v:")) != EOF)
 *     {
 *         switch (c)
 *         {
 *             case 'n':
 *                 use_only_nonzero = TRUE;
 *                 break;
 *             case 's':
 *                 stats_everywhere = TRUE;
 *                 break;
 *             case 't':
 *                 memset(year, 0, 5);
 *                 memset(mon,  0, 3);
 *                 memset(day,  0, 3);
 *                 memset(hour, 0, 3);
 *                 memset(min,  0, 3);
 *                 memset(sec,  0, 3);
 *
 *                 strncpy(year, &optarg[ 0], 4);
 *                 strncpy(mon,  &optarg[ 5], 2);
 *                 strncpy(day,  &optarg[ 8], 2);
 *                 strncpy(hour, &optarg[11], 2);
 *                 strncpy(min,  &optarg[14], 2);
 *                 strncpy(sec,  &optarg[17], 2);
 *
 *                 sprintf(storm_print_chars,
 *                         "%2s/%2s/%4s %2s:%2s:%2s",
 *                         mon, day, year, hour, min, sec);
 *
 *                 RPGCS_ymdhms_to_unix_time(&storm_print_time,
 *                                           atoi(year),
 *                                           atoi(mon),
 *                                           atoi(day),
 *                                           atoi(hour),
 *                                           atoi(min),
 *                                           atoi(sec));
 *                 break;
 *             case 'v':
 *                 storm_print_volume = atoi(optarg);
 *                 break;
 *         }
 *     }
 *
 *     return;
 * }
 */

/******************************************************************************
   Function name: get_adapt()

   Description:
   ============
     Gets the adaptable parameters in ~/cfg/dea/gauge_radar.alg

   Inputs:

     None.

   Outputs:

     Four global variables set:

        time_t storm_print_time
        int    storm_print_volume
        int    stats_everywhere
        int    use_only_nonzero

   Return:

      None.

   Called by: main()

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   22 Nov 2010    0000      James Ward         Initial implementation
******************************************************************************/

void get_adapt(void)
{
   char year[5], mon[3], day[3];
   char hour[3], min[3], sec[3];

   double value = 0.0;
   char*  get_string_value = NULL;

   if(RPGC_ade_get_values(GAUGE_RADAR_DEA_FILE,
                          "use_only_nonzero", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
                   ">> get_adapt() failed - use_only_nonzero");
   }
   else
   {
      use_only_nonzero = (int) value;
   }

   if(RPGC_ade_get_values(GAUGE_RADAR_DEA_FILE,
                          "stats_everywhere", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
                   ">> get_adapt() failed - stats_everywhere");
   }
   else
   {
      stats_everywhere = (int) value;
   }

   if(RPGC_ade_get_values(GAUGE_RADAR_DEA_FILE,
                          "storm_print_volume", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
                   ">> get_adapt() failed - storm_print_volume");
   }
   else
   {
      storm_print_volume = (int) value;
   }

   if(RPGC_ade_get_string_values(GAUGE_RADAR_DEA_FILE,
                                 "storm_print_time", &get_string_value) != 0)
   {
      RPGC_log_msg(GL_INFO,
                   ">> get_adapt() failed - storm_print_time");
   }
   else
   {
      memset(year, 0, 5);
      memset(mon,  0, 3);
      memset(day,  0, 3);
      memset(hour, 0, 3);
      memset(min,  0, 3);
      memset(sec,  0, 3);

      strncpy(year, &get_string_value[ 0], 4);
      strncpy(mon,  &get_string_value[ 5], 2);
      strncpy(day,  &get_string_value[ 8], 2);
      strncpy(hour, &get_string_value[11], 2);
      strncpy(min,  &get_string_value[14], 2);
      strncpy(sec,  &get_string_value[17], 2);

      sprintf(storm_print_chars,
              "%2s/%2s/%4s %2s:%2s:%2s",
              mon, day, year, hour, min, sec);

      RPGCS_ymdhms_to_unix_time(&storm_print_time,
                                atoi(year),
                                atoi(mon),
                                atoi(day),
                                atoi(hour),
                                atoi(min),
                                atoi(sec));
   }

   /* Print values for debugging */

   if(storm_print_time > 0)
    {
      RPGC_log_msg(GL_INFO, "  storm_print_time: %s (%ld)",
                   storm_print_chars, storm_print_time);
   }
   else
   {
      RPGC_log_msg(GL_INFO, "  storm_print_time: NOT SET");
   }

   if(storm_print_volume > 0)
   {
      RPGC_log_msg(GL_INFO, "storm_print_volume: %d",
                   storm_print_volume);
   }
   else
   {
      RPGC_log_msg(GL_INFO, "storm_print_volume: NOT SET");
   }

   if(stats_everywhere == FALSE)
      RPGC_log_msg(GL_INFO, "  stats_everywhere: FALSE");
   else
      RPGC_log_msg(GL_INFO, "  stats_everywhere: TRUE");

   if(use_only_nonzero == FALSE)
      RPGC_log_msg(GL_INFO, "  use_only_nonzero: FALSE");
   else
      RPGC_log_msg(GL_INFO, "  use_only_nonzero: TRUE");

} /* end get_adapt() ================================== */
