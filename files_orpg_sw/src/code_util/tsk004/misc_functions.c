/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:54 $
 * $Id: misc_functions.c,v 1.13 2009/05/15 17:37:54 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/* misc_functions.c */

#include "misc_functions2.h"






int read_word(char *buffer,int *offset) {
  /* read a word (4 bytes) from the buffer then
  increment the offset pointer */

/* LINUX changes NEW LOGIC */
  int d1;
  memcpy(&d1,buffer+*offset,4);
#ifdef LITTLE_ENDIAN_MACHINE 
  d1=INT_BSWAP(d1);
#endif
  *offset+=4;  /* increment offset counter */
  
  return(d1);

}



short read_half(char *buffer,int *offset) {
  /* read a half word (2 bytes) from buffer then
  increment the offset pointer */

/*LINUX changes NEW LOGIC */   
  short d1;
  memcpy(&d1,buffer+*offset,2);
#ifdef LITTLE_ENDIAN_MACHINE 
   d1=SHORT_BSWAP(d1);
#endif
  *offset+=2;  /* increment offset counter */
  
  return(d1);

}



unsigned char read_byte(char *buffer,int *offset) {
  /* read a byte from buffer then increment the offset pointer */
  unsigned char c1;
  
  c1=(unsigned char)buffer[*offset];
   
  *offset+=1;  /* increment offset counter */
  
  return(c1);

}



/*============================================================================*/



/****************************************************************
   Description:
      Packs 4 bytes pointed to by "value" into 2 unsigned shorts.
      "value" can be of any type.  The address where the 4 bytes
      starting at "value" will be stored starts @ "loc".  
                                                          
      The Most Significant 2 bytes (MSW)  of value are stored at 
      the byte addressed by "loc", the Least Significant 2 bytes 
      (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where to store value. 
      value - pointer to data value.

   Output:
      loc - stores the MSW halfword of "value" at
            (unsigned short *) loc and the LSW halfword of
            "value" at ((unsigned short *) loc) + 1.

   Returns:
      Always returns 0.

****************************************************************/
int write_orpg_product_int( void *loc, void *value ){

   unsigned int   fw_value = *((unsigned int *) value);
   unsigned short hw_value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   hw_value = (unsigned short) (fw_value >> 16) & 0xffff;
   *msw = hw_value;

   hw_value = (unsigned short) (fw_value & 0xffff);
   *lsw = hw_value;

   return 0;

} /* end of write_orpg_product_int() */
 
 
/****************************************************************
   Description:
      Unpacks the data value @ loc.  The unpacked value will be 
      stored at "value". 

      The Most Significant 2 bytes (MSW) of the packed value are
      stored at the byte addressed by "loc", the Least Significant
      2 bytes (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where packed value is stored.
      value - address to received the packed value.

   Output:
      value - holds the unpacked value.

   Returns:
      Always returns 0.

****************************************************************/
int read_orpg_product_int( void *loc, void *value ){
   unsigned int *fw_value = (unsigned int *) value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   *fw_value = 
      (unsigned int) (0xffff0000 & ((*msw) << 16)) | ((*lsw) & 0xffff);

   return 0;

/* End of read_orpg_product_int() */
}


int write_orpg_product_float( void *loc, void *value )
{
int ret;
    
    ret = write_orpg_product_int( loc, value );
    
    return ret;
    
}



int read_orpg_product_float( void *loc, void *value )
{
int ret;

    ret = read_orpg_product_int( loc, value );
    
    return ret;
    
}

/*============================================================================*/




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

  fprintf(stderr,"Decompressing Product\n");

  if(phd==NULL) {
    fprintf(stderr,"ERROR: Unable to allocate memory\n");
    free(*buffer);
    buffer = NULL;
    return;
  }
       
/* LINUX changes */
memcpy(phd,*buffer+96,120);

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
/*DEBUG*/
/* fprintf(stderr,"DEBUG DECOMPRESS - orig message length is %d \n",phd->msg_len);*/
/* fprintf(stderr,"DEBUG DECOMPRESS - compressed portion is %d \n",src_len); */
/* fprintf(stderr,"DEBUG DECOMPRESS - uncompressed portion is %d \n",dest_len); */

       if((dest_buf = malloc(dest_len+96+sizeof(Graphic_product))) == NULL) {
           fprintf(stderr,"ERROR: Unable to allocate memory\n");
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
               fprintf(stderr,"Decompression Error");
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
/*DEBUG*/
/* fprintf(stderr,"DEBUG DECOMPRESS - final message length is %d \n",phd->msg_len); */

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

} /*  end product_decompress */




