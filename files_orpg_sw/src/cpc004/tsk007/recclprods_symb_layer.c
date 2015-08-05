/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/01/12 16:24:31 $
 * $Id: recclprods_symb_layer.c,v 1.7 2004/01/12 16:24:31 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/*******************************************************************************
Module:        recclprods_symb_layer.c

Description:   this module contains two routines. The first is used to
               construct the product symbology block and the data layer
               packet. The second is used to complete the construction of
               the product description block (which is a part of the ICD
               product header). This module is used for both the Doppler
               and reflectivity REC algorithm products. The output will be
               in accordence with AF1F packet type run length encoding.
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               2/21/02 - ADS - modified the finish_pdb module to correct
                 an error in the data thresholds. level_12 was formerly set
                 to be the 100% threshold. This has been removed and the 
                 remaining color levels were moved down to fill in the rest
                 of the table.
               4/9/02 - ADS - modified the number of bins to be included
                 in each RLE radial by using the n_bins field from the
                 intermediate product header rather than a fixed 230 bins.
$Id: recclprods_symb_layer.c,v 1.7 2004/01/12 16:24:31 steves Exp $
*******************************************************************************/

/* local include file */
#include "recclprods_symb_layer.h"


/*******************************************************************************
Description:    build_symbology_layer constructs the symbology block
                which encapsulates one or more data layers within the
                formatted output product. the function also builds the
                data layer using run length encoding.
                
Input:          char *inbuf      pointer to the input buffer (for header info)
                int count        number of radials in this elevation
                int *length      pointer to a length accumulator variable
                int prod_id......product code (132 for refl, 133 for dop)
                int max_bufsize  maximum size allocated within outbuf
                short radial_data[][MAX_1KMBINS] - 2D array holding 
                                 preprocessed data
                
Output:         int *length      is used to return the accumulated length of
                                 the constructed product
                short *buffer    pointer to the output buffer where the RLE
                                 data is stored
                
Returns:        a success or error code
                
Globals:        none
Notes:          products 132/133 do NOT require that the maximum values
                be tracked for inclusion in a product dependent parameter
************************************************************************/

