/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/10/03 16:08:53 $
 * $Id: dualpol4bit.c,v 1.11 2011/10/03 16:08:53 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <dualpol4bit.h>
#include <dp_elev_func_prototypes.h>
#include <hca.h>

#define DEBUG_AZ -1  /*Set to positive azimuth value for debug output */
#define DEBUG_PRODID  160

#define HC_MODE_FILTER_SIZE  9

/* Map of internal hydro class numbers to external (i.e. product) numbers    */
/* Index this array with the internal hydro class number (e.g. RA, HA, etc.) */
/* to obtain the hydro class numbers used in the HC products.                */

/* These external hydro class codes are scaled by 10 to leave room for future classes */
                    /* U0 U1  RA  HR   RH  BD  BI  GC  DS  WS  IC  GR   UK ND */
static float Class_external[NUM_CLASSES] = 
                       {0.,0.,60.,70.,100.,80.,10.,20.,40.,50.,30.,90.,140.,0.};

static float Scale = 1;
static float Offset = 0;

/* Function prototypes */
void create_color_table( short *clr, int p);

/*********************************************************************

   Description:
      Buffer control module for a dual pol 4 bit base product.

   Returns:
      Currently, always returns 0.

*********************************************************************/
int Dualpol4bit_buffer_control(){

    int ref_flag, vel_flag, wid_flag;
    int p, opstat, prods_requested, prods_processed, ret;

    char *obrptr[N4BIT_PRODS] = {NULL};
    char *bdataptr = NULL;

    /* Initialization. */
    opstat = 0;

    /* Acquire output buffers for the 16 data level products,
       0 to 230 km. Range (1 km X 1 deg resolution). */
    prods_requested = prods_processed = 0;
    for (p = 0; p < N4BIT_PRODS; p++) {
       Proc_rad[p] = 1;
       obrptr[p] = RPGC_get_outbuf_by_name( OUTDATA_NAME[p], OBUF_SIZE, &opstat );

       if( opstat != NORMAL ){
          RPGC_log_msg(GL_INFO,"An output buffer is unavailable: #%d!, opstat=%d",p,opstat);
          obrptr[p] = NULL;
          Proc_rad[p] = 0;  
       }
       else {
          prods_requested++;
       }
    }/* end for all products */

    prods_processed = prods_requested;

    if (!prods_requested){
       RPGC_log_msg( GL_INFO, "No Output Buffers Available for dualpol4bit\n",p );
       RPGC_abort();
       return 0;
    }

    /* Request first input buffer (Radial base data) and process it. */
    bdataptr = RPGC_get_inbuf_by_name( INDATA_NAME, &opstat);

    /* Check for Base Reflectivity disabled. */
    if( opstat == NORMAL ){
       RPGC_what_moments( (Base_data_header *) bdataptr, &ref_flag, 
                           &vel_flag, &wid_flag );
       if( !ref_flag ){

          /* Moment disabled .. Release input buffer and do abort processing. */
		RPGC_rel_inbuf( bdataptr );
          RPGC_log_msg(GL_INFO,"Moment disabled!");
		RPGC_abort_because( PROD_DISABLED_MOMENT );
		return 0;

       } /* end if !ref_flag */
    } /* end if opstat == NORMAL */

    /* Keep processing radials until end of elevation */
    while (1) {

       if( opstat == NORMAL ){

          /* Initialize the end of elevation flag */
          Endelcut = 0;

          /* Loop for all 4-bit products */
          for (p = 0; p < N4BIT_PRODS; p++) {

             if ( obrptr[p] != NULL ) {

                /* Call the product generation control routine. */
                if ((ret = Dualpol4bit_product_generation_control( obrptr[p], bdataptr, p )) < 0){
                   /* Destroy output buffer since nothing to do for this data field */
                   RPGC_rel_outbuf( obrptr[p], DESTROY );
                   obrptr[p] = NULL;
                   prods_processed--;

                } /* end if ret < 0 */

                else if ( Endelcut ) {
 
                   /* Elevation cut completed, release output product buffer. */
                   RPGC_rel_outbuf( obrptr[p], FORWARD|RPGC_EXTEND_ARGS, Totalbytes[p]);

                } /* end if Endelcut && buffer not null */
             } /* end if obrptr[p] != NULL */
          } /* end for all output products */    

         /* Release the input radial. */
         RPGC_rel_inbuf( bdataptr );

         if (!Endelcut && prods_processed) {
            /* Retrieve next input radial. */
            bdataptr = RPGC_get_inbuf_by_name( INDATA_NAME, &opstat );
            continue;
          } /* if !Endelcut */
          else {
             break;
          }
       }
       else {
          
          RPGC_log_msg(GL_INFO,"Input data stream canceled!");
          for (p = 0; p < N4BIT_PRODS; p++) {
             if (obrptr[p] != NULL)
                RPGC_rel_outbuf( obrptr[p], DESTROY );
          } /*end for all output products */

          RPGC_abort_because( opstat );
          break;

       } /* end if opstat != NORMAL */
     
    } /*end while */

    /* Do we need to abort? */
    if( (prods_requested > 0) && (prods_processed == 0) )
       RPGC_abort();

    /* Return to wait for activation. */
    return 0;

/* End of Dualpol4bit_buffer_control() */
} 


