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
Module:         sample1_digrefl.c

Description:    digital reflectivity is a simple, sample algorithm for
                the ORPG. The algorithm has been implemented using
                ANSI-C interfaces and source code.
                
                This program can be used as a template to build more
                complex algorithms and includes a significant amount of 
                in-line commenting to describe each step. 
                
                The structure of this algorithm complies with the guidance
                provided in the CODE Guide Vol 3.
                
                digital_reflectivity ingests radials of base data and 
                creates an "ICD-formatted" digital radial data array 
                product using reflectivity data.
                
                This algorithm also includes "TEST" sections not contained
                in operational algorithms.
                
                Key source files for this algorithm include:

                sample1_digrefl.c       the algorithm driver files
                sample1_digrefl.h
        
                s2_print_diagnostics.c  source to display message header
                s2_print_diagnostics.h  information (DEBUG output)
        
                s1_symb_layer.c         source to create the symbology
                s1_symb_layer.h         and data packet layers
                

Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.11, April 2005   T. Ganger
              Modified to demo new DEA adaptation data using
                  sample_dig.alg as test adaptation data.
                  
Version 2.0   March 2006   T. Ganger
              Revised to use RPGP_set_packet_16_radial to avoid byte-swap
              Replaced stderr messages outside of test/debug sections with
                   RPGC_log_msg() calls.
              Revised to use current guidance for abort functions and 
                   abort reason codes.
              External product_to_disk function added.
              
Version 2.1   June 2006   T. Ganger
              Revised to use RPGC_reg_io
              Revised to demonstrate the multiple task_name for an executable 
                   name feature introduced in Build 9.                  
              Revised to use the new 'by_name' get_inbuf/get_outbuf functions
              
Version 2.2   July 2006   T. Ganger
              Modified to use the radial header offset to surveillance
                  data 'ref_offset'.  Tested RPGC_get_surv_data.
              Only malloc space for the header and surveillance data rather 
                   than the complete radial
              Replaced WAIT_ALL with WAIT_DRIVING_INPUT.

Version 2.3   March 2007   T. Ganger  
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.
              Added note that MAX_BASEDATA_REF_SIZE must be used instead of
                  BASEDATA_REF_SIZE for super resolution elevations.
              RPGC_get_radar_data failed in this algorithm for Build 9.

Version 2.4   June 2007   T. Ganger  
              Added and changed in-line comments for clarification
                  
Version 2.5   June 2007   T. Ganger  
              For Build 10, removed reading discontinued VCP structure 
                  fields.  Several field were temporary (actually spares)
              For Build 10, changed REFL_RAWDATA to RECOMBINED_REFL_RAWDATA
              
Version 2.6   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated use of defined output buffer ids by using
                  RPGC_get_id_from_name
              Moved API test from main to separate function.
              Corrected the contents of dependent parameter 4 from maximum 
                  reflectivity data level to maximum value in dBZ
              Reduced the size of the internal elevation data array by 
                  using an array of bytes instead of shorts.
              Modified to handle variable array sizes and specifically to
                  handle super resolution input and produce super resolution
                  output. 
              When copying each radial, demonstrated ensuring that data 
                  values beyond the last good data bin (if any) are set to '0'.
              Used the new default abort reason code PGM_PROD_NOT_GENERATED.
              
Version 2.7   June 2008    T. Ganger  (Sample Algorithm ver 1.19)
              Modified to include the new parameter to the function
                  RPGC_is_buffer_from_last_elev.
              Eliminated Red Hat 5 compile warnings.
              
Version 2.8   Movember 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Added test to ensure that the algorithm did not attempt to
                  read or copy more bins than are actually in each 
                  base data radial.  Depending upon the method used to read 
                  the radial, a specific index is tested.
              
Version 2.9   February 2009    T. Ganger  (Sample Algorithm ver 1.21)
              Modified test section printing out radial information 
                  including status to reflect ORDA usually having 720
                  super resolution radials.
              Added test section to print dual pol data parameters.
              
$Id$
************************************************************************/

/* see the following include file for descriptions of constants for
for expected ICD block sizes */
#include "sample1_digrefl.h"

/* GLOBAL TEST DEFINES */

int DEBUG=FALSE;           /* test flag: set to true for diag messages */

int TEST_API=FALSE;

int TEST_ADAPT=FALSE;




/* prototype for function which tests several api functions */
void sample1_test_api(Base_data_radial *bdr_ptr, char *output_name, int output_id);


/* variables for new adaptation data scheme */
typedef struct {
  short  my_element_1;
  double my_element_2;
  int    my_element_3;
} sample1_adapt_t;

sample1_adapt_t sample1_adapt;   
                      
/* prototype for adaptation callback function for new scheme */
/* OK to use if not registered as a callback */
/* int read_my_adapt();  */

/* REQUIRED for use as a callback */
int read_my_adapt_fx(void *struct_address); 


