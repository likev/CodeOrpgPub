/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/01/12 16:24:51 $
 * $Id: recclprods_tab.c,v 1.4 2004/01/12 16:24:51 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/*******************************************************************************
Module:        recclprods_tab.c

Description:   tabular alphanumeric block generation driver for cpc004/tsk007 
               for the REC (Radar Echo Classifier) task. From this module, the 
               ICD compliant TAB block is appended to the output buffer.
               
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_tab.c,v 1.4 2004/01/12 16:24:51 steves Exp $
*******************************************************************************/


/* local include file */
#include "recclprods_tab.h"


/*******************************************************************************
Description:      generate_tab is the main generation module for the
                  creation of the tabular alphanumeric block which is appended
                  to both product 132, the clutter likelihood reflectivity
                  product and product 133, the clutter likelihood doppler
                  product.
                  
Input:            char* inbuf       pointer to the input buffer containing the
                                    intermediate data buffer
                  int begin_offset  offset where to start storing output
                  int prod_id       132=REC REFL, 133=REC DOP
                  int max_bufsize   maximum buffer size for outbuf
                  
Output:           char* output      pointer to the output buffer where the 
                                    final product is being compiled
                  
Returns:          returns the offset of the end of the TAB block on success
                  or REC_FAILURE upon failure
                  
Globals:          none
Notes:            The content of the TAB will be the same for both products
                  132 and 133 except for the TAB product codes which is 
                  defined in a309.h
*******************************************************************************/

