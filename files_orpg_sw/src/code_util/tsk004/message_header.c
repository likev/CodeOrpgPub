/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:54 $
 * $Id: message_header.c,v 1.9 2009/05/15 17:37:54 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/* message_header.c */


#include "message_header.h"
#include "misc_functions.h"


int print_message_header(char* buffer) {
  /* print out the contents of the ORPG message Header */
  /* the product should be in the buffer...if not return FALSE */
  Graphic_product *gp=malloc(120);
  int dd,dm,dy;


  if(gp==NULL) {
    fprintf(stderr,"ERROR: Unable to allocate memory\n");
    return(FALSE);
  }

  if(buffer==NULL) {
    fprintf(stderr,"ERROR: The product buffer is empty\n");
    return(FALSE);
  }
    

    memcpy(gp,buffer+96,120);
/*LINUX changes*/
#ifdef LITTLE_ENDIAN_MACHINE
  MISC_short_swap(gp,60);
  gp->msg_time=INT_SSWAP(gp->msg_time);
  gp->msg_len=INT_SSWAP(gp->msg_len);
  gp->latitude=INT_SSWAP(gp->latitude);
  gp->longitude=INT_SSWAP(gp->longitude);
  gp->gen_time=INT_SSWAP(gp->gen_time);
  gp->sym_off=INT_SSWAP(gp->sym_off);
  gp->gra_off=INT_SSWAP(gp->gra_off);
  gp->tab_off=INT_SSWAP(gp->tab_off);
#endif


  fprintf(stderr,"\n\n");
  fprintf(stderr,"******************************************************\n");
  fprintf(stderr,"******** ORPG MESSAGE HEADER BLOCK (18 bytes) ********\n");
  fprintf(stderr,"******************************************************\n");
  fprintf(stderr,"        ---------------------  ORPG Message Header Block  -----------------------\n");
  fprintf(stderr,"Halfword#                                               Decimal       Hexadecimal\n");
  fprintf(stderr,"01   \tNEXRAD Message Code\t\t\t\t%-10hd or 0x%04hX\n",gp->msg_code,gp->msg_code);
  calendar_date(gp->msg_date,&dd,&dm,&dy);

  fprintf(stderr,"02   \tDate of Message\t\t\t\t\t%-10hu or 0x%04hX\n",
                                       (unsigned short)gp->msg_date,gp->msg_date);
  if(gp->msg_date != 0) {
      fprintf(stderr,"     \tDecoded Date of Message:\t%-s %d, %d\n",
                                                           month[dm],dd,dy);
  } else {
      fprintf(stderr,"     \tDecoded Date of Message: \n");
  }
  fprintf(stderr,"03-04\tTime of Message \t\t\t\t%-10d or 0x%08X\n",gp->msg_time,gp->msg_time);
  if(gp->msg_time != 0) {
      fprintf(stderr,"     \tDecoded Time of Message: \t%-s\n",
                                        msecs_to_string(gp->msg_time*1000));  
  } else {
      fprintf(stderr,"     \tDecoded Time of Message: \n");
  }
  fprintf(stderr,"05-06\tProduct Size in Bytes (excludes internal header)%-10u or 0x%08X\n",
/*  unsigned integer to support large products */
                                                 (unsigned int) gp->msg_len, gp->msg_len);
  fprintf(stderr,"07   \tID of the Source\t\t\t\t%-10hd or 0x%04hX\n",gp->src_id,gp->src_id);
  fprintf(stderr,"08   \tID of the Receiver\t\t\t\t%-10hd or 0x%04hX\n",gp->dest_id,gp->dest_id);
  fprintf(stderr,"09   \tNumber of Blocks\t\t\t\t%-10hd or 0x%04hX\n",gp->n_blocks,gp->n_blocks);

  free(gp);  
  return(TRUE);

}




