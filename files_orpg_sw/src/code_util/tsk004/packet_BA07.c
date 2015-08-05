/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:56 $
 * $Id: packet_BA07.c,v 1.7 2009/05/15 17:37:56 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/* packet_BA07.c */

#include "packet_BA07.h"
#include "bscan_format.h"


void packet_BA07(char *buffer,int *offset,int *flag) {
  /* display Packet BA07 or BA0F the RLE Raster Data Packet
     flag[7] contains scale data: 1=refl 2=vel 0.5m/s 3=vel 1m/s 4=sw */
     
   int i; 
   short num_bytes=0;
   int start=0,end=0;
   int row_found=FALSE;
   short num_rows;
   short num_cols=0;
/*   short code=2; */ /* used for sending rle decoding instructions to the BSCAN module */
/* CVT 4.3 internal diagnostics */
/*   enum{NOMOD,RLE,BSCAN};  DEFINED IN cvt.h */
/*    char *truefalse[]={"FALSE","TRUE"}; */
/*    char *format[]={"NONE","RLE","BSCAN"}; */
/*    char *scale[]={"NOSCALE","REFL","VEL1","VEL2","SW"}; */

   fprintf(stderr,"\n---------------- Decoding Packet BA07x BA0Fx------------\n");
   
   fprintf(stderr,"Packet Type (Legacy internal code)\t\t%x\n",(unsigned short)read_half(buffer,offset));
   fprintf(stderr,"Packet Type (Legacy internal code)\t\t%x\n",read_half(buffer,offset));
   fprintf(stderr,"I Coordinate Start\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"J Coordinate Start\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"X Scale INT\t\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"X Scale Fractional (N/A - for Legacy PUP)\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"Y Scale INT\t\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"Y Scale Fractional (N/A - for Legacy PUP)\t%hd\n",read_half(buffer,offset));
   num_rows=read_half(buffer,offset);
   fprintf(stderr,"Number of Rows\t\t\t\t\t%hd\n",num_rows);
   fprintf(stderr,"Packing Descriptor\t\t\t\t%hd\n",read_half(buffer,offset));
   fprintf(stderr,"\n");

   /*add test for format flag etc. */
/* CVT 4.3 */
   if((flag[0]==ROW) && (flag[5]==RLE)) {
      fprintf(stderr,"Rows Left in Run-Length Encoded Format\n");
   } else if(flag[0]==ROW) {
      fprintf(stderr,"Rows Unpacked to show all Data Values\n");
   }
/* CVT 4.3 internal diagnostics */
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
      

   if(flag[4]==TRUE) 
      fprintf(stderr,"WARNING: This is a Raster Product. Use of Degree Modifier is Ignored\n");
      
   if(flag[5]==BSCAN) {
      fprintf(stderr,"ERROR: BSCAN Output is not available in Raster Products\n");
      return;
   }

   /* check for 'all' flag & reset start and end flags */
   if(flag[3]==TRUE) {
       start=0;
       end=num_rows;
   }

   /* quality control input values */
   if(start>num_rows) {
      fprintf(stderr,"Range Error: The Row Start Value of %d Exceeds the Max Number of Rows (%d)\n",
                                          start,num_rows);
      return;
   }
   
   if(end<0) {
      fprintf(stderr,"Range Error: The Row End Value of %d is out of bounds\n",end);
      return;
   }

   

   /* process for each row */
   for(i=1;i<=num_rows;i++) {
      
      /* read the row header */    
      num_bytes=read_half(buffer,offset);
      /*fprintf(stderr,"radial %d #rle halfwords %04hd start angle %04hd delta angle %04hd\n",
        i,num_rle_halfwords,start_angle,delta_angle);*/


      /*  CVT 4.1 test for not exceeding first row length added */
      /*  FUTURE ENHANCEMENT: check to see of all rows are the same length */
      /*                      is that a product requirement?       */
      { /*  begin block ------------------------------------------ */
      /* block finds number of columns in first row */
      unsigned char c,run;
      int k, j;
      
      if( i==1 ) { /*  for first row only */
         num_cols=0;
         
         for(j=0; j<num_bytes; j++) {
                     
             c = buffer[*offset+j];
             run=c>>4;  
               
                for(k=0; k<(int)run; k++)
                    num_cols++;
        
         } /*  end for num_bytes */

         fprintf(stderr,"packet_BA07 - number of columns is %d\n", num_cols);

      } /*  end if i == 1     */

      } /*  end block -------------------------------------------- */

     
      if(end==0) {  /* only one row is requested */
         if(start!=i) {
            advance_offset(buffer,offset,num_bytes);
            continue;
         }
      }
     
      if(end!=0) { /* more than one row is requested */
       /* fprintf(stderr,"row %d start=%d end=%d\n",i,start,end);*/
       
         if(start>end) {
          /* range from 358 to 2 degrees has start val > end */
            if(i<start && i>end) {
               advance_offset(buffer,offset,num_bytes);
               continue;
            }
         
         } else {
             /* normal processing with start val < end val */
            if(i<start || i>end) {
               advance_offset(buffer,offset,num_bytes); 
               continue;
            }
         }

      } /*  end if end != 0 */

     
      row_found=TRUE;
      
      if(flag[5]==RLE) {
         /* output row data in hexidecimal RLE encoded format */
         print_raster_rle("BA07 RLE Output",i,buffer,offset,num_bytes);
        
      } else {
        /* output radial data in decimal non-RLE encoded format */
         decode_raster_rle("BA07 Decimal Output",i,buffer,offset,
         num_bytes,num_cols,flag[7]);
      }
      
   } /* end of i loop */

   if(row_found==FALSE) {
     fprintf(stderr,"WARNING: No data was found for row range start: %hd end: %hd\n",
      start,end);
   }


}



void print_raster_rle(char *msg,short row,char *buffer,int *offset,
   short num_bytes) {
   /* print one raster row in run length encoded format */
   
   int i; /* j; */
   
   int k;
   
   unsigned char c; /* run,val; */

/*   FUTURE ENHANCEMENT: Include looking for beginning of next row and/or */
/*                       end of data packet for consistency check */
   
   k=0;
   
   fprintf(stderr,"%s - Row: %03i\n",msg,row);
   for(i=0;i<num_bytes;i++) {
      
      c=read_byte(buffer,offset);
      fprintf(stderr,"%x ",c);
      
      if(k==19) {
          fprintf(stderr,"\n");
          k=0;
      
      } else {
           k++;
      }
      
   } /*  end for */
   
   fprintf(stderr,"\n"); 
   
   
}




void decode_raster_rle(char *msg,short row,char *buffer,int *offset,
                       short num_bytes, short n_cols, int flag) {
   /* decode one row of run length encoded data and display in decimal */
   
   int i,j;
   
   int k;
   
   unsigned char c,run,val;
   short bin_count;
   short error;


/*   FUTURE ENHANCEMENT: Include looking for beginning of next row and/or */
/*                       end of data packet for consistency check */
   
   k=0;
   bin_count=0;
   error=0;
   
   fprintf(stderr,"%s - Row: %03i\n",msg,row);
   for(i=0; i<num_bytes; i++) {
      
      c=read_byte(buffer,offset);
      /*fprintf(stderr,"%x\n",c);*/
      
      run=c>>4;
      val=c & 0x0f;

      
      for(j=0; j<(int)run; j++) {
        
         bin_count++;

         /*  CVT 4.1  detect unpacked bins exceeding num_bins */
         if(bin_count>n_cols && error==0) {
             fprintf(stderr, "DATA ERROR Packet BA07, number of unpacked "
                              "bins exceeds number of data elements %d\n"
                              "           HW per radial is %d, reading RLE HW %d\n",
                              n_cols, num_bytes/2, (i+1)/2);
             error=1;
         }        
        
         fprintf(stderr,"%02u ",val);
         
         if(k==19) {
            fprintf(stderr,"\n");
            k=0;
         
         } else {
            k++;
         }
          
      } /*  end for j run */

   } /*  end for i num_bytes */
   
   fprintf(stderr,"\n"); 
   
   
}

