/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:53 $
 * $Id: inventory_functions.c,v 1.12 2009/05/15 17:37:53 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
 
/* inventory_functions.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lb.h>
#include <prod_gen_msg.h>
#include "inventory_functions.h"

#include <time.h>


char time_string[25];
void CVT_get_vol_time_string (Prod_header *hdr_ptr, char *in_str);



/* from using the 'cvt i LBNAME' command */
/* =================================================================== */
int ORPG_ProductLB_inventory(char *prod_fname) {
   /* show the inventory of a specific product linear buffer. the
    * path for ORPGDIR will be pre-pended to the input prod_fname.*/
   char filename[128];
   char *charval;
   int num_products;
   int lbfd=-1;
   LB_info list[MAX_PRODUCTS]; /* current max is 1000 for alert messages */
   int len,i,result;
   char *lb_data=NULL;  
   Prod_header *hdr;
   int product_in_db=FALSE;

   fprintf(stderr,"\n      ORPG Products Linear Buffer Inventory Utility\n");

   /* check to make sure that a filename has been passed to the module */
   if(strlen(prod_fname)<3 || prod_fname==NULL) {
      fprintf(stderr,"LB ERROR: The following Linear Buffer Name: %s\n", prod_fname);
      fprintf(stderr,"  is an illegal entry. Please try again\n");
      return(FALSE);
   }

   /* if the LB filename does not include a trailing '.lb', then append */
   if(strstr(prod_fname,".lb")==NULL) {
      strcat(prod_fname,".lb"); /* concatenate '.lb' to input fname */
   }

   /* get the path to the datacode directory */
   charval=getenv("ORPGDIR");
   if(charval==NULL) {
      fprintf(stderr,"ERROR: ORPGDIR environmental variable is not set\n");
      return(FALSE);
   }

   /* construct a path for the specific product linear buffer */
   if(prod_fname[0]=='/')
      sprintf(filename,"%s%s",charval,prod_fname);
    else
      sprintf(filename,"%s/%s",charval,prod_fname);
      
   
   filename[strlen(filename)]=0;
   fprintf(stderr,"full Linear Buffer Path:  %s\n\n",filename);

   result=check_for_directory_existence(filename);
   if(result==FALSE) {
      fprintf(stderr,"LB ERROR: The Product Linear Buffer for %s was NOT found at\n",
         prod_fname);
      fprintf(stderr,"      %s\n",filename);
      return(FALSE); /* abort the program */
   }

   /* open the product linear buffer */
   lbfd=LB_open(filename,LB_READ,NULL);
   if(lbfd<0) {
      fprintf(stderr,"LB ERROR: Could NOT open the following linear buffer:\n");
      fprintf(stderr,"  %s\n",prod_fname);
      return(FALSE);
   }

   /* retrieve information from the linear buffer */
   num_products=LB_list(lbfd,list,MAX_PRODUCTS);
   /*fprintf(stderr,"LB Inventory: Number of Products Available = %d\n",num_products);*/

   /* CVT 4.4 */
   if(num_products==0)
      fprintf(stderr,"\nThere are no products in the Buffer\n\n");

   for(i=1;i<=num_products;i++) {
/* DEBUG */
/*fprintf(stderr,"    DEBUG: Message %3d: ID=%d Size=%d Mark=%d\n",i,list[i].id, */
/*  list[i].size,list[i].mark);                                                  */
      
      len=LB_read(lbfd,&lb_data,LB_ALLOC_BUF,list[i-1].id);

      if(lb_data==NULL) {
          fprintf(stderr,"ERROR Product Inventory - unable to read message\n");
          return(FALSE);
      }
      
      hdr=(Prod_header *) lb_data;
      
      if(hdr->g.len<=0) break;
      
      product_in_db=FALSE;      
      /* check for message length. if 96 bytes then it is a final product
       * or it could be a warehoused intermediate product. If it is greater than
       * 96 bytes it is an intermediate product */
      if(len==96)
         product_in_db=TRUE;

     if(i==1) {
         if(strstr(filename,"product_data_base")!=NULL)
            fprintf(stderr," Inventory of Product Database LB %hd\n",hdr->g.prod_id);
         else {
            fprintf(stderr," Inventory of Linear Buffer: %s\n",prod_fname);
            if(product_in_db==TRUE)
               fprintf(stderr,"  MSG#  LBMSGLEN LBMSGNUM PRODLEN  VOLNUM   ELEV   VOL DATE    TIME\n");
                  
               else
               fprintf(stderr,"  MSG#  LBMSGLEN LBMSGNUM PRODLEN  VOLNUM   ELEV   VOL DATE    TIME\n");
        }
     }

         
      if(product_in_db==TRUE) {
         CVT_get_vol_time_string (hdr, time_string);
         fprintf(stderr,"  %3d    %06d    %04hd    %06d    %03d     %02d   %s\n",i,len,hdr->g.id,
                 hdr->g.len, hdr->g.vol_num, get_elev_ind(lb_data, orpg_build_i), time_string);

      } else {
         CVT_get_vol_time_string (hdr, time_string);
         fprintf(stderr,"  %3d    %06d    %04hd    %06d    %03d     %02d   %s\n",i,len,hdr->g.prod_id,
                 hdr->g.len, hdr->g.vol_num, get_elev_ind(lb_data, orpg_build_i), time_string);
      }
      
      if(lb_data!=NULL) 
          free(lb_data);
      
   } /*  end for */

   LB_close(lbfd);

   fprintf(stderr,"\nNOTE: THE MESSAGE LIST IS NOT SORTED, internal message order is used.\n");
   fprintf(stderr,"Key:\n");
   /*CVT 4.3.3 */
   fprintf(stderr,"  MSG#= The message sequence number as listed within the specific linear buffer\n");
   fprintf(stderr,"  LBMSGLEN=Linear Buffer Message Length\n");
   if(product_in_db==TRUE)
      fprintf(stderr,"  LBMSGNUM=Message Sequence Number of this product in the main database\n");
   else
      fprintf(stderr,"  LBID=Linear Buffer ID associated with this product\n");
   fprintf(stderr,"  PRODLEN=Product Message Length (includes 96 byte internal header)\n");
   fprintf(stderr,"  VOLNUM=Volume Number associated with the product\n");
   fprintf(stderr,"  ELEV= The Elevation Index associated with the product\n");
   fprintf(stderr,"\n");

   return(TRUE);

} /* end ORPG_ProductLB_inventory() */