/*============================================================================*/


/* checks HW 10, 20, and 29 in Product Description Block    */
/* returns FASLE if unexpected value is found               */
/* Inputs: bufptr - pointer to beginning of product         */
/*         error_flag - TRUE or FALSE determines whether    */
/*                     message concerning bad HW is printed */
int check_icd_format(char *bufptr, int error_flag)  {

  Graphic_product *gp;
  int pdb_divider_found=FALSE;
  int elev_ind_found=FALSE;
  int vol_num_found=FALSE;
  
  short divider, elev_ind, vol_num;
  
    gp = (Graphic_product*)bufptr;

    divider = gp->divider;
    elev_ind = gp->elev_ind;
    vol_num = gp->vol_num;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    divider = SHORT_BSWAP(divider);
    elev_ind = SHORT_BSWAP(elev_ind);
    vol_num = SHORT_BSWAP(vol_num);
#endif
    

    
    if(divider == -1)
        pdb_divider_found = TRUE;
    /* to support TDWR products use 25 vice 20*/
    if((elev_ind >= 0) && (elev_ind <= 25))
        elev_ind_found = TRUE;    
    
    if((vol_num >= 1) && (vol_num <= 80))
        vol_num_found = TRUE;

        
    if(pdb_divider_found && elev_ind_found && vol_num_found) {
        fprintf(stderr,"Binary File appears to contain an ICD Formatted Product\n");
        return TRUE;
    }
    
    if(error_flag) {
        fprintf(stderr,
             "ERROR - LB Message / Binary File contents do not match ICD Format.\n");
        if(pdb_divider_found == FALSE)
             fprintf(stderr,
                  "PDB HW 10 - Block Divider value (%d) is not -1\n",divider);
        if(vol_num_found == FALSE)
             fprintf(stderr,"PDB HW 20 - Volume number (%d) is not 1 - 80.\n",
                                                                      vol_num);
        if(elev_ind_found == FALSE)
             fprintf(stderr,"PDB HW 29 - Elevation index (%d) is not 0 - 20.\n",
                                                                       elev_ind);
             
        fprintf(stderr,"The product must past this test for the selected "
                       "CODEview Text Function\n");

    } else {
        fprintf(stderr,
             "INFO - LB Message / Binary File contents do not match ICD Format\n"); 
        
    }   

    return FALSE;
        
} /* end check_icd_format */



/*============================================================================*/



