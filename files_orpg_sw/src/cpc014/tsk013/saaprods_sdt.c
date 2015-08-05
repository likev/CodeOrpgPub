/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2005/10/17 18:51:28 $
 * $Id: saaprods_sdt.c,v 1.4 2005/10/17 18:51:28 cheryls Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaprods_sdt.c

Description:   Snow Storm Total Depth product generation driver for cpc014/tsk013
               SAA (Snow Accumulation Algorithm) task. From this module, the ICD
               compliant products are generated and a success/failure result code
               is returned to the main calling routine for each product.  The 
               products generated are one hour water equivalent (OSW, 144), 
               one-hour depth (OSD, 145), storm total snow water equivalent 
               (SSW, 146), and the storm total snow depth (SSD, 147).
               
Input:            int max_bufsize   maximum buffer size for outbuf
                  int PROD_ID	    Product linear buffer number
                  
Output:           char* output      pointer to the output buffer where the 
                                    final product will be created
                  
Returns:       returns SAA_SUCCESS or SAA_FAILURE
                  
Globals:        char* inbuf
		int hi_sf_flg

Notes:         none
               
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 8/08/03 - Zittel
               11/04/2004	SW CCR NA04-30810	Build8 Changes
               10/17/2005	Change Range to nmi	Build8
               
*******************************************************************************/


/* local include file */
#include "saaprods_sdt.h"
#include "saaprods_main.h"
#include "build_saa_color_tables.h"

#define MIN_IN_HOUR 60
#define KM2NMI      0.54   /* Convert range to max value moved to this task for 
                              Build 8                                            */

extern int hi_sf_flg;						      /* Build 8 */

int compute_area(short w[0], int x, int y, int z);