/* ==================================================================== */
int ORPG_requestLB_inventory(char *prod_fname) {
   /* show the inventory of the product request linear buffer */
   char filename[128];
   char *charval;
   int num_products;
   int lbfd=-1;
   int num_msgs=0;
   LB_info list[MAX_PRODUCTS];
   int len,i,result;
   char *lb_data=NULL; 
   Prod_request *hdr;

   fprintf(stderr,"\n      ORPG Product Request Linear Buffer Inventory Utility\n");

   /* check to make sure that a filename has been passed to the module */
   if(strlen(prod_fname)<3 || prod_fname==NULL) {
      fprintf(stderr,"LB ERROR: The following Linear Buffer Name: %s\n", prod_fname);
      fprintf(stderr,"  is an illegal entry. Please try again\n");
      return(FALSE);
   }

   /* if the LB filename does not include a trailing '.lb', then append */
   if(strstr(prod_fname,".lb")==NULL) {
      strcat(prod_fname,".lb"); /* concatenate '.lb' to input fname */
   }

   /* get the path to the datacode directory */
   charval=getenv("ORPGDIR");
   if(charval==NULL) {
      fprintf(stderr,"ERROR: ORPGDIR environmental variable is not set\n");
      return(FALSE);
   }

   /* construct a path for the specific product linear buffer */
   if(prod_fname[0]=='/')
      sprintf(filename,"%s%s",charval,prod_fname);
   else
      sprintf(filename,"%s/%s",charval,prod_fname);
      
   
   filename[strlen(filename)]=0;
   fprintf(stderr,"full Linear Buffer Path:  %s\n\n",filename);


   
   result=check_for_directory_existence(filename);
   if(result==FALSE) {
      fprintf(stderr,"LB ERROR: The Product Request Linear Buffer for %s was NOT found at\n",
         prod_fname);
      fprintf(stderr,"      %s\n",filename);
      return(FALSE); /* abort the program */
   }

   /* open the product request linear buffer */
   lbfd=LB_open(filename,LB_READ,NULL);
   if(lbfd<0) {
      fprintf(stderr,"LB ERROR: Could NOT open the following linear buffer:\n");
      fprintf(stderr,"  %s\n",prod_fname);
      return(FALSE);
   }
   
   /* show linear buffer status information */
   result=get_LB_status(lbfd);

   /* retrieve information from the linear buffer */
   num_products=LB_list(lbfd,list,MAX_PRODUCTS);
   fprintf(stderr,"REQ LB Inventory: Number of Requests Available = %d\n",num_products);

   for(i=1;i<=num_products;i++) {
      int j;
      int cnt=0;
     
      /*fprintf(stderr,"Message %3d: ID=%d Size=%d Mark=%d\n",i,list[i].id,
         list[i].size,list[i].mark);*/

      num_msgs=list[i-1].size/24;
      /*fprintf(stderr,"number of messages to process=%d\n",num_msgs);*/

      len=LB_read(lbfd,&lb_data,LB_ALLOC_BUF,list[i-1].id);

      if(lb_data==NULL) {
          fprintf(stderr,"ERROR reading request buffer - unable to read message\n");
          return(FALSE);
      }

      if(i==0) {
         fprintf(stderr,"\n            Inventory of Product Request Linear Buffer\n");
         fprintf(stderr,"MSG#  LB ID  P1    P2    P3    P4    P5    P6  ElevIdx Req# Seq#  Type\n");
      }

      /* each message list contains a dummy structure with a -1 lb ID. The following list
       * does NOT print the dummy structure (to reinstate make the loop interator '<=' ) */
      for(j=1;j<num_msgs;j++) {

         hdr=(Prod_request *) (lb_data+cnt);
       
         fprintf(stderr,"%03i %5i %5i %5i %5i %5i %5i %5i %5i %5i %5i %6i\n",i,
           hdr->pid,hdr->param_1,hdr->param_2,hdr->param_3,hdr->param_4,hdr->param_5,
           hdr->param_6,hdr->elev_ind,hdr->req_num,hdr->vol_seq_num,hdr->type);

         cnt+=24;
      
      } /*  end for j */

      if(lb_data!=NULL) 
          free(lb_data);
      
   } /*  end for i */

   LB_close(lbfd);

   fprintf(stderr,"\nKey:\n");
   fprintf(stderr,"  LB ID = Linear Buffer ID\n");
   fprintf(stderr,"  P1-P6 = 6 Product Dependent Parameters contained within the Request Message\n");
   fprintf(stderr,"  ElevIdx = Elevation Index\n");
   fprintf(stderr,"    For Elevation Products: a POSITIVE index refers to the elevation, -1 means\n");
   fprintf(stderr,"    all elevations and -2 means NOT SCHEDULED\n");
   fprintf(stderr,"    For Volume Products: -1 means scheduled and -2 means NOT SCHEDULED\n");
   fprintf(stderr,"  Req# = A Unique request sequence number\n");
   fprintf(stderr,"  Seq# = Volume Scan Sequence Number\n");
   fprintf(stderr,"  Type = Product request type: 0=ALERT_OT_REQUEST or 1=USER_OT_REQUEST\n");
   fprintf(stderr,"\n");

   return(TRUE);
   
} /* end ORPG_requestLB_inventory() */