int print_pdb_header(char* buffer) {
  /* print out the contents of the ORPG product description block Header */
  /* the product should be in the buffer...if not return FALSE */
  Graphic_product *gp=malloc(120);
  int dd,dm,dy;  
  /*CVT 4.4.1 */
  unsigned int volume_scan_time;
  int retval;


  if(gp==NULL) {
    fprintf(stderr,"ERROR: Unable to allocate memory\n");
    return(FALSE);
  }

  if(buffer==NULL) {
    fprintf(stderr,"ERROR: The product buffer is empty\n");
    free(gp);
    return(FALSE);
  }

  memcpy(gp,buffer+96,120);
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
  MISC_short_swap(gp,60);
  gp->msg_time=INT_SSWAP(gp->msg_time);
  gp->msg_len=INT_SSWAP(gp->msg_len);
  gp->latitude=INT_SSWAP(gp->latitude);
  gp->longitude=INT_SSWAP(gp->longitude);
  gp->gen_time=INT_SSWAP(gp->gen_time);
  gp->sym_off=INT_SSWAP(gp->sym_off);
  gp->gra_off=INT_SSWAP(gp->gra_off);
  gp->tab_off=INT_SSWAP(gp->tab_off);
#endif



  fprintf(stderr,"\n\n");
  fprintf(stderr,"******************************************************\n");
  fprintf(stderr,"***** ORPG PRODUCT DESCRIPTION BLOCK (102 bytes) *****\n");
  fprintf(stderr,"******************************************************\n");
  fprintf(stderr,"        ------------------ORPG Product Description Block  -----------------------\n");
  fprintf(stderr,"Halfword#                                               Decimal       Hexadecimal\n");
  fprintf(stderr,"10   \tPDB divider\t\t\t\t\t%-10hd or 0x%04hX\n",gp->divider,gp->divider);
  fprintf(stderr,"11-12\tRadar Latitude\t\t\t\t\t%-10d or 0x%08X\n",gp->latitude,gp->latitude);  
  fprintf(stderr,"     \tDecoded Radar Latitude:\t\t%8.3f N\n",gp->latitude/1000.0);
  fprintf(stderr,"13-14\tRadar Longitude\t\t\t\t\t%-10d or 0x%08X\n",gp->longitude,gp->longitude);
  fprintf(stderr,"     \tDecoded Radar Longitude:\t%8.3f W\n",gp->longitude/1000.0);  
  fprintf(stderr,"15   \tRadar Height (MSL)\t\t\t\t%-10hd or 0x%04hX\n",gp->height,gp->height);
  fprintf(stderr,"16   \tInternal Product Code\t\t\t\t%-10hd or 0x%04hX\n",gp->prod_code,gp->prod_code);
  fprintf(stderr,"17   \tOperational Weather Mode\t\t\t%-10hd or 0x%04hX\n",gp->op_mode,gp->op_mode);
  fprintf(stderr,"18   \tVCP Number\t\t\t\t\t%-10hd or %04hX\n",gp->vcp_num,gp->vcp_num);
  fprintf(stderr,"19   \tRequest Sequence Number\t\t\t\t%-10hd or 0x%04hX\n",gp->seq_num,gp->seq_num);
  fprintf(stderr,"20   \tVolume Scan Number\t\t\t\t%-10hd or 0x%04hX\n\n",gp->vol_num,gp->vol_num);
  
  calendar_date(gp->vol_date,&dd,&dm,&dy);

  fprintf(stderr,"21   \tVolume Scan Date\t\t\t\t%-10hu or 0x%04hX\n",
                                             (unsigned short)gp->vol_date,gp->vol_date);
  if(gp->vol_date != 0) {
      fprintf(stderr,"     \tDecoded Volume Scan Date:\t%-s %d, %d\n",month[dm],dd,dy);
  } else {
      fprintf(stderr,"     \tDecoded Volume Scan Date: \n");
  }
  
  /* CVT 4.4.1 - undecoded volume time as an int rather than two shorts */
  retval = read_orpg_product_int( (void *) &gp->vol_time_ms, (void *) &volume_scan_time );
  
  fprintf(stderr,"25-26\tVolume Scan Start Time \t\t\t\t%-10u or 0x%08X\n",
                                             volume_scan_time,volume_scan_time);
  if(gp->gen_time != 0) {
      fprintf(stderr,"     \tDecoded Volume Scan Start Time: %-s\n\n",
                                      _88D_secs_to_string(volume_scan_time));
  } else {
      fprintf(stderr,"     \tDecoded Volume Scan Start Time: \n\n");
  }
  
  fprintf(stderr,"24   \tProduct Generation Date\t\t\t\t%-10hu or 0x%04hX\n",
                                             (unsigned short)gp->gen_date,gp->gen_date);
  if(gp->gen_date != 0) {
      calendar_date(gp->gen_date,&dd,&dm,&dy);
      fprintf(stderr,"     \tDecoded Product Generation Date: %-s %d, %d\n",month[dm],dd,dy);
  } else {
      fprintf(stderr,"     \tDecoded Product Generation Date: \n");
  }
  fprintf(stderr,"25-26\tProduct Generation Time\t\t\t\t%-10u or 0x%08X\n",
                                             (unsigned int)gp->gen_time,gp->gen_time);
  if(gp->gen_time != 0) {
      fprintf(stderr,"     \tDecoded Product Generation Time: %-s\n\n",
                                      _88D_secs_to_string(gp->gen_time));
  } else {
      fprintf(stderr,"     \tDecoded Product Generation Time: \n\n");
  }
  fprintf(stderr,"                                            Signed    Unsigned        Hex\n");
  fprintf(stderr,"27   \tProduct Dependent Parameter 1\t%10hd  %10hu     or 0x%04hX\n",gp->param_1,(unsigned short)gp->param_1,gp->param_1);
  fprintf(stderr,"28   \tProduct Dependent Parameter 2\t%10hd  %10hu     or 0x%04hX\n",gp->param_2,(unsigned short)gp->param_2,gp->param_2);
  fprintf(stderr,"                                                        Decimal       Hexadecimal\n");
  fprintf(stderr,"29   \tVolume Elevation Index\t\t\t\t%-10hd or 0x%04hX\n",gp->elev_ind,gp->elev_ind);

  fprintf(stderr,"                                            Signed    Unsigned        Hex\n");
  fprintf(stderr,"30   \tProduct Dependent Parameter 3\t%10hd  %10hu     or 0x%04hX\n\n",gp->param_3,(unsigned short)gp->param_3,gp->param_3);

  fprintf(stderr,"31   \tData Level Threshold 1\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_1,(unsigned short)gp->level_1,gp->level_1);
  fprintf(stderr,"32   \tData Level Threshold 2\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_2,(unsigned short)gp->level_2,gp->level_2);
  fprintf(stderr,"33   \tData Level Threshold 3\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_3,(unsigned short)gp->level_3,gp->level_3);
  fprintf(stderr,"34   \tData Level Threshold 4\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_4,(unsigned short)gp->level_4,gp->level_4);
  fprintf(stderr,"35   \tData Level Threshold 5\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_5,(unsigned short)gp->level_5,gp->level_5);
  fprintf(stderr,"36   \tData Level Threshold 6\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_6,(unsigned short)gp->level_6,gp->level_6);
  fprintf(stderr,"37   \tData Level Threshold 7\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_7,(unsigned short)gp->level_7,gp->level_7);
  fprintf(stderr,"38   \tData Level Threshold 8\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_8,(unsigned short)gp->level_8,gp->level_8);
  fprintf(stderr,"39   \tData Level Threshold 9\t\t%10hd  %10hu     or 0x%04hX\n", gp->level_9,(unsigned short)gp->level_9,gp->level_9);
  fprintf(stderr,"40   \tData Level Threshold 10\t\t%10hd  %10hu     or 0x%04hX\n",gp->level_10,(unsigned short)gp->level_10,gp->level_10);
  fprintf(stderr,"41   \tData Level Threshold 11\t\t%10hd  %10hu     or 0x%04hX\n",gp->level_11,(unsigned short)gp->level_11,gp->level_11);
  fprintf(stderr,"42   \tData Level Threshold 12\t\t%10hd  %10hu     or 0x%04hX\n",gp->level_12,(unsigned short)gp->level_12,gp->level_12);
  fprintf(stderr,"43   \tData Level Threshold 13\t\t%10hd  %10hu     or 0x%04hX\n",gp->level_13,(unsigned short)gp->level_13,gp->level_13);
  fprintf(stderr,"44   \tData Level Threshold 14\t\t%10hd  %10hu     or 0x%04hX\n",gp->level_14,(unsigned short)gp->level_14,gp->level_14);
  fprintf(stderr,"45   \tData Level Threshold 15\t\t%10hd  %10hu     or 0x%04hX\n",gp->level_15,(unsigned short)gp->level_15,gp->level_15);
  fprintf(stderr,"46   \tData Level Threshold 16\t\t%10hd  %10hu     or 0x%04hX\n\n",gp->level_16,(unsigned short)gp->level_16,gp->level_16);
  
  fprintf(stderr,"47   \tProduct Dependent Parameter 4\t%10hd  %10hu     or 0x%04hX\n",gp->param_4,(unsigned short)gp->param_4,gp->param_4);
  fprintf(stderr,"48   \tProduct Dependent Parameter 5\t%10hd  %10hu     or 0x%04hX\n",gp->param_5,(unsigned short)gp->param_5,gp->param_5);
  fprintf(stderr,"49   \tProduct Dependent Parameter 6\t%10hd  %10hu     or 0x%04hX\n",gp->param_6,(unsigned short)gp->param_6,gp->param_6);
  fprintf(stderr,"50   \tProduct Dependent Parameter 7\t%10hd  %10hu     or 0x%04hX\n",gp->param_7,(unsigned short)gp->param_7,gp->param_7);
  fprintf(stderr,"51   \tProduct Dependent Parameter 8\t%10hd  %10hu     or 0x%04hX\n",gp->param_8,(unsigned short)gp->param_8,gp->param_8);
  fprintf(stderr,"52   \tProduct Dependent Parameter 9\t%10hd  %10hu     or 0x%04hX\n",gp->param_9,(unsigned short)gp->param_9,gp->param_9);
  fprintf(stderr,"53   \tProduct Dependent Parameter 10\t%10hd  %10hu     or 0x%04hX\n\n",gp->param_10,(unsigned short)gp->param_10,gp->param_10);
  
  fprintf(stderr,"                                                        Decimal       Hexadecimal\n");
  fprintf(stderr,"54   \tProduct Version (if available)\t\t\t%-10hd or 0x%04hX\n",
                                               (gp->n_maps  >> 8) & 0xff,(gp->n_maps  >> 8) & 0xff);  
  fprintf(stderr,"     \tSpot Blanking Bit\t\t\t\t%-10hd or 0x%04X\n\n",
                                               gp->n_maps & 0xff,gp->n_maps & 0xff);
  
  fprintf(stderr,"55-56\tSymbology Offset (halfwords)\t\t\t%-10d or 0x%08X\n",gp->sym_off,gp->sym_off);
  fprintf(stderr,"57-58\tGraphic Block Offset (halfwords)\t\t%-10d or 0x%08X\n",gp->gra_off,gp->gra_off);
  fprintf(stderr,"59-60\tTabular Block Offset (halfwords)\t\t%-10d or 0x%08X\n",gp->tab_off,gp->tab_off);
  
  free(gp);
  
  return(TRUE);

}
