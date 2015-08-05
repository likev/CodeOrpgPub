/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:40:38 $
 * $Id: saaprods_tab.c,v 1.7 2014/03/18 14:40:38 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaprods_tab.c

Description:   Tabular alphanumeric block generation driver for cpc013/tsk009 
               for the saaprods task. From this module, the ICD compliant TAB
               block is appended to the output buffer.  The tabular blocks are
               for products OSW, OSD, SSW, & SSD.
               
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 8/08/03 			- Zittel
               Move km to nmi range conversion to 
                  saaprods_sdt.c for Build 8 10/17/05		- Zittel
               
*******************************************************************************/


/* local include file */
#include "saaprods_main.h"
#include "saaprods_tab.h"
#include "saa_adapt.h"
#include "saa_arrays.h"
#include "orpgsite.h"

#define PROD_OFFSET 144
#define MIN_IN_HOUR 60

saa_adapt_t saa_adapt;

/*******************************************************************************
Description:      generate_tab is the main generation module for the
                  creation of the tabular alphanumeric block which is appended
                  to products 144, one-hour snow water equivalent (OSW); 145 
                  one-hour snow depth (OSD); 146, storm-total snow water 
                  equivalent (SSW); and 147, storm-total snow depth (SSD).
                  
                  int begin_offset  offset where to start storing output
                  int prod_id       144=SW1HRACC, 145=SD1HRACC
                  int max_bufsize   maximum buffer size for outbuf
                  
Output:           char* output      pointer to the output buffer where the 
                                    final product is being compiled
                  
Returns:          returns the offset of the end of the TAB block on success
                  or SAA_FAILURE upon failure
                  
Globals:          saa_obuf structure

Notes:            none

History:	  11/04/2004	SW CCR NA04-30810	Build8 changes

*******************************************************************************/