/* =================================================================== */
int get_LB_status(int fd) {
   /* display linear buffer status message */
    LB_attr attr;
    LB_status stat;
    int ret;
    
    stat.n_check = 0;
    stat.attr = &attr;
    ret = LB_stat (fd, &stat);
    
    if (ret < 0) {
       printf ("LB_stat failed (ret = %d)\n", ret);
       return(FALSE);
    }
      
    printf ("\nLinear Buffer Status Utility\n");
    attr.remark[LB_REMARK_LENGTH - 1] = '\0';
    printf ("LB Remark:\t\t\t\t%s\n", attr.remark);
    printf ("LB Access Permission\t\t\t%o\n",(unsigned int)attr.mode);
    if(attr.msg_size > 0)
       printf ("Avg Message Size\t\t\t%d\n", attr.msg_size);
    else
       printf ("Avg Message Size\t\t\t%s\n","undefined");
    
    printf ("Max Nbr Messages\t\t\t%d\n", attr.maxn_msgs);
    if (attr.types & LB_MEMORY)
        printf ("LB Type\t\t\t\t\t%s\n","LB_MEMORY");
    if (attr.types & LB_REPLACE)
        printf ("LB Type\t\t\t\t\t%s\n","LB_REPLACE");
    if (attr.types & LB_MUST_READ)
        printf ("LB Type\t\t\t\t\t%s\n","LB_MUST_READ");
    if (attr.types & LB_SINGLE_WRITER)
        printf ("LB Type\t\t\t\t\t%s\n","LB_SINGLE_WRITER");
    if (attr.types & LB_NOEXPIRE)
        printf ("LB Type\t\t\t\t\t%s\n","LB_NOEXPIRE");
    if (attr.types & LB_UNPROTECTED)
        printf ("LB Type\t\t\t\t\t%s\n","LB_UNPROTECTED");
    if (attr.types & LB_DIRECT)
        printf ("LB Type\t\t\t\t\t%s\n","LB_DIRECT");
    if (attr.types & LB_UN_TAG)
        printf ("LB Type\t\t\t\t\t%s\n","LB_UN_TAG");
    if (attr.types & LB_MSG_POOL)
        printf ("LB Type\t\t\t\t\t%s\n","LB_MSG_POOL");
    if(stat.updated==LB_TRUE)
       fprintf(stderr,"LB Updated\t\t\t\tTRUE\n");
    else
       fprintf(stderr,"LB Updated\t\t\t\tFALSE\n");
       
    printf ("\n");
    
    return(TRUE);

} /* end get_LB_status() */



