/*
 * $revision$
 * $state$
 * $Logs$
 */


/************************************************************************
 *      Module:      superob_timeok.c                                   *
 *                                                                      *
 *      Description: this module is ude to determine if the time        *
 *                   is within the time window                          *
 *                                                                      *
 *      Input:       valid_julian_day: center time of time window       *
 *                                     in julian day                    *
 *                   valid_julian_time: center time of time window      *
 *                                      since midnight in second        *
 *                   julday:	       data collection time             *
 *                                     in julian day                    *
 *                   jultime:          data collection time             *
 *                                     since midnight  in second        *
 *                   delt:             radius of time window in second  *
 *      Output:      dtime:            time deviation from base time    * 
 *      returns:     TRUE(1): if it is within the time window           *
 *                   FALSE(0):if it is not within the time window       *
 *      Globals:     none                                               *
 ************************************************************************/

/* system include file							*/
#include <stdlib.h>

#define DAY2SECOND		86400  /* seconds in one day		*/

int 
superob_timeok(int valid_julian_day,int valid_julian_time,
           int julday,int jultime, int delt, int offset, float *dtime) 
{ 
 /* declare local variable                                             */
 int time_ok =0;
    
    /* if the valid_time is at the same day as data time               */ 
    if(julday==valid_julian_day)
     {
      if(abs(jultime-valid_julian_time)<=(delt-offset)) /* in the time window   */
       {
       time_ok=1;
      *dtime=jultime-valid_julian_time;       /* set the time deviation*/
       }
     }
    else if(julday > valid_julian_day)   /* data day ahead of valid day*/
     {
     if(abs(jultime+86400-valid_julian_time)<=(delt-offset))/* in the time window*/
       {
       time_ok=1;
      *dtime=jultime+86400-valid_julian_time; /* set the time deviation */
       }
     else
      time_ok=0;
     }
    else
     {
      if(abs(valid_julian_time+86400-jultime) <= (delt-offset)) /* valid day ahead of data day*/
       {
       time_ok=1;
      *dtime=jultime-(valid_julian_time+86400);    /* set the time deviation*/
       }
     else
      time_ok=0;
     }

  return time_ok;
}
