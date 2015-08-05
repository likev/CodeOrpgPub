/*
 * RCS info
 * $Author: cm $
 * $Locker:  $
 * $Date: 2010/09/14 12:43:24 $
 * $Id: get_saa_usp_requests.c,v 1.6 2010/09/14 12:43:24 cm Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        get_saa_usp_requests.c

Description:   module to acquire user requests and product parameters for cpc014/tsk014
	       which creates User Selectable Water Equivalent and Depth products for 
	       the Snow Accumulation Algorithm.  The parameters received are the ending
	       hour for the accumulations and the number of hours to include.  Note, a
	       -1 indicates user is requesting the current hour. Other valid numbers are
	       0 to 23.  The user may specify from 1 to 30 hours to include in the
	       accumulation.

CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, October 2003
               
History:       
               Initial implementation 10/08/03 - Zittel
               10/24/04	SW CCR NA04-30810	Changes for Build8:  If the requested hour
               					is greater than the current hour, ajdust the
               					julian day to the previous julian day.
               
*******************************************************************************/
/*  Function to extract user request information for the USW and USD products */

#include "saausers_main.h"

#define MIN_PER_DAY 1440
#define MIN_PER_HR    60
#define MIN_PER_VOL    6
#define MISSING_TIME  -1

extern User_array_t *SAA_requests;

int check_for_duplicates(int, int);