/* from using the 'cvt i' command */
/* ==================================================================== */
int ORPG_Database_inventory() {
   /* show the inventory of the database linear buffer */
   char filename[128];
   int num_products;
   int lbfd=-1;
   LB_info list[MAX_PRODUCTS];
   int len,i; /* result; */
   char lb_data[96];
   Prod_header *hdr;
   int TEST=FALSE;

   fprintf(stderr,"\n      ORPG Products Database Inventory Utility\n");



   /* here we are using a product data base (determined in dispatcher.c) */
   strcpy(filename,cvt_prod_data_base);
   
   /* open the product linear buffer */
   lbfd=LB_open(filename,LB_READ,NULL);
   if(lbfd<0) {
      fprintf(stderr,"ERROR opening the following linear buffer: %s\n",filename);
      return(FALSE);
   }

   /* retrieve information from the linear buffer */
   num_products=LB_list(lbfd,list,MAX_PRODUCTS);
   if(TEST) fprintf(stderr,"num_products available=%i\n",num_products);

   /* CVT 4.4 */ /* CVT 4.4.1 - changed ==0 to <=1 */
   if(num_products<=1)
      fprintf(stderr,"\nThere are no products in the Product Database\n\n");
      
   for(i=1;i<=num_products;i++) {
     /*fprintf(stderr,"Message %i: ID=%d Size=%d Mark=%d\n",i,list[i].id,
      *  list[i].size,list[i].mark);*/

      len=LB_read(lbfd,lb_data,96,list[i-1].id);

      hdr=(Prod_header *) lb_data;

     if(i==1) {   

        fprintf(stderr,"Inventory of the Product Database Linear Buffer\n");

        fprintf(stderr,"  MSG#      LBID    PRODLEN   VOLNUM     ELEV    VOL DATE    TIME\n");

     } else if(len == LB_EXPIRED) {
        /* may decide to show expired messages as a future option */
        /*  fprintf(stderr,"  N/A     N/A   EXPIRED   N/A    N/A\n");  */

     } else if((len <= 0) && (len != LB_BUF_TOO_SMALL)) {
        fprintf(stderr,"   N/A    LINEAR BUFFER READ ERROR \n");    

     } else {
           CVT_get_vol_time_string (hdr, time_string);
           fprintf(stderr,"  %4d    ID %4hd    %06d    V %03d     E %02d   %s\n", 
               i, hdr->g.prod_id, hdr->g.len, hdr->g.vol_num, 
               get_elev_ind(lb_data, orpg_build_i), time_string);
        
     }
    

   } /*  end for i */

   LB_close(lbfd);

   fprintf(stderr,"\nNOTE: THE PRODUCT LIST IS NOT SORTED, internal message order is used.\n");
   fprintf(stderr,"Key:\n");
   fprintf(stderr,"  MSG#= Message Sequence Number within the Products Database Linear Buffer\n");
   fprintf(stderr,"  LBID=Linear Buffer ID associated with each product\n");
   fprintf(stderr,"    Use this value to find the product name within the configuration\n");
   fprintf(stderr,"    file: product_attr_table\n");
   fprintf(stderr,"  PRODLEN=Product Message Length (includes 96 byte internal header)\n");
   fprintf(stderr,"  VOLNUM=Volume Number associated with the product\n");
   fprintf(stderr,"  ELEV= The Elevation Index associated with the product\n");
   fprintf(stderr,"\n");

   return(TRUE);

} /* end ORPG_Database_inventory() */
   


