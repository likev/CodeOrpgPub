/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:52 $
 * $Id: cvt_packet_16.c,v 1.7 2009/05/15 17:37:52 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/* cvt_packet 16 */


#include "cvt_packet_16.h"

/* CVT 4.4 */
int params_found;



void packet_16(char *buffer,int *offset,int *flag) {
  /* display Packet 16 Digital Radial Data Array Packet 
     flag[7] contains scale data: 1=refl 2=vel 0.5m/s 3=vel 1m/s 4=sw */
   

   
   short num_radials=0;
   int i; 
   short code=1;
   short num_bytes=0;
   short start_angle,delta_angle;
   short istart_angle;
   short range_bins;
   short radial_found=FALSE;
   int start=0,end=0;
/* CVT 4.3 internal diagnostics */
/*   enum{NOMOD,RLE,BSCAN};  defined in cvt.h */
/*    char *truefalse[]={"FALSE","TRUE"}; */
/*    char *format[]={"NONE","RLE","BSCAN"}; */
/*    char *scale[]={"NOSCALE","REFL","VEL1","VEL2","SW","FDECODE","PDECODE"}; */
      

   printf("\n---------------- Decoding Packet 16 -----------------\n");
   
   printf("Index of First Range Bin:\t\t%hd\n",read_half(buffer,offset));
   range_bins=read_half(buffer,offset);
   printf("Number of Range Bins:\t\t\t%hd\n",range_bins);
   printf("I center of sweep:\t\t\t%hd\n",read_half(buffer,offset));
   printf("J center of sweep:\t\t\t%hd\n",read_half(buffer,offset));
   printf("Scale Factor:\t\t\t\t%hd\n",read_half(buffer,offset));
   num_radials=read_half(buffer,offset);
   printf("Number of Radials\t\t\t%hd\n",num_radials);
   printf("\n");

   /*add test for format flag etc. */
   if(flag[7]==NOSCALE) {
      fprintf(stderr,"  Data Values left in Raw Value\n");
   } else if(flag[7]==REFL) {
      fprintf(stderr,"  Data Values Decoded as Reflectivity\n");
   }
   else if(flag[7]==VEL1) {
      fprintf(stderr,"  Data Values Decoded as Velocity (mode 1)\n");
   }
   else if(flag[7]==VEL2) {
      fprintf(stderr,"  Data Values Decoded as Velocity (mode 2)\n");
   }
   else if(flag[7]==SW) {
      fprintf(stderr,"  Data Values Decoded as Spectrum Width\n");
   }
/* CVT 4.4 */
   else if(flag[7]==FDECODE) {
      fprintf(stderr,"  Data Values Decoded using File Parameters (fdecode)\n");
   }
   else if(flag[7]==PDECODE) {
      fprintf(stderr,"  Data Values Decoded using Product Parameters (pdecode)\n");
   }
   
   if(flag[5]==BSCAN) {
      fprintf(stderr,"BSCAN Format Output Selected\n");
   }


/*  CVT 4.3 intneral disgnostics */
/*    printf("Diagnostic Information:\n"); */
/*    start=flag[1]; */
/*    printf("Start Field:\t\t\t\t%hd\n",start); */
/*    end=flag[2]; */
/*    printf("End Field:\t\t\t\t%hd\n",end); */
/*    printf("All Flag:\t\t\t\t%s\n",truefalse[flag[3]]); */
/*    printf("Degree Flag:\t\t\t\t%s\n",truefalse[flag[4]]); */
/*    printf("Output Format:\t\t\t\t%s\n",format[flag[5]]); */
/*    printf("Scale Flag:\t\t\t\t%s\n",scale[flag[7]]); */


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
      printf("Range Error: The Radial Start Value of %d Exceeds "
            "the Max Number of Radials (%d)\n",
                 start,num_radials);
     return;
   }

   if(end<0) {
      printf("Range Error: The Radial End Value of %d is out of bounds\n",end);
      return;
   }

   if(flag[4]==TRUE && (start>359 || start<0)) {
      printf("Range Error: The Radial Start Value of %d must be within 0-359 degrees\n",
                 start);
      return;
   }

   if(flag[4]==TRUE && (end >359 || start<0)) {
      fprintf(stderr,
          "Range Error: The Radial End Value of %d must be within 0-359 degrees\n",
                      end);
      return;
   }
   
   if(flag[5]==BSCAN) {
      /* output data in BSCAN format */
      generate_BSCAN(num_radials,range_bins,code,buffer,offset,flag);
      return;
   }   
   
   if(flag[5]==RLE)
      printf("\n*** WARNING: Packet 16 does NOT support RLE output ***\n");

   printf("\n");

   /* CVT 4.4 */
   if(flag[7]==PDECODE || flag[7]==FDECODE) {
      params_found = get_decode_params(flag[7], md.ptr_to_product, &s_o_params);
      if(params_found == FALSE)
        fprintf(stderr,
                "ERROR in obtaining decode parameters, raw levels displayed\n\n");
   }
   
