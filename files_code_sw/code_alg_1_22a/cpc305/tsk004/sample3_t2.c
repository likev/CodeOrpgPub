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
Module:         sample3_t2.c

Description:    sample3_t2.c is part of a chain-algorithm demonstration.
                The first task reads radial data and produces elevation
                data.  The second task reads elevation data and produces
                volume data.
                
                THE PURPOSE OF THIS SAMPLE ALGORITHM IS TO DEMONSTRATE
                A MULTIPLE TASK ALGORITHM.  NO USEFUL SCIENCE IS 
                ACCOMPLISHED BY THE SECOND TASK
                
                The structure of this algorithm complies with the guidance
                provided in the CODE Guide Vol 3, with one exception.  
                Though the algorithm has been tested for memory leaks, 
                logic to handle a failure of "malloc" in this file is 
                incomplete.  Even when the failure is tested for, the 
                cleanup process is not adjusted correctly.

               
                sample3_t2.c, the second part of the chain algorithm, 
                accepts the intermediate elevation data and allocates 
                memory to contain each elevation. When the volume is 
                complete or the elevation specified via an adaptable
                parameter is received, the task generates a 460 KM
                digital reflectivity CD-compliant final product.  The
                adaptable parameter can be changed from the HCI.  
                
                Even though multiple elevations of data are stored, no
                volume processing is actually performed.  The algorithm
                simply selects an elevation (based upon the adaptation
                data) to convert to a final product.
                
                Previously this task was artificially limited to process
                no more than the first 5 elevations of a volume coverage
                pattern (VCP). This artificial limitation has been 
                eliminated.  The task terminates processing at the last
                elevation in the VCP or the elevation stated in the 
                adaptation data, which ever occurs first.
                  
                  
                Key source files for this algorithm include:

                sample3_t2.c           program source
                sample3_t2.h           main include file
                
                s3_t1_prod_struct.h    definition of s3_t1_intermed_prod_hdr

                s3_symb_layer.c        creates the ICD compliant product
                s3_symb_layer.h        local include file
                
                s3_print_diagnostics.c used for diagnostic output
                s3_print_diagnostics.h local include file
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.4,  April 2005   T. Ganger
              Modified to use the new adaptation data (DEA) using
                   sample3_t2.alg as source of data

Version 1.5   March 2006   T. Ganger
              Replaced stderr messages outside of test/debug sections with
                   RPGC_log_msg() calls.
              Revised to use current guidance for abort functions and 
                   abort reason codes.                

Version 1.6   June 2006   T. Ganger
              Revised to use RPGC_reg_io
              Revised to use the new 'by_name' get_inbuf/get_outbuf functions 

Version 1.7   March 2007   T. Ganger
              BUGFIX. Task crashed with abnormal inner loop exit (get_inbuf 
                  or other failure) because excessive number of elevation 
                  data was freed.
              
Version 1.8   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated use of defined output buffer ids by using
                  RPGC_get_id_from_name
              Noted setting number of bins per radial at 230 is artificial
                  and dependent upon using recombined data.
              Corrected the contents of dependent parameter 4 from maximum 
                  reflectivity data level to maximum value in dBZ
              Provided the number of bins in each array to the subroutine
                  responsible to build the symbology block. Will permit
                  dynamically changing the array size in the future.
              Used the new default abort reason code PGM_PROD_NOT_GENERATED.
              
Version 2.0   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Updated description of algorithm task and main function.  
                  Added comments to clarify processing.
              Used RPGC_is_buffer_from_last_elev to determine when the
                  last elevation produced by the first task is received 
                  rather than the Legacy method of using the flag in the 
                  elevation input product.
              Eliminated artificial limit on number of elevations to read.
              Used a new structure to read the input intermediate product.
              
$Id$
************************************************************************/

/* see the following include file for descriptions of constants and
for expected ICD block sizes */
#include "sample3_t2.h"


