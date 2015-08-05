/* RCS info */
/* $Author:  */
/* $Locker:   */
/* $Date:  */
/* $Id:  */
/* $Revision:  */
/* $State: */

#include "dualpol8bit.h"
#include <hca.h>

#define DEBUG_AZ -1 /*Set to positive azimuth value for debug output */
#define MAX_KDP_DISPLAY  10.0  /* Maximum Kdp value for product display */   
#define MAX_PHI_DISPLAY 720.0  /* Maximum Phi value for (debug) product display */

/* Map of internal hydro class numbers to external (i.e. product) numbers    */
/* Index this array with the internal hydro class number (e.g. RA, HA, etc.) */
/* to obtain the hydro class numbers used in the HC products.                */

/* These external hydro class codes are scaled by 10 to leave room for future classes */
                    /* U0 U1  RA  HR   RH  BD  BI  GC  DS  WS  IC  GR   UK ND */
static float Class_external[NUM_CLASSES] = 
                       {0.,0.,60.,70.,100.,80.,10.,20.,40.,50.,30.,90.,140.,0.};

static float Scale = 1;
static float Offset = 0;
static unsigned short Mxpos[N8BIT_PRODS];  /* Maximum data level. */
static unsigned short Mnpos[N8BIT_PRODS];  /* Minimum data level. */
static unsigned short Mxbin[N8BIT_PRODS];  /* bin number of max data */
static unsigned short Mnbin[N8BIT_PRODS];  /* bin number of min data */
static unsigned short Mxazm[N8BIT_PRODS];  /* start azimuth of max data */
static unsigned short Mnazm[N8BIT_PRODS];  /* start azimuth of min data */

