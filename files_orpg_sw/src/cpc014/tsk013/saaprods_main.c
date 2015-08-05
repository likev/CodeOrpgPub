/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:40:37 $
 * $Id: saaprods_main.c,v 1.8 2014/03/18 14:40:37 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaprods_main.c

Description:   cpc013/tsk009 is the product producting portion of the
               SAA (Snow Accumulation) Algorithm. The front-end processing
               for this algorithm can be found in cpc013/tsk008.
               
               tsk009 takes intermediate product 280 from tsk008 and
               creates four ICD-compliant output products:
               - Product 144: OSW One Hour Snow Accumulation (Water Equiv.) 
               - Product 145: OSD One Hour Snow Accumulation (Depth)
               - Product 146: SSW Storm Total Snow Accumulation (Water Equiv.)
               - Product 147: SSD Storm Total Snow Accumulation (Depth)
               
               Each product contains a symbology layer populated with an
               AF1F packet (16-level run length encoded radials) and a 
               TAB (Tabular Alphanumeric Block) containing adaptable 
               parameters. If no accumulation is available a null symbology
               layer is generated with text messages using 0001 packet.
               
               Program Flow:
               
                        generate_TAB
                              |
               main -  generate_saa - build_symbology_layer - output(main)
                                               |
                                         run_length_encode
               
               tsk009 is controlled by a loop within main that is queued to
               wait for any input. When either intermediate product 280 or 281
               has been generated an API call is used to determine which 
               buffer has been populated. With this information, an API call
               is used to determine if a request for a final product associated
               with the intermediate product has been generated. If so, the
               request is routed into either the snow accumulation or user
               routine. Independent routines handle processing of the 
               intermediate data for each data type. Radial information is
               placed into a 2 dimensional array (with resolution of 1kmx1deg)
               which is then passed to the symbology block generator. Here the
               data are sent, radial by radial to the run length encode module.
               The output is placed into the output buffer according to AF1F
               packet specifications. Once complete, the flow returns to the
               product generation routine which calls on the TAB module.
               Finally, with the product generation completed within the 
               output buffer, control is returned to main where if no errors
               were reported the final product is forwarded to the product
               database linear buffer.
               
               Modules used in tsk009 ------------------------------------------
               saaprods_main.c         main routine and task control loop
               saaprods_main.h
               
               saaprods_saa.c          product generation driver for
               saaprods_saa.h          reflectivity data
               
               saaprods_symb_layer.c   symbology layer generator and driver
               symb_layer.h            for run length encoder routines
               saa_nullsymb_layer.c
               saa_nullsymb_layer.h
               
               build_saa_color_tables.c  color tables for the reflectivity and
               build_saa_color_tables.h  Doppler products
               
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
               
*******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
/* Local include file */
#include "saaprods_main.h"
#include "saa_adapt.h"

/*  The following flag is added for compatibility with the library function
    radial_run_length_encode.  It is not used by this task, saaprods          */
int hi_sf_flg;

Scan_Summary *vol_summary;

/*******************************************************************************
Description:      this is the main processing routine for cpc013/tsk009. It 
                  contains the product registration, adaptation access and
                  all linear buffer receive/transmit routines. All input and
                  output buffers are passed to processing routines by pointer
                  which returns a fail or success condition upon completion.
                  
Input:            none
Output:           none
Returns:          none
Globals:          none
Notes:            All input and output for this module are provided through 
                  ORPG API services calls.
*******************************************************************************/

