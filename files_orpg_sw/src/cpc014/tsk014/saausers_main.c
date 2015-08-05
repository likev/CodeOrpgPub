/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:40:38 $
 * $Id: saausers_main.c,v 1.9 2014/03/18 14:40:38 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saausers_main.c

Description:   cpc014/tsk014 is the product producting portion of the
               SAA (Snow Accumulation) Algorithm. The front-end processing
               for this algorithm can be found in cpc013/tsk008.
               
               tsk014 takes intermediate product 280 from tsk008 and
               creates two ICD-compliant output products:
               - Product 150: USW User Select. Snow Accumulation (Water Equiv.)
               - Product 151: USD User Select. Snow Accumulation (Depth)
               
               Each product contains a symbology layer populated with an
               AF1F packet (16-level run length encoded radials) and a 
               TAB (Tabular Alphanumeric Block) containing adaptable 
               parameters. If no accumulation is available a null symbology
               layer is generated with text messages using 0001 packet.
               
               Program Flow:
               
                        generate_TAB
                              |
               main - generate_usr - build_symbology_layer -output(main)
                                             |
                                      run_length_encode
               
               tsk014 is controlled by a loop within main that is queued to
               wait for any input. When intermediate product 280
               has been generated an API call is used to determine which 
               products users have requested.  Radial information is
               placed into a 2 dimensional array (with resolution of 1kmx1deg)
               which is then passed to the symbology block generator. Here the
               data are sent, radial by radial to the run length encode module.
               The output is placed into the output buffer according to AF1F
               packet specifications. Once complete, the flow returns to the
               product generation routine which calls on the TAB module.
               Finally, with the product generation completed within the 
               output buffer, control is returned to main where if no errors
               were reported the final product is forwarded to the product
               database linear buffer.  Alternatively, if there is insufficient 
               data to produce the requested product, an appropriate no-data 
               message is placed in the symbololgy layer, using packet 01, 
               
	       This is the main processing routine for cpc014/tsk014. It 
               contains the product registration, adaptation access and
               all linear buffer receive/transmit routines. All input and
               output buffers are passed to processing routines by pointer
               which returns a fail or success condition upon completion.
               
               Modules used in tsk014 ------------------------------------------
               saausers_main.c         main routine and task control loop
               saausers_main.h
               
	       get_saa_usp_requests.c  determine requests for USW and USD
	       
               saabuild_usp.c          product generation driver
               saabuild_usp.h         
               
               saaaccum_usp.c		accumulates hourly observations
               saaaccum_usp.h 		from users' requests.
               
               saausers_symb_layer.c   symbology layer generator and driver
               saausers_symb_layer.h        for run length encoder routines
               
               saausers_nullsymb_layer.c  symbology later generator and driver
               saausers_nullsymb_layer.h	when data is not available
               
               build_saa_color_tables.c  color tables for the USW and USD
               build_saa_color_tables.h     products
               
               Run Length Encoding Routines ------------------------------------
               
               radial_run_length_encode.c  source to perform run length
               radial_run_length_encode.h  encoding on radial data
               
               padfront.c                source to pad the beginning of
               padfront.h                radials with 0's
                
               padback.c                 source to pad the end of radials
               padback.h                 with 0's
                
               short_isbyte.c            source to shift run length 
               short_isbyte.h            encoded bytes to proper position
                
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist, Applications Branch, Radar Operations Center
              
               
History:
               Initial implementation 09/05/03 - Zittel
               
Notes:            All input and output for this module are provided through 
                  ORPG API services calls.
*******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
/* Local include file */
#include "saausers_main.h"
#include "saa_adapt.h"

#define MSECS_IN_HOUR  3600000
#define MIN_IN_DAY    1440

User_array_t *SAA_requests;