int TEST=FALSE;           /* Boolean to control diagnostic output       */
                          /* set to TRUE to increase number of messages */
 int VERBOSE=FALSE; 
/* int VERBOSE=TRUE; */

/* file scope variables for adaptation data value */
  int input_elev;
                         

/* prototype for adaptation data access function for DEA                  */
/*     this function cannot be registered as a callback because a         */
/*     parameter providing the address of the c structure is not provided */
int read_sample3_t2_adapt();


/************************************************************************
Description:    main function to drive the chain-algorithm demonstration
                (second task out of two).
Input:          receive an elevation intermediate product via a structure 
                in s3_t1_prod_struct.h with radial data in an array of 
                bytes (unsigned char) produced by sample3_t1.
Output:         a digital reflectivity final product associated with the
                elevation index adaptable parameter
Returns:        none
Globals:        none
Notes:          1). This sample algorithm task reads and stores multiple 
                elevations of an intermediate product.  No volume 
                processing is performed other than to select an elevation 
                to convert to a final product.
                2). This algorithm can set different radial sizes not to
                exceed 460 bins.  Since the maximum number of radials is
                hard coded to 400 (and a static array used), this
                algorithm cannot use any of the super resolution data 
                types.  Hence 460 is the maximum number of bins.
                3). The program takes no command line arguments.
                    

************************************************************************/
int main(int argc,char* argv[]) {
   /* algorithm variable declarations and definitions ------------------  */
   int PROCESS=TRUE;         /* Boolean used to control the processing    */
                             /* portion of the program. Process until set */
                             /* to FALSE.                                 */
   int i;                    /* loop variables                            */
   int loop_exit_stat=0;    /* variable: holds exit status from loop      */
   int opstatus;            /* variable:  used to hold API return values  */

   char *buffer;            /* pointer to contain a complete ICD product  */
   char *inbuf;             /* pointer to intermediate input buffer       */
   /* Sample Alg 1.20 - changed CVG_radial* to unsigned char* and         */
   /*                   rad_data[] to input_prod_data[]                   */
   unsigned char *input_prod_data[MAX_ELEVS+1]; /* container to hold      */
                            /* intermediate products for processing       */

   int result=0;            /* variable: holds function results           */
   /* Sample Alg 1.20 */
   int last_elev=FALSE;     /* Boolean used to hold the status of function*/
                            /* RPGC_is_buffer_from_last_elev              */
   int elev_ind;            /* output of RPGC_is_buffer_from_last_elev    */
   int last_index;          /* out out of RPGC_is_buffer_from_last_elev    */

   int elev_count=1;        /* elevation counter through the loop         */
   
   /* ------------------------------------------------------------------- */

   fprintf(stderr,"\nBegin Sample 3 Task 2 Algorithm\n");

   /* LABEL:REG_INIT  Algorithm Registration and Initialization Section   */

   /* register inputs and outputs based upon the            */
   /* contents of the task_attr_table (TAT).                */
   /* input:SAMPLE3_IP(1999), output:SAMPLE3_FP(1992)       */
   RPGC_reg_io(argc, argv);


   /* register algorithm infrastructure to read the Scan Summary Array    */   
   RPGC_reg_scan_summary();
   

  /* the adaptation data read function cannot be registered as a 
   * callback function because it does not have the needed parameters
   */


   /* ORPG task initialization routine. Input parameters argc/argv are    */
   /* not used in this algorithm                                          */
   RPGC_task_init(VOLUME_BASED, argc, argv);
   
   fprintf(stderr, "-> sample3_t2 algorithm initialized\n");

   RPGC_log_msg( GL_INFO,
      "-> sample3_t2 algorithm initialized\n");

   /* while loop that controls how long the task will execute. As long as */
   /* PROCESS remains TRUE, the task will continue.                       */
   while(PROCESS) {
      /* Boolean used to control diagnostic output */
      int test_out=FALSE;
      
      /* system call to indicate a data driven algorithm. block algorithm */
      /* until good data is received the product is requested             */
      RPGC_wait_act(WAIT_ALL);
      
      /* LABEL:BEGIN_PROCESSING Released from Algorithm Flow Control Loop */
      if (VERBOSE) 
           fprintf(stderr,
                "-> sample3_t2 passed wait act loop control. count=%d\n",
                                                               elev_count);
      RPGC_log_msg( GL_INFO, 
           "-> sample3_t2 passed wait act loop control. count=%d\n",
                                                               elev_count);

      /* open output buffer                                               */ 
      buffer = (char*)RPGC_get_outbuf_by_name("SAMPLE3_FP", BUFSIZE, &opstatus);
      
      if(opstatus != NORMAL) {
         RPGC_log_msg( GL_INFO,
            "ERROR: Sample 3 Task 2 Aborting, RPGC_get_outbuf..opstatus=%d\n",
                                                                    opstatus);
         RPGC_abort();
         continue;
      }         

      if (VERBOSE) {
        fprintf(stderr,"-> sample3_t2 successfully obtained output buffer\n");
      }

      clear_buffer(buffer);
      
    /* reading new DEA adaptation data elements manually because   */
    /* the access function does not include the required parameter */
    /* in order to be registered as a callback function            */
    read_sample3_t2_adapt();

    if (VERBOSE) {    
      fprintf(stderr, "-> sample3_t2 - adaptation elev_index selection = %d\n",
                                                                    input_elev);
    }

      /* Set Exit Status to NORMAL */
      loop_exit_stat = 0;   
      
      /* PROCESS ELEVATION LOOP                                   */   
      while(TRUE) {

         /* Sample Alg 1.20 - replaced CVG_radial with s3_t1_intermed_prod_hdr */
         s3_t1_intermed_prod_hdr *ptr=NULL;  /* pointer to input product header */
         int length=0;          /* holds calculated length of product     */
         
         /* LABEL:PROCESS_ELEV Here we process each elevation individually.*/ 
         /* obtain an input buffer from intermediate buffer */
         if (VERBOSE) {
           fprintf(stderr, "Sample 3 Task 2 - Waiting for Inbuf - Elev=%d\n",
                                                                  elev_count);
         }

         inbuf = (char *)RPGC_get_inbuf_by_name("SAMPLE3_IP", &opstatus);
         
         if (VERBOSE) {
           fprintf(stderr, "-> sample3_t2 get inbuf opstatus = %d \n", opstatus);
         }

         if(opstatus != NORMAL){
            loop_exit_stat=opstatus;
            RPGC_log_msg( GL_INFO,"ERROR: Aborting from RPGC_get_inbuf\n");
            RPGC_rel_outbuf((void*)buffer, DESTROY); 
            RPGC_abort();
            break;
         }
         
         if (VERBOSE) {
           fprintf(stderr,
                     "-> sample3_t2 successfully read intermediate product\n");
         }

          /* NOTE: This algorithm originally always used 230 as the number of  */
          /*           bins per radial.  This task can now read intermediate   */
          /*           products having different sizes.                        */
          /*        This provides flexibility in independently modifying the   */
          /*           first task of this sample algorithm                     */

         /* Sample Alg 1.20 - replaced CVG_radial with s3_t1_intermed_prod_hdr */
         ptr = (s3_t1_intermed_prod_hdr*)inbuf;

         /* allocate memory to hold current intermediate product          */
         /* Sample Alg 1.20 - changed 6 to 5 */
         length = (5 * sizeof(int)) + (2 * ptr->num_radials * sizeof(int)) + 
                  (ptr->num_range_bins * ptr->num_radials * sizeof(char))
                                                   + sizeof(Base_data_header);  
         if (VERBOSE) {
           fprintf(stderr, "-> sample3_t2 allocation size = %d\n", length);  
           }
         
         /* allocate memory for incoming intermediate product             */
         /* Sample Alg 1.20 - replaced CVG_radial* with unsigned char* */
         input_prod_data[elev_count] = (unsigned char*)malloc((size_t)length);
         
         if(input_prod_data[elev_count] == NULL) {
            RPGC_log_msg( GL_ERROR,
                  "MALLOC Error: malloc failed on input_prod_data[%d]\n", 
                         elev_count);
            RPGC_rel_inbuf((void*)inbuf);
            RPGC_rel_outbuf((void*)buffer, DESTROY);  
            RPGC_abort_because(PGM_MEM_LOADSHED);
            loop_exit_stat = TERMINATE;   /* SHOULD THIS BE NO_DATA?  */
            break;
         }
         
         if (VERBOSE) {
           fprintf(stderr, 
                   "-> sample3_t2 memory allocated for input_prod_data[%d]\n",
                        elev_count);
         }
         
         /* copy input buffer to allocated memory */
         memcpy(input_prod_data[elev_count], inbuf, length);
         if (VERBOSE) {
           fprintf(stderr, "-> sample3_t2 data copied to temporary storage\n");
         }
      

         /* get pointers to the data set                                  */
         /* Sample Alg 1.20 - replaced CVG_radial with s3_t1_intermed_prod_hdr  */
         ptr = (s3_t1_intermed_prod_hdr*)input_prod_data[elev_count];
         
         /* show integrity of the data set (optional)                     */
         if(TEST) {
            fprintf(stderr, "-> sample3_t2: test output num_range_bins=   %d\n",
                                                            ptr->num_range_bins);
            fprintf(stderr, "-> sample3_t2: test output num_radials=      %d\n",
                                                               ptr->num_radials);
            fprintf(stderr, "-> sample3_t2: test output num_bytes_per_bin=%d\n",
                                                         ptr->num_bytes_per_bin);
         }

         /* diagnostic output of the data to disk (optional)  */
         if(test_out == TRUE) {  
            char *c_ptr;
            c_ptr = (char *)input_prod_data[elev_count];
            product_to_disk(c_ptr, length, "sample3_fp", elev_count);
         } 

         /* LABEL: LAST_ELEV Two tests to determine if we are done            */
         
         /* For Test 1. Is this the last elevation produced by the first task?*/
         /* Originally we used a flag sent in the intermediate product. Now   */
         /* Now use use the recommended API function to determine the last    */
         /* elevation received by the RPG. If the last elevation, then break  */
         /* out of the inner loop with a NORMAL exit loop status              */

         last_elev = RPGC_is_buffer_from_last_elev( (void*)inbuf, 
                                                        &elev_ind, &last_index ); 
         
         /* now that we've read everything in that we want to  */
         RPGC_rel_inbuf((void*)inbuf);
         if (VERBOSE)
           fprintf(stderr, "-> sample3_t2 released input buffer. count=%d\n",
                                                                  elev_count);

         /* Test 1. no more data to receive */
         if( (elev_ind == last_index) || (last_elev == -1) ) { 
            /* either the last elev or function failed */
            if(TEST) fprintf(stderr, "-> sample3_t2 RECEIVED LAST ELEVn");
            break;
         }

         /* Test 2. Is this the elevation selected by adaptation data to */
         /* be used to construct the product? This produces the product  */
         /* as soon as possible.                                         */
         if(elev_count == input_elev) {
            if(TEST) fprintf(stderr, "-> sample3_t2 SELECTED ELEVATION\n");
            break;  
         }
         
         /* increment elev count */
         elev_count++;
         
      } /* -------- end of process elevation loop  ----------- */

      if (VERBOSE) {
        fprintf(stderr,
               "-> sample3_t2 out of Elevation Process loop. Elev cnt=%d\n",
               elev_count);
      }

      /* LABEL: OUTPUT_PROD  */

      if(loop_exit_stat == NORMAL) {  /* successfully received input data */
               
         /* create output product-dependent on the adaptation elevation index*/
         if(input_elev > 0 && input_elev <= last_index) {     
            /* convert product to the s3_t2_internal_rad struct & create output*/
            result = translate_product(input_prod_data[input_elev], buffer);
            
            if (VERBOSE) {
              fprintf(stderr,
                   "-> sample3_t2 returned from translate product. result=%d\n",
                                                                         result);
            }
              
         } else  { /* adaptation data out of range, use first elevation */
            if (VERBOSE) {
              fprintf(stderr,
                     "-> Adaptation Data out of range, first elevation used.\n");
            }
            /* convert product to the s3_t2_internal_rad struct & create output */
            result = translate_product(input_prod_data[1], buffer);
            
            if (VERBOSE) {
              fprintf(stderr,
                     "-> sample3_t2 returned from translate product. result=%d\n",
                                                                           result);
            }
            
         } /* end else out of range */
         
      }  /* end of if(loop_exit_stat==NORMAL) block      */
 
      
      /* If product successfully output and not last elevation */
      /* abort the remaining volume scan; the alternative is   */
      /* continue reading elevations until end of volume       */
      if( (loop_exit_stat==0) && (result==0)  /* successful product */
          && (elev_count < last_index)) {     /* not last elevation */
          
         RPGC_abort_remaining_volscan();
         
         if (VERBOSE) fprintf(stderr,
                  "aborting remaining volume, result=%d, last_elev=%d\n",
                                                       result, last_elev);
      }
        
      /* LABEL_CLEANUP  */
      
      /*  with abnormal loop exit, correct elevation count for free(). */
      if(loop_exit_stat != 0)
         elev_count--;
      
      /* clean up memory after product assembly                           */
      if (VERBOSE) {
        fprintf(stderr, "-> sample3_t2 freeing memory, elev_count is %d\n",
                                                                elev_count);
      }

      for(i = 1; i <= elev_count; i++) {
         free(input_prod_data[i]);
      }
      
      /* reset elevation counter                                          */
      elev_count = 1;
      
      if (VERBOSE) fprintf(stderr, "-> sample3_t2 reset for new volume\n");
      
   } /* end of Main PROCESS loop */

   fprintf(stderr, "\nsample3_t2 Program Terminated\n");
   
   return(0);
   
} /* end main */







