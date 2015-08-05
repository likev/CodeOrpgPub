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
Module:         s3_symb_layer.c  (for the 2 module chain algorithm)

Description:    this module contains two routines. the first is used to
                construct the product symbology block and the data layer
                packet. the second is used to complete the construction of
                the product description block (which is a part of the ICD
                product header). These programs work specifically with
                an internal data structure (s3_t2_internal_rad) which is 
                loaded from the intermediate product structure.
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.2,   June 2004   T. Ganger
               Linux upgrade

Version 1.3    March 2006  T. Ganger
               Revised to use RPGC_set_packet_16_radial to avoid byte-swap
               
Version 1.4   August 2007    T. Ganger
              Separated first layer header from the symbology block
                 structure for clarity
              Added defined offsets
               
Version 1.5   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Noted setting number of bytes per radial at 230 is artificial
                  and dependent upon using recombined data.
              Corrected the return value of build_symbology_layer from 
                  maximum reflectivity data level to maximum value
                  in dBZ.
              Reorganized in order to use return value from packet 16
                  helper function for size of packet 16 radial data.
              Modified to provide the capability to use a dynamic value
                  of number of radials instead of a predefined constant.
              Set all parameters using RPGC_set_dep_params.
              
Version 1.6   June 2008    T. Ganger  (Sample Algorithm ver 1.19)
              Changed the surv_data variable to an array of unsigned char.
              Eliminated Red Hat 5 compile warnings.
              
Version 2.0   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Modified to write the new Scale Offset decoding parameters
                  into the threshold level fields rather than the 
                  tradiational min value, increment, etc.
              Used a new structure for internal data array.
              Used the data structures defined in packet_16.h.
              Correct the value of scale factor in the packet 16 header.
              

$Id$
************************************************************************/

#include "s3_symb_layer.h"



/************************************************************************
Description:    build_symbology_layer constructs the symbology block
                which encapsulates one or more data layers within the
                formatted output product. the function also builds the
                data layer using a digital radial data array packet. this
                packet consists of one byte per bin (no run length 
                encoding)
Input:          char* buffer    pointer to the output buffer
                s3_t2_internal_rad *rad_ptr
                int rad_count   number of radials in this elevation
                int bin_count   number of data bins to process
                int *length     pointer to a length accumulator variable
Output:         int *length is used to return the accumulated length of
                the constructed product
Returns:        a short integer containing the maximum reflectivity value
                    dBZ found in all of the radials; or -1 on error
Globals:        only constants (found in capitals). see include files
Notes:          

************************************************************************/
/* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
short build_symbology_layer(char* buffer, s3_t2_internal_rad *rad_ptr, int rad_count,
                                                  int bin_count, int *length) {
  /* build the symbology block and digital radial data array and
   return max reflectivity */

  /* buffer entry points (offsets) ---------------------------------------
    bytes shorts
       0     0    message header block offset (120 bytes or 60 shorts)
     120    60    symbology block offset      ( 10 bytes or  5 shorts) 
     130    65    first layer header offset   (  6 bytes or  3 shorts)
     136    68    packet 16 header offset     ( 14 bytes or  7 shorts)
     150    75    packet 16 radial data offset

  constant SYMB_OFFSET = 120
  constant SYMB_SIZE = 10
  constant LYR_HDR_SIZE = 6
  constant PKT_HDR_SIZE = 14
  ----------------------------------------------------------------------*/


  /* variable declarations and definitions ---------------------------- */
  /* uses separate symbology header and layer header structures     */
  /*               to demo how multiple layers could be handled     */
  sym_block_hdr sym;      /* symbology block struct                     */
  layer_hdr lyr;          /* symb layer header struct                   */
  /* Sample Alg 1.20 - replaced packet_16_hdr with Packet_16_hdr_t      */
  Packet_16_hdr_t hdr;    /* packet 16 header struct                    */
  /* Sample Alg 1.20 - replaced packet_16_data with Packet_16_data_t    */
  Packet_16_data_t rad;   /* packet 16 data layer struct                */

  unsigned char *rad_data; /* pointer to char dynamically create a single  */
                         /* radial array from the array of shorts      */
                         /* provided by the intermediate product       */

  int r, b;               /* loop variables for radial and bin          */
  int offset=0;           /* pointer offset into output buffer          */
  short max_value=0;      /* holds maximum reflectivity value for return*/

  int packet_16_data_begin;
  int packet_16_data_size;

  int num_bytes=0;
  
  int TEST=FALSE;