int main(int argc,char* argv[]) {

  /* variable declarations and definitions ---------------------------------- */
  
   int i,j;
   int debugit=FALSE;         /* test flag: set to TRUE for diag messages       */
   int PROCESS=TRUE;        /* constant flag used for while loop              */
   int MISSING_DATE = -1;   /* constant flag used to identify missing hourly
                               data                                           */
   int PROD_ID;             /* counter for finding products to make           */
   char *inbuf;             /* pointer to an input buffer                     */
   char *outbuf;            /* pointer to an output buffer                    */
   int result;              /* variable to hold function results              */
   int last_LB_date[MAX_HOURS];  /*  variable to save clock hour dates        */
   int last_LB_time[MAX_HOURS];  /*  variable to save clock hour times	      */
   int n_requests;
   int bufferLength;
   int vnum;
   int vol_date;
   int vol_hour;
   int rd_status = -1;
   int user_lb_avail = FALSE;
   int lbd_read = 0;
   int req_indx;
   int req_total;
   int out_status;
   /* ----------------------------------------------------------------------- */
 
   if(debugit)
     fprintf(stderr,"\nBegin Snow Accumulation User Select Prod Gen. cpc013/tsk010\n");

   /* register the input now as Volume-based data                          */
   RPGC_in_data(SAAACCUM,VOLUME_DATA);

   /* six output products are possible depending on the request.              */
   RPGC_out_data(USDACCUM,VOLUME_DATA,USDACCUM);
   RPGC_out_data(USWACCUM,VOLUME_DATA,USWACCUM);

   /* register algorithm infrastructure to read the Scan Summary Array         */
   RPGC_reg_scan_summary();

   /* ORPG task initialization routine. Input parameters argc/argv are         */
   /* not used in this algorithm. task timing is elevation-based.              */
   RPGC_task_init(VOLUME_BASED,argc,argv);
   
   /*  Build the saa color lookup tables                                       */
   build_saa_color_tables();

/*  Initialize last_LB arrays.  The latest linear buffer date and time will 
    be compared to contents of of these arrays.					*/
   for(i=0;i<MAX_HOURS;++i){
   	last_LB_date[i] = 0;
   	last_LB_time[i] = 0;
   }
   /*  Open data base file for Read/Write                                     */
   RPGC_data_access_open( SAAUSERSEL, LB_READ|LB_WRITE );

   /* While loop controls how long the task will execute. As long             */
   /* as PROCESS remains true, the task will continue. The task will          */
   /* terminate only upon a fatal error condition                             */
   while(PROCESS) {

      int opstatus;        /* holds return status from RPGC calls             */

      if(debugit)
         fprintf(stderr,"\n\n->SAA USERS waiting for any data now\n");

      /* wait for any of the inputs to become available.                      */
      RPGC_wait_act(WAIT_DRIVING_INPUT);
      
      if(debugit)
         fprintf(stderr,"\n\n-> Passed wait for all -> begin processing\n");
         
      /* obtain and populate the input buffer with intermediate data          */
      inbuf = (char *)RPGC_get_inbuf( SAAACCUM, &opstatus );
    
      if(debugit){
         fprintf(stderr,"-> Passed get inbuf. opstat=%d\n", opstatus);
      }
/* ==================================================================== */
      
      if(opstatus == NORMAL) {

         vnum = RPGC_get_current_vol_num();
         if(debugit){fprintf(stderr,"volume number = %d\n",vnum);}

/*  Try to open and read the header message in the file SAAUSERSEL.DAT  */

         lbd_read = read_USRSELHDR_lb( 1 );
         if(debugit){
            if(lbd_read > 0){
               for(i=0;i<MAX_HOURS;++i)
                  fprintf(stderr,"Date/time/flg = %5d/%04d/%d\n",hskp_data.usr_date[i],
                     hskp_data.usr_time[i],hskp_data.data_avail_flag[i]);
               }
         }
         if(lbd_read > 0){
            for(i=0;i<MAX_HOURS ;++i){
               if(debugit){
                  fprintf(stderr,"Date/time/flg = %5d/%04d/%d\n",hskp_data.usr_date[i],
                     hskp_data.usr_time[i],hskp_data.data_avail_flag[i]);
                  }
/*  If the current date and time read from linear buffer has changed for 
    position i, then read the linear buffer to get the new clock hour data.
    Upon task start-up, all the time slots will need to be read.            */

               if(last_LB_date[i] != hskp_data.usr_date[i] || 
                  last_LB_time[i] != hskp_data.usr_time[i]){
                  rd_status = read_SAAUSERSEL_lb(i+2);
		  
	          if(rd_status != 1){

/*   We did not successfully read the clock hour data so flag the data as 
     unavailable                                                         */
	          	hskp_data.usr_date[i] 		= MISSING_DATE;
	          	hskp_data.usr_time[i] 		= MISSING_DATE;
	          	hskp_data.data_avail_flag[i] 	= 0;
                  }
                  if(debugit){fprintf(stderr,"i = %d, rd_status = %d\n",i,rd_status);}
            
                  if(hskp_data.usr_date[i] > 0 )
                     julian_minutes[i] = hskp_data.usr_date[i]*MIN_IN_DAY + hskp_data.usr_time[i];
                  else
                     julian_minutes[i] = MISSING_DATE;
                
/*  Save the clock hour housekeeping data for future checking            */
                  last_LB_date[i] = hskp_data.usr_date[i];
                  last_LB_time[i] = hskp_data.usr_time[i];
                
                  if(debugit){
                     fprintf(stderr,"msg_id = %d, lb read status = %d\n",i, rd_status);
                     fprintf(stderr,"Date/time/jul_min/flg = %5d/%04d/%10d/%d\n",hskp_data.usr_date[i],
                         hskp_data.usr_time[i],julian_minutes[i],hskp_data.data_avail_flag[i]);
                  }
               }
            user_lb_avail = TRUE;
            }

         }
         else
            user_lb_avail = FALSE;

         if(debugit){fprintf(stderr,"user_lb_avail = %d\n",user_lb_avail);}
         if(user_lb_avail == FALSE){
            RPGC_rel_inbuf(inbuf);
            if(debugit)
              fprintf(stderr,"Released inbuf, user_lb not available\n");
            RPGC_abort();
            continue;
            }

         bufferLength = RPGC_get_inbuf_len( (char*)inbuf);
         if(bufferLength != sizeof(saa_inp)){
            RPGC_rel_inbuf(inbuf);
            RPGC_log_msg( GL_ERROR, "ABORT8: Buffer length incorrect, length = %d\n", bufferLength);
            if(debugit){fprintf(stderr,"Main: bufferLength = %d\n",bufferLength);}
            continue;
            }
         else{
           memcpy(&saa_inp, inbuf, sizeof(saa_inp));
           if(debugit){fprintf(stderr,"Main:time_span_thresh = %f\n",saa_inp.thr_time_span);}
           }
           

         if(debugit){
           fprintf(stderr,"date = %d, time = %d\n",saa_inp.begin_date,saa_inp.begin_time);
           fprintf(stderr,"VCP = %d\n",saa_inp.vcp_num);
           for (j=0;j<230;++j){
             fprintf(stderr,"%5d",saa_inp.sdo_data[255][j]);
             if(j % 15 ==1)
              fprintf(stderr,"\n");
              }
          }
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
      }  /*  End of SAAACCUM Block  */
     

   else if(opstatus!=NORMAL) {

      /* error check from get inbuf specifically for SAAACCUM     */
      	   if(debugit)
      	      fprintf(stderr,"ABORT7: Non-normal opstatus = %d\n",opstatus);

            RPGC_log_msg( GL_INFO,
               "SAAUSERS ABORT7: Error obtaining input buffer. opstat=%d\n",
               opstatus);
            if(opstatus==TERMINATE)
               RPGC_abort();
            else
               RPGC_abort_datatype_because(SAAACCUM, opstatus);
            continue;
     }

   /*  Release the input buffer  */
   if(debugit)
     fprintf(stderr,"Primary point to release inbuf\n");
   RPGC_rel_inbuf(inbuf);

/* Check to see which products have been requested and open output buffers    */
       
/*  Get Scan Summary                         */
      RPGC_get_scan_summary(vnum);
      vol_date = ORPGVST_get_volume_date();
      vol_hour = ORPGVST_get_volume_time()/(MSECS_IN_HOUR);
      if(debugit){fprintf(stderr,"Volume Date/Hour = %d/%d\n",vol_date,vol_hour);}

/*  Get product parameters for usp products  */

      n_requests = get_saa_usp_requests(vol_date,vol_hour);
      if(n_requests>0){

        if(debugit){fprintf(stderr,"No. requests = %d\n",n_requests);}
        for(PROD_ID=USWACCUM;PROD_ID<=USDACCUM;++PROD_ID){

	   if(PROD_ID == USWACCUM)
       	      req_total = userreq.swu_num_rqsts;
       	   else
              req_total = userreq.sdu_num_rqsts;

           if(debugit){fprintf(stderr,"Request Total = %d\n",req_total);}
           for(i =0;i<req_total;i++){
  
              if(PROD_ID == USWACCUM)
                 req_indx = userreq.swu_req_indx[i];
              else
                 req_indx = userreq.sdu_req_indx[i];
              if(debugit){
                 fprintf(stderr,"-> Begin Processing for PROD_ID = %d, req_indx = %d\n",
                                PROD_ID, req_indx);
                 }
  
              /* Obtain an output buffer for PROD_ID                          */
              outbuf=(char*)RPGC_get_outbuf_for_req(PROD_ID,BUFSIZE,
                    &SAA_requests[req_indx], &out_status);

              if(out_status!=NORMAL) {
                 if(debugit){fprintf(stderr,"Error obtaining output buffer. out_status = %d\n",out_status);}
                 RPGC_log_msg( GL_INFO,
                    "SAAUSERS ABORT8: Error obtaining output buffer. opstat=%d\n",out_status);
                 RPGC_abort_request(&SAA_requests[req_indx], opstatus);
                 continue;
               }
               if(debugit){
	          fprintf(stderr,"req_indx = %d, user_pcode = %d, ",req_indx,PROD_ID);
	          fprintf(stderr,"p0/1/2/3/4/5 = %6d, ",SAA_requests[req_indx].ua_dep_parm_0);
	          fprintf(stderr," %6d, ",SAA_requests[req_indx].ua_dep_parm_1);
	          fprintf(stderr," %6d, ",SAA_requests[req_indx].ua_dep_parm_2);
	          fprintf(stderr," %6d, ",SAA_requests[req_indx].ua_dep_parm_3);
	          fprintf(stderr," %6d, ",SAA_requests[req_indx].ua_dep_parm_4);
	          fprintf(stderr," %6d, ",SAA_requests[req_indx].ua_dep_parm_5);
	          fprintf(stderr,"elidx = %2d, ",SAA_requests[i].ua_elev_index);
	          fprintf(stderr,"rqst# = %3d, ",SAA_requests[i].ua_req_number);
	          fprintf(stderr,"spare = %d\n",SAA_requests[i].ua_spare);
        	  }     

               result = generate_usp_output(outbuf,lbd_read,PROD_ID,i);

               /* upon successful completion of the product release the buffer   */
               if(result==0)
                  RPGC_rel_outbuf(outbuf,FORWARD);
               else {
                  RPGC_log_msg( GL_INFO,"SAAUSERS ABORT3: outbuf destroy\n");
                  RPGC_rel_outbuf(outbuf,DESTROY);
                  RPGC_abort_datatype_because(PROD_ID, PROD_CPU_SHED);
                  continue;
               }
               if(debugit)
                  fprintf(stderr,"-> Processing complete for %d\n",PROD_ID);
           }  /*  end of request loop */
        }  /* End product loop  */
       free(SAA_requests);
      }  /* end if n_requests > 0 */
   }  /* end of while process loop ========================================== */

   return(0);

}  /* end of main processing loop =========================================== */
