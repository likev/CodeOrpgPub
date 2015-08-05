/************************************************************************
Module:         s2_symb_layer.c

Description:    this module contains two routines. The first is used to
                construct the product symbology block and the data layer
                packet. The second is used to complete the construction of
                the product description block (which is a part of the ICD
                product header). This module has been modified for a
                run length encoded radial base reflectivity product.
----------------------------------------------------------------------------
          Modified to reflect the new size of the base data header
          
----------------------------------------------------------------------------
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org

Version 1.4, June 2004     T. Ganger
             Linux Upgrade
             
Version 1.5  March 2006    T. Ganger
             Misc cleanup
             
Version 1.6  July 2006     T. Ganger             
             Modified to use RPGC_run_length_encode() rather than the local 
                  rle function.
             Used the radial header fields for index to first good bins
                  and number of bins rather than just assuming the 
                  usual values.
             
Version 1.7  May 2007      T. Ganger
             Clarified comments to the RPGC_run_length_encode() inputs
             
Version 1.8  June 2007     T. Ganger             
             Modified to avoid confusion.  It appeared that 1-based indexes
                  were being used as input for  RPGC_run_length_encode()
                  rather than 0-based indexes.
             Changed a few comments to point out that the array of radials
                  contains modified base data radials, only the header and
                  the surveillance data are included.

Version 1.9  August 2007   T. Ganger
             Reorganized to clarify setting the 11 parameters to the
                 RPGC_run_length_encode() function.
             Fixed error In matching the 5th parameter to RLE (end_index)
                 to the 6th parameter (num_data_bins)
             Added logic to reduce the size of the product by reducing the
                 number of padded zeroes at higher elevations.
             Separated first layer header from the symbology block
                 structure for clarity
             Added defined offsets
             
Version 2.0   February 2008    T. Ganger    (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique filenames with respect to other algorithms.
              Corrected the return value of build_symbology_layer from 
                  maximum reflectivity data level to maximum value
                  in dBZ
              Modified to provide the capability to use a dynamic value
                  of number of radials instead of a predefined constant.
              When searching for max value, demonstrated not looking at
                  data beyond the last good bin in a radial.
              Set calibration constant using RPGC_set_product_float and
                  set all parameters using RPGC_set_dep_params.
                  
Version 2.1   November 2008    T. Ganger    (Sample Algorithm ver 1.20)
              Used the data structures defined in packet_af1f.h.
              


$Id$
************************************************************************/

#include "s2_symb_layer.h"

/************************************************************************
Description:    build_symbology_layer constructs the symbology block
                which encapsulates one or more data layers within the
                formatted output product. the function also builds the
                data layer using run length encoding. 
Input:          int* buffer     pointer to the output buffer
                char **radial   an array of pointers holding radial
                                structures (these are surveillance
                                only radials)
                int rad_count   number of radials in this elevation
                int bin_count   number of data bins to process
                int *length     pointer to a length accumulator variable
Output:         int *length is used to return the accumulated length of
                the constructed product
Returns:        a short integer containing the maximum reflectivity value
                found in all of the radials
Globals:        only constants (found in capitals). see include files
Notes:          This algorithm sets the radial size to 460 which is 
                meaningful only if using the traditional resolution
                data obtained from the recombination algorithm: BASEDATA,
                RECOMBINED_RAWDATA, etc.
************************************************************************/