/****************************************************************************/
/****************************************************************************/


/************************************************************************
Description:    main function to drive the digital radial data array
                algorithm
Input:          one radial at a time as defined in the Base_data_radial
                structure (see the basedata.h include file)
Output:         a formatted ICD graphical product which is forwarded to
                the ORPG for distribution
Returns:        none
Globals:        none
Notes:          1). This algorithm is used to demonstrate and test
                API functions and DEA adaptation data.  It also provides
                an example of basic algorithm structure.
                2). This algorithm provides a demonstration of having
                two different task names for the executable file name.  
                The command line arguments determine the task name
                3). Modified to eliminate a hard coded radial size of
                230. 
                4).Modified for Super Res data input.
                
************************************************************************/

int main(int argc,char* argv[]) {
  /* algorithm variable declarations and definitions ------------------ */
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
  short elev_idx=0;         /* variable: used to hold current elev index*/
  short elevation=0;        /* variable: used to hold the value of elev */
  int vol_num=0;            /* current volume number                    */
  int opstatus;             /* variable:  used to hold return values    */
                            /* from API system calls                    */
  char *out_buffer;         /* pointer: access to allocated memory to   */
                            /* hold a completed ICD product             */

  short radial_status=0;    /* variable: status of each radial input    */

  /* array of pointers to radial structures   */
  char *radial[BASEDATA_MAX_SR_RADIALS];

  int bins_to_process = 0;  /* number of data bins to process in radial */
  int bins_to_70k;
  int max_num_bins = BASEDATA_REF_SIZE;  /* 460 */
                            /* could be reduced to limit range of product */
                            /* and is also limited by 70,000 ft MSL       */
  int max_num_radials = BASEDATA_MAX_RADIALS;  /* 400 */
  int new_buffer_size;

  unsigned char *rad_data=NULL;

/* Sample Alg 1.20 - make sure we do not read more than is in a radial */
  int max_bins_to_copy;
  int bins_to_copy;

  short *surv_data=NULL;   /* pointer to the surveillance data in the radial */
  int index_first_bin=999;  /* used with method 2 */
  int index_last_bin=999;   /* used with method 2 */
  
  Generic_moment_t gen_moment;  /* used with method 3 */
  /* cannot pass a NULL pointer to RPGC_get_radar_data */
  Generic_moment_t *my_gen_moment = &gen_moment;  /* method 3 */

  
  int rc = 0;               /* return code from function call           */
  
/* structure for adaptation data */
  Siteadp_adpt_t site_adapt;


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  */
/*     variables to hold the different data names for the two 
 *     defined task names.  See task_attr_table.sample_snippet  
 */
  int OUTPUTDATA; 
  char INDATA_NAME[65];
  char OUTDATA_NAME[65];
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

  char argument[65]; 


/* test / debug variable declarations and definitions ------------------*/
  int test_out=FALSE;      /* Boolean to control diagnostic file output */  
   
  int VERBOSE=FALSE;        /* verbose processing output                */

/* end test / debug variables and definitions  ------------------------ */


  hdr = NULL;
                               
  fprintf(stderr, "\nBegin Digital Reflectivity Algorithm in C\n");

  /* LABEL:REG_INIT  Algorithm Registration and Initialization Section  */
  
  RPGC_init_log_services(argc, argv);
  

  /* register inputs and outputs based upon the            */
  /* contents of the task_attr_table (TAT).                */
  /* for task sample1_base: input:SR_REFLDATA(78), output:SR_DIGREFLBASE(1990)*/
  /* for task sample1_raw: input:REFL_RAWDATA(54),                            */
  /*                       output:SR_DIGREFLRAW(1995)                         */
  RPGC_reg_io(argc, argv);  
  
  /* register algorithm infrastructure to read the Scan Summary Array   */
  RPGC_reg_scan_summary();


  /* register site adaptation data */
  rc = RPGC_reg_site_info( &site_adapt );
  if ( rc < 0 )
  {
    RPGC_log_msg( GL_INFO, 
         "SITE INFO: cannot register adaptation data callback function\n");
  }

  /* register adaptation data callback (some ported algorithm use old method) */
  /* supported timing types are ADPT_UPDATE_BOV (beginning of volume), */ 
  /* ADPT_UPDATE_BOE (beginning of elevation), ADPT_UPDATE_ON_CHANGE,  */
  /* ADPT_UPDATE_WITH_CALL, and ADPT_UPDATE_ON_EVENT (not tested)      */

  /* Register algorithm adaptation data */
  rc = RPGC_reg_ade_callback( read_my_adapt_fx,
                              &sample1_adapt,
                              "alg.sample1_dig.",
                              ADPT_UPDATE_BOV );
  if ( rc < 0 )
  {
    RPGC_log_msg( GL_INFO, 
         "SAMPLE1: cannot register adaptation data callback function\n");
  }


  /* ORPG task initialization routine. Input parameters argc/argv are   */
  /* not used in this algorithm                                         */
  RPGC_task_init(ELEVATION_BASED, argc, argv);




/*   +++++ demo multiple task names +++++++++++++++++++++++++++++++++   */
    /* This section of code demonstrates having multiple 
     * task names with one file name.  This algorithm has different
     * input and output buffers for two different task names
     */
     
    /* get my task name so I can request the correct buffers     */
        /* because the function RPGC_reg_custom_options() can   */
        /* not be used to get the task name from the -T option,  */
        /* we have to manually parse the command line for "-T"  */
    for( i = 0; i < argc; i++ ) {
        strcpy(argument, argv[i]);
        if(strcmp( argument, "-T") == 0) {
            strcpy(argument, argv[i+1]);
            break;
        }
    } /* end for i < argc */

    /* New 'by_name' functions use the registration name     */
    strcpy(INDATA_NAME, "SR_REFLDATA");
    strcpy(OUTDATA_NAME, "SR_DIGREFLBASE");
    OUTPUTDATA = RPGC_get_id_from_name( "SR_DIGREFLBASE");

    fprintf(stderr, "Sample 1 task_name is '%s'\n", argument);
    if(strcmp(argument, "sample1_base") == 0) {
        strcpy(INDATA_NAME, "SR_REFLDATA");
        strcpy(OUTDATA_NAME, "SR_DIGREFLBASE");
        OUTPUTDATA = RPGC_get_id_from_name( "SR_DIGREFLBASE");
    } else if(strcmp(argument, "sample1_raw") == 0) {
        strcpy(INDATA_NAME, "REFL_RAWDATA");
        strcpy(OUTDATA_NAME, "SR_DIGREFLRAW");
        OUTPUTDATA = RPGC_get_id_from_name( "SR_DIGREFLRAW");
    }
    fprintf(stderr, "-> Input is '%s', output is '%s' \n", 
                                 INDATA_NAME, OUTDATA_NAME);
/* +++++ end code section demo multiple task names ++++++++++++++++++++   */



  fprintf(stderr, "-> algorithm initialized and proceeding to loop control\n");
  
  RPGC_log_msg( GL_INFO,
       "-> algorithm initialized and proceeding to loop control\n");


  if(TEST_ADAPT)
  {
      /* New Site Info */
      fprintf(stderr, "->ADAPTATION DATA RDA elevation = %d\n", 
                                           site_adapt.rda_elev); 
      fprintf(stderr, "->ADAPTATION DATA RPG id = %d\n", 
                                      site_adapt.rpg_id); 
      fprintf(stderr, "->ADAPTATION DATA RPG name = %s\n", 
                                      site_adapt.rpg_name);  
  }




  /* while loop that controls how long the task will execute. As long as*/
  /* PROCESS remains true, the task will continue. The task will        */
  /* terminate upon an error condition                                  */
  while(PROCESS) {

    /* system call to indicate a data driven algorithm. block algorithm */
    /* until good data is received the product is requested             */
    RPGC_wait_act(WAIT_DRIVING_INPUT);

  /* LABEL:BEGIN_PROCESSING Released from Algorithm Flow Control Loop   */
  if (VERBOSE)
       fprintf(stderr, "-> algorithm passed loop control and begin processing\n");
  
  RPGC_log_msg( GL_INFO,
       "-> algorithm passed loop control and begin processing\n");



/* ************ ADAPTATION DATA TEST SECTION *********** */
  /* print out the contents of adaptation data for demonstration purposes */
  if(TEST_ADAPT)
  {   
  /* test reading new DEA adaptation data elements */
  /* without the callback registration, would need */
  /* to manually read adaptation data              */
  /*      read_my_adapt() */
  /*      read_my_adapt_fx(&sample1_adapt); */
    
      fprintf(stderr, "->DEA Test Element 1 Data = %d\n", 
                               sample1_adapt.my_element_1); 
      fprintf(stderr, "->DEA Test Element 2 Data = %f\n", 
                               sample1_adapt.my_element_2); 
      fprintf(stderr, "->DEA Test Element 3 Data = %d\n", 
                               sample1_adapt.my_element_3);  
  }
/* ********** END ADAPTATION DATA TEST SECTION ********** */


    /* allocate a partition (accessed by the pointer, out_buffer) within the*/
    /* SR_DIGREFLBASE / SR_DIGREFLRAW linear buffer. error return in opstatus */

    out_buffer = (char *)RPGC_get_outbuf_by_name(OUTDATA_NAME, BUFSIZE, 
                                                               &opstatus);
    
    /* check error condition of buffer allocation. abort if abnormal    */
    if(opstatus != NORMAL) {
        RPGC_log_msg( GL_INFO,
             "ERROR: Aborting from RPGC_get_outbuf...opstatus=%d\n", opstatus);
        RPGC_abort(); 
        continue;
    }

    if (VERBOSE) {
      fprintf(stderr, "-> successfully obtained output buffer\n");  
    }  
    /* make sure that the buffer space is initialized to zero         */
    clear_buffer(out_buffer);
    
    /* LABEL:READ_FIRST_RAD Read first radial of the elevation and    */
    /* accomplish the required moments check                          */
    /* ingest one radial of data from the BASEDATA / RAWDATA linear   */
    /* buffer. The data will be accessed via the basedataPtr pointer  */

    if (VERBOSE) {    
      fprintf(stderr, "-> reading first radial buffer\n"); 
    }
 

    basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name(INDATA_NAME, 
                                                                 &opstatus);
    hdr = (Base_data_header *) basedataPtr;

    if (VERBOSE) {
       fprintf(stderr, "     opstatus = %d \n", opstatus);
    }
    /* check radial ingest status before continuing                   */
    if(opstatus != NORMAL){
        RPGC_log_msg(GL_INFO, "ERROR: Aborting from RPGC_get_inbuf\n");
        RPGC_rel_outbuf((void*)out_buffer,DESTROY);  
        RPGC_abort(); 
        continue;
    }

    if (VERBOSE) {
      fprintf(stderr, "-> successfully read first radial buffer\n");
    } 
  
    /* test to see if the required moment (reflectivity) is enabled   */
    /* NOTE: the other method, using RPGC_reg_moments, should not be  */
    /*       used in this algorithm because one of the configured     */
    /*       task names uses RAWDATA                                   */
    RPGC_what_moments((Base_data_header*)basedataPtr, &ref_enable,
                                         &vel_enable, &spw_enable);

    if(ref_enable != TRUE) {
        RPGC_log_msg(GL_INFO, "ERROR: Aborting from RPGC_what_moments\n");
        RPGC_rel_inbuf((void*)basedataPtr);
        RPGC_rel_outbuf((void*)out_buffer, DESTROY);
        RPGC_abort_because(PGM_DISABLED_MOMENT); 
        continue;
    }

    if (VERBOSE) {
      fprintf(stderr, "-> required moments enabled\n");
    }
     

/* ********** API TEST SECTION ********************************* */
/* DEMONSTRATION OF USING VARIOUS HELPER FUNCTIONS this function */
/* an the end of this file demonstrates use of functions that    */
/* that obtain information about the current volume / elevation. */
  if (TEST_API) {
    sample1_test_api( basedataPtr, OUTDATA_NAME, OUTPUTDATA);
  }
/* ********** END API TEST SECTION ***************************** */


/* Sample Alg 1.21 */
/* BEGIN TEST SECTION */
/*    test_read_dp_moment_params(basedataPtr); */

/* END TEST SECTION */

     
    if (VERBOSE) {
      fprintf(stderr, "-> begin ELEVATION PROCESSING SEGMENT\n");
    }
    /* ELEVATION PROCESSING SEGMENT. continue to ingest and process     */
    /* individual base radials until either a failure to read valid     */
    /* input data (a radial in correct sequence) or until after reading */
    /* and processing the last radial in the elevation                  */
    
    /* get the current elevation angle & index from the incoming data */
    elev_idx = (short)RPGC_get_buffer_elev_index((void *)basedataPtr); 
    
    /* get the current volume number using the following system call  */
    vol_num = RPGC_get_buffer_vol_num((void *)basedataPtr);
    
    /* if not ingesting base data, RPGCS_get_target_elev_ang must be used */
    elevation = hdr->target_elev; 
    

/* ***************** begin handle Super Res *******************************/

    /* since this algorithm currently only produces high resolution products   */
    /* from elevations having both 0.5 deg radials and 250 m reflectivity      */
    /* we shortcut the algorithm here if the elevations cut is not appropriate */
    if( hdr->azm_reso != 1 || hdr->surv_bin_size != 250 ) {
       if(VERBOSE) fprintf(stderr,"Product Not Generated, not a SR Elevation\n"); 
       RPGC_log_msg(GL_INFO,
                    "Aborting: cut does not contain required resolution\n");
       RPGC_rel_inbuf((void*)basedataPtr);
       RPGC_rel_outbuf((void*)out_buffer, DESTROY);
       RPGC_abort_because(PGM_PROD_NOT_GENERATED);      
       continue;
   }


    /* NOTE: using hdr->surv_bin_size for this test was chosen over */
    /* using hdr->n_surv_bins.  However, if reading velocity or     */
    /* spectrum width data, hdr->n_dop_bins would be used to set    */
    /* the maximum size of the radial.                              */
    if(hdr->surv_bin_size == 250) { 
/*    if(hdr->n_surv_bins > 460) { */
        max_num_bins = MAX_BASEDATA_REF_SIZE;
        if(DEBUG) fprintf(stderr,"Detected Hi Res 250 meter bin \n");
    } else if(hdr->surv_bin_size == 1000) { 
/*    } else if(hdr->n_surv_bins <= 460) { */
        max_num_bins = BASEDATA_REF_SIZE;
    } else {
        RPGC_log_msg(GL_INFO, "ERROR: Bad surveillance bin size %d\n", 
                                                   hdr->surv_bin_size);
        RPGC_rel_inbuf((void*)basedataPtr);
        RPGC_rel_outbuf((void*)out_buffer, DESTROY);
        RPGC_abort_because(PGM_INPUT_DATA_ERROR);      
        continue;
    }

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


    if(hdr->azm_reso == 1) {
        max_num_radials = BASEDATA_MAX_SR_RADIALS;
        if(DEBUG) fprintf(stderr, "Detected Super Res Radial (0.5 degree) \n");
    } else if(hdr->azm_reso == 2) {
        max_num_radials = BASEDATA_MAX_RADIALS;
    }
    

    /* calculate max buffer size and realloc out buffer if required */
    if(max_num_radials == BASEDATA_MAX_SR_RADIALS || 
        max_num_bins == MAX_BASEDATA_REF_SIZE ) { /*need to resize output buffer*/
            new_buffer_size = (BASEDATA_MAX_SR_RADIALS * MAX_BASEDATA_REF_SIZE) +
                               sizeof(Graphic_product) + sizeof(Symbology_block) +
                               sizeof(Packet_16_hdr_t);

        out_buffer = RPGC_realloc_outbuf( (void *)out_buffer, new_buffer_size, 
                                                                      &opstatus );
        if(opstatus != NORMAL){
            RPGC_log_msg(GL_INFO, "ERROR: Unable to realloc out buffer\n");
            RPGC_rel_inbuf((void*)basedataPtr);
            RPGC_rel_outbuf((void*)out_buffer, DESTROY);
            RPGC_abort(); 
            continue;
        }
        
    } /* end calculate max size and realloc */
    
/* ***************** End handle Super Res *******************************/



    while(TRUE) {
    
      /* LABEL:PROCESS_RAD Here we process each radial individually     */
      /* any processing that can be accomplished without comparison     */
      /* between other radial can be accomplished here                  */
      
      radial_status = basedataPtr->hdr.status & 0xf;
      
      
      
      /* redundant test information for quality control                 */
      if (DEBUG) {
         /* Sample Alg 1.21 - modified test from 362 to 717 for ORDA SR */
         if ( (rad_idx <= 3) || (rad_idx >= 717) ) {
            fprintf(stderr,"  RADIAL CHECK FOR RADIAL %d\n", rad_idx+1);
            fprintf(stderr,"    Azimuth=%f\n",hdr->azimuth);
            fprintf(stderr,"    Start Angle=%hd\n",hdr->start_angle);
            fprintf(stderr,"    Angle Delta=%hd\n",hdr->delta_angle);
            /* Sample Alg 1.21 - read the radial status flag  */
            fprintf(stderr, "    Radial_Status = %d where\n"
                            "    0=beg_ele, 1=int, 2=end_ele, 3=beg_vol, 4=end_vol,\n"
                            "    8=pseudo_end_ele, 9=pseudo_end_vol\n", 
                                                                    radial_status);
         } /* end if rad_idx */

      } /* end DEBUG */



      /* for this algorithm...we'll copy the radial header and the      */
      /* reflectivity structure to the temporary storage array. This    */
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
/*      surv_data = (short *) ((char *) basedataPtr + hdr->ref_offset); */
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
/*      surv_data = (short *) RPGC_get_radar_data( (void *)basedataPtr, */
/*                                   RPGC_DREF, my_gen_moment );        */
/*        max_bins_to_copy = hdr->surv_range + hdr->n_surv_bins - 1;      */

      /* test for NULL moment pointer and 0 offset */
      if(surv_data == NULL) {
         RPGC_log_msg( GL_INFO,"ERROR: NULL radial detected: %d\n",
            rad_idx);
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)out_buffer, DESTROY);
         RPGC_abort_because(PGM_INPUT_DATA_ERROR);
         opstatus = TERMINATE;
         break;
      }
      
      if(hdr->ref_offset == 0) {
         RPGC_log_msg( GL_INFO, "ERROR: reflectivity offset 0: %d\n",
            rad_idx);
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)out_buffer, DESTROY);
         RPGC_abort_because(PGM_INPUT_DATA_ERROR);
         opstatus = TERMINATE;
         break;
      }

      /* allocate one radial's worth of memory  */
      /* used an internal radial of bytes rather than shorts */
      radial[rad_idx] = (char *)malloc(sizeof(Base_data_header) + 
                                                   (bins_to_process)); 

      
      if(radial[rad_idx] == NULL) {
         RPGC_log_msg( GL_INFO, "MALLOC Error: malloc failed on radial %d\n",
            rad_idx);
         RPGC_rel_inbuf((void*)basedataPtr);
         RPGC_rel_outbuf((void*)out_buffer, DESTROY);
         RPGC_abort_because(PGM_MEM_LOADSHED);
         opstatus = TERMINATE;
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
         
      /* Sample Alg 1.21 - added test of bins being copied */
      if(DEBUG) {
         /* Sample Alg 1.21 - modified test from 362 to 717 for ORDA SR */
         if ( (rad_idx <= 3) || (rad_idx >= 717) ) {
            fprintf(stderr,"  FOR RADIAL %d\n", rad_idx+1);
            fprintf(stderr,"copying %d bins\n", bins_to_copy);
         }
      } /* end DEBUG */
        
      /* when writing the output product, any data bins */
      /* beyond max_bins_to_copy are set to '0'         */
      rad_data = (unsigned char *) ( radial[rad_idx] + sizeof(Base_data_header) );
      for(i = 0; i < bins_to_process; i++)
         /* ensure any data bins beyond the last good bin are set to 0 */
         if( i < bins_to_copy ) 
             rad_data[i] = (unsigned char) surv_data[i];
         else
             rad_data[i] = (unsigned char) 0;


      /* now that the data that we want from the radial is in temporary */
      /* storage...release the input buffer                             */
      RPGC_rel_inbuf((void*)basedataPtr);

      /* increment the radial counter */
      rad_idx++; 
      
      /* check for radial overflow in the input array  */
      /* this should never happen unless the RDA fails */
      if(rad_idx > max_num_radials) {
          fprintf(stderr,
               "Count Error: radial index %d exceeded array limits %d\n", 
                                                     rad_idx, max_num_radials);
          RPGC_log_msg( GL_INFO,
               "Count Error: radial index %d exceeded array limits %d\n", 
                                                     rad_idx, max_num_radials);
          RPGC_rel_outbuf((void*)out_buffer, DESTROY);
          RPGC_abort_because(PGM_INPUT_DATA_ERROR);
          opstatus = TERMINATE;
          break;
      }

      /* if end of elevation or volume then exit loop */
      /* this is the ACTUAL end of elevation rather than Pseudo end     */
      /* NOTE: when reading historical data prior to the ORDA, this     */
      /*       results in the possiblilty of an overlapping radial      */
      /*       This could be avoided by terminating radial processing   */
      /*       at pseudo end of elevation / volume and then just        */
      /*       reading  basedata radial messages until actual end.  See */
      /*       sample algorithms 2 and 3 for an example of eliminatig   */
      /*       the possibility of overlapped radials                    */
      if(radial_status == GENDEL || radial_status == GENDVOL) {
          if (VERBOSE)
               fprintf(stderr, "-> End of elevation found\n");
          /* exit with opstatus==NORMAL and no ABORT */
          break;
      }
     
      /* LABEL:READ_NEXT_RAD Read the next radial of the elevation      */
      /* ingest one radial of data from the BASEDATA / RAWDATA linear   */
      /* buffer. The data will be accessed via the basedataPtr pointer  */
      
      

      basedataPtr = (Base_data_radial*)RPGC_get_inbuf_by_name(INDATA_NAME, 
                                                                 &opstatus);
      hdr = (Base_data_header *) basedataPtr;
      
      /* check radial ingest status before continuing                   */
      if(opstatus != NORMAL) {
          RPGC_log_msg( GL_INFO," ERROR reading input buffer\n");
          RPGC_rel_outbuf((void*)out_buffer, DESTROY);  
          RPGC_abort();
          break;
      }
 

    } /* end while TRUE, reading radials */



    if (VERBOSE) {
      fprintf(stderr,"=> out of radial ingest loop...elev count=%d\n",elev_idx);
    }
    
    /* LABEL:PROCESS_ELEV Here we accomplish any processing of the      */
    
    if(opstatus == NORMAL) {
        /* elevation as a whole.  Processing that requires comparisons  */
        /* between radials should be accomplished here                  */
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

      /* step 1: build the product description block -uses a system call*/
      if (VERBOSE) {
        fprintf(stderr,
           "\nCreating the product description block now: vol=%d\n", vol_num);
      }
      RPGC_prod_desc_block((void*)out_buffer, OUTPUTDATA, vol_num);
      

      /* step 2: build the symbology layer & digital radial data array. */
      /* this routine returns both the overall length (thus far) of the */
      /* product and the maximum reflectivity of all radials            */
      if (VERBOSE) {
        fprintf(stderr,"begin building the symbology and p16 layers\n");
      }

      max_refl = build_symbology_layer(out_buffer, radial, rad_idx, 
                                                   bins_to_process, &length);
      
      /* step 3: finish building the product description block by       */
      /* filling in certain values such as elevation index, maximum     */
      /* reflectivity, accumulated product length, etc                  */
      finish_pdb(out_buffer, elev_idx, elevation, max_refl, length);


      /* generate the product message header (use system call) and input*/
      /* total accumulated product length minus 120 bytes for the MHB & PDB */
      result = RPGC_prod_hdr((void*)out_buffer, OUTPUTDATA, &length);

      if (VERBOSE) {
        fprintf(stderr, "-> completed product length=%d\n", length);
      }

      /*(this routine adds 120 bytes for the MHB & PDB to the      */
      /* "length" parameter prior to creating the final header)         */

      
      /* if the creation of the product has been a success              */
      if(result == 0) { /*success*/
         /* print product header for testing purposes                    */
         int res;
         if (VERBOSE) {
             fprintf(stderr, "product header creation success...display header\n");
             res = print_message_header(out_buffer);
             res = print_pdb_header(out_buffer);
             fprintf(stderr, "\nprinting of product header complete\n");
         }
         
         
         
          /* diagnostic - interrupts product output and creates a 
           * binary output of product to file.
           * this is useful if product problems cause task failure
           */
         if(test_out == TRUE) {
             
             product_to_disk(out_buffer, length, "sample1_dig", elev_idx);
         
             RPGC_log_msg( GL_INFO,
                "Interrupted product output for diagnostic output to file.\n");
             RPGC_rel_outbuf( (void*)out_buffer, DESTROY );
          
         }    /* end test_out TRUE */
         else /* normal product output */
             
         /* LABEL:OUTPUT_PROD    forward product and close buffer        */
             RPGC_rel_outbuf((void*)out_buffer, FORWARD);


      } else {  /* product failure (destroy the buffer & contents)        */
         RPGC_log_msg( GL_INFO, "product header creation failure\n");
         RPGC_rel_outbuf((void*)out_buffer, DESTROY);
         RPGC_abort_because(PGM_PROD_NOT_GENERATED);
      } /* end result !=0 */

    }  /* end of if(opstatus==NORMAL) block */


    /* LABEL:CLEANUP   section to reset variables / free memory         */
    /* free each previously allocated radial array                      */
    if (DEBUG) {
        fprintf(stderr,"free radial array (rad_idx=%i)\n",rad_idx);
    }
    
    for(i = 0; i < rad_idx; i++) {
        if(radial[i]!=NULL)
            free(radial[i]);
    }

    rad_idx = 0; /* reset radial counter to 0                               */


    RPGC_log_msg( GL_INFO,"process reset: ready to start new elevation\n");
    
    } /* end while PROCESS == TRUE */

  fprintf(stderr,"\nProgram Terminated\n");
  
  return(0);
  
} /* end main */

