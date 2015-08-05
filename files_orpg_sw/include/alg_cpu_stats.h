/* 
 * RCS info 
 * $Author: nolitam $ 
 * $Locker:  $ 
 * $Date: 2002/12/11 21:31:43 $ 
 * $Id: alg_cpu_stats.h,v 1.2 2002/12/11 21:31:43 nolitam Exp $ 
 * $Revision: 1.2 $ 
 * $State: Exp $ 
 */ 

#ifndef ALG_CPU_STATS_H
#define ALG_CPU_STATS_H

typedef struct alg_cpu_stats{

   unsigned int vol_scan_num; 		/* Volume scan sequence number. */

   int          elev_cnt;		/* # elevations process executed. */

   float        total_cpu;		/* Total CPU usage (secs/Volume) */

   float        avg_cpu;		/* Average CPU usage (secs/Elevation) */

   float        avg_cpu_percent;	/* Average CPU usage (percent/Elevation) */

} alg_cpu_stats_t;

#endif