int main(int argc,char* argv[]) {

  /* variable declarations and definitions ---------------------------------- */
  
   int j;
   int debugit=FALSE;         /* test flag: set to TRUE for diag messages       */
   int PROCESS=TRUE;        /* constant flag used for while loop              */
   int PROD_ID;             /* counter for finding products to make           */
   char *inbuf;             /* pointer to an input buffer                     */
   char *outbuf;            /* pointer to an output buffer                    */
   int result;              /* variable to hold function results              */
   int bufferLength;
   int vcp,vnum;
   int vtime;
   /* ----------------------------------------------------------------------- */
 
   if(debugit)
     fprintf(stderr,"\nBegin Snow Accumulation Product Generator cpc013/tsk009\n");

   /* register the input now as Volume-based data                          */
   RPGC_in_data(SAAACCUM,VOLUME_DATA);

   /* four output products are possible depending on the request.              */
   RPGC_out_data(OSDACCUM,VOLUME_DATA,OSDACCUM);
   RPGC_out_data(OSWACCUM,VOLUME_DATA,OSWACCUM);
   RPGC_out_data(SSDACCUM,VOLUME_DATA,SSDACCUM);
   RPGC_out_data(SSWACCUM,VOLUME_DATA,SSWACCUM);

   /* register algorithm infrastructure to read the Scan Summary Array         */
   RPGC_reg_scan_summary();

   /* ORPG task initialization routine. Input parameters argc/argv are         */
   /* not used in this algorithm. task timing is elevation-based.              */
   RPGC_task_init(VOLUME_BASED,argc,argv);
   
   /*  Build the saa color lookup tables                                       */
   build_saa_color_tables();

   /* While loop controls how long the task will execute. As long             */
   /* as PROCESS remains true, the task will continue. The task will          */
   /* terminate only upon a fatal error condition                             */
   while(PROCESS) {
      int opstatus;        /* holds return status from RPGC calls             */

      if(debugit)
         fprintf(stderr,"\n\n->SAA PRODS waiting for any data now\n");

      /* wait for any of the inputs to become available.                      */
      RPGC_wait_act(WAIT_DRIVING_INPUT);
      
      if(debugit)
         fprintf(stderr,"\n\n-> Passed wait for inbuf any -> begin processing\n");
         
      /* obtain and populate the input buffer with intermediate data          */
      inbuf = (char *)RPGC_get_inbuf(SAAACCUM,&opstatus);
      if(opstatus == NORMAL){
         if(debugit){
            fprintf(stderr,"-> Passed get inbuf. opstat=%d\n", opstatus);
            }

         vnum = RPGC_get_current_vol_num();
         if(debugit){fprintf(stderr,"vnum = %d\n",vnum);}
         vol_summary = (Scan_Summary *)RPGC_get_scan_summary( vnum);
         vtime = vol_summary->volume_start_time;
         if(debugit){fprintf(stderr,"vtime = %d\n",vtime);}
         bufferLength = RPGC_get_inbuf_len( (char*)inbuf);
         if(debugit){
            fprintf(stderr,"volume number = %d\n",vnum);
            fprintf(stderr,"size of input buffer is %d\n",bufferLength);
            }
         if(bufferLength != sizeof(saa_inp)){
            RPGC_rel_inbuf(inbuf);
            RPGC_log_msg( GL_ERROR,
                "SAAPRODS ABORT88: Buffer length incorrect. Length = %d\n",
                                 bufferLength);
            if(debugit){fprintf(stderr,"Main: bufferLength = %d, saa_inp = %d\n",bufferLength,sizeof(saa_inp));}
            continue;
            }
         else{
            memcpy(&saa_inp, inbuf, sizeof(saa_inp));
            /*  Release the input buffer  */
            RPGC_rel_inbuf(inbuf);
            if(debugit){fprintf(stderr,"Main:ohp_end_time = %d\n",saa_inp.ohp_end_time);}
            }

         if(debugit){
            fprintf(stderr,"date = %d, time = %d\n",saa_inp.begin_date,saa_inp.begin_time);
            fprintf(stderr,"VCP = %d\n",saa_inp.vcp_num);
            for (j=0;j<230;++j){
               fprintf(stderr,"%5d",saa_inp.sdo_data[45][j]);
               if(j % 15 ==1)
               fprintf(stderr,"\n");
               }
            }

/* Check to see which products have been requested and open output buffers    */

         for(PROD_ID = OSWACCUM;PROD_ID <= SSDACCUM ;++PROD_ID){ 
         /* check to see if there is a request for PROD_ID                    */
            if(RPGC_check_data(PROD_ID) == NORMAL) {
      
               if(debugit)
                  fprintf(stderr,"-> Begin Processing for PROD_ID = %d\n",PROD_ID);
         
               /* Obtain an output buffer for PROD_ID                          */
               outbuf=(char*)RPGC_get_outbuf(PROD_ID,BUFSIZE,&opstatus);
         
               if(opstatus!=NORMAL) {
                  RPGC_log_msg( GL_INFO,
                     "SAAPRODS ABORT8: Error obtaining output buffer. opstat=%d\n",
                            opstatus);
                  if(opstatus==NO_MEM)
                     RPGC_abort_datatype_because(PROD_ID, PROD_MEM_SHED);
                  else
                     RPGC_abort_datatype_because(PROD_ID, opstatus);
                  continue;
                  }

                result=generate_sdt_output(outbuf,BUFSIZE,PROD_ID);
         
                /* upon successful completion of the product release the buffer   */
                if(result==0)
                   RPGC_rel_outbuf(outbuf,FORWARD);
                else {
                   RPGC_log_msg( GL_INFO,"SAAPRODS ABORT3: outbuf destroy\n");
                   RPGC_rel_outbuf(outbuf,DESTROY);
                   RPGC_abort_datatype_because(PROD_ID, PROD_CPU_SHED);
                   continue;
                   }
            
                if(debugit)
                   fprintf(stderr,"-> Processing complete for %d\n",PROD_ID);
         
               }  /* end of check_data block                                        */
   
            else {
 	      /*get the vcp number using RPGC_get_buffer_vcp_num  */
 	         if(debugit){
	            vcp = RPGC_get_buffer_vcp_num((void*)inbuf);
          fprintf(stderr,"Current VCP: %d, Vol Num:%d\n", vcp,vnum);
                 fprintf(stderr,"-> No Requests for PROD_ID = %d Found: vol %d\n",
                               PROD_ID,vnum);
                 }
              }
            }  /*  end of PROD_ID loop for SWO, SDO, SWT, and SDT */
      } 
   else{  /*  Non-normal inbuf processing  */

      RPGC_log_msg( GL_INFO,
         "SAAPRODS ABORT7: Error obtaining input buffer. opstat=%d\n",opstatus);
      if(opstatus==TERMINATE)
          RPGC_abort();
      else
          RPGC_abort_datatype_because(SAAACCUM, opstatus);
      continue;
      }  /*  End of inbuf processing  */
  
   }  /* end of while process loop ========================================== */

   return 0;

}  /* end of main processing loop =========================================== */
