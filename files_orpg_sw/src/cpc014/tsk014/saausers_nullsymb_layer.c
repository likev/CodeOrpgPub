/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:40:38 $
 * $Id: saausers_nullsymb_layer.c,v 1.4 2014/03/18 14:40:38 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saausers_nullsymb_layer.c

Description:   this module is used to construct the null product symbology block 
	       and the data layer packet. This module is used for the SAA.  The
               output will be in accordance with packet type 1.
               
Input:          int msg_type     pointer to message to be displayed 
                int *length      pointer to a length accumulator variable
                int prod_id......product code (150,151)
                int max_bufsize  maximum size allocated within outbuf
                
Output:         int *length      is used to return the accumulated length of
                                 the constructed product
                short *buffer    pointer to the output buffer where the text
                                 data is stored
                
Returns:        a success or error code
                
Globals:        none

Notes:          

CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, September 2003
               
History:
               Initial implementation 09/08/03 - Zittel

*******************************************************************************/

/* local include file */
#include "saausers_nullsymb_layer.h"

short build_nullsymbology_layer(short* buffer,int msg_type,
   int *length,int prod_id,int max_bufsize) {

  /* buffer entry points (offsets) ---------------------------------------
    bytes shorts
       0     0    message header block (120 bytes or 60 shorts)
     119    60    symbology block entry (16 bytes or  8 shorts) 
     135    68    packet 0001 header    ( 4 bytes or  2 shorts)
     137    70    layer data entry
  -----------------------------------------------------------------------*/
  
   /* variable declarations and definitions ---------------------------- */
   sym_block sym;         /* symbology block struct                     */
   packet_1_layer_t pkt1; /* packet 01 header struct                    */
   int offset=0;          /* pointer offset into output buffer          */
   int block_length;      /* temporary storage for block length         */
   int layer_length;      /* temporary storage for layer length         */
   int debugit=FALSE;       /* test flag: set to true for diagnostics     */
 
/*  Add the size of the product description block & symbology layer to the offset */
    offset +=68; 
    
   /* NOTE: the output buffer will begin at index 1                      */
   if(debugit)
      fprintf(stderr,
        "SAA: inside build nullsymbology layer, msg_type = %d\n",msg_type);
   
   /* compile the packet 1 layer header                                  */
   pkt1.pcode=(short)0x0001; /* packet code 0x0001                       */
   pkt1.num_bytes = BPR + 4; /* Length of Data Block in bytes            */
   pkt1.istart=ICENTER;      /* i center of display (ramtec values)      */
   pkt1.jstart=JCENTER;      /* j center of display (ramtec values)      */

   sprintf(pkt1.data, "%s",err);
   if(debugit){fprintf(stderr,"message = %s \n",pkt1.data);}
   memcpy(buffer+offset,&pkt1,sizeof(packet_1_layer_t));
   offset+=sizeof(packet_1_layer_t)/2;
   if(debugit){fprintf(stderr,"offset = %d\n",offset);}

   /* the total length of the product minus the Graphic_product          */
   /* structure length (120 bytes or 60 shorts) which is added in the    */
   /* RPG_prod_hdr function                                              */
   *length= 68*2 + sizeof(packet_1_layer_t);
   if(debugit){fprintf(stderr,"Length = %d\n",*length);}

   pkt1.jstart+=7;      /* j center of display (ramtec values)      */
   msg_type = abs(msg_type);
   sprintf(pkt1.data, "%s",no_data_msg[msg_type]);
   if(debugit){fprintf(stderr,"message = %s \n",pkt1.data);}
   memcpy(buffer+offset,&pkt1,sizeof(packet_1_layer_t));
   offset+=sizeof(packet_1_layer_t)/2;
   if(debugit){fprintf(stderr,"offset = %d\n",offset);}

   *length += sizeof(packet_1_layer_t);
   if(debugit){fprintf(stderr,"Length = %d\n",*length);}

   /* compile the null symbology block header to be inserted at pos ptr+60    */
   if(debugit) fprintf(stderr,"building null symbology block now\n");
   sym.divider=(short)-1;      /* block divider (constant value)         */
   sym.block_id=(short)1;      /* block ID=1 (constant value)            */
   block_length=*length-120;   /* length of whole symbology block        */
   RPGC_set_product_int( (void *) &sym.block_length, block_length );
   sym.num_layers=(short)1;    /* number of data layers included         */
   sym.divider2=(short)-1;     /* block divider (constant value)         */
   /* layer length: block_length - 16 bytes (sym block length)           */
   layer_length = block_length - 16;
   RPGC_set_product_int( (void *) &sym.layer_length, layer_length );
   if(debugit){fprintf(stderr,"symb_layer = %d\n",layer_length);}
   /* copy the symbology block into the output buffer                    */
   if(debugit) fprintf(stderr,"copy null symbology block to buffer now\n");
   memcpy(buffer+60,&sym,16);

   if(debugit)
      fprintf(stderr,"Symbology Block Generation Complete: block length=%d offset=%d\n",
              sym.block_length,*length);

   return(SAA_SUCCESS);
   }
