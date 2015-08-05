/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2009/12/14 19:19:00 $ */
/* $Id: hhc8bit.c,v 1.4 2009/12/14 19:19:00 ccalvert Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#include "hhc8bit.h"
#include <qperate_Consts.h>
#include <qperate_types.h>
#include <hca.h>

#define DEBUG_AZ -1 /*Set to positive azimuth value for debug output */

/* Map of internal hydro class numbers to external (i.e. product) numbers    */
/* Index this array with the internal hydro class number (e.g. RA, HA, etc.) */
/* to obtain the hydro class numbers used in the HC products.                */

#define MISSING_DATA    140
/* These external hydro class codes are scaled by 10 to leave room for future classes */
                    /* U0 U1  RA  HR   RH  BD  BI  GC  DS  WS  IC  GR   UK ND */
static float Class_external[NUM_CLASSES] = 
                       {0.,0.,60.,70.,100.,80.,10.,20.,40.,50.,30.,90.,140.,0.};

static float Scale = 1;
static float Offset = 0;
static int Totalbytes = 0;
       float Percent_filled;
       float Highest_angle;
       short Mode_filter_size;

/*********************************************************************

   Description:
      Buffer control module for a hybrid hydroclass 8 bit base product.

   Returns:
      Currently, always returns 0.

*********************************************************************/
int Hhc8bit_buffer_control(){

    int opstat;

    char *obrptr = NULL;
    char *qpedataptr = NULL;

    /* Initialization. */
    opstat = 0;
    Proc_rad = 1;

    /* Acquire output buffer for 256 data level  
       product, 0 to 230 km. Range (0.25 km X 1deg resolution). */
    obrptr = RPGC_get_outbuf_by_name( OUTDATA_NAME, OBUF_SIZE, &opstat );

    if( opstat == NORMAL ){

      /* Request input buffer from qperate and process it. */
	 qpedataptr = RPGC_get_inbuf_by_name( INDATA_NAME, &opstat);

      if( opstat == NORMAL ){

         /* Call the product generation control routine. */
         Hhc8bit_product_generation_control( obrptr, qpedataptr );

         /* Send the product out */
         RPGC_rel_outbuf( obrptr, FORWARD|RPGC_EXTEND_ARGS, Totalbytes);

         /* Release the input buffer. */
         RPGC_rel_inbuf( qpedataptr );

      } 
      else {
         /* If the input data stream has been canceled for some reason, 
            then release and destroy the product buffer, and return to 
            wait for activation. */
         RPGC_rel_outbuf( obrptr, DESTROY );
         RPGC_log_msg(GL_INFO,"Input data stream canceled!");
         RPGC_abort();

	 }/* end if opstat == NORMAL */
    } /* end if opstat == NORMAL */
    else {
       RPGC_log_msg(GL_INFO,"No available output buffers!");
       RPGC_abort_because( opstat );
    }
	 
    /* Return to wait for activation. */
    return 0;

/* End of Hhc8bit_buffer_control() */
} 


/********************************************************************* 

   Description:
      Controls the processing of data for the 256-level HHC product. 

   Inputs:
      obrptr - pointer to product buffer.
      qpedataptr - pointer to input message.

   Returns:
      Always returns 0.

******************************************************************* */
int Hhc8bit_product_generation_control( char *obrptr, char *qpedataptr ){

    /* Local variables */
    int   volnumber, vcpnumber; 
    int   ndpbyts, excess, i, r;
    int   itemp;

    Scan_Summary *summary = NULL;
    HHC_Buf_t *hhcdata = (HHC_Buf_t *) qpedataptr;

    /* Static variables. */
    static int              pbuffind = 0, ndpb = 0;
    static int              numbins = MAXBINS; 
    static short           *outbuff = NULL;
    static int              hc_shortbuf_size = 0;
    static short           *hc_shortbuf = NULL;


    /* Initialize the radial counter. */
    Radcount = 0;

    /* Initialize Buffer index counter and number of packet bytes. */
    pbuffind = ndpb = ndpbyts = 0;
    outbuff = (short *) (obrptr + sizeof(Graphic_product) + 
                         sizeof(Symbology_block) + sizeof(Packet_16_hdr_t));

    /* Get elevation angle from volume coverage pattern and elevation 
       index number. */
    volnumber = RPGC_get_buffer_vol_num( qpedataptr );
    summary   = RPGC_get_scan_summary( volnumber );
    vcpnumber = summary->vcp_number;

    /* Extract fields that become product dependent parameters */
    Mode_filter_size = hhcdata->mode_filter_length;
    Percent_filled   = hhcdata->pct_hybrate_filled;
    Highest_angle    = hhcdata->highest_elang;

    /* Pack product header fields. */
    Hhc8bit_product_header( obrptr, volnumber );

    /* Perform individual radial processing */
    for (r = 0; r < MAX_AZM; r++){

       /* Calculate remaining words available in output buffer. */
	  excess = OBUF_SIZE - (ndpb + 150);
	  if( excess > EST_PER_RAD ){
          short  *hhc_data     = NULL;

          if (strcmp(OUTDATA_NAME, "HHC8BIT") == 0) {

             hhc_data = (short *)hhcdata->HybridHCA[r];

             if (hhc_data == NULL) {
                RPGC_log_msg(GL_ERROR,"Failed to obtain input data %s for %s",
                                       INDATA_NAME,OUTDATA_NAME);
                RPGC_abort_dataname_because("HHC8BIT", PGM_DISABLED_MOMENT);
                return 0;
             }

             /* We need to convert the internal HCA codes to external codes 
                according to the ICD.                                         */
             numbins = MAXBINS;
             if (numbins > hc_shortbuf_size) {
                if (hc_shortbuf != NULL)
                   free (hc_shortbuf);
                hc_shortbuf = MISC_malloc(numbins*sizeof(short));
                hc_shortbuf_size = numbins;
             }

             for (i=0; i < numbins; i++) {
                if (hhc_data[i] >= 0) {
                   itemp = hhc_data[i];
                   hc_shortbuf[i] = (short) Class_external[itemp];
                }
                else {
                   hc_shortbuf[i] = (short)MISSING_DATA;
                }
             }/* end for all bins */
          }/* if HHC8BIT */

          /* Increment the radial count. */
          Radcount++;
       
          if (strcmp(OUTDATA_NAME,"HHC8BIT") == 0) {

             /* This branch handles 1-byte HCA data. Modify the scale from the original
                because the Class_external array is scaled for future expansion.      */
             ndpbyts = RPGC_digital_radial_data_array( (void *) hc_shortbuf, RPGC_SHORT_DATA,
                                                        0, numbins-1, 0, numbins, 1, (float)r*10.,
                                                        10., (void *) &outbuff[pbuffind] );
          }

          if (Radcount == DEBUG_AZ) {
             for(i=303;i<337;i++){
                RPGC_log_msg(GL_INFO,"hhc_data[%d]=%d  hc_shortbuf=%d",
                                      i,(short)hhc_data[i],hc_shortbuf[i]);
             }
          }

          /* Update buffer counters and pointers. */
          ndpb += ndpbyts;
          pbuffind += ndpbyts / 2;

       } /* end if excess > EST_PER_RAD */

    } /* end for all radials */

    /* Fill remaining fields in product buffer. */
    Hhc8bit_end_of_product_processing( ndpb, numbins, obrptr );

    /* Return to buffer control routine. */
    return 0;

/* End of Hhc8bit_product_generation_control() */
} 


