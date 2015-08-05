/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/12 23:41:41 $
 * $Id: saaaccum_usp.c,v 1.6 2009/02/12 23:41:41 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaaccum_usp.c

Description:   User requested USW and USD accumulation driver for cpc014/tsk014
               SAA (Snow Accumulation Algorithm) task. It generates accumulations 
               for the creation of products 150 & 151, the User Selectable
               Snow Water Equivalent and the User Selectable Snow Depth for up to 10 
               requests for the products. After determining the amount of accumulation 
               for the type of product requested, an ICD compliant product is generated
               via a call to generate_sdt_output in the saabuild_usp file.

               
CCR#:          NA98-16301

Input:         char* inbuf       pointer to the input buffer containing the
                                    intermediate data buffer
               int max_bufsize   maximum buffer size for outbuf
                  
Output:        char* output      pointer to the output buffer where the 
                                    final product will be created
                  
Returns:       returns SAA_SUCCESS or SAA_FAILURE
                  
Globals:       none

Notes:         none
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 8/08/03 - Zittel
               11/05/2004	SW CCR NA04-30810	Changes for Build8 (Clear 
               						dates/times for product 
               						message type -2; Set hours 
               						to 0 if they equal to 24. 
 	       03/09/2006	SW CCR NA06-06603	Changes for Build9. Display
 	       						beginning date/time for first
 	       						hour used in USD/USW.
	       02/04/2009	SW CCR NA08-21638/21654	Until requisite hours available,
							set start/end date/time to
							user requested dates/times else
							display actual dates/times.
							TAB dates/times are forced to
							match product description block
							halfwords 48-51. 
*******************************************************************************/


/* local include file */
#include "saaaccum_usp.h"
#include "saausers_main.h"

#define MIN_PER_DAY  1440
#define MIN_PER_HR     60
#define TWO_THIRDS   0.666
#define MAX_ACCUM_CAP 32767

extern int julian_minutes[MAX_HOURS];

int sort_jul_min(int, int);

int generate_usp_output(char *outbuf, int lbd, int prod_id,int rqst_num){
   int i,j,k;           /* loop variables                                     */
   int debugit=FALSE;     /* flag to turn on debugging output to stderr         */

   int result;
   int usp_flg = FALSE;
   int minutes, index;
   int hour_count = 0;
   
   switch (prod_id)
   {
   case USWACCUM:
   {
      usp.max_swu = 0;
      memset(usp.total_swu,0, sizeof(usp.total_swu));
      memset(usp.swu_avail_hrs,0,sizeof(usp.swu_avail_hrs));
      hour_count = sort_jul_min(userreq.swu_beg_minutes[rqst_num],
                                userreq.swu_end_minutes[rqst_num]);

/*  Do we have at least two-thirds of the required number of observations?  */

      usp.swu_hour_cnt = hour_count;
      if((float)hour_count/(float)userreq.swu_num_hours[rqst_num] > TWO_THIRDS){
         if(debugit){fprintf(stderr,"hour_count = %d\n",hour_count);}
         usp_flg = TRUE;
      }
      else{
	 hskp_data.usr_first_date = userreq.swu_beg_minutes[rqst_num]/MIN_PER_DAY;	  /* Build 12  */
	 hskp_data.usr_first_time = (userreq.swu_beg_minutes[rqst_num] - 
                                     hskp_data.usr_first_date*MIN_PER_DAY);               /* Build 12  */
	 hskp_data.usr_last_date = userreq.swu_end_minutes[rqst_num]/MIN_PER_DAY;	  /* Build 12  */
	 hskp_data.usr_last_time = (userreq.swu_end_minutes[rqst_num] -
				    hskp_data.usr_last_date*MIN_PER_DAY);                 /* Build 12  */
         usp_flg = -2;
         }

      if(debugit){
         for (i=0;i<MAX_HOURS;++i)
           fprintf(stderr,"i = %d, flag = %d, \n",i,hskp_data.data_avail_flag[i]);
         fprintf(stderr,"MAX_HOURS = %d\n", MAX_HOURS);
         fprintf(stderr,"Rqst_num = %d, Begtim = %d, Endtim = %d\n", rqst_num,
                userreq.swu_beg_minutes[rqst_num], userreq.swu_end_minutes[rqst_num]);
       }
      for(k=0;k<hour_count;++k){
         index = usp.time_order[k];
         minutes = julian_minutes[index];
         if(debugit){fprintf(stderr,"148: minutes = %d\n",minutes);}
         usp.swu_avail_hrs[k] = (hskp_data.usr_time[index]+30)/MIN_PER_HR;
         if(debugit){
            fprintf(stderr,"Usp_avail_hr = %d, hour_count = %d\n",
                 usp.swu_avail_hrs[k],k);
         }
         if(usp_flg==TRUE){
            if(debugit){fprintf(stderr,"Adding SWU accumulations for msg %d\n",index+2);}
            for(i=0;i<MAX_SAA_RADIALS;++i)
               for(j=0;j<MAX_SAA_BINS;++j){
                  usp.total_swu[i][j] += usraccum[index].swu_data[i][j];
                  if(usp.total_swu[i][j] < 0)
                     usp.total_swu[i][j] = MAX_ACCUM_CAP;
               }
                  
            if(debugit){
               for (j=0;j<230;++j){
                  fprintf(stderr,"%5d",usraccum[index].swu_data[255][j]);
                  if(j % 15 ==1)
                     fprintf(stderr,"\n");
               }
               fprintf(stderr,"\n");
            }
         } /* end usp_flg test  */
       }  /* end hour_count loop  */

/*  Find the area of the max category in the low scale in case the results need to be set to the 
	high scale.  For Build8 the name of function changed from usp_comp_area to compute_area 
	and put in saalib                                                                       */
      if(usp_flg==TRUE){
         usp.max_swu = compute_area(usp.total_swu[0],MAX_SAA_RADIALS,MAX_SAA_BINS,prod_id);
         }							
      else
         usp.max_swu = 0;
      if(debugit){fprintf(stderr,"USP: max SWU = %d\n",usp.max_swu);}
      break;
   }  /* End case SWUSPACC  */
      
   case USDACCUM:
   {
      usp.max_sdu = 0;
      memset(usp.total_sdu,0, sizeof(usp.total_sdu));
      memset(usp.sdu_avail_hrs,0,sizeof(usp.sdu_avail_hrs));
      hour_count = sort_jul_min(userreq.sdu_beg_minutes[rqst_num],
                                userreq.sdu_end_minutes[rqst_num]);

/*  Do we have at least two-thirds of the required number of observations?  */

      usp.sdu_hour_cnt = hour_count;
      if((float)hour_count/(float)userreq.sdu_num_hours[rqst_num] > TWO_THIRDS){
         if(debugit){fprintf(stderr,"hour_count = %d\n",hour_count);}
         usp_flg = TRUE;
         }
      else{
	 hskp_data.usr_first_date = userreq.sdu_beg_minutes[rqst_num]/MIN_PER_DAY;	  /* Build 12  */
	 hskp_data.usr_first_time = (userreq.sdu_beg_minutes[rqst_num] - 
                                     hskp_data.usr_first_date*MIN_PER_DAY);               /* Build 12  */
	 hskp_data.usr_last_date = userreq.sdu_end_minutes[rqst_num]/MIN_PER_DAY;	  /* Build 12  */
	 hskp_data.usr_last_time = (userreq.sdu_end_minutes[rqst_num] -
				    hskp_data.usr_last_date*MIN_PER_DAY);                 /* Build 12  */
         usp_flg = -2;
         }
      if(debugit){
         for (i=0;i<MAX_HOURS;++i)
           fprintf(stderr,"i = %d, flag = %d, \n",i,hskp_data.data_avail_flag[i]);
         fprintf(stderr,"MAX_HOURS = %d\n", MAX_HOURS);
         fprintf(stderr,"Rqst_num = %d, Begtim = %d, Endtim = %d\n", rqst_num,
                userreq.sdu_beg_minutes[rqst_num], userreq.sdu_end_minutes[rqst_num]);
      }

      for(k=0;k<hour_count;++k){
         index = usp.time_order[k];
         minutes = julian_minutes[index];
         if(debugit){fprintf(stderr,"149: minutes = %d\n",minutes);}
         usp.sdu_avail_hrs[k] = (hskp_data.usr_time[index]+30)/MIN_PER_HR;
         if(debugit){
            fprintf(stderr,"Usp_avail_hr = %d, hour_count = %d\n",
                      usp.sdu_avail_hrs[hour_count],hour_count);
            }
         if(usp_flg==TRUE){
            if(debugit){fprintf(stderr,"Adding SDU accumulations for msg %d\n",index+2);}
            for(i=0;i<MAX_SAA_RADIALS;++i)
               for(j=0;j<MAX_SAA_BINS;++j){
                  usp.total_sdu[i][j] += usraccum[index].sdu_data[i][j];
                  if(usp.total_sdu[i][j] < 0)
                     usp.total_sdu[i][j] = MAX_ACCUM_CAP;
               }
          } /*  end usp_flg test  */
       }  /* hour_count loop  */


/*  Find the area of the max category in the low scale in case the results need to be set to the 
	high scale.  For Build8 the name of function changed from usp_comp_area to compute_area 
	and put in saalib                                                                       */
      if(usp_flg==TRUE){
         usp.max_sdu = compute_area(usp.total_sdu[0],MAX_SAA_RADIALS,MAX_SAA_BINS,prod_id);
         }
      else
         usp.max_sdu = 0;
      if(debugit){fprintf(stderr,"USP: max SDU = %d\n",usp.max_sdu);}
      break;
   } /*  end case SDUSPACC  */
   default:
        {
	if(debugit){fprintf(stderr,"Prod_id not SWU or SDU; value = %d\n",prod_id);}
	return(-1);
	}
  }  /* end switch  */
  if(debugit){fprintf(stderr,"USP: usp_flg = %d, rqst_num = %d\n",usp_flg,rqst_num);}
  result=generate_sdt_output(outbuf,BUFSIZE,prod_id,usp_flg,rqst_num);
  return (result);
}


/*  Sorting routine based on Julian minutes  */
int sort_jul_min(int min_time, int max_time)
{
   int i,j,k,count;
   int index[MAX_HOURS];
   int temp, temp1, temp_minutes;
   int debugit = FALSE;
   
   k = 0;
   count = 0;
   for(i=0;i<MAX_HOURS;++i)
     index[i] = i;
   for(i=0;i<MAX_HOURS-1;++i){
     temp = julian_minutes[index[i]];

     if(temp < 0)
        continue;
        
     for(j = i+1;j<MAX_HOURS;++j){
       temp1 = julian_minutes[index[j]];
       if(temp1< 0)
          continue;
       if(temp1<temp){
         temp = temp1;
         k = index[j];
         index[j] = index[i];
         index[i] = k;
        }
      }
   }

   hskp_data.usr_first_date = 0;
   hskp_data.usr_first_time = 0;
   hskp_data.usr_last_date = 0;
   hskp_data.usr_last_time = 0;
   if(hskp_data.data_avail_flag[index[0]] > 0){
      hskp_data.usr_first_date = hskp_data.usr_date[index[0]];
      hskp_data.usr_first_time = hskp_data.usr_time[index[0]];
      hskp_data.usr_last_date = hskp_data.usr_first_date;
      hskp_data.usr_last_time = hskp_data.usr_first_time;
   }
   usp.all_hour_cnt = 0;
   for(i=0;i<MAX_HOURS;++i){
     if(hskp_data.data_avail_flag[index[i]] > 0 ){
       hskp_data.usr_last_date = hskp_data.usr_date[index[i]];
       hskp_data.usr_last_time = hskp_data.usr_time[index[i]];
       /*  Test code added 12/22/2003  WDZ  */
       usp.all_avail_hrs[i] = (hskp_data.usr_time[index[i]]+30)/MIN_PER_HR;
       if(usp.all_avail_hrs[i] == 24)
          usp.all_avail_hrs[i] = 0;			/* Build8 Change  */
       if(debugit)
           fprintf(stderr,"all_hour_cnt = %d, hour = %d\n",usp.all_hour_cnt,
                   usp.all_avail_hrs[i]);
       ++usp.all_hour_cnt;
       /*  End test code block added 12/22/2003  WDZ  */
       }
   }
   if(debugit){
      fprintf(stderr,"usp start date = %d\n",hskp_data.usr_first_date);
      fprintf(stderr,"usp start time = %d\n",hskp_data.usr_first_time);
      fprintf(stderr,"usp end date = %d\n",hskp_data.usr_last_date);
      fprintf(stderr,"usp end time = %d\n",hskp_data.usr_last_time);
      }

   for(i=0;i<MAX_HOURS;++i){
   /* Round the julian minutes to the nearest hour */
     temp_minutes = ((julian_minutes[index[i]]+30)/60)*60;
     
     /*  If the julian minutes time is within the bounds user
         requested then save the pointer to the housekeeping data */
     if(temp_minutes > min_time && 
          temp_minutes <= max_time){
          if(debugit){
             fprintf(stderr,"\nMin Time/Jul_Time/Max Time = %d/%d/%d",
                   min_time,temp_minutes,max_time);
             }

       usp.time_order[count] = index[i];
       if(debugit){fprintf(stderr," t_order = %2d, i = %2d\n",usp.time_order[count],i);}
       
       /*  increment the number of hourly observations found */
       count += 1;
       /*  Begin Test block inserted 12/22/2003 WDZ  */
       /*  Next two lines changed 03/09/2006 by WDZ to correct start date/time
           for USD/USW products, CCR NA06-06603                                 */
       hskp_data.usr_first_date = hskp_data.usr_start_date[usp.time_order[0]];
       hskp_data.usr_first_time = hskp_data.usr_start_time[usp.time_order[0]];
       hskp_data.usr_last_date  = hskp_data.usr_date[usp.time_order[count-1]];
       hskp_data.usr_last_time  = hskp_data.usr_time[usp.time_order[count-1]];
       /*  End Test Block inserted 12/22/2003  WDZ  */
       }
   }
   return (count);
}           