/* checks block offsets, block lengths, and product length  */
/* prints message concerning suspected problem found        */
/* returns FASLE if unexpected value is found               */
/* Inputs: bufptr - pointer to beginning of product         */
/*         verbose - if TRUE provides more info w/ no error */
int check_offset_length(char *bufptr, int verbose) {

  Graphic_product *gp;

  int sum_len=0;     /* sum of layer / page lengths */
  int calc_length=0; /* calculated length of sym block / GAB */
  int symb_length=0; /* length of symbology block */   
  int gab_length=0;  /* length of gab */ 
  int tab_length=0;  /* length of tab */
  int calc_prod_length;   /* calculated product length */
  int message_length; 
  int opt_blocks;  /* number of optional blocks: symb, GAB, TAB */

  int symb_present = FALSE;
  int gab_present = FALSE;
  int tab_present = FALSE;
   
  /* the following valuses are set: -1 FALSE, 0 UNK or N/A, 1 TRUE */  
  int good_symb_offset=0; 
  int good_gab_offset=0;
  int good_tab_offset=0;  
  int good_symb_length=0; 
  int good_gab_length=0; 

  int symb_off, gab_off, tab_off;
  short n_blocks, divider;  /*  cvt 4.1 change from int   */
  int offset;  
  short symb_id, gab_id, tab_id;
  
  int retval=TRUE;  /* SET TO FALSE WITH ANY ERROR */


    fprintf(stderr, "\nPerforming Consistency Check of Product Length with\n");
    fprintf(stderr, "Block Offsets and Block Lengths\n");    
    gp = (Graphic_product*)bufptr;

    symb_off = gp->sym_off;
    gab_off = gp->gra_off;
    tab_off = gp->tab_off;
    message_length = gp->msg_len;
    n_blocks = gp->n_blocks;

/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    symb_off = INT_BSWAP(symb_off);
    gab_off = INT_BSWAP(gab_off);
    tab_off = INT_BSWAP(tab_off);
    message_length = INT_BSWAP(message_length);
    n_blocks = SHORT_BSWAP(n_blocks);
#endif
    

    
    opt_blocks = 0; 
    
    if(symb_off > 0) {
        opt_blocks++;
        symb_present = TRUE;
    }
    if(gab_off > 0) {
        opt_blocks++;
        gab_present = TRUE;
    }
    if(tab_off > 0) {
        opt_blocks++;
        tab_present = TRUE;
    }
  

    /* original test for a SATAP does not work because of irregularities:  */
    /*        a. the Block ID is 1 if there is only one page to the SATAP  */
    /*        b. some SATAP have the GAB offset for some reason            */
      /* IF SYMB PRESENT AND GAB AND TAB ARE NOT PRESENT               */
      /*   AND IF EITHER BLOCK ID != 1  OR FIRST LAYER DIVIDER != -1   */
      /*   THEN  REPORT RESULTS FOR SATAP AND QUIT.                    */ 
    if((symb_present == TRUE) && (tab_present != TRUE)) {    
        short divider,pages,blockID; 
        int offset; /* offset in bytes */
        int characters, i;
        
         offset = (120);
         
         divider = read_half(bufptr,&offset); /* first optional block */
  
         blockID = read_half(bufptr,&offset); /* block ID (if a symbology block) */
         offset = (120+10);
         divider = read_half(bufptr,&offset); /*first layer(if a symbology block)*/

        /* previous test for a SATAP did not work!!   */
        if( (divider != -1)) {  /*  no symb block layer divider */
            fprintf(stderr,"\n");
            fprintf(stderr, 
                 "--- Stand Alone Tabular Alphanumeric Product (SATAP) Detected ---\n");
            if(symb_off != 60) {
                 fprintf(stderr,
                      "Error - Offset to SATAP (%d) is Incorrect\n",symb_off);
                 retval=FALSE;
            }
           offset = (120+2);
           pages = read_half(bufptr,&offset);
           characters = read_half(bufptr,&offset);         
           
           if(verbose)
                fprintf(stderr,"    SATAP has %d pages\n",pages);
           if(pages<1 || pages>24) {
              fprintf(stderr,
                   "Warning - Number of Pages (%d) in the Stand Alone\n",pages);
               fprintf(stderr,
                    "         Alphanumeric product is out of range (1-24).\n");
               retval=FALSE;
            }

            if(retval != FALSE) {
                offset = (120 + 4);
                for(i=1; i<=pages; i++) {
                    characters = read_half(bufptr,&offset);
                    if(characters<0 || characters>80) {
                        fprintf(stderr,
                             "Warning - Number of Characters (%d) in page %d\n",
                                                                    characters, i);
                        fprintf(stderr,
                             "          of the SATAP is out of range (0-80).\n");
                        retval=FALSE;
                    } /*  end if characters */
                    
                    while( (retval != FALSE) && (offset < (message_length)) ) {
                        divider = read_half(bufptr,&offset);
                        
                        if(divider == -1) {
                            break;
                        }
                        
                    } /*  end while */
                    
                    if( offset > (message_length) ) {
                        fprintf(stderr,
                             "Warning - Page divider for page %d not found.\n",i);
                        fprintf(stderr,"          Terminating SATAP check.\n");
                        retval=FALSE;
                        break;
                    }
                
               
                } /*  end for pages */
            
            } /*  end if retval not FALSE */
             
            
            fprintf(stderr,"\nFinished Consistency Check\n\n");
            return (retval);    
            
        }  /*  end if no layer 2 divider */
        
    } /*  end if symb_present and not tab_present */
  
  
    /* check number of blocks with non-zero block offsets */  
    fprintf(stderr,"\n");
    fprintf(stderr, "--- Checking number of blocks vs. block offset values ---\n");

    /* Several existing products do not have a consistent value for the */
    /* number of blocks field: VAD, STP. OHP, and THP.  Because display */
    /* devices are not currently affected, this error was modified      */
    /* to be a WARNING.                                                 */     
    if((n_blocks - 2) == opt_blocks) { /* we are OK */ 
        if(verbose) {
             fprintf(stderr, 
                  "    Product has %d blocks:  the MHB and the PDB plus\n",
                 n_blocks); 
             if(symb_present == TRUE)
                 fprintf(stderr, "    the Symbology Block  ");
             if(gab_present == TRUE)
                 fprintf(stderr, " the GAB  ");
             if(tab_present == TRUE)
                 fprintf(stderr, " the TAB ");
             fprintf(stderr, "\n");   
        }  
    } else {
        fprintf(stderr, 
             "Warning - The number of Blocks %d (HW 9) does not agree with the\n",
                       n_blocks);
        fprintf(stderr, 
             "        optional blocks present as determined by the block offsets.\n");
        fprintf(stderr, "     Symbology Block Offset is %d\n",symb_off );
        fprintf(stderr, "     GAB Offset is %d\n",gab_off );
        fprintf(stderr, "     TAB Offset is %d\n",tab_off );
        fprintf(stderr, 
             "        which indicate the total number of blocks to be %d.\n",
                       (opt_blocks+2));
        fprintf(stderr, 
             "Several products (VAD, STP, OHP, and THP) have this error but \n");
        fprintf(stderr, "this has not affected current display systems. \n");
    } 
   
     
    /* check offsets to blocks present in this product*/
    fprintf(stderr,"\n");
    fprintf(stderr, "--- Checking block offsets with block identifiers ---\n");  
    if(symb_present == TRUE) {
        offset = (2*symb_off);
        divider = read_half(bufptr,&offset);
        symb_id = read_half(bufptr,&offset);
        good_symb_offset = 1;
        if(divider != -1) {
            fprintf(stderr,
                 "Error - Symbology Block Divider is %d using offset %d (HW 55 & 56)\n",
                            divider, symb_off);  
            good_symb_offset = -1;
            retval=FALSE;
        }  
        if(symb_id != 1) {
            fprintf(stderr,
                 "Error - Symbology Block ID is %d using offset %d (HW 55 & 56)\n",
                            symb_id, symb_off);
            good_symb_offset = -1; 
            retval=FALSE;               
        }
        if(good_symb_offset == 1) {
            symb_length = read_word(bufptr,&offset);
            if(verbose) 
                fprintf(stderr, 
                     "    Symbology Block Offset is %d halfwords (%d bytes)\n",
                           symb_off,(2*symb_off));
        } else 
            fprintf(stderr, 
                 "Possible Error in the Symbology Block Offset Detected\n");
    }
        
    if(gab_present == TRUE) {
        offset = (2*gab_off);
        divider = read_half(bufptr,&offset);
        gab_id = read_half(bufptr,&offset);
        good_gab_offset = 1;
        
        if(divider != -1) {
            fprintf(stderr,
                 "Error - GAB Divider is %d using GAB offset %d (HW 57 & 58)\n",
                            divider, gab_off);  
            good_gab_offset = -1;
            retval=FALSE;
        }  
        if(gab_id != 2) {
            fprintf(stderr,
                 "Error - GAB ID is %d using GAB offset %d (HW 57 & 58)\n",
                            gab_id, gab_off);
            good_gab_offset = -1; 
            retval=FALSE;               
        }
        if(good_gab_offset == 1) {   
            gab_length = read_word(bufptr,&offset);
            if(verbose) 
                fprintf(stderr, "    GAB Offset is %d halfwords (%d bytes)\n",
                            gab_off,(2*gab_off)); 
        } else
            fprintf(stderr, "Possible Error in the GAB Offset Detected\n"); 
    }
      
    if(tab_present == TRUE) { 
        offset = (2*tab_off);
        divider = read_half(bufptr,&offset);
        tab_id = read_half(bufptr,&offset);
        good_tab_offset = 1;
        if(divider != -1) {
            fprintf(stderr,
                 "Error - TAB Divider is %d using TAB offset %d (HW 59 & 60)\n",
                            divider, tab_off);  
            good_tab_offset = -1; 
            retval=FALSE;
        }  
        if(tab_id != 3) {
            fprintf(stderr,
                 "Error - TAB ID is %d using TAB offset %d (HW 59 & 60)\n",
                            tab_id, tab_off);
            good_tab_offset = -1; 
            retval=FALSE;               
        }
        if(good_tab_offset == 1) {   
            tab_length = read_word(bufptr,&offset);
            if(verbose) 
                fprintf(stderr, "    TAB Offset is %d halfwords (%d bytes)\n",
                            tab_off,(2*tab_off)); 
        } else
            fprintf(stderr, "Possible Error in the TAB Offset Detected\n");    
    }     
  
  
      
    /* IF SYMB PRESENT */
    /* parse through symbology block layers and check layer lengths */
    if(symb_off>0 && symb_off<=400000) {
      /* load the symbology block header */
      
      int i;
      short s_id;     
      short divider;  
      short layers;
      int offset; /* offset in bytes */

      fprintf(stderr,"\n");
      fprintf(stderr, "--- Checking Symbology Block layer lengths ---\n");

        offset = (2*symb_off) + 2; 
        s_id = read_half(bufptr,&offset);

        if(s_id != 1)
            fprintf(stderr,
                    "Error - Not in Symbology Block as expected (block id is %d),\n"
                    "        check offset (HW 55 & 56)\n", s_id);
        else { /* continue  checking Sumbology Block layer lengths */
            
            offset+=4;
            layers = read_half(bufptr,&offset);
      
            if(verbose) fprintf(stderr,
                     "    Symbology Block Length is %d.\n",symb_length);
            

            if(layers>18 || layers<1) {
                 fprintf(stderr,
                     "Warning - Number of Layers (%d) is out of range (1-18).\n",
                                                                          layers);
                 fprintf(stderr,
                     "          CVT will accept up to %d layers for "
                     "development purposes.\n", MAX_LAYERS);
            } else if(verbose) 
                 fprintf(stderr, "    Symbology block has %d layers.\n",layers);
    
            /* offset of the first symb layer divider */
            offset = (2 * symb_off) + 10;
            sum_len=0; 
            for(i=1;i<=layers;i++) {
                int length=0;
                         
                divider=read_half(bufptr,&offset); /* read layer divider */
                if(divider != -1) {
                   fprintf(stderr,
                        "Error - Symbology Block Layer Divider Not Found\n");
                   fprintf(stderr,
                        "        Divider for Layer %d is '%d' rather than '-1'.\n",
                                         i, divider);
                }
                
                length=read_word(bufptr,&offset); /*read the layer length */

                if(length==0)
                    fprintf(stderr,"CAUTION- Layer %2d Length of 0 bytes is not "
                                   "standard product construcion. -CAUTION\n", i);
                else 
                    if(verbose) 
                        fprintf(stderr,
                             "    Layer %2d Length = %d bytes\n",i,length);
                
                sum_len+=length; /* sum up length of symbology block  */
                
                offset+=length; /* advance pointer to beginning of next block */
                
            } /* end for */
        
            /* size of sym block is sum of layer lengths plus 6 bytes  */
            /* for each layer header and 10 bytes for the block header */
            calc_length = sum_len + (6 * layers) + 10;
            if (calc_length != symb_length) {
               fprintf(stderr,
                    "Error - Symbology Block Length (HW 63 and 64) does not agree\n");
               fprintf(stderr,
                    "        with the total length of the %d layer(s).\n",layers);
            }
            /* if GAB present, check for divider and ID */
            /* else if TAB present, check for divider and ID */
            
            if(gab_present == TRUE) {
                offset = (2*symb_off) + symb_length;
                divider = read_half(bufptr,&offset);
                gab_id = read_half(bufptr,&offset);
                good_symb_length = 1;
                if(divider != -1) {
                    fprintf(stderr,
                         "Error - GAB Divider is %d using Symb Block Length of %d\n",
                                    divider, symb_length);  
                    good_symb_length = -1;
                    retval=FALSE;
                }  
                if(gab_id != 2) {
                    fprintf(stderr,
                         "Error - GAB ID is %d using Symb Block Length of %d\n",
                                    gab_id, symb_length);
                    good_symb_length = -1; 
                    retval=FALSE;               
                }
                if(good_symb_length == -1)
                    fprintf(stderr,
                         "Possible Error in the Length of the Symbology Block "
                         "Detected\n");
            
            } else if(tab_present == TRUE) {
                offset = (2*symb_off) + symb_length;
                divider = read_half(bufptr,&offset);
                tab_id = read_half(bufptr,&offset);
                good_symb_length = 1;
                if(divider != -1) {
                    fprintf(stderr,
                        "Error - TAB Divider is %d using Symb Block Length of %d\n",
                                    divider, symb_length);  
                    good_symb_length = -1;
                    retval=FALSE;
                }  
                if(tab_id != 3) {
                    fprintf(stderr,
                         "Error - TAB ID is %d using Symb Block Length of %d\n",
                                    tab_id, symb_length);
                    good_symb_length = -1; 
                    retval=FALSE;               
                }
                if(good_symb_length == -1)
                    fprintf(stderr,
                         "Possible Error in the Length of the Symbology Block "
                         "Detected\n");
            }
  
        } /* end continue checking layer lengths */
  
    } else if(symb_off>400000) {
        fprintf(stderr,"Warning - Symbology Block offset is out of range\n");
        retval = FALSE;
        
    } /* end if symbology block present */
  
  
    /* parse through GAB pages and check page lengths */
  
    if(gab_off>0 && gab_off<=400000) {
        int offset=0;
        int length=0;
        short pages=0;
        short page_n;
        short g_id;     /*  cvt 4.1 added */
        int i;

        fprintf(stderr,"\n");
        fprintf(stderr, "--- Checking GAB Page lengths ---\n"); 
        
        if(verbose) fprintf(stderr,"    GAB Length is %d.\n",gab_length);   
                     
        offset = (2*gab_off) + 2; 
        g_id = read_half(bufptr,&offset);

        if(g_id != 2)        
            fprintf(stderr,"Error - Not in GAB as expected (block id is %d),\n"
                           "        check offset to GAB\n", g_id);
                           
        else { /* continue  checking GAB page lengths */
            offset+=4;
            pages = read_half(bufptr,&offset);
            if(verbose) fprintf(stderr,"    Number of GAB pages is %d\n",pages);
            sum_len=0;
            for(i=1;i<=pages;i++) {
                page_n = read_half(bufptr,&offset);
                if(i != page_n)
                    fprintf(stderr,
                         "Error - Page Number for GAB Page %d is %d\n",i,page_n);
                length=read_half(bufptr,&offset); /*read the page length */     
                if(verbose) fprintf(stderr,
                         "    Page %2d Length = %d bytes\n",i,length);  

                sum_len+=length; /* sum up length of GAB pages  */
                offset+=length; /* advance pointer to beginning of next page */ 
            }
            /* size of the GAB is sum of page lengths plus 4 bytes  */
            /* for each page header and 10 bytes for the GAB header */
            calc_length = sum_len + (4 * pages) + 10;
            if (calc_length != gab_length) {
               fprintf(stderr,
                    "Error - Graphic Alphanumeric Block Length does not agree\n");
               fprintf(stderr,
                    "        with the total length of the %d page(s).\n",pages);
               retval = FALSE;
            }            
        
            /* if TAB present check for beginning of TAB */
            if(tab_present == TRUE) {
               offset = (2*gab_off) + gab_length;
               divider = read_half(bufptr,&offset);
               tab_id = read_half(bufptr,&offset);
               good_gab_length = 1;
               if(divider != -1) {
                   fprintf(stderr,
                        "Error - TAB Divider is %d using GAB Length of %d\n",
                                   divider, gab_length);  
                   good_gab_length = -1;
                   retval=FALSE;
               }  
               if(tab_id != 3) {
                   fprintf(stderr,"Error - TAB ID is %d using GAB Length of %d\n",
                                   tab_id, gab_length);
                   good_gab_length = -1; 
                   retval=FALSE;               
               }
               if(good_gab_length == -1)
                   fprintf(stderr,
                        "Possible Error in the Length of the GAB Detected\n"); 
            }
         
        }   /* end continue checking GAB page lengths */      

    } /* end if GAB present */
  
  
    /* parse through TAB pages and check page lengths */
    /* checked during the display of the TAB          */
   
  
    if(tab_off>0 && tab_off<=400000) {
        int offset=0;
        short pages=0;
        short t_id;     /*  cvt 4.1 added */

        fprintf(stderr,"\n");
        fprintf(stderr, "--- Checking TAB Page lengths ---\n"); 
        
        if(verbose) fprintf(stderr,"    TAB Length is %d.\n",tab_length);   
                     
        offset = (2*tab_off) + 2; 

        t_id = read_half(bufptr,&offset);

        if(t_id != 3)        
            fprintf(stderr,"Error - Not in TAB as expected (block id is %d),\n"
                           "        check offset to TAB\n", t_id);

        else { /* continue  checking TAB page lengths */
            offset+=126; /* skip block length, a MHB, a PDB, and a divider */
            pages = read_half(bufptr,&offset);
            if(verbose) fprintf(stderr,"    Number of TAB pages is %d\n",pages);
            if(verbose) 
                fprintf(stderr,
                     "TAB Page consistency is checked when displaying the TAB.\n");
                            
        } /* end continue checking TAB pages */
        
    }  /* end if TAB present */
    
    
    /* check block lengths with product length */
    /* only accomplish if block offsets are correct */  
    fprintf(stderr,"\n");
    fprintf(stderr, "--- Comparing block lengths with product message length.\n");  
    calc_prod_length = 120;  /* the length of the MHB and PDB */
    if(symb_present == TRUE)
        calc_prod_length+=symb_length;
    if(gab_present == TRUE)
        calc_prod_length+=gab_length;
    if(tab_present == TRUE)
        calc_prod_length+=tab_length;
    if(calc_prod_length != message_length) {
        fprintf(stderr,
             "ERROR - The product length (%d bytes) does not agree with the sum of the\n",
                   message_length);
        fprintf(stderr,
            "        block lengths plus 120 bytes for the MHB and PDB (%d bytes)\n",
                   calc_prod_length);
        fprintf(stderr,"\n");
        fprintf(stderr,
             "First address any errors reported in the individual block lengths\n");
        retval = FALSE;
    } else
        if(verbose) {
            fprintf(stderr, "    Product Message Length is %d bytes\n",message_length);
            fprintf(stderr, "    Standard headers (MHB & PDB): 120 bytes\n");
            if(symb_present == TRUE)
                fprintf(stderr, "    Symbology Block Length is %d bytes",symb_length);
            if(gab_present == TRUE)
                fprintf(stderr, "\n    GAB Length is %d bytes",gab_length);
            if(tab_present == TRUE)
            fprintf(stderr, "\n    TAB Length is %d bytes",tab_length);
            fprintf(stderr,"\n");
        } 
    fprintf(stderr,"\nFinished Consistency Check\n\n");
         
    return (retval);

} /* end check_offset_length */






