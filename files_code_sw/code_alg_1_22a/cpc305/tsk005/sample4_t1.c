/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/************************************************************************
Module:         sample4_t1.c

Description:    sample4_t1.c is part of a chain-algorithm demonstration
                algorithm for the ORPG. The overall intent of the
                algorithm is to demonstrate a two task algorithm that 
                produces multiple intermediate products and multiple 
                final products.
                
                THE PURPOSE OF THIS SAMPLE ALGORITHM TASK IS TO DEMONSTRATE
                AN ALGORITHM USING THE 'WAIT_ALL' FORM OF THE CONTROL
                LOOP AND HAVING MORE THAN ONE OUTPUT.  Constructions of a 
                simple intermediate product is accomplished.  No useful 
                science is demonstrated.
                
                This algorithm also demonstrates a task reading an elevation
                input and a radial input.  The radial base data is not 
                actually used, the task simply reads until the actual end 
                of elevation.
                
                The structure of this algorithm complies with the guidance
                provided in the CODE Guide Vol 3.
                
                sample4_t1.c determines which (of two) elevation based 
                intermediate products are requested creates the products
                requested.  These products contain a base data header and
                a short text message.

                Key source files for this algorithm include:

                sample4_t1.c         program source
                sample4_t1.h         main include file
                
                
                
Authors:        Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org

Version 1.2   March 2006   T. Ganger
              Revised to use current guidance for abort functions and 
                   abort reason codes.
              Modified to demonstrate output buffer reallocation and to
                   produce a small intermediate product message rather  
                   than an empty intermediate product message.
                   
Version 1.3   June 2006   T. Ganger
              Revised to use RPGC_reg_io
              Revised to use the new 'by_name' get_inbuf/get_outbuf 
                   functions 
                   
Version 1.4   August 2006   T. Ganger
              Modified to use the radial header offset to surveillance
                  data 'ref_offset'. Tested RPGC_get_surv_data.
              Replaced WAIT_ALL with WAIT_DRIVING_INPUT
              Deleted first argument from realloc_outbuf
              
Version 1.5   March 2007  T. Ganger
              Demonstrated the prod_attr_table method of declaring input 
                  optional.
              Demonstrated RPGC_get_radar_data and RPGCS_radar_data_conversion
                  used with basic moment R. RPGC_get_radar_data is not reliable
                  in Build 9, wait for Build 10.
              Demonstrated combining radial and elevation inputs.
              
Version 1.6   June 2007  T. Ganger
              Modified for change in parameter type for RPGC_get_radar_data.
              
Version 1.8   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              
Version 2.0   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Modified to provide a correct example of an algorithm having more 
                  than out output with a WAIT_ALL loop having a driving input.  
                  The algorithm may produce only one or all of the products 
                  depending upon the requests for the outputs. Previously this 
                  task had an undocumented assumption that both output 
                  intermediate products were always requested (scheduled). Uses 
                  RPGC_check_data_by_name() before acquiring the output buffer 
                  and creating the product.  Uses RPGC_abort_dataname_because() 
                  intead of RPGC_abort() where appropriate.  In subsequent 
                  processing, including release of output buffers, tested each 
                  output buffer for NULL before execution. 

Version 2.1   March 2009    T. Ganger  (Sample Algorithm ver 1.21)
              Minor admin changes to improve clarity.
                
$Id$
************************************************************************/

#include "sample4_t1.h"



