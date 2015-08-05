/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/04/09 19:51:43 $
 * $Id: product_names.c,v 1.12 2008/04/09 19:51:43 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/* product_names.c */

#include "product_names.h"


/* loads the product descriptions from the ORPG product attribute
 * table into the global variable product_names as well as the 
 * product short names in to the global variable short_prod_names
 * and the 3 character mnemonics in to product_mnemonics
 * All are indexed by product ID
 *
 * SIDEFFECTS:  allocates enough memory to store all of the
 * description strings, short names, and mnemonics
 * RESULT:  a fully populated product_names, short_prod_names, and
 * product_mnemonics
 */
 
/* CVG 6.5 - added logic to limit the product name string to 70 characters
 * plus the NULL character.  Currently the longest description is 62 
 * characters by LRA and LRM.  This permits a reduction in size of the 
 * product list message created by the background task.  The limiting
 * factor is the size of db_entry_string[112] in global2.h.
 */
/* cvg 8.0 - called from both cvg and cvg_read_db (the new exec'd child
 * process).  Called with FALSE from cvg and TRUE from cvg_read_db.
 */
 
void load_product_names(int from_child, int initial_read)
{
    int lbfd=-1;
    int err, result, len;
    LB_info msg_info;
    int offset;
    char *buf, *charval=NULL, filename[256], prod_desc[PROD_DESC_MAX+1];
    Pd_attr_entry *prod_attr;
    int i, j;

    /*  cvg 8.4 increase array size to handle inproper configuration of a  */
    /*          mnemonic longer than 3 characters                          */
    char prod_mnemon[32]; /*  hold 3 chararacter mnemonic padded with blanks  */
                          /*  we truncate to 3 plus terminator */
    size_t loc;

    FILE *list_file;
    
    char list_buf[120];
    /* cvg 8.5.2 increase from 20 to 64 to handle inproper configuration of a */
    /*           short name longer than 31 characters                         */
    char prod_short[32];  /* hold 31 character short name      */
                          /* we truncate to 31 plus terminator */
    
    int position, num_read, prod_id;

    /* get the name arrays ready */
/*     assoc_init_s(&product_names); */
/*     assoc_init_s(&short_prod_names); */
/*     assoc_init_s(&product_mnemonics); */

    /* CVG 8.0: An alternate source of product descriptions is provided 
     * to handle three situations:
     *     1. The prod_info.lb (CVG 8.0a) / pat.lb (CVG 8.0b) file does not 
     *        exist or cannot be opened because of problems with ORPGDIR
     *     2. CVG was compiled with STANDALONE_CVG defined
     *     3. The user selected the alternate source.
     */


    if(use_cvg_list_flag == FALSE) { 
        
        /* get the path for the product attribute table */   
        charval = getenv("ORPGDIR");
        if(charval==NULL) {
            if(from_child == FALSE) {
                fprintf(stderr,"NOTE: Variable ORPGDIR not defined.\n");
                fprintf(stderr,"**********************************************\n");
/* CVG 8.0b for Build 9 */
/*                 fprintf(stderr,"* The file prod_info.lb must be located      *\n"); */
/*                 fprintf(stderr,"*      in the $ORPGDIR/pdist  directory      *\n"); */
                fprintf(stderr,"* The file pat.lb must be located in the       *\n");
                fprintf(stderr,"*      $ORPGDIR/mngrpg  directory.             *\n");
            }
            use_cvg_list_flag = TRUE;
            if(from_child == FALSE)
                write_descript_source(TRUE); /*  communicate with backround task */
            
        }        
        /* put filename together */
/* CVG 8.0b for Build 9 */
/*         sprintf(filename, "%s/pdist/prod_info.lb", charval); */
        sprintf(filename, "%s/mngrpg/pat.lb", charval);
        result = check_for_directory_existence(filename);
        if(result == FALSE) {
            if(from_child == FALSE) {
                fprintf(stderr,
                      "NOTE: The Product Attribute Table file was not found.\n");
                fprintf(stderr,
                      "**********************************************\n");
/* CVG 8.0b for Build 9 */
/*               fprintf(stderr,"* The file prod_info.lb was not found in     *\n"); */
/*               fprintf(stderr,"*      the $ORPGDIR/pdist  directory         *\n"); */
                fprintf(stderr,"* The file pat.lb was not found in the       *\n");
                fprintf(stderr,"*      $ORPGDIR/mngrpg  directory.           *\n");
            }
            use_cvg_list_flag = TRUE;
            if(from_child == FALSE)
                write_descript_source(TRUE); /*  communicate with background task */
            

        } else {
            if(from_child == FALSE) {
                fprintf(stderr,
                      "* Individual product descriptions are obtained from\n");
                fprintf(stderr,
                      "*      the ORPG Product Information. For CODE this is\n");
                fprintf(stderr,
                      "*      derived from 'product_attr_table' and snippets.\n");
            }
        }

    } /*  end if use_cvg_list_flag FALSE */
    
     /****************************************************************/
     
     /*  CVG 8.0  ADDED LOGIC TO BUILD NAMES FROM AN ALTERNATE LIST  */
     
    if(use_cvg_list_flag == TRUE) {
       if(from_child == FALSE) {
           fprintf(stderr,"* Individual product descriptions are obtained from\n");
           fprintf(stderr,"*      the CVG Product Description List.  The file \n");
           fprintf(stderr,"*      'prod_names' in the ~/.cvgN.N directory.    \n");
       }
        /*  OPEN PRODUCT NAME FILE */
        sprintf(filename, "%s/prod_names", config_dir);
        if((list_file=fopen(filename, "r"))==NULL) {
            fprintf(stderr, "*********************************************\n");
            fprintf(stderr, "*  Could not open product_names             *\n");
            fprintf(stderr, "*                                           *\n");
            fprintf(stderr, "*  The file 'prod_names' in the             *\n");
            fprintf(stderr, "*     ~/.cvgN.N directory is                *\n");
            fprintf(stderr, "*     either missing or corrupt             *\n");
            fprintf(stderr, "*********************************************\n");
            fprintf(stderr, "* Product descriptions will not appear in   *\n");
            fprintf(stderr, "*     the product database select window.   *\n");
            fprintf(stderr, "*     Listed as 'Not Configured/Available'  *\n");
            fprintf(stderr, "*********************************************\n");
            return;
        }
 
        if(initial_read == TRUE)
            prev_cvg_list_flag = use_cvg_list_flag;

        /*  LOAD PRODUCT NAMES */
        while(feof(list_file) == 0) {  /* continue reading until we get to eof */

            read_to_eol(list_file, list_buf); 

            num_read = sscanf(list_buf, "%d%s%s%n", &prod_id, prod_short, 
                                     prod_mnemon, &position);
/* cvg 8.5.2 */
            if(strlen(prod_short) > 31) {
                prod_short[31] = '\0';
            }
            assoc_insert_s(short_prod_names, prod_id, prod_short);    

            loc = strlen(prod_mnemon);

/*  cvg 8.4 error if mnemonic longer than 3 characters */
            if((loc>3) && (from_child == FALSE)) {
                fprintf(stderr,"CONFIGURATION ERROR IN CVG file 'prod_names'\n");
                fprintf(stderr,"              Product ID %d has mnemonic ",
                                                                        prod_id);
                fprintf(stderr,"longer than 3 characters.\n");
            }

            for(j=(int)loc; j < 3; j++)
                prod_mnemon[j] = ' ';
            prod_mnemon[3] = '\0';    
               
            assoc_insert_s(product_mnemonics, prod_id, prod_mnemon);

            /* get description string, can include white space */
            /* skip initial white space */
            j=0;
            while(list_buf[position+j] == ' ') 
                j++;
                
            if(strlen(&list_buf[position+j])>=1)
                assoc_insert_s(product_names, prod_id, &list_buf[position+j]);
            else 
                assoc_insert_s(product_names, prod_id, " ");

    
        } /*  end while          */

        return;
        
    }/*  END ALTERNATE METHOD OF GETTING PRODUCT NAMES */
    
    /****************************************************************/

    /* BUILD NAMES FROM PRODUCT INFO LINEAR BUFFER - THE PRIMARY SOURCE */
    
    /* open the product attribute table linear buffer */
    lbfd = LB_open(filename, LB_READ, NULL);
    if(lbfd < 0) {
       fprintf(stderr,"ERROR opening the following linear buffer: %s\n", filename);
       return;
    }

    /* get info about the message containing the product attributes */
    err = LB_msg_info(lbfd, PROD_ATTR_MSG_ID, &msg_info);
    if(err < 0) {
        fprintf(stderr,
           "ERROR reading the product attribute message, err_no: %d, msg_id: %d\n", 
                                                             err, PROD_ATTR_MSG_ID);
    return;
    }


    /* read in the product attribute table */
    len = LB_read(lbfd, &buf, LB_ALLOC_BUF, PROD_ATTR_MSG_ID);
    if(len < 0) {
        fprintf(stderr,
           "ERROR reading from the product info linear buffer, len: %d, msg_id: %d\n", 
                                                               len, PROD_ATTR_MSG_ID);
    return;
    }
 

    /* go through the table to grab the descriptions */
    offset = 0;
    while(offset < msg_info.size) {
        /* pick the next entry out of the buffer */
        prod_attr = (Pd_attr_entry *)(buf + offset);

    /* we store the descriptions (which are kept as a null-terminated
     * string at the given offset) indexed by their product ID */

        if( ((char *)(prod_attr) + prod_attr->desc)[0] == ' ' )  {
            /*  intermediate product without a mnemonic */
            sprintf(prod_mnemon, "---");
            assoc_insert_s(product_mnemonics, prod_attr->prod_id, 
                       prod_mnemon);   
            len = strlen( (char *)(prod_attr) + (prod_attr->desc + 1 ) );
    
            if(len<=PROD_DESC_MAX) {
                assoc_insert_s(product_names, prod_attr->prod_id, 
                       (char *)(prod_attr) + (prod_attr->desc + 1));
            } else {
    
                strncpy(prod_desc, (char *)(prod_attr) + (prod_attr->desc + 1), 
                                                                 PROD_DESC_MAX);
                prod_desc[PROD_DESC_MAX]= '\0';
                assoc_insert_s(product_names, prod_attr->prod_id, prod_desc);
            }
                        
        } else { /*  a final product with mnemonic */
        
            /* need to extract mnemonic from description here */    
            loc = strcspn( (char *)(prod_attr) + prod_attr->desc, " " );

/*  cvg 8.4 error if mnemonic longer than 3 characters */
            if((loc>3) && (from_child == FALSE)) {
              fprintf(stderr,"CONFIGURATION ERROR IN ORPG 'product_attr_table'\n");
              fprintf(stderr,"              Product ID %d has mnemonic ",
                                                               prod_attr->prod_id);
              fprintf(stderr,"longer than 3 characters.\n");
            }

            for(i=0; i < (int)loc; i++) {
                 prod_mnemon[i] = ((char *)(prod_attr) + prod_attr->desc)[i];
            for(j=(int)loc; j < 3; j++)
                prod_mnemon[j] = ' ';
            prod_mnemon[3] = '\0';    
            }     
            
/* DEBUG */
/* fprintf(stderr,"MNEMONIC of product %d is %d characters long\n", */
/*                 prod_attr->prod_id, (int) loc); */
/* fprintf(stderr,"MNEMONIC of product %d is '%s'\n", prod_attr->prod_id,  */
/*                                                            prod_mnemon); */
        
            assoc_insert_s(product_mnemonics, prod_attr->prod_id, 
                       prod_mnemon);
            len = strlen( (char *)(prod_attr) + (prod_attr->desc + loc + 1) );
    
            if(len<=PROD_DESC_MAX) {
                
/* TEST - can be used to create the alternate list from product_info.lb */
/* fprintf(stderr,"%d %s %s %s\n",prod_attr->prod_id, prod_attr->name,  */
/*             prod_mnemon, (char *)(prod_attr)+(prod_attr->desc + loc + 1) ); */
                
                assoc_insert_s(product_names, prod_attr->prod_id, 
                       (char *)(prod_attr) + (prod_attr->desc + loc + 1));    
                       
            } else {
                
                strncpy(prod_desc, (char *)(prod_attr)+(prod_attr->desc + loc + 1), 
                                                                    PROD_DESC_MAX);
                prod_desc[PROD_DESC_MAX]= '\0';
                
/* TEST - catches and truncates long descriptions  */
/* fprintf(stderr,"%d %s %s %s\n",prod_attr->prod_id, prod_attr->name,  */
/*                                              prod_mnemon, prod_desc); */
                
                assoc_insert_s(product_names, prod_attr->prod_id, prod_desc);
            }
            
        } /*  end else a final product */
    
        
        /* now, we do the same for the short names */
/* cvg 8.5.2 */
        strcpy(prod_short, prod_attr->name);
        if(strlen(prod_short) > 31) {
            prod_short[31] = '\0';
        }
        /* assoc_insert_s(short_prod_names, prod_attr->prod_id, prod_attr->name); */
        assoc_insert_s(short_prod_names, prod_attr->prod_id, prod_short);
    
        /* go to the next record */
        offset += prod_attr->entry_size;
        
    } /*  end while */

    if(verbose_flag)
        printf("Read Product Descriptions from the configured product info LB\n");
    
    /* clean up */
    free(buf);
    
    
}


/*  CVG 8.0                                                               */
/*  no load function provided, cvg begins with default based upon whether */
/*  cvg compiled with standalone and the existence of the orpg file.      */
/*****************************************************************/     
                                                                        
void write_descript_source(int use_cvg_list)                            
{                                                                       
    char filename[150];                                                 
    FILE *desc_source_file;                                             
                                                                        
    /* open the program preferences data file */                        
    sprintf(filename, "%s/descript_source", config_dir);                
                                                                        
    if((desc_source_file=fopen(filename, "w"))==NULL) {                 
        fprintf(stderr, "**************************************\n");    
        fprintf(stderr, "*  Could not open description source *\n");    
        fprintf(stderr, "*  file.                             *\n");    
        fprintf(stderr, "*                                    *\n");    
        fprintf(stderr, "**************************************\n");    
                                                                        
    exit(0);                                                            
    }                                                                   
                                                                        
    fprintf(desc_source_file,"%d", use_cvg_list);                       
/* cvg 8.0 */
    fflush(desc_source_file);                                                                        
    fclose(desc_source_file);                                           
}                                                                       
