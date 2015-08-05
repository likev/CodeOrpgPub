/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 18:19:11 $
 * $Id: product_load.c,v 1.1 2009/05/15 18:19:11 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* product_load.c */

#include "product_load.h"



/* ==================================================================== */
char* Load_ORPG_Product(int msg_id, int force) {
   /* load the product identified by the message sequence number (msg_id) into
   a buffer. The buffer is allocated here and pointer returned. Return
   NULL if allocation failure. */
   char filename[128];

   int num_products, icd_format;
   int lbfd=-1;

   short param_8;
   
   LB_info list[MAX_PRODUCTS];
   int len, msg_len; 
 
   Prod_header *hdr;
   Graphic_product *gp, *phd;
   char* buffer=NULL;  /*  CVT 4.3 */

   fprintf(stderr,"\n*** ORPG DATABASE PRODUCT LOAD UTILITY ***\n");

   /* get the path for the products database */

   /* here we are using a product data base (determined in dispatcher.c) */
   strcpy(filename,cvt_prod_data_base);

   /* open the product linear buffer */
   lbfd=LB_open(filename,LB_READ,NULL);
   if(lbfd<0) {
      fprintf(stderr,"ERROR opening the following linear buffer: %s\n",filename);
      return(NULL);
   }

   /* retrieve information from the linear buffer */
   num_products=LB_list(lbfd,list,MAX_PRODUCTS);
   fprintf(stderr,"-> Number of Products Available=%d\n",num_products);
   fprintf(stderr,"-> LB Message Sequence Number=%d\n",msg_id);

   /* check for potential overflow with msg_id variable */
   if(msg_id<1 || msg_id> num_products) {
      fprintf(stderr,"ERROR: input message sequence number is out of bounds\n");
      LB_close(lbfd);
      return(NULL);
   }

   len=LB_read(lbfd,&buffer,LB_ALLOC_BUF,list[msg_id-1].id);

   if(buffer==NULL) {
        fprintf(stderr,
                "ERROR Loading Product - unable to read the buffer message\n");
        return NULL;
   }

   hdr=(Prod_header *) buffer;
   gp=(Graphic_product *) (buffer + 96);
   fprintf(stderr,"-> Product Info: LBuffer# %03hd MSGLEN %06d VOLNUM %02d ELEV %02d\n",
      hdr->g.prod_id, hdr->g.len, hdr->g.vol_num, get_elev_ind(buffer, orpg_build_i));

   LB_close(lbfd);
   /*fprintf(stderr,"buffer len=%d  list size=%d\n",len,list[msg_id].size);*/


   /* make sure we read in everything */
   if(len != list[msg_id-1].size) {
     if(buffer!=NULL) /*  CVT 4.3 */
         free(buffer);
     return NULL;
   }

   /* check to see if the body of the product is compressed */

   
    /* MUST FIRST CHECK FOR VALID ICD PRODUCT, compressed */
    /* intermediate products are NOT yet supported */

    icd_format=check_icd_format(buffer+96, FALSE);
  

    if( (icd_format == TRUE) || (force==TRUE) ) {
        /* if a product is empty (length == 120 for just MHB and PDB) do NOT */
        /* test for compression                                              */
        param_8=gp->param_8;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
        param_8=SHORT_BSWAP(param_8);
#endif
        
        if((len-96)>120) { /*  not an empty message */
           /* the following will work unless the first short of the */
           /* compressed symbology block is equal to -1             */
           /* first, pdp 8 should not be 0 and 
            * second, the section divider (-1) should not be present
            */

           if( ( (param_8 == 1) || (param_8 == 2) ) &&
               ( ((buffer[96+120]&0xff) != 0xff) ||
                 ((buffer[96+121]&0xff) != 0xff) ) ) {

               product_decompress(&buffer);
        
               phd = (Graphic_product *)(buffer+96);
               msg_len = phd->msg_len;
#ifdef LITTLE_ENDIAN_MACHINE  
               msg_len = INT_BSWAP(msg_len); 
#endif
               fprintf(stderr,"Uncompressed message length is %d (excluding "
                              "96 byte internal header)\n", msg_len);
           } /*  end if compressed */

        }  /*  end if not empty message */

    } else { /*  not an icd product */
        /*  FUTURE SUPPORT FOR COMPRESSED INTERMEDIATE PRODUCTS HERE  */
             ;
    } /*  end not an icd product */

   return buffer;

} /* end Load_ORPG_Product() */














