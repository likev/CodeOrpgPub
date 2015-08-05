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
Module:         sample4_t2.c

Description:    sample4_t2.c is part of a chain-algorithm demonstration
                algorithm for the ORPG. The overall intent of the
                algorithm is to demonstrate a two task algorithm that 
                produces multiple intermediate products and multiple 
                final products.
                
                THE PURPOSE OF THIS SAMPLE ALGORITHM TASK IS TO DEMONSTRATE
                AN ALGORITHM USING THE 'WAIT_ANY' FORM OF THE CONTROL
                LOOP AND HAVING MORE THAN ONE OUTPUT.  Constructions of a 
                simple product using a single text data packet is 
                accomplished.  No useful science is demonstrated.
                
                The structure of this algorithm complies with the guidance
                provided in the CODE Guide Vol 3.
                
                sample4_t2.c reads the available intermediate product, 
                determines which product was read and if the corresponding
                final product was requested.  The final product is a basic 
                geographic product containing trivial text data.

                
                Key source files for this algorithm include:

                sample4_t2.c         program source
                sample4_t2.h         main include file
                
                
                
Authors:        Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org

              
Version 2.0   April 2006   T. Ganger
              Revised to use current guidance for abort functions and 
                   abort reason codes.
              Rewritten to create a simple graphic product containing
                   a single text packet rather than a stand-alone
                   tabular alphanumeric product (SATAP)
                              
Version 2.1   June 2006    T. Ganger
              Revised to use RPGC_reg_io
              Revised to use the new 'by_name' get_outbuf, check_data, 
                   and abort_dataname functions              

Version 2.2   August 2007  T. Ganger
              Separated first layer header from the symbology block
                 structure for clarity
              Added defined offsets
              
Version 2.3   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated use of defined output buffer ids by using
                  RPGC_get_id_from_name and by testing the registration
                  name for the output rather than the id.
                
Version 2.4   November 2008   T. Ganger   (Sample Algorithm ver 1.20)
              Clarified overall description of task.
              
$Id$
************************************************************************/


#include "sample4_t2.h"


/* SET TO GET VARIOUS DIAGNOSTIC OUTPUTS */
int TEST = FALSE;
int VERBOSE = FALSE; 
int test_out=FALSE; /* Boolean to control diagnostic product file output */  



/* Function prototypes.----------------------------------------------- */

static int Process_input( char *inbuf, char *out_data_name, 
                                      int output_size );

void build_data_layer(char *outbuffer, char *text_data, int *length);

void finish_prod_desc_block(char *outbuffer, short elev_index, 
                            short elevation, int prod_len);

void clear_buffer(char *buffer, int bufsize);



