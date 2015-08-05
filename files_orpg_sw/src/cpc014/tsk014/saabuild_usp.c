/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 21:32:34 $
 * $Id: saabuild_usp.c,v 1.9 2008/01/04 21:32:34 aamirn Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saabuild_usp.c

Description:   Snow Storm Total Depth product generation driver for cpc014/tsk014
               SAA (Snow Accumulation Algorithm) task. From this module, the ICD
               compliant product is generated and a success/failure result code
               is returned to the main calling routine.
               
Input:         prod_id
	       int max_bufsize   maximum buffer size for outbuf
                  
Output:           char* output      pointer to the output buffer where the 
                                    final product will be created
                  
Returns:          returns SAA_SUCCESS or SAA_FAILURE
                  
Globals:          hi_sf_flg, 

Notes:            none

CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 8/08/03    - Zittel
               Build 8 Range Correction 10/17/05 - Zittel
               
*******************************************************************************/

/* local include file */
#include "saabuild_usp.h"
#include "saausers_main.h"
#include "build_saa_color_tables.h"

#define KM2NMI 0.54        /* Convert range to max value moved to this task for 
                              Build 8                                            */

extern int hi_sf_flg;

int generate_sdt_output(char *outbuf, int max_bufsize, int PROD_ID,int usp_flg,int usp_rnum) {
   int azm_maxval, rng_maxval, max_value;
   register int i;      /* loop variables                                     */
   register int j;      /* loop variables                                     */
   int result = -1;     /* variable to hold function call results             */
   int debugit=FALSE;      /* flag to turn on debugging output to stderr         */
   SDT_prod_header_t phdr;   /* pointer to an intermediate product header    */
   Outbuf_t *saa_obuf;      /* pointer to a temporary product array               */
   int length=0;        /* length accumulator                                 */
   int product_length;  /* length of the final/completed product              */
   int offset=0;        /* used to hold the access/offset point for structs   */
   short radial_data[MAX_SAA_RADIALS][MAX_SAA_BINS]; /* input radial container     */
   int tab_offset;      /* holds the offset for the beginning of the TAB block*/
   int msg_type;
   int scale_factor;

   saa_obuf = (Outbuf_t*)outbuf;
   if(debugit){
     fprintf(stderr,"VCP = %d\n",saa_inp.vcp_num);
     fprintf(stderr,"saa_inp.begin_date = %d, saa_inp.begin_time = %d\n",
                   saa_inp.begin_date, saa_inp.begin_time);
     }
   phdr.date = saa_inp.begin_date;
   phdr.time = saa_inp.begin_time;
   phdr.volume_scan_num = RPGC_get_current_vol_num();
   phdr.flags = 0;
   phdr.rpg_elev_ind =0;
   RPGC_log_msg( GL_INFO, "Begin creating product %d for volume: %d\n",
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
    max_value = 0;     
    scale_factor = 10;
    hi_sf_flg = FALSE;

/*  Copy USW data to the radial_data array  */      
   if(usp_flg == TRUE){
      if(PROD_ID == USWACCUM ){

         if(usp.max_swu > AREA_THRES){
            hi_sf_flg = TRUE;
            for(i=0;i<MAX_SAA_RADIALS;i++) 
               for(j=0;j<MAX_SAA_BINS;j++) 
                  radial_data[i][j]=usp.total_swu[i][j]/scale_factor;
                  
          }
          else
             memcpy(&radial_data, usp.total_swu, sizeof(radial_data));
       }

/*  Copy USD data to the radial_data array  */      
      if(PROD_ID == USDACCUM ){

         if(usp.max_sdu > AREA_THRES){
            hi_sf_flg = TRUE;
            for(i=0;i<MAX_SAA_RADIALS;i++)
               for(j=0;j<MAX_SAA_BINS;j++)
                  radial_data[i][j]=usp.total_sdu[i][j]/scale_factor;
          }
          else
             memcpy(&radial_data, usp.total_sdu, sizeof(radial_data));
       }
       

/*  Find the maximum value in the data and the azimuth and range to the    */
/*  maximum value                                                          */   
      max_value = saa_max_value(radial_data[0], &azm_maxval, &rng_maxval);
      if(debugit)
         fprintf(stderr,"max_value = %d,azm = %d, rng = %d\n",
                   max_value,azm_maxval,rng_maxval);
   }  /*  End If check usp_flg  */

     if(debugit) {
        fprintf(stderr,"Max_val = %d at %d/%d\n",max_value,azm_maxval,rng_maxval);

       if(saa_inp.ohp_flg){
         fprintf(stderr,"\narray loaded first radial to follow\n");
         for(j=0;j<MAX_SAA_BINS;j++)
           fprintf(stderr," %02X",(unsigned)radial_data[0][j]);
         fprintf(stderr,"\n\n");
         }
       }
      
   /* call the symbology layer generator. the length of the product will      */
   /* be returned through "length"                                            */
   
   /*  Load the azimuth and range to the maximum value into the output array  */
       saa_obuf->halfword[51] = azm_maxval;
   /*  Conversion from km to nmi moved from saausers_tab.c for Build 8  */
       saa_obuf->halfword[52] = (int)((float)(rng_maxval)*KM2NMI + 0.5);
   /*  Load type of range correction static; = 0/RCA = 1  in the lower byte
   	and load the scale factor into the high byte per Steve Smith         
   	Scale	Water Equiv.	Depth
   	0  low 	 0.001           0.01	
   	1  high  0.01   	  0.1                                  */
   phdr.flags  = (hi_sf_flg << 8 ) | (saa_inp.use_RCA_flag & 0xff);
/*   saa_obuf->halfword[29] = (hi_sf_flg << 8 ) | (saa_inp.use_RCA_flag & 0xff); */
   if(debugit){fprintf(stderr,"flags = %d\n",phdr.flags);}
   
   tab_offset = 0;

   if(PROD_ID==USWACCUM || PROD_ID==USDACCUM){
     if(debugit){fprintf(stderr,"SDT: hi_sf_flg = %d\n",hi_sf_flg);}
   /* Put in start and ending dates and times for USW and USD  */
     saa_obuf->halfword[47] = hskp_data.usr_first_date;
     saa_obuf->halfword[48] = hskp_data.usr_first_time;
     saa_obuf->halfword[49] = hskp_data.usr_last_date;
     saa_obuf->halfword[50] = hskp_data.usr_last_time;
     if(PROD_ID==USWACCUM){
        saa_obuf->halfword[26] = userreq.swu_end_hour[usp_rnum];
        saa_obuf->halfword[27] = userreq.swu_num_hours[usp_rnum];
     }
     else{
        saa_obuf->halfword[26] = userreq.sdu_end_hour[usp_rnum];
        saa_obuf->halfword[27] = userreq.sdu_num_hours[usp_rnum];
     }
     if(usp_flg==TRUE){
       result=build_saa_symbology_layer((short*)outbuf,MAX_SAA_RADIALS,&length,
          radial_data,PROD_ID,max_bufsize);
       if(debugit){fprintf(stderr,"Result from symbology = %d\n",result);}
       if(result == SAA_FAILURE){
          msg_type = 1;  /*  Indicates product size exceeds buffer size */
          result=build_nullsymbology_layer((short*)outbuf,msg_type,&length,
              PROD_ID,max_bufsize);
		  /* If test added for Build8.  Return failure only if null product
		      cannot be built.                                WDZ 02142005  */
		  if(result == SAA_FAILURE)
             return SAA_FAILURE;
          }
       /*  Insert the maximum value into the product buffer */
       saa_obuf->halfword[46] = max_value;
       }
     if(usp_flg != TRUE){
       msg_type = abs(usp_flg);
       result=build_nullsymbology_layer((short*)outbuf,msg_type,&length,
           PROD_ID,max_bufsize);
       if(debugit){fprintf(stderr,"Result from symbology = %d\n",result);}
       if (result == SAA_FAILURE)
          return SAA_FAILURE;
          
       saa_obuf->halfword[46] = 0;
       }
     }


   if(debugit){fprintf(stderr,"Reji's max value = %d\n",saa_obuf->halfword[46]);}
   /* save the offset for the start of the TAB block                          */
   tab_offset=length;
   /* step 3: enter the TAB block generator                                   */
   result=RPGC_prod_hdr((void*)outbuf,PROD_ID,&product_length);

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
   finish_SDT_pdb((short*)outbuf,phdr.rpg_elev_ind ,phdr.flags,length,
      tab_offset,PROD_ID);
   /* generate the product message header (system call) and input total       */
   /* accumulated product length (minus 120 bytes) for the MHB & PDB          */
   product_length-=120;
   result=RPGC_prod_hdr((void*)outbuf,PROD_ID,&product_length);
   if(debugit){fprintf(stderr,"Result from prod_hdr = %d\n",result);}
   saa_obuf->halfword[17] = saa_inp.vcp_num;
   saa_obuf->halfword[19] = phdr.volume_scan_num;  /* RPGC_get_current_vol_num(); */
   saa_obuf->halfword[28] = phdr.rpg_elev_ind;
   saa_obuf->halfword[53] = SAA_VERSION * 256 + saa_inp.vol_sb;
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
