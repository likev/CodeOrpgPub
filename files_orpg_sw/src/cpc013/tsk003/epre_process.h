 /*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:01:46 $
 * $Id: epre_process.h,v 1.7 2014/09/02 20:01:46 dberkowitz Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef _EPRE_PROCESS_H_   
#define _EPRE_PROCESS_H_

#include <alg_adapt.h>
#include <epre_main.h>

#define MAX_RNG 230

/* Function Prototypes ---------------------------------------------- */

void copy_adapt_supl(float *adapt, int *supl);

void copy_precip_status_msg (int *precip_status);

int get_elev_time(int zdate, int ztime);

void compute_avg_datetime(time_t begtime, time_t endtime, int *avgdate, int *avgtime);

void read_refl(void *inbuf, int *elflag, int *elindx, int *tilts,
               double *elangle, int *volnum, int *vcpnum, int *radnum,
               int *beg_time, int *beg_date, int *end_time, int *end_date,
	       double azm_array[], short zrefl[MAX_RADIALS_ELEV][MAX_RNG],
	       int *status, int *bde_status);

void read_clutterap(char *inbuf, int *volnum, int *elindx,
                    int *spotblk, short ap[MAX_RADIALS][MAX_RNG],
                    int *status);

int c_epre(int *startup,
           int *newHysFlag,
           int passedVcpNum,
           int passedTilt,
           int nextLastElev,
           int passedVolNum,
           unsigned long daTimeSecs,
           int elevangle,
           int elevind,
           int radialNumber,
           int *rainDetected,
           short passed_ref[MAX_RAD][MAX_RNG],
           short clut_ap[MAX_RAD][MAX_RNG],
           double passed_azimuth[],
           short hys[MAX_AZM][MAX_RNG],
           short hys_el[MAX_AZM][MAX_RNG]);

#endif /* _EPRE_PROCESS_H_ */ 
