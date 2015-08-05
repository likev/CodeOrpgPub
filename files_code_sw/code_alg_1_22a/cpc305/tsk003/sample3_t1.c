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
Module:         sample3_t1.c

Description:    sample3_t1.c is part of a chain-algorithm demonstration.
                The first task reads radial data and produces elevation
                data.  The second task reads elevation data and produces
                volume data.
                
                THE PURPOSE OF THIS SAMPLE ALGORITHM IS TO DEMONSTRATE
                A MULTIPLE TASK ALGORITHM.  NO USEFUL SCIENCE IS 
                ACCOMPLISHED BY THE SECOND TASK
                
                The structure of this algorithm complies with the guidance
                provided in the CODE Guide Vol 3.
                
                sample3_t1.c reads each radial and places the base data
                header and reflectivity moment into a container. When
                the elevation completes a product is output. This structure
                is an early demonstration of a polar array intermediate
                product and will be replaced in the future with the generic
                radial component. 
                
                Key source files for this algorithm include:

                sample3_t1.c         program source
                sample3_t1.h         main include file
                
                s3_t1_prod_struct.h  definition of s1_t1_intermed_prod_hdr
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.4,  May 2004     T. Ganger
              Linux update
              
Version 1.5   March 2006   T. Ganger
              Replaced stderr messages outside of test/debug sections with
                   RPGC_log_msg() calls.
              Revised to use current guidance for abort functions and 
                   abort reason codes.
              Simplified reading short data into byte arrays

Version 1.6   June 2006   T. Ganger
              Revised to use RPGC_reg_io
              Revised to use the new 'by_name' get_inbuf/get_outbuf functions

Version 1.7   July 2006   T. Ganger
              Modified to use the radial header offset to surveillance
                  data 'ref_offset'.  Tested RPGC_get_surv_data.
              Only malloc space for the header and surveillance data rather 
                   than the complete radial
              Replaced WAIT_ALL with WAIT_DRIVING_INPUT.
              
Version 1.8   March 2007  T. Ganger
              Included test code that aborts product output on designated
                  elevations in order to test this task's output as an
                  optional input to sample 2 task 1 (tsk005).
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.
              Added note that MAX_BASEDATA_REF_SIZE must be used instead of
                  BASEDATA_REF_SIZE for super resolution elevations.
              Removed the non-standard test for radial limit.  This would
                  have to be a dynamic test with SR_ data types.
              RPGC_get_radar_data failed in this algorithm for Build 9.
              
Version 1.9   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated use of defined output buffer ids by using
                  RPGC_get_id_from_name
              Modified to handle variable array sizes and changed product
                  range from 230 km to 460 km.
              When copying each radial, demonstrated ensuring that data 
                  values beyond the last good data bin (if any) are set to '0'.

              
Version 2.0   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Updated description of algorithm task and main function.  
                  Added comments to clarify processing.
              Added test to ensure that the algorithm did not attempt to
                  read or copy more bins than are actually in each 
                  base data radial.  Depending upon the method used to read 
                  the radial, a specific index is tested.
              Fixed a problem where the elevations after the first had some
                  blank radials.
              Eliminated overlapping radials from the output product by 
                  terminating processing at pseudo end of elev / volume and
                  reading until actual end of elev / volume.
              Used a new structure to write the output intermediate product.
              
Version 2.1   March 2009    T. Ganger  (Sample Algorithm ver 1.21)
              Modified test section printing out radial information 
                  including status to reflect ORDA usually having 360
                  radials.
                  
                  
$Id$
************************************************************************/

/* see the following include file for descriptions of constants and
for expected ICD block sizes */
#include "sample3_t1.h"