short build_symbology_layer(short* buffer,char *inbuf,int count,
   int *length,short radial_data[][MAX_1KMBINS],int prod_id,int max_bufsize) {
  /* build the symbology block and 16-level RLE AF1F data packet. */

  /* buffer entry points (offsets) ---------------------------------------
    bytes shorts
       0     0    message header block (120 bytes or 60 shorts)
     119    60    symbology block entry (16 bytes or  8 shorts) 
     135    68    packet AF1F header    (14 bytes or  7 shorts)
     148    75    layer data entry
  -----------------------------------------------------------------------*/
  
   /* variable declarations and definitions ---------------------------- */
   sym_block sym;          /* symbology block struct                     */
   packet_AF1F_hdr hdr;    /* packet AF1F header struct                  */
   int i,j;                /* loop counters                              */
   int offset=0;           /* pointer offset into output buffer          */
   int num_rle_bytes;      /* number of bytes encoded by rle routine     */
   int num_shorts=0;       /* number of short integers run length encoded*/
   int DEBUG=FALSE;        /* test flag: set to true for diagnostics     */
   Rec_prod_header_t *phdr;/* pointer to the intermediate product header */
   int num_bins;           /* number of bins to process per radial       */
   

   /* acquire a pointer to the intermediate product & radial header      */
   int hdr_offset=sizeof(Rec_prod_header_t);
   Rec_rad_header_t *rptr=(Rec_rad_header_t*)(inbuf+hdr_offset);
   phdr=(Rec_prod_header_t*)inbuf;
   num_bins=phdr->n_bins;
   if(prod_id==RECCLDOP)
      num_bins/=4;
    
   /* NOTE: the output buffer will begin at index 1                      */
   if(DEBUG)
      fprintf(stderr,
        "REC: inside build symbology layer. num of radials=%d num bins %d\n",
        count,num_bins);
   
   if(DEBUG) {
      /* prints out first radial of raw data */
      for(i=0;i<MAX_1KMBINS;i++) 
         fprintf(stderr," %02X",(unsigned)radial_data[0][i]);
      fprintf(stderr,"\nnbins=%hd vcp=%hd elev_num=%hd\n",phdr->n_bins,
         phdr->vcp_num,phdr->elev_num);
      }
   
   /* compile the packet AF1F layer header                               */
   hdr.pcode=(short)0xAF1F;  /* packet code AF1F                         */
   hdr.first_range_bin=0;    /* distance to the first range bin          */
   hdr.num_bins=num_bins;    /* number of bins in each radial            */
   hdr.icenter=ICENTER;      /* i center of display (ramtec values)      */
   hdr.jcenter=JCENTER;      /* j center of display (ramtec values)      */
   hdr.scalefactor=phdr->cos_ele_short; /* scale factor                  */
   hdr.num_radials=count;    /* number of radials included               */

   /* load the packet layer header into the buffer. buffer offset=68     */
   /* header size=14 bytes                                               */
   memcpy(buffer+68,&hdr,14);

   /* display beginning of the data buffer                               */
   if(DEBUG) {
     for(i=0;i<160;i++)
       fprintf(stderr,"%04x ",(unsigned short)buffer[i]);
     fprintf(stderr,"\n\n");
     }

   /* set offset to beginning of data layer                              */
   offset=75;

   /* process for each radial                                            */
   for(i=0;i<count;i++) {
      /* cast a short pointer to each raw base data radial               */
      short* sptr=(short*)radial_data[i]; 
      
      /* construct the AF1F radial header which consists of the number of */
      /* rle halfwords in the radial...the start angle and angle delta.   */
      /* get a handle to access the radial structure from the radial array*/
   
      if(DEBUG)
         fprintf(stderr,"\n-> processing for i=%03i angle %4.1f\n",
            i,rptr->start_angle/10.0);
      
      /* start the run length encoding process                            */
      {
      int start_angle=(int)rptr->start_angle;  /* radial start angle      */
      int angle_delta=(int)rptr->delta_angle;  /* radial angle delta      */
      int start_index=0;                       /* RLE start index         */
      int end_index=num_bins-1;                /* RLE end index           */
      int num_data_bins=num_bins-1;            /* max number of data bins */
      int buff_step=1;                         /* RLE compression step    */
      int ctable_index=1;                      /* color table index       */
      int buf_index=offset;                    /* offset into the buffer  */

      /* run length encode now. return number of bytes encoded            */
      num_rle_bytes=radial_run_length_encode(start_angle,angle_delta,sptr,
         start_index,end_index,num_data_bins,buff_step,ctable_index,
         buf_index, buffer,prod_id);
      
      /* calculate the number of short integers encoded                   */
      num_shorts=num_rle_bytes/2;
      
      if(DEBUG) {
         fprintf(stderr,"\nAfter radial run length: number of rle bytes=%i\n",
            num_rle_bytes);
         fprintf(stderr,"number of rle shorts=%i offset start=%d end=%d\n",
            num_shorts,offset,offset+num_shorts);
         for(j=offset;j<offset+num_shorts;j++)
            fprintf(stderr,"%04x ",(unsigned short)buffer[j]);
         fprintf(stderr,"\n\n\n");
         }
      
      } /* end of rle local block */

      /* increment the pointer offset for the output buffer by the      */
      /* number of short integers encoded                               */
      offset+=num_shorts;
      
      /* check to make sure that the offset does not go beyond the bounds*/
      if((offset*2)>=max_bufsize) {
         RPGC_log_msg( GL_ERROR,
                 "REC symb_block error: offset exceeded outbuf size\n");
         return(REC_FAILURE);
         }
      
      /* advance the radial pointer by one                              */
      rptr++;
   } /* end of i/processing loop ====================================== */

   /* the total length of the product minus the Graphic_product          */
   /* structure length (120 bytes or 60 shorts) which is added in the    */
   /* RPG_prod_hdr function                                              */
   *length=(offset-60)*2;


   /* compile the symbology block header to be inserted at pos buffer+60 */
   if(DEBUG) fprintf(stderr,"building symbology block now\n");
   sym.divider=(short)-1;      /* block divider (constant value)         */
   sym.block_id=(short)1;      /* block ID=1 (constant value)            */
   /* length of whole symbology block                                    */
   RPGC_set_product_int( (void *) &sym.block_length, *length );   
   sym.num_layers=(short)1;    /* number of data layers included         */
   sym.divider2=(short)-1;     /* block divider (constant value)         */
   /* layer length: block_length - 16 bytes (sym block length)           */
   RPGC_set_product_int( (void *) &sym.layer_length, (*length)-16 );

   /* copy the symbology block into the output buffer                    */
   if(DEBUG) fprintf(stderr,"copy symbology block to buffer now\n");
   memcpy(buffer+60,&sym,16);

   /* for TAB - reset offset by adding 120 bytes back to length */
   *length+=120;
   if(DEBUG)
      fprintf(stderr,"Symbology Block Generation Complete: block length=%d offset=%d\n",
      sym.block_length,*length);

   return(REC_SUCCESS);
   }