/*============================================================================*/



void advance_offset(char *buffer, int *offset,short num_bytes) {
  /* advance the buffer pointer by num_bytes */
  int i;
  unsigned char c;

  for(i=0;i<num_bytes;i++)
    c=read_byte(buffer,offset);

}




/*============================================================================*/







/* CVT 4.4 */
/* reads in from file char by char until we get to the end of the line */
void read_to_eol(FILE *list_file, char *buf)
{
    int i=0;

    do {
        fread(&(buf[i]), 1, 1, list_file);  /* read in one char */
    } while((buf[i++] != '\n') && 
        ((ferror(list_file)) == 0) && 
        ((feof(list_file)) == 0));

    buf[--i] = '\0';

    if((ferror(list_file)) != 0) {
        fprintf(stderr, "Error reading stream: %d\n", ferror(list_file));
    }

} /* end read_to_eol */





/*****************************************************************************/



/* CVT 4.4 */
/*  The following look-up conversion is used for product code <= 130 */
/*  other wise the product id is the product code                    */
/*  a return ID value of 0 is used for a product code that is not    */
/*  assigned to a final product                                      */
int prod_code_to_id(int prod_code) {
    
/* see misc_functions2.h for definition of code_to_id[] */

    return code_to_id[prod_code];
    
} /* end prod_id_to_code */





/*****************************************************************************/







/* function reads elev_ind from 96 byte pre-ICD header */
/* if CV_ORPG_BUILD is not set, orpg_build = 999 */
short get_elev_ind(char *bufptr, int orpg_build) {

    Prod_header_b5 *hdr5; /* structure Build 5 and earlier */
    Prod_header_b6 *hdr6; /* structure Build 6 and later (DEFAULT) */
    
    if(orpg_build >= 6) {
        hdr6=(Prod_header_b6 *) bufptr; 
        return (hdr6->g.elev_ind);    
     } else {
        hdr5=(Prod_header_b5 *) bufptr; 
        return (hdr5->elev_ind);          
     } 

}