/************************************************************************
Description:    main function to drive the chain-algorithm demonstration
                (first task out of two). 
Input:          basedata radial as defined in the Base_data_radial
                structure (see the basedata.h include file)
Output:         an intermediate product formatted via a header structure in
                s3_t1_prod_struct.h with radial data in an array of bytes 
                (unsigned char).
Returns:        none
Globals:        none
Notes:          1). This sample algorithm task is a demonstration of 
                    producing an elevation product from radial data.  A  
                    copy of all radials is made into an elevation 
                    structure to facilitate processing multiple radials.
                    Memory could be saved if all of the processing is on 
                    a radial by radial basis.
                2). This algorithm can set different radial sizes not to
                    exceed 460 bins.  Since the maximum number of radials
                    is hard coded to 400 (and a static array used), this
                    algorithm cannot use any of the super resolution
                    data types.  Hence 460 is the maximum number of bins.
                3). The program takes no command line arguments.
                
************************************************************************/
int main(int argc,char* argv[]) {
   Base_data_radial
       *basedataPtr=NULL;    /* pointer to a base data radial structure   */
   Base_data_header *hdr;    /* pointer to the base data header structure */

   int *out_buffer;          /* pointer: access to allocated memory       */
                             /* which holds a completed output product    */ 

   /* A limit of 400 radials precludes this algorithm from */
   /* using one of the super resolution data types         */
   char *radial[BASEDATA_MAX_RADIALS];
                             /* array of pointers to radial structures    */
                             /* used to contain an entire elevation       */
                             /* WARNING Must be increased from 400 to     */
                             /* 800 if registered for an SR_ data type    */
     
   int ref_enable;           /* variables: used to hold the status of     */
   int vel_enable;           /* the individual moments of base data.      */
   int spw_enable;           /* tested after reading the first radial     */

   int PROCESS=TRUE;         /* Boolean used to control the daemon        */
                             /* portion of the program. Process until set */
                             /* to FALSE.                                 */

   int TEST=FALSE;            /* Boolean to allow diagnostic output        */
/*   int TEST=TRUE; */

   int VERBOSE=FALSE;       /* Boolean for verbose output                */
/*   int VERBOSE=TRUE; */

   int i;                    /* loop variable                             */
   int rad_idx=0;            /* counter: radial index being processed    */
   int last_rad=0;           /* Sample Alg 1.21 */
   short elev_idx=0;         /* variable: holds current elevation index   */
   short target_elev=0;      /* short: target elevation times 10          */
   int vcp_num=0;            /* variable: holds vcp number                */
   int loop_exit_stat=0;     /* variable: holds exit status from loop     */
   int opstatus;             /* variable: holds function return values    */
   short radial_status=0;      /* variable: status of each radial input     */

  int output_id;


  int bins_to_process = 0;  /* number of data bins to process in radial */
  int bins_to_70k;
  int max_num_bins = BASEDATA_REF_SIZE;  /* 460 */
                            /* could be reduced to limit range of product */
                            /* and is also limited by 70,000 ft MSL       */

/* Sample Alg 1.20 - make sure we do not read more than is in a radial */
  int max_bins_to_copy;
  int bins_to_copy;
  
/*  int new_buffer_size; */


  short *surv_data;   /* pointer to the surveillance data in the radial */
  int index_first_bin=999;  /* used with method 2 */
  int index_last_bin=999;   /* used with method 2 */
  
  Generic_moment_t gen_moment;  /* used with method 3 */
  /* if passed as NULL causes a malloc by RPGC_get_radar_data */
  Generic_moment_t *my_gen_moment = &gen_moment;  /* method 3 */
  
   /* special test flag to abort this task in order to test use of this
    * product as an optional input to sample 4 task 1                    */
   int ABORT_FOR_OPT_SAMPLE_4=FALSE;
/*   int ABORT_FOR_OPT_SAMPLE_4=TRUE; */

   /* ------------------------------------------------------------------- */
   
   hdr = NULL;
   
   fprintf(stderr, "\nBegin Sample 3 Task 1 Algorithm\n");

   /* LABEL:REG_INIT  Algorithm Registration and Initialization Section   */

   /* register inputs and outputs based upon the            */
   /* contents of the task_attr_table (TAT).                */
   /* input:REFLDATA(79), output:SAMPLE3_IP(1999)           */
   RPGC_reg_io(argc, argv);
  
   /* register algorithm infrastructure to read the Scan Summary Array    */
   RPGC_reg_scan_summary();

   /* ORPG task initialization routine. Input parameters argc/argv are    */
   /* not used in this algorithm                                          */
   RPGC_task_init(ELEVATION_BASED, argc, argv);

   /* output message to both the task output file and the task log file */   
   fprintf(stderr,
      "-> algorithm initialized and proceeding to loop control\n");
   RPGC_log_msg( GL_INFO,
      "-> algorithm initialized and proceeding to loop control\n");


   /* while loop that controls how long the task will execute. As long as */
   /* PROCESS remains TRUE, the task will continue. The task will         */
   /* terminate upon an error condition                                   */
   while(PROCESS) {

      /* system call to indicate a data driven algorithm. block algorithm */
      /* until good data is received the product is requested             */
      RPGC_wait_act(WAIT_DRIVING_INPUT);

      /* LABEL:BEGIN_PROCESSING Released from Algorithm Flow Control Loop */
      if (VERBOSE) fprintf(stderr,
          "\n-> sample3_t1 algorithm passed ACL and began processing\n");
      RPGC_log_msg( GL_INFO,
          "-> sample3_t1 algorithm passed ACL and began processing\n");

      /* allocate a partition (accessed by the pointer, buffer) within the*/
      /* SAMPLE3_IP linear buffer. error return in opstatus              */

      out_buffer = (int*)RPGC_get_outbuf_by_name("SAMPLE3_IP", BUFFSIZE, &opstatus);
       
      /* check error condition from buffer allocation. abort if abnormal  */
      if(opstatus != NORMAL) {
         RPGC_log_msg( GL_INFO,
            "ERROR: Aborting from RPGC_get_outbuf...opstatus=%d\n", opstatus);
         RPGC_abort(); 
         continue;
      }
      
      if (VERBOSE) {
        fprintf(stderr, "-> successfully obtained output buffer size %d\n", 
                                                                   BUFFSIZE);
      }
      /* make sure that the buffer space is initialized to zero           */
      clear_buffer((char*)out_buffer);
    
      /* LABEL:READ_FIRST_RAD Read first radial of the elevation and      */
      /* accomplish the required moments check.                           */
      /* ingest one radial of data from the BASEDATA linear buffer. The   */
      /* data will be accessed via the basedataPtr pointer                */
      if (VERBOSE) {    
        fprintf(stderr, "sample3_t1 -> reading first radial buffer\n"); 
      }

      basedataPtr =
          (Base_data_radial*)RPGC_get_inbuf_by_name("REFLDATA", &opstatus);

      hdr = (Base_data_header *) basedataPtr;

      /* check radial ingest status before continuing                     */
      if(opstatus != NORMAL){
         RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_get_inbuf\n");
         RPGC_rel_outbuf((void*)out_buffer, DESTROY);
         RPGC_abort();
         continue;
      }
      
      if (VERBOSE) {
        fprintf(stderr,
                "sample3_t1 -> successfully read first radial buffer\n");
      }
      
      /* test to see if the required moment (reflectivity) is enabled     */
      RPGC_what_moments((Base_data_header*)basedataPtr, &ref_enable,
                                           &vel_enable, &spw_enable);

      if(ref_enable != TRUE){
         RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_what_moments\n");
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)out_buffer, DESTROY);
         RPGC_abort_because(PGM_DISABLED_MOMENT); 
         continue;
      }

      if (VERBOSE) {        
        fprintf(stderr, "sample3_t1 -> required moments enabled\n");
      }


      /* ELEVATION PROCESSING SEGMENT. continue to ingest and process     */
      /* individual base radials until either a failure to read valid     */
      /* input data (a radial in correct sequence) or until after reading */
      /* and processing the last radial in the elevation.                 */
      /* Set Exit Status to NORMAL */
      
      /* get the current elevation angle & index from the incoming data*/
      elev_idx = (short)RPGC_get_buffer_elev_index((void *)basedataPtr);
      
      vcp_num = RPGC_get_buffer_vcp_num((void*)basedataPtr);
      target_elev = (short)RPGCS_get_target_elev_ang(vcp_num, elev_idx);
      
      
      /* limit number of bins to the 70.000 ft MSL ceiling */
      bins_to_70k = RPGC_bins_to_ceiling( basedataPtr, hdr->surv_bin_size );

      if(bins_to_70k < max_num_bins)
          bins_to_process = bins_to_70k;
      else
          bins_to_process = max_num_bins;   /* 460 */
      /* Sample Alg 1.20 - ensure even number of bins for alignment of BDH */
      if( (bins_to_process % 2) !=0)
          bins_to_process++;
          
      /* NOTE: when accomplishing numerical calculations including assembly   */
      /*       of the final product arrays, the number of good bins must be   */
      /*       accounted for.  This can be made by                            */
      /*          a. reducing the size of the internal data array processed   */
      /*             (bins_to_process), or by                                 */
      /*          b. ensuring all data values after the last good bin are     */
      /*             set to a value of '0'.                                   */
      /*       This algorithm chooses b and is tested for each radial         */

      rad_idx = 0;
      loop_exit_stat = 0;

      /* ------------------------------------------------------------------ */

      while(TRUE) {
    
         /* LABEL:PROCESS_RAD Here we process each radial individually.   */
         /* Any processing that can be accomplished without comparison    */
         /* between other radial can be accomplished here.                */
      

/* DEBUG */
 /* fprintf(stderr,"\nDEBUG at top of radial processing loop\n"); */

         /* ===== BEGIN SPECIAL TEST CODE ==================================== */
         /*     THIS CODE BLOCK ABORTS THIS TASK IN ORDER TO TEST THE USE      */
         /*     OF THIS PRODUCT AS AN OPTIONAL INPUT FOR SAMPLE 4 TASK 1.      */
         if(ABORT_FOR_OPT_SAMPLE_4 == TRUE) {
            if(elev_idx == 2) {
                RPGC_log_msg( GL_INFO,
                    "Test Induced Abort at elevation %d\n", elev_idx);
                if(VERBOSE)
                   fprintf(stderr, "Induced Abort at elevation %d\n", elev_idx);
                RPGC_rel_inbuf((void*)basedataPtr);
                RPGC_rel_outbuf((void*)out_buffer, DESTROY);
                RPGC_abort_because(PGM_INPUT_DATA_ERROR);
                loop_exit_stat = TERMINATE;     
                break;
            } else {
                if(VERBOSE)
                  if(rad_idx==1)
                    fprintf(stderr, "Abort not induced at elev %d\n", elev_idx);
            }
         }
         /* ===== END SPECIAL TEST CODE ====================================== */


/* While all of the following work, Method 2 is typical for basic moments */
/* Method 3 must be used for advanced Dual Pol moments                    */
         /* Method 1 */
/*         surv_data = (short *) ((char *) basedataPtr + hdr->ref_offset); */
/*         max_bins_to_copy = hdr->surv_range + hdr->n_surv_bins - 1;      */
                          /* '-1' because the indexes are 1-based */
                          /* we do not assume hdr->surv_range is always 1 */
         /* Method 2 */
         surv_data = (short *) RPGC_get_surv_data( (void *)basedataPtr, 
                                  &index_first_bin, &index_last_bin);
         max_bins_to_copy = index_last_bin + 1; 
                          /* '+1'because indexes are 0-based */
                          /* we do not assume that index_first_bin is always 0 */
         
         /* Method 3 */
/*         surv_data = (short *) RPGC_get_radar_data( (void *)basedataPtr, */
/*                                      RPGC_DREF, my_gen_moment );        */
/*         max_bins_to_copy = hdr->surv_range + hdr->n_surv_bins - 1;      */

         /* test for NULL moment pointer (Methods 2 & 3) and for 0 offset */
         if(surv_data == NULL) {
            RPGC_log_msg( GL_INFO, "ERROR: NULL radial detected: %d\n",
               rad_idx);
            RPGC_rel_inbuf((void*)basedataPtr);
            RPGC_rel_outbuf((void*)out_buffer, DESTROY);
            RPGC_abort_because(PGM_INPUT_DATA_ERROR);
            loop_exit_stat = TERMINATE;
            break;
         }
         
         if(hdr->ref_offset == 0) {
            RPGC_log_msg( GL_INFO, "ERROR: reflectivity offset 0: %d\n",
               rad_idx);
            RPGC_rel_inbuf((void*)basedataPtr);
            RPGC_rel_outbuf((void*)out_buffer, DESTROY);
            RPGC_abort_because(PGM_INPUT_DATA_ERROR);
            loop_exit_stat = TERMINATE;
            break;
         }


         /* allocate one radial's worth of memory                         */
         radial[rad_idx]=
             (char *)malloc( sizeof(Base_data_header) + (bins_to_process * 2) );


         if(radial[rad_idx] == NULL) {
            RPGC_log_msg( GL_INFO,
                "MALLOC Error: malloc failed on radial %d\n", rad_idx);
            RPGC_rel_inbuf((void*)basedataPtr);
            RPGC_rel_outbuf((void*)out_buffer, DESTROY);
            RPGC_abort_because(PGM_MEM_LOADSHED);
            loop_exit_stat = TERMINATE;     
            break;
         }
      
         /* copy the radial header */
         memcpy(radial[rad_idx], basedataPtr, sizeof(Base_data_header));
      
         /* copy the reflectivity data */
/* Sample Alg 1.20 - only copy the the lessor of (a) number of bins in  */
/*                   the radial or (b) bins_to_process                  */
         if(bins_to_process <= max_bins_to_copy)
            bins_to_copy = bins_to_process;
         else 
            bins_to_copy = max_bins_to_copy;
         /* later when writing the output product, any data bins */
         /* beyond max_bins_to_copy are set to '0'               */

/* DEBUG T1-a */

         memcpy(radial[rad_idx] + sizeof(Base_data_header),
                  (char *)surv_data, bins_to_copy * 2);

      
         /* read the radial status flag                                   */
         radial_status = basedataPtr->hdr.status & 0xf;

         /* now that the data that we want from the radial is in temporary*/
         /* storage...release the input buffer                            */
         RPGC_rel_inbuf((void*)basedataPtr);

         /* increment the radial counter                                  */
         rad_idx++; 

/* DEBUG T1-b */

         /* if end of elevation or volume then exit loop.                 */
         /* Sample Alg 1.20 - modified to exit radial processing at       */
         /*                   pseudo end rather than actual end in        */
         /*                   order to avoid overlapping radials if       */
         /*                   ingesting historical data before ORDA       */
         /*      This requires subsequently reading to the actual end     */
         /*      of elevation                                             */
         if( radial_status == GENDEL || radial_status == GENDVOL || 
             radial_status == PGENDEL || radial_status == PGENDVOL ) {
            if(TEST) fprintf(stderr,
                       "-> End of elevation found, loop exit status=%d\n",
                        loop_exit_stat);
            /* exit with loop_exit_stat==NORMAL and no ABORT */
            break;
         }
     
         /* LABEL:READ_NEXT_RAD Read the next radial of the elevation.    */
         /* ingest one radial of data from the BASEDATA linear buffer. The*/
         /* data will be accessed via the basedataPtr pointer             */
         

         basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("REFLDATA",
                                                                  &opstatus);

         hdr = (Base_data_header *) basedataPtr;

         /* check radial ingest status before continuing                  */ 
         if(opstatus != NORMAL){
            loop_exit_stat = opstatus;
            RPGC_log_msg( GL_INFO,
               "ERROR: Aborting from RPGC_get_inbuf, loop exit status=%d\n",
                                                             loop_exit_stat);
            RPGC_rel_outbuf((void*)out_buffer, DESTROY);
            RPGC_abort();
            break;
         }

      } /* end while TRUE, read radials */

      /* ------------------------------------------------------------------ */
      
      /* Sample Alg 1.21 */
      last_rad = rad_idx;
    
    
      if (VERBOSE) {
        fprintf(stderr, "=> out of radial ingest loop...elev index=%d\n",
                                                                elev_idx);
      }
    
      /* LABEL:PROCESS_ELEV Here we accomplish any processing of the      */
      
      if(loop_exit_stat == NORMAL) {
        
         /* Sample Alg 1.20 - may have exited on pseudo end, now must read */
         /*                  until the actual end of elevation / vol       */
         while(TRUE) { /* read remaining radials if any */
            if( radial_status != GENDEL && radial_status != GENDVOL ) {

               basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("REFLDATA",
                                                                        &opstatus);
               if(opstatus != NORMAL){
                  loop_exit_stat = opstatus;
                  RPGC_log_msg( GL_INFO,
                    "ERROR: Aborting from RPGC_get_inbuf, loop exit status=%d\n",
                                                                  loop_exit_stat);
                  RPGC_rel_outbuf((void*)out_buffer, DESTROY);
                  RPGC_abort();
                  break;
               }
               hdr = (Base_data_header *) basedataPtr;
               radial_status = basedataPtr->hdr.status & 0xf;
               RPGC_rel_inbuf((void*)basedataPtr);
               rad_idx++; /* Sample Alg 1.21 */
               
            } else { /* we found actual end of elevation / volume */
               if (TEST) {
                  fprintf(stderr, 
                          "=> finished finding actual end, radial = %d\n", rad_idx+1);
                  fprintf(stderr, 
                          "   Radial Status = %d  where\n"
                          "    0=beg_ele, 1=int, 2=end_ele, 3=beg_vol, 4=end_vol,\n"
                          "    8=pseudo_end_ele, 9=pseudo_end_vol\n",  
                                                            radial_status);
               } /* end TEST */
                                                            
               /* exit with loop_exit_stat==NORMAL and no ABORT */
               break;
            }
            
         } /* end while */
        
         /* elevation as a whole.  Processing that requires comparisons   */
         /* between radials should be accomplished here                   */
         /* Note: in this algorithm we have not yet ensured that the      */
         /* the data values beyond the last good bin for each radial have */
         /* have been set to '0'                                          */
      } /* end if loop_exit_stat == NORMAL */
      
      /* the end of the ELEVATION PROCESSING SEGMENT of the algorithm     */


      /* LABEL:ASSEMBLE The assembly of the intermediate product.         */
      /* product conforms to the structure defined in s3_t1_prod_struct.h */
      if(loop_exit_stat == NORMAL) {
         int length=0;   /* accumulated product length                    */
         /* Sample Alg 1.20 - replace CVG_radial with s3_t1_intermed_prod_hdr */
         s3_t1_intermed_prod_hdr *out_hdr = (s3_t1_intermed_prod_hdr *)out_buffer;
         
         Base_data_header *bdh=NULL;  /* base data header struct          */
         
         unsigned char *char_buf; /* used to access individual raw data bins  */
         short *short_data; /* pointer to short to read base data levels  */
                            /* from a base data radial                    */
         int offset;     /* offset into various memory buffers            */
         /* Sample Alg 1.20 */
         int rad_beg;    /* holds offset to beginning of current radial   */
         int r, b;       /* loop variables for radial and bin             */
         FILE *outfile;  /* file pointer used for diagnostic output       */
         char fname[30]; /* variable used to hold output file name        */
         int test_out=FALSE; /* Boolean used to control diagnostic output */

         if (VERBOSE) {
           fprintf(stderr, "=> loop exit status is normal\n");
         }

         /* load the beginning portion of the s1_t1_prod_radial structure */
         
         output_id = RPGC_get_id_from_name( "SAMPLE3_IP");
         /* Sample Alg 1.20 - replaced raw_rad with out_hdr */
         out_hdr->linearbuffer_id = output_id;   /* output LB ID          */

         /* limited to 460 bins because radial limit precludes */
         /* using a super resolution type                      */
         /* Sample Alg 1.20 - replaced raw_rad with out_hdr */
         out_hdr->num_range_bins = bins_to_process; 

         if(TEST) fprintf(stderr,
                 "setting last elev flag: out_hdr->flag=%d\n",out_hdr->flag);
                 
         /* Sample Alg 1.20 - replaced raw_rad with out_hdr */
         /* Sample Alg 1.21 - changed rad_idx to last_rad */
         out_hdr->num_radials = last_rad;          /* #radials read this elev*/
         out_hdr->num_bytes_per_bin = 1;         /* 1 byte/bin 256 levels */

         /* first fill in the angle structs by extracting the start angle */
         /* & delta angle information for the base data radials that we've*/
         /* read so far - each set of angle info is stored contiguously   */
         /* Sample Alg 1.20 - changed CVG_RADIAL_HEADER_OFFSET (6) */
         /*                   to S3_T1_DATA_OFFSET (5)             */
         offset = S3_T1_DATA_OFFSET;   /* offset=5 4-byte integers */
         /* load start angle data                                         */
         /* Sample Alg 1.20 - replaced raw_rad with out_hdr */
         for(r = 0; r < out_hdr->num_radials; ++r) {
            bdh = (Base_data_header *)(radial[r]);
            out_buffer[offset++] = bdh->start_angle;
         }
         
         /* load angle delta data          */
         /* Sample Alg 1.20 - replaced raw_rad with out_hdr */
         for(r = 0; r < out_hdr->num_radials; ++r) {
            bdh = (Base_data_header *)(radial[r]);
            out_buffer[offset++] = bdh->delta_angle;
         }

         /* now, fill in the actual data - one byte at a time (offset=8)  */
         /* here, char_buf points to the beginning of the data array      */
         char_buf = (unsigned char *)(&(out_buffer[offset])); 
                                                     
         offset = 0;  /* set offset to zero */
         
         /* Sample Alg 1.20 - replaced raw_rad with out_hdr */
         for(r = 0; r < out_hdr->num_radials; ++r) {
            bdh = (Base_data_header *)(radial[r]);
            /* reading basedata values as shorts solves Endian issues     */ 
            short_data = (short *) &*(radial[r]+sizeof(Base_data_header));
/* DEBUG T1-1*/
            rad_beg = offset; /* used in debug printout */

            for(b = 0; b < bins_to_process; ++b) {
               /* ensure any data bins beyond the last good bin are set to 0 */
               if( b < bins_to_copy )
                  char_buf[offset++] = (unsigned char) short_data[b];
               else
                  char_buf[offset++] = (unsigned char) 0;
               
            } /* end for j<bins_to_process */
/* DEBUG T1-2 */
         } /* end for r< num_radials */

         if (VERBOSE) {
           fprintf(stderr, "=> finished loading all bins into char array\n");
         }

         /* recalculate offset value                                     */
         /* Sample Alg 1.20 - changed raw_rad to out_hdr and  */
         /*     CVG_RADIAL_HEADER_OFFSET to S3_T1_DATA_OFFSET */
         offset = S3_T1_DATA_OFFSET*sizeof(int) + 
                   2 * out_hdr->num_radials*sizeof(int) +
                    bins_to_process * out_hdr->num_radials * sizeof(char);
                      
         if (TEST) {
            /* diagnostics to test integrity of a radial of data           */
            bdh = (Base_data_header*)(radial[0]);
            fprintf(stderr,
               "bdh test: vcp=%hd target elev=%hd start angle=%hd delta=%hd\n",
              bdh->vcp_num, bdh->target_elev, bdh->start_angle, bdh->delta_angle);
            fprintf(stderr,
               "bdh test: msg len=%hd msg type=%hd spot blank range=%hd\n",
                         bdh->msg_len, bdh->msg_type, bdh->spot_blank_flag);
         }
      
         /* copy a basedata HEADER into the output product for use in */
         /* sample3_t2                                                */
         /* here, char_buf points to the beginning of the output buffer     */
         /* NOTE: alignment assured by having number of bins always even   */
         char_buf = (unsigned char*)out_buffer;
         memcpy(char_buf+offset, bdh, sizeof(Base_data_header));

         /* calculate the final overall length of the output product   */
         /* Sample Alg 1.20 - changed raw_rad to out_hdr and 6 to 5 */
         length = 5 * sizeof(int) + 2 * out_hdr->num_radials * sizeof(int) + 
            bins_to_process * out_hdr->num_radials * sizeof(char) + 1 * sizeof(int) 
            + sizeof(Base_data_header);

         if (VERBOSE) {
           fprintf(stderr, "->final length of product=%d\n",length);
           fprintf(stderr, "  number of radials is %d, bins processed is %d\n",
                                         out_hdr->num_radials, bins_to_process);
         }

         /* forward constructed data structure to intermediate LB.        */
         /* if diagnostic output is desired...store data now              */
        
         if(test_out == TRUE) {
            sprintf(fname, "sample3_t1out.%d", elev_idx);
            if((outfile = fopen(fname, "w")) != NULL) {
               /* copy the data out of the buffer in memory to a file     */
               fwrite(out_buffer, length, 1, outfile);
               fclose(outfile);
            }
         }
            
         /* LABEL:OUTPUT_PROD    forward product and close buffer         */
         RPGC_rel_outbuf((void*)out_buffer, FORWARD);
         
      }    /* end of if(opstatus==NORMAL) block                          */

      /* LABEL:CLEANUP   section to reset variables / free memory         */
      /* free each previously allocated radial array                      */
      if (VERBOSE) {
        /* Sample Alg 1.21 - changed rad_idx to last_rad */
        fprintf(stderr, "free radial array (last_rad=%i)\n", last_rad);
      }
      
      /* Sample Alg 1.21 - changed rad_idx to last_rad */
      for(i = 0; i < last_rad; i++) {
         if(radial[i]!=NULL)
             free(radial[i]);
      }

      rad_idx = 0; /* reset radial counter to 0                               */
      last_rad = 0;
 
      if(VERBOSE)
         fprintf(stderr, "Elev Complete:  Elev Index=%d  Target Elev=%d\n",
                                                      elev_idx,target_elev);
    
   }    /* end while PROCESS == TRUE                                      */

   fprintf(stderr,"\nSample 3 Task 1 Program Terminated\n");
   
   return(0);
   
} /* end of main ----------------------------------------------------- */



/***************************************************************************
Description:    clear_buffer: initializes a portion of allocated 
                memory to zero
Input:          pointer to the input buffer (already cast to char*)
Output:         none
Returns:        none
Globals:        the constant BUFFSIZE is defined in the include file
Notes:          none
***************************************************************************/
void clear_buffer(char *buffer) {
   /* zero out the input buffer */
   int i;

   for(i = 0; i < BUFFSIZE; i++)
      buffer[i] = 0;

   return;
   
}