/************************************************************************/
/* Main routine for sample4_t2 test program.                             */
int main( int argc, char *argv[] ){

   /* algorithm variable declarations and definitions  */
   int opstatus;            /* variable:  used to hold API return values */
   char *inbuf;             /* pointer to intermediate input buffer      */
   
   /* keep until Build 9 changes completed */
   int in_datatype;         /* prod ID returned from RPGC_get_inbuf_any  */
   
   int output_size;         /* output product size, in bytes             */

   char OUTDATA_NAME[65];
  
  
/*----------------------------------------------------------------------*/

   /* Initialize log-error services. */
   RPGC_init_log_services( argc, argv );


   /* register inputs and outputs based upon the            */
   /* contents of the task_attr_table (TAT).                */
   /* input:  SAMPLE4_IP1(1998)  SAMPLE4_IP2(1997)          */
   /* output: SAMPLE4_FP1(1993)  SAMPLE4_FP2(1994)          */
   RPGC_reg_io(argc, argv);


   /* Specify task timing. */
   RPGC_task_init( ELEVATION_BASED, argc, argv );

   fprintf(stderr, "Sample4_t2 initialization complete. \n");
   RPGC_log_msg( GL_INFO, "Sample4_t2 initialization complete. \n");
   

   /* Main processing loop ... */
   while(1) {

      /* Wait for any of the inputs to become available. */
      RPGC_wait_for_any_data( WAIT_ANY_INPUT );

      if(VERBOSE)
          fprintf(stderr, "Sample4_t2 released for processing \n");
      RPGC_log_msg( GL_INFO, "Sample4_t2 released for processing \n");
      
      /* One of the inputs is available, get the input and process 
         accordingly. */
      in_datatype = 0;
      opstatus = NORMAL;

      /* WISH LIST: WOULD BE NICE TO HAVE a get_inbuf_by_name_any or */
      /*            a get_name_from_id function                      */
      /* Instead, we compare the data_type to the value returned by  */
      /* RPGC_get_id_from_name                                       */
 
      inbuf = (char *) RPGC_get_inbuf_any( &in_datatype, &opstatus );
      
       /* For SAMPLE4_IP1 input, do the following algorithm processing.*/
      if( in_datatype == RPGC_get_id_from_name( "SAMPLE4_IP1") ) {

         RPGC_log_msg( GL_INFO, 
                      "RPGC_get_inbuf_any returned SAMPLE4_IP1\n" );
         strcpy(OUTDATA_NAME, "SAMPLE4_FP1");
         output_size = SAMPLE4_FP_SIZE;

        /* For SAMPLE3_IP2 input, do the following algorithm processing.*/
      } else if( in_datatype == RPGC_get_id_from_name( "SAMPLE4_IP2") ) {

         RPGC_log_msg( GL_INFO, 
                     "RPGC_get_inbuf_any returned SAMPLE4_IP2\n" );
         strcpy(OUTDATA_NAME, "SAMPLE4_FP2");
         output_size = SAMPLE4_FP_SIZE;

      } else { /* Can this actually happen? */
         RPGC_log_msg( GL_INFO, "Unknown Datatype Received %d\n",
                      in_datatype );
         if( opstatus == NORMAL )
             RPGC_rel_inbuf( inbuf );
         RPGC_abort();
         continue;
      }

      /* If non-NORMAL status returned, abort processing. */
      if( opstatus != NORMAL ){
         RPGC_log_msg( GL_INFO, 
                     "RPGC_get_inbuf_any Returned Bad Status (%d)\n",
                      opstatus );
         if( opstatus == TERMINATE ) /* IS THIS NECESSARY?  */
            RPGC_abort();
         else
            RPGC_abort_dataname_because( OUTDATA_NAME, opstatus );
         continue;
      }


      /* Process the input data  .... generate the output data. */
      Process_input( inbuf, OUTDATA_NAME, output_size );
   
   } /* End of "while" loop. */


} /* End of main */




