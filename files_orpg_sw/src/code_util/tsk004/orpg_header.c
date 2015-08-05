/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:55 $
 * $Id: orpg_header.c,v 1.10 2009/05/15 17:37:55 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/* orpg_header.c */


#include "orpg_header.h"
#include <orpgsite.h>

#include <time.h>

int print_ORPG_header(char* buffer) {
  /* print out the contents of the ORPG (pre-ICD) Header */
  /* the product should be in the buffer...if not return FALSE */
  Prod_header *hdr;
  Prod_header_b5 *hdr5=NULL; /* needed to reference compression fields */
  Prod_header_b6 *hdr6; /* needed to reference new struct member 'spare' */
  time_t tval;
  struct tm *t1;
  char time_string[75];
  char *month[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
      "Sep","Oct","Nov","Dec"};
  /* cvt 4.4.1 */
  /* char *dow[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; */

  
  if(buffer==NULL) {
    fprintf(stderr,"ERROR: The product buffer is empty\n");
    return(FALSE);
    }
    
  hdr = (Prod_header *) buffer;

  hdr5 = (Prod_header_b5 *)  buffer;
  hdr6 = (Prod_header_b6 *)  buffer;


  fprintf(stderr,"\n\n");
  fprintf(stderr,"*******************************************************\n");
  fprintf(stderr,"**** ORPG INTERNAL PRODUCT HEADER (96 total bytes) ****\n");
  fprintf(stderr,"*******************************************************\n");
  fprintf(stderr,"        ------------------  Product Generation Message (52 Bytes) ---------------\n");
  fprintf(stderr,"Halfword#                                               Decimal       Hexadecimal\n");
/*************************************************************************/  
  /*ALL BUILDS*/
  fprintf(stderr,"01   \tProduct ID (linear buffer number)\t\t%-10hd or 0x%04hX\n",
    hdr->g.prod_id,hdr->g.prod_id);
  fprintf(stderr,"02   \tReplay or Realtime Product\t\t\t%-10hd or 0x%04hX\n",
    hdr->g.input_stream,hdr->g.input_stream);
  fprintf(stderr,"03-04\tLinear Buffer message ID\t\t\t%-10d or 0x%08X\n",
    (unsigned int)hdr->g.id,hdr->g.id);

  fprintf(stderr,"05-06\tGeneration Time \t\t\t\t%-10d or 0x%08X\n",
    (unsigned int)hdr->g.gen_t,(unsigned int)hdr->g.gen_t);
   
  /* process for generation time */
  if(hdr->g.gen_t != 0) {
      /* CVT 4.4.1 - removed day of week 'dow[t1->tm_wday]' */
      t1 = gmtime(&hdr->g.gen_t);
      sprintf(time_string,"%s %2d, %4d \n"
                          "                                        %02d:%02d:%02d",
          month[t1->tm_mon],t1->tm_mday,t1->tm_year+1900,t1->tm_hour,
          t1->tm_min,t1->tm_sec);
      fprintf(stderr,"     \tDecoded Generation Time:\t%-s\n",time_string);
  } else {
    fprintf(stderr,"     \tDecoded Generation Time: \n");
  }

  /* process for scan start time */
  tval=(time_t)hdr->g.vol_t;
  fprintf(stderr,"07-08\tVolume Scan Start Time\t\t\t\t%-10d or 0x%08lX\n",(int)tval,tval);
  
  if(hdr->g.vol_t != 0) {
      /* CVT 4.4.1 - removed day of week 'dow[t1->tm_wday]' */
      t1 = gmtime(&tval);
      sprintf(time_string,"%s %2d, %4d \n"
                          "                                        %02d:%02d:%02d ",
          month[t1->tm_mon],t1->tm_mday,t1->tm_year+1900,t1->tm_hour,
          t1->tm_min,t1->tm_sec);
      fprintf(stderr,"     \tDecoded Scan Start Time:\t%-s\n",time_string);
  } else {
    fprintf(stderr,"     \tDecoded Scan Start Time: \n");
  }

  fprintf(stderr,"09-10\tTotal Product Length (includes 96 byte header)\t%-10d or 0x%08X\n",
    hdr->g.len,hdr->g.len);
    
/********************************************************************/   
  /*BUILD6+*/
if(orpg_build_i <=5) { 
        
  fprintf(stderr,"11-12\tProduct Request Number\t\t\t\t%-10d or 0x%08X\n",
   hdr->g.req_num,hdr->g.req_num);
   
} else { /* Build 6 and newer includes elevation index */

  fprintf(stderr,"11   \tProduct Request Number\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->g.req_num, hdr->g.req_num);
  fprintf(stderr,"12   \tElevation Index\t\t\t\t\t%-10hd or 0x%04hX\n",
    get_elev_ind(buffer, orpg_build_i), get_elev_ind(buffer, orpg_build_i));  

}

/***************************************************************/
  /*ALL BUILDS*/            
  fprintf(stderr,"13-14\tVolume Scan Sequence Number\t\t\t%-10d or 0x%08X\n\n",
    hdr->g.vol_num,hdr->g.vol_num);

  fprintf(stderr,"                                            Signed    Unsigned        Hex\n");
  fprintf(stderr,"15   \tRequest Product Parameter 1\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.req_params[0],(unsigned short)hdr->g.req_params[0],hdr->g.req_params[0]);
  fprintf(stderr,"16   \tRequest Product Parameter 2\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.req_params[1],(unsigned short)hdr->g.req_params[1],hdr->g.req_params[1]);
  fprintf(stderr,"17   \tRequest Product Parameter 3\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.req_params[2],(unsigned short)hdr->g.req_params[2],hdr->g.req_params[2]);
  fprintf(stderr,"18   \tRequest Product Parameter 4\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.req_params[3],(unsigned short)hdr->g.req_params[3],hdr->g.req_params[3]);
  fprintf(stderr,"19   \tRequest Product Parameter 5\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.req_params[4],(unsigned short)hdr->g.req_params[4],hdr->g.req_params[4]);
  fprintf(stderr,"20   \tRequest Product Parameter 6\t%10hd  %10hu     or 0x%04hX\n\n",
    hdr->g.req_params[5],(unsigned short)hdr->g.req_params[5],hdr->g.req_params[5]);
  fprintf(stderr,"21   \tResponse Product Parameter 1\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.resp_params[0],(unsigned short)hdr->g.resp_params[0],hdr->g.resp_params[0]);
  fprintf(stderr,"22   \tResponse Product Parameter 2\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.resp_params[1],(unsigned short)hdr->g.resp_params[1],hdr->g.resp_params[1]);
  fprintf(stderr,"23   \tResponse Product Parameter 3\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.resp_params[2],(unsigned short)hdr->g.resp_params[2],hdr->g.resp_params[2]);
  fprintf(stderr,"24   \tResponse Product Parameter 4\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.resp_params[3],(unsigned short)hdr->g.resp_params[3],hdr->g.resp_params[3]);
  fprintf(stderr,"25   \tResponse Product Parameter 5\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.resp_params[4],(unsigned short)hdr->g.resp_params[4],hdr->g.resp_params[4]);
  fprintf(stderr,"26   \tResponse Product Parameter 6\t%10hd  %10hu     or 0x%04hX\n",
    hdr->g.resp_params[5],(unsigned short)hdr->g.resp_params[5],hdr->g.resp_params[5]);


  fprintf(stderr,"\n        ------------------  Product Header Message (44 Bytes) -------------------\n");
  fprintf(stderr,"Halfword#                                               Decimal       Hexadecimal\n");
  
  /* process for elevation time */
  tval=(time_t)hdr->elev_t;
  fprintf(stderr,"26-28\tElevation Time\t\t\t\t\t%-10u or 0x%08X\n",(unsigned int)tval, 
                                                            (unsigned int)tval);
  if(hdr->elev_t != 0) {
      /* CVT 4.4.1 - removed day of week 'dow[t1->tm_wday]' */
      t1 = gmtime(&tval);
      sprintf(time_string,"%s %2d, %4d \n"
                          "                                        %02d:%02d:%02d ",
          month[t1->tm_mon],t1->tm_mday,t1->tm_year+1900,t1->tm_hour,
          t1->tm_min,t1->tm_sec);
      fprintf(stderr,"     \tDecoded Elevation Time:\t\t%-s\n",time_string);
  } else {
    fprintf(stderr,"     \tDecoded Elevation Time: \n");
  }

   fprintf(stderr,"29   \tElevation Count\t\t\t\t\t%-10hd or 0x%04hX\n",
       hdr->elev_cnt,hdr->elev_cnt);

/********************************************************************/   
  /*BUILD6+*/
if(orpg_build_i <=5) { 

  fprintf(stderr,"30   \tElevation Index\t\t\t\t\t%-10hd or 0x%04hX\n",
    get_elev_ind(buffer, orpg_build_i), get_elev_ind(buffer, orpg_build_i));   
  fprintf(stderr,"31   \tArchive 3 Flag\t\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->archIII_flg,hdr->archIII_flg);
  fprintf(stderr,"32   \tBase Data Status\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->bd_status,hdr->bd_status);

} else { /* build 6 and newer replaced elev_ind with new member 'spare' */

  fprintf(stderr,"30   \tArchive 3 Flag\t\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->archIII_flg,hdr->archIII_flg);
  fprintf(stderr,"31   \tBase Data Status\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->bd_status,hdr->bd_status);
  fprintf(stderr,"32   \tSpare\t\t\t\t\t\t%-10hd or 0x%04hX\n",
    hdr6->spare, hdr6->spare);
    
}

/***************************************************************/
  /*ALL BUILDS*/
  fprintf(stderr,"33-34\tSpot Blank Bitmap\t\t\t\t%-10d or 0x%08X\n",
    hdr->spot_blank_bitmap,hdr->spot_blank_bitmap);
  fprintf(stderr,"35   \tWeather Mode\t\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->wx_mode,hdr->wx_mode);
  fprintf(stderr,"36   \tVCP Number\t\t\t\t\t%-10hd or 0x%04hX\n",
    hdr->vcp_num,hdr->vcp_num);

/****************************************************************/
  /*BUILD5+*/ /* the HW 37-40 were defined in ORPG Build 5 */
  
  if(orpg_build_i <=4) {
    fprintf(stderr,"37-38\tReserved Word 1\t\t\t\t\t%-10d or 0x%08X\n",
      hdr->reserved[0],hdr->reserved[0]);
    fprintf(stderr,"39-40\tReserved Word 2\t\t\t\t\t%-10d or 0x%08X\n",
      hdr->reserved[1],hdr->reserved[1]);
    fprintf(stderr,"41-42\tReserved Word 3\t\t\t\t\t%-10d or 0x%08X\n",
      hdr->reserved[2],hdr->reserved[2]);
    fprintf(stderr,"43-44\tReserved Word 4\t\t\t\t\t%-10d or 0x%08X\n",
      hdr->reserved[3],hdr->reserved[3]);
    fprintf(stderr,"45-46\tReserved Word 5\t\t\t\t\t%-10d or 0x%08X\n",
      hdr->reserved[4],hdr->reserved[4]);
    fprintf(stderr,"47-48\tReserved Word 6\t\t\t\t\t%-10d or 0x%08X\n",
      hdr->reserved[5],hdr->reserved[5]);
     
  } else { /* build 5 and newer includes compression fields */
  
    fprintf(stderr,"37-38\tCompression Method\t\t\t\t%-10d or 0x%08X\n",
      hdr5->compr_method,hdr5->compr_method);
    fprintf(stderr,"39-40\tUncompressed Size (includes 96-byte header)\t%-10d or 0x%08X\n",
      hdr5->orig_size,hdr5->orig_size);
     
    fprintf(stderr,"41-42\tReserved Word 1\t\t\t\t\t%-10d or 0x%08X\n",
      hdr5->reserved[0],hdr5->reserved[0]);
    fprintf(stderr,"43-44\tReserved Word 2\t\t\t\t\t%-10d or 0x%08X\n",
      hdr5->reserved[1],hdr5->reserved[1]);
    fprintf(stderr,"45-46\tReserved Word 3\t\t\t\t\t%-10d or 0x%08X\n",
      hdr5->reserved[2],hdr5->reserved[2]);
    fprintf(stderr,"47-48\tReserved Word 4\t\t\t\t\t%-10d or 0x%08X\n",
      hdr5->reserved[3],hdr5->reserved[3]);
     
  }
/****************************************************************/
fprintf(stderr,"\n=================================================================================\n");  
  return(TRUE);
}