/************************************************************************
Description:    clear_buffer: initializes a portion of allocated 
                memory to zero
Input:          pointer to the input buffer (already cast to char*)
Output:         none
Returns:        none
Globals:        the constant BUFSIZE is defined in the include file
Notes:          none
************************************************************************/
void clear_buffer(char *buffer) {
  /* zero out the input buffer */
  int i;

  for(i = 0; i < BUFSIZE; i++)
    buffer[i]=0;

  return;
  }



/************************************************************************
Description:    translate_product takes the raw intermediate product and
                places the data into a s3_t2_internal_rad structure
Input:          unsigned char *in_data_ptr - a pointer to the raw  
                                             intermediate product
                char *buffer - a pointer to the output buffer
Output:         none
Returns:        returns -1 if an error, otherwise returns 0
Globals:        none
Notes:          none
***************************************************************************/
/* Sample Alg 1.20 REPLACED replaced CVG_radial* with unsigned char * */
int translate_product(unsigned char *in_data_ptr, char *buffer) {

   /* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
   s3_t2_internal_rad *rad_data=NULL; /* pointer to an internal structure */
   int size_in_bytes;         /* temporary size storage variable          */
   int result2;               /* variable: holds function results         */
   int r, b;                  /* loop variables for radial and bin        */
   int rad_count;             /* hold count of number of radials          */   


   if (VERBOSE) {
     fprintf(stderr, "->sample3_t2 inside translate_product\n");
   }
   
   /* allocate memory for data transfer and assign data access pointers   */
   /* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
   rad_data = (s3_t2_internal_rad*)malloc(sizeof(s3_t2_internal_rad)); 
   
   if(rad_data == NULL) {
      RPGC_log_msg( GL_ERROR,
          "MALLOC Error: malloc failed on rad_data\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_MEM_LOADSHED);
      return(-1);
   }

   /* save the first 5 values (all 4 byte integers) in the input product  */
   /* Sample Alg 1.20 - changed 6 to 5 */
   size_in_bytes = sizeof(int) * 5;
   memcpy(rad_data, in_data_ptr, (size_t) size_in_bytes);
   in_data_ptr += size_in_bytes;
   
   rad_count = rad_data->num_radials;  /* bins_to_process from task 1 */
   
   if(TEST) {
      fprintf(stderr, "-> sample3_t2 read linear buffer id=%d\n",
            rad_data->linearbuffer_id);
      fprintf(stderr, "-> sample3_t2 read last elevation flag=%d\n",
            rad_data->flag);
   }
   
   /* now copy over the start angle data (all 4 byte integers)            */
   size_in_bytes = sizeof(int) * rad_data->num_radials;
   rad_data->start_angle = (int*)malloc( (size_t) (size_in_bytes) );
   
   if(rad_data->start_angle == NULL) {
      RPGC_log_msg( GL_ERROR,
         "MALLOC Error: malloc failed on rad_data->start_angle\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_MEM_LOADSHED);
      return(-1);
   }   
   
   memcpy(rad_data->start_angle, in_data_ptr, (size_t) size_in_bytes);
   in_data_ptr += size_in_bytes;

   if(TEST) {   
     fprintf(stderr, "-> sample3_t2 start angle first radial=%.1f\n",
                                     (rad_data->start_angle[0]/10.0));
     fprintf(stderr, "-> sample3_t2 start angle last radial=%.1f\n",
               (rad_data->start_angle[rad_data->num_radials-1]/10.0));  
   }      

   /* now copy over the angle delta data  (all 4 byte integers)           */
   rad_data->angle_delta = (int*)malloc((size_t)size_in_bytes);
   
   if(rad_data->angle_delta == NULL) {
      RPGC_log_msg( GL_ERROR,
         "MALLOC Error: malloc failed on rad_data->angle_delta\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_MEM_LOADSHED);
      return(-1);
      }   
   memcpy(rad_data->angle_delta, in_data_ptr, size_in_bytes);
   in_data_ptr += size_in_bytes;

   /* now, copy over the bin data -- 1 byte per bin in the input product  */
   /*                    but 4 bytes per bin in the internal array        */
   rad_data->radial_data = malloc(sizeof(int *) * rad_data->num_radials);
   
   if(rad_data->radial_data == NULL) {
      RPGC_log_msg( GL_ERROR,
         "MALLOC Error: malloc failed on rad_data->radial_data\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_MEM_LOADSHED);
      return(-1);
      }   
   
   /* allocate bins for each radial, 4 bytes per bin in internal array    */
   for(r = 0; r < rad_data->num_radials; r++) {
      rad_data->radial_data[r] = (int*)malloc(sizeof(int) *
                                              rad_data->num_range_bins);
      if(rad_data->radial_data[r] == NULL) {
         RPGC_log_msg( GL_ERROR,
             "MALLOC Error: malloc failed on rad_data->radial_data #%d\n",r);
         RPGC_rel_outbuf((void*)buffer, DESTROY);
         RPGC_abort_because(PGM_MEM_LOADSHED);
         return(-1);
      }   
   }
   /* copy the bin data now                                               */
   /* NOTE: the number of range bins is based upon the calculated number  */
   /*       of bins to process.  If this is greater than the number of    */
   /*       good bins, the data have been padded with '0'.                */
   for(r = 0; r < rad_data->num_radials; r++) {
/* DEBUG T2-1 */
      /* the input array (in_data_ptr) is an array of unsigned char    */
      /* the internal array (rad_data->radial_data) is an array of int    */
      for(b = 0; b < rad_data->num_range_bins; b++)
         rad_data->radial_data[r][b] = (int) *(in_data_ptr++);
/* DEBUG T2-2 */
   } /* end for r < number of radials */

   /* now copy over the base data header                                  */
   rad_data->bdh = (Base_data_header*)malloc(sizeof(Base_data_header));
   
   if(rad_data->bdh == NULL) {
      RPGC_log_msg( GL_ERROR, "MALLOC Error: malloc failed on rad_data->bdh\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_MEM_LOADSHED);
      return(-1);
   }   
   
   memcpy(rad_data->bdh, in_data_ptr, sizeof(Base_data_header));

   if (VERBOSE) {   
     fprintf(stderr, "sample3_t2 copy complete: num radials = %d\n", 
                                              rad_data->num_radials);
     fprintf(stderr, "sample3_t2 elevation number=%d   target_elev=%.1f\n",
                  rad_data->bdh->elev_num, rad_data->bdh->target_elev/10.0);
   }
      
   /* LABEL: ASSEMBLE assemble the final ICD product now                  */
   result2 = assemble_product(buffer, rad_data->num_range_bins, rad_data);
   
   /* clean up memory after product assembly                              */
   if (VERBOSE) {
     fprintf(stderr,
            "-> sample3_t2 freeing local memory rad_count=%d\n",rad_count);
   }
   
   free(rad_data->start_angle);
   free(rad_data->angle_delta);
   for(r = 0; r < rad_count; r++)
      free(rad_data->radial_data[r]);
   free(rad_data->radial_data);
   free(rad_data->bdh);
   free(rad_data);
   
   if (VERBOSE) {
     fprintf(stderr,"-> sample3_t2 memory clear complete\n");
   }
   
   /* Were we successful in assembling the product?     */
   if(result2 == 0) 
      return(0);  /* success */
   else
      return(-1); /* failure */
      
}  /* end translate_product() */



/***************************************************************************
Description:    assemble_product
Input:          s3_t2_internal_rad *rad_data - pointer to internal structure
                int number_bins
                
                char *buffer - buffer where ICD product will be constructed
Output:         none
Returns:        returns -1 if an error, otherwise returns 0
Globals:        none
Notes:          none
***************************************************************************/   
/* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
int assemble_product(char *buffer, int number_bins, s3_t2_internal_rad *rad_data) 
{
   int result3;     /* function return value                               */
   int length=0;   /* accumulated product length                          */
   int vol_num=0;  /* current volume number                               */
   int max_refl=0; /* maximum reflectivity value                          */

   int output_id;
   
   
   /* get the current volume number from the base data header             */
   /* that has been passed as part of the intermediate product            */
   vol_num = rad_data->bdh->volume_scan_num;

   /* building the ICD formatted output product requires a few steps      */
   /* step 1: build the product description block (pdb)                   */
   /* step 2: build the symbology block & data layer                      */
   /* step 3: complete missing values in the pdb                          */
   /* step 4: build the message header block (mhb)                        */
   /* step 5: forward the completed product to the system                 */

   output_id = RPGC_get_id_from_name( "SAMPLE3_FP");

   /* step 1: build the product description block -uses a system call*/
   if (VERBOSE) {
     fprintf(stderr,
           "\nsample3_t2: Creating the product description block now: vol=%d\n",
                                                                        vol_num);
   }
   
   RPGC_prod_desc_block((void*)buffer, output_id, vol_num);
      
   /* for testing show the header information now                         */
   if (TEST) {
      fprintf(stderr, "\n==> OUTPUT INITIAL MSG HEADER AND PROD DESC BLOCK\n");
      print_message_header(buffer);
      print_pdb_header(buffer);
   }

   /* step 2: build the symbology layer & digital radial data array.      */
   /* this routine returns both the overall length (thus far) of the      */
   /* product and the maximum reflectivity of all radials                 */
   if (VERBOSE) {
     fprintf(stderr, "sample3_t2: begin building the symbology and p16 layers\n");
   }

   max_refl = build_symbology_layer(buffer, rad_data , rad_data->num_radials,
                                    number_bins,  &length);
/* build_symbology_layer returns -1 on failure to malloc */
   if(max_refl == -1) {
      RPGC_log_msg( GL_ERROR,
         "MALLOC Error: malloc failed on rad_data->radial_data\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_MEM_LOADSHED);
      return(-1);
   }


   /* step 3: finish building the product description block by            */
   /* filling in certain values such as elevation index, maximum          */
   /* reflectivity, accumulated product length, etc                       */
      
   /* get elevation index and actual elevation using base data header     */
   /* that has been passed as part of the intermediate product            */
   if (VERBOSE) {
     fprintf(stderr, "sample3_t2: elev_ind=%hd  elevation=%hd\n",
            rad_data->bdh->rpg_elev_ind,rad_data->bdh->target_elev);
   }

   finish_pdb(buffer, rad_data->bdh->rpg_elev_ind,
              rad_data->bdh->target_elev, max_refl, length);      
   /* alternatively, we could use the new API functions to accomplish this*/
   /* which would eliminate the need for passing the base data header     */
   /* for example, we would have obtained the elevation index after       */
   /* reading the intermediate product:                                   */
   /*          elev_idx=(short)RPGC_get_buffer_elev_index((void *)inbuf); */
   /* and use the following to get target elevation:                      */
   /*     vcp_num=RPGC_get_buffer_vcp_num((void*)inbuf);                  */
   /*     target_elev=(short)RPGCS_get_target_elev_ang(vcp_num,elev_idx); */
       

   /* generate the product message header (use system call) and input     */
   /* total accumulated product length minus 120 bytes                    */
   result3 = RPGC_prod_hdr((void*)buffer, output_id, &length);
   
   if (VERBOSE) {
     fprintf(stderr, "-> sample3_t2 completed product length=%d\n", length);
   }
   if (TEST) {
      fprintf(stderr, "\n==> after prod_hdr\n");
      print_message_header(buffer);
      print_pdb_header(buffer);
   }
   
   /*(this routine adds the length of the product header to the           */
   /* "length" parameter prior to creating the final header)              */
      
   /* if the creation of the product has been a success                   */
   if(result3 == 0) { /*success*/
      /* print product header for testing purposes                        */
      int res;
      
      if(TEST) {
         fprintf(stderr,
         "sample3_t2: product header creation success...PRINT header\n");
         res=print_message_header(buffer);
         res=print_pdb_header(buffer); 
         fprintf(stderr, "\nsample3_t2: printing of header complete\n");
      }
         
      /* LABEL:OUTPUT_PROD    forward product and close buffer            */
      RPGC_rel_outbuf((void*)buffer, FORWARD);
      return(0);
      
   } else {  /* product failure (destroy the buffer & contents)            */
      RPGC_log_msg( GL_INFO, "sample3_t2: product header creation failure\n");
      RPGC_rel_outbuf((void*)buffer, DESTROY);
      RPGC_abort_because(PGM_PROD_NOT_GENERATED);
      return(-1);
   }
      
}  /* end of assemble product */



/*************************************
 * adaptation data read function 
 *************************************/
int read_sample3_t2_adapt()
{
    
double get_value = 0.0;
int ret = -1;


  ret = RPGC_ade_get_values("alg.sample3_t2.", "elev_select", &get_value);
  if(ret == 0) {
    input_elev = (int)get_value;
    
  } else {
    input_elev = 1;
    RPGC_log_msg( GL_INFO, 
         "input_elev DEA value unavailable, using program default\n");
  }

  return ret;
    
}