/*  int TEST=TRUE; */

  unsigned int block_len;
  
  /* Sample Alg 1.20 */
  short sf;               /* product scale factor (Packet 16 Header)    */

  /* treat the int* buffer as a char* buffer to more easily access byte */
  /* size words */


  /* set offset to beginning of data layer                              */
  offset = SYMB_OFFSET + SYMB_SIZE + LYR_HDR_SIZE + PKT_HDR_SIZE;

  packet_16_data_begin = offset;

  if(TEST) fprintf(stderr,"begin loading radial data\n");

  /* get a pointer to load the data array from the intermediate  product radial */
  /* and write to data packet 16 radial */
  rad_data = (unsigned char *) malloc( (size_t)bin_count );
  
  if(rad_data == NULL)
      return(-1);

/* DESIGN NOTE: if the use of the indexes in the intermediate product data */
/*              array: rad_ptr->radial_data[i][j] were reversed, there    */
/*              would be no need to allocate and load the rad_data array  */
/*              because (&rad_ptr->radial_data[i][0]) could have been     */
/*              passed directly to RPGC_set_packet_16_radial()            */
    
  /* load the reflectivity layer data. process for each radial          */
  for(r = 0; r < rad_count; r++) {
    
    /* Sample Alg 1.20 - replaced the element names with names for standard */
    /*                   structure Packet_16_data_t                         */
    rad.size_bytes = (short)bin_count;   /* number of bytes per radial   */
    rad.start_angle=(short)(rad_ptr->start_angle[r]);
    rad.delta_angle=(short)(rad_ptr->angle_delta[r]);
/* DEBUG T2-3 */

    for(b = 0; b < bin_count; b++) {
      /* Note: bin_count can be used because data beyond last good bin is '0' */
      rad_data[b] = (unsigned char)(rad_ptr->radial_data[r][b]);

      /* Find maximum value for this radial */
      if(rad_data[b] > max_value) 
          max_value = (short) rad_data[b];
          
    } /* end of b loop */

/* DEBUG T2-4 */

    /* A helper function should be used to write the individual data    */
    /* fields into the packet 16 data array.  One reason for this is    */
    /* that individual bytes in the array must be swapped out of place. */
    /* This is a result of the ORPG originally being on a Big Endian   */
    /* platform and the infrastructure accomplished byte-swapping when   */
    /* the product is written to the database.                          */

    /* Since the data beyond the last good bin in each radial (if any exist)*/
    /* have already been padded with '0' in the main module, this bin_count */
    /* can be used without being corrected for last good bin in this        */
    /* function. A new function RPGC_digital_radial_data_array is available */
    /* that includes a correction for index of last good bin.               */
    num_bytes = RPGP_set_packet_16_radial( (unsigned char *) buffer+offset, 
                                           (short)rad.start_angle, 
                                            (short)rad.delta_angle, rad_data, 
                                            (int)bin_count ); 
     
    /* WARNING: For Build 10, the return value 'num_bytes' does NOT include */
    /*          the 6-byte radial header; prior to Build 10 'num_butes'     */
    /*          DID include the 6-byte radial header.  This may change.     */

    offset += num_bytes + 6;

  } /* end of r loop */


  if(rad_data != NULL) free(rad_data);
    

  packet_16_data_size = offset - packet_16_data_begin;

  /* return the total length of the product minus the Graphic_product   */
  /* structure length which is added in the RPG_prod_hdr function       */
  *length = offset - SYMB_OFFSET;


  /* build the symbology block */
  if(TEST) fprintf(stderr,"building symbology block now\n");
  
  /* ----- Set Symbology Block Header ----------------------------------------- */
  sym.divider=(short)-1;      /* block divider (constant value)         */
  sym.block_id=(short)1;      /* block ID=1 (constant value)            */

  block_len = (unsigned int)( SYMB_SIZE + LYR_HDR_SIZE + PKT_HDR_SIZE +
                                                    packet_16_data_size );
  RPGC_set_product_int( (void *) &sym.block_length, block_len );
    
  sym.num_layers=(short)1;    /* number of data layers included         */
  
  /* ----- Set Symbology Block Layer Header ----------------------------------- */
  lyr.divider2=(short)-1;     /* first layer divider (constant value)   */

  /* layer length: block_length - (sym header and layer header)         */
  RPGC_set_product_int( (void *) &lyr.layer_length,
              (unsigned int)(block_len - SYMB_SIZE - LYR_HDR_SIZE) );

  
  /* ----- copy the symbology header and layer header into the output buffer -- */
  if(TEST) fprintf(stderr,"copy symbology block to buffer now\n");
  
  /* OLD: previously the structure containing the symb header and the
   *      the layer header could be copied as a block
   */
  /* NEW: the separate symb header and layer header structures have
   *      alignment issues and some data fields must be copied separately
   */
  memcpy(buffer + SYMB_OFFSET, &sym, SYMB_SIZE);
  memcpy(buffer + SYMB_OFFSET + SYMB_SIZE, &lyr.divider2, 2);
  memcpy(buffer + SYMB_OFFSET + SYMB_SIZE + 2, &lyr.layer_length, 4);


  /* ----- load the packet 16 header ---------------------------------------- */
  /* Sample Alg 1.20 - replaced the element names with names for standard */
  /*                   structure Packet_16_hdr_t                          */
  hdr.code=16;             /* packet code 16                           */
  hdr.first_bin=0;    /* distance to the first range bin          */

  hdr.num_bins = num_bytes;   /* number of bins in each radial            */

  hdr.icenter=0;            /* i center of display                      */
  hdr.jcenter=0;            /* j center of display                      */
  /* Sample Alg 1.20 */
  sf = (short)(rad_ptr->bdh->cos_ele*1000.0); /* create scale factor (scaled short)*/
  hdr.scale_factor=sf;      /* scale factor                             */
  hdr.num_radials=rad_count;/* number of radials included               */

  /* load the packet layer header into the buffer. buffer offset=136.   */
  /* header size=14                                                     */
  if(TEST) fprintf(stderr,"copy packet 16 layer header to buffer now\n");
  
  memcpy(buffer + SYMB_OFFSET + SYMB_SIZE + LYR_HDR_SIZE, 
                                                &hdr, PKT_HDR_SIZE);

