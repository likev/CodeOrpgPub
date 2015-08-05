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
Module:         sample2_radrefl.c

Description:    sample2_radrefl.c is a sample algorithm for the ORPG.
                The algorithm has been implemented using ANSI-C 
                interfaces and source code and takes advantage of existing 
                ORPG system services.
                
                This program can be used as a template to build more
                complex (integrated) algorithms and includes a 
                significant amount of in-line commenting to describe
                each step.

                The structure of this algorithm complies with the guidance
                provided in the CODE Guide Vol 3.
            
                sample2_radrefl.c ingests radials of base data and creates
                an "ICD-formatted" 16-level run length encoded product 
                using reflectivity data.
                
                Key source files for this algorithm include:

                sample2_radrefl.c        the algorithm driver files
                sample2_radrefl.h
        
                s2_print_diagnostics.c   source to display message &
                s2_print_diagnostics.h   header information
        
                s2_symb_layer.c          source to create the symbology
                s2_symb_layer.h          and data packet layers

                Run Length Encoding Routines -------------------------

                This algorithm now uses the new run length encode
                function provided by the Build 9 ORPG.

-----------------------------------------------------------------------
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.9,  June 2004    T. Ganger
              Modified to reflect the new size of the base data header.
              
Version 2.0   March 2006   T. Ganger
              Replaced stderr messages outside of test/debug sections with
                   RPGC_log_msg() calls.
              Revised to use current guidance for abort functions and 
                   abort reason codes.
                   
Version 2.1   June 2006   T. Ganger
              Revised to use RPGC_reg_io
              Revised to use the new 'by_name' get_inbuf/get_outbuf functions

Version 2.2   July 2006   T. Ganger
              Modified to use the radial header offset to surveillance
                  data 'ref_offset'.  Tested RPGC_get_surv_data.
              Only malloc space for the header and surveillance data rather 
                   than the complete radial
              Modified to use RPGC_run_length_encode rather than local 
                  function.
              Replaced WAIT_ALL with WAIT_DRIVING_INPUT.   
              In symb_layer.c: Used the radial header fields for index to 
                  first good bins and number of bins rather than just assuming
                  the usual values.

Version 2.3   March 2007   T. Ganger  
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.
              Added note that MAX_BASEDATA_REF_SIZE must be used instead of
                  BASEDATA_REF_SIZE for super resolution elevations.
              RPGC_get_radar_data failed in this algorithm for Build 9.
              
Version 2.4   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated use of defined buffer id's by using
                  RPGC_get_id_from_name
              Changed number of bins from 230 to 460 maximum and used a 
                  variable to hold the value rather than a constant.
              Used globally defined constants for maximum number of bins
                  and maximum number of radials.
              Reduced size of the internal data array if needed to account
                  for the last "good" bin in the input array.
              Used the new default abort reason code PGM_PROD_NOT_GENERATED.
              Passed calibration constant as a single float rather than
                  two shorts.
            
Version 2.5   Movember 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Added test to ensure that the algorithm did not attempt to
                  read or copy more bins than are actually in each 
                  base data radial.  Depending upon the method used to read 
                  the radial, a specific index is tested.
              Eliminated overlapping radials from the output product by 
                  terminating processing at pseudo end of elev / volume and
                  reading until actual end of elev / volume.
                  
Version 2.6   February 2009    T. Ganger  (Sample Algorithm ver 1.21)
              Changed input data from REFLDATA to SAMPLE2IN in order to
                  facilitate modification to tast_attr_table to switch
                  between REFLDATA(79) and DUALPOL_REFLDATA(307).
              If ingesting DUALPOL_REFLDATA, increased max number of bins,
                  increased max number of radials, and increased the size
                  of the output buffer allocated. In Build 12 the max
                  number of radials will not have to be increased.
              Modified test section printing out radial information 
                  including status to reflect ORDA usually having 360
                  radials.
              Added test section to print dual pol data parameters.
     
$Id$
************************************************************************/


/* see the following include file for descriptions of constants for
for expected ICD block sizes */
#include "sample2_radrefl.h"