char *load_product_file(char *prod_fname, int header_type, int force) {
  /* load the product from the single-product binary file */
  /* return NULL if file cannot be opened */
  

    int c, c1, c2, i;
/*  CVT 4.3 */
/*     char headbuf[122],  pre_icd[96]; */
    char headbuf[120],  pre_icd[96];
    char *out_buf=NULL;
/*    int sym_len, tab_len, gab_len, offset; not needed by new logic */
    int msg_len;
    short param_8;
    Graphic_product *gp; 
    Graphic_product *phd; 



    FILE *data_file;

    char filename[256];
    char work_dir[240];
    int icd_format;


    for (i=0;i<96;i++)
        pre_icd[i]=0;
    
    /* if a relative path is given, we prepend the current working dir */
    if (prod_fname[0]=='/') {
       sprintf(filename,"%s",prod_fname);
       fprintf(stderr," filename from absolute path is: %s\n",filename);
       }
    else {
       if(getcwd(work_dir, 240-1) == NULL) {
          fprintf(stderr,"ERROR - path to filename is too long");
          return NULL;
         }
         sprintf(filename, "%s/%s", work_dir,prod_fname);
         fprintf(stderr," filename from relative path is %s\n:",filename);
    }
    
    data_file = fopen(filename,"rb"); /* do I want "r" or "rb" ? */
    if (data_file==NULL) {
        fprintf(stderr, "ERROR opening product file %s\n",filename);
        return(out_buf);
    }
     
  /******************************************************************/

    /* sometimes you need to skip a header in WMO format */
    /* we look for the second occurrence of the string "\n\n\l" */
    if(header_type == HEADER_WMO) {
        while((c = fgetc(data_file))) {
            if(c == EOF) {
                fclose(data_file); /*  cvt 4.3 */
                return NULL;
            }
    
            if(c == 0x0d) {
                c1 = fgetc(data_file);
                c2 = fgetc(data_file);
                if( (c1 == 0x0d) && (c2 == 0x0a) ) {
                    break;
                }
            }
        } /*  end while */

        /* "...get along, try again." */
        while((c = fgetc(data_file))) {
    
            if(c == EOF) {
                fclose(data_file); /*  cvt 4.3 */
                return NULL;
            }
    
            if(c == 0x0d) {
                c1 = fgetc(data_file);
                c2 = fgetc(data_file);
                if( (c1 == 0x0d) && (c2 == 0x0a) ) {
                    break;
                }
            }
        }
    } else if(header_type == HEADER_PRE_ICD) {
        /* scarf up a pre-icd header, if it has one */
        fread(pre_icd, 1, 96, data_file);
    if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
        fprintf(stderr, "Error reading pre_ICD header in load_product_file\n");
        fclose(data_file); /*  cvt 4.3 */
        return NULL;
    }
    }
     
        fprintf(stderr,"loading product headers...\n");

         /* first, load the product headers */
         /* reading the MHB, PDB plus the 2-byte SYMB divider */
/*  CVT 4.3 */
/*     fread(headbuf, 1, 122, data_file); */
    fread(headbuf, 1, 120, data_file);

    if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
        fprintf(stderr, "Error reading MHB & PDB in load_product_file\n");
        fclose(data_file); /*  cvt 4.3 */
        return NULL;
    }