/*************************************************************************

   Description:
      Processes the input buffer and generates an output buffer based on 
      outstanding request. 

   Inputs:
      inbuf - pointer to input buffer
      out_data_name - output data name associated with input datatype
      output_size - size, in bytes, of output data name

   Outputs:
      None 

   Returns:
      -1 on error, 0 otherwise.
      
**************************************************************************/ 
static int Process_input( char *inbuf, char *out_data_name, 
                          int output_size ){

   char *outbuf;            /* pointer to contain a SATAP product*/
   int opstatus;            /* status returned from RPGC_get_outbuf call */

   Base_data_header *bdh;  /* the first base data header */

   char data_string[SAMPLE4_IP_SIZE - 24]; /* intermediate product data   */
   
   int ret_val;
   
   int prod_id;
   
   
   /* Check to see if there is a request for output datatype. */
   if( RPGC_check_data_by_name( out_data_name ) != NORMAL ) { /* no request */ 
    
      RPGC_log_msg( GL_INFO, "No Requests For %s Found\n", out_data_name  );
      RPGC_rel_inbuf( inbuf );
      
      return(-1);
    
   } else { /* there is a request for the product out_datatype */

      /* Get output buffer associated with this input. */         
      outbuf = (char*) RPGC_get_outbuf_by_name(out_data_name, output_size, 
                                                                    &opstatus);

      if( opstatus != NORMAL ) {
         RPGC_log_msg( GL_INFO, 
                      "RPGC_get_outbuf for %s Returned Bad Status (%d)\n",
                      out_data_name , opstatus);
         RPGC_abort_dataname_because( out_data_name, opstatus );
         RPGC_rel_inbuf( inbuf );
         
         return(-1);

      }  else { /* opstatus NORMAL, create the product */

         int vol_num, outlen = 0;


         clear_buffer(outbuf, output_size);

         /* If there is processing of the input and building of the 
            output, do it here. */
         vol_num = RPGC_get_buffer_vol_num( inbuf );

         prod_id = RPGC_get_id_from_name( out_data_name );
         
         /* step 1: build the product description block -uses a system call*/
         RPGC_prod_desc_block( outbuf, prod_id, vol_num );
        

         /* step 2: build the symbology layer & and packet 1 data.      */
         
         /* get the character text from the intermediate product */
         strcpy( data_string, inbuf + sizeof(Base_data_header) ); 

         /* this routine returns both the overall length (thus far) of the      */
         /* product and the maximum reflectivity of all radials                 */
         build_data_layer(outbuf, data_string, &outlen);



         /* step 3: finish building the product description block by            */
      
         /* using elevation index and actual elevation using base data header   */
         /* that has been passed as part of the intermediate product            */
         bdh = (Base_data_header *) inbuf;

         finish_prod_desc_block(outbuf, bdh->rpg_elev_ind,
                                        bdh->target_elev, outlen); 
         /* if we did not pass the base data header in the intermediate product */
         /* helper functions could be used to obtain elevation index and angle  */


         
         /* step 4: build the message header block (mhb)                        */
         ret_val = RPGC_prod_hdr( outbuf, prod_id, &outlen );

        if (TEST) {
           fprintf(stderr, "\n==> after prod_hdr\n");
           print_message_header(outbuf);
           print_pdb_header(outbuf);
        }


         
         if(ret_val != 0) {
            RPGC_log_msg( GL_INFO, "product header creation failure\n");
            RPGC_rel_outbuf((void*)outbuf, DESTROY);
            RPGC_rel_inbuf( inbuf );
            RPGC_abort_because(PGM_PROD_NOT_GENERATED);
            return(-1);
         }


         /* diagnostic - interrupts product output and creates a 
          * binary output of product to file.
          * this is useful if product problems cause task failure
          */
         if(test_out == TRUE) {

            product_to_disk(outbuf, outlen, out_data_name, bdh->rpg_elev_ind);

            RPGC_log_msg( GL_INFO,
               "Interrupted product output for diagnostic output to file.\n");
            RPGC_rel_outbuf( (void*)outbuf, DESTROY );
         
         } else {
            
            /* step 5: forward the completed product to the system    */
            
            RPGC_rel_outbuf( (void*)outbuf, FORWARD );
            
         }


         /* Release the input buffer, then wait for any data. */
         RPGC_rel_inbuf( inbuf );
         
         return(0);

      } /* end else opstatus NORMAL */


   } /* end else there is a request for out_ datatype */


} /* End of Product_processing() */