/*   FUTURE ENHANCEMENT: Include looking for beginning of next radial and/or */
/*                       end of data packet for consistency check */

   for(i=1;i<=num_radials;i++) {
      
      /* read the radial header */    
      num_bytes=read_half(buffer,offset); 
      start_angle=read_half(buffer,offset);
      istart_angle=(short)(start_angle/10.0);
      delta_angle=read_half(buffer,offset);
      /*printf("radial %d #bytes %04hd start angle %04hd delta angle %04hd\n",
                                       i,num_bytes,start_angle,delta_angle);*/

      if(flag[4]==FALSE) { /* process actual radial values */
        
         if(end==0) {  /* only one radial is requested */
            if(start!=i) {
               advance_offset(buffer,offset,num_bytes); 
               continue;
            }
         }

         if(end!=0) { /* more than one radial is requested */
            /* printf("radial %d start=%d end=%d\n",i,start,end);*/

            if(start>end) { /* range from 358 to 2 degrees has start val > end */
                
               if(i<start && i>end) {
                  advance_offset(buffer,offset,num_bytes); 
                  continue;
               }

            } else { /* normal processing with start val < end val */
                  
               if(i<start || i>end) {
                  advance_offset(buffer,offset,num_bytes); 
                  continue;
               }

            } /*  end else normal */

         } /*  end if end!=0 */

      } /* end if flag[4] FALSE*/


      if(flag[4]==TRUE) { /* process degree values */
        
         if(end==0) {  /* only one radial is requested */
            if(start!=istart_angle) {
               advance_offset(buffer,offset,num_bytes); 
               continue;
            }
         }

         if(end!=0) { /* more than one radial is requested */
            /* printf("radial %d start=%d istart_angle=%d end=%d\n",
                                        i,start,istart_angle,end);*/

            if(start>end) { /* range from 358 to 2 degrees has start val > end */
                 
               if(istart_angle<start && istart_angle>end) {
                  advance_offset(buffer,offset,num_bytes); 
                  continue;
               }

            } else { /* normal processing with start val < end val */
               
               if(istart_angle<start || istart_angle>end) {
                  advance_offset(buffer,offset,num_bytes); 
                  continue;
               }

            } /*  end else normal */
         
         } /*  end if end!=0 */

      } /* end if flag[4] TRUE */

      radial_found=TRUE;
      

      /* output radial data in decimal non-RLE encoded format */
      /* CVT 4.4 added the rad parameter and the dec_places parameter */
      print_radial("Packet 16 Decimal Output",i,start_angle,delta_angle,buffer,offset,
                     num_bytes,flag[7],flag[8]);
      
      
   } /* end of i loop */

   if(radial_found==FALSE) {
      printf("WARNING: No data was found for radial range start: %hd end: %hd\n",
                  start,end);
   }
   
   /* CVT 4.4 */
   if(flag[7]==PDECODE || flag[7]==FDECODE) {
      if(params_found == FALSE)
        fprintf(stderr,
                "ERROR in obtaining decode parameters, raw levels displayed\n\n");
   }


}




/* CVT 4.4 added the rad parameter and the dec_places parameter */
void print_radial(char *msg,int rad,short start_angle,short delta,char *buffer,
                  int *offset,short num_bytes,int flag, int dec_places) {
   /* print one radial in decimal format */
   
   int i; /* j; */
   int k, last_in_line;
   unsigned char c; /* run,val; */

   float result;
/* CVT 4.4 */
   char result_str[15];
   
   k = 0;
   
   last_in_line = 9; /* 10 values per line decoded */
       
   printf("%s - Radial %d  Angle: %05.1f  Delta: %4.1f\n",
                            msg,rad,start_angle/10.0,delta/10.0);
   
   if(flag==REFL)
      printf("Selected Reflectivity Decoding in dBZ, -999.0 for below threshold\n");
   if(flag==VEL1 || flag==VEL2 || flag==SW)
      printf("Selected Doppler Decoding in m/s, -999.0 below threshold, -888.0 folded\n");
      
   for(i=0;i<num_bytes;i++) {
      
      c=read_byte(buffer,offset);
/*      if(flag!=0) { */
      if(flag==REFL || flag==VEL1 || flag==VEL2 || flag==SW) {
           result = scale_parameter((short)c,flag);
           printf("%6.1f ", result);
      /* added CVT 4.4 */
      } else if(flag==FDECODE || flag==PDECODE) {
           if(params_found == TRUE) {
              decode_level( (unsigned int)c, result_str, &s_o_params, dec_places);
              printf("%s ", result_str);
           } else {
              printf("%3hd   ",(short)c);
           }
      } else {/* explicit pass through */
           printf("%3hd   ",(short)c);
      }

      if(k==last_in_line) {
          printf("\n");
          k=0;

      } else {
          k++;
      }

   }
   
   printf("\n"); 


}