int get_saa_usp_requests(int vol_date, int vol_hour)
{


   int debugit = FALSE;
   int i, n_requests = 0,prod_id;
   int el_index = -1;
   int num_swu_rqsts = 0;
   int num_sdu_rqsts = 0;
   
/*  Get the SAA USD or USW product parameters                           */
    SAA_requests = (User_array_t *)RPGC_get_customizing_data(el_index, &n_requests);
    if(SAA_requests == NULL)
       return (0);
    if(debugit){fprintf(stderr,"n_requests = %d\n",n_requests);}

    for(i=0;i<n_requests;++i){
	prod_id = SAA_requests[i].ua_prod_code;
        if(debugit){
          fprintf(stderr,"i = %d, user_pcode = %d, ",i,prod_id);
          fprintf(stderr,"p0/1/2/3/4/5 = %6d, ",SAA_requests[i].ua_dep_parm_0);
          fprintf(stderr," %6d, ",SAA_requests[i].ua_dep_parm_1);
          fprintf(stderr," %6d, ",SAA_requests[i].ua_dep_parm_2);
          fprintf(stderr," %6d, ",SAA_requests[i].ua_dep_parm_3);
          fprintf(stderr," %6d, ",SAA_requests[i].ua_dep_parm_4);
          fprintf(stderr," %6d, ",SAA_requests[i].ua_dep_parm_5);
          fprintf(stderr,"elidx = %2d, ",SAA_requests[i].ua_elev_index);
          fprintf(stderr,"rqst# = %3d, ",SAA_requests[i].ua_req_number);
          fprintf(stderr,"spare = %d\n",SAA_requests[i].ua_spare);
        }
        if(prod_id == USWACCUM){
           userreq.swu_req_indx[num_swu_rqsts] = i;
           userreq.swu_end_hour[num_swu_rqsts] = SAA_requests[i].ua_dep_parm_0;
           userreq.swu_num_hours[num_swu_rqsts] = SAA_requests[i].ua_dep_parm_1;
           if(userreq.swu_end_hour[num_swu_rqsts] == MISSING_TIME)
                userreq.swu_end_hour[num_swu_rqsts] = vol_hour;

           /*  If the vol_hour and the user requested hour are not the same then we need to adjust
               the vol_date to vol_date - 1 if the vol_hour - user_requested hour is less than 0.   
               Logic added 10/08/2004 for Build8 WDZ					   */
          
	   if( vol_hour - userreq.swu_end_hour[num_swu_rqsts] < 0 ){
	       userreq.swu_end_minutes[num_swu_rqsts] = (vol_date - 1)*MIN_PER_DAY + 
                                                    userreq.swu_end_hour[num_swu_rqsts]*MIN_PER_HR;
               }
           else
	       userreq.swu_end_minutes[num_swu_rqsts] = vol_date*MIN_PER_DAY + 
                                                    userreq.swu_end_hour[num_swu_rqsts]*MIN_PER_HR;

           userreq.swu_beg_minutes[num_swu_rqsts] = userreq.swu_end_minutes[num_swu_rqsts] - 
                                                    userreq.swu_num_hours[num_swu_rqsts]*MIN_PER_HR;
           if(debugit){
              fprintf(stderr,"Get Rqsts; Beg/end mins = %d/%d\n", userreq.swu_beg_minutes[num_swu_rqsts],
                 userreq.swu_end_minutes[num_swu_rqsts]);
              }
           if((check_for_duplicates(num_swu_rqsts,prod_id)==FALSE) && num_swu_rqsts < MAX_USR_RQSTS)
              num_swu_rqsts += 1;
           else
              continue;
        }
        if(prod_id == USDACCUM){
           userreq.sdu_req_indx[num_sdu_rqsts] = i;
           userreq.sdu_end_hour[num_sdu_rqsts] = SAA_requests[i].ua_dep_parm_0;
           userreq.sdu_num_hours[num_sdu_rqsts] = SAA_requests[i].ua_dep_parm_1;
           if(userreq.sdu_end_hour[num_sdu_rqsts] == MISSING_TIME)
                userreq.sdu_end_hour[num_sdu_rqsts] = vol_hour;

           /*  If the vol_hour and the user requested hour are not the same then we need to adjust
               the vol_date to vol_date - 1 if the vol_hour - user_requested hour is less than 0.   
               Logic added 10/08/2004 for Build8 WDZ					   */
          
	   if( vol_hour - userreq.sdu_end_hour[num_sdu_rqsts] < 0 ){
	       userreq.sdu_end_minutes[num_sdu_rqsts] = (vol_date - 1)*MIN_PER_DAY + 
                                                    userreq.sdu_end_hour[num_sdu_rqsts]*MIN_PER_HR;
               }
           else
	       userreq.sdu_end_minutes[num_sdu_rqsts] = vol_date*MIN_PER_DAY + 
                                                    userreq.sdu_end_hour[num_sdu_rqsts]*MIN_PER_HR;
           userreq.sdu_beg_minutes[num_sdu_rqsts] = userreq.sdu_end_minutes[num_sdu_rqsts] -
                                                    userreq.sdu_num_hours[num_sdu_rqsts]*MIN_PER_HR;
           if(debugit){
              fprintf(stderr,"Get Rqsts; Beg/end mins = %d/%d\n", userreq.sdu_beg_minutes[num_sdu_rqsts],
                 userreq.sdu_end_minutes[num_sdu_rqsts]);
              }
           if((check_for_duplicates(num_sdu_rqsts,prod_id)==FALSE) && num_sdu_rqsts < MAX_USR_RQSTS)
              num_sdu_rqsts += 1;
           else
              continue;
        }
    }
    
    userreq.swu_num_rqsts = num_swu_rqsts;
    userreq.sdu_num_rqsts = num_sdu_rqsts;
    return (n_requests);
}

int check_for_duplicates(int nbreqs, int prod_id)
{
/* If the new request matches any request already stored, don't save it.  */

   int i;
   if(nbreqs==0)
     return (FALSE);
   switch (prod_id)
   {
      case USWACCUM:
      {
      for(i=0;i<nbreqs;++i)
         if((userreq.swu_end_hour[nbreqs] == userreq.swu_end_hour[i]) &&
             userreq.swu_num_hours[nbreqs] == userreq.swu_num_hours[i])
             return (TRUE);
          
      return (FALSE);
      }
      case USDACCUM:
      {
      for(i=0;i<nbreqs;++i)
         if((userreq.sdu_end_hour[nbreqs] == userreq.sdu_end_hour[i]) &&
             userreq.sdu_num_hours[nbreqs] == userreq.sdu_num_hours[i])
             return (TRUE);
          
      return (FALSE);
      }
      default:
         {
         fprintf(stderr,"Error: Invalid product id (%d) in get_saa_usp_requests\n",prod_id);
         return (FALSE);
         }
   }
}
          