/************************************************************************/
void build_data_layer(char *outbuffer, char *text_data, int *length) {
 
    
  /* buffer entry points (offsets) ---------------------------------------
    bytes shorts
       0     0    message header block offset (120 bytes or 60 shorts)
     120    60    symbology block offset      ( 10 bytes or  5 shorts) 
     130    65    first layer header offset   (  6 bytes or  3 shorts)
     136    68    packet 1 header offset      (  8 bytes or  4 shorts)     
     144    72    packet 1 character data offset

  constant SYMB_OFFSET = 120
  constant SYMB_SIZE = 10
  constant LYR_HDR_SIZE = 6
  constant PKT_HDR_SIZE = 8
  ----------------------------------------------------------------------*/
     

  /* variable declarations and definitions ---------------------------- */
  /* added Sample Alg 1.17 */
  sym_block_hdr sym;      /* symbology block header struct              */
  layer_hdr lyr;          /* symb layer header struct                   */ 

  pkt_1_hdr pkt;          /* packet 1 header sruct                      */  
    
  unsigned int block_len;
  short num_chars;
  int i;

  char added_text[] = " (Final Product)";
  char output_text[ SAMPLE4_FP_SIZE -
                    (SYMB_OFFSET+SYMB_SIZE+LYR_HDR_SIZE+PKT_HDR_SIZE) ];
  char a_space[] = " ";
 
  if (VERBOSE) {
    fprintf(stderr, "building symbology block now\n");
  } 


   if(TEST) {
     fprintf(stderr, "Input Character Data is:\n'");
     for(i = 0; i < strlen(text_data); i++)
         fprintf(stderr, "%c", text_data[i]);
     fprintf(stderr, "'\n");
   }


  /* PRODUCT DATA PROCESSING - modify the input text for output */
  /*--------------------------------------------------------------------*/

  /* combine the first 28 characters from input text with added text */
  if(strlen(text_data) >= 28)
      num_chars = 28;
  else
      num_chars = strlen(text_data);
      
  memcpy(output_text, text_data, num_chars);
  memcpy(output_text+num_chars, added_text, strlen(added_text));
  
  /* total text length is input text plus added text */
  num_chars += (short) strlen(added_text);

  if(TEST)
     fprintf(stderr, "Length of output text is %d characters.\n", num_chars);
       
  /* packet code 1 requires an even number of characters */
  /* if odd, num_chars is incremented and a space is appended to output */
  if( (num_chars % 2) != 0 ) {
       num_chars++;  
       memcpy(output_text+num_chars, a_space, 1);
       if(TEST)
          fprintf(stderr, "Padded output text with a space\n.");
  }
  


  /* set symbology block header and first layer length */
  /*--------------------------------------------------------------------*/    
  sym.divider = (short)-1;      /* block divider (constant value)         */
  sym.block_id = (short)1;      /* block ID=1 (constant value)            */
  
  /* block length: symb & layer header + packet 1 hdr + num characters  */
  block_len = (unsigned int)(SYMB_SIZE + LYR_HDR_SIZE + PKT_HDR_SIZE + num_chars);
  RPGC_set_product_int( (void *) &sym.block_length, block_len );

  sym.num_layers = (short)1;    /* number of data layers included         */
  
  lyr.divider2 = (short)-1;     /* block divider (constant value)         */
  
  /* first layer length: block_length - sym block length                */
  RPGC_set_product_int( (void *) &lyr.layer_length,
              (unsigned int)(block_len - SYMB_SIZE - LYR_HDR_SIZE) );

  
  /* set packet 1 header */
  /*--------------------------------------------------------------------*/ 
  pkt.code = (short)1;
  /* length of data is num_chars + 4bytes for the I and J position */
  pkt.num_bytes = num_chars+4;
  pkt.pos_i = (short)270;
  pkt.pos_j = (short)270;


  /*copy symbology header and layer 1 header to product                 */
  /*--------------------------------------------------------------------*/ 
  /* Sample Alg 1.17 */
  /* OLD: previously the structure containing the symb header and the
   *      the layer header could be copied as a block
   */
  /* NEW: the separate symb header and layer header structures have
   *      alignment issues and some data fields must be copied separately  
   */
  memcpy(outbuffer + SYMB_OFFSET, &sym, SYMB_SIZE);
  memcpy(outbuffer + SYMB_OFFSET + SYMB_SIZE, &lyr.divider2, 2);
  memcpy(outbuffer + SYMB_OFFSET + SYMB_SIZE + 2, &lyr.layer_length, 4);


  /*  copy packet 1 header and character data to product                */
  /*--------------------------------------------------------------------*/
  memcpy(outbuffer + SYMB_OFFSET + SYMB_SIZE + LYR_HDR_SIZE, 
                                                &pkt, PKT_HDR_SIZE);

  /* the packet 1 character data does NOT include the terminating Null */ 
  memcpy(outbuffer + SYMB_OFFSET + SYMB_SIZE + LYR_HDR_SIZE + PKT_HDR_SIZE, 
                                                     output_text, num_chars);
    
  /* return the total length of the product minus the Graphic_product   */
  /* structure length which is added in the RPGC_prod_hdr function       */ 
  *length = SYMB_SIZE + LYR_HDR_SIZE + PKT_HDR_SIZE + num_chars; 


  if(TEST) {
    
     unsigned short *short_ptr;

     /* print contents of symbology and first layer header for diagnostic */
     print_symbology_header( (outbuffer + SYMB_OFFSET) );


     /* print contents of data packet 1 for diagnostic */
     short_ptr = (unsigned short *)&pkt;
          
     fprintf(stderr,"\n");
     fprintf(stderr,"Packet 1.code is      %d(%d)\n",pkt.code,
                          short_ptr[0]);
     fprintf(stderr,"Packet 1.num_bytes is %d(%d)\n",pkt.num_bytes,
                          short_ptr[1]);
     fprintf(stderr,"Packet 1.pos_i is     %d\n",pkt.pos_i);
     fprintf(stderr,"Packet 1.pos_j is     %d\n",pkt.pos_j);
     fprintf(stderr,"Input String Length is %d, Characters in Product is %d\n",
                                  strlen(text_data), num_chars);
     fprintf(stderr,"Output Character Data is:\n'");
     for(i = 0; i < num_chars; i++)
         fprintf(stderr, "%c", output_text[i]);
     fprintf(stderr,"'\n");
    
  }
    
    
}



