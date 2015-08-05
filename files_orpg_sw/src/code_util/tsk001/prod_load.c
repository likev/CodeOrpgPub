/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:54 $
 * $Id: prod_load.c,v 1.5 2009/05/15 17:52:54 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* prod_laod.c */

#include "prod_load.h"


static int accept_icd_error = FALSE;


/* NOTE - The product is not byte-swapped during the loading process.  */
/*        Necessary data fields are read into local variables which    */
/*        are swapped as required.  The contents of the product itself */
/*        are swapped after loading and consistency check in the       */
/*        function parse_packet_numbers() in symbology_block.c         */


/* load the product identified by the message sequence number (msg_id) from the 
 * given LB into a buffer. The buffer is allocated here and pointer returned. 
 * Return NULL if allocation failure 
 */
char* Load_ORPG_Product(int msg_id, char *lb_filename) 
{
   int num_products, len;
   int lbfd = -1;
   int listSize = maxProducts+1;
   LB_info list[listSize]; 
   Prod_header *hdr;
   char* buffer;
 
   short divider, elev_ind, vol_num;
   Graphic_product *gp; 

    int msg_len;
    short param_8;
    
    Widget d_icd, accept_box;
    XmString xmstr; 
    short test_msg_code;
 
   if(verbose_flag) 
       printf("\n      ORPG Database Product Load Utility\n");
   
   /* open the linear buffer we want */
   lbfd = LB_open(lb_filename, LB_READ, NULL);
   if(lbfd < 0) {
       fprintf(stderr,"ERROR READING MESSAGE FROM DATABASE: "
                      "Could not open the following linear buffer: %s\n", 
                                                               lb_filename);
       return(NULL);
   }
   
   /* get the linear buffer's inventory */
   num_products = LB_list(lbfd, list, maxProducts+1);
   if(verbose_flag) 
       printf("num_products available=%i msg_id=%d\n", num_products, msg_id);
   
   /* the following is not necessarily valid if database has been purged */
   /* check for potential overflow with msg_id variable */
   if(msg_id<1 || msg_id>num_products) {
       fprintf(stderr,"ERROR READING MESSAGE FROM DATABASE: "
                      "input message sequence number is out of bounds\n");
       return(NULL);
   }
   
   /* read in the product we want - allocates the memory for us */
   len = LB_read(lbfd, &buffer, LB_ALLOC_BUF, list[msg_id-1].id);

   if(buffer==NULL) {
       fprintf(stderr,"ERROR Load_ORPG_Product - unable to read buffer message\n");
       return(NULL);
   }

   if(verbose_flag) {
       hdr = (Prod_header *)buffer;
       printf("Product Info: LBuffer# %03hd MSGLEN %06d VOLNUM %02d ELEV %02d\n",
/*        hdr->g.prod_id,hdr->g.len,hdr->g.vol_num,hdr->elev_ind); */
          hdr->g.prod_id, hdr->g.len, hdr->g.vol_num, 
          get_elev_ind(buffer,orpg_build_i));
   }

   /* clean up */
   LB_close(lbfd);
   
   /* make sure we read in everything */
   if(len != list[msg_id-1].size) {
     free(buffer);
     fprintf(stderr,
             "ERROR READING MESSAGE FROM DATABASE: message length incorrect\n");
     return NULL;
   }

    gp = (Graphic_product*)(buffer+96);
    test_msg_code = gp->msg_code;
    divider = gp->divider;
    elev_ind = gp->elev_ind;
    vol_num = gp->vol_num;

    msg_len = gp->msg_len;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    test_msg_code = SHORT_BSWAP(test_msg_code);
    divider = SHORT_BSWAP(divider);
    elev_ind = SHORT_BSWAP(elev_ind);
    vol_num = SHORT_BSWAP(vol_num);

    msg_len = INT_BSWAP(msg_len);
#endif


/*  CHECK FOR ICD FORMAT HERE */
    /* if intermediate products from the database are supported in the future
     * an alternate test and decompress must be provided.
     */
/* TEST CODE */
/* if(test_msg_code==1992) */
/*     elev_ind=50; */

    if(test_for_icd(divider, elev_ind, vol_num, FALSE)==FALSE)  { 
        fprintf(stderr, "LOADING PRODUCT - Buffer Message not an ICD Product\n");

        if(accept_icd_error == FALSE) {
            
            test_for_icd(divider, elev_ind, vol_num, TRUE);
            
            xmstr = XmStringCreateLtoR(
             "The test for valid ICD final product failed. The product selected\n"
             "may be an intermediate product which are not yet supported by    \n"
             "CVG.  Or there may be an error in this product.                  \n\n"
             "See terminal output for details.                                 \n\n"
             "Attempting to display this product may cause CVG to crash.  If   \n"
             "you wish to attempt display of this product check the box below. \n"
             "and reselect the product for display.                            \n",
                    XmFONTLIST_DEFAULT_TAG);
    
            d_icd = XmCreateErrorDialog(shell, "icd_test", NULL, 0);
            XtVaSetValues(XtParent(d_icd), XmNtitle, "CAUTION", NULL);
            XtVaSetValues(d_icd, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d_icd, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d_icd, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d_icd);
    
            accept_box = XtVaCreateManagedWidget("Accept this error.            "
                                            "                                   "
                                         "                                  \n\n"
                                         "Then reattempt to display the product.", 
                 xmToggleButtonWidgetClass,  d_icd,
                 XmNset,                     XmUNSET,
                 NULL);
            XtAddCallback(accept_box, XmNvalueChangedCallback, 
                                                       accept_icd_error_cb, NULL);
            
            XmStringFree(xmstr);
            
        } /*  end if accept_icd is FALSE */

        if(accept_icd_error == FALSE) {
            return NULL;
            
        } else {
            accept_icd_error = FALSE;
        }/*  end if accept_icd is FALSE         */
        
        
    } /*  end if icd test fails */




   /* check to see if the body of the product is compressed */
   
    /* Since we only support Display of Final Products (or pseudo  */
    /* final product created from CVG Raw structure) from the      */
    /* product database, there is no need to confirm valid ICD     */
    /* structure before testing for compression.  If we support    */
    /* intermediate products or other new formats in the future    */
    /* then some test will have to be created to avoid             */
    /* accidentially  decompressing something else.                */
    

    /* if a product is empty (length == 120 for just MHB and PDB) do NOT */
    /* test for compression                                              */
    if(msg_len>120) { /*  test for compression, otherwize assume uncompressed */

        param_8=gp->param_8;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
        param_8=SHORT_BSWAP(param_8);
#endif
       /* the following will work unless the first short of the */
       /* compressed symbology block is equal to -1             */
       /* first, pdp 8 should not be 0 and 
        * second, the section divider (-1) should not be present
        */

        if( ( (param_8 == 1) || (param_8 == 2) ) &&
            ( ((buffer[96+120]&0xff) != 0xff) ||
              ((buffer[96+121]&0xff) != 0xff) ) ) 

           product_decompress(&buffer);

    }  /*  end if not an empty product */

   return buffer;
   
   
}  /*  end Load_ORPG_Product */