/* from using the 'cvt slb ID' command */
/* ==================================================================== */
int ORPG_Database_search(short lb_id) {
   /* search the database linear buffer and look for the linear buffer ID */
   char filename[128];
   int num_products;
   int lbfd=-1;
   LB_info list[MAX_PRODUCTS];
   int len,i; /* result; */
   char lb_data[96];
   Prod_header *hdr;
   int found=FALSE;
   int TEST=FALSE;

   fprintf(stderr,"\n      ORPG Products Database Search Utility: LB ID=%d\n",lb_id);


   /* here we are using a product data base (determined in dispatcher.c) */
   strcpy(filename,cvt_prod_data_base);

   /* open the product linear buffer */
   lbfd=LB_open(filename,LB_READ,NULL);
   if(lbfd<0) {
      fprintf(stderr,"ERROR opening the following linear buffer: %s\n",filename);
      return(FALSE);
   }

   /* retrieve information from the linear buffer */
   num_products=LB_list(lbfd,list,MAX_PRODUCTS);
   if(TEST) fprintf(stderr,"num_products available=%i\n",num_products);

   /* CVT 4.4 */ /* CVT 4.4.1 - changed ==0 to <=1 */
   if(num_products<=1)
      fprintf(stderr,"\nThere are no products in the Product Database\n");

   for(i=1;i<=num_products;i++) {
     /*fprintf(stderr,"Message %i: ID=%d Size=%d Mark=%d\n",i,list[i].id,
      *   list[i].size,list[i].mark);*/

      len=LB_read(lbfd,lb_data,96,list[i-1].id);

      if(lb_data==NULL) {
          fprintf(stderr,
                  "ERROR Searching Database - unable to read buffer message\n");
          return(FALSE);
      }

      hdr=(Prod_header *) lb_data;

      if((len <= 0) && (len != LB_BUF_TOO_SMALL))
         continue;
         
      if(len == LB_EXPIRED)
         continue;

      if(TEST)
         fprintf(stderr,"i=%d prod_id=%hd target=%hd\n",i,hdr->g.prod_id,lb_id);

      if(found==FALSE && lb_id==hdr->g.prod_id) {
         fprintf(stderr," Search was a Success!\n");
         fprintf(stderr,"  MSG#      LBID    PRODLEN   VOLNUM     ELEV    VOL DATE    TIME\n");
         found=TRUE;
      }

     if(found==TRUE && hdr->g.prod_id==lb_id) {
        CVT_get_vol_time_string (hdr, time_string);
        fprintf(stderr,"  %4d    ID %4hd    %06d    V %03d     E %02d   %s\n", 
            i, hdr->g.prod_id, hdr->g.len, hdr->g.vol_num, 
            get_elev_ind(lb_data, orpg_build_i), time_string);
     }

   } /*  end for i */
   
      /* CVT 4.4 */
   if(found==FALSE)
      fprintf(stderr,"\nThere are no products ID %d in the Product Database\n", lb_id);

   LB_close(lbfd);
   
   if(found==TRUE) {
      fprintf(stderr,"\nNOTE: THE PRODUCT LIST IS NOT SORTED, internal message order is used.\n");
      fprintf(stderr,"Key:\n");
      fprintf(stderr,"  MSG#=Message Sequence Number within the Products Database Linear Buffer\n");
      fprintf(stderr,"  LBID=Linear Buffer ID associated with this product\n");
      fprintf(stderr,"    Use this value to find the product name within the configuration\n");
      fprintf(stderr,"    file: product_attr_table\n");
      fprintf(stderr,"  PRODLEN=Product Message Length (includes 96 byte internal header)\n");
      fprintf(stderr,"  VOLNUM=Volume Number associated with the product\n");
      fprintf(stderr,"  ELEV= The Elevation Index associated with the product\n");
      fprintf(stderr,"\n");
   }
   else {
      fprintf(stderr,"Inventory Search failed for Linear Buffer ID: %i\n",lb_id);
   }

   return(TRUE);

} /* end ORPG_Database_search() */
  
  
  
  
/* ==================================================================== */
void CVT_get_vol_time_string (Prod_header *hdr_ptr, char *in_str) {
   time_t tval;
   struct tm *t1;
   
   tval=(time_t)hdr_ptr->g.vol_t;
      if(tval != 0) {
      t1 = gmtime(&tval);
      sprintf(in_str,"%4d/%02d/%02d %02d:%02d:%02d",
          t1->tm_year+1900, t1->tm_mon+1, t1->tm_mday,
          t1->tm_hour, t1->tm_min, t1->tm_sec);
   } else {
      sprintf(in_str,"Time Error");
   }
   
}