/********************************************************************* 

   Description:
      Controls the processing of data for the 16-level Base 
      product on a radial basis. 

   Inputs:
      obrptr - pointer to product buffer.
      bdataptr - pointer to radial message.
      p - index to the output product being generated

   Returns:
      Returns 0 unless data field not found in which case returns -1.

******************************************************************* */
int Dualpol4bit_product_generation_control( char *obrptr, char *bdataptr, int p ){

    /* Local variables */
    int volnumber, vcpnumber, elevindex = 0; 
    int nrlebyts, delta, start, excess, i;

    int   itemp;
    short *clr = NULL;   /* Color table */

    double coselev, elang;
    
    Scan_Summary *summary = NULL;
    Base_data_header *radhead = (Base_data_header *) bdataptr;

    /* Static variables. */
           Generic_moment_t gm;   /* Input generic moment data */
    static int              pbuffind[N4BIT_PRODS] = {0}, ndpb[N4BIT_PRODS] = {0};
    static int              shortbins;
    static short           *outbuff[N4BIT_PRODS] = {NULL};
           unsigned short   shortbuf[OUTBINS];
           unsigned char    charbuf[BASEDATA_DOP_SIZE];

    
    /* Beginning of product initialization. */
    if( (radhead->status == GOODBVOL) 
                    || 
        (radhead->status == GOODBEL) ){

       /* Initialize the min and max data level. */
	  Mxpos[p] = 0;
       Mnpos[p] = 999;

       /* Initialize the radial counter. */
       Radcount[p] = 0;

       /* Initialize Buffer index counter and number of packet bytes. */
	  pbuffind[p] = 0;
       ndpb[p]     = 0;
       outbuff[p] = (short *) (obrptr + sizeof(Graphic_product) + 
                           sizeof(Symbology_block) + sizeof(packet_af1f_hdr_t));

       /* Get elevation angle from volume coverage pattern and elevation 
          index number. */
	  volnumber = RPGC_get_buffer_vol_num( bdataptr );
       summary   = RPGC_get_scan_summary( volnumber );
	  vcpnumber = summary->vcp_number;
       elevindex = RPGC_get_buffer_elev_index( bdataptr );

	  Elmeas = (int) RPGCS_get_target_elev_ang(  vcpnumber, elevindex );
	  elang = Elmeas * .1f;

       /* Cosine of elevation angle computation for combination with 
          scale factor for the scan projection correction. */
	  coselev = cos( elang * DEGTORAD );
	  Vc = coselev * 1e3f;

       /* Pack product header fields. */
	  Dualpol4bit_product_header( obrptr, volnumber, p );
       if (DEBUG_AZ > 0)
          RPGC_log_msg(GL_INFO,"Starting elevation: %4.1f degrees. obrptr=%x bdataptr=%x",
                            elang, obrptr, bdataptr);
    } /*end if beginning of elevation or volume */
 
    /* Perform individual radial processing if radial not flagged 'BAD'. */
    if( Proc_rad[p] ){
    
       /* Calculate remaining words available in output buffer. */
	  excess = OBUF_SIZE - (ndpb[p] + 150);
	
       if( excess > EST_PER_RAD ){
           unsigned char  *dp_data = NULL;
           unsigned short *dp16_data = NULL;

           if (strcmp(OUTDATA_NAME[p],"KDP4BIT") == 0 && radhead->n_surv_bins != 0) {
              dp16_data = (unsigned short *)RPGC_get_radar_data((void *)radhead, RPGC_DKDP, &gm);
              if (dp16_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("KDP4BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }

              /* Determine minimum and maximum data value in the radial. */
              for( i = 0;  i < gm.no_of_gates; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp16_data[i] > Mxpos[p] )
                    Mxpos[p] = dp16_data[i];

                 /* Is this the minimum for the elevation? */
                 if( dp16_data[i] < Mnpos[p] && dp16_data[i] > RF_FLAG)
                    Mnpos[p] = dp16_data[i];
              } 

              /* Reduce the range resolution of the data from 0.25 km to 1 km by
                 taking every fourth bin. (THIS MAY CHANGE TO AN AVERAGE OR MAX?)
                 Starting with 1 limits the "range jump" illusion when comparing 
                 with the 8-bit version. Also reduce data precision to 8 bits if needed. */
              shortbins = 0;
              if (gm.data_word_size == 8) {
                 dp_data = (unsigned char*)dp16_data;
                 for (i=1; i < gm.no_of_gates; i+=4) {
                    shortbuf[shortbins] = dp_data[i];
                    shortbins++;

                    /* Cap the KDP values at 10.0 deg/km per ICD */
                    if ((int)shortbuf[shortbins] > RF_FLAG) {
                       float temp;
                       temp = ((float)shortbuf[i] - Offset) / Scale;
                       if (temp > MAX_KDP_DISPLAY) temp = MAX_KDP_DISPLAY;
                       shortbuf[shortbins] = (unsigned char)roundf((temp * Scale) + Offset);
                    }
                    if (shortbins == OUTBINS) break;
                 } /* end for all bins */
              }
              else {
                 for (i=1; i < gm.no_of_gates; i+=4) {
                    if (dp16_data[i] > RF_FLAG)
                       shortbuf[shortbins] = dp16_data[i]/256;
                    else
                       shortbuf[shortbins] = dp16_data[i];
                    shortbins++;
                    if (shortbins == OUTBINS) break;
                 } /* end for all bins */
              }

              if (radhead->azi_num == DEBUG_AZ)
                 RPGC_log_msg(GL_INFO,"name=%s shortbins = %d, no_gates=%d",
                                  gm.name,shortbins, gm.no_of_gates);
           }
           else if (strcmp(OUTDATA_NAME[p],"CC4BIT") == 0 && radhead->n_surv_bins != 0) {
              dp16_data = (unsigned short *)RPGC_get_radar_data((void *)radhead, RPGC_DRHO, &gm);
              if (dp16_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("CC4BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }

              /* Determine minimum and maximum data value in the radial. */
              for( i = 0;  i < gm.no_of_gates; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp16_data[i] > Mxpos[p] )
                    Mxpos[p] = dp16_data[i];

                 /* Is this the minimum for the elevation? */
                 if( dp16_data[i] < Mnpos[p] && dp16_data[i] > RF_FLAG)
                    Mnpos[p] = dp16_data[i];
              }
          
              /* Reduce the range resolution of the data from 0.25 km to 1 km by
                 taking every fourth bin. (THIS MAY CHANGE TO AN AVERAGE OR MAX?)
                 Starting with 1 limits the "range jump" illusion when comparing 
                 with the 8-bit version. Also reduce data precision to 8 bits if needed. */
              shortbins = 0;
              if (gm.data_word_size == 8) {
                 dp_data = (unsigned char*)dp16_data;
                 for (i=1; i < gm.no_of_gates; i+=4) {
                    shortbuf[shortbins] = dp_data[i];
                    shortbins++;
                    if (shortbins == OUTBINS) break;
                 } /* end for all bins */
              }
              else {
                 for (i=1; i < gm.no_of_gates; i+=4) {
                    if (dp16_data[i] > RF_FLAG)
                       shortbuf[shortbins] = dp16_data[i]/256;
                    else
                       shortbuf[shortbins] = dp16_data[i];
                    shortbins++;
                    if (shortbins == OUTBINS) break;
                 } /* end for all bins */
              }

              if (radhead->azi_num == DEBUG_AZ)
                 RPGC_log_msg(GL_INFO,"name=%s shortbins = %d, no_gates=%d",
                                  gm.name,shortbins, gm.no_of_gates);
           }
           else if (strcmp(OUTDATA_NAME[p],"ZDR4BIT") == 0 && radhead->n_surv_bins != 0) {
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DZDR, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("ZDR4BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }

              /* Determine minimum and maximum data value in the radial. */
              for( i = 0;  i < gm.no_of_gates; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp_data[i] > Mxpos[p] )
                    Mxpos[p] = dp_data[i];

                 /* Is this the minimum for the elevation? */
                 if( dp_data[i] < Mnpos[p] && dp_data[i] > RF_FLAG)
                    Mnpos[p] = dp_data[i];
              }  
   
              /* Reduce the range resolution of the data from 0.25 km to 1 km by
                 taking every fourth bin. (THIS MAY CHANGE TO AN AVERAGE OR MAX?)
                 Starting with 1 limits the "range jump" illusion when comparing 
                 with the 8-bit version.                                         */
              shortbins = 0;
              for (i=1; i < gm.no_of_gates; i+=4) {
                 shortbuf[shortbins] = dp_data[i];
                 if(shortbuf[shortbins] > 255) {
                    shortbuf[shortbins] = 255;
                    RPGC_log_msg(GL_INFO,"ZDR Overflow dp_data[%d]= %d",i,dp_data[i]);
                 }
                 shortbins++;
                 if (shortbins == OUTBINS) break;
              } /* end for all bins */

              if (radhead->azi_num == DEBUG_AZ)
                 RPGC_log_msg(GL_INFO,"name=%s shortbins = %d, no_gates=%d",
                                  gm.name,shortbins, gm.no_of_gates);
           }
           else if (strcmp(OUTDATA_NAME[p],"HC4BIT") == 0 && radhead->n_surv_bins != 0) {
              strcpy(gm.name,"DHCA");
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DANY, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("HC4BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }

              /* The mode filter works inplace on the input buffer so copy dp_data
                 to a work buffer (charbuf)                                        */
              memcpy(charbuf, dp_data, gm.no_of_gates);

              /* Apply a mode filter to the HC data now in charbuf.
                 The current filter size is 9 bins.                                */
              mode_filter((char*)charbuf, (int)gm.no_of_gates, (int)HC_MODE_FILTER_SIZE);

              /* Reduce the range resolution of the data from 0.25 km to 1 km by
                 taking every fourth bin. Starting with 1 limits the "range jump" 
                 illusion seen when comparing the 4-bit product with the 8-bit version. */
              shortbins = 0;
              for (i=1; i < gm.no_of_gates; i+=4) {
                 shortbuf[shortbins] = charbuf[i];
                 if(shortbuf[shortbins] > 255) {
                    shortbuf[shortbins] = 255;
                    RPGC_log_msg(GL_INFO,"HCA Overflow dp_data[%d]= %d",i,dp_data[i]);
                 }
                 shortbins++;
                 if (shortbins == OUTBINS) break;
              } /* end for all bins */

              /* We need to convert the internal HCA codes to external codes 
                 according to the ICD.                                         */
              for (i=0; i < shortbins; i++) {
                 itemp = shortbuf[i];
                 shortbuf[i] = (short) Class_external[itemp];
               }/* end for all bins */

              if (radhead->azi_num == DEBUG_AZ)
                 RPGC_log_msg(GL_INFO,"name=%s shortbins = %d, no_gates=%d",
                                  gm.name,shortbins, gm.no_of_gates);
           }

           /* Retrieve the scale factor and offset for the data */
           Scale  = gm.scale;
           Offset = gm.offset;

           /* Retrieve start angle and delta angle measurements from the 
              input radial header. */
           start = radhead->start_angle;
           delta = radhead->delta_angle;
          
           int start_index = (gm.first_gate_range/gm.bin_size);
           int end_index = shortbins - 1;

           /* Cannot exceed number of data bins for the product, we have
              selected 230 bins as the max range for this product.       */
           if( ((end_index - start_index) + 1) > OUTBINS )
              end_index = (OUTBINS - start_index) - 1;

           if (radhead->azi_num == DEBUG_AZ) {
             RPGC_log_msg(GL_INFO,"start = %d, end = %d", start_index,end_index);
             RPGC_log_msg(GL_INFO," rpg_elev_ind = %d   elev_num = %d",
                                    radhead->rpg_elev_ind, radhead->elev_num);
           }

           /* Increment the radial count. */
           Radcount[p]++;

           clr=(short*)malloc(256*sizeof(short));   
    
           /* Load the color table   */
           create_color_table(clr, p);

           if (radhead->azi_num == DEBUG_AZ)
             RPGC_log_msg(GL_INFO,"p=%d start=%d delta=%d shortbins=%d,start_index=%d end_index=%d first_gate=%d size=%d shortbuf[end_index]=%d",
    p, start, delta, shortbins, start_index, end_index,gm.first_gate_range, gm.bin_size,shortbuf[end_index]);
           RPGC_run_length_encode(start,        /* Radial start angle (deg*10)*/
                                  delta,        /* Radial width (deg*10)*/
                                  (short*) shortbuf, /* Input data to be run-length encoded*/
                                  start_index,  /* Index into shortbuf for first good bin*/
                                  end_index,    /* Index into shortbuf for last good bin */ 
                                  (int)shortbins, /* Max number of data bins to encode*/
                                  (int)1,       /* Range resolution reduction factor*/
                                  clr,          /* Pointer to color table*/
                                  &nrlebyts,    /* Number of rle bytes for this radial*/
                                  pbuffind[p],  /* Index into outbuff for beginning*/
                                                /* of rle data for this radial.*/
                                  outbuff[p] ); /* The final product*/

/***           if (radhead->azi_num == DEBUG_AZ && Prod_id[p] == DEBUG_PRODID) {
              int j;
              for(i=0;i<60;i++){
                 j = i*4;
                 RPGC_log_msg(GL_INFO,"shortbuf[%d] = %d, dp_data[%d]=%d",
                                     i,shortbuf[i], j,(short)(dp_data[j]));
              }
              RPGC_log_msg(GL_INFO,"Back from RLE, nrlebyts=%d",nrlebyts);
           }***/
           /* Free the allocated color table array */
           if (clr != NULL) free(clr);

           /* Update buffer counters and pointers. */
	      ndpb[p] += nrlebyts;
	      pbuffind[p] += nrlebyts / 2;

	   } /* end if excess > EST_PER_RAD */

    } /* end if Proc_rad */

    /* Test for last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){

       Endelcut = 1;

       /* If last radial encountered, fill remaining fields in product buffer. */
       elevindex = RPGC_get_buffer_elev_index( bdataptr );
       Dualpol4bit_end_of_product_processing( ndpb[p], elevindex, p,
                                              shortbins, obrptr );
    }

    /* Return to buffer control routine. */
    return 0;

/* End of Dualpol4bit_product_generation_control() */
} 


/********************************************************************
   
   Description:
      Fills in product description block, symbology block information.

   Inputs:
      outbuff - pointer to output buffer.
      vol_num - volume scan number.
      p -index to output product being generated.

   Returns:
      Always returns 0.

********************************************************************/
int Dualpol4bit_product_header( char *outbuff, int vol_num, int p ){

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                                 (outbuff + sizeof(Graphic_product));
    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, Prod_id[p], vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    params[2] = Elmeas;

/**    if (strcmp(OUTDATA_NAME[p],"HC4BIT") == 0) {**/
       /*  The HC product requires the mode filter size be in the header
           instead of the min and max values.                             */
/**       params[3] = (short)9;
    }**/
    
    RPGC_set_dep_params( outbuff, params );
    
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

/* End of Dualpol4bit_product_header() */
}


/**********************************************************************

   Description:
      Fill remaining Product Header fields for 16-level Dual 
      Pol product. 


   Inputs:
      ndpd - number of bytes in the product so far.
      shortbins - number of bins in a product radial.
      p - index to the output product being generated.
      outbuff - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int Dualpol4bit_end_of_product_processing( int ndpb, int elev_ind, int p,
                                           int shortbins, char *outbuff ){

    int bytecnt;
    float maxval = 0.0;
    float minval = 0.0;

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                           (outbuff + sizeof(Graphic_product));
    packet_af1f_hdr_t *packet_af1f = (packet_af1f_hdr_t *) (outbuff + sizeof(Graphic_product) +
                                                                      sizeof(Symbology_block));

    /* Construct the packet AF1F layer header. */
    packet_af1f->code              = (short)0xAF1F; /* packet code AF1F                    */
    packet_af1f->index_first_range = 0;             /* index of the first range bin        */
    packet_af1f->num_range_bins    = shortbins;     /* number of bins in each radial       */
    packet_af1f->i_center          = 256;           /* i center of display (ramtek values) */
    packet_af1f->j_center          = 280;           /* j center of display (ramtek values) */
    packet_af1f->scale_factor      = Vc;            /* scale factor                        */
    packet_af1f->num_radials       = Radcount[p];   /* number of radials included          */

    /* Load the packet layer header into the buffer. buffer offset=68     */
    /* header size=14 bytes                                               */
/*    memcpy(outbuff+68,&packet_af1f,sizeof(packet_af1f_hdr_t);*/

    /* Enter the data threshold values                                    */
    if (strcmp(OUTDATA_NAME[p],"KDP4BIT") == 0) {
      phd->level_1=(short)0x8002; /*   ND  */
      phd->level_2=(short)0x1114; /* -2.0  */
      phd->level_3=(short)0x110A; /* -1.0  */
      phd->level_4=(short)0x1105; /* -0.5  */
      phd->level_5=(short)0x1000; /*  0.0  */
      phd->level_6=(short)0x4019; /*  0.25 */
      phd->level_7=(short)0x1005; /*  0.5  */
      phd->level_8=(short)0x100A; /*  1.0  */
      phd->level_9=(short)0x100F; /*  1.5  */
      phd->level_10=(short)0x1014;/*  2.0  */
      phd->level_11=(short)0x1019;/*  2.5  */
      phd->level_12=(short)0x101E;/*  3.0  */ 
      phd->level_13=(short)0x1028;/*  4.0  */
      phd->level_14=(short)0x1032;/*  5.0  */
      phd->level_15=(short)0x1046;/*  7.0  */
      phd->level_16=(short)0x8003;/*   RF  */
    }
    else if (strcmp(OUTDATA_NAME[p],"CC4BIT") == 0) {
      phd->level_1=(short)0x8002; /*   ND   */
      phd->level_2=(short)0x4014; /*  0.20  */
      phd->level_3=(short)0x402D; /*  0.45  */
      phd->level_4=(short)0x4041; /*  0.65  */
      phd->level_5=(short)0x404B; /*  0.75  */
      phd->level_6=(short)0x4050; /*  0.80  */
      phd->level_7=(short)0x4055; /*  0.85  */
      phd->level_8=(short)0x405A; /*  0.90  */
      phd->level_9=(short)0x405D; /*  0.93  */
      phd->level_10=(short)0x405F;/*  0.95  */
      phd->level_11=(short)0x4060;/*  0.96  */
      phd->level_12=(short)0x4061;/*  0.97  */
      phd->level_13=(short)0x4062;/*  0.98  */
      phd->level_14=(short)0x4063;/*  0.99  */
      phd->level_15=(short)0x4064;/*  1.00  */
      phd->level_16=(short)0x8003;/*   RF  */
    }
    else if (strcmp(OUTDATA_NAME[p],"ZDR4BIT") == 0) {
      phd->level_1=(short)0x8002; /*   ND  */
      phd->level_2=(short)0x1128; /* -4.0  */
      phd->level_3=(short)0x1114; /* -2.0  */
      phd->level_4=(short)0x1105; /* -0.5  */
      phd->level_5=(short)0x1000; /*  0.0  */
      phd->level_6=(short)0x4019; /*  0.25 */
      phd->level_7=(short)0x1005; /*  0.5  */
      phd->level_8=(short)0x100A; /*  1.0  */
      phd->level_9=(short)0x100F; /*  1.5  */
      phd->level_10=(short)0x1014;/*  2.0  */
      phd->level_11=(short)0x1019;/*  2.5  */
      phd->level_12=(short)0x101E;/*  3.0  */ 
      phd->level_13=(short)0x1028;/*  4.0  */
      phd->level_14=(short)0x1032;/*  5.0  */
      phd->level_15=(short)0x103C;/*  6.0  */
      phd->level_16=(short)0x8003;/*   RF  */
    }
    else if (strcmp(OUTDATA_NAME[p],"HC4BIT") == 0) {
      phd->level_1=(short)0x8002; /*   ND  */
      phd->level_2=(short)0x8004; /*   BI  */
      phd->level_3=(short)0x8005; /*   GC  */
      phd->level_4=(short)0x8006; /*   IC  */
      phd->level_5=(short)0x8009; /*   DS  */
      phd->level_6=(short)0x8008; /*   WS  */
      phd->level_7=(short)0x800A; /*   RA  */
      phd->level_8=(short)0x800B; /*   HR  */
      phd->level_9=(short)0x800C; /*   BD  */
      phd->level_10=(short)0x8007;/*   GR  */
      phd->level_11=(short)0x800D;/*   HA  */
      phd->level_12=(short)0x8000;/* Blank */ 
      phd->level_13=(short)0x8000;/* Blank */
      phd->level_14=(short)0x8000;/* Blank */
      phd->level_15=(short)0x800E;/*   UK  */
      phd->level_16=(short)0x8003;/*   RF  */

    }

    /* Number of blocks in product = 3   */
    phd->n_blocks=(short)3;

    /* Message length                    */
    RPGC_set_product_int( (void *) &phd->msg_len, 
                          (unsigned int) ndpb );

    /* ICD block offsets (in bytes)      */

    RPGC_set_product_int( (void *) &phd->sym_off, 60 );
    RPGC_set_product_int( (void *) &phd->gra_off, 0 );
    RPGC_set_product_int( (void *) &phd->tab_off, 0 );

    /* Elevation index                   */
    phd->elev_ind=(short)elev_ind;

    /* Assign the minimum and maximum data levels to the product header. */
    /* (Does not apply to the HC product.)                               */
    minval = ((float)Mnpos[p] - Offset) / Scale;
    maxval = ((float)Mxpos[p] - Offset) / Scale;

    if (strcmp(OUTDATA_NAME[p],"KDP4BIT") == 0) {
       /* Cap the max KDP value at 10 deg/km per ICD */
       if (maxval > MAX_KDP_DISPLAY) maxval = MAX_KDP_DISPLAY;
       minval = minval / KDP_MINMAX_PRECISION;
       maxval = maxval / KDP_MINMAX_PRECISION;
    }
    else if (strcmp(OUTDATA_NAME[p],"CC4BIT") == 0) {
       minval = minval / CC_MINMAX_PRECISION;
       maxval = maxval / CC_MINMAX_PRECISION;
    }
    else if (strcmp(OUTDATA_NAME[p],"ZDR4BIT") == 0) {
       minval = minval / ZDR_MINMAX_PRECISION;
       maxval = maxval / ZDR_MINMAX_PRECISION;
    }

    if (strcmp(OUTDATA_NAME[p],"HC4BIT") == 0) {
       /*  The HC product requires the mode filter size be in the header
           instead of the min and max values.                             */
       phd->param_4 = (short)HC_MODE_FILTER_SIZE;
    }
    else {
       phd->param_4 = (short) RPGC_NINT( minval );
       phd->param_5 = (short) RPGC_NINT( maxval );
    }

    /* Calculate and store the product message length, the product block 
       length and the product layer length. */

    /* Length of product layer. */
    bytecnt = ndpb + sizeof(packet_af1f_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* Complete the product header. */
    RPGC_prod_hdr( outbuff, Prod_id[p], &bytecnt);

    Totalbytes[p] = bytecnt+100;

    /* Return to the product generation control routine. */
    return 0;

/* End of Dualpol4bit_end_of_product_processing() */
} 

void create_color_table(short *clr, int p) {
  /* fill the short array 'clr' (which stands for color) with
  16 color levels */
  int i;
 
  if (strcmp(OUTDATA_NAME[p],"KDP4BIT") == 0) {
    /* These for-loop values assume a 16-bit word size for KDP */
      clr[0]=0;            /* SNR<TH or <-2.0   */
      clr[1]=15;           /*  Range Folded     */
    for(i=2;i<23;i++)
      clr[i]=1;            /* -2.0<=deg/km<-1.0 */
    for(i=23;i<33;i++)
      clr[i]=2;            /* -1.0<=deg/km<-0.5 */
    for(i=33;i<43;i++)
      clr[i]=3;            /* -0.5<=deg/km< 0.0 */
    for(i=43;i<48;i++)
      clr[i]=4;            /*  0.0<=deg/km< 0.25*/
    for(i=48;i<53;i++)
      clr[i]=5;            /* 0.25<=deg/km< 0.5 */
    for(i=53;i<63;i++)
      clr[i]=6;            /*  0.5<=deg/km< 1.0 */
    for(i=63;i<73;i++)
      clr[i]=7;            /*  1.0<=deg/km< 1.5 */
    for(i=73;i<83;i++)
      clr[i]=8;            /*  1.5<=deg/km< 2.0 */
    for(i=83;i<93;i++)
      clr[i]=9;            /*  1.0<=deg/km< 2.5 */
    for(i=93;i<103;i++)
      clr[i]=10;           /*  1.5<=deg/km< 3.0 */
    for(i=103;i<123;i++)
      clr[i]=11;           /*  3.0<=deg/km< 4.0 */
    for(i=123;i<143;i++)
      clr[i]=12;           /*  4.0<=deg/km< 5.0 */
    for(i=143;i<183;i++)
      clr[i]=13;           /*  5.0<=deg/km< 7.0 */
    for(i=183;i<256;i++)
      clr[i]=14;           /*  7.0<=deg/km<10.0 */
  }
  else if (strcmp(OUTDATA_NAME[p],"CC4BIT") == 0) {
    /* These for-loop values assume a 16-bit word size for CC */
      clr[0]=0;            /* SNR<TH or <-0.20*/
      clr[1]=15;           /*  Range Folded   */
    for(i=2;i<75;i++)
      clr[i]=1;            /* 0.20<=CC< 0.45  */
    for(i=75;i<135;i++)
      clr[i]=2;            /* 0.45<=CC< 0.65  */
    for(i=135;i<165;i++)
      clr[i]=3;            /* 0.65<=CC< 0.75  */
    for(i=165;i<180;i++)
      clr[i]=4;            /* 0.75<=CC< 0.80  */
    for(i=180;i<195;i++)
      clr[i]=5;            /* 0.80<=CC< 0.85  */
    for(i=195;i<210;i++)
      clr[i]=6;            /* 0.85<=CC< 0.90  */
    for(i=210;i<219;i++)
      clr[i]=7;            /* 0.90<=CC< 0.93  */
    for(i=219;i<225;i++)
      clr[i]=8;            /* 0.93<=CC< 0.95  */
    for(i=225;i<228;i++)
      clr[i]=9;            /* 0.95<=CC< 0.96  */
    for(i=228;i<231;i++)
      clr[i]=10;           /* 0.96<=CC< 0.97  */
    for(i=231;i<234;i++)
      clr[i]=11;           /* 0.97<=CC< 0.98  */
    for(i=234;i<237;i++)
      clr[i]=12;           /* 0.98<=CC< 0.99  */
    for(i=237;i<240;i++)
      clr[i]=13;           /* 0.99<=CC< 1.00  */
    for(i=240;i<256;i++)
      clr[i]=14;           /* 1.00<=CC< 1.05  */
 }
  else if (strcmp(OUTDATA_NAME[p],"ZDR4BIT") == 0) {
      clr[0]=0;            /* SNR<TH or <-4.0*/
      clr[1]=15;           /*  Range Folded  */
    for(i=2;i<96;i++)
      clr[i]=1;            /* -4.0<=dB<-2.0  */
    for(i=96;i<120;i++)
      clr[i]=2;            /* -2.0<=dB<-0.5  */
    for(i=120;i<128;i++)
      clr[i]=3;            /* -0.5<=dB< 0.0  */ 
    for(i=128;i<132;i++)
      clr[i]=4;            /*  0.0<=dB< 0.25 */
    for(i=132;i<136;i++)
      clr[i]=5;            /* 0.25<=dB< 0.5  */
    for(i=136;i<144;i++)
      clr[i]=6;            /*  0.5<=dB< 1.0  */
    for(i=144;i<152;i++)
      clr[i]=7;            /*  1.0<=dB< 1.5  */
    for(i=152;i<160;i++)
      clr[i]=8;            /*  1.5<=dB< 2.0  */
    for(i=160;i<168;i++)
      clr[i]=9;            /*  2.0<=dB< 2.5  */
    for(i=168;i<176;i++)
      clr[i]=10;           /*  2.5<=dB< 3.0  */
    for(i=176;i<192;i++)
      clr[i]=11;           /*  3.0<=dB< 4.0  */
    for(i=192;i<208;i++)
      clr[i]=12;           /*  4.0<=dB< 5.0  */
    for(i=208;i<224;i++)
      clr[i]=13;           /*  5.0<=dB< 6.0  */
    for(i=224;i<256;i++)
      clr[i]=14;           /*  6.0<=dB< 8.0  */
  }
  else if (strcmp(OUTDATA_NAME[p],"HC4BIT") == 0) {
      clr[0]=0;            /* SNR < Thresh   */
      clr[1]=15;           /* Range Folded   */
    for(i=10;i<19;i++)
      clr[i]=1;            /* Biological     */
    for(i=20;i<29;i++)
      clr[i]=2;            /* Ground Clutter */
    for(i=30;i<39;i++)
      clr[i]=3;            /* Ice Crystals   */
    for(i=40;i<49;i++)
      clr[i]=4;            /* Dry Snow       */
    for(i=50;i<59;i++)
      clr[i]=5;            /* Wet Snow       */
    for(i=60;i<69;i++)
      clr[i]=6;            /* Rain           */
    for(i=70;i<79;i++)
      clr[i]=7;            /* Heavy Rain     */
    for(i=80;i<89;i++)
      clr[i]=8;            /* Big Drops      */
    for(i=90;i<99;i++)
      clr[i]=9;            /* Graupel        */
    for(i=100;i<109;i++)
      clr[i]=10;           /* Hail and Rain  */
    for(i=110;i<119;i++)
      clr[i]=11;           /* TBD            */
    for(i=120;i<129;i++)
      clr[i]=12;           /* TBD            */
    for(i=130;i<139;i++)
      clr[i]=13;           /* TBD            */
    for(i=140;i<256;i++)
      clr[i]=14;           /* Unknown        */
  }
  return;
}