/************************************************************************
Description:    finish_pdb fills in values within the Graphic_product
                structure which were not filled in during the system call
                
Input:          short *buffer:          pointer to the output buffer
                short rpg_elev_index:   observed rpg elevation index
                short target_elevation: target elevation
                int   prod_len:         length of entire product (minus
                                        the pre-icd header)
                int   tab_offset:       offset to beginning of TAB block
                int prod_id:            product ID (refl or dop)
Output:         short *buffer:          completed header blocks within buffer
Returns:        none
Globals:        none
Notes:          prod 132 has 11 data level thresholds whild prod 133 has 12
                due to the inclusion of RF (range folding)
************************************************************************/
void finish_pdb(short* buffer,short rpg_elev_ind,short target_elevation,
   int prod_len,int tab_offset,int prod_id) {
   int DEBUG = FALSE;

  /* complete entering values into the product description block (pdb)
     Enter: threshold levels, product dependent parameters, version and
     block offsets for symbology, graphic and tabular attribute.        */

  /* cast the msg header and pdb block to the pointer hdr               */
  Graphic_product* hdr=(Graphic_product*)buffer;

  if(DEBUG){
     fprintf(stderr,"Entering the Product Description Block\n");
     fprintf(stderr,"->len=%d tab_offset=%d\n",prod_len,tab_offset);
   }

   /* enter the data threshold values                                    */
   /* NOTE: data level thresholds correspond to the categories defined   */
   /* within recclprods_color_table.c. However, the color table array    */
   /* indices start at zero while the product description block level    */
   /* thresholds begin with 1.                                           */
   hdr->level_1=(short)0x8002;    /* code for ND - no data               */
   hdr->level_2=(short)0;
   hdr->level_3=(short)10;
   hdr->level_4=(short)20;
   hdr->level_5=(short)30;
   hdr->level_6=(short)40;
   hdr->level_7=(short)50;
   hdr->level_8=(short)60;
   hdr->level_9=(short)70;
   hdr->level_10=(short)80;
   hdr->level_11=(short)90;
   if(prod_id==RECCLREF)
      hdr->level_12=(short)0x8000;  /* Blank field for Refl version */
    else
      hdr->level_12=(short)0x8003;  /* Range Fold (RF) for Doppler version */
   hdr->level_13=(short)0x8000;
   hdr->level_14=(short)0x8000;     /* last four are not used here */
   hdr->level_15=(short)0x8000;
   hdr->level_16=(short)0x8000;
   
   /* product dependent parameters - only PDP 3 is used for the elevation */
   hdr->param_3=(short)target_elevation;

   /* number of blocks in product = 4 (MHB/PDB/SYM/TAB)                  */
   hdr->n_blocks=(short)4;

   /* overall message length (including TAB block)                       */
   RPGC_set_product_int( (void *) &hdr->msg_len, (unsigned int) prod_len );

   /* ICD block offsets (in short integers)                               */
   /* symbology offset                                                    */
   RPGC_set_product_int( (void *) &hdr->sym_off, (unsigned int) 60 );                   
   /* no graphic alpha block                                              */
   RPGC_set_product_int( (void *) &hdr->gra_off, 0 );                    
   /* tabular alpha block offset                                          */
   RPGC_set_product_int( (void *) &hdr->tab_off, (unsigned int) (tab_offset/2) ); 
   
   /* elevation index                                                     */
   hdr->elev_ind=(short)rpg_elev_ind;

   return;
   }


