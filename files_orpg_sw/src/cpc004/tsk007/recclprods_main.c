/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/03/30 23:16:17 $
 * $Id: recclprods_main.c,v 1.6 2006/03/30 23:16:17 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/*******************************************************************************
Module:        recclprods_main.c

Description:   cpc004/tsk007 is the back-end/product producting portion of the
               REC (Radar Echo Classifier) Algorithm. The front-end processing
               for this algorithm can be found in cpc004/tsk006.
               
               tsk007 takes intermediate products 298 and 299 from tsk006 and
               creates two ICD-compliant output products:
               - Product 132: CLR Clutter Likelihood Reflectivity 
               - Product 133: CLD Clutter Likelihood Doppler
               
               Each product contains a symbology layer populated with an
               AF1F packet (16-level run length encoded radials) and a 
               TAB (Tabular Alphanumeric Block) containing adaptable 
               parameters.
               
               Program Flow:
               
                        generate_TAB
                              |
                        generate_ref
                      /              \
               main -                  build_symbology_layer -output(main)
                      \              /          |
                        generate_dop     run_length_encode
               
               tsk007 is controlled by a loop within main that is queued to
               wait for any input. When either intermediate product 298 or 299
               has been generated an API call is used to determine which 
               buffer has been populated. With this information, an API call
               is used to determine if a request for a final product associated
               with the intermediate product has been generated. If so, the
               request is routed into either the reflectivity or Doppler
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
               
               Modules used in tsk007 ------------------------------------------
               recclprods_main.c         main routine and task control loop
               recclprods_main.h
               
               recclprods_ref.c          product generation driver for
               recclprods_ref.h          reflectivity data
               
               recclprods_dop.c          product generation driver for
               recclprods_dop.h          Doppler data
               
               recclprods_symb_layer.c   symbology layer generator and driver
               symb_layer.h              for run length encoder routines
               
               recclprods_color_table.c  color tables for the reflectivity and
               recclprods_color_table.h  Doppler products
               
               recclprods_helpers.c      helper functions used in both Doppler
               recclprods_helpers.h      and reflectivity processing
               
               Run Length Encoding Routines ------------------------------------
               
               radial_run_length_encode.c  source to perform run length
               radial_run_length_encode.h  encoding on radial data
               
               padfront.c                source to pad the beginning of
               padfront.h                radials with 0's
                
               padback.c                 source to pad the end of radials
               padback.h                 with 0's
                
               short_isbyte.c            source to shift run length 
               short_isbyte.h            encoded bytes to proper position
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_main.c,v 1.6 2006/03/30 23:16:17 steves Exp $
*******************************************************************************/


/* Local include file */
#include "recclprods_main.h"