/********************************************************************
   
   Description:
      Fills in product description block, symbology block information.

   Inputs:
      obrptr - pointer to output buffer.
      vol_num - volume scan number.

   Returns:
      Always returns 0.

********************************************************************/
int Hhc8bit_product_header( char *obrptr, int vol_num ){

    Graphic_product *phd = (Graphic_product *) obrptr;
    Symbology_block *sym = (Symbology_block *) 
                                 (obrptr + sizeof(Graphic_product));

    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, Prod_id, vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    params[3] = Mode_filter_size;
    params[4] = (short) RPGC_NINT(Percent_filled * 100);
    params[5] = (short) RPGC_NINT(Highest_angle * 10);
    RPGC_set_dep_params( obrptr, params );
    if(DEBUG_AZ > 0) {
       RPGC_log_msg(GL_INFO,"Mode filter size = %d, filled = %d, highest angle = %d",
                              params[3], params[4], params[5]);
    }

    if (strcmp(OUTDATA_NAME,"HHC8BIT") == 0)
       phd->elev_ind = 0;
        
    /* Store offset to symbology. */
    RPGC_set_prod_block_offsets( phd, sizeof(Graphic_product)/sizeof(short), 0, 0 );

    /* Store product block divider, block ID, number of layers and 
       layer divider. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 1;
    sym->layer_divider = -1;

    /* Length of layer, length of block, length of message, scale factor,
       number of radials are stored in the end-of-product module. */
    return 0;

/* End of Hhc8bit_product_header() */
}


/**********************************************************************

   Description:
      Fill remaining Product Header fields for 256-level HHC product. 


   Inputs:
      ndpd - number of bytes in the product so far.
      numbins - number of bins in a product radial.
      obrptr - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int Hhc8bit_end_of_product_processing( int ndpb, int numbins, 
                                        char *obrptr ){

    int bytecnt;

    Graphic_product *phd = (Graphic_product *) obrptr;
    Symbology_block *sym = (Symbology_block *) 
                           (obrptr + sizeof(Graphic_product));
    Packet_16_hdr_t *packet_16 = (Packet_16_hdr_t *) 
              (obrptr + sizeof(Graphic_product) + sizeof(Symbology_block));

    /* Complete the packet 16 header. */
    RPGC_digital_radial_data_hdr( 0, numbins, 0, 0, 1000., Radcount, 
                                  (void *) packet_16 );

    /* Data level threshold codes. */
    RPGC_set_product_float((void*)&phd->level_1,Scale);
    RPGC_set_product_float((void*)&phd->level_3,Offset);
    phd->level_6 = (short)255; /* Number of levels including flags */
    phd->level_7 = (short)0; /* Leading flags:  0 = below threshold, 1 = RF */
    phd->level_8 = (short)0; /* No trailing flags */

    /* Calculate and store the product message length, the product block 
       length and the product layer length. */

    /* Length of product layer. */
    bytecnt = ndpb + sizeof(Packet_16_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* Complete the product header. */
    RPGC_prod_hdr( obrptr, Prod_id, &bytecnt);

    Totalbytes = bytecnt+100;

    /* Return to the product generation control routine. */
    return 0;

/* End of Hhc8bit_end_of_product_processing() */
} 