/* decompresses a compressed product
 * assumes that the headers are not compressed, while the data is
 */
void product_decompress(char **buffer)
{
   Graphic_product *phd = malloc(120);/* 1st, holds copy of MHB & PDB */
                                      /* 2nd, access resulting product */
   char *src_buf = *buffer+96+sizeof(Graphic_product);/*compressed part*/
   char *dest_buf = NULL;  /* resulting uncompressed product */
   unsigned int src_len, dest_len, ret;

   if(verbose_flag)
        fprintf(stderr, "Decompressing Product \n");

    memcpy(phd,*buffer+96,120);


/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_short_swap(phd,60);
    phd->msg_time=INT_SSWAP(phd->msg_time);
    phd->msg_len=INT_SSWAP(phd->msg_len);
    phd->latitude=INT_SSWAP(phd->latitude);
    phd->longitude=INT_SSWAP(phd->longitude);
    phd->gen_time=INT_SSWAP(phd->gen_time);
    phd->sym_off=INT_SSWAP(phd->sym_off);
    phd->gra_off=INT_SSWAP(phd->gra_off);
    phd->tab_off=INT_SSWAP(phd->tab_off);
#endif

    src_len = (unsigned int)(phd->msg_len - sizeof(Graphic_product));
    
    dest_len = ((unsigned int)(phd->param_9 & 0xffff) << 16) +
            (unsigned int)(phd->param_10 & 0xffff);


    if((dest_buf = malloc(dest_len+96+sizeof(Graphic_product))) == NULL) {
        free(*buffer);
        buffer = NULL;
        free(phd);
        return;
    }

    /* do the decompression */
    switch((unsigned short)(phd->param_8)) {
    case COMPRESSION_BZIP2:
        default:
        ret = BZ2_bzBuffToBuffDecompress(dest_buf+96+sizeof(Graphic_product),
                      &dest_len, src_buf, src_len,
                      BZIP2_NOT_SMALL, BZIP2_NOT_VERBOSE);
        if(ret != BZ_OK) {
            free(*buffer);
            free(dest_buf);
            buffer = NULL;
            free(phd);
            return;
        }
    
    } /*  end switch */

    /* put everything back together again */
    memcpy(dest_buf, *buffer, sizeof(Graphic_product)+96);
    free(phd);
    phd = (Graphic_product *)(dest_buf+96);
    phd->msg_len = dest_len + sizeof(Graphic_product);

#ifdef LITTLE_ENDIAN_MACHINE
phd->msg_len=INT_BSWAP(phd->msg_len);
#endif

    phd->param_8 = (unsigned short) COMPRESSION_NONE;
    phd->param_9 = 0;
    phd->param_10 = 0;
     
#ifdef LITTLE_ENDIAN_MACHINE
    phd->param_8=SHORT_BSWAP(phd->param_8);
    phd->param_9=SHORT_BSWAP(phd->param_9);
    phd->param_10=SHORT_BSWAP(phd->param_10);
#endif 
      
    free(*buffer);
    *buffer = dest_buf;

} /*  end decompress_product() */





/* if detection mode for the header type is auto, detects the type
 * of header in the file (none, WMO, or internal 96 byte).
 * allocates a block of memory and loads a product from the 
 * specified file into it
 */
