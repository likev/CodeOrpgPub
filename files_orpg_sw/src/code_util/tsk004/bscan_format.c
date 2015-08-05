/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:50 $
 * $Id: bscan_format.c,v 1.5 2009/05/15 17:37:50 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* bscan_format.c */

#include <stdio.h>
#include "bscan_format.h"
/* CVT 4.4 */
#include "basedata.h"

void generate_BSCAN(short num_radials,short range_bins,short code,char *buffer,
   int *offset,int *flag) {
   /* output data in the OH BSCAN format 
   code=1 for digital data and 2 for RLE data that needs to be decoded.

   flag[7] contains scale data: 1=refl 2=vel 0.5m/s 3=vel 1m/s 4=sw */

   int i,n;
   short num=0;
   short count=0;
   short delta_angle;
   short start_index=0; /* index of the smallest azimuth angle */
   float min_az=999.9;  /* used to find the start_index */
   
/* CVT 4.4 */
/*   short data[400][920]; */
/*   short angle[400];     */
   short data[BASEDATA_MAX_SR_RADIALS][MAX_BASEDATA_REF_SIZE];
   short angle[BASEDATA_MAX_SR_RADIALS];

   /*
   short radial_found=FALSE;
   int start=0,end=0;
    */
/* CVT 4.3 */
/*   enum{NOMOD,RLE,BSCAN};  defined in cvt.h */
/*    char *truefalse[]={"FALSE","TRUE"}; */
/*    char *format[]={"NONE","RLE","BSCAN"}; */


   printf("Begin BSCAN Generator\n");

   /*  PART 1 - FILL THE DATA ARRAYS ==================================== */
   for(i=0;i<num_radials;i++) {

/* CVT 4.4 */
/*      if(i>=400) { */
      if(i>=BASEDATA_MAX_SR_RADIALS) {
         printf("BSCAN ERROR: number of radials exceeded ORPG LIMIT %d\n",
                                                  BASEDATA_MAX_SR_RADIALS);
         break;
      }


      /*  input data is RLE data packet ------------------------------- */
      if(code==2) {
         int j,k;
         int count=0;
   
         /* read the radial header AF1F*/    
         num=read_half(buffer,offset); /*number of rle halfwords (2 bytes) */
         angle[i]=read_half(buffer,offset);
         /* when exiting the i loop, start_index set to location of the */
         /* smallest azimuth angle                                      */
         if(angle[i]/10.0<min_az) {
             min_az=angle[i]/10.0;
             start_index=i;
         }
         
         delta_angle=read_half(buffer,offset);
/* DEBUG */
/* printf("radial %d #rle halfwords %04hd start angle %04hd delta angle %04hd\n",*/
/*                                                   i,num,angle[i],delta_angle);*/
   
         /* decode run length encoded data and stuff into data array */
/* DEBUG */
/*printf("DEBUG BSCAN Angle: %05.1f  Delta: %4.1f\n", */
/*          angle[i]/10.0,delta_angle/10.0);          */

         for(j=0;j<(2*num);j++) {
            char c,run,val;
            c=read_byte(buffer,offset);
              
            run=c>>4;
            val=c & 0x0f;

            for(k=0;k<(int)run;k++) {
                
               data[i][count++] = (short)val;
/* CVT 4.4 */
/*               if(count>=920) {                                               */
/*                   printf("RLE Decode: decode counter exceeded 920 bins per " */
/*                          "radial on radial %hd rle word %hd\n",i,j);         */
               if(count>=range_bins) {
                   printf("DATA ERROR Packet AF1F, number of unpacked "
                          "bins exceeds number of data elements %d\n"
                          "           HW per radial is is %d, reading RLE HW %d\n",
                          range_bins,num/2,(j+1)/2);
                   return;
               }
        
            } /* end k loop */
            
         } /* end j loop */

      } else {   /*  code == 1, input data is digital data ---------------- */

         int j;
          /* read the radial header 16 */    
         num=read_half(buffer,offset); /* number of bytes in this radial */
         angle[i]=read_half(buffer,offset);
         
         /* CVT 4.4 - previously was only in RLE processing */
         /* when exiting the i loop, start_index set to location of the */
         /* smallest azimuth angle                                      */
         if(angle[i]/10.0<min_az) {
             min_az=angle[i]/10.0;
             start_index=i;
         }
         
         delta_angle=read_half(buffer,offset);
/* DEBUG */
/* printf("DEBUG BSCAN radial %d #bytes %04hd start angle %04hd delta  %04hd\n",*/
/*                                               i,num,angle[i],delta_angle);   */
    
         for(j=0;j<num;j++) {
            unsigned char val;
/* DEBUG */
/* printf("DEBUG BSCAN Angle: %05.1f  Delta: %4.1f\n", */
/*          angle[i]/10.0,delta_angle/10.0);           */
          
            val=read_byte(buffer,offset);

            data[i][j] = (short)val;
          
         } /* end j loop */
    
      } /* end code 1 else */
    
    
   }  /* end of i loop (for each radial) fill data array */
    
    

   /*  PART 2 - PRINT THE DATA ========================================= */
   {
   short value,pass,j;
    /* determine number of blocks to print out */
   value=(int)range_bins/50;
   printf("number of passes to print %i\n",value);
   pass=0;
 
   for(j=0;j<value;j++) {
 
      /* print first 50 range bins */
      printf("\n\n  AZM  ");
      for(i=pass+1;i<pass+50+1;i++) 
        printf(" %4i",i);
      
      printf("\n");
    
      printf("-------");
      for(i=0;i<50;i++) 
        printf("-----");
        
      printf("\n");
      
      /* output first batch */
      count=start_index;
      /* CVG 4.4 - simplify logic */
      /* for(i=start_index;i<start_index+num_radials;i++) { */
      for(i=0;i<num_radials;i++) {
    
         printf("%5.1f  ",angle[count]/10.0);
         for(n=pass;n<pass+50;n++) {
            printf(" %4i",data[count][n]);
         } /* end n loop */
    
         printf("\n");
         count++;

      /* CVG 4.4 - BUG FIX if there were not exactly 360 radials in the */
      /*             original non-super resolution products, then a few   */
      /*             radials could be repeated and a few missed.          */
      /*             Also this mod works for SR products                  */
         /* if(count>359) count=0; */
         if( count > (num_radials-1) ) count = 0;
         
      } /* end i loop */
    
      pass+=50;
      
   } /* end j loop */
    
 
   /* print final section */
   value=range_bins%50;
   printf("\nfinal length=%i\n",value);
    
   /* print first 50 range bins */
   printf("\n\n  AZM  ");
   for(i=pass+1;i<pass+value+1;i++) 
      printf(" %4i",i);
     
   printf("\n");
   
   printf("-------");
   for(i=pass;i<pass+value;i++) 
       printf("-----");
       
   printf("\n");
 
   /* output first batch */
   count=start_index;
   /* CVG 4.4 - simplify logic */
   /* for(i=start_index;i<start_index+num_radials;i++) { */
   for(i=0;i<num_radials;i++) {
 
      printf("%5.1f  ",angle[count]/10.0);
      for(n=pass;n<pass+value;n++) {
         printf(" %4i",data[count][n]);
      } /* end n loop */
 
      printf("\n");
      count++;       
      
      /* CVG 4.4 - BUG FIX if there were not exactly 360 radials in the */
      /*             original non-super resolution products, then a few   */
      /*             radials could be repeated and a few missed.          */
      /*             Also this mod works for SR products                  */
      /* if(count>359) count=0; */
      if( count > (num_radials-1) ) count = 0;
      
   } /* end i loop */
 
   printf("\n");
     
   } /* end block */
   /*  END PART 2 ================ */

} /* end generate_bscan */






