/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:52 $
 * $Id: saa_main.c,v 1.10 2008/01/04 20:54:52 aamirn Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */


/*******************************************************************************
Description:
    saa_main.c is the main file for the Snow Accumulation Algorithm. 
    The algorithm receives HYBSCAN data as input
    and produces one hour, snowfall total accumulations
    From Tim O'Bannon, January 2002.

Input:   none
Output:  none
Returns: none
Globals: none
Notes:   All input and output for this module are provided through
         ORPG API services calls.
*******************************************************************************/

/* Global Include Files ---------------------------------------------------- */
#include "rpgc.h"
#include "saa_main.h"
#include "saaConstants.h"
#include <stdio.h>
#include <stdlib.h>
#include "saa_adapt.h"
#include "input_buffer_struct.h"
#include "saa.h"
#include "saa_Print_Supplemental_Data.h"
#include "saa_generate_intermed_products.h"
#include "saa_file_io.h"
#include "saa_compute_products.h"

/* Include files for File I/O */
#include <sys/file.h>
#include <sys/fcntl.h>
#include <unistd.h>


/****************************************************************************/
int main(int argc, char* argv[])
{
   
/* Variable Declarations and Definitions ***/
   short algProcess 		= TRUE;     	/* Loop control for entire algorithm */
   int  iostat;                 		/* Status from API calls */ 
   HYBSCAN_buf_t* hybrefPtr 	= NULL;		/* Pointer to input buffer of base elevation data */                         	      	  
   char *outbuf;              			/* pointer to output buffer */
   int saa_status               = -1 ;
   int  fd_ohp			= -1;		/* file descriptor */
   int  num_ohp_bytes_read	=  0;		/* number of bytes read from the OHP file*/
   int  num_tot_bytes_read      =  0;
   int  readStatus		=  0;

   int fd_total			= -1;
   int adapt_thr_time_span;
   float adapt_tspan_temp;
   int adapt_thr_time;  
   float adapt_thr_time_temp;
   int rc;                 /* return code from function call */



/* SAA variables declaration ***/
   int new_vol;
   int i;

   	
/* Flag Variables Declaration ***/
  
   data_read_from_files		=  0;    /*in saa_main.h */
   data_read_from_total_files   =  0; 
   lb_descriptor=-1;
   usr_data_available=FALSE;
   
/* Initialize Variables for SAA ***/

  
   previous_ZS_mult	=-1;/*kd 12/29 */
   previous_ZS_power	=-1;
 
 

/* ----------------------------------------------------------------------- */

   fprintf( stderr,"\nBegin CP013 Task 8: Snow Accumulation Algorithm\n" );

/* LABEL:REG_INIT  Algorithm Registration and Initialization Section*/
 
/* Initialize log_error services. */
   RPGC_init_log_services( argc, argv );
   if(SAA_DEBUG){fprintf( stderr,"Registered for log error services.\n");}

/* Register inputs and output */
   RPGC_in_data (HYBRSCAN, VOLUME_DATA );
   RPGC_out_data(SAAACCUM, VOLUME_DATA, 0);
   if(SAA_DEBUG){fprintf( stderr,"Registered for I/O services.\n");}

/* Register for SAA adaptation data        */

    rc = RPGC_reg_ade_callback( saa_callback_fx,
                                &saa_params,
                                SAA_DEA_NAME,
                                BEGIN_VOLUME );
    if ( rc < 0 )
      RPGC_log_msg( GL_ERROR, "SAA: cannot register adaptation data callback function\n");

/* Register for ORPGEVT_RESET_SAAACCUM event. */
   RPGC_reg_for_external_event( ORPGEVT_RESET_SAAACCUM,
                                Event_handler,
                                ORPGEVT_RESET_SAAACCUM );

/* ORPG task initialization routine. Input parameters argc/argv are
 * not used in this algorithm */
   RPGC_task_init( VOLUME_BASED, argc, argv );
   if(SAA_DEBUG){fprintf( stderr, "Initialized task.\n");}
  
  
/*file open 11/6/03 kd */
  
   fd_ohp=open_saa_ohp_file();
   if(fd_ohp == -1){
   		
   	RPGC_log_msg(GL_ERROR,"SAA Main: Error Opening File  SAAHOURLY.DAT\\n");	
   }
   
   if(SAA_DEBUG && (fd_ohp >=0)){
   	fprintf(stderr,"SAAHOURLY.DAT file opened; fd_ohp = %d.\n", fd_ohp);
   }
   
   
  
   fd_total=open_saa_total_files();
   if(fd_total == -1){
   		
   	RPGC_log_msg(GL_ERROR,"SAA Main: Error Opening File  SAATOTAL.DAT\\n");	
   }
   if(SAA_DEBUG && (fd_total >=0)){
   	fprintf(stderr,"SAATOTAL.DAT file opened; fd_total = %d.\n", fd_total);
   }
   
  
  
   if(fd_ohp != -1){
   	lseek(fd_ohp,0,SEEK_SET);
   	/*read back data from SAAHOURLY.DAT */
	readStatus = read_from_files(fd_ohp,&num_ohp_bytes_read);
	if(readStatus != -1){
		if(num_ohp_bytes_read >= sizeof(saa_ohp)){
		  	data_read_from_files = 1;
		}
		else{
			data_read_from_files = 0;
		}
		

	}
   }
   if(fd_total != -1) {
   	lseek(fd_total,0,SEEK_SET);
    	/*read back data from SAATOTAL.DAT */
	readStatus = read_from_saa_total_files(fd_total, &num_tot_bytes_read);
	if(readStatus != -1){
		if(num_tot_bytes_read >= sizeof(saa_swe_storm_total)*4 ){
		  	data_read_from_total_files = 1;
		}
		else{
			data_read_from_total_files = 0;
		}
		

	}
   }
   /* try to read LB as the program starts since 10/18/03 kd */  	
        lb_descriptor =read_LB_Header(SAAUSERSEL, 1 ) ;

        if (SAA_DEBUG){

           for ( i=0; i< NUM_MSGS;i++)
                fprintf(stderr," Date/Time=%d/%d \n",usr_header.usr_date[i],usr_header.usr_time[i]);

        }

        /* if reading lb is Ok then read all the  messages of the existing LB and set the usr_data_available flag to TRUE */
        if (lb_descriptor > 0 )
        {       for ( i=2; i < NUM_MSGS+2; i++)  /* Changed +1 to +2 11/03/03 WDZ */
                {
                        readStatus=read_LB(lb_descriptor,i);
                        if (SAA_DEBUG) { fprintf( stderr, "msg_id =%d , lb Read Status= %d\n",
                                                  i,readStatus); }
                }
                usr_data_available= TRUE;

        }
        /* if the LB does not exist, then create the LB and initialize all data
to default values */


        else if (lb_descriptor < 0 )
	{
		lb_descriptor =create_LB();

		if (lb_descriptor > 0 )
		{
			 /* initialize data elements of the LB to default values */
			 wr_status=init_LB(lb_descriptor);
			 if (wr_status==0)
			 {
				usr_data_available=TRUE;
			 }
			 else
				usr_data_available= FALSE;
		}
			
 	}    
        if(SAA_DEBUG) { fprintf( stderr," usr_data_available: %d \n",usr_data_available); }

/* While loop that controls how long the task will execute. As long as
 * PROCESS remains TRUE, the task will continue. */

   while( algProcess )
   {
   	
        /* get threshold time span and threshold time then convert to float min->hr before 
	   copying  to local structure */
   		
   	memcpy(& adapt_thr_time_span, &(saa_params.g_thr_time_span),sizeof(int));
        adapt_tspan_temp =(float)(adapt_thr_time_span/60.0);
        memcpy(& adapt_thr_time, &(saa_params.g_thr_mn_time),sizeof(int));
        adapt_thr_time_temp=(float)(adapt_thr_time/60.0);
   	        
   	memcpy(& (saa_adapt.cf_ZS_mult) ,&(saa_params.g_cf_ZS_mult),sizeof(float) );
   	memcpy(& (saa_adapt.cf_ZS_power),&(saa_params.g_cf_ZS_power),sizeof(float));
   	memcpy(& (saa_adapt.s_w_ratio), & (saa_params.g_sw_ratio),sizeof(float));
   	memcpy(& (saa_adapt.thr_lo_dBZ),& (saa_params.g_thr_lo_dBZ),sizeof(float));
   	memcpy(& (saa_adapt.thr_hi_dBZ),& (saa_params.g_thr_hi_dBZ),sizeof(float));
   	memcpy(& (saa_adapt.thr_mn_hgt_corr),&(saa_params.g_thr_mn_hgt_corr),sizeof(float));
   	memcpy(& (saa_adapt.cf1_rng_hgt),& (saa_params.g_cf1_rng_hgt),sizeof(float));
   	memcpy(& (saa_adapt.cf2_rng_hgt),& (saa_params.g_cf2_rng_hgt),sizeof(float));
   	memcpy(& (saa_adapt.cf3_rng_hgt),& (saa_params.g_cf3_rng_hgt),sizeof(float));
   	memcpy(& (saa_adapt.thr_time_span),&(adapt_tspan_temp),sizeof(float));
   	memcpy(& (saa_adapt.thr_mn_pct_time),&(adapt_thr_time_temp),sizeof(float));
   	memcpy(& (saa_adapt.use_RCA_flag),& (saa_params.g_use_RCA_flag),sizeof(int));
   	memcpy(& (saa_adapt.rhc_base_elev),& (saa_params.g_rhc_base_elev),sizeof(float));
   	if(SAA_DEBUG) { fprintf( stderr,"GET EXTERNAL ADAPTABLE PARAMS\n"); }
   	if(SAA_DEBUG) { fprintf( stderr,"adapt_tspan_temp:%f\n",adapt_tspan_temp); }
   	if(SAA_DEBUG) { fprintf( stderr,"adapt_thr_time_temp:%f\n",adapt_thr_time_temp); }
   	if(SAA_DEBUG) { fprintf( stderr,"saa_adapt.s_w_ratio:%f\n",saa_adapt.s_w_ratio); }
   	
   	
   	/* Suspend until driving input available. */
        RPGC_wait_act(WAIT_DRIVING_INPUT);   /*  WDZ 9/16/2003   */

    	if(SAA_DEBUG){fprintf(stderr, "Task_Started\n");}

   	/* Initialize new volume scan */
   	new_vol = 1;

    	/* Open input buffer */
    	/* The buffer pointer needs to be cast to 'HYBSCAN_buf_t*'.
    	 * If the function returns a NULL pointer, then there is an error*/
    	hybrefPtr = (HYBSCAN_buf_t*)RPGC_get_inbuf(HYBRSCAN, &iostat);
    
    	if(SAA_DEBUG){fprintf(stderr,"iostat = %d, NORMAL = %d\n",iostat,NORMAL);}
  
    	if ((iostat != NORMAL) || (hybrefPtr == NULL))
     	{
       		RPGC_log_msg( GL_INFO, "RPGC_get_inbuf HYBRSCAN (%d)\n",iostat);
       		if(hybrefPtr != NULL){
       			RPGC_rel_inbuf((void*)hybrefPtr);
       			hybrefPtr = NULL;
       			if(SAA_DEBUG){fprintf( stderr,"Input Buffer HYBRSCAN released.\n" );}
       		}
       		
       		RPGC_cleanup_and_abort( iostat );
     	}
    	else{	
    		/*get the vcp number using RPGC_get_buffer_vcp_num */
    		currentvcpnumber = RPGC_get_buffer_vcp_num((void*)hybrefPtr);
		if(SAA_DEBUG){fprintf(stderr,"Current VCP from RPGC call: %d\n", currentvcpnumber);}
    		
    		/*copy the input buffer to the local structure */
    		memcpy(&(hybscan_buf.HydroMesg),hybrefPtr->HydroMesg,C_HYZMESG*sizeof(int));  		
    		memcpy(&(hybscan_buf.HydroAdapt),hybrefPtr->HydroAdapt,C_HYZADPT*sizeof(float));
    		memcpy(&(hybscan_buf.HydroSupl),hybrefPtr->HydroSupl,C_HYZSUPL*sizeof(int));
    		memcpy(&(hybscan_buf.HyScanZ),hybrefPtr->HyScanZ,MAX_AZM*MAX_RNG*sizeof(short));
		memcpy(&(hybscan_buf.HyScanE),hybrefPtr->HyScanE,MAX_AZM*MAX_RNG*sizeof(short));
    	   	
    	   	/*copy the supplemental buffer to hybscan_suppl*/
    	   	memcpy(&hybscan_suppl,hybrefPtr->HydroSupl,sizeof(HYBSCAN_suppl_t));
    	   	
    	   	
		/*Release input buffer, hybrefPtr*/
		if(hybrefPtr != NULL){
       			RPGC_rel_inbuf((void*)hybrefPtr);
       			hybrefPtr = NULL;
       			if(SAA_DEBUG){fprintf( stderr,"Input Buffer HYBRSCAN released.\n" );}
       		}/*end if*/
       		
   		
    		/*call the saa function to do all processing */
    		if((saa_status=saa() != 0)){
    			/* an error occured in the processing */
    		
    			
    		}
    		else
    		{
    			
    			if((fd_ohp != -1)){
    				/*right now, setting the write location to zero */
    				lseek(fd_ohp,0,SEEK_SET);
    				write_to_files(fd_ohp);
    				if(SAA_DEBUG){
    				   fprintf(stderr,"Write HOURLY.\n");
    				   for(i=0;i<SAA_OHP_BUFSIZE;++i)
    				      fprintf(stderr,"OHP time[%2d] = %8d\n",i,saa_ohp.swe_buffer[i].time);
    				} 
    			}
    			if(fd_total != -1){
    			/*setting the write location to zero*/
    			lseek(fd_total,0,SEEK_SET);
    			
    			write_to_saa_total_files(fd_total);
    			}
    			/* Obtain the output buffer for SAAACCUM*/
    			outbuf = (char*)RPGC_get_outbuf(SAAACCUM,SAA_BUFSIZE,&iostat);
    		
    			if(iostat != NORMAL){
    				RPGC_log_msg(GL_INFO,
    			 	"SAA Abort: Error Obtaining Output Buffer. opstat = %d\n",iostat);
    			 	if(iostat == NO_MEM){
    			 		RPGC_abort_datatype_because(SAAACCUM,PROD_MEM_SHED);
    			 	}
    			 	else{
    			 		RPGC_abort_datatype_because(SAAACCUM,iostat);
    			 	}
    			 	
    				
    			}	
    			else {
    				if(SAA_DEBUG){fprintf(stderr,"Intermediate Output buffer acquired.\n");}
    				/*create intermediate buffer*/
    				generate_intermediate_products(outbuf,SAA_BUFSIZE);
    				if(SAA_DEBUG){fprintf(stderr,"Intermediate Output product created.\n");}
    				/* releasing output buffer */
       				RPGC_rel_outbuf(outbuf,FORWARD);
    		
    			} /*end output stat block*/
    			/*to keep track any changes of the cf_ZS_mult and cf_ZS_power whenever saa() is called 12/29 kd */
    			previous_ZS_mult=saa_adapt.cf_ZS_mult;
	                previous_ZS_power=saa_adapt.cf_ZS_power;
	                if (SAA_DEBUG) { fprintf(stderr,"previous_ZS_mult %f\n",previous_ZS_mult);}
	                if (SAA_DEBUG) { fprintf(stderr,"previous_ZS_power %f\n",previous_ZS_power);}
    		
    		} /*else for good return from call to saa */
       		
    	}/*end else for get input buffer */       	
       	
   } /* end while(algProcess) */
   
   
   fprintf( stderr,"\nSAA Program Terminated\n" );
   return (0);

} /* END of  MAIN */
/****************************************************************************/