/************************************************************************
Description:    main function to drive the sample radial data array
                algorithm
Input:          one radial at a time as defined in the Base_data_radial
                structure (see basedata.h)
Output:         a formatted ICD graphical product which is forwarded to
                the ORPG for distribution
Returns:        none
Globals:        none
Notes:          1). This sample algorithm is a demonstration of producing
                a run length encoded (RLE) product from base data. 
                2). This algorithm provides the capability to set the 
                desired product range up to 460 bins.  This limit is
                related to registration for a non super resolution data
                type : BASEDATA, RECOMBINED_RAWDATA, etc.
                3). The program takes no command line arguments.

************************************************************************/
int main(int argc,char* argv[]) {
  /* variable declarations and definitions ---------------------------- */
  Base_data_radial 
      *basedataPtr=NULL;    /* a pointer to a base data radial structure*/
  Base_data_header *hdr;    /* a pointer to the base data header        */

  int ref_enable;           /* variables: used to hold the status of    */
  int vel_enable;           /* the individual moments of base data.     */
  int spw_enable;           /* tested after reading first radial        */
  
  int PROCESS=TRUE;         /* Boolean used to control the daemon       */
                            /* portion of the program. Process until set*/
                            /* to false.                                */
  int i;                    /* loop variable                            */
  
  int rad_idx=0;            /* counter: radial index being processed    */
  int last_rad=0;           /* Sample Alg 1.21 */
  short elev_idx=0;         /* variable: used to hold current elev index*/
  short elevation=0;        /* variable: used to hold the value of elev */
  int vol_num=0;            /* current volume number                    */
  int opstatus;             /* variable:  used to hold return values    */
                            /* from system calls                        */
  short *buffer;            /* pointer: access to allocated memory to   */
                            /* hold a completed ICD product             */
  short radial_status=0;    /* variable: status of each radial input    */
  /* Sample Alg 1.21 - increased to an array of 800 pointers for test   */
  char *radial[BASEDATA_MAX_SR_RADIALS]; /* 800 */
                            /* WARNING Must be increased from 400 to    */
                            /* 800 if registered for an SR_ data type   */
  float cal_const;          /* calibration constant (used for pdp)      */

  int output_id, input_id;

  /* dynamically determine number of bins to process */
  int bins_to_process = 0;  /* number of data bins to process in radial */
  int bins_to_70k;
  int max_num_bins = BASEDATA_REF_SIZE;  /* 460 */
                            /* could be reduced to limit range of product   */
                            /* and is subsequently limited by 70,000 ft MSL */
  int max_num_radials = BASEDATA_MAX_RADIALS;  /* 400 */

/* Sample Alg 1.20 - make sure we do not read more than is in a radial */
  int max_bins_to_copy;
  int bins_to_copy;
  short *short_data=NULL;

  short *surv_data=NULL;   /* pointer to the surveillance data in the radial */
  int index_first_bin=999;  /* used with method 2             */
  int index_last_bin=999;   /* used with method 2             */
  
  Generic_moment_t gen_moment;  /* used with method 3         */
  /* if passed as NULL causes a malloc by RPGC_get_radar_data */
  Generic_moment_t *my_gen_moment = &gen_moment;  /* method 3 */


/* test / debug variable declarations and definitions ------------------*/
  int TEST=FALSE;           /* test flag: set to true for diag messages */

  int VERBOSE=FALSE;        /* verbose processing output                */


  hdr = NULL;
   
  fprintf(stderr, "\nBegin Radial Reflectivity Algorithm in C\n");

  /* LABEL:REG_INIT  Algorithm Registration and Initialization Section  */
  
  RPGC_init_log_services(argc, argv);
  
 
  /* register inputs and outputs based upon the            */
  /* contents of the task_attr_table (TAT).                */
  /* Sample Alg 1.21 */
  /* input: either REFLDATA(79), or DUALPOL_REFLDATA(307)  */
  /*        based upon definition of SAMPLE2IN             */
  /* output:RADREFL(1991)                                  */
  RPGC_reg_io(argc, argv); 
 
/* register algorithm infrastructure to read the Scan Summary Array   */
  RPGC_reg_scan_summary();

  /* register adaptation data and read current data into structures    */


  /* ORPG task initialization routine. Input parameters argc/argv are   */
  /* not used in this algorithm                                         */
  RPGC_task_init(ELEVATION_BASED, argc, argv);
  
  fprintf(stderr, "-> algorithm initialized and proceeding to loop control\n");

  RPGC_log_msg( GL_INFO,
       "-> algorithm initialized and proceeding to loop control\n");


  /* Sample Alg 1.21 */
  input_id = RPGC_get_id_from_name( "SAMPLE2IN");


  /* while loop that controls how long the task will execute. As long as*/
  /* PROCESS remains true, the task will continue. The task will        */
  /* terminate upon an error condition                                  */
  while(PROCESS) {

    /* system call to indicate a data driven algorithm. block algorithm */
    /* until good data is received and the product is requested         */
    RPGC_wait_act(WAIT_DRIVING_INPUT);

   /* LABEL:BEGIN_PROCESSING Released from Algorithm Flow Control Loop   */
   if (VERBOSE) fprintf(stderr,
        "-> algorithm passed loop control and begin processing\n");

   RPGC_log_msg( GL_INFO,
        "-> algorithm passed loop control and begin processing\n");

    /* allocate a partition (accessed by the pointer, buffer) within the*/
    /* RADREFL linear buffer. error returns in opstatus                 */

    /* Sample Alg 1.21 - With the option to register DUALPOL_REFLDATA in */
    /*       instead of REFLDATA, the maximum number fo bins could be    */
    /*       1840 instead of 460                                         */
    /*       DUALPOL_REFLDATA will have an azimuth resolution of 1.0 deg */
    /*       for Build 12, however in Build 11 we get 0.5 deg            */
    if(input_id == 307) {
        buffer = (short*)RPGC_get_outbuf_by_name("RADREFL", SR_BUFSIZE, &opstatus);
        max_num_bins = MAX_BASEDATA_REF_SIZE; /* 1840 */
        /* the following will not be required with the final version of */
        /* dpprep output.  The azimuth will be recombined to 400 max    */ 
        max_num_radials = 800;
        if(VERBOSE) 
            fprintf(stderr,"Setting num radials to %d and radial size to %d\n",
                                                 max_num_radials, max_num_bins);
    } else {
        buffer = (short*)RPGC_get_outbuf_by_name("RADREFL", BUFSIZE, &opstatus);
        max_num_bins = BASEDATA_REF_SIZE; /* 460 */
        if(VERBOSE) 
            fprintf(stderr,"Using Legacy num radials and radial size\n"); 
        
    }
    
    
    
    /* check error condition of buffer allocation. abort if abnormal    */
    if(opstatus != NORMAL) {
        RPGC_log_msg( GL_INFO,
            "ERROR: Aborting from RPGC_get_outbuf...opstatus=%d\n",
                                                          opstatus);
        RPGC_abort(); 
        continue;
        }
    if (VERBOSE) {
      fprintf(stderr, "-> successfully obtained output buffer\n");  
    }      
    /* make sure that the buffer space is initialized to zero           */
    clear_buffer((char*)buffer, BUFSIZE);

   /* LABEL:READ_FIRST_RAD Read first radial of the elevation and      */
   /* accomplish the required moments check                            */
    /* ingest one radial of data from the BASEDATA linear buffer. The */
    /* data will be accessed via the basedataPtr pointer              */
    
    if (VERBOSE) {
      fprintf(stderr, "-> reading first radial buffer\n"); 
    }
    /* Sample Alg 1.21 */
    basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("SAMPLE2IN",
                                                                  &opstatus);

    hdr = (Base_data_header *) basedataPtr;
      
    if (VERBOSE) {
        fprintf(stderr, "     opstatus = %d \n", opstatus);
    }
     
    /* check radial ingest status before continuing                   */
    if(opstatus != NORMAL){
        RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_get_inbuf\n");
        RPGC_rel_outbuf((void*)buffer, DESTROY);
        RPGC_abort();
        continue;
    } 

    if (VERBOSE) {
        fprintf(stderr, "-> successfully read first radial buffer\n"); 
    }
  
    /* test to see if the required moment (reflectivity) is enabled   */
    RPGC_what_moments((Base_data_header*)basedataPtr, &ref_enable,
                                         &vel_enable, &spw_enable);

    if(ref_enable != TRUE){
        RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_what_moments\n");
        RPGC_rel_inbuf((void*)basedataPtr);
        RPGC_rel_outbuf((void*)buffer, DESTROY);
        RPGC_abort_because(PGM_DISABLED_MOMENT); 
        continue;
    }
      
    if (VERBOSE) {
      fprintf(stderr, "-> required moments enabled\n");
    }

/* Sample Alg 1.21 */
/* BEGIN TEST SECTION */
/*    if(input_id == 307) {                         */
/*        test_read_dp_moment_params(basedataPtr);  */
/*    }                                             */
/* END TEST SECTION */

    if (VERBOSE) {
      fprintf(stderr, "-> begin ELEVATION PROCESSING SEGMENT\n");
    }
    /* ELEVATION PROCESSING SEGMENT. continue to ingest and process     */
    /* individual base radials until either a failure to read valid     */
    /* input data (a radial in correct sequence) or until after reading */
    /* and processing the last radial in the elevation                  */
    
    /* get the current elevation & index from the incoming data       */
    elev_idx = (short)RPGC_get_buffer_elev_index( (void *)basedataPtr );
      
    /* get the current volume number using the following system call  */
    vol_num = RPGC_get_buffer_vol_num( (void *)basedataPtr );
      
    /* if not ingesting base data, RPGCS_get_target_elev_ang must be used */
    elevation = basedataPtr->hdr.target_elev;  /* target elevation */
    
    /* determine the number of bins to process here      */
    /* limit number of bins to the 70.000 ft MSL ceiling */
    bins_to_70k = RPGC_bins_to_ceiling( basedataPtr, hdr->surv_bin_size );



    if(bins_to_70k < max_num_bins)
        bins_to_process = bins_to_70k;
    else
        bins_to_process = max_num_bins;
    /* Sample Alg 1.20 - */
    /* ensure even number of bins to prevent possible alignment issues */
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



    
    while(TRUE) {

      /* LABEL:PROCESS_RAD Here we process each radial individually     */
      /* any processing that can be accomplished without comparison     */
      /* between other radial can be accomplished here                  */
      
      radial_status = basedataPtr->hdr.status & 0xf;
      
      cal_const = basedataPtr->hdr.calib_const;       /* calibration const*/

/* DEBUG */
/*fprintf(stderr,"DEBUG at top of radial processing loop\n"); */
   
      /* for this algorithm...we'll copy the radial header and the      */
      /* REFLECTIVITY structure to the temporary storage array. This    */
      /* effectively accumulates radials into an entire elevation.  The */
      /* routine requires several steps. First, allocate enough memory  */
      /* to hold the radial header plus data. Then copy the header and  */
      /* radial data structure portion into the allocated space. The    */
      /* radial can then be accessed via the pointer in the radial array*/
      
      /* NOTE: IT IS NOT ALWAYS NECESSARY TO COPY THIS DATA.  IF ONLY   */
      /*       PROCESSING A SINGLE RADIAL, THE RADIAL COULD BE          */
      /*       PROCESSED IN THE INPUT BUFFER AND WRITTEN DIRECTLY TO AN */
      /*       OPEN OUTPUT BUFFER. THIS WOULD REDUCE MEMORY USAGE.      */


/* While all of the following work, Method 2 is typical for basic moments */
/* Method 3 must be used for advanced Dual Pol moments                    */
      /* Method 1 */
/*      surv_data = (short *) ((char *) basedataPtr + hdr->ref_offset);  */
/*        max_bins_to_copy = hdr->surv_range + hdr->n_surv_bins - 1;      */
                          /* '-1' because the indexes are 1-based */
                          /* we do not assume hdr->surv_range is always 1 */
                          
      /* Method 2 */
      surv_data = (short *) RPGC_get_surv_data( (void *)basedataPtr, 
                                  &index_first_bin, &index_last_bin);
      max_bins_to_copy = index_last_bin + 1; 
                      /* '+1'because indexes are 0-based */
                      /* we do not assume that index_first_bin is always 0 */

      /* Method 3 */
/*      surv_data = (short *) RPGC_get_radar_data( (void *)basedataPtr,  */
/*                                   RPGC_DREF, my_gen_moment );         */
/*        max_bins_to_copy = hdr->surv_range + hdr->n_surv_bins - 1;      */

      /* test for NULL moment pointer and 0 offset */
      if(surv_data == NULL) {
         RPGC_log_msg( GL_INFO, "ERROR: NULL radial detected: %d\n",
            rad_idx);
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)buffer, DESTROY);
         RPGC_abort_because(PGM_INPUT_DATA_ERROR);
         opstatus = TERMINATE;
         break;
      }
      
      if(hdr->ref_offset == 0) {
         RPGC_log_msg( GL_INFO, "ERROR: reflectivity offset 0: %d\n",
            rad_idx);
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)buffer, DESTROY);
         RPGC_abort_because(PGM_INPUT_DATA_ERROR);
         opstatus = TERMINATE;
         break;
      }
      
      /* allocate one redials worth of memory                          */
      radial[rad_idx] = (char *)malloc(sizeof(Base_data_header) +
                                                        (bins_to_process * 2) );
      /* NOTE: If registered for SR_ data and reading from an SR   */
      /*       elevation, must use MAX_BASEDATA_REF_SIZE           */


      if(radial[rad_idx] == NULL) {
         RPGC_log_msg( GL_INFO, "MALLOC Error: malloc failed on radial %d\n",
            rad_idx);
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)buffer, DESTROY);
         RPGC_abort_because(PGM_MEM_LOADSHED);
         opstatus = TERMINATE;
         break;
      }
      
      /* copy the radial header first                                   */
      memcpy(radial[rad_idx], basedataPtr, sizeof(Base_data_header));
      
      /* copy the reflectivity data second                              */
      /* Sample Alg 1.20 - only copy the the lessor of (a) number of bins in  */
      /*                   the radial or (b) bins_to_process                  */
      if(bins_to_process <= max_bins_to_copy)
         bins_to_copy = bins_to_process;
      else 
         bins_to_copy = max_bins_to_copy;
      /* when writing the output product, any data bins */
      /* beyond max_bins_to_copy are set to '0'         */
      /* Sample Alg 1.20 - used bins_to_copy instead of bins_to_process */
      memcpy(radial[rad_idx] + sizeof(Base_data_header),
                       (char *)surv_data, (bins_to_copy * 2));
      
      short_data = (short *) ( radial[rad_idx] + sizeof(Base_data_header) );
      for(i = 0; i < bins_to_process; i++)
         /* ensure any data bins beyond the last good bin are set to 0 */
         if( i >= bins_to_copy ) 
             short_data[i] = (short) 0;
         
             /* NOTE: If registered for SR_ data and reading from an SR   */
             /*       elevation, must use MAX_BASEDATA_REF_SIZE           */


      /* test section: this optional block has been used to double check*/
      /* the integrity of the new radial memory block                   */
      if(TEST) {
         /* Sample Alg 1.21 - modified test from 363 to 357 for ORDA */
         if ( (rad_idx <= 3) || (rad_idx >= 357) ) {
            short test_status;
            Base_data_header *hdrtest=(Base_data_header*)radial[rad_idx];
            fprintf(stderr,"  RADIAL CHECK FOR RADIAL %d\n", rad_idx+1);
            fprintf(stderr,"    Azimuth=%f\n",hdrtest->azimuth);
            fprintf(stderr,"    Start Angle=%hd\n",hdrtest->start_angle);
            fprintf(stderr,"    Angle Delta=%hd\n",hdrtest->delta_angle);
            fprintf(stderr,"    calib const=%f\n",cal_const);
            /* read the radial status flag  */
            test_status = hdrtest->status & 0xf;
            fprintf(stderr, "    Radial_Status = %d where\n"
                            "    0=beg_ele, 1=int, 2=end_ele, 3=beg_vol, 4=end_vol,\n"
                            "    8=pseudo_end_ele, 9=pseudo_end_vol\n", 
                                                                    test_status);
         } /* end if rad_idx */
         
      } /* end if TEST */
      

      /* now that the data that we want from the radial is in temporary */
      /* storage...release the input buffer                             */
      RPGC_rel_inbuf((void*)basedataPtr);

      /* increment the radial counter                                   */
      rad_idx++; 