char *load_icd_product_disk(FILE *data_file, int filesize)
{
    int c, c1, c2;

    char headbuf[120], pre_icd[96];
    char *out_buf=NULL;
    int msg_len;
    short param_8;
    short divider, elev_ind, vol_num;
    Graphic_product *gp;

    int wmo_skip_error = FALSE;
    int num_char_read=0;

    Widget d;
    XmString xmstr;

    Widget d_icd, accept_box;
    short test_msg_code;

    Boolean is_set;


    /* auto detect not selected, use position of radio box */
    if(header_type != AUTO_DETECT) {
        
        XtVaGetValues(diskfile_icdwmo_but, XmNset, &is_set, NULL);
            if(is_set == True) 
                header_type = HEADER_WMO;
                
        XtVaGetValues(diskfile_preicdheaders_but, XmNset, &is_set, NULL);
            if(is_set == True) 
                header_type = HEADER_PRE_ICD;
                
        XtVaGetValues(diskfile_icd_but, XmNset, &is_set, NULL);
            if(is_set == True)
                header_type = HEADER_NONE;
        
    } /*  end if not AUTO_DETECT */

    

/* TEST */
/* header_type = AUTO_DETECT; */

    /* LOGIC TO AUTO DETECT THE HEADER TYPE //////////////////// */
    /* This will work most of the time but...                  */
    /* If detection fails the user is prompted to specify type */

    if(header_type == AUTO_DETECT) {
        /* attempt to detect an ICD product at the beginning of the file */
        /* reading the MHB, PDB, plus the 2 byte SYMB divider */

        fread(headbuf, 1, 120, data_file);
        
        if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
            fprintf(stderr,"Error attempting to read the first "
                           "120 bytes of the selected file.\n");
            return NULL;
        }
    
        gp=(Graphic_product*)(headbuf);
        /* is this a valid ICD product? */
        divider = gp->divider;
        elev_ind = gp->elev_ind;
        vol_num = gp->vol_num;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
        divider = SHORT_BSWAP(divider);
        elev_ind = SHORT_BSWAP(elev_ind);
        vol_num = SHORT_BSWAP(vol_num);
#endif
        if(test_for_icd(divider, elev_ind, vol_num, FALSE)==TRUE) {
            header_type = HEADER_NONE;
            fprintf(stderr,"Detected ICD product with no header in disk file\n.");
        }
        fseek(data_file, 0, SEEK_SET);

    }   /*  end if AUTO_DETECT find at beginning  */

    /* if unsuccessful, attempt to detect an ICD product after 96 bytes */
    if(header_type == AUTO_DETECT) {
        fseek(data_file, 96, SEEK_SET);

        fread(headbuf, 1, 120, data_file);
        
        if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
            fprintf(stderr,"Error attempting to read the first "
                           "216 bytes of the selected file.\n");
            return NULL;
        }
    
        gp=(Graphic_product*)(headbuf);

        /* is this a valid ICD product? */
        divider = gp->divider;
        elev_ind = gp->elev_ind;
        vol_num = gp->vol_num;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
        divider = SHORT_BSWAP(divider);
        elev_ind = SHORT_BSWAP(elev_ind);
        vol_num = SHORT_BSWAP(vol_num);
#endif

        if(test_for_icd(divider, elev_ind, vol_num, FALSE)==TRUE) {
            header_type = HEADER_PRE_ICD;
            fprintf(stderr,"Detected ICD product with 96 byte internal header "
                           "in disk file\n.");
        }
        fseek(data_file, 0, SEEK_SET);
        
    }   /*  end if AUTO_DETECT  find after PRE ICD */

    /* if unsuccessful, attempt to detect a WMO header */
    /* we look for the second occurance of the string "\n\n\l" */
    if(header_type == AUTO_DETECT) {
        while((c = fgetc(data_file))) {
            num_char_read++;
            
            if(c == EOF) {
                wmo_skip_error = TRUE;
                break;
            }
            if(c == 0x0d) {
                c1 = fgetc(data_file);
                num_char_read++;
                c2 = fgetc(data_file);
                num_char_read++;
                if( (c1 == 0x0d) && (c2 == 0x0a) ) {
                    break;
                }
            }
        } /*  end while */

        /* "...get along, try again." */
        if(wmo_skip_error == FALSE) {
            while((c = fgetc(data_file))) {
                num_char_read++;
        
                if(c == EOF) {
                    wmo_skip_error = TRUE;
                    break;
                }
                if(c == 0x0d) {
                    c1 = fgetc(data_file);
                    num_char_read++;
                    c2 = fgetc(data_file);
                    num_char_read++;
                    if( (c1 == 0x0d) && (c2 == 0x0a) ) {
                        break;
                    }
                }
            } /*  end while */
        } /*  end if no skip error         */
        
        if(wmo_skip_error == FALSE) {

            fread(headbuf, 1, 120, data_file);
            
            if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
                
                return NULL;
            }
        
            gp=(Graphic_product*)(headbuf);
            /* is this a valid ICD product? */
            divider = gp->divider;
            elev_ind = gp->elev_ind;
            vol_num = gp->vol_num;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
            divider = SHORT_BSWAP(divider);
            elev_ind = SHORT_BSWAP(elev_ind);
            vol_num = SHORT_BSWAP(vol_num);
#endif
            if(test_for_icd(divider, elev_ind, vol_num, FALSE)==TRUE) {
                header_type = HEADER_WMO;
                fprintf(stderr,
                        "Detected ICD product with WMO header in disk file\n.");
            }         
            
        } /*  end if no skip error */
        fseek(data_file, 0, SEEK_SET); 
        
    }   /*  end if AUTO_DETECT  find after WMO */

