/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 14:48:41 $
 * $Id: packet_AF1F.c,v 1.8 2009/08/19 14:48:41 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* packet_AF1F.c */

#include "packet_AF1F.h"
#include "bscan_format.h"


void packet_AF1F(char *buffer,int *offset,int *flag) {
  /* display Packet AF1F Rdial Data Packet (16 level) information 
     flag[7] contains scale data: 1=refl 2=vel 0.5m/s 3=vel 1m/s 4=sw */
   short num_radials=0;
   int i; /* j; */
   short num_rle_halfwords=0;
   short range_bins;
   short start_angle,delta_angle;
   short istart_angle;
   short radial_found=FALSE;
   int start=0,end=0;
   short code=2; /* used for sending rle decoding instructions to the BSCAN module */
/* CVT 4.3 internal diagnostics */
/*   enum{NOMOD,RLE,BSCAN};  defined in cvt.h */
/*    char *truefalse[]={"FALSE","TRUE"}; */
/*    char *format[]={"NONE","RLE","BSCAN"}; */
/*    char *scale[]={"NOSCALE","REFL","VEL1","VEL2","SW"}; */

   fprintf(stderr,"\n---------------- Decoding Packet AF1Fx -----------------\n");
   
   fprintf(stderr,"Index of First Range Bin:\t\t\t%hd\n",read_half(buffer,offset));
   range_bins=read_half(buffer,offset);
   fprintf(stderr,"Number of Range Bins:\t\t\t\t%hd\n",range_bins);
   fprintf(stderr,"I center of sweep:\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"J center of sweep:\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"Scale Factor:\t\t\t\t\t%hd\n",read_half(buffer,offset));
   num_radials=read_half(buffer,offset);
   fprintf(stderr,"Number of Radials\t\t\t\t%hd\n",num_radials);
   fprintf(stderr,"\n");

   /* test for format flag etc. */
   if((flag[0]==RADIAL) && (flag[5]==RLE)) {
      fprintf(stderr,"Radials Left in Run-Length Encoded Format\n");
   } else if(flag[0]==RADIAL) {
      fprintf(stderr,"Radials Unpacked to show all Data Values\n");
   }
   if(flag[5]==BSCAN) {
      fprintf(stderr,"BSCAN Format Output Selected\n");
   }

/*  CVT 4.3  internal diagnostics */
/*    fprintf(stderr,"Diagnostic Information:\n"); */
/*    start=flag[1]; */
/*    fprintf(stderr,"Start Field:\t\t\t\t\t%hd\n",start); */
/*    end=flag[2]; */
/*    fprintf(stderr,"End Field:\t\t\t\t\t%hd\n",end); */
/*    fprintf(stderr,"All Flag:\t\t\t\t\t%s\n",truefalse[flag[3]]); */
/*    fprintf(stderr,"Degree Flag:\t\t\t\t\t%s\n",truefalse[flag[4]]); */
/*    fprintf(stderr,"Output Format:\t\t\t\t\t%s\n",format[flag[5]]); */
/*    fprintf(stderr,"Scale Flag:\t\t\t\t\t%s\n",scale[flag[7]]); */
/*    fprintf(stderr,"\n"); */


   start=flag[1]; 
   end=flag[2]; 
   
   /* CVT 4.4 */
   if(flag[1]==0 && flag[2]==0) /* assume all radials desired */
      flag[3] = TRUE;
   
   
   /* check for 'all' flag & reset start and end flags */
   if(flag[3]==TRUE) {
      if(flag[4]==TRUE) { /* use degree values */
         start=0;
         end=359;

      } else { /* use all values */
         start=1;
         end=num_radials;
      }

   }

   /* quality control input values */
   if(start>num_radials) {
      fprintf(stderr,"Range Error: The Radial Start Value of %d Exceeds the Max Number of Radials (%d)\n",
                                         start,num_radials);
      return;
   }
   if(end<0) {
      fprintf(stderr,"Range Error: The Radial End Value of %d is out of bounds\n",end);
      return;
   }
   if(flag[4]==TRUE && (start>359 || start<0)) {
      fprintf(stderr,"Range Error: The Radial Start Value of %d must be within 0-359 degrees\n",
                                        start);
      return;
   }
   if(flag[4]==TRUE && (end >359 || start<0)) {
      fprintf(stderr,"Range Error: The Radial End Value of %d must be within 0-359 degrees\n",
                                          end);
      return;
   }

   if(flag[5]==BSCAN) {
      /* output data in BSCAN format (bypass all further radial processing */
      generate_BSCAN(num_radials,range_bins,code,buffer,offset,flag);
      return;
   }

   /* process for each radial */
   for(i=1;i<=num_radials;i++) {
      
      /* read the radial header */    
      num_rle_halfwords=read_half(buffer,offset); /* a halfword is 2 bytes */
      start_angle=read_half(buffer,offset);
      istart_angle=(short)(start_angle/10.0);
      delta_angle=read_half(buffer,offset);
      /*fprintf(stderr,"radial %d #rle halfwords %04hd start angle %04hd delta angle %04hd\n",
         i,num_rle_halfwords,start_angle,delta_angle);*/
 
      if(flag[4]==FALSE) { /* process actual radial values */
         
          if(end==0) {  /* only one radial is requested */
              if(start!=i) {
                  advance_offset(buffer,offset,2*num_rle_halfwords); 
                  continue;
              }
              
          } /*  end if end==0 */
 
          if(end!=0) { /* more than one radial is requested */
          /* fprintf(stderr,"radial %d start=%d end=%d\n",i,start,end);*/
   
              if(start>end) {
              /* range from 358 to 2 degrees has start val > end */
                  if(i<start && i>end) {
                      advance_offset(buffer,offset,2*num_rle_halfwords); 
                      continue;
                  }
        
              } else {
              /* normal processing with start val < end val */
                  if(i<start || i>end) {
                      advance_offset(buffer,offset,2*num_rle_halfwords); 
                      continue;
                  }
                  
              } /*  end else */
 
          } /*  end if end!=0 */
 
      } /* end if flag[4] FALSE*/
 
 
      if(flag[4]==TRUE) { /* process degree values */
         
          if(end==0) {  /* only one radial is requested */
              if(start!=istart_angle) {
                  advance_offset(buffer,offset,2*num_rle_halfwords); 
                continue;
              }
          }
          
          if(end!=0) { /* more than one radial is requested */
          /* fprintf(stderr,"radial %d start=%d istart_angle=%d end=%d\n",
                                         i,start,istart_angle,end);*/
 
              if(start>end) {
              /* range from 358 to 2 degrees has start val > end */
                  if(istart_angle<start && istart_angle>end) {
                      advance_offset(buffer,offset,2*num_rle_halfwords); 
                      continue;
                  }
             } else {
                 /* normal processing with start val < end val */
                 if(istart_angle<start || istart_angle>end) {
                     advance_offset(buffer,offset,2*num_rle_halfwords); 
                     continue;
                 }
                 
             } /*  end else */
 
         } /*  end if end!=0 */
 
      } /* end if flag[4] TRUE */
 
      radial_found=TRUE;
       
      if(flag[5]==RLE) {
         /* output radial data in hexidecimal RLE encoded format */
         print_rle("AF1F RLE Output",i,start_angle,delta_angle,buffer,offset,
                                     2*num_rle_halfwords);
 
      } else {
         /* output radial data in decimal non-RLE encoded format */
         decode_rle("AF1F Decimal Output",i,start_angle,delta_angle,buffer,offset,
                                    2*num_rle_halfwords, range_bins, flag[7]);
      }
      
   } /* end of i loop */

   if(radial_found==FALSE) {
      fprintf(stderr,"WARNING: No data was found for radial range start: %hd end: %hd\n",
                                                    start,end);
   }


}