int generate_TAB(char *inbuf, char *outbuf, int begin_offset,int prod_id,
   int max_bufsize) {
   
   TAB_header th;             /* TAB header structure                         */
   alpha_header ah;           /* alpha block header structure                 */
   alpha_data ad;             /* alpha data structure                         */
   Rec_prod_header_t *rhdr;   /* pointer to an intermediate radial header     */
   rec_cl_target_t *adapt;    /* pointer to adaptation data block             */
   int offset=begin_offset;   /* initialize beginning offset                  */
   int TAB_size=0;            /* holds the size of the completed TAB block    */
   int dd,dm,dy;              /* holds calendar month/day/year output         */
   short end_flag=(short)-1;  /* 0xFFFF end of TAB page flag                  */
   char time[12];             /* container to hold calculated time string     */
   char mhb[125];             /* container to hold the TAB msg header block   */
   Graphic_product *gp;       /* pointer to a graphic product structure       */
   int DEBUG=FALSE;           /* debug boolean                                */
   
   if(DEBUG)
      fprintf(stderr,"REC: Begin generate_TAB. begin offset=%d\n",begin_offset);

   /* step 1 - make a copy of the main MHB (message header block which must   */
   /* be duplicated from the main product) in local memory for TAB updating   */
   memcpy(mhb,outbuf,sizeof(Graphic_product));
   gp=(Graphic_product*)mhb;  /* acquire a pointer to the MHB                 */
   if(DEBUG)
      fprintf(stderr,"graphic pointer set - prod_code=%hd vcp_num=%hd\n",
         gp->prod_code, gp->vcp_num);

   /* step 2 - set current offset to beginning of the alpha block. copy the   */
   /* alpha header into the output buffer                                     */
   ah.divider=(short)-1;
   ah.num_pages=NUM_TAB_PAGES;
   offset=begin_offset+sizeof(TAB_header)+sizeof(Graphic_product);
   memcpy(outbuf+offset,&ah,sizeof(alpha_header));
   offset+=sizeof(alpha_header);
   if(DEBUG)
      fprintf(stderr,"passed alpha header - start of data offset=%d\n",offset);
      
   /* quality control - check and be sure that we are not near the allocated  */
   /* end size of the output buffer                                           */
   if(offset>=max_bufsize) {
      RPGC_log_msg( GL_ERROR,
          "REC TAB Error: offset size exceeds outbuf allocation size\n");
      return(REC_FAILURE);
      }
   
   /* step 3 - set up pointers to access the radial header & adaptation block */
   rhdr=(Rec_prod_header_t *)inbuf;   /* pointer to REC product header        */
   adapt=(rec_cl_target_t *)(inbuf+rhdr->adapt_offset); /* pointer to adapt block */
   
   /* step 4 - create time information for the TAB page title                 */
   calendar_date(rhdr->date,&dd,&dm,&dy);
   sprintf(time,"%s",msecs_to_string(rhdr->time));
   time[6]=0;
   if(DEBUG)
      fprintf(stderr,"date output: %d %d %d  time %s    prod_id=%d\n",
         dm,dd,dy,time,prod_id);

   /* step 5 - format data strings and stuff into the TAB block. Each line   */
   /*          will be padded to 80 characters                               */
   
   ad.num_char=LINE_WIDTH;    /* preset line width (80 characters)           */
   
   /* PAGE 1 LINE 1 */
   sprintf(ad.data,
      "                 Radar Echo Classifier Adaptation Data Parameters               ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 2 */
   sprintf(ad.data,
      "      Date: %02d/%02d/%04d    Time: %s    Vol: %02hd    Elev: %4.1f    Page: 1       ",
      dm,dd,dy,time,rhdr->volume_scan_num, rhdr->target_elev/10.0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 3 */
   sprintf(ad.data,
      "                                                                                ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 4 */
   sprintf(ad.data,
      "AP/Clutter Target Scaling Function Thresholds:                                  ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 5 */
   sprintf(ad.data,
      "Texture of Reflectivity generating a 0%% likelihood                <= %4.1f dBZ**2",
      adapt->Ztxtr0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 6 */
   sprintf(ad.data,
      "Texture of Reflectivity generating a 100%% likelihood              >= %4.1f dBZ**2",
      adapt->Ztxtr1);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 7 */
   sprintf(ad.data,
      "Abs. value of Sign of Refl. Change generating a 0%% likelihood     >= %4.1f       ",
      adapt->Zsign0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 8 */
   sprintf(ad.data,
      "Abs. value of Sign of Refl. Change generating a 100%% likelihood   <= %4.1f       ",
      adapt->Zsign1);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 9 */
   sprintf(ad.data,
      "Abs. value of (Refl. Spin Change-50) generating a 0%% likelihood    = %4.1f       ",
      adapt->Zspin0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 10*/
   sprintf(ad.data,
      "Abs. value of (Refl. Spin Change-50) generating a 100%% likelihood  = %4.1f       ",
      adapt->Zspin1);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 11*/
   sprintf(ad.data,
      "Abs. value of Mean Velocity generating a 0%% likelihood            >= %4.1f m/s   ",
      adapt->Vmean0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 12*/
   sprintf(ad.data,
      "Abs. value of Mean Velocity generating a 100%% likelihood          <= %4.1f m/s   ",
      adapt->Vmean1);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 13*/
   sprintf(ad.data,
      "Standard Deviation of Velocity generating a 0%% likelihood         >=  %03.1f m/s   ",
      adapt->Vstdv0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 1 LINE 14*/
   sprintf(ad.data,
      "Standard Deviation of Velocity generating a 100%% likelihood       <=  %03.1f m/s   ",
      adapt->Vstdv1);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 1 LINE 15*/
   sprintf(ad.data,
      "Mean Spectrum Width generating a 0%% likelihood                    >= %4.1f m/s   ",
      adapt->Wmean0);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 1 LINE 16*/
   sprintf(ad.data,
      "Mean Spectrum Width generating a 100%% likelihood                  <= %4.1f m/s   ",
      adapt->Wmean1);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* END OF PAGE 1 FLAG */
   memcpy(outbuf+offset,&end_flag,sizeof(short));
   offset+=sizeof(short);
   

   /* quality control - check and be sure that we are not near the allocated  */
   /* end size of the output buffer                                           */
   if(offset>=max_bufsize) {
      RPGC_log_msg( GL_ERROR,
              "REC TAB Error: offset size exceeds outbuf allocation size\n");
      return(REC_FAILURE);
      }

   /* PAGE 2 LINE 1 */
   sprintf(ad.data,
      "          Radar Echo Classifier Adaptation Data Parameters        Page: 2       ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 2 */
   sprintf(ad.data,
      "AP/Clutter Target Spin Characteristic Thresholds:                               ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 3 */
   sprintf(ad.data,
      "Spin Change Threshold                                                %4.1f dBZ   ",
      adapt->ZspinThr);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 4 */
   sprintf(ad.data,
      "Spin Reflectivity Threshold                                          %4.1f dBZ   ",
      adapt->ZThr);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 5 */
   sprintf(ad.data,
      "                                                                                ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 6 */
   sprintf(ad.data,
      "AP/Clutter Target Category Weighting:                                           ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 7 */
   sprintf(ad.data,
      "Texture of Reflectivity weight                                        %4.2f      ",
      adapt->ZtxtrWt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
      
   /* PAGE 2 LINE 8 */
   sprintf(ad.data,
      "Sign of Reflectivity Change weight                                    %4.2f      ",
      adapt->ZsignWt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 9 */
   sprintf(ad.data,
      "Reflectivity Spin Change weight                                       %4.2f      ",
      adapt->ZspinWt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 10 */
   sprintf(ad.data,
      "Mean Velocity weight                                                  %4.2f      ",
      adapt->VmeanWt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 11 */
   sprintf(ad.data,
      "Standard Deviation of Velocity weight                                 %4.2f      ",
      adapt->VstdvWt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 12 */
   sprintf(ad.data,
      "Mean Spectrum Width weight                                            %4.2f      ",
      adapt->WmeanWt);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 13 */
   sprintf(ad.data,
      "                                                                                ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 14 */
   sprintf(ad.data,
      "Extents for Radial Processing:                                                  ");
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* PAGE 2 LINE 15 */
   sprintf(ad.data,
      "Azimuthal Extent                                                      %1d radials ",
      adapt->deltaAz);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 16 */
   sprintf(ad.data,
      "Reflectivity Range Extent                                             %1d bins    ",
      adapt->deltaRng);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);

   /* PAGE 2 LINE 17 */
   sprintf(ad.data,
      "Doppler Range Extent                                                  %1d bins    ",
      adapt->deltaBin);
   memcpy(outbuf+offset,&ad,sizeof(alpha_data));
   offset+=sizeof(alpha_data);
   
   /* END OF PAGE 2 FLAG */
   memcpy(outbuf+offset,&end_flag,sizeof(short));
   offset+=sizeof(short);
   
   if(DEBUG) fprintf(stderr,"completed TAB Construction: offset=%d\n",offset);

   /* calculate the total TAB block size                                      */
   TAB_size=offset-begin_offset;
   if(DEBUG)
      fprintf(stderr,"total tab size=%d\n",TAB_size);
   
   /* quality control - check and be sure that we are not near the allocated  */
   /* end size of the output buffer                                           */
   if(offset>=max_bufsize) {
      RPGC_log_msg(GL_ERROR,
          "REC TAB Error: offset size exceeds outbuf allocation size\n");
      return(REC_FAILURE);
      }
   
   /* step 6 - fill in message header fields                                  */
   
   /* update the total block size of the TAB block in the TAB header          */
   th.divider=(short)-1;
   th.block_id=(short)3;
   RPGC_set_product_int( (void *) &th.block_length, (unsigned int) TAB_size );
   memcpy(outbuf+begin_offset,&th,sizeof(TAB_header));
   
   /* update the following fields in the TAB message header block             */
   /* insert the proper product codes for products 132 and 133                */
   if(prod_id==RECCLREF)
      gp->msg_code=(short)RECCLDIGREFTAB;
   else
      gp->msg_code=(short)RECCLDIGDOPTAB;
   /* size of tab minus the header                                            */
   RPGC_set_product_int( (void *) &gp->msg_len, (unsigned int) (TAB_size-sizeof(TAB_header)));
   gp->n_blocks=(short)3;                   /* 3 blocks mhb/pdb and alpha     */
   /* symb (alpha) block header                                               */
   RPGC_set_product_int( (void *) &gp->sym_off, 60 );
   /* no GAB block                                                            */
   RPGC_set_product_int( (void *) &gp->gra_off, 0 );                           
   /* no TAB block                                                            */
   RPGC_set_product_int( (void *) &gp->tab_off, 0 );                           
   
   /* store updated TAB message header block into the output buffer           */
   memcpy(outbuf+begin_offset+sizeof(TAB_header),&mhb,sizeof(Graphic_product));
   
   if(DEBUG)
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
    int			h, m, s, frac;
    static char		stime[12];

    frac =	time - 1000*(time/1000);
    time =	time/1000;
    h =		time/3600;
    time =	time - h*3600;
    m =		time/60;
    s =		time - m*60;

    (void) sprintf(stime, "%02d:%02d", h, m);
    return(stime);
}


#if 0
Original 4 steps
   fprintf(stderr,"Begin generate_TAB\n");
   
   /* step 1 - set up the TAB block header                                   */
   /*
   th.divider=(short)-1;
   th.block_id=(short)3;
   th.block_length=0;
   memcpy(outbuf+offset,&th,sizeof(TAB_header));*/
   offset+=sizeof(TAB_header);
   
   /* step 2 - copy the msg hdr block/prod desc block from beginning of      */
   /*          outbuf to the current offset position. specific TAB related   */
   /*          fields will be updated in step 6 upon product completion      */
   memcpy(outbuf+offset,outbuf,sizeof(Graphic_product));
   offset+=sizeof(Graphic_product);
   
   /* step 3 - set up the TAB alpha block header                             */
   ah.divider=(short)-1;
   ah.num_pages=NUM_TAB_PAGES;
   memcpy(outbuf+offset,&ah,sizeof(alpha_header));
   offset+=sizeof(alpha_header);

   /* step 4 - set up pointers to access the adaptation block                */

   rhdr=(Rec_prod_header_t *)inbuf;  /* pointer to REC product header        */
   fprintf(stderr,"adapt offset=%d\n",rhdr->adapt_offset);
   adapt=(rec_cl_target_t *)(inbuf+rhdr->adapt_offset); /* pointer to adapt block */
   fprintf(stderr,"adapt value Ztxtr1=%f\n",adapt->Ztxtr1);
   /*
      sprintf(fname,"%svol%d.elev%d","/noaa/home/cdr1_16/src/cpc004/tsk007/tabtest",
      rhdr->volume_scan_num,rhdr->elev_num);
   
   fp=fopen(fname,"w");
   if(fp==NULL) fprintf(stderr,"error opening tabtest output file\n");
   */
   
   calendar_date(rhdr->date,&dd,&dm,&dy);
   fprintf(stderr,"date output: %d %d %d      prod_id=%d\n",dm,dd,dy,prod_id);
   sprintf(time,"%s",msecs_to_string(rhdr->time));
   time[9]=0;
   fprintf(stderr,"time = %s\n",time);
   
#endif