short build_symbology_layer(short* buffer,char **radial,int rad_count,
                                             int bin_count, int *length) {
  /* build the symbology block and 16-level RLE AF1F data packet. Also
     return the maximum reflectivity */

  /* buffer entry points (offsets) --------------------------------------
    bytes shorts
       0     0    message header block offset (120 bytes or 60 shorts)
     120    60    symbology block offset      (1 0 bytes or  5 shorts) 
     130    65    first layer header offset   (  6 bytes or  3 shorts)
     136    68    packet AF1F header offset   ( 14 bytes or  7 shorts)
     150    75    AF1F radial data offset
     
  constant SYMB_OFFSET = 120
  constant SYMB_SIZE = 10
  constant LYR_HDR_SIZE = 6
  constant PKT_HDR_SIZE = 14
  ----------------------------------------------------------------------*/

/* IMPORTANT NOTE: This algorithm uses 460 as the number of bytes per   */
/*           radial by setting the number of data bins at 460.  This    */
/*           makes sense only if using the traditional resolution data  */
/*           from the output of the recombination alg: BASEDATA,        */
/*           RECOMBINED_RAWDATA, etc.                                   */

  /* variable declarations and definitions ---------------------------- */
  sym_block_hdr sym;      /* symbology block struct                     */
  layer_hdr lyr;          /* symb layer header struct                   */
  /* Sample Alg 1.20 replaced packet_AF1F_hdr with packet_af1f_hdr_t */
  packet_af1f_hdr_t hdr;  /* packet AF1F header struct                  */
  Base_data_header *bdh;  /* pointer to base data header struct         */
  int r;                  /* loop variable for radial                   */
  short max_value=0;      /* holds maximum reflectivity value for return*/
  
  
  int TEST=FALSE;         /* test flag: set to true for diagnostics     */
  int VERBOSE=FALSE;      /* verbose processing output                  */

  
  short sf;               /* product scale factor (AF1F Header)         */

  /* NOTE: the output buffer will begin at index 1                      */
  
  int start_angle;   /* param 1 to RLE function     */
  
  int angle_delta;   /* param 2 to RLE function     */
  
  short* inbuf;      /* param 3 to RLE function     */
  
  int start_index;   /* param 4 to RLE function     */
  
  int end_index;     /* param 5 to RLE function     */
  
  /* the max size of the product radial array */
  int num_data_bins; /* param 6 to RLE function     */
  
  /* currently set to 1, unsure of intended purpose */
  int buff_step;     /* param 7 to RLE function     */
  
  /* Array used to hold color table values    */ 
  short *clr;        /* param 8 to RLE function     */
  
  /* number of bytes encoded by rle routine     */
  int num_rle_bytes; /* param 9 to RLE function (value returned by function) */
  
  /* pointer into the output buffer buffer */
  int buf_index = 0; /* param 10 to RLE function  */
  
  /* the first parameter to this function, RLE data output here */
  /* short* buffer */  /* param 11 to RLE function  */


  /* enter the packet AF1F layer header values                          */
  /* Sample Alg 1.20 - replaced hdr element names with names for standard */
  /*                   structure packet_af1f_hdr_t                        */
  hdr.code = (short)0xAF1F;  /* packet code AF1F                          */
  hdr.index_first_range = 0;    /* distance to the first range bin        */

  hdr.num_range_bins = bin_count;   /* number of bins in each radial      */
  
  hdr.i_center = 256;          /* i center of display (ramtec values)     */
  hdr.j_center = 280;          /* j center of display (ramtec values)     */
  bdh = (Base_data_header*)radial[0];  /* cast a base data hdr structure  */
  sf = (short)(bdh->cos_ele*1000.0); /* create scale factor (scaled short)*/
  hdr.scale_factor = sf;       /* scale factor                            */
  hdr.num_radials = rad_count;    /* number of radials included           */
  
  

  /* load the packet  header into the buffer. buffer offset=68     */
  /* header size=14 bytes                                          */
  if(VERBOSE) {
    fprintf(stderr,"copy packet AF1F layer header to buffer now\n");
    fprintf(stderr, "  radial count is %d, bin count is %d\n", 
                                         rad_count, bin_count);
  }
  
  
  /* ALL OFFSETS TO PRODUCT ARRAY ARE IN 2-BYTE INTEGERS */


  memcpy(buffer + ( (SYMB_OFFSET+SYMB_SIZE+LYR_HDR_SIZE) / 2 ), 
                                                &hdr, PKT_HDR_SIZE);
                                                
  if(VERBOSE) {
    fprintf(stderr,"finished copy packet AF1F layer header to buffer\n");
  }
  /* display beginning of the data buffer                               */
/*  if(TEST) {
 *     for(i=0;i<160;i++)
 *       fprintf(stderr,"%04x ",(unsigned short)buffer[i]);
 *       }
 */
  /* offset to beginning of AF1F packet data array (total 75 shorts)    */
  /* param 10 to RLE function */
  buf_index = (SYMB_OFFSET + SYMB_SIZE + LYR_HDR_SIZE + PKT_HDR_SIZE) / 2;

  /* Construct the reflectivity layer data. process for each radial     */
  for(r = 0; r < rad_count; r++) {
    
    /* cast a short pointer to each surveillance only base data radial  */
    short* sptr = (short*)radial[r]; 
    int val;  /* variable used to hold the max value from each radial   */
   
    /* get a handle to access the radial structure from the radial array*/
    bdh = (Base_data_header*)radial[r];  /* cast a base data hdr structure*/

    /* process each radial and save the maximum value                   */
    val = get_max_value(sptr, bin_count);
    if(val > max_value)  max_value = val; /* update the elev max value   */
    
    if(TEST)
       fprintf(stderr, "\n-> processing for r=%03i az angle %4.1f  max val=%i\n",
                                             r, bdh->start_angle/10.0, max_value);
    

    /* start the run length encoding process                            */
    { /* ============================================================== */
      
      start_angle = (int)bdh->start_angle;  /* param 1 to RLE function   */
      
      angle_delta = (int)bdh->delta_angle;  /* param 2 to RLE function   */
      
      /* move ptr to beg of data section  (a surveillance only radial) */ 
      
      inbuf = sptr + (sizeof(Base_data_header) / 2); /* param 3 to RLE function */
       
      /* used with new rle function (values from basedata header)    */
      /* input indexes are zero based, header values are 1-based     */
      start_index = bdh->surv_range - 1;  /* param 4 to RLE function */
      
      end_index = bdh->n_surv_bins - 1;   /* param 5 to RLE function */
      
      /* the max size of the product radial array */
      num_data_bins = bin_count; /* param 6 to RLE function          */
      
      /* currently set to 1, unsure of intended purpose */
      buff_step = 1;   /* param 7 to RLD function                    */

      /* cannot exceed number of data bins for the product */
      if( ((end_index - start_index) + 1) > num_data_bins )
           end_index = (num_data_bins - start_index) - 1;
/*******************************************************************/
  
      /* run length encode now. return number of bytes encoded    */
      if(VERBOSE) fprintf(stderr, "beginning run length encoding now\n");

      /* ----- THE NEW RLE FUNCTION ------------- */
      /* build the color table locally                             */
      /* allocate memory to be used for the color table            */
      
      clr = (short*)malloc(256*sizeof(short)); /* param 8 to RLE function */
      /* load the color table                                      */
      create_color_table(clr);

      if(clr == NULL) {
         fprintf(stderr, "ERROR Failure to create color table\n");
      }

      if(VERBOSE) fprintf(stderr, "Finished Creation of Color Table\n");
  


      RPGC_run_length_encode( start_angle,   /* radial start angle (deg*10)        */
                              angle_delta,   /* radial width (deg*10)              */
                              inbuf,         /* input data to be run-length encoded*/
                              start_index,   /* index into inbuf for first good bin*/
                              end_index,     /* index into inbuf for last good bin,*/
                                             /* NOT to exceed num_data_bins!!!     */
                              num_data_bins, /* max number of data bins to encode  */
                              buff_step,     /* usually 1                          */
                              clr,           /* pointer to color table             */
                              &num_rle_bytes,/* number of rle bytes for this radial*/
                              buf_index,     /* index into buffer for beginning    */
                                             /* of rle data for this radial        */
                              buffer );      /* typically buffer containing        */
                                             /* the final product                  */
  
      /* free the allocated color table array                              */
      if(clr != NULL) free(clr);
      /* ---------------------------------------- */
  
        

      if(VERBOSE) {
         fprintf(stderr,"\nAfter radial run length encode: number of rle bytes=%i\n",
                                                                       num_rle_bytes);
         fprintf(stderr, "start_index=%d; end_index=%d\n",
                                   start_index, end_index);
      }
        
    } /* end of rle block =============================================*/


    /* increment offset for next call to run_length_encode */
    buf_index += num_rle_bytes/2;  /* param 10 to RLE function */

    if(VERBOSE) 
        fprintf(stderr,"Updated offset (buf_index) for next run = %i\n",
                                                              buf_index);

  } /* end of r loop */


  /* return the total length of the product (in bytes) minus the        */
  /* Graphic_product structure length (120 bytes or 60 shorts) which is */
  /* added in the RPGC_prod_hdr function                                */
  
  *length = (buf_index - (SYMB_OFFSET / 2) ) * 2;

  /* construct the symbology block header to be inserted at pos ptr+60  */
  if(VERBOSE) fprintf(stderr, "building symbology block now\n");
  
  sym.divider = (short)-1;      /* block divider (constant value)         */
  sym.block_id = (short)1;      /* block ID=1 (constant value)            */

  RPGC_set_product_int( (void *) &sym.block_length, 
                          (unsigned int)(*length) );

               
  sym.num_layers = (short)1;    /* number of data layers included         */
  
  lyr.divider2 = (short)-1;     /* layer divider (constant value)         */
  
  /* layer length: block_length - 16 bytes (sym header and layer header)*/
  RPGC_set_product_int( (void *) &lyr.layer_length, 
                     (unsigned int)(*length - SYMB_SIZE - LYR_HDR_SIZE) ); 
                       

  /* copy the symbology block into the output buffer                    */
  if(VERBOSE) fprintf(stderr, "copy symbology block to buffer now\n");
  
  /* OLD: previously the structure containing the symb header and the
   *      the layer header could be copied as a block
   * memcpy(buffer+60,&sym,16);
   */
  /* NEW: the separate symb header and layer header structures have
   *      alignment issues and some data fields must be copied separately
   */
  memcpy(buffer + (SYMB_OFFSET / 2), &sym, SYMB_SIZE);

  memcpy(buffer + ( (SYMB_OFFSET + SYMB_SIZE) / 2 ), &lyr.divider2, 2);
  memcpy(buffer + ( (SYMB_OFFSET + SYMB_SIZE + 2) / 2 ), &lyr.layer_length, 4);



  if (VERBOSE) 
    fprintf(stderr,"Symbology Block Generation Complete: length=%d max_refl=%hd\n",
                                                                *length, max_value);

  if(TEST) {
     /* print contents of symbology and first layer header for diagnostic */
     print_symbology_header( (char*)( buffer + (SYMB_OFFSET / 2) ) );
     
  }

  /* corrected returned maximum data level to returned */
  /*                  maximum value in dBZ.            */

  return( (short) RPGC_NINT( RPGCS_reflectivity_to_dBZ(max_value) ) );

  
}