/* TEST */
/* header_type = AUTO_DETECT; */
    
    /* if unsuccessful prompt the user to specify a header type */
    if(header_type == AUTO_DETECT) {
        
        /* ADD PROMPT TO THE USER TO SPECIFY HEADER TYPE MANUALLY */
        fprintf(stderr,
                "Error - Unable to automatically detect the header type.\n");
        
        d = XmCreateErrorDialog(prod_disk_sel, "Header Detection Error", NULL, 0);
        xmstr = XmStringCreateLtoR(
                                "The presence and type of header in the selected\n"
                                "product file could not be determined.\n\n"
                                "Please specify the header type in the file"
                                "selection dialog.",
                               XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(XtParent(d), XmNtitle, "File Error", NULL);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        
        return NULL;
    }   /*  end if AUTO_DETECT   */
    
/*  END AUTO DETECT LOGIC ///////////////////////////////////////////////// */

    
    
    /* NOTE: DOES A WMO HEADER ALWAYS CONTAIN 30 BYTES? */
    
    num_char_read=0;
    
    if(header_type == HEADER_WMO) {
        /* we look for the second occurance of the string "\n\n\l" */
        while((c = fgetc(data_file))) {
            
            num_char_read++;
            
            if(c == EOF) {
                
                return NULL;
            }
    
            if(c == 0x0d) {
                c1 = fgetc(data_file);
                num_char_read++;
                c2 = fgetc(data_file);
                num_char_read++;
                if( (c1 == 0x0d) && (c2 == 0x0a) ) {
                    break;
                }
            }
        } /*  end while */

        /* "...get along, try again." */
        while((c = fgetc(data_file))) {
            
            num_char_read++;
    
            if(c == EOF) {
                
                return NULL; 
            }
    
            if(c == 0x0d) {
                c1 = fgetc(data_file);
                num_char_read++;
                c2 = fgetc(data_file);
                num_char_read++;
                if( (c1 == 0x0d) && (c2 == 0x0a) ) {
                    break;
                }
            }
        } /*  end while */
        
    } else if(header_type == HEADER_PRE_ICD) {
        /* read a pre-icd header, if it has one */
        
        fread(pre_icd, 1, 96, data_file);
        
        num_char_read = 96;
        
        if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
            fprintf(stderr,"Error attempting to read the first "
                           "96 bytes of the selected file.\n");
            return NULL;
        }
    }
    
    if(verbose_flag) 
        printf("loading product headers...\n");


    /* first, load the product headers */
    /* reading the MHB, PDB, */

    fread(headbuf, 1, 120, data_file);
    
    if((ferror(data_file) != 0) || (feof(data_file) != 0)) {
        if(header_type == HEADER_NONE)
            fprintf(stderr,"Error attempting to read the first "
                           "120 bytes of the selected file.\n");
        if(header_type == HEADER_PRE_ICD)
            fprintf(stderr,"Error attempting to read the first "
                           "216 bytes of the selected file.\n");
        if(header_type == HEADER_WMO)
            fprintf(stderr,"Error attempting to read the first "
                           "%d bytes of the selected file.\n",
                                               120+num_char_read);
        return NULL;
    }

   /* CHECK FOR ICD FORMAT HERE 
    * ABORT WITH MESSAGE IF NOT 
    */

    /* easy access to some of the data fields */
    gp = (Graphic_product*)(headbuf);

    test_msg_code = gp->msg_code;
    divider = gp->divider;
    elev_ind = gp->elev_ind;
    vol_num = gp->vol_num;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    test_msg_code = SHORT_BSWAP(test_msg_code);
    divider = SHORT_BSWAP(divider);
    elev_ind = SHORT_BSWAP(elev_ind);
    vol_num = SHORT_BSWAP(vol_num);
#endif

/* TEST CODE */
/* if(test_msg_code==19) */
/*     elev_ind=50; */
    
    if(test_for_icd(divider, elev_ind, vol_num, FALSE)==FALSE)  { 
        
        fprintf(stderr, "LOADING PRODUCT - Binary File Not an ICD Product\n");

        if(accept_icd_error == FALSE) {
            
            test_for_icd(divider, elev_ind, vol_num, TRUE);
            
            xmstr = XmStringCreateLtoR(
             "The test for valid ICD final product failed. The product selected\n"
             "may be an intermediate product which are not yet supported by    \n"
             "CVG.  Or there may be an error in this product.                  \n\n"
             "See terminal output for details.                                 \n\n"
             "Attempting to display this product may cause CVG to crash.  If   \n"
             "you wish to attempt display of this product check the box below. \n"
             "and reselect the product for display.                            \n",
                 XmFONTLIST_DEFAULT_TAG);
    
            d_icd = XmCreateErrorDialog(prod_disk_sel, "icd_test", NULL, 0);
            XtVaSetValues(XtParent(d_icd), XmNtitle, "CAUTION", NULL);
            XtVaSetValues(d_icd, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d_icd, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d_icd, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d_icd);
    
            accept_box = XtVaCreateManagedWidget("Accept this error.            "
                                            "                                   "
                                         "                                  \n\n"
                                         "Then reattempt to display the product.", 
                 xmToggleButtonWidgetClass,  d_icd,
                 XmNset,                     XmUNSET,
                 NULL);
            XtAddCallback(accept_box, XmNvalueChangedCallback, 
                                                        accept_icd_error_cb, NULL);
            
            XmStringFree(xmstr);
            
        } /*  end if accept_icd is FALSE */

        if(accept_icd_error == FALSE) {
            return NULL;
            
        } else {
            accept_icd_error = FALSE;
        }/*  end if accept_icd is FALSE         */
        
        
    } /*  end if icd test fails */




    msg_len = gp->msg_len;
    param_8=gp->param_8;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    msg_len = INT_BSWAP(msg_len);
    param_8=SHORT_BSWAP(param_8);
#endif    

    /* HERE IS WHERE WE COMPARE THE filesize TO THE msg_len
     * ABORT WITH MESSAGE IF THE FILE IS TOO SMALL
     */