/* === CVT 4.3 moved initial loading of the product before decompression ===== */
 
    /* message length used here and in decompress (assumes ICD product) */
    gp = (Graphic_product*)(headbuf);
    msg_len = gp->msg_len;
#ifdef LITTLE_ENDIAN_MACHINE
    msg_len = INT_BSWAP(msg_len);
#endif

    if((out_buf = malloc(msg_len+96)) == NULL) {
        fclose(data_file); /*  cvt 4.3 */
        return NULL;
    }
    /* if we've already read in the Pre-ICD header, add it onto the product. */
    if(header_type == HEADER_PRE_ICD)
        memcpy(out_buf, pre_icd, 96);
    else
        bzero(out_buf, 96);       /* otherwise, make sure it's full of zeros */
 
   /* headbuff already contains the first 120 bytes of the product */
   /* need to read msg_len-120 additional bytes */ 
   /* put it all together */
        
      /*  FUTURE ENHANCEMENT: need to modify to not read beyond the filesize */
/*  CVT 4.3 */
/*     memcpy(out_buf+96, headbuf, 122); */
/*     fread(out_buf+218, 1, msg_len-122, data_file); */
    memcpy(out_buf+96, headbuf, 120);   
    fread(out_buf+216, 1, msg_len-120, data_file);

    if(ferror(data_file) != 0) {
        fprintf(stderr, "Error reading product data in load_product_file\n");
        fclose(data_file); /*  cvt 4.3 */
        if(out_buf!=NULL)
            free(out_buf);
        return NULL;
    }
/* ===== end add initial loading of the product =============================== */

    icd_format=check_icd_format((char *)headbuf, TRUE);

    /* check to see if the body of the product is compressed */  
    /* the following will work as long as the first short of the */
    /* compressed symbology block is NOT equal to -1             */
    /* first, pdp 8 should not be 0 and 
     * second, the section divider (-1) should not be present
    */

/* future support of intermediate products would require a pre-ICD header
   to get the message length, or rely on the correct file size*/
    
    if( (icd_format == TRUE) || (force==TRUE) ) { /*  only format currently supported */
        
        /* read some required fields */
/*         gp = (Graphic_product*)(headbuf); */
/*         msg_len = gp->msg_len; */

        param_8=gp->param_8;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
/*         msg_len = INT_BSWAP(msg_len); */
        param_8=SHORT_BSWAP(param_8);
#endif
    
/*  CVT 4.3  */
        /* if a product is empty (length == 120 for just MHB and PDB) do NOT */
        /* test for compression                                              */
        if(msg_len>120) { /*  test for compression, otherwize assume uncompressed */

/*  CVT 4.3 // a bug causing a product to be decompressed just because param_8 not 0 */
/*             if( (param_8 != 0) && ((headbuf[120]&0xff) != 0xff) && */
/*                                   ((headbuf[121]&0xff) != 0xff) ) { */
            if( ( (param_8 == 1) || (param_8 == 2) ) &&
                ( ((out_buf[96+120]&0xff) != 0xff) ||
                  ((out_buf[96+121]&0xff) != 0xff) ) ) {
                product_decompress(&out_buf);
                phd = (Graphic_product *)(out_buf+96); 
    /* BUGFIX */
                msg_len = phd->msg_len;
    #ifdef LITTLE_ENDIAN_MACHINE
                msg_len = INT_BSWAP(msg_len);
    #endif 
                fprintf(stderr,"Uncompressed message length is %d "
                               "(excluding 96 byte internal header)\n", msg_len);
                fclose(data_file);
                return out_buf;
            } /*  end product is compressed */
        
        }  /*  end not an empty product */
    
    } else { /*  not an icd product */
        /*  FUTURE SUPPORT FOR INTERMEDIATE PRODUCTS HERE  */
             ; 
    } /*  end not an icd product  */

    fclose(data_file);
    return out_buf;

} /* end load_product_file() */