int generate_sdt_output(char *outbuf, int max_bufsize, int PROD_ID) {
   int azm_maxval, rng_maxval, max_value;
   int i,j;                /* loop variables                                     */
   int result = -1;        /* variable to hold function call results             */
   int debugit=FALSE;      /* flag to turn on debugging output to stderr         */
   SDT_prod_header_t phdr; /* pointer to an intermediate product header    */
   Outbuf_t *saa_obuf;     /* pointer to a temporary product array               */
   int length=0;           /* length accumulator                                 */
   int product_length;     /* length of the final/completed product              */
   int offset=0;           /* used to hold the access/offset point for structs   */
   short radial_data[MAX_SAA_RADIALS][MAX_SAA_BINS]; /* input radial container     */
   int tab_offset;         /* holds the offset for the beginning of the TAB block*/
   int msg_type;
   int area;		   /* Build8, variable to hold area returned from compute_area */

   saa_obuf = (Outbuf_t*)outbuf;
   if(debugit){
     fprintf(stderr,"VCP = %d\n",saa_inp.vcp_num);
     fprintf(stderr,"saa_inp.begin_date = %d, saa_inp.begin_time = %d\n",
                   saa_inp.begin_date, saa_inp.begin_time);
     fprintf(stderr,"saa_inp.current_date = %d, saa_inp.current_time = %d\n",
                   saa_inp.current_date, saa_inp.current_time);
     fprintf(stderr,"saa_inp.ohp_start_date = %d, saa_inp.ohp_start_time = %d\n",
                   saa_inp.ohp_start_date, saa_inp.ohp_start_time);
     fprintf(stderr,"saa_inp.ohp_end_date = %d, saa_inp.ohp_end_time = %d\n",
                   saa_inp.ohp_end_date, saa_inp.ohp_end_time);
     fprintf(stderr,"saa_inp.stp_start_date = %d, saa_inp.stp_start_time = %d\n",
                   saa_inp.stp_start_date, saa_inp.stp_start_time);
     fprintf(stderr,"saa_inp.stp_missing_period = %7.4f\n",saa_inp.stp_missing_period);
     fprintf(stderr,"saa_inp.ohp_missing_period = %7.4f\n",saa_inp.ohp_missing_period);
     }
   phdr.date = saa_inp.begin_date;
   phdr.time = saa_inp.begin_time;
   phdr.volume_scan_num = RPGC_get_current_vol_num();
   phdr.flags = 0;					/* Name change for Build 8 */
   phdr.rpg_elev_ind =0;
   RPGC_log_msg( GL_INFO, "saaprods: begin creating product %d for volume: %d\n",
                 PROD_ID,phdr.volume_scan_num);

   /* building the ICD formatted output product requires a few steps ======== */
   /* step 1: build the product description block (pdb)                       */
   /* step 2: build the symbology block & data layer                          */
   /* step 3: build the tabular alphanumeric block                            */
   /* step 4: complete missing values in the pdb                              */
   /* step 5: build the message header block (mhb)                            */
   /* step 6: forward the completed product to the system                     */
   
   /* step 1: build the product description block -uses an ORPG system call   */
   /* which requires passing in a pointer to the output buffer, the product   */
   /* code and the current volume scan number                                 */

   RPGC_prod_desc_block((void*)outbuf,PROD_ID,phdr.volume_scan_num);
   
   if(debugit)
      fprintf(stderr,"Back from call to RPGC_prod_desc_block, vscan = %3d\n",
              phdr.volume_scan_num);
   
   /* step 2: build the symbology layer & RLE (run length encode) radial data */
   /* packet (AF1F). The first part of this process is to translate the data  */
   /* within the input buffer to the radial container array which is then fed */
   /* into the symbology block generator.                                     */
   
    if(debugit){fprintf(stderr,"SDT: Calculated offset = %d\n",offset);}
    azm_maxval = 0;
    rng_maxval = 0;
    max_value  = 0;
    area       = 0;							/* Build8 */
    hi_sf_flg  = FALSE;     						/* Build8 */

/*  Copy SSW data to the radial_data array  */
   if(saa_inp.stp_flg){
      if(PROD_ID == SSWACCUM ){
      /* Next 3 lines added for Build8  */
         area = compute_area(saa_inp.swt_data[0],MAX_SAA_RADIALS, MAX_SAA_BINS, SSWACCUM);
         if(area > AREA_THRES)
            hi_sf_flg = TRUE;
         memcpy(&radial_data, saa_inp.swt_data, sizeof(radial_data));
      }

/*  Copy SSD data to the radial_data array  */      
      if(PROD_ID == SSDACCUM ){
      /* Next 3 lines added for Build8  */
         area = compute_area(saa_inp.sdt_data[0],MAX_SAA_RADIALS, MAX_SAA_BINS, SSDACCUM);
         if(area > AREA_THRES)
            hi_sf_flg = TRUE;
         for(i=0;i<MAX_SAA_RADIALS;i++)
            for(j=0;j<MAX_SAA_BINS;j++)
               radial_data[i][j]=saa_inp.sdt_data[i][j]/10;
      }
   }
   
   if(saa_inp.ohp_flg){
/*  Copy OSW data to the radial_data array  */      
      if(PROD_ID == OSWACCUM )
         memcpy(&radial_data, saa_inp.swo_data, sizeof(radial_data));

/*  Copy OSD data to the radial_data array  */
      if(PROD_ID == OSDACCUM )
         memcpy(&radial_data, saa_inp.sdo_data, sizeof(radial_data));

      if(debugit) {
         if(saa_inp.ohp_flg){
            fprintf(stderr,"\narray loaded first radial to follow\n");
            for(j=0;j<MAX_SAA_BINS;j++)
              fprintf(stderr," %02X",(unsigned)radial_data[0][j]);
            fprintf(stderr,"\n\n");
          }
       }

   }

/*  The following line was modified to show in the high byte if the hi_sf_flg was set for Build8 */    
    phdr.flags = saa_inp.use_RCA_flag + 256*hi_sf_flg;	      /* Build8  */

   if(debugit){fprintf(stderr,"PROD_ID = %d, flags = %d\n",PROD_ID, phdr.flags);}

   /* Initialize the tab offset to zero                          */   
   tab_offset = 0;
   
   if(PROD_ID==OSWACCUM || PROD_ID==OSDACCUM){
     /* Put in start and ending dates and times for SDO and SWO  */
     saa_obuf->halfword[26] = saa_inp.ohp_missing_period*MIN_IN_HOUR;
     saa_obuf->halfword[47] = saa_inp.ohp_start_date;
     saa_obuf->halfword[48] = saa_inp.ohp_start_time;
     saa_obuf->halfword[49] = saa_inp.ohp_end_date;
     saa_obuf->halfword[50] = saa_inp.ohp_end_time;
     if(saa_inp.ohp_flg){
       /* call the symbology layer generator. the length of the product will      
          be returned through "length"                                            */
       result=build_saa_symbology_layer((short*)outbuf,MAX_SAA_RADIALS,&length,
          radial_data,PROD_ID,max_bufsize);
       if(debugit){fprintf(stderr,"Result from symbology = %d\n",result);}
       if(result == SAA_FAILURE){
          msg_type = 1;
          result = build_nullsymbology_layer((short*)outbuf,msg_type,&length,
            PROD_ID,max_bufsize);
          if(result == SAA_FAILURE)
             return SAA_FAILURE;
       }
       /*  Insert the maximum value into the product buffer */
       max_value = saa_max_value(radial_data[0], &azm_maxval, &rng_maxval);

   /*  Load the max value, azimuth, and range to the maximum value into the output array  */
       saa_obuf->halfword[46] = max_value;
       saa_obuf->halfword[51] = azm_maxval;
   /*  Conversion from km to nmi moved from saaprods_tab.c for Build 8  */
       saa_obuf->halfword[52] = (int)((float)(rng_maxval)*KM2NMI + 0.5);
       if(debugit)
          fprintf(stderr,"max_value = %d,azm = %d, rng = %d\n",
                  max_value,azm_maxval,rng_maxval);
       }
     if(!saa_inp.ohp_flg){
       msg_type = 2;
       result=build_nullsymbology_layer((short*)outbuf,msg_type,&length,
           PROD_ID,max_bufsize);
       if(debugit){fprintf(stderr,"Result from symbology = %d\n",result);}
       if(result == SAA_FAILURE)
          return SAA_FAILURE;
       saa_obuf->halfword[46] = 0;
       saa_obuf->halfword[51] = 0;
       saa_obuf->halfword[52] = 0;
       }
     }

   if(PROD_ID == SSWACCUM || PROD_ID == SSDACCUM){
     /*  Put in starting and ending dates and times for SDT & SWT products */
     saa_obuf->halfword[26] = saa_inp.stp_missing_period*MIN_IN_HOUR;
     saa_obuf->halfword[47] = saa_inp.stp_start_date;
     saa_obuf->halfword[48] = saa_inp.stp_start_time;
     saa_obuf->halfword[49] = saa_inp.ohp_end_date;
     saa_obuf->halfword[50] = saa_inp.ohp_end_time;
     if(!saa_inp.stp_flg){
        msg_type = 3;
        result=build_nullsymbology_layer((short*)outbuf,msg_type,&length,
             PROD_ID,max_bufsize);
        if(debugit){fprintf(stderr,"Result from symbology = %d\n",result);}
        if(result == SAA_FAILURE)
           return SAA_FAILURE;
        saa_obuf->halfword[46] = 0;
        saa_obuf->halfword[51] = 0;
        saa_obuf->halfword[52] = 0;
        saa_obuf->halfword[50] = saa_obuf->halfword[48];
        }
     if(saa_inp.stp_flg){
       /*  Insert the maximum value into the product buffer */
       max_value = saa_max_value(radial_data[0], &azm_maxval, &rng_maxval);

   /*  Load the max value, azimuth, and range to the maximum value into the output array  */
       saa_obuf->halfword[46] = max_value;
       saa_obuf->halfword[51] = azm_maxval;
   /*  Conversion from km to nmi moved from saaprods_tab.c for Build 8  */
       saa_obuf->halfword[52] = (int)((float)(rng_maxval)*KM2NMI + 0.5);
       if(debugit)
          fprintf(stderr,"max_value = %d,azm = %d, rng = %d\n",
                  max_value,azm_maxval,rng_maxval);
        /* call the symbology layer generator. the length of the product will      */
        /* be returned through "length"                                            */
        result=build_saa_symbology_layer((short*)outbuf,MAX_SAA_RADIALS,&length,
           radial_data,PROD_ID,max_bufsize);
        if(debugit){fprintf(stderr,"Result from symbology = %d\n",result);}
        if(result == SAA_FAILURE){
           msg_type = 1;
           result = build_nullsymbology_layer((short*)outbuf,msg_type,&length,
             PROD_ID,max_bufsize);
           if(result == SAA_FAILURE)
 	      return SAA_FAILURE;
   	 }
      }
   }

   /* save the offset for the start of the TAB block                          */
   tab_offset=length;
   result=RPGC_prod_hdr((void*)outbuf,PROD_ID,&product_length);
   /* step 3: enter the TAB block generator                                   */
   product_length=generate_TAB(outbuf,tab_offset,PROD_ID,max_bufsize);
   
   if(debugit)
      fprintf(stderr,"SDT accumulated product length =%d\n",
         product_length);
   if(product_length == SAA_FAILURE)
      return (SAA_FAILURE);
   
   /* step 4: finish building the product description block by filling in     */
   /* certain values such as elevation index, target elev and block offsets   */
   if(debugit)
      fprintf(stderr,"before finish_SDT_pbd: length=%d tab_offset=%d\n",
         product_length, tab_offset);
   finish_SDT_pdb((short*)outbuf,phdr.rpg_elev_ind, phdr.flags,length,
      tab_offset,PROD_ID);		/* phdr.flags used for Build 8  */
      
   /* generate the product message header (system call) and input total       */
   /* accumulated product length (minus 120 bytes) for the MHB & PDB          */
   product_length-=120;
   result=RPGC_prod_hdr((void*)outbuf,PROD_ID,&product_length);
   if(debugit){fprintf(stderr,"Result from prod_hdr = %d\n",result);}  
   saa_obuf->halfword[17] = saa_inp.vcp_num;
   saa_obuf->halfword[19] = phdr.volume_scan_num;  /* RPGC_get_current_vol_num(); */
   saa_obuf->halfword[28] = phdr.rpg_elev_ind;
   saa_obuf->halfword[53] = SAA_VERSION*256 + saa_inp.vol_sb;
   if(debugit) {
      for (i=0;i<product_length/2;++i)
        {
        fprintf(stderr," %04X ",saa_obuf->halfword[i]);
        if(i % 8 == 1)
           fprintf(stderr,"\nI = %5d ",i+2);
        }
      }
   return(result);
}