/* end of symbology and packet 16 header process ------------------------- */



  if(TEST) {
     fprintf( stderr,
        "Symbology Block Generation Complete: prod length=%d max_refl=%hd\n"
        "          max reflectivity in dBZ is %d\n",
        *length,max_value, (short) RPGC_NINT( RPGCS_reflectivity_to_dBZ(max_value)) );
     /* print contents of symbology and first layer header for diagnostic */
     print_symbology_header((buffer + SYMB_OFFSET) );
     
  }

  return( (short) RPGC_NINT( RPGCS_reflectivity_to_dBZ(max_value) ) );
  
} /* end build_symbology_layer */



/************************************************************************
Description:    finish_pdb fills in values within the Graphic_product
                structure which were not filled in during the system call
Input:          char* buffer:     pointer to the output buffer
                short elev_index: observed elevation index
                float elevation:  floating point elevation value
                short max_refl:   calculated maximum reflectivity
Output:         completed buffer with ICD formatting
Returns:        none
Globals:        none
Notes:          none
************************************************************************/

void finish_pdb(char* buffer, short elev_index, short target_elev, short max_refl,
   int prod_len) {
  /* complete entering values into the product description block (pdb)
     Enter: threshold levels, product dependent parameters, version and
     block offsets for symbology, graphic and tabular attribute.        */


  /* cast the msg header and pdb block to the pointer hdr               */
  Graphic_product* hdr=(Graphic_product*)buffer;

  short params[10];

  memset( params, 0, 10*sizeof(short) );



  /* enter the data threshold values                                    */
  /* Sample Alg 1.20 - used Build 12 Scale-Offset parameters */
  /* float value for Scale written to level 1 & 2 */
  RPGC_set_product_float( (void*)&hdr->level_1, 2.0);
  /* float value for Offset written to level 3 & 4 */
  RPGC_set_product_float( (void*)&hdr->level_3, 66.0);
  hdr->level_5=(short)0;
  hdr->level_6=(short)255; /* maximum data level */
  hdr->level_7=(short)2;   /* number of leading flags */
  hdr->level_8=(short)0;   /* number of trailing flags */
  hdr->level_9=(short)0;
  hdr->level_10=(short)0;
  hdr->level_11=(short)0;
  hdr->level_12=(short)0;
  hdr->level_13=(short)0;
  hdr->level_14=(short)0;
  hdr->level_15=(short)0;
  hdr->level_16=(short)0;

  /* product dependent parameters */
  params[2] = (short)target_elev;  /* for hdr->param_3 */
  params[3] = (short)max_refl;   /* for hdr->param_4 */

  RPGC_set_dep_params( buffer, params );


  /* number of blocks in product = 3                                    */
  hdr->n_blocks=(short)3;

  /* message length                                                     */
  RPGC_set_product_int( (void *) &hdr->msg_len, 
                        (unsigned int) prod_len );

  /* ICD block offsets                                                  */

  RPGC_set_product_int( (void *) &hdr->sym_off, 60 );
  RPGC_set_product_int( (void *) &hdr->gra_off, 0 );
  RPGC_set_product_int( (void *) &hdr->tab_off, 0 );

  /* elevation index                                                    */
  hdr->elev_ind=elev_index;

  return;
  
}



