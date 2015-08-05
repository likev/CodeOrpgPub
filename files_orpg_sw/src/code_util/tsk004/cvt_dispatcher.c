/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:51 $
 * $Id: cvt_dispatcher.c,v 1.11 2009/05/15 17:37:51 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/* cvt_dispatcher.c  dispatch the task to do proper processing */

#include "cvt_dispatcher.h"





/* =================================================================== */
/* ================================================================== */
int dispatch_tasks(int argc,char *argv[]) {
  int i,result,value;
  int EXTRACT_flag=FALSE,OUTPUT_flag=FALSE,INPUT_flag=FALSE;
  int buffer_allocated=FALSE;
  int NOHDR=FALSE;
/*  cvt 4.3 */
  char *buffer=NULL;
  int TEST=FALSE;
  
  int alt_db = FALSE;
  
  /* for TEST only */
  char *extract_string[]={"NONE","EXTRACT","HEXDUMP",
   "BOTH EXTRACT and HEXDUMP"};

  if(TEST) {
    fprintf(stderr,"---------- Begin dispatch_tasks module ----------\n");
    fprintf(stderr,"test of arg passing: number of args=%d\n",argc);
    for(i=0;i<argc;i++) {
      fprintf(stderr,"arg %d  %s\n",i,argv[i]);
    }
  }

/* *** MISC INFORMATION SECTION *** */


  /* check and see if the quick reference screen has been requested */
  if((value=check_args(argc,argv,"h",0))>0) {
    quick_help();
    return(TRUE);
  }


  /* check and see if the complete help screen has been requested */
  if((value=check_args(argc,argv,"help",0))>0) {
    help_screen();
    return(TRUE);
  }

  /* check and see if the packet definition screen has been requested */
  if((value=check_args(argc,argv,"packets",0))>0) {
    packet_definitions();
    return(TRUE);
  }

  /* check and see if version information has been requested */
  if((value=check_args(argc,argv,"version",0))>0) {
    version_information();
    return(TRUE);
  }  

  /* determine the location of the product data base file */
  /* only if using the product database as the source     */
  if((value=check_args(argc,argv,"load",0))==0) {
      char *alt_prod_db, *data_dir;    
      if((alt_prod_db = getenv("CVT_DB")) == NULL) {
#ifdef LIB_LINK_STATIC
          fprintf(stderr,"  *************************************************\n");
          fprintf(stderr,"  *  A database file name must always be selected *\n");
          fprintf(stderr,"  *  using CVT_DB with a standalone installation. *\n");
          fprintf(stderr,"  *************************************************\n");
          exit(0);
#endif
          
          if((data_dir = getenv("ORPG_PRODUCTS_DATABASE")) == NULL) {
#ifdef LIB_LINK_DYNAMIC             
            fprintf(stderr, 
             "Environmental variable ORPG_PRODUCTS_DATABASE not set.\n");
            fprintf(stderr, 
             "     ORPG_PRODUCTS_DATABASE should be set for this ORPG account.\n");
            fprintf(stderr, 
             "     Or the use CVT_DB to select an alternate database file.\n");
            exit(0);
#endif                
          }
          
          sprintf(cvt_prod_data_base, "%s", data_dir);
          alt_db = FALSE;
          print_db_source( alt_db );

      } else { /*  we ARE using the CVT_DB variable */
          sprintf(cvt_prod_data_base, "%s", alt_prod_db);
          alt_db = TRUE;
          print_db_source( alt_db );

      }
   
  }  /* end if not loading single product file */

  /* Determine if the check for ICD product should be skipped */
  /* in other words, force load */

  if((value=check_args(argc,argv,"-f",0))>0) {
    ignore_icd_check=TRUE; 
    fprintf(stderr,
      "\nWARNING: the -f option overrides the check for a valid ICD product.\n");
    fprintf(stderr,
      "         When used, CVT may incorrectly treat an intermediate product as a final product.\n");
    fprintf(stderr,
      "         Actions normally requiring a valid product will be permitted.\n"); 
    fprintf(stderr,"         These include product load and display.\n");  

  } else {     
    ignore_icd_check=FALSE;
  }

  /* determine if decompression should be skipped for product extraction function */

  if((value=check_args(argc,argv,"-nodc",0))>0) {
    skip_decompress=TRUE;   
  } else {     
    skip_decompress=FALSE;
  }


     
     
/* *** INPUT OVERRIDE SECTION *** */
     
  /* Input/Output flags: to override input from the default linear buffer,*/
  /* the command parameter '-input' can be used. Example, the following   */
  /* indicates that product 'x' is to be extracted from linearbuffername  */
  /* and deposited in the default output location.                        */
  /*    cvt msg x extract -input linearbuffername                         */
  /*                                                                      */
  /* Output is treated the same way using the '-output' flag. Example:    */
  /*    cvt msg x extract -input mybuffer -output newbuffer               */
  /* indicates that both input and output will be redirected. If using    */
  /* subdirectories in the path, be sure to use forward slants.           */
  /* example: cv msg x extract -input buf1.lb -output base/buf2.out       */

  /* check for input override information */
  /*  add "-in" to "-input" as an option */
  if( (value=check_args(argc,argv,"-input",0))>0 ) {
     INPUT_flag=value+1;
     if(TEST) fprintf(stderr,"INPUT_flag set to %d\n",INPUT_flag);
  }
  if( (value=check_args(argc,argv,"-in",0))>0 ) {
     INPUT_flag=value+1;
     if(TEST) fprintf(stderr,"INPUT_flag set to %d\n",INPUT_flag);
  }  
   
  /* check for output override information */
  /*  add "-out" to "-output" as an option */
  if( (value=check_args(argc,argv,"-output",0))>0 ) {
     OUTPUT_flag=value+1;
     if(TEST) fprintf(stderr,"OUTPUT_flag set to %d\n",OUTPUT_flag);     
  }
  if( (value=check_args(argc,argv,"-out",0))>0 ) {
     OUTPUT_flag=value+1;
     if(TEST) fprintf(stderr,"OUTPUT_flag set to %d\n",OUTPUT_flag);     
  }    

  /* EXTRACT_flag is used to indicate whether a particular message        */
  /* should be extracted as either a binary or hexdump. if command        */
  /* line parameters 'extract' or 'hexdump' are absent, then              */
  /* EXTRACT_flag will hold a value of NONE. If 'extract' exists, then    */
  /* EXTRACT_flag will hold the value of EXTRACT. If 'hexdump' exists,    */
  /* then EXTRACT_flag will hold the value of HEXDUMP. If for some reason */
  /* both 'extract' and 'hexdump' exist in the command line, then         */
  /* EXTRACT_flag will hold the value of BOTH                             */
        
  /* check and see if there is a request to extract the product to disk */
  if((value=check_args(argc,argv,"extract",0))>0) {
    EXTRACT_flag=EXTRACT;
    if(TEST) fprintf(stderr,"EXTRACT_flag set to EXTRACT\n");
    }

  /* check and see if there is a request to produce a hex dump to disk */
  if((value=check_args(argc,argv,"hexdump",0))>0) {
    if(EXTRACT_flag==EXTRACT)
       EXTRACT_flag=BOTH;  /* if extract has already been selected -> both */
     else
       EXTRACT_flag=HEXDUMP;
    if(TEST) 
        fprintf(stderr,"EXTRACT_flag set to %s\n",extract_string[EXTRACT_flag]);
    }       

  /* check and see if there is a request to extract a product...if the flag
   *  -NOHDR is included. this will remove the first 96 bytes of the product
   *  during the extract process */
  if(EXTRACT_flag!=NONE) {
     if( (value=check_args(argc,argv,"-nohdr",0))>0 ||
         (value=check_args(argc,argv,"-NOHDR",0))>0) {
       NOHDR=TRUE;
       if(TEST) fprintf(stderr,"NO Pre ICD Header Flag Set\n");
       }
    }

/* *** PRODUCT INVENTORY SECTION *** */

  /* Inventory Modules - short circuit the function upon success */
  if((value=check_args(argc,argv,"i",0))>0) {
    if(value+1==argc) {
      /* process main product database inventory - fires when "cv i" is entered*/
      result=ORPG_Database_inventory();
    }
    else {
      /* process individual LB Inventory - fires when "cv i lbname" is entered */
      int tempval;
      if((tempval=test_bounds(argc,value+1))==BADVAL) return(FALSE);
      result=ORPG_ProductLB_inventory(argv[tempval]);
    }
      
      /*  Could repeat db source here! */
      print_db_source( alt_db );
      
    return(TRUE);
  }

  /* Search Inventory Module - if searchlb is the keyword, then look to see
   *  if the lbident code is in the main product linear buffer file. The
   *  search is limited to the products database linear buffer */
     /*  add "-slb" to "-searchlb" as an option */
  if( ((value=check_args(argc,argv,"searchlb",0))>0)  ||
      ((value=check_args(argc,argv,"slb",0))>0)      ) {
    /* a linear buffer code must follow the key word */
    int tempval;
    if((tempval=test_bounds(argc,value+1))==BADVAL) {
      fprintf(stderr,"INPUT ERROR: Expecting an integer after the keyword: searchlb\n"); 
      return(FALSE);
    }
    result=ORPG_Database_search((short)atoi(argv[tempval]));
    
    /*  Could repeat db source here! */
    print_db_source( alt_db );
    
    return(TRUE);
  }

  /* show the inventory of the products request linear buffer */
  if((value=check_args(argc,argv,"req",0))>0) {
    int tempval;
    if((tempval=test_bounds(argc,value+1))==BADVAL) {
      fprintf(stderr,"INPUT ERROR: Expecting an integer after the keyword: req\n");
      return(FALSE);
    }
    result=ORPG_requestLB_inventory(argv[tempval]);
    return(TRUE);
  }

/* *** PRODUCT LOAD SECTION *** */

  /* PRODUCT ACCESS IS ACCOMPLISHED IN TWO WAYS:
   *     1. From a linear buffer with the args: "msg X"
   *     2. From a binary file with the args: "load FNAME"
   */
   
   
   /* FIRST SOURCE - ACCESS FROM A LINEAR BUFFER */
  if((value=check_args(argc,argv,"msg",0))>0) { 
    /* the returned result in "value" contains the index of "msg" in "argv" */
    int msg_seq_num=0;   /* product LB message sequence number */
    if(TEST) fprintf(stderr,"begin product access\n");
    /* CVT 4.4 - added report non-digit as error field */
    msg_seq_num=get_int_value(argc,argv,value+1,TRUE);
    if(TEST) fprintf(stderr,"returned message sequence num=%d\n",msg_seq_num);
    if(msg_seq_num<0) {
      fprintf(stderr,"INPUT ERROR: Expecting an integer sequence number after the keyword: msg\n");
      return(FALSE);
    }

   /* if the EXTRACT_flag was set...short circuit to the extract_lb call */
   if(EXTRACT_flag>0) {
      if(INPUT_flag==0 && OUTPUT_flag==0)
         /* no input/output specified - use defaults */
         Extract_LB_Product(msg_seq_num,EXTRACT_flag,NULL,NULL,NOHDR,
                            ignore_icd_check, skip_decompress);
      else if(INPUT_flag>0 && OUTPUT_flag==0)
         /* input path is available - use default output */
         Extract_LB_Product(msg_seq_num,EXTRACT_flag,argv[INPUT_flag],NULL,NOHDR,
                            ignore_icd_check, skip_decompress);
      else if(INPUT_flag==0 && OUTPUT_flag>0)
         /* use default input - output path specified */
         Extract_LB_Product(msg_seq_num,EXTRACT_flag,NULL,argv[OUTPUT_flag],NOHDR,
                            ignore_icd_check, skip_decompress);
      else
         /* both input and output pathes have been specified */
         Extract_LB_Product(msg_seq_num,EXTRACT_flag,argv[INPUT_flag],
                            argv[OUTPUT_flag],NOHDR,ignore_icd_check, 
                            skip_decompress);
 
      return(TRUE);
    }

    /* allocate buffer and load product...returning a pointer to the filled buffer */
    buffer=Load_ORPG_Product(msg_seq_num, ignore_icd_check);
    
    if(buffer!=NULL) {
      if(TEST) fprintf(stderr,"product has been loaded into buffer \n");
      buffer_allocated=TRUE;
    }
    else {
      fprintf(stderr,"DISPATCH ERROR: failure loading product into buffer\n");
      fprintf(stderr,"                from LB message sequence number %d\n",msg_seq_num);
      buffer_allocated=FALSE;
      return(FALSE);
    }

  } /* end if "msg" */
    
    /* SECOND SOURCE - ACCESS FROM A BINARY FILE */
  else if((value=check_args(argc,argv,"load",0))>0){ 
    int file_ind, type_hdr;
    if(TEST) fprintf(stderr,"begin product access\n");
    file_ind=value+1;
    
    if(TEST) fprintf(stderr,"the binary file is: %s\n",argv[file_ind]);
    
    /* what type of product header is in the file */
    if((value=check_args(argc,argv,"-nohdr",0))>0 ||
       (value=check_args(argc,argv,"-NOHDR",0))>0) 
           type_hdr=HEADER_NONE;
    else if((value=check_args(argc,argv,"-wmohdr",0))>0 ||
       (value=check_args(argc,argv,"-WMOHDR",0))>0) 
           type_hdr=HEADER_WMO;
    else /* the default is a product with a Pre-ICD header */
        type_hdr=HEADER_PRE_ICD;

       
    /* allocate buffer and load product..returning a pointer to the filled buffer */
    buffer=load_product_file(argv[file_ind],type_hdr,ignore_icd_check);
    
    if(buffer!=NULL) {
      if(TEST) fprintf(stderr,"product has been loaded into buffer \n");
      buffer_allocated=TRUE;
    }
    else {
      fprintf(stderr,"DISPATCH ERROR: failure loading product into buffer\n");
/*      fprintf(stderr,"                for file %s\n",argv[file_ind]); */
      fprintf(stderr,"                from binary file %s\n",argv[file_ind]);
      buffer_allocated=FALSE;
      return(FALSE);
    }
    
  } 
  else {
    fprintf(stderr,"\n- INPUT ERROR: CHECK COMMAND LINE ARGUMENTS\n");
    fprintf(stderr," \n");
    fprintf(stderr," A quick command list can be obtained with 'cvt h' \n");
    fprintf(stderr," \n");
    fprintf(stderr," More extensive help is available with 'cvt help' \n");
    fprintf(stderr," \n");
    
    return(FALSE);
  }

/* *** PRODUCT ACTIONS SECTION *** */

    /* after product loaded from LB or from a file, take appropriate action */

    /* initialize the message structure (always returns true) */
    result=initialize_msg_struct(buffer);
    


    /* print out selected components - entry point to remainder of program */
    result=print_product_components(argc,argv,buffer);

  if(buffer_allocated==TRUE) free(buffer);
  return(TRUE);
  
} /* end dispatch_tasks() */