/*  DEBUG */
/* fprintf(stderr,"DEBUG load_icd_disk - file size is %d\n", filesize);  */
/* fprintf(stderr,"DEBUG load_icd_disk - product message length is %d\n",  */
/*              msg_len); */
/* fprintf(stderr,"DEBUG load_icd_disk - non-product header size is %d\n",  */
/*              num_char_read); */
/* fprintf(stderr,"DEBUG load_icd_disk - have read %d bytes at this point\n", */
/*              num_char_read + 120); */

    /*  this test is conservative, it requires file size to correspond  */
    /*  to message length */
    /*  if test for valid ICD product was accomplished before, the test */
    /*  could be relaxed to message length not too big for file size */
    if(msg_len != (filesize - num_char_read) ) {

        fprintf(stderr,"ERROR LOADING PRODUCT DISK FILE - product msg_length %d\n",
                                       msg_len);
        fprintf(stderr,
                    "does not match (filesise %d - non-product header size %d)\n",
                                       filesize, num_char_read);
        return NULL;
    }


    if((out_buf = malloc(msg_len+96)) == NULL) {
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

    /*  TO DO: need to modify to not read beyond the filesize */
    memcpy(out_buf+96, headbuf, 120);
    fread(out_buf+216, 1, msg_len-120, data_file);
    
    if(ferror(data_file) != 0) {
        fprintf(stderr, "Error reading product data in load_product_file\n");
        return NULL;
    }

/* ///// end add initial loading of the product //////////////////////////////// */
    
    
/*  DEBUG */
/* fprintf(stderr, */
/* "DEBUG load_icd_disk param_8 %d, outbuf[120+96] is %d, outbuf[121+96] is %d\n", */
/*      param_8, out_buf[120+96], out_buf[121+96]); */

    /* if a product is empty (length == 120 for just MHB and PDB) do NOT */
    /* test for compression                                              */
    if(msg_len>120) { /*  test for compression, otherwize assume uncompressed */
        
        /* the following will work unless the first short of the */
        /* compressed symbology block is equal to -1             */
        /* first, pdp 8 should not be 0 and 
         * second, the section divider (-1) should not be present
        */

        if( ( (param_8 == 1) || (param_8 == 2) ) &&
            ( ((out_buf[96+120]&0xff) != 0xff) ||
              ((out_buf[96+121]&0xff) != 0xff) ) ) {

            product_decompress(&out_buf);
            return out_buf;
        
        }  /*  end product is compressed */

    }  /*  end not an empty product */


    return out_buf;
    
} /*  end load_icd_product_disk() */





  /*
   * NOTE, THIS FUNCTION WAS NEVER MODIFIED FOR LINUX (Little Endian Environment)
   *       AND was not modified for compression
   */
