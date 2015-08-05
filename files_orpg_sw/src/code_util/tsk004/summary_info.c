/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:56 $
 * $Id: summary_info.c,v 1.9 2009/05/15 17:37:56 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/* summary_info.c */

#include "summary_info.h"

int display_summary_info(char *buffer, int force) {
  /* display summary information for the ICD product in the buffer */
  Prod_header *hdr;
  Graphic_product *gp=malloc(120);
  int result=TRUE;
  time_t tval;
  int offset=96;

  int dd,dm,dy;
  char * month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  /* cvt 4.4.1 */
  /* char *dow[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; */
  struct tm *t1;
  char time_string[75];

  if(gp==NULL) {
    fprintf(stderr,"ERROR: Unable to allocate memory\n");
    return(FALSE);
  }

   /* first make sure that the buffer had data in it */
   if(buffer==NULL) {
      fprintf(stderr,"Summary Info ERROR: The product buffer is empty\n");
      return(FALSE);
   }
    

   /* load the Pre-ORPG Header */
   hdr=(Prod_header*)buffer;
  
   /* load the Message Header block and Product Description Block */
   memcpy(gp,buffer+96,120);

   
   result=check_icd_format((char*)gp,FALSE);  
   
   
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

   /* begin to output specific product information */
   fprintf(stderr,"\nPRODUCT SUMMARY INFORMATION\n");
   fprintf(stderr,"---------------------------\n");

   if( (result==TRUE) || (force==TRUE) ) {
      fprintf(stderr,"Message Code:\t\t\t\t\t%hd\n",gp->msg_code); 
   } else 
      fprintf(stderr,"Message Code:\t\t\t\t\tUNK\n");

   /* included summary info wheh no Pre-ICD header present by using information */
   /*           in the MHB and PDB */
   if(hdr->g.vol_t==0 &&  hdr->g.len==0 && hdr->vcp_num==0) {
      fprintf(stderr,"Summary Command Executed on a Product Message with no Internal Header\n");
      fprintf(stderr,"Linear Buffer ID:\t\t\t\t[no hdr]\n");
        calendar_date(gp->vol_date,&dd,&dm,&dy);
      fprintf(stderr,"Volume Scan Start Time\t\t\t\t%s %d, %d \n"
                     "                                                %s  (UTC)\n",
                          month[dm-1], dd, dy, 
                         _88D_secs_to_string( ( (int)(gp->vol_time_ms) << 16 ) | 
                                              ( (int)(gp->vol_time_ls) & 0xffff )) );
      fprintf(stderr,"Total Product Length (without 96 byte header)\t%d\n", gp->msg_len);
      fprintf(stderr,"Volume Scan Sequence Number\t\t\t[no hdr]\n");
      fprintf(stderr,"Elevation Count\t\t\t\t\t[no hdr]\n");
      fprintf(stderr,"Elevation Index\t\t\t\t\t%d *\n", gp->elev_ind);
      fprintf(stderr,"Weather Mode\t\t\t\t\t%d\n", gp->op_mode);
      fprintf(stderr,"VCP Number\t\t\t\t\t%d\n", gp->vcp_num);
      fprintf(stderr,"    * Without the internal header, the elevation index\n"
                     "      returned for all volume products is 0.\n\n");
   } else { 
      fprintf(stderr,"Linear Buffer ID:\t\t\t\t%hd\n",hdr->g.prod_id);
      tval=(time_t)hdr->g.vol_t;
      /*  to avoid confusion, replaced local time (which is easy to get from  */
      /*          ctime() with UTC because the product contains UTC */
      /* CVT 4.4.1 - removed day of week 'dow[t1->tm_wday]' */
      t1 = gmtime(&tval);
      sprintf(time_string,"%s %2d, %4d \n"
                  "                                                %02d:%02d:%02d ",
          month[t1->tm_mon],t1->tm_mday,t1->tm_year+1900,t1->tm_hour,
          t1->tm_min,t1->tm_sec);
      fprintf(stderr,"Volume Scan Start Time\t\t\t\t%s (UTC)\n", time_string);
      /* to avoid confusion if product was originally compressed and subswquently */
      /*         decompressed when extracted */
      fprintf(stderr,"Total Product Length (without 96 byte header)\t%d\n",gp->msg_len);
      fprintf(stderr,"Volume Scan Sequence Number\t\t\t%d\n",hdr->g.vol_num);
      fprintf(stderr,"Elevation Count\t\t\t\t\t%hd\n",hdr->elev_cnt);
      fprintf(stderr,"Elevation Index\t\t\t\t\t%hd\n",get_elev_ind(buffer, orpg_build_i));
      fprintf(stderr,"Weather Mode\t\t\t\t\t%hd\n",hdr->wx_mode);
      fprintf(stderr,"VCP Number\t\t\t\t\t%hd\n\n",hdr->vcp_num);
   }

   if(result==FALSE && force==FALSE) {
      return(FALSE);
   }

   /* IF SYMB PRESENT AND GAB AND TAB ARE NOT PRESENT               */
   /*   AND IF EITHER BLOCK ID != 1  OR FIRST LAYER DIVIDER != -1   */
   /*   THEN  PRINT STAND ALONE ALPHA PRODUCT INFO AND QUIT         */
   if((gp->sym_off>0) && (gp->gra_off==0) && (gp->tab_off==0)) {
    
      short divider,pages,blockID; 

         offset = (96+120);
         divider = read_half(buffer,&offset); /* first optional block */
 
         blockID = read_half(buffer,&offset); /* block ID (if a symbology block) */
         offset = (96+120+10);
         divider = read_half(buffer,&offset); /* first layer (if a symbology block) */
    
      if((blockID !=1) || (divider != -1)) {
         offset = (96+120+2);
         pages = read_half(buffer,&offset);
         fprintf(stderr, "Stand Alone Tabular Alphanumeric Product Information:\n");
         fprintf(stderr,"  Number of Pages=%i\n",pages);
         free(gp);
         return (TRUE);   
      }      

   }

   /* list whether or not symbology, alpha and graphic alpha blocks are included */
   if(gp->sym_off>0 && gp->sym_off<=400000) {
      /* load the symbology block header */
      Sym_hdr *sh=malloc(10);
      int i,offset;
      unsigned short packet_code;

      memcpy(sh,buffer+216,10);
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
  MISC_short_swap(sh,5);
  sh->block_length=INT_SSWAP(sh->block_length);
#endif
    
      fprintf(stderr," symbology offset = %d (%d bytes)\n", 
                                      gp->sym_off, (2*gp->sym_off));
      fprintf(stderr,"Symbology Block Information:\n");
      fprintf(stderr,"  Block Length = %d bytes\n",sh->block_length);
      fprintf(stderr,"  Number of Layers = %i\n",sh->n_layers);


      if(sh->n_layers>18 || sh->n_layers<1) {
         fprintf(stderr,
            "WARNING: Number of Layers (%d) is out of range (1-18).\n",
                                                                      sh->n_layers);
         fprintf(stderr,
            "         CVT will accept up to %d layers for development purposes.\n",
                                                                      MAX_LAYERS);
      }
         
      /* calculate offset the layer divider */
      offset=96 + (2 * gp->sym_off) + 10;
      
      for(i=1;i<=sh->n_layers;i++) {
         int length=0;

         offset+=2; /* advance beyond the layer divider */
         length=read_word(buffer,&offset); /*read the layer length */
         fprintf(stderr,"  Layer %2d Length=%d bytes\n",i,length);

         packet_code=read_half(buffer,&offset); /* read packet code */

         if(length!=0)
             fprintf(stderr,"           First Packet Code=%x Hex or %d decimal\n",
                                                  packet_code,packet_code);
         else
             fprintf(stderr,"\n");
             
         offset-=2; /* back up offset pointer to beginning of data layer */
         offset+=length; /* advance pointer to beginning of next block */
         
      } /*  end for */

      free(sh);
      /* fprintf(stderr,"end offset=%d\n",offset); */
  
   } else if(gp->sym_off>400000) {
      fprintf(stderr,"WARNING:Symbology Block offset is out of range 0 - 400000\n");

   } else {
      fprintf(stderr,"Symbology Block is NOT available\n");

   } /* end of symbology blocks */

   if(gp->gra_off>0 && gp->gra_off<=400000) {
      int offset=0;
      int length=0;
      short pages=0;
 
      fprintf(stderr,"Graphic Alphanumeric Block Information:\n");
      offset=96 + (2 * gp->gra_off) + 4; /* set offset to gab length value */
      length=read_word(buffer,&offset); /* read length field */
      fprintf(stderr,"  Block Length=%d  gab offset=%d (%d bytes)\n", length,
                                                   gp->gra_off , (2*gp->gra_off));
      pages=read_half(buffer,&offset);
      fprintf(stderr,"  Number of Pages: %hd\n",pages);

   } else if(gp->gra_off>400000) {
      fprintf(stderr,"WARNING: Graphic Alpha Block Offset is out of range\n");

   } else {
      fprintf(stderr,"Graphic Alphanumeric Block is NOT available\n");

   }
 
   if(gp->tab_off>0 && gp->tab_off<=400000) {
      int offset=0;
      int length=0;
 
      fprintf(stderr,"Tabular Alphanumeric Block Information:\n");
      offset=96 + (2 * gp->tab_off) + 4; /* set offset to tab length value */
      length=read_word(buffer,&offset); /* read length field */
      fprintf(stderr,"  Block Length=%d  tab offset=%d (%d bytes)\n", length,
                                                   gp->tab_off, (2*gp->tab_off));

   } else if(gp->tab_off>400000) {
       fprintf(stderr,"WARNING: Tabular Alphanumeric Block is out of range\n");

   } else {
     fprintf(stderr,"Tabular Alphanumeric Block is NOT available\n");
   
   }
   
   free(gp);
   return(TRUE);
   
}

