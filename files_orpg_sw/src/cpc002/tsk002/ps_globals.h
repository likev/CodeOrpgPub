/**************************************************************************

   Module:  ps_globals.h

   Description:
      This include file provides definitions and declarations of global
      variables.  The contents of this file may result in compile-time
      allocation of storage.

      All global variables are identified by the "Psg_" prefix.

      Only file-scope globals may be defined in any other Schedule
      Routine Products task source file.

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/07/27 15:32:15 $
 * $Id: ps_globals.h,v 1.19 2005/07/27 15:32:15 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */

#ifndef PS_GLOBALS_H
#define PS_GLOBALS_H

/*
 * System Include Files/Local Include Files
 * Truly global header files ...
 * Those header files required to define global variables ...
 */
#include <errno.h>
#include <stdio.h>

#include <ps_def.h>

#ifdef PS_MAIN
       int Psg_output_prod_status_flag;	/* Flag, if set, indicates product
					   status is to be output at this time. */

       int Psg_verbose_level; 		/* Verbosity level ... The higher the level,
                                           the more verbose. */
		
       int Psg_wx_mode_beginning;	/* Weather mode at ps_routine startup. */

       int Psg_cpu_monitor_alarm_expired;/* Flag, if set, indicates its time to request
                                            task status information from mrpg. */

       int Psg_cpu_info_avail;		/* Flag, if set, indicates task status is 
                                           available from mrpg. */

       int Psg_cpu_monitoring_rate;	/* Rate, in seconds, for which we provide
                                           CPU information. */
 
       int Psg_cpu_overhead;		/* Overhead value, in percent of CPU, to 
                                           add as overhead.  */

extern char *optarg ;
#endif

#ifdef PS_CONVERT_PARAMS
extern int Psg_verbose_level ;
#endif

#ifdef PS_PROD_TASK_TABLES
extern int Psg_verbose_level ;
extern int Psg_product_scheduling_changed ;
#endif

#ifdef PS_HANDLE_PROD
extern   int Psg_output_prod_status_flag ;
extern short Psg_cur_wx_mode ;
extern   int Psg_verbose_level ;
extern   int Psg_wx_mode_beginning ;
#endif

#ifdef PS_PROCESS_EVENTS
       short Psg_cur_wx_mode;		/* Current volume scan weather mode. */

extern   int Psg_check_failed_ones_for_last_vol ;
extern   int Psg_output_prod_status_flag ;
extern   int Psg_verbose_level ;
extern   int Psg_wx_mode_beginning ;
extern   int Psg_task_status_changed ;
#endif

#ifdef PS_PROCESS_MSG
extern   int Psg_output_prod_status_flag ;
extern short Psg_cur_wx_mode ;
extern   int Psg_verbose_level ;
#endif

#ifdef PS_RDA_RPG_STATUS
#endif

#ifdef PS_TASK_STATUS_LIST
         int Psg_task_status_changed ;
         int Psg_product_scheduling_changed ;
extern   int Psg_verbose_level ;
#endif

#ifdef PS_VOL_PROD_LIST
         int Psg_check_failed_ones_for_last_vol ;

extern short Psg_cur_wx_mode ;
extern   int Psg_verbose_level ;
extern   int Psg_wx_mode_beginning ;
extern   int Psg_output_prod_status_flag ;
#endif

#ifdef PS_MONITOR_CPU
extern   int Psg_cpu_monitor_alarm_expired ;
extern   int Psg_cpu_info_avail ;
extern   int Psg_cpu_monitoring_rate ;
extern   int Psg_cpu_overhead ;
extern   int Psg_verbose_level ;
#endif


#endif /*DO NOT DELETE*/