/* loads information stored in CVG Raw Format into a icd-ish product */
char *load_cvg_raw_disk(FILE *data_file, int filesize)
{
    int pid, data_type, packet_size, i, j, offset, msg_len, flag;
    int num_range_bins, num_radials, num_bytes_per_bin, num_bytes, num_rows;
    int *start_angles, *angle_deltas, num_columns;
    char *out_buf, *packet;
    short *halfword_packet;
    unsigned char bin;


    /* HERE IS WHERE WE SHOULD COMPARE THE filesize TO THE msg_len
     * ABORT WITH MESSAGE IF THE FILE IS TOO SMALL
     */


    /* first read in the type of data we're dealing with */
    fread(&data_type, 4, 1, data_file);

    /* and the product ID */
    fread(&pid, 4, 1, data_file);

    /* and an unused flag variable */
    fread(&flag, 4, 1, data_file);


    if(verbose_flag) {
        printf("CRF header data\n");
        printf("data type = %d  product ID = %d\n", data_type, pid);
        printf("flag = 0x%08x\n", flag);
    }

    if(data_type == CVG_RAW_DATA_RADIAL) {
        /* for raw radial data, we put it in a digital radial data array packet */
        /* read in the rest of the header data */
        fread(&num_range_bins, 4, 1, data_file);
        fread(&num_radials, 4, 1, data_file);
        fread(&num_bytes_per_bin, 4, 1, data_file);

        if(verbose_flag) {
            printf("number of range bins = %d\n", num_range_bins);
            printf("number of radials = %d\n", num_radials);
            printf("number of bytes per bin = %d\n", num_bytes_per_bin);
        }
    
        /* now, read in the angle data */
        start_angles = malloc(4*num_radials);
        angle_deltas = malloc(4*num_radials);
        fread(start_angles, 4, num_radials, data_file);
        fread(angle_deltas, 4, num_radials, data_file);
    
    /* DEBUG */
        /*
        printf("\nangles:\n");
        for(i=0; i<num_radials; i++)
            printf("%8x", start_angles[i]);
        printf("\nangle deltas:\n");
        for(i=0; i<num_radials; i++)
            printf("%8x", angle_deltas[i]);
        printf("\n\n");
        */
    
        /* now that only the bin data is left, we can put together a packet */
        /* first, figure out how big it should be */
        packet_size = 14 + (6 + num_range_bins) * num_radials;
        num_bytes = num_range_bins;
        packet = malloc(packet_size);
    
        /* store the header info */
        halfword_packet = (short *)packet;
        bzero(packet, 14);        /* any unset fields should be zero */
        halfword_packet[0] = 16;  /* packet code */
        halfword_packet[2] = num_range_bins;
        halfword_packet[6] = num_radials;
    
        /* now, we read in each radial */
        for(i=0, offset=14; i<num_radials; i++) {
            /* radial header junk */
            packet[offset++] = (num_bytes >> 8) & 0xff;
            packet[offset++] = num_bytes & 0xff;
            packet[offset++] = (start_angles[i] >> 8) & 0xff;
            packet[offset++] = start_angles[i] & 0xff;
            packet[offset++] = (angle_deltas[i] >> 8) & 0xff;
            packet[offset++] = angle_deltas[i] & 0xff;
            /*
            if(verbose_flag)
                printf("number of bytes in packet = %d  start angle = %d "
                       "  delta angle = %d\n",
                   num_bytes, start_angles[i], angle_deltas[i]);
                   */
            /* we only display up to 256 levels, so we only store the low-order
             * byte of the data
             */
            /*printf("\nradial %d:\n", i);*/
            for(j=0; j<num_range_bins; j++) {
                fread(&bin, num_bytes_per_bin, 1, data_file);
            packet[offset++] = bin;
            /*printf("%4hx", packet[offset-1]);*/
            }
            /*
            if(verbose_flag)
                printf("end offset = %d\n", offset);
            */
        }
    
        /* now, put together the whole packet */
        msg_len = 216+16+packet_size;
        out_buf = malloc(msg_len);
        bzero(out_buf, msg_len);        /* zero out the buffer */
        /* CVG assumes there is no Pre-ICD header in CVG Raw disk files */
        halfword_packet = (short *)out_buf;
        halfword_packet[0] = pid;
        halfword_packet[48+4] = (msg_len >> 16) & 0xffff;
        halfword_packet[48+5] = msg_len & 0xffff;
        halfword_packet[48+54] = 0;
        halfword_packet[48+55] = 60;    /* symbology offset */
    
        halfword_packet[48+9] = halfword_packet[108] = halfword_packet[108+5] = -1;
        halfword_packet[108+1] = 1;
        halfword_packet[108+2] = ((msg_len-216) >> 16) & 0xffff;
        halfword_packet[108+3] = (msg_len-216) & 0xffff;
        halfword_packet[108+4] = 1;
        halfword_packet[108+6] = ((msg_len-216-16) >> 16) & 0xffff;
        halfword_packet[108+7] = (msg_len-216-16) & 0xffff;
        memcpy(out_buf+216+16, packet, packet_size);
    
        /* finally, free up all the memory we don't need */
        free(packet);
        free(start_angles);
        free(angle_deltas);
        
    } else if(data_type == CVG_RAW_DATA_RASTER) {          
        /* and for raw raster data, we put it in a packet of our own design
     * essentially a non-RLE radial packet 
     */                
        /* read in the rest of the header data */
        fread(&num_rows, 4, 1, data_file);
        fread(&num_columns, 4, 1, data_file);
        fread(&num_bytes_per_bin, 4, 1, data_file);
                       
        /* now that only the bin data is left, we can put together a packet */
        /* first, figure out how big it should be */
        packet_size = 14 + num_rows * num_columns;
        num_bytes = 2 + num_columns;
        packet = malloc(packet_size);
                           
        /* store the header info */
        halfword_packet = (short *)packet;
        bzero(packet, 14);        /* any unset fields should be zero */
        halfword_packet[0] = 35;  /* packet code */
        halfword_packet[1] = num_rows;
                           
        /* now, we read in each radial */
        for(i=0, offset=4; i<num_rows; i++) {
            /* radial header junk */
            packet[offset++] = (num_bytes >> 8) & 0xff;
            packet[offset++] = num_bytes & 0xff;
                           
            /* we only display up to 256 levels, so we only store the low-order
             * byte of the data
             */            
            for(j=0; j<num_columns; j++) {
                fread(&bin, num_bytes_per_bin, 1, data_file);
                packet[offset++] = bin;
            }              
        }                  
                           
        /* now, put together the whole packet */
        msg_len = 216+16+packet_size;
        out_buf = malloc(msg_len);
        /* CVG assumes there is no Pre-ICD header in CVG Raw disk files */
        halfword_packet = (short *)out_buf;
        halfword_packet[0] = pid;
        halfword_packet[48+4] = (msg_len >> 16) & 0xffff;
        halfword_packet[48+5] = msg_len & 0xffff;
        halfword_packet[48+54] = 0;
        halfword_packet[48+55] = 60;    /* symbology offset */
                           
        halfword_packet[48+9] = halfword_packet[108] = halfword_packet[108+5] = -1;
        halfword_packet[108+1] = 1;
        halfword_packet[108+2] = ((msg_len-216) >> 16) & 0xffff;
        halfword_packet[108+3] = (msg_len-216) & 0xffff;
        halfword_packet[108+4] = 1;
        halfword_packet[108+6] = ((msg_len-216-14) >> 16) & 0xffff;
        halfword_packet[108+7] = (msg_len-216-14) & 0xffff;
        memcpy(out_buf+216+16, packet, packet_size);
                           
        /* finally, free up all the memory we don't need */
        free(packet);
    
    } else {           
        out_buf = NULL;
        printf("invalid data format\n");
    }                  
                       
    fclose(data_file); 
                       
    return out_buf;    
    
}  /*  end load_cvg_raw_disk() */
   
  
   
  /* 
   * NOTE, THIS FUNCTION WAS NEVER MODIFIED FOR LINUX (Little Endian Environment)
   *       AND was not modified for compression
   */                     