/************************************************************************/
void finish_prod_desc_block(char *outbuffer, short elev_index, 
                            short elevation, int prod_len) {
    
  /* complete entering values into the product description block (pdb)
     Enter: threshold levels, product dependent parameters, version and
     block offsets for symbology, graphic and tabular attribute.        */

  /* cast the msg header and pdb block to the pointer hdr               */
  Graphic_product* hdr=(Graphic_product*)outbuffer;

  /* enter the data threshold values                                    */
  hdr->level_1 = (short)0;
  hdr->level_2 = (short)0;
  hdr->level_3 = (short)0;
  hdr->level_4 = (short)0;
  hdr->level_5 = (short)0;
  hdr->level_6 = (short)0;
  hdr->level_7 = (short)0;
  hdr->level_8 = (short)0;
  hdr->level_9 = (short)0;
  hdr->level_10 = (short)0;
  hdr->level_11 = (short)0;
  hdr->level_12 = (short)0;
  hdr->level_13 = (short)0;
  hdr->level_14 = (short)0;
  hdr->level_15 = (short)0;
  hdr->level_16 = (short)0;

  /* product dependent parameters 
   * these will follow the base products where  
   * halfword 30 (pdp3)=elev angle
   */
  hdr->param_3 = (short)elevation;


  /* number of blocks in product = 3                                    */
  hdr->n_blocks=(short)3;

  /* message length                                                     */
  /* BUILD 6 LINUX change                                               */
  RPGC_set_product_int( (void *) &hdr->msg_len, 
                        (unsigned int) prod_len );

  /* ICD block offsets                                                  */

  RPGC_set_product_int( (void *) &hdr->sym_off, 60 );
  RPGC_set_product_int( (void *) &hdr->gra_off, 0 );
  RPGC_set_product_int( (void *) &hdr->tab_off, 0 );

  /* elevation index                                                    */
  hdr->elev_ind=(short)elev_index;

  return;
    
} /* end finish_prod_desc_block */



/************************************************************************/
void clear_buffer(char *buffer, int bufsize) {
  /* zero out the input buffer */
  int i;

  for(i = 0; i < bufsize; i++)
    buffer[i] = 0;

  return;
  
}