/****************************************************************************/
/****************************************************************************/



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
    buffer[i] = 0;

  return;
  
}


/*****access function v1*************************************
 * adaptation data access function not used as a callback
 *      the structure may be a global rather than a parameter
 ************************************************************/
int read_my_adapt()
{
    
double get_value = 0.0;
int ret = -1;

  ret = RPGC_ade_get_values( "alg.sample1_dig.","db_Element_1", &get_value);
  if(ret == 0)
  {
    sample1_adapt.my_element_1 = (short)get_value;
  }
  else
  {
    sample1_adapt.my_element_1 = TRUE;
    RPGC_log_msg( GL_INFO, 
         "my_element_1 DEA value unavailable, using program default\n");
  }
  
  
  ret = RPGC_ade_get_values("alg.sample1_dig.", "db_Element_2", &get_value);
  if(ret == 0)
  {
    sample1_adapt.my_element_2 = (float)get_value;
  }
  else
  {
    sample1_adapt.my_element_2 = 10.0;
    RPGC_log_msg( GL_INFO, 
         "my_element_2 DEA value unavailable, using program default\n");
  }


  ret = RPGC_ade_get_values("alg.sample1_dig.", "db_Element_3", &get_value);
  if(ret == 0)
  {
    sample1_adapt.my_element_3 = (int)get_value;
  }
  else
  {
    sample1_adapt.my_element_3 = -1;
    RPGC_log_msg( GL_INFO, 
         "my_element_3 DEA value unavailable, using program default\n");
  }
  
  return ret;
    
} /* end read_my_adapt */