/*******************************************************************************
Description:      this is the main processing routine for cpc004/tsk007. It 
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
  
   int DEBUG=FALSE;          /* test flag: set to TRUE for diag messages       */
   int PROCESS=TRUE;        /* constant flag used for while loop              */
   char *inbuf;             /* pointer to an input buffer                     */
   char *outbuf;            /* pointer to an output buffer                    */
   int result;              /* variable to hold function results              */
   
   /* ----------------------------------------------------------------------- */
 
   fprintf(stderr,
      "\nBegin Radar Echo Classifer Product Generator cpc004/tsk007\n");

   /* the REC product generator expects two intermediate products as          */
   /* input: RECCLDIGREF is the digital reflectivity product (#298), &        */
   /* RECCLDIGDOP is the digital velocity product (#299).                     */
   /* register the input now as elevation-based data                          */
   RPGC_in_data(RECCLDIGREF,ELEVATION_DATA);
   RPGC_in_data(RECCLDIGDOP,ELEVATION_DATA);

   /* two output products are possible depending on the request.              */
   /* Reflectivity -> RECCLREF (pcode/LB# 132)       both are elevation       */
   /* Doppler -> RECCLDOP (pcode/LB# 133)            based products           */
   RPGC_out_data(RECCLREF,ELEVATION_DATA,RECCLREF);
   RPGC_out_data(RECCLDOP,ELEVATION_DATA,RECCLDOP);

   /* register algorithm infrastructure to read the Scan Summary Array        */
   RPGC_reg_scan_summary();

   /* ORPG task initialization routine. Input parameters argc/argv are        */
   /* not used in this algorithm. task timing is elevation-based.             */
   RPGC_task_init(ELEVATION_BASED,argc,argv);

   /* while loop that controls how long the task will execute. As long        */
   /* as PROCESS remains true, the task will continue. The task will          */
   /* terminate only upon a fatal error condition                             */
   while(PROCESS) {
      int datatype;        /* returned product code of intermediate data      */
      int opstatus;        /* holds return status from RPGC calls             */

      if(DEBUG)
         fprintf(stderr,"\n\n->REC PROD waiting for any data now\n");

      /* wait for any of the inputs to become available.                      */
      RPGC_wait_for_any_data(WAIT_ANY_INPUT);
      

      if(DEBUG)
         fprintf(stderr,"\n\n-> Passed wait for inbuf any -> begin processing\n");
         
      /* obtain and populate the input buffer with intermediate data          */
      inbuf = (char *)RPGC_get_inbuf_any(&datatype,&opstatus);
         
      if(DEBUG)
         fprintf(stderr,"-> Passed get inbuf any. datatype=%d opstat=%d\n",
            datatype,opstatus);
         
         
      /* ==================================================================== */
      /* Reflectivity Product Processing Section                              */
      if(datatype==RECCLDIGREF) {
         
         /* error check from get inbuf specifically for RECCLREF     */
         if(opstatus!=NORMAL) {
            RPGC_log_msg( GL_INFO,
               "RECCLPRODS ABORT1: Error obtaining input buffer. opstat=%d\n",
               opstatus);
            if(opstatus==TERMINATE)
               RPGC_abort();
            else
               RPGC_abort_datatype_because(RECCLREF, opstatus);
            continue;
            }
            
         /* check to see if there is a request for RECCLREF                   */
         if(RPGC_check_data(RECCLREF) == NORMAL) {
         
            if(DEBUG)
               fprintf(stderr,"-> Begin Processing for RECCLREF\n");
            
            /* Obtain an output buffer for RECCLREF                           */
            outbuf=(char*)RPGC_get_outbuf(RECCLREF,BUFSIZE,&opstatus);
            
            if(opstatus!=NORMAL) {
               RPGC_log_msg( GL_INFO,
                  "RECCLPRODS ABORT2: Error obtaining output buffer. opstat=%d\n",
                  opstatus);
               if(opstatus==NO_MEM)
                  RPGC_abort_datatype_because(RECCLREF, PROD_MEM_SHED);
               else
                  RPGC_abort_datatype_because(RECCLREF, opstatus);
               RPGC_rel_inbuf(inbuf);
               continue;
               }
            
            /* build the reflectivity output product now                      */
            result=generate_refl_output(inbuf,outbuf,BUFSIZE);
            
            /* release the input buffer and go to the top of the loop         */
            RPGC_rel_inbuf(inbuf);
            
            /* upon successful completion of the product release the buffer   */
            if(result==0)
               RPGC_rel_outbuf(outbuf,FORWARD);
            else {
               RPGC_log_msg( GL_INFO,"RECCLPRODS ABORT3: outbuf destroy\n");
               RPGC_rel_outbuf(outbuf,DESTROY);
               RPGC_abort_datatype_because(RECCLREF, PROD_CPU_SHED);
               }
               
            if(DEBUG)
               fprintf(stderr,"-> RECCLREF processing complete\n");
            continue;
            
         }  /* end of check_data block                                        */
      else {
         Rec_prod_header_t *rhdr=(Rec_prod_header_t*)inbuf;
         int vol=rhdr->volume_scan_num;
         int ele=rhdr->elev_num;
            
         /* release the input buffer and go to the top of the loop            */
         RPGC_rel_inbuf(inbuf);
            
         if(DEBUG)
            fprintf(stderr,"-> No Requests for RECCLREF Found: vol %d elev %d\n",
            vol,ele);
         continue;
         }
         
      }   /* end of reflectivity processing section ========================= */
      
        
      /* ==================================================================== */
      /* Doppler Product Processing Section                                   */
      else if(datatype==RECCLDIGDOP) {
         
         /* error check from get inbuf specifically for RECCLDOP           */
         if(opstatus!=NORMAL) {
            RPGC_log_msg( GL_INFO,
               "RECCLPRODS ABORT4: Error obtaining input buffer. opstat=%d\n",
               opstatus);
            if(opstatus==TERMINATE)
               RPGC_abort();
            else
               RPGC_abort_datatype_because(RECCLDOP, opstatus);
            continue;
            }
         
         /* check to see if there is a request for RECCLDOP                   */
         if(RPGC_check_data(RECCLDOP) == NORMAL) {
            
            if(DEBUG)
               fprintf(stderr,"-> Begin Processing for RECCLDOP\n");
            
            /* Obtain an output buffer for RECCLDOP                           */
            outbuf=(char*)RPGC_get_outbuf(RECCLDOP,BUFSIZE,&opstatus);
            
            if(opstatus!=NORMAL) {
               RPGC_log_msg( GL_INFO,
                  "RECCLPRODS ABORT5: Error obtaining output buffer. opstat=%d\n",
                  opstatus);
               if(opstatus==NO_MEM)
                  RPGC_abort_datatype_because(RECCLDOP, PROD_MEM_SHED);
               else
                  RPGC_abort_datatype_because(RECCLDOP, opstatus);
               RPGC_rel_inbuf(inbuf);
               continue;
               }
            
            /* build the output product now                                   */
            result=generate_dop_output(inbuf,outbuf,BUFSIZE);
            
            /* release the input buffer and go to the top of the loop         */
            RPGC_rel_inbuf(inbuf);
            
            /* upon successful completion of the product release buffer       */
            if(result==0)
               RPGC_rel_outbuf(outbuf,FORWARD);
            else {
               fprintf(stderr,"RECCLPRODS ABORT6: outbuf destroy\n");
               RPGC_rel_outbuf(outbuf,DESTROY);
               RPGC_abort_datatype_because(RECCLDOP, PROD_MEM_SHED);
               }
            
            if(DEBUG)
               fprintf(stderr,"-> RECCLDOP processing complete\n");
            continue;
            
         }  /* end of check_data block                                        */
      else {
         Rec_prod_header_t *rhdr=(Rec_prod_header_t*)inbuf;
         int vol=rhdr->volume_scan_num;
         int ele=rhdr->elev_num;
            
         /* release the input buffer and go to the top of the loop            */
         RPGC_rel_inbuf(inbuf);
            
         if(DEBUG)
            fprintf(stderr,"-> No Requests for RECCLDOP Found: vol %d elev %d\n",
            vol,ele);
         continue;
         }
         
      }   /* end of doppler processing section ============================== */
   else {
      /* handles any unexpected data types that may occur (i.e. vol restart)  */
      fprintf(stderr,"RECCLPRODS ABORT7: unexpected data type received\n");
      RPGC_rel_inbuf(inbuf);
      RPGC_abort();
      continue;
      }
   }  /* end of while process loop ========================================== */

   return 0;
   
}  /* end of main processing loop =========================================== */