/*  loads CRF data from a certain message in a linear buffer */
char *load_cvg_raw_lb(char *lb_filename, int position)
{                      
    int num_products, len, flag;
    int lbfd = -1, msg_id = position;
   int listSize = maxProducts+1;
   LB_info list[listSize]; 
    Prod_header *hdr;  
    char* buffer;      
    int pid, data_type, packet_size, i, j, offset, buf_offset, msg_len;
    int num_range_bins, num_radials, num_bytes_per_bin, num_bytes, num_rows;
    int *start_angles, *angle_deltas;
    char *out_buf, *packet;
    short *halfword_packet;
    unsigned char bin; 
                       
    if(verbose_flag)   
        printf("\n      ORPG Database Product Load Utility\n");
                       
    /* open the linear buffer we want */
    lbfd = LB_open(lb_filename, LB_READ, NULL);
    if(lbfd < 0) {     
        if(verbose_flag) 
        printf("ERROR opening the following linear buffer: %s\n", lb_filename);
        return(NULL);      
    }
   
    /* get the linear buffer's inventory */
    num_products = LB_list(lbfd, list, maxProducts+1);
    if(verbose_flag) 
        printf("num_products available=%i msg_id=%d\n", num_products, msg_id);
   
    /* check for potential overflow with msg_id variable */
    if(msg_id<1 || msg_id>num_products) {
        if(verbose_flag) 
            printf("ERROR: input message sequence number is out of bounds\n");
        return(NULL);
    }
   
    /* read in the product we want - allocates the memory for us */
    len = LB_read(lbfd, &buffer, LB_ALLOC_BUF, list[msg_id-1].id);

    if(buffer == NULL) {
        fprintf(stderr,"ERROR load_cvg_raw- unable to read buffer message\n");
        return NULL;
    }

    if(verbose_flag) {
        hdr = (Prod_header *)buffer;
        printf("Product Info: LBuffer# %03hd MSGLEN %06d VOLNUM %02d ELEV %02d\n",
          hdr->g.prod_id, hdr->g.len, hdr->g.vol_num, 
          get_elev_ind(buffer,orpg_build_i));
    }

    /* clean up */
    LB_close(lbfd);
   
    /* make sure we read in everything */
    if(len != list[msg_id-1].size) {
         free(buffer);
         return NULL;
    }

    /* first read in the type of data we're dealing with */
    buf_offset = 96;
    data_type = read_word(buffer, &buf_offset);

    /* and the product ID */
    pid = read_word(buffer, &buf_offset);

    /* and an unused flag variable */
    flag = read_word(buffer, &buf_offset);

    if(verbose_flag) {
        printf("CRF header data\n");
        printf("data type = %d  product ID = %d\n", data_type, pid);
        printf("flag = 0x%08x\n", flag);
    }

    if(data_type == CVG_RAW_DATA_RADIAL) {
        /* for raw radial data, we put it in a digital radial data array packet */
        /* read in the rest of the header data */
        num_range_bins = read_word(buffer, &buf_offset);
        num_radials = read_word(buffer, &buf_offset);
        num_bytes_per_bin = read_word(buffer, &buf_offset);

    if(verbose_flag) {
        printf("number of range bins = %d\n", num_range_bins);
        printf("number of radials = %d\n", num_radials);
        printf("number of bytes per bin = %d\n", num_bytes_per_bin);
    }

    /* now, read in the angle data */
    start_angles = malloc(4*num_radials);
    angle_deltas = malloc(4*num_radials);
    for(i=0; i<num_radials; ++i)
        start_angles[i] = read_word(buffer, &buf_offset);
    for(i=0; i<num_radials; ++i)
        angle_deltas[i] = read_word(buffer, &buf_offset);

/* DEBUG */
    /*
    printf("\nangles:\n");
    for(i=0; i<num_radials; i++)
        printf("%8x", start_angles[i]);
    printf("\nangle deltas:\n");
    for(i=0; i<num_radials; i++)
        printf("%8x", angle_deltas[i]);
    printf("\n\n");
    */

    /* now that only the bin data is left, we can put together a packet */
    /* first, figure out how big it should be */
    packet_size = 14 + (6 + num_range_bins) * num_radials;
    num_bytes = num_range_bins;
    packet = malloc(packet_size);

    /* store the header info */
    halfword_packet = (short *)packet;
    bzero(packet, 14);        /* any unset fields should be zero */
    halfword_packet[0] = 16;  /* packet code */
    halfword_packet[2] = num_range_bins;
    halfword_packet[6] = num_radials;

    /* now, we read in each radial */
    for(i=0, offset=14; i<num_radials; i++) {
        /* radial header junk */
        packet[offset++] = (num_bytes >> 8) & 0xff;
        packet[offset++] = num_bytes & 0xff;
        packet[offset++] = (start_angles[i] >> 8) & 0xff;
        packet[offset++] = start_angles[i] & 0xff;
        packet[offset++] = (angle_deltas[i] >> 8) & 0xff;
        packet[offset++] = angle_deltas[i] & 0xff;
        /*
        if(verbose_flag)
            printf("number of bytes in packet = %d  start angle = %d "
                   " delta angle = %d\n",
               num_bytes, start_angles[i], angle_deltas[i]);
         */
        /* we only display up to 256 levels, so we only store the low-order
         * byte of the data
         */
        /*printf("\nradial %d:\n", i);*/
        for(j=0; j<num_range_bins; j++) {
            switch(num_bytes_per_bin) {
          case 4:
            bin = read_word(buffer, &buf_offset) & 0xff;
            break;
          case 2:
            bin = read_half(buffer, &buf_offset) & 0xff;
            break;
          case 1:
          default:
            bin = read_byte(buffer, &buf_offset);
            break;
        }
        packet[offset++] = bin;
        /*printf("%4hx", packet[offset-1]);*/
        }
        /*
        if(verbose_flag)
            printf("end offset = %d\n", offset);
        */
    }

    /* now, put together the whole packet */
    msg_len = 216+16+packet_size;
    out_buf = malloc(msg_len);
    bzero(out_buf, msg_len);        /* zero out the buffer */
        /* MOD include the Pre-ICD Header*/
        memcpy(out_buf, buffer, 96); 
    halfword_packet = (short *)out_buf;
    halfword_packet[0] = pid;
    halfword_packet[48+4] = (msg_len >> 16) & 0xffff;
    halfword_packet[48+5] = msg_len & 0xffff;
    halfword_packet[48+54] = 0;
    halfword_packet[48+55] = 60;    /* symbology offset */

    halfword_packet[48+9] = halfword_packet[108] = halfword_packet[108+5] = -1;
    halfword_packet[108+1] = 1;
    halfword_packet[108+2] = ((msg_len-216) >> 16) & 0xffff;
    halfword_packet[108+3] = (msg_len-216) & 0xffff;
    halfword_packet[108+4] = 1;
    halfword_packet[108+6] = ((msg_len-216-16) >> 16) & 0xffff;
    halfword_packet[108+7] = (msg_len-216-16) & 0xffff;
    memcpy(out_buf+216+16, packet, packet_size);

    /* finally, free up all the memory we don't need */
    free(packet);
    free(start_angles);
    free(angle_deltas);
    } else if(data_type == CVG_RAW_DATA_RASTER) {
        int num_data_bytes, *num_bytes_in_row;

        /* and for raw raster data, we put it in a packet of our own design
     * essentially a non-RLE raster packet 
     */
        /* read in the rest of the header data */
        num_rows = read_word(buffer, &buf_offset);
        num_bytes_per_bin = read_word(buffer, &buf_offset);

        /* now, save the number of bytes that is in each row */
        /* and keep a running total */
        num_data_bytes = 0;
        num_bytes_in_row = malloc(sizeof(int *) * num_rows);
        for(i=0; i<num_rows; i++) {
            num_bytes_in_row[i] = read_word(buffer, &buf_offset);
            num_data_bytes += num_bytes_in_row[i];
        }
    
        /* now that only the bin data is left, we can put together a packet */
        /* first, figure out how big it should be */
        packet_size = 14 + num_data_bytes;
        packet = malloc(packet_size);
    
        /* store the header info */
        halfword_packet = (short *)packet;
        bzero(packet, 4);        /* any unset fields should be zero */
        halfword_packet[0] = 35;  /* packet code */
        halfword_packet[1] = num_rows;
    
        /* now, we read in each row */
        for(i=0, offset=4; i<num_rows; i++) {
            num_bytes = 2 + num_bytes_in_row[i];
    
            /* row header junk */
            packet[offset++] = (num_bytes >> 8) & 0xff;
            packet[offset++] = num_bytes & 0xff;
    
            /* we only display up to 256 levels, so we only store the low-order
             * byte of the data
             */
            for(j=0; j<num_bytes_in_row[i]; j+=num_bytes_per_bin) {
                switch(num_bytes_per_bin) {
                  case 4:
                    bin = read_word(buffer, &buf_offset) & 0xff;
                    break;
                  case 2:
                    bin = read_half(buffer, &buf_offset) & 0xff;
                    break;
                  case 1:
                  default:
                    bin = read_byte(buffer, &buf_offset);
                    break;
                } /*  end switch */
                packet[offset++] = bin;
            } /*  end for j */
            
        } /*  end for i */
    
        /* now, put together the whole packet */
        msg_len = 216+16+packet_size;
        out_buf = malloc(msg_len);
        bzero(out_buf, msg_len);        /* zero out the buffer */
            /* MOD include the Pre-ICD Header */
        memcpy(out_buf, buffer, 96);
        halfword_packet = (short *)out_buf;
        halfword_packet[0] = pid;
        halfword_packet[48+4] = (msg_len >> 16) & 0xffff;
        halfword_packet[48+5] = msg_len & 0xffff;
        halfword_packet[48+54] = 0;
        halfword_packet[48+55] = 60;    /* symbology offset */
    
        halfword_packet[48+9] = halfword_packet[108] = halfword_packet[108+5] = -1;
        halfword_packet[108+1] = 1;
        halfword_packet[108+2] = ((msg_len-216) >> 16) & 0xffff;
        halfword_packet[108+3] = (msg_len-216) & 0xffff;
        halfword_packet[108+4] = 1;
        halfword_packet[108+6] = ((msg_len-216-14) >> 16) & 0xffff;
        halfword_packet[108+7] = (msg_len-216-14) & 0xffff;
        memcpy(out_buf+216+16, packet, packet_size);
    
        /* finally, free up all the memory we don't need */
        free(packet);
    
    
    } else {
        out_buf = NULL;
        printf("invalid data format\n");
    }

    free(buffer);

    return out_buf;    

} /*  end load_cvg_raw_lb() */