/* =================================================================== */
int check_for_directory_existence(char *dirname) {
   /* if the directory exists return TRUE else FALSE */
   FILE *fp;
   
   
   fp=fopen(dirname,"rb");
   if(fp==NULL) {
      /*fprintf(stderr,"CHECK DIR ERROR: %s does NOT exist!\n",dirname);*/
      return(FALSE);
   }
   else {
      fclose(fp);
      return(TRUE);
   }
}



/* ============================================================================ */
void Extract_LB_Product(int msg_id,int flag, char *extract_lb,char *output_file,
   int nohdr, int force, int no_decomp) {
   /* extract a product within the products database linear buffer              */
   /* (unless specifically sent as a path). extract_lb contains a pointer to the*/
   /* filename within the datacode subdirectory. Output is sent to              */
   /* buffer.out if output_file==NULL otherwise use the char string for         */
   /* storage. anchor the output to the HOME directory.                         */
   /*                                                                           */
   /* nohdr is set to TRUE if the first 96 bytes of the product should NOT      */
   /* be written to disk (the Pre-ICD header).                                  */
   /*                                                                           */
   /* The flag parameter is set to 0=NONE, 1=EXTRACT, 2=HEXDUMP 3=BOTH          */

   char filename[128];
   char *charval;
   int num_products;
   int lbfd=-1;

   LB_info list[MAX_PRODUCTS];
   int len,msg_len,result; 

   Prod_header *hdr;
   char *buffer=NULL;  /* cvt 4.3 */
   Graphic_product *phd;
   short param_8;

   int start_pos, icd_format;

   fprintf(stderr,"\n*** ORPG Linear Buffer Extract Utility ***\n");

   /* get the input path. if the input is NULL, then use the
    * database LB...otherwise set the anchor to ORPGDIR and add
    * the path sent in via the parmeter */
   if(extract_lb==NULL){

      /* here we are using a product data base (determined in dispatcher.c) */
      strcpy(filename,cvt_prod_data_base);
   }
    else {
      charval=getenv("ORPGDIR");
      if(charval==NULL) {
         fprintf(stderr,"ERROR: environmental variable ORPGDIR is not set\n");
         return;
      } 
      sprintf(filename,"%s/%s",charval,extract_lb);      
   }

   /* set up the input path */
   fprintf(stderr,"-> Accessing Linear Buffer: %s for product %i\n",
         filename,msg_id);
  
   result=check_for_directory_existence(filename);
   if(result==FALSE) {
      fprintf(stderr,"ERROR: The Linear Buffer was not found at: \n");
      fprintf(stderr,"      %s\n",filename);
      return; /* abort the program */
   }

   /* open the product linear buffer */
   lbfd=LB_open(filename,LB_READ,NULL);
   if(lbfd<0) {
      fprintf(stderr,"ERROR opening the following linear buffer: %s\n",filename);
      return;
   }

   /* retrieve information from the linear buffer */
   num_products=LB_list(lbfd,list,MAX_PRODUCTS);
   fprintf(stderr,"-> Number of Products Available=%d\n",num_products);
   fprintf(stderr,"-> Message Sequence Number=%d\n",msg_id);

   /* check for potential overflow with msg_id variable */
   if(msg_id<1 || msg_id> num_products) {
      fprintf(stderr,"ERROR: input message sequence number is out of bounds\n");
      LB_close(lbfd);
      return;
   }

   len=LB_read(lbfd,&buffer,LB_ALLOC_BUF,list[msg_id-1].id);

   if(buffer==NULL) {
        fprintf(stderr,
                "ERROR Extractiing Product - Unable to read buffer message\n");
        return;
   }

   hdr=(Prod_header *) buffer;

   fprintf(stderr,"-> Product Info: LBuffer# %03hd MSGLEN %06d VOLNUM %02d ELEV %02d\n",
          hdr->g.prod_id, hdr->g.len, hdr->g.vol_num, get_elev_ind(buffer, 
                                                                 orpg_build_i));

   LB_close(lbfd);
   /*fprintf(stderr,"buffer len=%d  list size=%d\n",len,list[msg_id].size);*/


   /* make sure we read in everything */
   if(len != list[msg_id-1].size) {
      if(buffer!=NULL) 
          free(buffer);
      fprintf(stderr,"ERROR: reading linear buffer message\n");
      return;
   }
   
   /* check to see if the body of the product is compressed */


  
/* MUST FIRST CHECK FOR VALID ICD PRODUCT, intermediate */
/* product decompression to be supported in future */

    icd_format=check_icd_format(buffer+96, FALSE);




/*  DEBUG */
/*    msg_len = hdr->g.len; */
/*    fprintf(stderr,"DEBUG EXTRACT: Total message length is %d (including 96 byte internal header.\n", msg_len);    */
/*  msg_len = phd->msg_len + 96;  */ 
   
   /*  length fields within the 96 byte internal header include the 96 bytes */
   /*  length fields in the product description block do NOT include the 96 bytes */
   /*  following logic completely reworked to handle compressed & uncompressed */
   /*  final products and uncompressed intermediate products, */
   
   if( (icd_format == TRUE) || (force==TRUE) ) {
      if(no_decomp==TRUE) {
          /*  get uncompressed length from internal header and ignore compression */
          msg_len = hdr->g.len-96;
          fprintf(stderr,"Product length is %d (excluding 96 byte internal header)\n",
                   msg_len);
   
      } else { /*  no_decomp==FALSE */
          /* if a product is empty (length == 120 for just MHB and PDB) do NOT */
          /* test for compression                                              */
          phd = (Graphic_product *)(buffer+96);
          param_8=phd->param_8;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
          param_8=SHORT_BSWAP(param_8);
#endif
          if((hdr->g.len-96)>120) {
              /* the following test for compression will work unless the first */
              /* short of the compressed symbology block is equal to -1        */
              /* first, pdp 8 should not be 0 and 
               * second, the section divider (-1) should not be present        */
              if( ( (param_8 == 1) || (param_8 == 2) ) &&
                  ( ((buffer[96+120]&0xff) != 0xff) ||
                    ((buffer[96+121]&0xff) != 0xff) ) ) {

                  fprintf(stderr,"Decompressing product for extraction.\n");
                  product_decompress(&buffer);
              
              }  /*  end test and decompress if required */
              
          } /*  end not an empty message */
          
          phd = (Graphic_product *)(buffer+96);
          msg_len = phd->msg_len;
#ifdef LITTLE_ENDIAN_MACHINE 
          msg_len = INT_BSWAP(msg_len);
#endif
          fprintf(stderr,"Product length is %d (excluding 96 byte internal header)\n",
                   msg_len);
        
      } /*  end no_decomp==FALSE */
      
   } else { /*  not an icd product */
    /*  FUTURE SUPPORT FOR COMPRESSED INTERMEDIATE PRODUCTS HERE  */
         msg_len = hdr->g.len-96;
         fprintf(stderr,"Intermediate Product length is %d (excluding 96 byte internal header).\n", 
                     msg_len); 
   } /*  end not an icd product */


   /* use to extract a specific product and dump to disk */
   if(flag==EXTRACT || flag==BOTH) {
     FILE *fp;
     char outname[128];
     int i;

     fprintf(stderr,"LB Product Extract Utility Initiated\n");
     /* get the path for the products database */
     charval=getenv("HOME");

     if(charval==NULL) {
       fprintf(stderr,"ERROR: environmental variable for HOME is not set\n");
       if(buffer!=NULL) 
           free(buffer);
       return;
     }
     else {
        strcpy(outname,charval);
       strcat(outname,"/");
        if(output_file==NULL)
          strcat(outname,"graphic/buffer.out");
        else
         strcat(outname,output_file);
        /*fprintf(stderr,"filename: %s\n\n",outname);*/
     }
   
     fp=fopen(outname,"w");
     if(fp==NULL) {
        fprintf(stderr,"ERROR: could not open %s for output\n",
            outname);
        fprintf(stderr,"Product Extraction Aborted\n");
        if(buffer!=NULL) 
            free(buffer);
        return; 
     }
         
     if(nohdr==TRUE)
         start_pos=96;
     else 
         start_pos=0;
    /* after making adjustments to handle intermediate uncompressed */
    /*         products and final compressed products on LINUX, the value */
    /*         of msg_len does not include the 96 byte header, */
     for(i=start_pos;i<(msg_len+96);i++)
        fprintf(fp,"%c",buffer[i]);
        
     fclose(fp);
     fprintf(stderr,"Product Extraction Complete to:  %s\n",outname);

   } /* end of extract block */
   
   /* use to create a hex dump of the product to disk */
   if(flag==HEXDUMP || flag==BOTH) {
     FILE *fp;
     char outname[128];
     int i,j;

     fprintf(stderr,"\nLB Hex Dump Utility Initiated\n");
     /* get the path for the products database */
     charval=getenv("HOME");
     
     if(charval==NULL) {
       fprintf(stderr,"ERROR: environmental variable for HOME is not set\n");
       if(buffer!=NULL) 
           free(buffer);
       return;
     }
     else {
       strcpy(outname,charval);
       strcat(outname,"/");
       if(output_file==NULL)
         strcat(outname,"graphic/hexdump.out");
        else
         strcat(outname,output_file);
     }
   
     fp=fopen(outname,"w");
     if(fp==NULL) {
         fprintf(stderr,"ERROR: Could not write to %s\n",outname);
         fprintf(stderr,"Hex Dump Output Aborted\n");
         if(buffer!=NULL) 
             free(buffer);
         return;
      }
      else {
         unsigned int byte_count=0;
         i=0;
         /*  ADD -nohdr option for HEXDUMP This works because 96 bytes */
         /*  is evenly divisible by bytes per line */
         if(nohdr==TRUE)
            i=96;
         else 
            i=0;

         fprintf(fp,"                 CODEview Text Hex Dump Utility\n\n");
         fprintf(fp,"           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
         fprintf(fp,"---------------------------------------------------------\n");

         /* after making adjustments to handle intermediate uncompressed */
         /*         products and final compressed products on LINUX, the value */
         /*         of msg_len does not include the 96 byte header,  */
         while(i<(msg_len+96)) {            
            fprintf(fp,"%08Xh: ",byte_count);
            byte_count+=16;
           
            for(j=0;j<16;j++) {
              fprintf(fp,"%02X ",(unsigned char)buffer[i++]);
              if(i>=(msg_len+96)) 
                 break;
            }
            
            fprintf(fp,"\n"); 
            
         } /* end of while */
         
        fclose(fp);
        fprintf(stderr,"Product output to %s\n",outname);      

     } /* end of output block */

   } /* end of hex dump block */

   if(buffer!=NULL) /*  CVT 4.3 */
       free(buffer);

} /* end Extract_LB_Product() */