/************************************************************************/
int main(int argc,char* argv[]) {

   Base_data_radial
       *basedataPtr=NULL;    /* pointer to a base data radial structure  */
   Base_data_header 
       *bdh=NULL;            /* base data header struct                  */

   char *inbuf=NULL;         /* pointer to optional input buffer         */
   
   char *buffer1=NULL;       /* pointer: access to allocated memory      */
   char *buffer2=NULL;       /* pointer: access to allocated memory      */
                             /* which holds a completed ICD product      */

   char *new_buffer1=NULL;   /* pointer to reallocated output product    */
   char *new_buffer2=NULL;   /* memory buffer                            */

   int bufsize;              /* size of product buffer                   */
        
   int ref_enable;           /* variables: used to hold the status of    */
   int vel_enable;           /* the individual moments of base data.     */
   int spw_enable;           /* tested after reading the first radial    */

   int PROCESS=TRUE;         /* Boolean used to control the daemon       */
                             /* portion of the program. Process until set*/
                             /* to FALSE.                                */
   int opstatus, status_opt; /* variables: hold function return values   */
   short radial_status;      /* variable: status of each radial input    */
   
   int opt_avail;            /* hold status of reading optional input    */

   /* data to be included in the product */
   char text1[] = "SAMPLE ALGORITHM 4 Product 1 (Intermediate)";
   char text2[] = "SAMPLE ALGORITHM 4 Product 2 (Intermediate)";



/* following support demonstration of multiple methods of reading base data */
   /*short *surv_data=NULL;  */ /* pointer to the surveillance data in the radial*/
   /*int index_first_bin=999;*/  /* used with method 2 */
   /*int index_last_bin=999; */  /* used with method 2 */
   int rad_idx;
   short *surv_data3=NULL; /* pointer to the surveillance data in the radial */


Generic_moment_t gen_moment;  /* used with method 3 */
/* cannot pass a NULL pointer to RPGC_get_radar_data */
Generic_moment_t *my_gen_moment = &gen_moment; /* method 3 */

float *surv_values=NULL;
int decode_res;
   
   int VERBOSE=FALSE;
/*   int VERBOSE=TRUE; */
   
   
   /* ------------------------------------------------------------------ */

   fprintf(stderr,"\nBegin Sample 4 Task 1 Algorithm\n");

   /* register inputs and outputs based upon the                      */
   /* contents of the task_attr_table (TAT).                          */
   /* input:REFLDATA(79), output:SAMPLE4_IP1(1998)  SAMPLE4_IP2(1997) */
   /* we also have a test optional input SAMPLE3_IP(1999)             */
   
    RPGC_reg_io(argc,argv);


    /* ------------------------------------------------- */
    /* the original method for designating an optional input is    */
    /* using the following function. Now we use the opt_prods_list */
    /* attribute in the product attribute table to designate an    */
    /* the optional input.          This function can be used to   */
    /* modify the default 5 second delay to an 8 second delay.     */
    RPGC_in_opt_by_name("SAMPLE3_IP", 8);

  
   /* Register algorithm infrastructure to read the Scan Summary Array   */
   RPGC_reg_scan_summary();

   /* ORPG task initialization routine. Input parameters argc/argv are   */
   /* not used in this algorithm                                         */
   RPGC_task_init(ELEVATION_BASED, argc, argv);

   /* output message to both the task output file and the task log file */   
   fprintf(stderr,
      "-> algorithm initialized and proceeding to loop control\n");
   RPGC_log_msg( GL_INFO,
      "-> algorithm initialized and proceeding to loop control\n");


   /* while loop that controls how long the task will execute. As long as*/
   /* PROCESS remains TRUE, the task will continue. The task will        */
   /* terminate upon an error condition                                  */
   while(PROCESS) {

      /* system call to indicate a data driven algorithm. block algorithm*/
      /* until good data is received at & least one product is requested */
      RPGC_wait_act(WAIT_DRIVING_INPUT);

      if (VERBOSE) fprintf(stderr,
          "-> sample4_t1 algorithm passed ACL and began processing\n");
      RPGC_log_msg( GL_INFO,
          "-> sample4_t1 algorithm passed ACL and began processing\n");


      /* initial size of output product buffer                           */
      bufsize = (int) sizeof(Base_data_header);

      /* Sample Alg version 1.20 */
      /* Check to see if there is a request for each output datatype. */
      /* If so, allocate memory (accessed by the pointer, buffer) in  */
      /* which the output is constructed. error return in opstatus    */
      buffer1 = NULL;
      buffer2 = NULL;
      
      if( RPGC_check_data_by_name( "SAMPLE4_IP1" ) == NORMAL ) {
          buffer1 = (char *)RPGC_get_outbuf_by_name("SAMPLE4_IP1", bufsize, 
                                                                &opstatus);
          /* check error condition from buffer allocation. abort if abnormal */
          if(opstatus != NORMAL) {
             RPGC_log_msg( GL_INFO,
                "ERROR: Aborting from RPGC_get_outbuf...opstatus=%d\n", opstatus); 
             RPGC_abort_dataname_because( "SAMPLE4_IP1", opstatus );
             /* note: buffer1 is NULL in this case */
          }
      } 

      if( RPGC_check_data_by_name( "SAMPLE4_IP2" ) == NORMAL ) {

          buffer2 = (char *)RPGC_get_outbuf_by_name("SAMPLE4_IP2", bufsize, 
                                                                  &opstatus);
          /* check error condition from buffer allocation. abort if abnormal */
          if(opstatus != NORMAL) {
             RPGC_log_msg( GL_INFO,
                "ERROR: Aborting from RPGC_get_outbuf...opstatus=%d\n", opstatus);
             RPGC_rel_outbuf((void *)buffer1, DESTROY); 
             RPGC_abort_dataname_because( "SAMPLE4_IP2", opstatus );
             /* note: buffer2 is NULL in this case */
          }
      }

      /* Sample Alg version 1.20 */
      /* all needed output buffers can not be obtained, already aborted above */
      if( (buffer1 == NULL)  &&  (buffer2 == NULL) )
          continue;
            
      /* LABEL:READ_FIRST_RAD Read first radial of the elevation and     */
      /* accomplish the required moments check.                          */
      /* ingest one radial of data from the BASEDATA linear buffer. The  */
      /* data will be accessed via the basedataPtr pointer               */

      basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("REFLDATA", 
                                                                &opstatus);
      bdh = (Base_data_header *) basedataPtr;
    
      /* check radial ingest status before continuing                    */
      if(opstatus != NORMAL){
         RPGC_log_msg( GL_INFO,"ERROR: Aborting from RPGC_get_inbuf\n");
         /* Sample Alg version 1.20 */
         if(buffer1 != NULL) RPGC_rel_outbuf((void*)buffer1, DESTROY);
         if(buffer2 != NULL) RPGC_rel_outbuf((void*)buffer2, DESTROY);
         RPGC_abort(); /* abort all requested prods */
         continue;
      }
  
      /* test to see if the required moment (reflectivity) is enabled    */
      RPGC_what_moments(bdh,&ref_enable,
                        &vel_enable,&spw_enable);

      if(ref_enable != TRUE){
         RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_what_moments\n");
         RPGC_rel_inbuf((void*)basedataPtr);
         /* Sample Alg version 1.20 */
         if(buffer1 != NULL) RPGC_rel_outbuf((void*)buffer1, DESTROY);
         if(buffer2 != NULL) RPGC_rel_outbuf((void*)buffer2, DESTROY);
         RPGC_abort_because(PGM_DISABLED_MOMENT); /* abort all requested prods */
         continue;
      }

      /* NORMALLY AN ELEVATION INPUT CAN BE READ IMMEDIATELY AFTER READING */
      /* THE FIRST RADIAL INPUT.  HOWEVER, SINCE THIS ELEVATION INPUT      */
      /* IS DESIGNATED OPTIONAL, IT MUST BE READ AFTER ALL RADIAL INPUTS   */
      /* ARE READ                                                          */
      
      opt_avail = FALSE;  /* flag to signal later processing of input, */
                          /* if available                              */
       
      rad_idx=0;
      
      while(TRUE) {
        
      /* RADIAL PROCESSING SEGMENT. continue to ingest and process       */
      /* individual base radials until either a failure to read valid    */
      /* input data (a radial in correct sequence) or until after reading*/
      /* and processing the last radial in the elevation.                */
         
         /* read the radial status flag to determine end of elevation    */
         radial_status=basedataPtr->hdr.status & 0xf;


         /* Radial processing here, This algorithm simply copies the     */
         /* base data header into the product buffer.                    */

         /* While all of the following work, Method 2 is typical for basic */
         /* moments, Method 3 must be used for advanced Dual Pol moments   */
         /* Method 1 */
         /* surv_data = (short *) ((char *) basedataPtr + bdh->ref_offset); */
                  
         /* Method 2 */
         /* surv_data = (short *) RPGC_get_surv_data( (void *)basedataPtr, */
         /*                          &index_first_bin, &index_last_bin);   */

         /* Method 3 */
         surv_data3 = (short *) RPGC_get_radar_data( (void *)basedataPtr,
                                  RPGC_DREF, my_gen_moment );


         /* Sample Alg 1.18 - test for NULL moment pointer and 0 offset */
         
         if(surv_data3 == NULL) {
            RPGC_log_msg( GL_INFO, "ERROR: NULL radial detected: %d\n",
                                                                    rad_idx);
            RPGC_rel_inbuf((void*)basedataPtr);
            /* Sample Alg version 1.20 */
            if(buffer1 != NULL) RPGC_rel_outbuf((void*)buffer1, DESTROY);
            if(buffer2 != NULL) RPGC_rel_outbuf((void*)buffer2, DESTROY);
            RPGC_abort_because(PGM_INPUT_DATA_ERROR); /* abort all requested prods */
            opstatus = TERMINATE;
            break;
         }
         
         if(bdh->ref_offset == 0) {
            RPGC_log_msg( GL_INFO,"ERROR: reflectivity offset 0: %d\n",
               rad_idx);
            RPGC_rel_inbuf((void*)basedataPtr);
            /* Sample Alg version 1.20 */
            if(buffer1 != NULL) RPGC_rel_outbuf((void*)buffer1, DESTROY);
            if(buffer2 != NULL) RPGC_rel_outbuf((void*)buffer2, DESTROY);
            RPGC_abort_because(PGM_INPUT_DATA_ERROR); /* abort all requested prods */
            opstatus = TERMINATE;
            break;
         }
   
   
         /* though not used, we demonstrate decoding data into dBZ values */
         decode_res = RPGCS_radar_data_conversion( (void *)surv_data3, 
                                          my_gen_moment, -999.0, -888.0, 
                                                            &surv_values);
                                          
         if(decode_res == -1) { /* decode error, abort processing ************** */
           
               /* if the products actually used the data, would abort here */
               
         } else { /* PROCESS RADIAL DATA AND COPY ****************************** */
           
            /* if radail data were actually used, the radial data would be */
            /* copied and processed as required                            */
            /* this algorithm just includes debug print outs               */
   
            /* BEGIN DEBUG ----------------------------------------------------- */
            /* if( (rad_idx==0) || (rad_idx==100) || (rad_idx==360) ) {          */
            /* fprintf(stderr,                                                   */
            /*   "DEBUG RADIAL %d: range to first bin is %d,  num bins is %d\n", */
            /*                        rad_idx, bdh->surv_range - 1,              */
            /*                        bdh->n_surv_bins );                        */
            /* fprintf(stderr,                                                   */
            /*   "DEBUG funct parameters    first bin is %d,  last bin is %d\n", */
            /*                        index_first_bin, index_last_bin);          */
            /* }                                                                 */
            /*                                                                   */
            /* if( ((rad_idx<=2) || (rad_idx==100)) && (surv_data!=NULL) ) {     */
            /* fprintf(stderr,                                                   */
            /*              "Radial %3d: 1   2   3   4   5   6   7   8   9  10"  */
            /*              "  11  12  13  14  15  16  17  18  19  20\n", rad_idx);*/
            /* fprintf(stderr,                                                   */
            /*              "          %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d"  */
            /*              " %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d \n",       */
            /*    surv_data[0], surv_data[1], surv_data[2], surv_data[3],        */
            /*    surv_data[4], surv_data[5], surv_data[6], surv_data[7],        */
            /*    surv_data[8], surv_data[9], surv_data[10], surv_data[11],      */
            /*    surv_data[12], surv_data[13], surv_data[14], surv_data[15],    */
            /*    surv_data[16], surv_data[17], surv_data[18], surv_data[19]);   */
            /* }                                                                 */
            /*                                                                   */
            /*                                                                   */
            /* if( (rad_idx==0) || (rad_idx==100) || (rad_idx==360) ) {          */
            /* fprintf(stderr,                                                   */
            /*     "DEBUG first gate range is %d, number of gates is %d\n",      */
            /*      my_gen_moment->first_gate_range, my_gen_moment->no_of_gates);*/
            /* }                                                                 */
            /*                                                                   */
            /* if( ((rad_idx<=2) || (rad_idx==100)) && (surv_data3!=NULL) ) {    */
            /* fprintf(stderr,                                                   */
            /*              "Radial %3d: 1   2   3   4   5   6   7   8   9  10"  */
            /*              "  11  12  13  14  15  16  17  18  19  20\n", rad_idx);*/
            /* fprintf(stderr,                                                   */
            /*              "          %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d"  */
            /*              " %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d \n",       */
            /*   surv_data3[0], surv_data3[1], surv_data3[2], surv_data3[3],     */
            /*   surv_data3[4], surv_data3[5], surv_data3[6], surv_data3[7],     */
            /*   surv_data3[8], surv_data3[9], surv_data3[10], surv_data3[11],   */
            /*   surv_data3[12], surv_data3[13], surv_data3[14], surv_data3[15], */
            /*   surv_data3[16], surv_data3[17], surv_data3[18], surv_data3[19]);*/
            /* }                                                                 */
            /*                                                                   */
            /* if( ((rad_idx<=2) || (rad_idx==100)) && (decode_res!=-1) ) {      */
            /* fprintf(stderr,"Decoded Values:\n");                              */
            /* fprintf(stderr,                                                   */
            /*  "  %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f\n"*/
            /*  "  %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f %3.2f\n",*/
            /*   surv_values[0], surv_values[1], surv_values[2], surv_values[3], */
            /*   surv_values[4], surv_values[5], surv_values[6], surv_values[7], */
            /*   surv_values[8], surv_values[9], surv_values[10], surv_values[11],*/
            /*   surv_values[12],surv_values[13],surv_values[14],surv_values[15],*/
            /*   surv_values[16],surv_values[17],surv_values[18],surv_values[19]);*/
            /* }                                                                 */
            /* END DEBUG ------------------------------------------------------- */
   
            if( surv_values != NULL) {
               free(surv_values);
               surv_values = NULL;
            }
            
         } /* end PROCESS RADIAL DATA AND COPY ********************************* */
            
            
            
         /* if end of elevation or volume process radial then exit loop. */
         /* this is the ACTUAL end of elevation rather than Pseudo end   */
         if(radial_status == GENDEL || radial_status == GENDVOL) {

            /* Sample Alg version 1.20 - only process requested product*/
            if(buffer1 != NULL) 
                memcpy((void*)buffer1,(void*)basedataPtr,sizeof(Base_data_header));
            if(buffer2 != NULL) 
                memcpy((void*)buffer2,(void*)basedataPtr,sizeof(Base_data_header));

            /* now that the data that we want from the radial is in      */
            /* the product...release the input buffer                    */
            RPGC_rel_inbuf((void*)basedataPtr);



           /* DEMONSTRATION begin read optional elevation input *********** */
           /* AFTER LAST RADIAL INPUT DEMO READING ELEVATION INPUT          */
           
           /* WITH NO TIME DELAY, THIS OPTIONAL PRODUCT MAY NOT BE READ.    */
           /* needed to add a sleep delay if using the product_attr_table   */
           /* method no delay is used and this elevation product is never   */
           /* actually read                                                 */

           inbuf = (char *)RPGC_get_inbuf_by_name("SAMPLE3_IP", &status_opt);
  
           if(status_opt != NORMAL){
              /* optional input not available */
              if (VERBOSE)
                 fprintf(stderr, 
                     "-> sample4_t1 optional elev input not available (%d)\n", 
                                                                     status_opt);
              /* remaining processing should not use optional input */
              opt_avail = FALSE;
           
           } else {
             if (VERBOSE)
                 fprintf(stderr, 
                     "-> sample4_t1 optional elev input successfully read\n");
              opt_avail = TRUE;
           }
           /* DEMONSTRATION end read optional elevation product************ */


            /* exit inner loop with opstatus==NORMAL and no ABORT */
            break;


         } else { /* not last radial, continue in the inner loop */
            /* this demonstration does nothing until the last radial     */
            /* release the input buffer                                  */
            RPGC_rel_inbuf((void*)basedataPtr);
         
         }
     
         /* LABEL:READ_NEXT_RAD Read the next radial of the elevation.   */
         /* ingest one radial of data from the BASEDATA linear buffer.   */
         /* The data will be accessed via the basedataPtr pointer        */

         basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("REFLDATA", 
                                                                  &opstatus);
         bdh = (Base_data_header *) basedataPtr;

         /* check radial ingest status before continuing                 */
         if(opstatus != NORMAL) {
            RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_get_inbuf\n");
            /* Sample Alg version 1.20 */
            if(buffer1 != NULL) RPGC_rel_outbuf((void*)buffer1, DESTROY);
            if(buffer2 != NULL) RPGC_rel_outbuf((void*)buffer2, DESTROY);
            RPGC_abort();   /* aborted all requested products */
            break;  /* break out of inner loop */
         }
         rad_idx++;
         
      } /* end while TRUE, process elevation */
      

      if(VERBOSE) {
          if(opstatus == NORMAL)
               fprintf(stderr, "DEBUG out of read loop, opstatus NORMAL.\n");
          else
              fprintf(stderr, "DEBUG out of read loop, opstatus %d.\n", opstatus); 
      }

      /* LABEL:ASSEMBLE The assembly of the intermediate product         */
      if(opstatus == NORMAL) {

         /* Elevation processing here. */
         
         /* could use optional input here, released after use  */
         if(opt_avail == TRUE) {
            /* simulate use of optional input if available */
            
            RPGC_rel_inbuf((void*)inbuf);
         }
        
        
         /* The reallocation of the output buffer size is demonstrated.  */
         
         bufsize = bufsize + 50;

         /* Sample Alg version 1.20 only process if requested   */
         /* PROCESS PRODUCT 1 ********************************* */
         if(buffer1 != NULL) {
            
             new_buffer1 = RPGC_realloc_outbuf( (void*)buffer1, 
                                                          bufsize, &opstatus );  
             
             if(opstatus != NORMAL) { /* abort product 1 */
                RPGC_log_msg( GL_INFO,
                   "ERROR: Aborting from RPGC_realloc_outbuf...opstatus=%d\n",
                   opstatus);
                RPGC_rel_outbuf((void *)buffer1, DESTROY);
                /* Sample Alg version 1.20 */
                RPGC_abort_dataname_because( "SAMPLE4_IP1", opstatus );
             
             } else { /* output product 1 */
             
             memcpy( (void*)new_buffer1+sizeof(Base_data_header), 
                   (void*)text1, strlen(text1)+1);
                   
                /* LABEL:OUTPUT_PROD    forward product and close buffer  */
                RPGC_rel_outbuf((void*)new_buffer1, FORWARD);
                
            } /* end output product 1 */

         } /* END PROCESS PRODUCT 1 *************************** */
         

         /* Sample Alg version 1.20 only process if requested   */
         /* PROCESS PRODUCT 2 ********************************* */
         if(buffer2 != NULL) {
            
             new_buffer2 = RPGC_realloc_outbuf( (void*)buffer2, 
                                                          bufsize, &opstatus );
    
             if(opstatus != NORMAL) { /* abort product 2 */ 
                RPGC_log_msg( GL_INFO,
                   "ERROR: Aborting from RPGC_realloc_outbuf...opstatus=%d\n",
                   opstatus);
                RPGC_rel_outbuf((void *)buffer2, DESTROY);
                /* Sample Alg version 1.20 */
                RPGC_abort_dataname_because( "SAMPLE4_IP2", opstatus );
             
             } else { /* output product 2 */
     
                memcpy( (void*)new_buffer2+sizeof(Base_data_header), 
                   (void*)text2, strlen(text2)+1);
                   
                /* LABEL:OUTPUT_PROD    forward product and close buffer  */
                RPGC_rel_outbuf((void*)new_buffer2, FORWARD);
                
             } /* end output product 2 */

         } /* END PROCESS PRODUCT 2 *************************** */
 

      }  /* end of if(opstatus==NORMAL) block                          */
      
      
      if(opt_avail == TRUE) {
         RPGC_rel_inbuf((void*)inbuf);
      }
      
      

   } /* end while PROCESS == TRUE                                      */

   return 0;

} /* end of main ----------------------------------------------------- */