/*  THIS FUNCTION IS NOT CURRENTLY USED */
char *get_default_data_directory(char *prodname) 
{
   /* load the default data subdirectory from the environmental variable ORPGDIR$ 
      module updated for beta 1.83 release */
   static char dir1[100],dir2[100],dir3[100];
   char *charval;
   int result;

   /* get the path for the products database */
   charval = getenv("ORPGDIR");
   if(charval==NULL) {
     if(verbose_flag) 
         printf("ERROR: ORPGDIR environmental variable is not set\n");
     return(NULL);
   }
   
   /* first pass check the base data directory */
   sprintf(dir1,"%s/base/%s",charval,prodname);
   result=check_for_directory_existence(dir1);
   if(result==TRUE) {
     if(verbose_flag) 
         printf("1st Pass Dir Found: %s\n",dir1);
     return (dir1);
   }

   /* second pass check the pdist data directory */
   sprintf(dir2,"%s/pdist/%s",charval,prodname);
   result=check_for_directory_existence(dir2);
   if(result==TRUE) {
     if(verbose_flag) 
         printf("2nd Pass Dir Found: %s\n",dir2);
     return (dir2);
   }

   /* third pass check the msgs data directory */
   sprintf(dir3,"%s/msgs/%s",charval,prodname);
   result=check_for_directory_existence(dir3);
   if(result==TRUE) {
       if(verbose_flag) 
           printf("3rd Pass Dir Found: %s\n",dir3);
       return (dir3);
   }
   
   if(verbose_flag) {
       printf("Could not find files in the following locations\n");
       printf("pass1: %s\n",dir1);
       printf("pass2: %s\n",dir2);
       printf("pass3: %s\n",dir3);
     
       printf("ERROR: Could not find file in base pdist or msgs subdirectory\n");
   }
   sprintf(dir1,"NONE");
   return(dir1);
} /*  end get_default_data_directory */





void accept_icd_error_cb(Widget w,XtPointer client_data, XtPointer call_data)
{
    
    XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) call_data;
    
    if(cbs->set == XmSET)        
        accept_icd_error = TRUE;
       
    else
        accept_icd_error = FALSE;
    
}  