/******access function v2************************************
 * adaptation data access function used as a callback
 *      the address of the structure must be the parameter
 ************************************************************/
int read_my_adapt_fx( void *struct_address )
{

double get_value = 0.0;
int ret = -1;

sample1_adapt_t *adapt_ptr = (sample1_adapt_t *)struct_address;
  
  ret = RPGC_ade_get_values("alg.sample1_dig.", "db_Element_1", &get_value);
  if(ret == 0)
  {
    adapt_ptr->my_element_1 = (short)get_value;
  }
  else
  {
    adapt_ptr->my_element_1 = TRUE;
    RPGC_log_msg( GL_INFO, 
         "my_element_1 DEA value unavailable, using program default\n");
  }
  
  
  ret = RPGC_ade_get_values("alg.sample1_dig.", "db_Element_2", &get_value);
  if(ret == 0)
  {
    adapt_ptr->my_element_2 = (float)get_value;
  }
  else
  {
    adapt_ptr->my_element_2 = 10.0;
    RPGC_log_msg( GL_INFO, 
         "my_element_2 DEA value unavailable, using program default\n");
  }


  ret = RPGC_ade_get_values("alg.sample1_dig.", "db_Element_3", &get_value);
  if(ret == 0)
  {
    adapt_ptr->my_element_3 = (int)get_value;
  }
  else
  {
    adapt_ptr->my_element_3 = -1;
    RPGC_log_msg( GL_INFO, 
         "my_element_3 DEA value unavailable, using program default\n");
  }
  
  return ret;    
}