/* CVT 4.4 added the rad parameter */
void print_rle(char *msg,int rad,short start_angle,short delta,char *buffer,
               int *offset,short num_bytes) {
   /* print one radial in run length encoded format */
   
   int i; /* j; */
   
   int k;
   
   unsigned char c; /* run,val; */

/*   FUTURE ENHANCEMENT: Include looking for beginning of next radial and/or */
/*                       end of data packet for consistency check */

   
   k=0;
   
   fprintf(stderr,"%s - Radial %d  Angle: %05.1f  Delta: %4.1f "
                  " Number of RLE Halfwords: %i\n",
                           msg,rad,start_angle/10.0,delta/10.0,num_bytes/2);
   for(i=0;i<num_bytes;i++) {
      
      c=read_byte(buffer,offset);
      fprintf(stderr,"%x ",c);
      
      if(k==19) {
          fprintf(stderr,"\n");
          k=0;
      } else {
           k++;
      }
      
   }
   
   fprintf(stderr,"\n"); 

}



/* CVT 4.4 added the rad parameter */
void decode_rle(char *msg,int rad, short start_angle,short delta,char *buffer,
                int *offset,short num_bytes,short num_bins, int flag) {
   /* decode one radial of run length encoded data and display in decimal */
   
   int i,j;
   
   int k;  /*  counter to limit output to 20 values per line */
   
   unsigned char c,run,val;
   short bin_count;
   short error;


/*   FUTURE ENHANCEMENT: Include looking for beginning of next radial and/or */
/*                       end of data packet for consistency check */
   
   k=0;
   bin_count=0;
   error=0;
   
   fprintf(stderr,"%s - Radial %d  Angle: %05.1f  Delta: %4.1f\n",
                      msg,rad,start_angle/10.0,delta/10.0);
   for(i=0; i<num_bytes; i++) {
      
       c=read_byte(buffer,offset);
       /*fprintf(stderr,"%x\n",c);*/
       
       run=c>>4;
       val=c & 0x0f;


       
       for(j=0; j<(int)run; j++) {
        
           bin_count++;
           
           /*  CVT 4.1  detect unpacked bins exceeding num_bins */
           if(bin_count>num_bins && error==0) {
               fprintf(stderr, "DATA ERROR Packet AF1F, number of unpacked "
                                "bins exceeds number of data elements %d\n"
                                "           HW per radial is %d, reading RLE HW %d\n",
                                num_bins, num_bytes/2, (i+1)/2);
               error=1;
           }
 
           fprintf(stderr,"%02u ",val);
               
           if(k==19) {
               fprintf(stderr,"\n");
               k=0;
           } else {
               k++;
           }
          
       } /*  end for j<run */

   } /*  end for i<num_bytes */
   
   fprintf(stderr,"\n"); 
   
}