/* DEBUG */
/*fprintf(stderr,"DEBUG finished copying to radial[%d]\n", rad_idx-1); */


      /* check for radial overflow in the input array                   */
      /* this should never happen if we define max_num_radials correctly*/
      /* and the ORPG has not been changed (pbd algorithm)              */
      if(rad_idx > max_num_radials) {
          RPGC_log_msg( GL_INFO,
               "Count Error: radial index %d exceeded array limits %d\n", 
                                                     rad_idx, max_num_radials);
          RPGC_rel_outbuf((void*)buffer, DESTROY);
          RPGC_abort_because(PGM_INPUT_DATA_ERROR);
          opstatus = TERMINATE;
          break;
      }

      /* if end of elevation or volume then exit loop.                 */
      /* Sample Alg 1.20 - modified to exit radial processing at       */
      /*                   pseudo end rather than actual end in        */
      /*                   order to avoid overlapping radials if       */
      /*                   ingesting historical data before ORDA       */
      /*      This requires subsequently reading to the actual end     */
      /*      of elevation                                             */
      if(radial_status == GENDEL || radial_status == GENDVOL || 
         radial_status == PGENDEL || radial_status == PGENDVOL ) {
          if (VERBOSE)
               fprintf(stderr,"-> End of elevation found\n");
          /* exit with opstatus==NORMAL and no ABORT */
          break;
      }

      /* LABEL:READ_NEXT_RAD Read the next radial of the elevation      */
      /* ingest one radial of data from the BASEDATA linear buffer. The */
      /* data will be accessed via the basedataPtr pointer              */
      

      /* Sample Alg 1.21 */
      basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("SAMPLE2IN",
                                                                  &opstatus);
                                                                  
      hdr = (Base_data_header *) basedataPtr;
      
      /* check radial ingest status before continuing                   */
      /* check radial ingest status before continuing                   */
      if(opstatus != NORMAL){
          RPGC_log_msg( GL_INFO, "ERROR: Aborting from RPGC_get_inbuf\n");
          RPGC_rel_outbuf((void*)buffer, DESTROY);        
          RPGC_abort();        
          break;
      }

    } /* end while TRUE, reading radials */
    
    /* Sample Alg 1.21 */
    last_rad = rad_idx;

    if (VERBOSE) {
      fprintf(stderr, "=> out of radial ingest loop...radial  = %d\n", rad_idx+1);
      fprintf(stderr, "   Radial Status = %d  where\n"
                      "    0=beg_ele, 1=int, 2=end_ele, 3=beg_vol, 4=end_vol,\n"
                      "    8=pseudo_end_ele, 9=pseudo_end_vol\n",  
                                                             radial_status);
    }

    /* LABEL:PROCESS_ELEV Here we accomplish any processing of the      */
    if(opstatus == NORMAL) {
        
         /* Sample Alg 1.20 - may have exited on pseudo end, now must read */
         /*                  until the actual end of elevation / vol       */
         while(TRUE) { /* read remaining radials if any */
            if( radial_status != GENDEL && radial_status != GENDVOL ) {
               if (TEST)
                  fprintf(stderr, "=> exited on pseudo, looking for actual end\n");
               /* Sample Alg 1.21 */
               basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name("SAMPLE2IN",
                                                                        &opstatus);
               if(opstatus != NORMAL){
                  RPGC_log_msg( GL_INFO,
                    "ERROR: Aborting from RPGC_get_inbuf, opstatus=%d\n",
                                                                  opstatus);
                  RPGC_rel_outbuf((void*)buffer, DESTROY);
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
                /* Sample Alg 1.22a */
                } /* end if TEST */
               /* exit with opstatus==NORMAL and no ABORT */
               break;
            }
            
         } /* end while */
        
        
        /* elevation as a whole.  Processing that requires comparisons      */
        /* between radials should be accomplished here                      */
        ;
        
    }
    /* the end of the ELEVATION PROCESSING SEGMENT of the algorithm     */



    /* LABEL:ASSEMBLE The assembly of the ICD formatted product         */
    /* if processing is normal then create the ICD output product       */
    if(opstatus == NORMAL) {
        int result;     /* function return value                        */
        int length=0;   /* accumulated product length                   */
        int max_refl=0; /* maximum reflectivity value                   */


      /* building the ICD formatted output product requires a few steps */
      /* step 1: build the product description block (pdb)              */
      /* step 2: build the symbology block & data layer                 */
      /* step 3: complete missing values in the pdb                     */
      /* step 4: build the message header block (mhb)                   */
      /* step 5: forward the completed product to the system            */

      output_id = RPGC_get_id_from_name( "RADREFL");

      /* step 1: build the product description block -uses a system call*/
      if (VERBOSE) {
        fprintf(stderr, "\nCreating the product description block now: vol=%d\n",
                                                                         vol_num);
      }
      RPGC_prod_desc_block((void*)buffer, output_id, vol_num);
      
      /* for testing diagnostics show the header information now        */
/*      if(TEST) {
 *         fprintf(stderr,"\n==> OUTPUT INITIAL MHB AND PDB\n");
 *         result=print_message_header((char*)buffer);
 *         result=print_pdb_header((char*)buffer);
 *      }
 */    
      /* step 2: build the symbology layer & RLE radial data packet.    */
      /* this routine returns both the overall length (thus far) of the */
      /* product and the maximum reflectivity of all radials            */
      if (VERBOSE) {
        fprintf(stderr,"begin building the symbology and AF1F layers\n");
      }
        
      /* Sample Alg 1.21 - changed rad_idx to last_rad */
      max_refl = build_symbology_layer(buffer, radial, last_rad, 
                                                 bins_to_process, &length);
      
      if (VERBOSE) {
        fprintf(stderr, "finish building the symbology and AF1F layers\n");
      }
      /* step 3: finish building the product description block by       */
      /* filling in certain values such as elevation index, maximum     */
      /* reflectivity, accumulated product length, etc                  */
      /* passing calibration constant as a single float */
      finish_pdb(buffer, elev_idx, elevation, cal_const, max_refl, length);

      /* generate the product message header (use system call) and input*/
      /* total accumulated product length minus 120 bytes for the MHB & PDB */
      result = RPGC_prod_hdr((void*)buffer, output_id, &length);

      if (VERBOSE) {
        fprintf(stderr,"-> completed product length=%d\n",length);
      }
      /*(this routine adds 120 bytes for the MHB & PDB to the      */
      /* "length" parameter prior to creating the final header)         */

      /* if the creation of the product has been a success              */
      if(result == 0) { /*success!!!*/
        int res;
        if(VERBOSE) {
           fprintf(stderr, "product header creation success...display header\n");
           res = print_message_header((char*)buffer);
           res = print_pdb_header((char*)buffer); 
           fprintf(stderr, "\nprinting of product header complete\n");
        }
        
        
        /* LABEL:OUTPUT_PROD   forward product and close buffer         */
        RPGC_rel_outbuf((void*)buffer, FORWARD);
        
      } else {  /* product failure!  (destroy the buffer & contents)      */
        RPGC_log_msg( GL_INFO, "product header creation failure. buffer abort\n");
        RPGC_rel_outbuf((void*)buffer, DESTROY);
        RPGC_abort_because(PGM_PROD_NOT_GENERATED);
      } /* end result !=0 */

    }  /* end of if(opstatus==NORMAL) block */
    
    
    /* LABEL:CLEANUP   section to reset variables / free memory         */    
    /* free each previously allocated radial array                      */
    
    /* Sample Alg 1.21 - changed rad_idx to last_rad */
    if(TEST) fprintf(stderr, "free radial array (last_rad=%i)\n", last_rad);
    for(i = 0; i < last_rad; i++) {
        if(radial[i]!=NULL)
            free(radial[i]);
    }

    rad_idx = 0; /* reset radial counter to 0                               */
    last_rad = 0;

    RPGC_log_msg( GL_INFO,"process reset: ready to start new elevation\n");
    
    } /* end while PROCESS == TRUE */

   fprintf(stderr, "\nRadial Reflectivity Program Complete\n");
   
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
void clear_buffer(char *buffer,int value) {
  /* zero out the input buffer */
  int i;

  for(i = 0; i < value; i++)
    buffer[i] = 0;

  return;
  
}