void sample1_test_api(Base_data_radial *bdr_ptr, char *output_name, int output_id)
{
  int test_vol_num;
  int test_elev_ind;
  int test_last_index;
  int test_vcp_num;
  Scan_Summary *test_scan_sum_ptr=NULL;
  int test_target_elev;
  int test_last_elev_index;
  int my_elev_index;
  int last_elev=99;
  int test_wxmode;
  Vcp_struct *test_vcp_ptr=NULL;
  int test_prod_code;
    
    if(bdr_ptr == NULL)
        return;
    
    test_vol_num = RPGC_get_buffer_vol_num((void *)bdr_ptr);
    fprintf(stderr, "API TEST  volume number is %d\n", test_vol_num);
    
    test_elev_ind = RPGC_get_buffer_elev_index((void *)bdr_ptr);
    fprintf(stderr, "API TEST  elevation index is %d \n", test_elev_ind);
    
    test_scan_sum_ptr = RPGC_get_scan_summary(test_vol_num);
    if(test_scan_sum_ptr != NULL) {
        fprintf(stderr, "API TEST  using the pointer to scan summary (%d)\n", 
                                                      (int)test_scan_sum_ptr);
        fprintf(stderr, "          VCP number: %d\n"
             "          number of rda cuts: %d, number or rpg cuts: %d\n",
             test_scan_sum_ptr->vcp_number, test_scan_sum_ptr->rda_elev_cuts, 
             test_scan_sum_ptr->rpg_elev_cuts);
    }
    
    test_vcp_num = RPGC_get_buffer_vcp_num((void *)bdr_ptr);
    fprintf(stderr,"API TEST  vcp number is %d \n", test_vcp_num);
    
    test_target_elev = RPGCS_get_target_elev_ang(test_vcp_num, test_elev_ind);
    fprintf(stderr,"API TEST  target elevation is %2.1f \n", 
                                                  (test_target_elev/10.0));
    
    test_last_elev_index = RPGCS_get_last_elev_index(test_vcp_num);
    fprintf(stderr, "API TEST  last elevation index is %d \n", 
                                         test_last_elev_index);
   
    last_elev = RPGC_is_buffer_from_last_elev( (void *)bdr_ptr,
                                     &my_elev_index, &test_last_index);
    if( last_elev == 1 )
        fprintf(stderr, "API TEST  buffer (elev %d) is last elev\n"
                        "          last index is: %d\n",
                                             my_elev_index, test_last_index);
    else if( last_elev == 0 )
        fprintf(stderr, "API TEST  buffer (elev %d) is NOT last elev\n"
                        "          last index is: %d\n",
                                               my_elev_index, test_last_index);
    else if( last_elev == -1 )
        fprintf(stderr, "API TEST  error in RPGC_is_buffer_from_last_elev\n");
    
    test_wxmode = RPGCS_get_wxmode_for_vcp(test_vcp_num);
    fprintf(stderr, "API TEST  weather mode is %d\n", test_wxmode);
                                                     
    test_vcp_ptr = RPGCS_get_vcp_data( test_vcp_num );
    if(test_vcp_ptr != NULL) {
        fprintf(stderr, "API TEST  using the pointer to vcp data (%d)\n",
                                                       (int)test_vcp_ptr);
        fprintf(stderr ,"          VCP number: %d, number of elevations: %d\n"
         "                         type %d  (2 - const ele, 16 - searchlight.\n",
                 test_vcp_ptr->vcp_num, test_vcp_ptr->n_ele, test_vcp_ptr->type);
    }
    
    test_prod_code = RPGC_get_code_from_id( output_id );
    fprintf(stderr, "API TEST  product code from  id  is %d\n", test_prod_code);
    
    test_prod_code = RPGC_get_code_from_name( output_name );
    fprintf(stderr, "API TEST  product code from name is %d\n", test_prod_code);
    
}  /* end sample1_test_api() */