/* =================================================================== */
/* =================================================================== */
int print_product_components(int argc,char *argv[],char* buffer) {
  /* print out the product components */
  int TEST=FALSE;
  int result,i;
  int value;
  
  int print_all=FALSE;
  int print_one=FALSE;
  /* CVT 4.4 */
  int list_com_flag = FALSE;
  int layer_flag = FALSE;
  int generic_flag = FALSE;
  
  int flag[FLAG_ARRAY_SIZE];  
  /*flag array components:                                                      */
  /* see corresponding enumerated types in cvt.h                                */
  /* ----------------------------------------------------------------           */
  /* comments also in sym_block.c                                               */
  /* ----------------------------------------------------------------           */
  /*   index 0 typecode        {NOPART,RADIAL,ROW,GAB,TAB,TABV,SATAP}           */
  /*   index 1 start field     integer val of start                             */
  /*   index 2 end field       0 or integer val of end                          */
  /*   index 3 'all' flag      {FALSE,TRUE}                                     */
  /*   index 4 degree flag     {FALSE,TRUE}                                     */
  /*   index 5 format          {NOMOD,RLE,BSCAN}                                */
  /* CVG 4.4 - added -1 for generic product */
  /*   index 6 layer#          {0:all   -1:generic   or layer # in symb block}  */
  /*   index 7 scale data      {1=R, 2=0.5m/s V, 3=1m/s V, 4=SW}                */
  /* CVT 4.4 */
  /*                           (5=FDECODE, 6=PDECODE                            */
  /*   index 8 component list  {LIST_NONE,LIST_ALL,LIST_AREA,LIST_RAD           */
  /*                            LIST_TEXT,LIST_TABLE,LIST_GRID,LIST_EVENT)      */
  /*   index 9 component print {PRINT_NONE,PRINT_ALL,PRINT_AREA,PRINT_RAD       */
  /*                            PRINT_TEXT,PRINT_TABLE,PRINT_GRID,PRINT_EVENT)  */
  /*   index 10 component idx  {integer val of component index}                 */
  /*   index 11 prod param print  {FALSE,TRUE}                                  */
  /*   index 12 comp param print  {FALSE,TRUE}                                  */
  /*                                                                            */
  /* flag[0] specifies the product block (GAB, TAB, TABV) to display            */
  /*         or that the product is a radial / raster data packet.              */
  /*                                                                            */
  /* flag[1] beginning radial / row (0 is not specified)                        */
  /*                                                                            */
  /* flgg[2] end radial / row (0 is not specified)                              */
  /*                                                                            */
  /* flag[3] specifies that all radials / rows be displayed                     */
  /*                                                                            */
  /* flag[4] specified whether the radial selectors are in degrees              */
  /*                                                                            */
  /* flag[5] requsts either the bscan output of radial data (flag[5] == 2) or   */
  /*         leaving radial or raster data in RLE (flag[5] == 1).               */
  /*                                                                            */
  /* flag[6] specifies the symbology layer to display 'layer X',                */
  /*         0 means all layers, -1 means generic product 'generic'             */
  /*                                                                            */
  /* For data packet 16:                                                        */
  /* flag[7] designates data decoding for reflectivity, 0.5 m/s velocity,       */
  /*         1.0 m/s velocity and spectrum width using the formulas are from    */
  /*         the base data ICD. The default is no decoding, flag[7] == 0. Using */
  /*         the modifier 'scaleX' (scaler, scalev1, scalev2, scalesw) will     */
  /*         apply the scaling / decode routine to each bin prior to display.   */
  /* For data packet 16 and the generic radial component:                       */
  /* CVT 4.4 */
  /* flag[7] designates data decoding using the Scale Offset parameters.        */
  /*         'pdecode' stipulates using parameters from within the product      */
  /*         'fdecode' stipulates using parameters from a user supplied file    */
  /*                                                                            */
  /* flag[8] designates how many decimal places are selected for the decoded    */
  /*         value.  (-1 not specified)                                         */
  /*                                                                            */
  /* flag[9] designates which components of a generic product should be listed  */
  /*         'list_all' lists all components, other values list all components  */
  /*         of a specific type: 'list_area', 'list_rad', 'list_text',          */
  /*         'list_table', list_grid' and 'list_event'                          */
  /*                                                                            */
  /* flag[10] designates which components of a generic product should be        */
  /*         printed. 'print_all' prints all components, other values print all */
  /*         components of a specific type: 'print_area', 'print_rad',          */
  /*         'print_text', 'print_table', 'print_grid' and 'print_event'        */
  /*                                                                            */
  /* flag[11] specifies a component index (int) which can modify other options  */
  /*          (-1 not specified)                                                */
  /*                                                                            */
  /* flag[12] specifies whether the product parameters should not be printed    */
  /*          'no_pparams'                                                      */
  /*                                                                            */
  /* flag[13] specifies whether the component parameters should not be printed  */
  /*          'no_cparams'                                                      */
  /* ----------------------------------------------------------------           */
 

 
  
  
  
  
  if(buffer==NULL) {
     fprintf(stderr,"PPC ERROR: Message Buffer Failure\n");
     return(FALSE);
   }


/*=============================================================================*/
/* Section displays summary info verbose consistency check and header info     */

  /* check for summary condition: 'cvt msg X summary' which is interpreted as 
   * a request to display the summary information about the message...then exit */
  if((result=check_args(argc,argv,"summary",0))>0 
       && (result=check_args(argc,argv,"msg",0))>0
       /* CVT 4.4 - added report non-digit as error field */
       && get_int_value(argc,argv,2,TRUE)>=0) {        
    fprintf(stderr,"\nENTERING DISPLAY SUMMARY INFO MODULE\n");
     result=display_summary_info(buffer,ignore_icd_check);
     return(result);
  }
     /* check for summary condition: 'cvt load FNAME summary' which is interpreted as
      * a request to display the summary information about the message..then exit */
  if((result=check_args(argc,argv,"summary",0))>0 
        && (result=check_args(argc,argv,"load",0))>0
        && argv[2]!=NULL) {
    fprintf(stderr,"\nENTERING DISPLAY SUMMARY INFO MODULE\n");
     result=display_summary_info(buffer,ignore_icd_check);
     return(result);
  }
     

  /* if only 'cvt msg X' was entered then produce the product summary */
  if( ((result=check_args(argc,argv,"msg",0))>0 && 
       /* CVT 4.4 - added report non-digit as error field */
       get_int_value(argc,argv,2,TRUE)>=0) && 
       (argc==3 || (argc==4 && (result=check_args(argc,argv,"-f",0))>0  )) ) {
    fprintf(stderr,"\nENTERING DISPLAY SUMMARY INFO MODULE\n");
     result=display_summary_info(buffer,ignore_icd_check);
     return(result);
  }

  /* if only 'cvt load FNAME' was entered then produce the product summary */
  if( ((result=check_args(argc,argv,"load",0))>0 && argv[2]!=NULL) && 
       (argc==3 || (argc==4 && (result=check_args(argc,argv,"-f",0))>0  )) ) {
     fprintf(stderr,"\nENTERING DISPLAY SUMMARY INFO MODULE\n");
     result=display_summary_info(buffer,ignore_icd_check);
     return(result);
  }

  /* if requested, accomplish only the consistency check and return */
  if((result=check_args(argc,argv,"check",0))>0) {
     fprintf(stderr,"\nENTERING PRODUCT CONSISTENCY CHECK MODULE\n");
     result=check_offset_length(buffer+96, TRUE);
     return(result);
  }    

  /* --- print out each header if requested --- */
  /* determine if ALL header blocks should be printed */
  if((result=check_args(argc,argv,"fullhdr",0))>0) {
    print_all=TRUE;
  }
  
  /* determine if the pre-ORPG header should be printed */
  if((result=check_args(argc,argv,"hdr",0))>0 || print_all) {
     if((result=print_ORPG_header(buffer))==FALSE) {
       fprintf(stderr,"ERROR returned from the ORPG Header printing routine\n");
       return(FALSE);
     } else {
        print_one=TRUE;
     }
  }
       
    /* ******* following functions require an ICD product **************** */
    /* future change: move the test inside the individual functions        */
    
    if( ((result=check_icd_format(buffer+96, TRUE))==FALSE) &&
                                    (ignore_icd_check==FALSE) ) {
       return(FALSE);
    
    } else { /* accomplish basic checks of product block lengths and offsets */
        if((result=check_offset_length(buffer+96, FALSE))==FALSE) {
            fprintf(stderr,
                "WARNING - The Errors reported may affect the selected CVT function\n");
            fprintf(stderr,
                "          Recommend fixing the problem before further analysis with CVT\n\n");
        }
    }
       
  /* determine if the message header block should be printed */
  if((result=check_args(argc,argv,"mhb",0))>0 || print_all) {
    if((result=print_message_header(buffer))==FALSE) {
      /* only error on return is a Null buffer  */
      return(FALSE);
    } else {
        print_one=TRUE;
    }

  }

  /* determine if the product display block should be printed */
  if((result=check_args(argc,argv,"pdb",0))>0 || print_all) {
    if((result=print_pdb_header(buffer))==FALSE) {
      /* only error on return is a Null buffer  */
      return(FALSE);
    } else {
        print_one=TRUE;
    }

  }

  /* determine if the symbology block header should be printed */
  if((result=check_args(argc,argv,"sym",0))>0 || print_all) {
    if((result=print_symbology_header(buffer))==FALSE) {
      /* only error on return is a Null buffer or bad offset */
      return(FALSE);
    } else {
        print_one=TRUE;
    }

  }  

  /* if printing any one or all headers, then stop processing now */
  if(print_all==TRUE || print_one==TRUE) return(TRUE);


/*=============================================================================*/
/* Section sets the values for flag 0, 6, 8, 9, 11, and 12 for product display */
/*                (modifiers in flag 1-5 and 7 set later)                      */

  /* process layer information - fill in flag array information */

  /* with two exceptions, initialize flag array to zero/FALSE */
  for(i=0;i<FLAG_ARRAY_SIZE;i++)
     flag[i]=FALSE;
  flag[8] = -1; 
  flag[11] = -1;


  /* check for message modifiers: radial, row, tab, tabv, gab, satap,
   * and layer or generic.
   *  Populate flag[0]. If none are found then return with error */
  result=0;
  if((result=check_args(argc,argv,"radial",0))>0) 
    flag[0] = RADIAL;
  if((result=check_args(argc,argv,"row",0))>0)
    flag[0] = ROW;
  if((result=check_args(argc,argv,"tab",0))>0)
    flag[0] = TAB;
  if((result=check_args(argc,argv,"tabv",0))>0)
    flag[0] = TABV;
  if((result=check_args(argc,argv,"gab",0))>0)
    flag[0] = GAB;
  if((result=check_args(argc,argv,"satap",0))>0)
    flag[0] = SATAP;
 
  if((result=check_args(argc,argv,"layer",0))>0) {
     int tempval;
     if((tempval=test_bounds(argc,result+1))==BADVAL) 
        return(FALSE);
     /* CVT 4.4 - added report non-digit as error field */
     flag[6] = get_int_value(argc,argv,tempval,TRUE);
     /* CVT 4.4 - used MAX_LAYERS instead of 30 */
     /* CVT 4.4 - permit 'layer 0' to select all layers for display */
     if(flag[6]<0 || flag[6]>MAX_LAYERS) {
       fprintf(stderr,"INPUT ERROR: The Layer Number must be between 1 and %d\n"
                      "             or set to 0 for All Layers\n",
                                                                  MAX_LAYERS);
       return(FALSE);
     }
     if(flag[6]>18)
       fprintf(stderr,"WARNING: Layer selected exceeds 18 (the current limit),"
                      " CVT accepts up to %d\n", MAX_LAYERS);
     layer_flag = TRUE;  /* CVT 4.4 */
  } /* end if "layer" */
 
 
 
   /* CVT 4.4 */
  if((result=check_args(argc,argv,"generic",0))>0) {
     flag[6] = -1;
     generic_flag = TRUE;

     if((result=check_args(argc,argv,"list_all",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_ALL;
     }
     if((result=check_args(argc,argv,"list_area",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_AREA;
     }
     if((result=check_args(argc,argv,"list_rad",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_RAD;
     }
     if((result=check_args(argc,argv,"list_text",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_TEXT;
     }
     if((result=check_args(argc,argv,"list_table",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_TABLE;
     }
     if((result=check_args(argc,argv,"list_grid",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_GRID;
     }
     if((result=check_args(argc,argv,"list_event",0))>0) {
        list_com_flag = TRUE;
        flag[9] = LIST_EVENT;
     }
  
     /* CVT 4.4 */
     /* The following test implements the desire that if any "list" command  */
     /* is made, we do NOT  want to also print any component.  Currently     */
     /* this test is not required because all print component functions will */
     /* short cut (e.g. do not accomplish print functions after accmplishing */
     /* the component list logic).                                           */
     if(list_com_flag==FALSE) {
        flag[10]= PRINT_ALL;  /* the default action */
        
        if((result=check_args(argc,argv,"print_area",0))>0)
           flag[10] = PRINT_AREA;
        if((result=check_args(argc,argv,"print_rad",0))>0)
           flag[10] = PRINT_RAD;
        if((result=check_args(argc,argv,"print_text",0))>0)
           flag[10] = PRINT_TEXT;
        if((result=check_args(argc,argv,"print_table",0))>0)
           flag[10] = PRINT_TABLE;
        if((result=check_args(argc,argv,"print_grid",0))>0)
           flag[10] = PRINT_GRID;
        if((result=check_args(argc,argv,"print_event",0))>0)
           flag[10] = PRINT_EVENT;
        
        /* CVT 4.4 - printing a component via component index  */
        if((result=check_args(argc,argv,"print",0))>0) {
           int tempval;
           if((tempval=test_bounds(argc,result+1))==BADVAL) 
              return(FALSE);
           /* CVT 4.4 - added report non-digit as error field */
           flag[11] = get_int_value(argc,argv,tempval,TRUE);
           flag[10] = PRINT_NONE; /* override the default */
        }
           
     } /* list_com_flag is FALSE */
     
     if((result=check_args(argc,argv,"no_pparams",0))>0)
        flag[12] = TRUE;
     
     if((result=check_args(argc,argv,"no_cparams",0))>0)
        flag[13] = TRUE;
  
  } /* END if "generic" */
  
  /* special case to permit using 'radial' and omit 'generic' */
  /* PRINT_NONE can result from any of the listing option */
  /* so the real test is generic_flag==false              */
  if( flag[0]==RADIAL && flag[10]==PRINT_NONE && generic_flag==FALSE)
     flag[10] = PRINT_RAD;
  
  

  /* if no modifiers were found up to this point...end program */
  /* CVT 4.4 - replaced flag[6]==0 with layer_flag==FALSE and generic_flag==FALSE */
  /* CVT 4.4 - added test for flag[9] and flag[10]                                 */
  if( flag[0]==0 && layer_flag==FALSE && generic_flag==FALSE && 
      print_all==FALSE && flag[9]==0 && flag[10]==0 ){
    return(FALSE);
  }


/*=============================================================================*/
/* Section displays traditional optional blocks GAB. TAB and the SATAP         */

  /* if flag[0] contains a SATAP request then process */
  if(flag[0]==SATAP) {
    display_SATAP(buffer);
    return(TRUE);
  }

  /* if flag[0] contains a TAB request then process */
  if((flag[0]==TAB) || (flag[0]==TABV)) {   
    int offset=96+(2*md.tab_offset);
    if(md.tab_offset==0) {
       fprintf(stderr,"TAB Not Present, No Action Taken\n");
       return(FALSE);   
    }
    if(md.tab_offset>400000) {
       fprintf(stderr,"TAB Offset Exceeds Limits, No Action Taken\n");
       return(FALSE);
    } 
    if(flag[0]==TAB)   
       display_TAB(buffer,&offset,FALSE);
    if(flag[0]==TABV)   
       display_TAB(buffer,&offset,TRUE);       

    return(TRUE);

  } /* end if TAB or TABV */

  /* if flag[0] contains a GAB request then process */
  if(flag[0]==GAB) {
    int offset=96+(2*md.gab_offset);
    if(md.gab_offset==0) {
       fprintf(stderr,"GAB Not Present, No Action Taken\n");
       return(FALSE);   
    }
    if(md.gab_offset>400000) {
       fprintf(stderr,"GAB Offset Exceeds Limits, No Action Taken\n");
       return(FALSE);
    }

    display_GAB(buffer,&offset);
    return(TRUE);

  }

/*=============================================================================*/
/* Section processes modifiers for 'radial' and 'row' and sets flags 1-5 and 7 */

  /* if row or radial exists, check for modifying values after them.
     the first arg can be 'all'. if it is not then it must be followed
     by one or more integers (start and end fields) */
  if(flag[0]==RADIAL || flag[0]==ROW) {
    /* check to see if the keyword 'all' exists */
    value=check_args(argc,argv,"all",0);
    if(value>0) {
       /* set the all flag */
       flag[3]=TRUE;
    
    } else {
      /* check to see if the field AFTER 'radial' or 'row' is an integer */
      if(flag[0]==RADIAL) {
         value=check_args(argc,argv,"radial",0);
      } else {
         value=check_args(argc,argv,"row",0); 
      } 
      /* CVT 4.4 - added report non-digit as error field */
      result=get_int_value(argc,argv,value+1,FALSE);
      if(result>0) {
         flag[1]=result;
         /* attempt to see if the end field is next */
         /* CVT 4.4 - added report non-digit as error field */
         result=get_int_value(argc,argv,value+2,FALSE);
         if(result>0) {
            flag[2]=result;
         } else {
            flag[2]=0;
         }
      
      } else { /*  did not get an integer argument after radial or row */
         flag[1]=0;  
      }
      
    } /* end else */
    
  } /* end if RADIAL or ROW */


  if(TEST) fprintf(stderr,"PPS Report: typecode=%i start=%i end=%i all flag=%i\n",
     flag[0],flag[1],flag[2],flag[3]);
  
  /* check format specifier: none, bscan, rle */
  if((result=check_args(argc,argv,"rle",0))>0)
    flag[5]=RLE;
  else if((result=check_args(argc,argv,"bscan",0))>0)
    flag[5]=BSCAN;
  else
    flag[5]=NOMOD;
    
  /* check to see if the 'deg' modifier is in the arg list */
  if((result=check_args(argc,argv,"deg",0))>0 || 
     (result=check_args(argc,argv,"degree",0))>0)
    flag[4]=TRUE;
   else
    flag[4]=FALSE;

  /* check for scale conversion modifiers */
  if((result=check_args(argc,argv,"scaler",0))>0) 
    flag[7]=REFL;
  else if((result=check_args(argc,argv,"scalev1",0))>0)
    flag[7]=VEL1;
  else if((result=check_args(argc,argv,"scalev2",0))>0)
    flag[7]=VEL2;
  else if((result=check_args(argc,argv,"scalesw",0))>0)
    flag[7]=SW;
/* CVT 4.4 - added PDECODE and FDECODE */
  if((result=check_args(argc,argv,"fdecode",0))>0)
    flag[7]=FDECODE;
  else if((result=check_args(argc,argv,"pdecode",0))>0)
    flag[7]=PDECODE;
  else
    flag[7]=NOSCALE;

  /* CVT 4.4 =========================================================== */
  /* test for number of decimal places modifier after fdecode or pdecode */
  if(flag[7]==PDECODE || flag[7]==FDECODE) {
     /* check to see if the field AFTER 'fdecode' or 'pdecode' is an integer */
     if(flag[7]==PDECODE) {
        value=check_args(argc,argv,"pdecode",0);
     } else {
        value=check_args(argc,argv,"fdecode",0); 
     } 
     /* CVT 4.4 - added report non-digit as error field */
     result=get_int_value(argc,argv,value+1,FALSE);
     if(result>=0) {
        if(result > 6) {
           flag[8] = 6; /* the maximum allowed */
        } else 
           flag[8]=result;
     
     } else { /*  did not get an integer argument after pdecode or fdecode */
        flag[8]=-1;  
     }
  } /* end if PDECODE or FDECODE */
  
  
  
  
  /* quality control flag consistency prior to continuing */
  if(flag[0]==ROW) {
    if(flag[4]==TRUE) {
      fprintf(stderr,"INPUT ERROR: You cannot use a 'deg' modifier with a ROW\n");
      return(FALSE);
    }
    if(flag[5]==BSCAN) {
      fprintf(stderr,"INPUT ERROR: You cannot use a 'bscan' modifier with a ROW\n");
      return(FALSE);
    }
  }




  /* TEST write contents of flag array for qc */
  if(TEST) qc_flag_array(flag);
  
  
  /*===========================================================================*/
  /* Section is fall through to print symbology block                          */
  
  result=print_symbology_block(buffer,flag);
   
  return(result);
  
} /* end print_product_components() */





/* =================================================================== */
/* =================================================================== */
void print_db_source( int alternate_db_used ) {
      if(alternate_db_used == FALSE) {
          fprintf(stderr, 
              "\n    Using the account's product data base:\n        %s\n", 
                                                         cvt_prod_data_base);
          fprintf(stderr, 
              "    defined by the ORPG_PRODUCTS_DATABASE environmental variable\n"
              "        An alternate data base can be specified with the CVT_DB variable.\n");   
              
      } else {
          alternate_db_used = TRUE;
          fprintf(stderr, 
              "\n    Using an alternate product database:\n        %s\n", 
                                                        cvt_prod_data_base);
          fprintf(stderr, 
              "     defined by the CVT_DB variable\n\n");  
              
      }
              
} /* end print_db_source() */



/* =================================================================== */
/* ==================================================================== */
void qc_flag_array(int *flag) {
  /* for qc purposes write out the flag array */
  /* THE FOLLOWING MUST AGREE WITH cvt.h */
  char *flag0_string[]={"NOPART","RADIAL","ROW","GAB","TAB","TABV","SATAP"};
  char *flag3_string[]={"FALSE","TRUE"};
  char *flag5_string[]={"NOMOD","RLE","BSCAN"};
  /* CVT 4.4 -  added FDECODE and PDECODE */
  char *flag7_string[]={"NOSCALE","REFL","VEL1","VEL2","SW","FDECODE","PDECODE"};
  /* CVT 4.4 */
  char *flag9_string[]={"LIST_NONE","LIST_ALL","LIST_AREA","LIST_RAD",
                        "LIST_TEXT","LIST_TABLE","LIST_GRID","LIST_EVENT"};
  char *flag10_string[]={"PRINT_NONE","PRINT_ALL","PRINT_AREA","PRINT_RAD",
                        "PRINT_TEXT","PRINT_TABLE","PRINT_GRID","PRINT_EVENT"};
  

  fprintf(stderr,"\n=====FLAG Array Contents=====\n");
  fprintf(stderr,"Element  0: Type Code = %i or %s\n",flag[0],
                  flag0_string[flag[0]]);
  fprintf(stderr,"Element  1: Start Field=%i\n",flag[1]);
  fprintf(stderr,"Element  2: End Field=  %i\n",flag[2]);
  fprintf(stderr,"Element  3: All Flag=   %i or %s\n",flag[3],
                  flag3_string[flag[3]]);
  fprintf(stderr,"Element  4: Degree Flag=%i or %s\n",flag[4],
                  flag3_string[flag[4]]);
  fprintf(stderr,"Element  5: Output Fmt =%i or %s\n",flag[5],
                  flag5_string[flag[5]]);
  /* CVT 4.4 - added custom print for 0 and -1 */
  if(flag[6] > 0)
     fprintf(stderr,"Element  6: Layer Nbr = %i\n",flag[6]);
  else if(flag[6] == 0)
     fprintf(stderr,"Element  6: All Layers Selected (%i)\n",flag[6]);
  else if(flag[6] == -1)
     fprintf(stderr,"Element  6: Generic Prod Selected (%i)\n",flag[6]);
     
  fprintf(stderr,"Element 7: Scale Flag= %i or %s\n",flag[7],
                  flag7_string[flag[7]]);
  /* CVT 4.4 */
  if(flag[8] == -1)
     fprintf(stderr,"Element  8: (Decimal Places Not Specified) \n");
  else 
     fprintf(stderr,"Element  8: %d decimal places selected \n",flag[8]);
  
  fprintf(stderr,"Element  9: comp list  = %i or %s\n",flag[9],
                  flag9_string[flag[9]]);
  fprintf(stderr,"Element 10: comp print = %i or %s\n",flag[10],
                  flag10_string[flag[10]]);
  
  if(flag[11] == -1)
     fprintf(stderr,"Element 11: (Component Index Not Specified) \n");
  else 
     fprintf(stderr,"Element 11: Component %d selected \n",flag[11]);
  
  fprintf(stderr,"Element 12: omit p params=%i or %s\n",flag[12],
                  flag3_string[flag[12]]);
  fprintf(stderr,"Element 13: omit c params=%i or %s\n",flag[13],
                  flag3_string[flag[13]]);
  
  fprintf(stderr,"\n");
  return;
  
} /* end qc_flag_array() */




/* =================================================================== */
/* =================================================================== */
int initialize_msg_struct(char *buffer) {
  /* initialize the global message data structure */
  Prod_header *hdr;
  Graphic_product *gp;
  Sym_hdr *sh;
  int TEST=FALSE;

  hdr = (Prod_header*)buffer;
  gp = (Graphic_product*)(buffer+96);
  sh = (Sym_hdr*)(buffer+216);
  
  
  /* CVG 4.4 set message_length  */
  md.ptr_to_product = (short*) (buffer+96);
  md.message_length = hdr->g.len; /* needs pre-icd header - swap not required */
  md.hdr_prod_id = hdr->g.prod_id;
  /* CVG 4.4 set product_length */
  md.product_length = gp->msg_len;  
  /* CVG 4.4 - used the length in the product + 96 rather than */
  /*           the len in the pre-icd header */
  md.total_length = gp->msg_len + 96;
  /* CVT 4.4 - set prod_code and prod_id */
  md.prod_code = gp->msg_code;
  if(md.prod_code<=130)
    md.prod_id = prod_code_to_id(md.prod_code);
  else
    md.prod_id = md.prod_code;
  md.n_blocks = gp->n_blocks;
  md.symb_offset = gp->sym_off;
  md.gab_offset = gp->gra_off;
  md.tab_offset = gp->tab_off;
  md.num_layers = sh->n_layers;
/* LINUX change */
#ifdef LITTLE_ENDIAN_MACHINE
  /* CVT 4.4 swap total_length and product_length */
  md.product_length = INT_BSWAP(md.product_length);
  md.total_length = md.product_length + 96;
  /* CVT 4.4 swap prod_code and prod_id */
  md.prod_code = SHORT_BSWAP(md.prod_code);
  if(md.prod_code<=130)
    md.prod_id = prod_code_to_id(md.prod_code);
  else
    md.prod_id = md.prod_code;
  md.n_blocks = SHORT_BSWAP(md.n_blocks);
  md.symb_offset = INT_BSWAP(md.symb_offset);
  md.gab_offset = INT_BSWAP(md.gab_offset);
  md.tab_offset = INT_BSWAP(md.tab_offset);
  md.num_layers = SHORT_BSWAP(md.num_layers);
#endif
  

  
  if(TEST) {
    fprintf(stderr,"---------- msg_structure block -----------\n");
    fprintf(stderr,"FROM INTERNAL 96-BYTE HEADER: \n");
    fprintf(stderr,"message length=      %d\n",md.message_length);
    fprintf(stderr,"header prod id=      %d\n",md.hdr_prod_id);
    fprintf(stderr,"FROM / DERIVED FROM MHB: \n");
    fprintf(stderr,"product code=        %d\n",md.prod_code);
    fprintf(stderr,"product id=          %d\n",md.prod_id);
    fprintf(stderr,"total length=        %d\n",md.total_length);
    fprintf(stderr,"product length=      %d\n",md.product_length);
    fprintf(stderr,"number of blocks=    %hd\n",md.n_blocks);
    fprintf(stderr,"FROM / Product Description Block PDB: \n");
    fprintf(stderr,"symbolgy offset=     %d\n",md.symb_offset);
    fprintf(stderr,"graphic alpha offset=%d\n",md.gab_offset);
    fprintf(stderr,"tabular alpha offset=%d\n",md.tab_offset);
    fprintf(stderr,"number of sym layers=%hd\n",md.num_layers);
  }
  return(TRUE);
  
} /* end initialize_msg_struct() */



/* =================================================================== */
/* =================================================================== */
/* CVT 4.4 - added report non-digit as error field */
int get_int_value(int argc,char *argv[],int index, int non_digit_error) {
  /* given the argv index, attempt to return an integer value.
    * a negative return means failure */
  int i;
  int TEST=FALSE;

  if(TEST)
    fprintf(stderr,"inside get_int_value: index=%d  argc=%d\n",index,argc);


  /* first check to be sure that it is in bounds */
  if(index>=argc) {
    /*fprintf(stderr,"ERROR: a command line argument does not exist to index %d\n",
              index);*/
    return(BADVAL);
  }

  if(TEST)
    fprintf(stderr,"word = %s   length=%d\n",argv[index],strlen(argv[index]));

  /* check to be sure that the indexed value is an integer     */
  /* This function should only be used after obtaining the     */
  /* radial / row specification arguments, the layer argument, */
  /* the msg argument, and (cvt 4.4) the fdecode pdecode arguments */
  for(i=0;i<strlen(argv[index]);i++) {
        
    if((isdigit((int)argv[index][i])==0) && /* the remaining tests are not really */
                                            /* needed.  They just catch using the */
                                            /* function after inappropriate args  */
          (strcmp(argv[index-1],"deg")!=0) &&
          (strcmp(argv[index-1],"rle")!=0) &&
          (strcmp(argv[index-1],"scaler")!=0) &&
          (strcmp(argv[index-1],"scalev1")!=0) &&
          (strcmp(argv[index-1],"scalev2")!=0) &&
          (strcmp(argv[index-1],"scalesw")!=0) &&
          /* cvt 4.4 - support for generic modifiers */
          (strcmp(argv[index-1],"generic")!=0) &&
          (strcmp(argv[index-1],"bscan")!=0)  ) {
        /* CVT 4.4 */
        if(non_digit_error==TRUE)
           fprintf(stderr,"ERROR: argv[%d]=%s contains non-digit characters\n",
                                                              index,argv[index]);
        return(BADVAL);
     }
     
  }  

   return(atoi(argv[index]));
   
} /* end get_int_value() */



/* =================================================================== */
/* =================================================================== */
int check_for_msg_modifier(int argc,char *argv[]) {
  /* run through the list of args and determine if one of the approved
   command line values has been included. these include: hdr, mhb, pdb, fullhdr,sym,
   radial,row,column */
   int result,found=FALSE;

   if((result=check_args(argc,argv,"hdr",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"mhb",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"pdb",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"fullhdr",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"sym",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"radial",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"row",0))>0) found=TRUE;
   /* CVT 4.4 */
   if((result=check_args(argc,argv,"layer",0))>0) found=TRUE;
   if((result=check_args(argc,argv,"generic",0))>0) found=TRUE;

   return(found);

} /* end check_for_msg_modifier() */




/* =================================================================== */
/* ==================================================================== */
int check_args(int argc,char *argv[],char *param,int startval) {
  /* check the command line arguments for input parameters.
     If the parameter IS in the list, then return the positive
     index of the parameter, otherwise return 0 */
  int TEST=FALSE;
  int i;

  if(TEST)
    fprintf(stderr,"inside check_args: input string=%s  strlen=%i st=%i end=%i\n",
     param,strlen(param),startval,argc);

  for(i=startval;i<argc;i++) {
      /* using a substring compare was a problem, could conflict with filenames  */
      if(strcmp(argv[i],param)==0){
         if(TEST) fprintf(stderr,"return double i=%i\n",i);
         return(i);
      }

  } /* end i loop */
  if(TEST) fprintf(stderr,"returning false\n");
  
  return(FALSE);
  
} /* end check_args() */



/* =================================================================== */
/* ==================================================================== */
int test_bounds(int argc,int value) {
  /* test whether or not value is >= argc. If so, then return
     an error condition */

  if(value<argc) return(value);

  /* error condition */
  fprintf(stderr,"BOUNDS ERROR: input value is beyond argv bounds\n");
  return(BADVAL);


} /* end test_bounds() */