int generate_TAB(char *outbuf, int begin_offset,int prod_id,
   int max_bufsize) {
 
   TAB_header th;             /* TAB header structure                         */
   alpha_header ah;           /* alpha block header structure                 */
   alpha_data ad;             /* alpha data structure                         */
   Outbuf_t *saa_obuf;	      /* pointer to output buffer                     */
   int offset=begin_offset;   /* initialize beginning offset                  */
   int TAB_size=0;            /* holds the size of the completed TAB block    */
   int dd,dm,dy;              /* holds calendar month/day/year output         */
   short end_flag=(short)-1;  /* 0xFFFF end of TAB page flag                  */
   char time[12];             /* container to hold calculated time string     */
   char mhb[125];             /* container to hold the TAB msg header block   */
   Graphic_product *gp;       /* pointer to a graphic product structure       */
   int debugit=FALSE;           /* debug boolean                                */
   char *title[4]  =  {" ONE HOUR SNOW WATER EQUIVALENT ACCUMULATION (OSW)  ",
                       "       ONE HOUR SNOW DEPTH ACCUMULATION (OSD)       ",
                       "STORM TOTAL SNOW WATER EQUIVALENT ACCUMULATION (SSW)",
                       "     STORM TOTAL SNOW DEPTH ACCUMULATION (SSD)      "};
   char site_name[5];		/*  RPG Name                               */ 
   char *rh_corr[2] = {"Static  ", "Used RCA"};
   char *prod_type[2] = {"Maximum Snow Depth:...........",
                         "Maximum Snow Water Equivalent:"};
   char *RCA_text[2] = {"No ", "Yes"};
   saa_obuf =(Outbuf_t*)outbuf;   
   if(debugit)
      fprintf(stderr,"SAA: Begin generate_TAB. begin offset=%d\n",begin_offset);
      
   /* Get the rpg site name                                                   */ 
   ORPGSITE_get_string_prop( ORPGSITE_RPG_NAME, site_name, sizeof(site_name));

   /* step 1 - make a copy of the main MHB (message header block which must   */
   /* be duplicated from the main product) in local memory for TAB updating   */
   memcpy(mhb,outbuf,sizeof(Graphic_product));
   gp=(Graphic_product*)mhb;  /* acquire a pointer to the MHB                 */
   if(debugit)
      fprintf(stderr,"graphic pointer set - prod_code=%d vcp_num=%d\n",
         gp->prod_code, gp->vcp_num);

   /* step 2 - set current offset to beginning of the alpha block. copy the   */
   /* alpha header into the output buffer                                     */
   ah.divider=(short)-1;
   ah.num_pages=NUM_TAB_PAGES;

   offset=begin_offset+sizeof(TAB_header)+sizeof(Graphic_product);
   memcpy(outbuf+offset,&ah,sizeof(alpha_header));
   offset+=sizeof(alpha_header);

   if(debugit)
      fprintf(stderr,"passed alpha header - start of data offset=%d\n",offset);
      
   /* quality control - check and be sure that we are not near the allocated  */
   /* end size of the output buffer                                           */
   if(offset>=max_bufsize) {
      RPGC_log_msg( GL_ERROR,
          "SAA TAB Error: offset size exceeds outbuf allocation size\n");
      return(SAA_FAILURE);
      }
   
   /* step 3 - set up pointers to access the radial header & adaptation block */
   
   /* step 4 - create time information for the TAB page title                 */
   calendar_date(saa_inp.current_date,&dd,&dm,&dy);

   sprintf(time,"%s",msecs_to_string(saa_inp.current_time*1000));
   time[6]=0;
   if(debugit)
      fprintf(stderr,"date output: %d %d %d  time %sZ   prod_id=%d\n",
         dm,dd,dy,time,prod_id);

   /* step 5 - format data strings and stuff into the TAB block. Each line   */
   /*          will be padded to 80 characters                               */
   
   ad.num_char=LINE_WIDTH;    /* preset line width (80 characters)           */

   /* PAGE 1 LINE 1 */
  
   if(debugit){
     fprintf(stderr,"TAB: Before Page 1, Line 1\n");
     fprintf(stderr,"prodid = %d\n", saa_obuf->halfword[15]);
     fprintf(stderr,"prodid text = %s \n",title[saa_obuf->halfword[15]-PROD_OFFSET]);
   }
   sprintf(ad.data,
      "          %s                   ",
          title[saa_obuf->halfword[15]-PROD_OFFSET]);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 2 */
  
   sprintf(ad.data,
      "                                                                                ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
     
   /* PAGE 1 LINE 3 */
   sprintf(ad.data,
      "      RPG Name: %4s    Date: %02d/%02d/%04d    Time: %sZ                                ",
      site_name,dm,dd,dy,time);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
     
   /* PAGE 1 LINE 4 */
  
   sprintf(ad.data,
      "                                                                                ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
     
   /* PAGE 1 LINE 5 */
  
   /* Get the Beginning Date                                                             */
   calendar_date(saa_obuf->halfword[47],&dd,&dm,&dy);
   sprintf(ad.data,
      "     Starting Date:................    %02d/%02d/%04d                                ",
      dm,dd,dy);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
     
   /* PAGE 1 LINE 6 */
  
   /* Get the Beginning Time                                                               */
   sprintf(time,"%s",msecs_to_string(saa_obuf->halfword[48]*60000));
   sprintf(ad.data,
      "     Starting Time:................    %sZ                                           ",
        time);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
     
   /* PAGE 1 LINE 7 */

   /* Get the Ending Date                                                            */
   calendar_date(saa_obuf->halfword[49],&dd,&dm,&dy);
   sprintf(time,"%s",msecs_to_string(saa_obuf->halfword[50]*60000));
   sprintf(ad.data,
      "     Ending Date:..................    %02d/%02d/%04d                                ",
      dm,dd,dy);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 8 */

   /* Get the Ending Time                                                            */
   sprintf(time,"%s",msecs_to_string(saa_obuf->halfword[50]*60000));
   sprintf(ad.data,
      "     Ending Time:..................    %sZ                                           ",
       time);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 9 */
   /* Get the maximum value and scale it according to the product type */

  if(prod_id == SSDACCUM){
    sprintf(ad.data,
      "     %s    %4.1f inches                                 ",
        prod_type[0],(float)(saa_obuf->halfword[46])/10);
    }
  if(prod_id == SSWACCUM){
    sprintf(ad.data,
      "     %s    %4.2f inches                                 ",
        prod_type[1],(float)(saa_obuf->halfword[46])/100);
    }   
  if(prod_id == OSDACCUM){
    sprintf(ad.data,
      "     %s    %4.2f inches                                 ",
        prod_type[0],(float)(saa_obuf->halfword[46])/100);
    }   
  if(prod_id == OSWACCUM){
    sprintf(ad.data,
      "     %s    %5.3f inches                                 ",
        prod_type[1],(float)(saa_obuf->halfword[46])/1000);
    }   
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 10 */

   /* Insert the azimuth of the maximum value  */
     /*         1         2         3         4         5         6         7         8          */
     /*12345678901234567890123456789012345678901234567890123456789012345678901234567890   */
   sprintf(ad.data,
      "     Azimuth of Maximum Value:.....    %3d degrees                               ",
                  saa_obuf->halfword[51]);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 11 */

   /* Insert the Range to the maximum value.  Note conversion to nautical miles moved to
      saaprods_sdt.c for Build 8                                                        */
   sprintf(ad.data,
      "     Range to Maximum Value:.......    %3d nautical miles                         ",
               saa_obuf->halfword[52]);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

     
   /* PAGE 1 LINE 12 */

   sprintf(ad.data,
      "     Range/height Correction Applied:  %s                                      ",
          rh_corr[saa_inp.use_RCA_flag]);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 13 */

   sprintf(ad.data,
      "     Missing Time..................  %5d minutes                                ",
          saa_obuf->halfword[26]);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

  
   /* END OF PAGE 1 FLAG */
  
   memcpy(outbuf+offset,&end_flag,sizeof(short));
   offset+=sizeof(short);

   
   /* PAGE 2 LINE 1 */
   sprintf(ad.data,
      "      Snow Accumulation Algorithm Configuration Parameters                      ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 2 */
   sprintf(ad.data,
      "                                                                                ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 3 */

   calendar_date(saa_inp.current_date,&dd,&dm,&dy);
   sprintf(time,"%s",msecs_to_string(saa_inp.current_time*1000));
   sprintf(ad.data,
      "      RPG Name: %4s    Date: %02d/%02d/%04d    Time: %sZ                                   ",
      site_name,dm,dd,dy,time);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 4 */
   sprintf(ad.data,
      "                                                                                ");
    /*         1         2         3         4         5         6         7         8          */
    /*12345678901234567890123456789012345678901234567890123456789012345678901234567890   */
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 5 */
   sprintf(ad.data,
      "     Z-S Multiplicative Coefficient.............................     %5.1f      ",
      saa_inp.cf_ZS_mult);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 6 */
   sprintf(ad.data,
      "     Z-S Power Coefficient......................................      %4.1f      ",
      saa_inp.cf_ZS_power);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 7 */
   sprintf(ad.data,
      "     Snow - Water Ratio.........................................     %5.1f in/in   ",
      saa_inp.s_w_ratio);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 8 */
   sprintf(ad.data,
      "     Minimum Reflectivity/Isolated Bin Threshold................      %4.1f dBZ   ",
      saa_inp.thr_lo_dBZ);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 9 */
   sprintf(ad.data,
      "     Maximum Reflectivity/Outlier Bin Threshold.................      %4.1f dBZ   ",
      saa_inp.thr_hi_dBZ);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 10 */
   sprintf(ad.data,
      "     Base Elevation for Default Range Height Correction.........     %5.1f deg    ",
      saa_inp.rhc_base_elev);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 11 */
   sprintf(ad.data,
      "     Minimum Height Correction Threshold........................      %5.2f km    ",
      saa_inp.thr_mn_hgt_corr);								/* Build8 */
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 12 */
   sprintf(ad.data,
      "     Range Height Correction Coefficient #1.....................      %7.4f       ",
      saa_inp.cf1_rng_hgt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 13 */
   sprintf(ad.data,
      "     Range Height Correction Coefficient #2.....................      %7.4f       ",
      saa_inp.cf2_rng_hgt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 14 */
   sprintf(ad.data,
      "     Range Height Correction Coefficient #3.....................      %7.4f       ",
      saa_inp.cf3_rng_hgt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 15 */
     /*         1         2         3         4         5         6         7         8          */
     /*12345678901234567890123456789012345678901234567890123456789012345678901234567890   */
   sprintf(ad.data,
      "     Time Span Threshold........................................     %4d min    ",
      (int)(saa_inp.thr_time_span*60.+0.5));
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 16 */
   sprintf(ad.data,
      "     Minimum Time Threshold.....................................     %4d min     ",
      (int)(saa_inp.thr_mn_pct_time*60+0.5));
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 17 */
   sprintf(ad.data,
      "     Use RCA Correction Flag (RCA Currently Not Available)......       %s         ",
      RCA_text[saa_inp.use_RCA_flag]);						/* Build8 */
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* END OF PAGE 2 FLAG */
   memcpy(outbuf+offset,&end_flag,sizeof(short));
   offset+=sizeof(short);
   

   /* quality control - check and be sure that we are not near the allocated  */
   /* end size of the output buffer                                           */
   if(offset>=max_bufsize) {
      RPGC_log_msg( GL_ERROR,
              "SAA TAB Error: offset size exceeds outbuf allocation size\n");
      return(SAA_FAILURE);
      }

     
   if(debugit) fprintf(stderr,"completed TAB Construction: offset=%d\n",offset);

   /* calculate the total TAB block size                                      */
   TAB_size=offset-begin_offset;
   if(debugit)
      fprintf(stderr,"total tab size=%d\n",TAB_size);
   
   /* quality control - check and be sure that we are not near the allocated  */
   /* end size of the output buffer                                           */
   if(offset>=max_bufsize) {
      RPGC_log_msg(GL_ERROR,
          "SAA TAB Error: offset size exceeds outbuf allocation size\n");
      return(SAA_FAILURE);
      }
   
   /* step 6 - fill in message header fields                                  */
   
   /* update the total block size of the TAB block in the TAB header          */
   th.divider=(short)-1;
   th.block_id=(short)3;
   RPGC_set_product_int( (void *) &th.block_length, TAB_size );
   memcpy(outbuf+begin_offset,&th,sizeof(TAB_header));
   
   /* update the following fields in the TAB message header block             */
   /* insert the proper product codes for products                            */
   gp->msg_code=(short)prod_id;
   RPGC_set_product_int( (void *) &gp->msg_len, TAB_size-sizeof(TAB_header) ); /* size of tab minus the header   */
   gp->n_blocks=(short)3;                   /* 3 blocks mhb/pdb and alpha     */
   RPGC_set_product_int( (void *) &gp->sym_off, 60 );                          /* symb (alpha) block header      */
   RPGC_set_product_int( (void *) &gp->gra_off, 0 );                           /* no GAB block                   */
   RPGC_set_product_int( (void *) &gp->tab_off, 0 );                           /* no TAB block                   */
   
   /* store updated TAB message header block into the output buffer           */
   memcpy(outbuf+begin_offset+sizeof(TAB_header),&mhb,sizeof(Graphic_product));
   
   if(debugit)
      fprintf(stderr,"TAB Module Complete: offset return=%d\n",offset);
   return(offset);
   }

void calendar_date (
    short	date,				/* days since January 1, 1970		*/
    int		*dd,				/* OUT:  day				*/
    int		*dm,				/* OUT:  month				*/
    int		*dy )				/* OUT:  year				*/
{

    int			l,n, julian;

    /* Convert modified julian to type integer */
    julian = date;

    /* Convert modified julian to year/month/day */
    julian += 2440587;
    l = julian + 68569;
    n = 4*l/146097;
    l = l -  (146097*n + 3)/4;
    *dy = 4000*(l+1)/1461001;
    l = l - 1461*(*dy)/4 + 31;
    *dm = 80*l/2447;
    *dd= l -2447*(*dm)/80;
    l = *dm/11;
    *dm = *dm+ 2 - 12*l;
    *dy = 100*(n - 49) + *dy + l;
/*    *dy = *dy - 1900;*/   

    return;
}


char *msecs_to_string (
    int		time)				/* milliseconds since midnight		*/
{
    int			h, m;
    static char		stime[12];

    time =	time/1000;
    h =		time/3600;
    time =	time - h*3600;
    m =		time/60;

    (void) sprintf(stime, "%02d:%02d", h, m);
    return(stime);
}
