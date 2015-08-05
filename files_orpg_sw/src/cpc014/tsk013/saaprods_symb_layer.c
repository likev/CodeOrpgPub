/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 21:30:18 $
 * $Id: saaprods_symb_layer.c,v 1.6 2008/01/04 21:30:18 aamirn Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaprods_symb_layer.c

Description:   this module contains two routines. The first is used to
               construct the product symbology block and the data layer
               packet. The second is used to complete the construction of
               the product description block (which is a part of the ICD
               product header). This module is used for the six Snow 
               Accumulation Algorithm products. The output will be
               in accordence with AF1F packet type run length encoding.
                
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
	Initial implementation 8/08/03 - Zittel
	11/04/2004	SW CCR NA04-30810	Build8 changes

*******************************************************************************/

/* local include file */
#include "saaprods_main.h"
#include "saaprods_symb_layer.h"

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
Notes:          none

************************************************************************/
   
/*  New function tailored to SAA products  */

short build_saa_symbology_layer(short* buffer,int count,
   int *length,short radial_data[360][MAX_SAA_BINS],int prod_id,int max_bufsize) {
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
   int debugit=FALSE;        /* test flag: set to true for diagnostics     */
   int num_bins;           /* number of bins to process per radial       */
   int temp;               /* temporary storage                          */
   
   num_bins=MAX_SAA_BINS;

   /* NOTE: the output buffer will begin at index 0                      */
   if(debugit)
      fprintf(stderr,
        "SDT: inside build symbology layer. offset =%d\n",offset);
   
   if(debugit) {
      /* prints out first radial of raw data */
      for(i=0;i<MAX_SAA_BINS;i++) 
         fprintf(stderr," %02X",(unsigned)radial_data[0][i]);
      fprintf(stderr,"\nnbins=%hd\n",num_bins);
      }
   
   /* compile the packet AF1F layer header                               */
   hdr.pcode=(short)0xAF1F;  /* packet code AF1F                         */
   hdr.first_range_bin=0;    /* distance to the first range bin          */
   hdr.num_bins=num_bins;    /* number of bins in each radial            */
   hdr.icenter=ICENTER;      /* i center of display (ramtec values)      */
   hdr.jcenter=JCENTER;      /* j center of display (ramtec values)      */
   hdr.scalefactor=230/num_bins; /* scale factor                  */
   hdr.num_radials=count;    /* number of radials included               */

   /* load the packet layer header into the buffer. buffer offset=68     */
   /* header size=14 bytes                                               */
   memcpy(buffer+68,&hdr,14);

   /* display beginning of the data buffer                               */
   if(debugit) {
     for(i=0;i<160;i++)
       fprintf(stderr,"%04x ",(unsigned short)buffer[i]);
     fprintf(stderr,"\n\n");
     }

   /* set offset to beginning of data layer in halfwords                 */
   offset=75;

   /* process for each radial                                            */
   for(i=0;i<count;i++) {
      /* cast a short pointer to each raw base data radial               */
      short* sptr=(short*)radial_data[i]; 
      /* construct the AF1F radial header which consists of the number of */
      /* rle halfwords in the radial...the start angle and angle delta.   */
      /* get a handle to access the radial structure from the radial array*/
   
      if(debugit){
         fprintf(stderr,"\n-> processing for i=%03i angle %4.1f\n",
            i, (float)i);
/*         for(j =0;j<115;++j){ */
/*           fprintf(stderr,"%3d",radial_data[i][j]); */
/*           if((j % 22) == 0) */
/*             fprintf(stderr,"\n"); */
/*           } */
         }
      
      /* start the run length encoding process                            */
      {
      int start_angle=i*10;                    /* radial start angle      */
      int angle_delta=10;                      /* radial angle delta      */
      int start_index=0;                       /* RLE start index         */
      int end_index=num_bins-1;                /* RLE end index           */
      int num_data_bins=num_bins-1;            /* max number of data bins */
      int buff_step=1;                         /* RLE compression step    */
      int ctable_index=23;                     /* color table index       */
      int buf_index=offset;                    /* offset into the buffer  */

      /* run length encode now. return number of bytes encoded            */
      num_rle_bytes=radial_run_length_encode(start_angle,angle_delta,sptr,
         start_index,end_index,num_data_bins,buff_step,ctable_index,
         buf_index, buffer,prod_id);
      
      /* calculate the number of short integers encoded                   */
      num_shorts=num_rle_bytes/2;
      
      if(debugit) {
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
      if((offset*2) + TAB_CUSHION >= max_bufsize) {
         RPGC_log_msg( GL_ERROR,
                 "SAA: symb+tab_cushion (%d) exceeds op buff size\n",
                  (offset*2) + TAB_CUSHION);
         return(SAA_FAILURE);
         }
      
      /* advance the radial pointer by one                              */
   } /* end of i/processing loop ====================================== */

   /* the total length of the product minus the Graphic_product          */
   /* structure length (120 bytes or 60 shorts) which is added in the    */
   /* RPG_prod_hdr function                                              */
   *length=(offset-60)*2;


   /* compile the symbology block header to be inserted at pos ptr+60    */
   if(debugit) fprintf(stderr,"building symbology block now\n");
   sym.divider=(short)-1;      /* block divider (constant value)         */
   sym.block_id=(short)1;      /* block ID=1 (constant value)            */
   temp = *length;             /* length of whole symbology block        */
   RPGC_set_product_int( (void *) &sym.block_length, temp );  /* length of whole symbology block        */
   sym.num_layers=(short)1;    /* number of data layers included         */
   sym.divider2=(short)-1;     /* block divider (constant value)         */
   /* layer length: block_length - 16 bytes (sym block length)           */
   RPGC_set_product_int( (void *) &sym.layer_length, temp-16 );

   /* copy the symbology block into the output buffer                    */
   if(debugit) fprintf(stderr,"copy symbology block to buffer now\n");
   memcpy(buffer+60,&sym,16);

   /* for TAB - reset offset by adding 120 bytes back to length */
   *length+=120;
   if(debugit)
      fprintf(stderr,"Symbology Block Generation Complete: block length=%d offset=%d\n",
      temp,*length);

   return(SAA_SUCCESS);
   }

void finish_SDT_pdb(short* buffer,short rpg_elev_ind,short hi_sf_rca_flgs,
   int prod_len,int tab_offset,int prod_id) {

   int debugit = FALSE;
   extern int hi_sf_flg;

  /* complete entering values into the product description block (pdb)
     Enter: threshold levels, product dependent parameters, version and
     block offsets for symbology, graphic and tabular attribute.        */

  /* cast the msg header and pdb block to the pointer hdr               */
  Graphic_product* hdr=(Graphic_product*)buffer;

  if(debugit){
     fprintf(stderr,"Entering the Product Description Block\n");
     fprintf(stderr,"->len=%d tab_offset=%d\n",prod_len,tab_offset);
   }

   /* enter the data threshold values                                    */
   /* NOTE: data level thresholds correspond to the categories defined   */
   /* within recclprods_color_table.c. However, the color table array    */
   /* indices start at zero while the product description block level    */
   /* thresholds begin with 1.                                           */

   /*  Added test for hi_sf_flg for Build8  */   
   if(prod_id == SSDACCUM && hi_sf_flg == FALSE){
   if(debugit)
     fprintf(stderr,"Using SSDACCUM Depth for product Thresholds\n");
	hdr->level_1=(short)0x8002;    /* code for ND - no data       */
   	hdr->level_2=(short)0x1800;
   	hdr->level_3=(short)0x1005;
   	hdr->level_4=(short)0x100A;
   	hdr->level_5=(short)0x1014;
   	hdr->level_6=(short)0x101E;
   	hdr->level_7=(short)0x1028;
   	hdr->level_8=(short)0x1032;
   	hdr->level_9=(short)0x103C;
   	hdr->level_10=(short)0x1050;
   	hdr->level_11=(short)0x1064;
   	hdr->level_12=(short)0x1078;
   	hdr->level_13=(short)0x1096;
   	hdr->level_14=(short)0x10C8;
   	hdr->level_15=(short)0x10FA;
   	hdr->level_16=(short)0x081E;
   }   
   /*  Added test for hi_sf_flg for Build8, new table for hi_sf_flg = TRUE  */   
   if(prod_id == SSDACCUM && hi_sf_flg == TRUE){
   	if(debugit)
     		fprintf(stderr,"Using SSDACCUM Depth for product Thresholds\n");
   	hdr->level_1=(short)0x8002;    /* code for ND - no data       */
   	hdr->level_2=(short)0x0800;
   	hdr->level_3=(short)0x0001;
   	hdr->level_4=(short)0x0002;
   	hdr->level_5=(short)0x0004;
   	hdr->level_6=(short)0x0006;
   	hdr->level_7=(short)0x0008;
   	hdr->level_8=(short)0x000A;
   	hdr->level_9=(short)0x000C;
   	hdr->level_10=(short)0x0010;
   	hdr->level_11=(short)0x0014;
   	hdr->level_12=(short)0x0018;
   	hdr->level_13=(short)0x001E;
   	hdr->level_14=(short)0x0028;
   	hdr->level_15=(short)0x0032;
   	hdr->level_16=(short)0x083C;
   }
   /*  Added test for hi_sf_flg for Build8  */   
   if(prod_id == SSWACCUM && hi_sf_flg == FALSE){
   	if(debugit)
     		fprintf(stderr,"Using SSWACCUM water equivalent for product Thresholds\n");
   	hdr->level_1=(short)0x8002;    /* code for ND - no data               */
   	hdr->level_2=(short)0x2800;
   	hdr->level_3=(short)0x2001;
   	hdr->level_4=(short)0x2002;
   	hdr->level_5=(short)0x2003;
   	hdr->level_6=(short)0x2004;
   	hdr->level_7=(short)0x2005;
   	hdr->level_8=(short)0x2006;
   	hdr->level_9=(short)0x2008;
   	hdr->level_10=(short)0x200A;
   	hdr->level_11=(short)0x200F;
   	hdr->level_12=(short)0x2014;
   	hdr->level_13=(short)0x2019;
   	hdr->level_14=(short)0x201E;
   	hdr->level_15=(short)0x2028;
   	hdr->level_16=(short)0x2832;
   }   
   /*  Added test for hi_sf_flg for Build8, new table for hi_sf_flg = TRUE  */   
   if(prod_id == SSWACCUM && hi_sf_flg == TRUE){
   	if(debugit)
     		fprintf(stderr,"Using SSWACCUM water equivalent for product Thresholds\n");
   	hdr->level_1=(short)0x8002;    /* code for ND - no data               */
   	hdr->level_2=(short)0x1800;
   	hdr->level_3=(short)0x1001;
   	hdr->level_4=(short)0x1002;
   	hdr->level_5=(short)0x1003;
   	hdr->level_6=(short)0x1004;
   	hdr->level_7=(short)0x1005;
   	hdr->level_8=(short)0x1006;
   	hdr->level_9=(short)0x1008;
   	hdr->level_10=(short)0x100A;
   	hdr->level_11=(short)0x100F;
   	hdr->level_12=(short)0x1014;
   	hdr->level_13=(short)0x1019;
   	hdr->level_14=(short)0x101E;
   	hdr->level_15=(short)0x1028;
   	hdr->level_16=(short)0x1832;
   }   
   if(prod_id == OSWACCUM ){
   if(debugit)
      fprintf(stderr,"Using OSWACCUM water equivalent for product Thresholds\n");
   hdr->level_1=(short)0x8002;    /* code for ND - no data               */
   hdr->level_2=(short)0x4800;
   hdr->level_3=(short)0x4001;
   hdr->level_4=(short)0x4002;
   hdr->level_5=(short)0x4003;
   hdr->level_6=(short)0x4005;
   hdr->level_7=(short)0x4007;
   hdr->level_8=(short)0x4009;
   hdr->level_9=(short)0x400B;
   hdr->level_10=(short)0x400D;
   hdr->level_11=(short)0x4010;
   hdr->level_12=(short)0x4014;
   hdr->level_13=(short)0x4019;
   hdr->level_14=(short)0x401E;
   hdr->level_15=(short)0x4023;
   hdr->level_16=(short)0x4828;
   } 
   if(prod_id == OSDACCUM ){
   if(debugit)
      fprintf(stderr,"Using OSDACCUM Depth for product Thresholds\n");
   hdr->level_1=(short)0x8002;    /* code for ND - no data               */
   hdr->level_2=(short)0x2800;
   hdr->level_3=(short)0x2001;
   hdr->level_4=(short)0x2002;
   hdr->level_5=(short)0x2003;
   hdr->level_6=(short)0x2005;
   hdr->level_7=(short)0x200A;
   hdr->level_8=(short)0x200F;
   hdr->level_9=(short)0x2014;
   hdr->level_10=(short)0x201E;
   hdr->level_11=(short)0x2028;
   hdr->level_12=(short)0x2032;
   hdr->level_13=(short)0x203C;
   hdr->level_14=(short)0x2046;
   hdr->level_15=(short)0x2050;
   hdr->level_16=(short)0x2864;
   }   
   /* product dependent parameters - only PDP 3 is used for the elevation */
   hdr->param_3=(short)hi_sf_rca_flgs;

   /* number of blocks in product = 4 (MHB/PDB/SYM/TAB)                  */
   hdr->n_blocks=(short)4;

   /* overall message length (including TAB block)                       */
   RPGC_set_product_int( (void *) &hdr->msg_len, prod_len );

   /* ICD block offsets (in short integers)                               */
   RPGC_set_product_int( (void *) &hdr->sym_off, 60 );  /* symbology offset                 */
   RPGC_set_product_int( (void *) &hdr->gra_off, 0 );   /* no graphic alpha block           */
   RPGC_set_product_int( (void *) &hdr->tab_off, (int)(tab_offset/2) );  /* tabular alpha block offset       */
   
   /* elevation index                                                     */
   hdr->elev_ind=(short)rpg_elev_ind;

   return;
   }