/*********************************************************************

   Description:
      Buffer control module for a dual pol 8 bit base product.

   Returns:
      Currently, always returns 0.

*********************************************************************/
int Dualpol8bit_buffer_control(){

    int ref_flag, vel_flag, wid_flag;
    int p, opstat, prods_requested, prods_processed, ret;

    char *obrptr[N8BIT_PRODS] = {NULL};
    char *bdataptr = NULL;

    /* Initialization. */
    opstat = 0;

    /* Acquire output buffers for the 256 data level products,
       0 to 300 km. Range (250 meter X 1 deg resolution). */
    prods_requested = prods_processed = 0;
    for (p = 0; p < N8BIT_PRODS; p++) {
       Proc_rad[p] = 1;
       obrptr[p] = RPGC_get_outbuf_by_name( OUTDATA_NAME[p], OBUF_SIZE, &opstat );

       if( opstat != NORMAL ){
             RPGC_log_msg(GL_INFO,"An output buffer (%s) is unavailable: #%d, opstat=%d !",
                                   OUTDATA_NAME[p],p,opstat);
             obrptr[p] = NULL;
             Proc_rad[p] = 0;
       }
       else {
          prods_requested++;
       }
    }/* end for all products */

    prods_processed = prods_requested;

    if (!prods_requested){
       RPGC_log_msg( GL_INFO, "No Output Buffers Available for dualpol8bit\n",p );
       RPGC_abort_because(PGM_MEM_LOADSHED);
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
          
          for (p = 0; p < N8BIT_PRODS; p++) {
             if (obrptr[p] != NULL)
                RPGC_rel_outbuf( obrptr[p], DESTROY );
          } /*end for all output products */
	  
          RPGC_abort_because( PGM_DISABLED_MOMENT );
	  return 0;

       } /* end if !ref_flag */
    } /* end if opstat == NORMAL */

    /* Loop for all input (base data) radials until end of elevation encountered. */
    while(1){

       if( opstat == NORMAL ){

          /* Initialize the end of elevation flag */
          Endelcut = 0;

          /* Loop for all 8-bit products */
          for (p = 0; p < N8BIT_PRODS; p++) {

             if ( obrptr[p] != NULL ) {
   
                /* Call the product generation control routine. */
                if( (ret = Dualpol8bit_product_generation_control( obrptr[p], bdataptr, p )) < 0){
                   /*Destroy output buffer since nothing to do for this data field */
                   RPGC_rel_outbuf( obrptr[p], DESTROY );
                   obrptr[p] = NULL;
                   prods_processed--;

                } /* end of it ret < 0 */
  
                else if ( Endelcut ){
 
                   /* Elevation cut completed, release output product buffer. */
                   RPGC_rel_outbuf( obrptr[p], FORWARD|RPGC_EXTEND_ARGS, Totalbytes[p]);

                } /* end if Endelcut */
             } /* end if obrptr[p] != NULL */
          } /* end for all output products */    

          /* Release the input radial. */
          RPGC_rel_inbuf( bdataptr );

          if (!Endelcut && prods_processed ) {

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
          for (p = 0; p < N8BIT_PRODS; p++) {
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

/* End of Dualpol8bit_buffer_control() */
} 


/********************************************************************* 

   Description:
      Controls the processing of data for the 256-level Base 
      product on a radial basis. 

   Inputs:
      obrptr - pointer to product buffer.
      bdataptr - pointer to radial message.
      p - index to the output product being generated.

   Returns:
      Returns 0 unless data field is not found in which case returns -1.

******************************************************************* */
int Dualpol8bit_product_generation_control( char *obrptr, char *bdataptr, int p ){

    /* Local variables */
    int   volnumber, vcpnumber, elevindex=0; 
    int   ndpbyts, delta, start, excess, i;
    int   itemp, frst_bin, last_bin;

    double coselev, elang;
    
    Scan_Summary *summary = NULL;
    Base_data_header *radhead = (Base_data_header *) bdataptr;

    /* Static variables. */
           Generic_moment_t gm;   /* Input generic moment data */
    static int              pbuffind[N8BIT_PRODS] = {0}, ndpb[N8BIT_PRODS] = {0};
    static int              numbins;
    static short           *outbuff[N8BIT_PRODS] = {NULL};
    unsigned short          shortbuf[MAXBINS];
    unsigned char           charbuf[MAXBINS];
    static int              hc_charbuf_size = 0;
    static char            *hc_charbuf = NULL;
    float                   temp;

    /* Beginning of product initialization. */
    if( (radhead->status == GOODBVOL) 
                    || 
        (radhead->status == GOODBEL) ){

       /* Initialize the min and max data level. */
       Mxpos[p] = 0;
       Mnpos[p] = 65000;

       /* Initialize the radial counter. */
       Radcount[p] = 0;

       /* Initialize Buffer index counter and number of packet bytes. */
	  pbuffind[p] = 0;
       ndpb[p]     = 0;
       outbuff[p]  = (short *) (obrptr + sizeof(Graphic_product) + 
                               sizeof(Symbology_block) + sizeof(Packet_16_hdr_t));

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
	  Dualpol8bit_product_header( obrptr, volnumber, p );
       if(DEBUG_AZ > 0)
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

           if (strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0 &&
                            radhead->n_surv_bins != 0) {
              dp16_data = (unsigned short *)RPGC_get_radar_data((void *)radhead, RPGC_DKDP, &gm);
              if (dp16_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("KDP8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }     
           }
           else if (strcmp(OUTDATA_NAME[p],"CC8BIT") == 0 &&
                                radhead->n_surv_bins != 0) {
              dp16_data = (unsigned short *)RPGC_get_radar_data((void *)radhead, RPGC_DRHO, &gm);
              if (dp16_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("CC8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }     
           }
           else if (strcmp(OUTDATA_NAME[p],"ZDR8BIT") == 0 &&
                                 radhead->n_surv_bins != 0) {
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DZDR, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("ZDR8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }     
           }
           else if (strcmp(OUTDATA_NAME[p],"HC8BIT")  == 0 &&
                                 radhead->n_surv_bins != 0) {
              strcpy(gm.name,"DHCA");
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DANY, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("HC8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }     

              /* We need to convert the internal HCA codes to external codes 
                 according to the ICD.                                         */
              numbins = gm.no_of_gates;
              if (numbins > hc_charbuf_size) {
	            if (hc_charbuf != NULL)
	               free (hc_charbuf);
	            hc_charbuf = MISC_malloc(numbins);
	            hc_charbuf_size = numbins;
              }

              for (i=0; i < numbins; i++) {
                 itemp = dp_data[i];
                 hc_charbuf[i] = (unsigned char) Class_external[itemp];
              }/* end for all bins */
           }
    /* Products below this point are non-operational and included only for testing purposes */
           else if (strcmp(OUTDATA_NAME[p],"SMZ8BIT") == 0 &&
                                 radhead->n_surv_bins != 0) {
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DSMZ, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("SMZ8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }
              /* Make sure we aren't processing too many bins */
              if (gm.no_of_gates > MAXBINS)  gm.no_of_gates = MAXBINS;
           }
           else if (strcmp(OUTDATA_NAME[p],"SNR8BIT") == 0 &&
                                 radhead->n_surv_bins != 0) {
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DSNR, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("SNR8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }
              /* Make sure we aren't processing too many bins */
              if (gm.no_of_gates > MAXBINS)  gm.no_of_gates = MAXBINS; 
           }
           else if (strcmp(OUTDATA_NAME[p],"PHI8BIT") == 0 &&
                                 radhead->n_surv_bins != 0) {
              dp16_data = (unsigned short *)RPGC_get_radar_data((void *)radhead, RPGC_DPHI, &gm);
              if (dp16_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("PHI8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }
              /* Make sure we aren't processing too many bins */
              if (gm.no_of_gates > MAXBINS)  gm.no_of_gates = MAXBINS;
           }
           else if (strcmp(OUTDATA_NAME[p],"SDZ8BIT") == 0 &&
                                 radhead->n_surv_bins != 0) {
              dp_data = (unsigned char *)RPGC_get_radar_data((void *)radhead, RPGC_DSDZ, &gm);
              if (dp_data == NULL) {
                 RPGC_log_msg(GL_INFO,"Failed to obtain generic moment input data for %s from %s",
                                        OUTDATA_NAME[p],INDATA_NAME);
                 RPGC_abort_dataname_because("SDZ8BIT", PGM_DISABLED_MOMENT);
                 return -1;
              }
              /* Make sure we aren't processing too many bins */
              if (gm.no_of_gates > MAXBINS)  gm.no_of_gates = MAXBINS;
           }
           /* Retrieve the scale factor and offset for the data */
           Scale  = gm.scale;
           Offset = gm.offset;

           /* Retrieve start angle and delta angle measurements from the 
              input radial header. */
           start = radhead->start_angle;
           delta = radhead->delta_angle;

           /* Calculate the start and end bin indicies of the good data */
           frst_bin = (gm.first_gate_range/gm.bin_size);
           last_bin = frst_bin + gm.no_of_gates - 1;
           
           /* Increment the radial count. */
           Radcount[p]++;

           /* Get the number of bins from the header */
           numbins = gm.no_of_gates;

           if (strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0 &&
                            radhead->n_surv_bins != 0) {
              /* For 16 bit KDP data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp16_data[i] > Mxpos[p] ){ 
                    Mxpos[p] = dp16_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                  /* Is this the minimum for the elevation? */
                 if( dp16_data[i] < Mnpos[p] && dp16_data[i] > RF_FLAG){
                    Mnpos[p] = dp16_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if KDP */
           if (strcmp(OUTDATA_NAME[p],"CC8BIT")  == 0 &&
                            radhead->n_surv_bins != 0) {
              /* For 16 bit CC data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp16_data[i] > Mxpos[p] ){
                    Mxpos[p] = dp16_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                  /* Is this the minimum for the elevation? */
                 if( dp16_data[i] < Mnpos[p] && dp16_data[i] > RF_FLAG){
                    Mnpos[p] = dp16_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if CC */
           if (strcmp(OUTDATA_NAME[p],"PHI8BIT") == 0  &&
                            radhead->n_surv_bins != 0) {
              /* For 16 bit PHI data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp16_data[i] > Mxpos[p] ){
                    Mxpos[p] = dp16_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                  /* Is this the minimum for the elevation? */
                 if( dp16_data[i] < Mnpos[p] && dp16_data[i] > RF_FLAG){
                    Mnpos[p] = dp16_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if PHI */
           if (strcmp(OUTDATA_NAME[p],"ZDR8BIT") == 0 &&
                            radhead->n_surv_bins != 0 ) {
              /* For 8 bit ZDR data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp_data[i] > Mxpos[p] ){
                    Mxpos[p] = dp_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                 /* Is this the minimum for the elevation? */
                 if( dp_data[i] < Mnpos[p] && dp_data[i] > RF_FLAG){
                    Mnpos[p] = dp_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if ZDR */
           if (strcmp(OUTDATA_NAME[p],"SNR8BIT") == 0 &&
                            radhead->n_surv_bins != 0 ) {
              /* For 8 bit SNR data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp_data[i] > Mxpos[p] ){
                    Mxpos[p] = dp_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                 /* Is this the minimum for the elevation? */
                 if( dp_data[i] < Mnpos[p] && dp_data[i] > RF_FLAG){
                    Mnpos[p] = dp_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if SNR */
           if (strcmp(OUTDATA_NAME[p],"SMZ8BIT") == 0 &&
                            radhead->n_surv_bins != 0 ) {
              /* For 8 bit SMZ data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp_data[i] > Mxpos[p] ){
                    Mxpos[p] = dp_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                 /* Is this the minimum for the elevation? */
                 if( dp_data[i] < Mnpos[p] && dp_data[i] > RF_FLAG){
                    Mnpos[p] = dp_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if SMZ */
           if (strcmp(OUTDATA_NAME[p],"SDZ8BIT") == 0 &&
                            radhead->n_surv_bins != 0 ) {
              /* For 8 bit SDZ data, determine minimum and maximum data value in the radial */
              for( i = 0;  i < numbins; i++ ){
                 /* Is this the maximum for the elevation? */
                 if( dp_data[i] > Mxpos[p] ){
                    Mxpos[p] = dp_data[i];
                    Mxbin[p] = i;
                    Mxazm[p] = start/10;
                 }
                 /* Is this the minimum for the elevation? */
                 if( dp_data[i] < Mnpos[p] && dp_data[i] > RF_FLAG){
                    Mnpos[p] = dp_data[i];
                    Mnbin[p] = i;
                    Mnazm[p] = start/10;
                 }
              } /* end for all bins */
           } /* end if SDZ */
           if (radhead->azi_num == DEBUG_AZ) {
             RPGC_log_msg(GL_INFO,"start=%d delta=%d numbins=%d",
                                   start, delta, numbins);
             RPGC_log_msg(GL_INFO,"product %s original word_size = %d scale=%f offset=%f",
                       OUTDATA_NAME[p], gm.data_word_size, Scale, Offset);       
           }
           
           ndpbyts = 0;
           /* Fill the digital radial data array */   
           if (strcmp(OUTDATA_NAME[p],"HC8BIT")  == 0 &&
                            radhead->n_surv_bins != 0) {
              /* This branch handles 1-byte HCA data. */
              short *tempbuff = outbuff[p];
              ndpbyts = RPGC_digital_radial_data_array( (void *) hc_charbuf, RPGC_BYTE_DATA,
                                                         frst_bin, last_bin, 0, numbins, 1, start,
                                                         delta, (void *) &tempbuff[pbuffind[p]] );
           }

           else if ((strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0 ||
                     strcmp(OUTDATA_NAME[p],"CC8BIT")  == 0 ||
                     strcmp(OUTDATA_NAME[p],"PHI8BIT") == 0) &&
                                  radhead->n_surv_bins != 0) {
              /* This branch handles 16 bit word data */
              short *tempbuff = outbuff[p];
              
              numbins = 0;
              if (gm.data_word_size == 8) {
                 dp_data = (unsigned char*)dp16_data;
                 for (i=0; i < gm.no_of_gates; i++) {
                    shortbuf[numbins] = dp_data[i];
                    /* Cap the KDP values at 10.0 deg/km per ICD */
                    if (strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0 &&
                        (int)shortbuf[numbins] > RF_FLAG) {
                       temp = ((float)shortbuf[numbins] - Offset) / Scale;
                       if (temp > MAX_KDP_DISPLAY){
/*                          if (DEBUG_AZ > 0) RPGC_log_msg(GL_INFO,"TRUNCATING KDP8 %f",temp);*/
                          temp = MAX_KDP_DISPLAY;
                       }
                       shortbuf[numbins] = (unsigned short)roundf((temp * KDP_ICD_SCALE) + KDP_ICD_OFFSET);
                    }
                    else if (strcmp(OUTDATA_NAME[p],"PHI8BIT") == 0 &&
                        (int)shortbuf[numbins] > RF_FLAG) {
                       temp = ((float)shortbuf[numbins] - Offset) / Scale;
                       if (temp > MAX_PHI_DISPLAY){
/*                          if (DEBUG_AZ > 0) RPGC_log_msg(GL_INFO,"TRUNCATING PHI8 %f",temp);*/
                          temp = MAX_PHI_DISPLAY;
                       }
                       shortbuf[numbins] = (unsigned short)roundf((temp * PHI_ICD_SCALE) + PHI_ICD_OFFSET);
                    }
                    numbins++;
                    if (numbins == MAXBINS) break;
                 } /* end for all bins */
              }
              else {
                 /* Reduce data precision to 8 bits */
                 gm.data_word_size = 8;
                 for (i=0; i < gm.no_of_gates; i++) {
                    if (dp16_data[i] > RF_FLAG) {
                       /* Obtain data value using provided scale and offset */
                       temp = ((float)dp16_data[i] - Offset) / Scale;

                       /* Cap the KDP values at 10.0 deg/km per ICD */
                       if (strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0) {
                          if (temp > MAX_KDP_DISPLAY){
/*                             if (DEBUG_AZ > 0)RPGC_log_msg(GL_INFO,"TRUNCATING KDP16 %f",temp);*/
                             temp = MAX_KDP_DISPLAY;
                          }
                          shortbuf[numbins] = (unsigned short)roundf((temp * KDP_ICD_SCALE) + KDP_ICD_OFFSET);
                       }
                       else if (strcmp(OUTDATA_NAME[p],"CC8BIT") == 0) {
                          shortbuf[numbins] = (unsigned short)roundf((temp * CC_ICD_SCALE) + CC_ICD_OFFSET);
                       }
                       else if (strcmp(OUTDATA_NAME[p],"PHI8BIT") == 0) {
                          /* Cap the Phi values at 720 degrees for display */
                          if (temp > MAX_PHI_DISPLAY){
/*                             if (DEBUG_AZ > 0)RPGC_log_msg(GL_INFO,"TRUNCATING PHI16 %f",temp);*/
                             temp = MAX_PHI_DISPLAY;
                          }
                          shortbuf[numbins] = (unsigned short)roundf((temp * PHI_ICD_SCALE) + PHI_ICD_OFFSET);
                       }
                    }
                    else {
                       /* Flag data can be transfered directly */
                       shortbuf[numbins] = dp16_data[i];
                    }
                    numbins++;
                    if (numbins == MAXBINS) break;
                 } /* end for all bins */
              } /* end reducing data precision */

              ndpbyts = RPGC_digital_radial_data_array( (void *) shortbuf, RPGC_SHORT_DATA,
                                                         frst_bin, last_bin, 0, numbins, 1, start,
                                                         delta, (void *) &tempbuff[pbuffind[p]] );
/**              if (radhead->azi_num == DEBUG_AZ && 0) {
                 int x;
                 for(x=0;x<60;x++){
                    RPGC_log_msg(GL_INFO,"dp16_data[%d]=%d",x,(short)(dp16_data[x]));
                 }
              }**/
           }   
           else if (radhead->n_surv_bins != 0) {
              /* This branch handles all other 8 bit word data */
              short *tempbuff = outbuff[p];
              numbins = 0;
              for (i=0; i < gm.no_of_gates; i++) {
                 if (dp_data[i] > RF_FLAG) {
                    temp = ((float)dp_data[i] - Offset) / Scale;
                    /* Here we assume the remaining debug products
                       do not have to be rescaled                  */
                    charbuf[numbins] = (unsigned short)roundf((temp * Scale) + Offset);
                 }
                 else {
                     /* Flag data can be transfered directly */
                     charbuf[numbins] = dp_data[i];
                 }
                 numbins++;
                 if (numbins == MAXBINS) break;
              } /* end for all bins */

              ndpbyts = RPGC_digital_radial_data_array( (void *) charbuf, RPGC_BYTE_DATA,
                                                         frst_bin, last_bin, 0, numbins, 1, start,
                                                         delta, (void *) &tempbuff[pbuffind[p]] );
/**              if (radhead->azi_num == DEBUG_AZ && 0) {
                 int x;
                 for(x=0;x<60;x++){
                    RPGC_log_msg(GL_INFO,"dp_data[%d]=%d",x,(short)(dp_data[x]));
                 }
              }**/
           } /* end filling digital radial data array */
           if(359 == radhead->azi_num)
               RPGC_log_msg(GL_INFO,"product %s final word_size = %d scale=%f offset=%f",
                                  OUTDATA_NAME[p], gm.data_word_size, Scale, Offset);        
           /* Update buffer counters and pointers. */
	      ndpb[p] += ndpbyts;
	      pbuffind[p] += ndpbyts / 2;
        }
        else {
          RPGC_log_msg(GL_ERROR,"BUFFER TOO SMALL  p=%d radcount[p]=%d OBUF_SIZE=%d, ndpb[p]=%d",
                                 p,Radcount[p],OBUF_SIZE, ndpb[p]);

	   } /* end if excess > EST_PER_RAD */

    } /* end if Proc_rad */

    /* Test for last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){

	  Endelcut = 1;

       /* If last radial encountered, fill remaining fields in product buffer. */
       elevindex = RPGC_get_buffer_elev_index( bdataptr );
	  Dualpol8bit_end_of_product_processing( ndpb[p], elevindex, numbins, p, obrptr );
    }

    /* Return to buffer control routine. */
    return 0;

/* End of Dualpol8bit_product_generation_control() */
} 


/********************************************************************
   
   Description:
      Fills in product description block, symbology block information.

   Inputs:
      outbuff - pointer to output buffer.
      vol_num - volume scan number.
      p - index to the output product being generated.

   Returns:
      Always returns 0.

********************************************************************/
int Dualpol8bit_product_header( char *outbuff, int vol_num, int p ){

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                                 (outbuff + sizeof(Graphic_product));
    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, Prod_id[p], vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    params[2] = Elmeas;

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

/* End of Dualpol8bit_product_header() */
}


/**********************************************************************

   Description:
      Fill remaining Product Header fields for 256-level Dual 
      Pol product. 


   Inputs:
      ndpd - number of bytes in the product so far.
      numbins - number of bins in a product radial.
      p - index to the output product being generated.
      outbuff - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int Dualpol8bit_end_of_product_processing( int ndpb, int elev_ind,
                                           int numbins, int p, char *outbuff ){

    int bytecnt;
    float maxval = 0.0;
    float minval = 0.0;

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                           (outbuff + sizeof(Graphic_product));
    Packet_16_hdr_t *packet_16 = (Packet_16_hdr_t *) 
              (outbuff + sizeof(Graphic_product) + sizeof(Symbology_block));

    /* Complete the packet 16 header. */
    RPGC_digital_radial_data_hdr( 0, numbins, 0, 0, Vc, Radcount[p], 
                                  (void *) packet_16 );


    if (strcmp(OUTDATA_NAME[p],"SMV8BIT") == 0) {
       /* Data level threshold codes. */
       phd->level_1 = (short)-635;
       phd->level_2 = (short)5;
       phd->level_3 = (short)254;
    }
    else {
       /* Data level threshold codes. */
       if (strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0) {
          phd->level_6 = (short)243; /* Maximum data value */
       }
       else {
          phd->level_6 = (short)255; /* Maximum data value */
       }
       if (strcmp(OUTDATA_NAME[p],"HC8BIT") == 0) {
          phd->level_7 = (short)0; /* No leading flags  */
          phd->level_8 = (short)0; /* No trailing flags */
       }
       else {
          phd->level_7 = (short)2; /* Leading flags:  0 = below threshold, 1 = RF */
          phd->level_8 = (short)0; /* No trailing flags */
       }
    }

    /* Elevation index                   */
    phd->elev_ind=(short)elev_ind;

    /* Assign the minimum and maximum data levels to the product header and */
    /* the ICD defined scale and offset.  However, use the orignal scale    */
    /* and offset as received from input to convert min and max values.     */
    if (DEBUG_AZ > 0) 
       RPGC_log_msg(GL_INFO,"product=%s Scale=%f, Offset=%f Mnpos=%d Mxpos=%d",
                          OUTDATA_NAME[p],Scale,Offset,Mnpos[p],Mxpos[p]);
    if (strcmp(OUTDATA_NAME[p],"KDP8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,KDP_ICD_SCALE);
       RPGC_set_product_float((void*)&phd->level_3,KDP_ICD_OFFSET);
       minval = (((float)Mnpos[p]) - Offset) / Scale;
       maxval = (((float)Mxpos[p]) - Offset) / Scale;
       /* Cap the max KDP value at 10 deg/km per ICD */
       if (maxval > MAX_KDP_DISPLAY) maxval = MAX_KDP_DISPLAY;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / KDP_MINMAX_PRECISION;
       maxval = maxval / KDP_MINMAX_PRECISION;
    }
    else if (strcmp(OUTDATA_NAME[p],"CC8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,CC_ICD_SCALE);
       RPGC_set_product_float((void*)&phd->level_3,CC_ICD_OFFSET);
       minval = (((float)Mnpos[p]) - Offset) / Scale;
       maxval = (((float)Mxpos[p]) - Offset) / Scale;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / CC_MINMAX_PRECISION;
       maxval = maxval / CC_MINMAX_PRECISION;
    }
    else if (strcmp(OUTDATA_NAME[p],"ZDR8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,ZDR_ICD_SCALE);
       RPGC_set_product_float((void*)&phd->level_3,ZDR_ICD_OFFSET);
       minval = ((float)Mnpos[p] - Offset) / Scale;
       maxval = ((float)Mxpos[p] - Offset) / Scale;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / ZDR_MINMAX_PRECISION;
       maxval = maxval / ZDR_MINMAX_PRECISION;
    }
  /* Products below this point are non-operational and included only for testing purposes */
    else if (strcmp(OUTDATA_NAME[p],"SMZ8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,Scale);
       RPGC_set_product_float((void*)&phd->level_3,Offset);
       minval = ((float)Mnpos[p] - Offset) / Scale;
       maxval = ((float)Mxpos[p] - Offset) / Scale;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / 0.5;
       maxval = maxval / 0.5;
    }
    else if (strcmp(OUTDATA_NAME[p],"SNR8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,Scale);
       RPGC_set_product_float((void*)&phd->level_3,Offset);
       minval = ((float)Mnpos[p] - Offset) / Scale;
       maxval = ((float)Mxpos[p] - Offset) / Scale;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / 0.5;
       maxval = maxval / 0.5;
    }
    else if (strcmp(OUTDATA_NAME[p],"PHI8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,PHI_ICD_SCALE);
       RPGC_set_product_float((void*)&phd->level_3,PHI_ICD_OFFSET);
       minval = (((float)Mnpos[p]) - Offset) / Scale;
       maxval = (((float)Mxpos[p]) - Offset) / Scale;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / 0.5;
       maxval = maxval / 0.5;
    }
    else if (strcmp(OUTDATA_NAME[p],"SDZ8BIT") == 0) {
       RPGC_set_product_float((void*)&phd->level_1,Scale);
       RPGC_set_product_float((void*)&phd->level_3,Offset);
       minval = ((float)Mnpos[p] - Offset) / Scale;
       maxval = ((float)Mxpos[p] - Offset) / Scale;
       RPGC_log_msg(GL_INFO,"%s Min[%d][%d]=%f  Max[%d][%d]=%f",
                       OUTDATA_NAME[p],Mnazm[p],Mnbin[p],minval,
                                       Mxazm[p],Mxbin[p],maxval);
       minval = minval / 0.5;
       maxval = maxval / 0.5;
    }

    phd->param_4 = (short) RPGC_NINT( minval );
    phd->param_5 = (short) RPGC_NINT( maxval );

    /* Calculate and store the product message length, the product block 
       length and the product layer length. */

    /* Length of product layer. */
    bytecnt = ndpb + sizeof(Packet_16_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* Complete the product header. */
    RPGC_prod_hdr( outbuff, Prod_id[p], &bytecnt);

    Totalbytes[p] = bytecnt+100;

    /* Return to the product generation control routine. */
    return 0;

/* End of Dualpol8bit_end_of_product_processing() */
} 