/*************************************************************************
Description:    get_max_value traverses each radial and returns the  
                maximum value found                                     
Input:          short* sptr: pointer to the a radial of data
                short num_bins: number of bins in the radial
Output:         none
Returns:        the maximum data value within the radial
Globals:        none
Notes:          none
*************************************************************************/
int get_max_value(short* sptr, int num_bins) {
  /* traverse the radial beginning at sptr and return the max value     */
  /* along num_bin values                                               */
  int i;            /* loop variable                                    */
  short val;        /* variable to hold the current data value          */
  int maxval=0;     /* instantaneous maximum value holder               */
  
  Base_data_header *bdh = (Base_data_header *)sptr;
  
  int VERBOSE=FALSE;      /* verbose processing output                  */ 

  int start = sizeof(Base_data_header)/2; /* calculate starting offset    */

  /* find the maximum value now                                         */
  for(i = start; i < start+num_bins; i++) {
     /* test for being beyond last good bin */
     if(i < bdh->n_surv_bins ) {
        val = sptr[i];
        if(val > maxval) maxval = val;
     }
  }

  if(TRUE)
    if (VERBOSE) {
        fprintf(stderr,"  Return Max Value: %i\n",maxval);
    }

  return(maxval);
  
}


/************************************************************************
Description:    finish_pdb fills in values within the Graphic_product
                structure which were not filled in during the system call
Input:          int* buffer: pointer to the output buffer
                short elev_index: observed elevation index
                short elevation: target elevation              
                short cal_msw: most significant 2 bytes of the cal const
                short cal_lsw: least significant 2 bytes of the cal const
                short max_refl: calculated maximum reflectivity
                int total product length minus the pre-icd header
Output:         completed buffer with ICD formatting via *buffer
Returns:        none
Globals:        none
Notes:          none
************************************************************************/
void finish_pdb(short* buffer, short elev_ind, short elevation,
   /*short cal_msw, short cal_lsw,*/ float calib_const, short max_refl, int prod_len) {
  /* complete entering values into the product description block (pdb)
   * Enter: threshold levels, product dependent parameters, version and
   * block offsets for symbology, graphic and tabular attribute.        */

  /* cast the msg header and pdb block to the pointer hdr               */
  Graphic_product* hdr=(Graphic_product*)buffer;

  short params[10];

  memset( params, 0, 10*sizeof(short) );
  
/*  fprintf(stderr,"\nEntering the Product Description Block\n");       */



  /* enter the data threshold values                                    */
  hdr->level_1=(short)-32766;
  hdr->level_2=(short)5;
  hdr->level_3=(short)10;
  hdr->level_4=(short)15;
  hdr->level_5=(short)20;
  hdr->level_6=(short)25;
  hdr->level_7=(short)30;
  hdr->level_8=(short)35;
  hdr->level_9=(short)40;
  hdr->level_10=(short)45;
  hdr->level_11=(short)50;
  hdr->level_12=(short)55;
  hdr->level_13=(short)60;
  hdr->level_14=(short)65;
  hdr->level_15=(short)70;
  hdr->level_16=(short)75;

  /* product dependent parameters */
  params[2] = (short)elevation;  /* for hdr->param_3 */
  params[3] = (short)max_refl;   /* for hdr->param_4 */
  RPGC_set_product_float( &params[7], calib_const); /* for hdr->param_8 */
                                                    /* and hdr->param_9 */
  RPGC_set_dep_params( buffer, params );

  /* number of blocks in product = 3                                    */
  hdr->n_blocks=(short)3;

  /* message length                                                     */
  RPGC_set_product_int( (void *) &hdr->msg_len, 
                        (unsigned int) prod_len );

  /* ICD block offsets (in bytes)                                       */

  RPGC_set_product_int( (void *) &hdr->sym_off, 60 );
  RPGC_set_product_int( (void *) &hdr->gra_off, 0 );
  RPGC_set_product_int( (void *) &hdr->tab_off, 0 );

  /* elevation index                                                    */
  hdr->elev_ind=(short)elev_ind;

  return;
  
}


