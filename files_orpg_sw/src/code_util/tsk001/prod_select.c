/* prod_select.c */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/10 20:23:45 $
 * $Id: prod_select.c,v 1.9 2012/01/10 20:23:45 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
 
#include "prod_select.h"
#include "byteswap.h"


/*  new list message */
char list_fname[256];


/* CHANGE *prod_list_size to list_header->prod_list_size?? */
int *prod_list_size=NULL; 

int num_prod_disp=0;  

static int *msg_num_filtered_list;

/* only used for temporary debug purposes: */
/* static unsigned int *sort_time_filtered_list; */

  char *diskfile_saved_icd_product = NULL, *diskfile_saved_lb_filename = NULL;
  void *diskfile_saved_generic_prod_data = NULL;

  int diskfile_pid;





/* this function initializes the statically size array structures based upon 
 * an externally defined value.  */
void init_db_prod_list()
{
    int max_list_size;
    
    
    
    
    /*  NOTE: 112 is based upon the db_entry_string[112] in global2.h */
    /*  PRODUCT LIST MESSAGE ////////////// */
    /* The current item text (112 chars) is made up of
     *
     *  when using date-time for sort and volume number filter:
     *      1 space
     *     11 chars for date-time string
     *      2 space
     *      2 Vol number
     *      2 space
     *      2 Elev number
     *      2 space
     *      4 ProdID
     *      4 space
     *      3 chars for name string
     *      2 space  CHANGED 1 TO 2
     *      4 Pcode
     *      2 space
     *     71 chars available for the Prod Description string    
     *   
     *  when using volume sequence number for sort and filter:
     *     11 chars for date-time string
     *      2 space
     *      5 Vol Seq number (1-32768)
     *      2 space
     *      2 Elev number
     *      2 space
     *      4 ProdID
     *      3 space  CHANGED 2 TO 3
     *      3 chars for name string
     *      2 space  CHANGED 1 TO 2
     *      4 Pcode
     *      2 space
     *     70 chars available for the Prod Description string
     *
     * The longest PROD description that exists is 62 characters for LRA & LRM 
     *     the limit is 69 characters plus string terminator '\0'
     *
     *
     * Affected: process_list_item() in read_database.c and
     *           filter_prod_list() in prod_select.c
     */
    
    
    
    
    if(prod_list_msg==NULL) {  
        max_list_size = maxProducts+1;

        list_msg_size = 8 + (max_list_size*112) + (max_list_size*sizeof(int)) + 
                     (max_list_size*sizeof(short)) + (max_list_size*sizeof(short));
                     
        descript_off = 8;
        sort_time_off = descript_off + (max_list_size*112);
        msg_num_off = sort_time_off + (max_list_size*sizeof(int));

        elev_num_off = msg_num_off + (max_list_size*sizeof(short));
    
        prod_list_msg = (char *) malloc( list_msg_size );
        
        msg_num_filtered_list = (int *) malloc( (maxProducts+1)*sizeof(int) );


    
        product_list = (db_entry_string *) (prod_list_msg + descript_off);
        msg_num_total_list = (short *) (prod_list_msg + msg_num_off);
       /* the complete volume date-time used in animation */
       sort_time_total_list = (unsigned int *) (prod_list_msg + sort_time_off);
       /* the elevation number used in animation */
       elev_num_total_list = (short *) (prod_list_msg + elev_num_off);

    }

    /* dfine the list filename*/
    sprintf(list_fname, "%s/cvg_db_list.lb", config_dir);     
    
    
} /*  end init_db_prod_list */













/****************************************************************************/
/* the following function: 1) updates the product list, 2) filters the list, 
   and 3) displays the list.
*/
void build_list_Callback(Widget w,XtPointer client_data,XtPointer call_data) {

   build_list();  

}



void build_list() {

    int result=FALSE;    

    int init_type;

/*  need to stop database animation before updating the list */

/*  If no database animation mode has been initialized, a FULL_INIT is signaled, */
/*  otherwize an UPDATE_LIST_INIT is used to retain history and initial volume. */

/* DEBUG */
/* fprintf(stderr,"DEBUG BUILD LIST - es_first_index_s1 is %d, first_index_num_s1 is %d\n", */
/*                                    es_first_index_s1, first_index_num_s1); */

/* / used new global animation structure variables */
                                   
    if(anim1.anim_type != FILE_SERIES) {
        anim1.stop_anim = TRUE;
        if(anim1.es_first_index == -1 && anim1.first_index_num == -1)
            init_type = ANIM_FULL_INIT;
        else
            init_type = ANIM_UPDATE_LIST_INIT;
        
        reset_elev_series(SCREEN_1, init_type);
        reset_time_series(SCREEN_1, init_type);
        reset_auto_update(SCREEN_1, init_type);
    }

/*  DEBUG */
/* fprintf(stderr,"DEBUG BUILD LIST - anim init_type is %d\n", init_type); */
    
    if(anim2.anim_type != FILE_SERIES) {
        anim2.stop_anim = TRUE;
        if(anim2.es_first_index == -1 && anim2.first_index_num == -1)
            init_type = ANIM_FULL_INIT;
        else
            init_type = ANIM_UPDATE_LIST_INIT;   
                 
        reset_elev_series(SCREEN_2, init_type);
        reset_time_series(SCREEN_2, init_type);
        reset_auto_update(SCREEN_2, init_type);
    }

    if(anim3.anim_type != FILE_SERIES) {
        anim3.stop_anim = TRUE;
        if(anim3.es_first_index == -1 && anim3.first_index_num == -1)
            init_type = ANIM_FULL_INIT;
        else
            init_type = ANIM_UPDATE_LIST_INIT;   
                 
        reset_elev_series(SCREEN_3, init_type);
        reset_time_series(SCREEN_3, init_type);
        reset_auto_update(SCREEN_3, init_type);
    }

    result = new_update_prod_list();
    
    if(result==TRUE)
        filter_prod_list();      
     
}




int new_update_prod_list()
{
int list_lbfd=-1;    
int length;
Prod_list_hdr *list_header;

int i=0, j;
   char buf[8];
   XmString xmstr1;

   char prod_info[10];
   XmString prod_info_xmstr;

/* DEBUG */
/* fprintf(stderr, "DEBUG NEW LIST- Entering new_update_prod_list\n"); */

    if(prod_list_msg==NULL) {
        fprintf(stderr,"ERROR BUILD LIST - prod_list_msg not initialized\n");  
        return (FALSE);
    }

    /*  remove contents of the product info panel (if any) */
    sprintf(prod_info,"     ");
    prod_info_xmstr = XmStringGenerate((XtPointer)prod_info, 
                       "tabfont", XmCHARSET_TEXT, NULL);
    XtVaSetValues(prod_info_label,
        XmNlabelString,     prod_info_xmstr,
        NULL);    
    XmStringFree(prod_info_xmstr);


    list_lbfd = LB_open (list_fname, LB_READ, NULL);
    if (list_lbfd < 0) {
         fprintf (stderr,"Failure to OPEN list buffer file: %s (LB_open ret = %d)\n", 
                  list_fname, list_lbfd);
         fprintf (stderr,"1. Press 'Update List & Filter' Button; 2. Restart CVG\n");
         
	     return (FALSE);
	}
/* DEBUG */
/* fprintf(stderr, "DEBUG NEW LIST - List Linear buffer opened, name %s\n", */
/*                list_fname);  */
       
    length = LB_read(list_lbfd, prod_list_msg, list_msg_size, 208);
    
    if(length != list_msg_size)
        fprintf(stderr,"ERROR, The size of the database list message %d. is not as expected (%d)\n"
                       "       this could be a result of corrupted configuration files or an\n"
                       "       incomplete installation of CVG. Ref Vol 1 guidance on installation.\n",
             length, list_msg_size);
    
    /*  future only open and close once */
    LB_close(list_lbfd);     


    list_header = (Prod_list_hdr *) prod_list_msg;

/* DEBUG */
/* fprintf(stderr, "DEBUG NEW LIST - maximum list size is %d, number of products is %d\n", */
/*              list_header->max_list_size, list_header->prod_list_size);   */


    /* existing variables used in display_list, filter_list, browse_list */
    prod_list_size = &list_header->prod_list_size;

 
    if(list_header->prod_list_size<=0) {
        
        fprintf(stderr,"******************Current Database Product List is Empty******************\n");
        fprintf(stderr,"This may be normal if the ORPG is running with no input data or an ORPG\n     clean start has been performed. Start data ingest.\n");
        fprintf(stderr,"This could reflect an incorrectly configured database in CVG prefs.\n     Double check preferences.\n");
        fprintf(stderr,"Confirm that the selected product database file exists and that you have\n     both read and write privileges.\n");
        fprintf(stderr,"Take appropriate corrective action and press the 'Update List & Filter' button.\n");
        fprintf(stderr,"******************Current Database Product List is Empty******************\n");
                
    }
       

    if(list_header->prod_list_size==0) {
               
        build_msg_list(0);
       
        return (FALSE);
    }
    

   if(list_header->prod_list_size==-1) {
    
       build_msg_list(-1);
    
       return (FALSE);
    
   }

    if(list_header->prod_list_size==-2) {
               
        build_msg_list(-2);
       
        return (FALSE);
    }
    

   if(list_header->prod_list_size==-3) {
    
       build_msg_list(-3);
    
       return (FALSE);
    
   }



   /* NOTE: these are set in init_db_prod_list */
   /* product_list = (db_entry_string *) (prod_list_msg + descript_off); */
   /* msg_num_total_list = (short *) (prod_list_msg + msg_num_off); */
   /* sort_time_total_list = (unsigned int *) (prod_list_msg + sort_time_off); */
   /* elev_num_total_list = (short *) (prod_list_msg + elev_num_off); */

/* DEBUG              */
/* fprintf(stderr, "DEBUG NEW LIST - description of message 1 is:\n     %s\n", */
/*              product_list[0]);    */



/*  FREE PREVIOUS LIST  */
    if (xmstr_prodb_list != NULL) {          

        /* ML FIX it looks like we should free one more but don't try */
        for(i=0; i<last_prod_list_size; i++) 
            XmStringFree(xmstr_prodb_list[i]);
        
        XtFree((char *)xmstr_prodb_list); 
        xmstr_prodb_list=NULL;        
    }



    /*  build XmString list here */
    sprintf(buf, "%d", list_header->prod_list_size);
    xmstr1 = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(num_prod_label, XmNlabelString, xmstr1, NULL);
    XmStringFree(xmstr1);

    /* allocate memory for the composite string list */
    xmstr_prodb_list = (XmString*) XtMalloc( sizeof(XmString) * 
                                             (list_header->prod_list_size+1) );
  
    for(j=0; j<list_header->prod_list_size; j++) {
     
       xmstr_prodb_list[j] = XmStringCreateLtoR(product_list[j], "tabfont");  
              
    } /* end for */
    
    last_prod_list_size = list_header->prod_list_size;        

    return (TRUE);   
    
} /*  end new_update_prod_list */
















/****************************************************************************/
/* the following function:  1) filters the list and 2) displays the list.
*/
void filter_list_Callback(Widget w,XtPointer client_data,XtPointer call_data) {

    filter_prod_list();

}




/* the following function: 1) filters the list, 
   and 2) displays the list.
*/
void filter_prod_list() {

   int k;
   char *pid_str, *vol_str, *pcode_str;
   int  vol, ele, pid, pcode;
   char dtime[20];
   int fpid=0, fvol, fpcode=0;
   char buf[8];
   
   int len, j;
   char *i_mnemon,   /*  input mnemonic string (max length 3) from button */
        c_mnemon[4], /*  filter string of length 3 used for comparison     */
       mnemonic[10], /*  mnemonic read from the list string - includes extra blanks */
      mnemonic_p[4]; /*  mnemonic of length 3 parsed from list */
   char *token;   
   
   XmString xmstr2;

   char prod_info[10];
   XmString prod_info_xmstr;
 
   
/* --------------------------------------------------------------------*/
 


/* DEBUG */
/* fprintf(stderr,"DEBUG - entering filter_prod_list\n");        */

   for(k=0;k<=10;k++)
       dtime[k]='\0';
   for(k=0;k<=20;k++)
       mnemonic[k]='\0';


   if(xmstr_prodb_list==NULL) {
       fprintf(stderr,"Filter List - no list to filter, xmstr_prodb_list is NULL\n");    
       return;
   }

   if(product_list==NULL) {
       fprintf(stderr,"Filter List - no list to filter, product_list is NULL\n");    
       return;
   }
   
   if(*prod_list_size==0) {
       fprintf(stderr,"Filter List - on products to filter, list size is 0\n");    
       return;
    
   }


   /*  remove contents of the product info panel (if any) */
   sprintf(prod_info,"     ");
   prod_info_xmstr = XmStringGenerate((XtPointer)prod_info, 
                       "tabfont", XmCHARSET_TEXT, NULL);
   XtVaSetValues(prod_info_label,
       XmNlabelString,     prod_info_xmstr,
       NULL);    
   XmStringFree(prod_info_xmstr);


   /* FILTER AND DISPLAY PRODUCT LIST */

   /* A. read the volume number filter if any */
   XtVaGetValues(vol_text, XmNvalue, &vol_str, NULL);  
   /*  converting numerical values gets rid of spaces (if any)  */
   fvol = atoi(vol_str);
   XtFree(vol_str);   
   
   /* B-1 if using product id, read it and set mnemonic to empty string */
   if(prod_filter == FILTER_PROD_ID) {   
      XtVaGetValues(prodid_text, XmNvalue, &pid_str, NULL);
      fpid = atoi(pid_str);
      XtFree(pid_str);
      c_mnemon[0] = '\0';
      fpcode = 0; 
   
   } else if(prod_filter == FILTER_P_CODE) { 
      XtVaGetValues(prodid_text, XmNvalue, &pcode_str, NULL);
      fpcode = atoi(pcode_str);
      XtFree(pcode_str);   
      c_mnemon[0] = '\0';
      fpid = 0; 
      
   } else if(prod_filter == FILTER_MNEMONIC) {   
      /* B-2 else if using mnemonic, read it and set product id to 0 */
      XtVaGetValues(prodid_text, XmNvalue, &i_mnemon, NULL);
      fpid = 0;
      fpcode = 0;
      
      /*  a simple substring compare would not be reliable so */
      /*  we strip leading spaces and pad with spaces to length 3 */
      
      /* strips off any spaces */
      if ((token = strtok (i_mnemon, " ")) == NULL) {
          c_mnemon[0] = '\0';                  

      } else {
              
          strcpy(c_mnemon, token);
          len = strlen(c_mnemon);
      
          /* as a convenience, convert lower case characters to upper case */
          for(j=0; j < len; j++) {
              if( (c_mnemon[j] >= 97) && (c_mnemon[j] <= 122)  )
                  c_mnemon[j] -= 32; 
          }     
          
          if( len != 3 ) { /*  pads spaces to len 3 */
      
              for(j=len ; j<3; j++)
                  c_mnemon[j] = ' ';
              c_mnemon[3] = '\0';
          }
      } /*  end else token is not NULL */

       XtFree(i_mnemon);
   
   } /*  end if filter mnemonic  */


/* --------------------------------------------------------------------*/
/***** then apply external filters */


   XmListDeleteAllItems(db_list);

   num_prod_disp = 0;


   /* NOTES FOR FOLLOWING LOGIC */
   /* 1. Can not filter on more than one of: prod_id, pcode, and mnemonic at 
    * the same time */
   /* 2. Assumption: No numeric filter has a value of 0 (can't filter on 0) */
   /* 3. Assumption: No mnemonic filter has a string length of 0 */
   /* 4. the date-time and mnemonic strings are read as a number of characters,
    * based upon the list entry structure.  Reading all item components as tokens
    * would allow modifidcation of the spacing between components of a list item
    */
   
   if( (fpid==0) && (fvol==0) && (fpcode==0) && (strlen(c_mnemon) == 0) ) { 
/* fprintf(stderr,"DEBUG - in first, no filter branch\n"); */
        for(k=0; k<*prod_list_size; k++) { 
            XmListAddItem(db_list, xmstr_prodb_list[k], 0);            
            num_prod_disp++;
            msg_num_filtered_list[num_prod_disp] = msg_num_total_list[k+1];           
/*  TEST sort_time_total_list             */
/*  sort_time_filtered_list[num_prod_disp] = sort_time_total_list[k+1]; */
            
        }
        
   } else if( ((fpid!=0) || (fpcode!=0)) && (fvol==0) ) { 
/* fprintf(stderr,"DEBUG - in second, filter prodid/code branch\n");     */
        for(k=0; k<*prod_list_size; k++) {
            sscanf(product_list[k],"%12c%d%d%d%7c%d",
                   dtime, &vol, &ele, &pid, mnemonic, &pcode);
/* DEBUG */
/* if(k <= 10) */
/* fprintf(stderr,"DEBUG mnemoninc - time='%s', vol=%d, pid=%d, menmonic='%s', pcode=%d\n", */
/*                  dtime, vol, pid, mnemonic, pcode); */

            if( ((fpid!=0) && (pid==fpid)) ||
                ((fpcode!=0) && (pcode==fpcode)) ) {
               XmListAddItem(db_list, xmstr_prodb_list[k], 0);
               num_prod_disp++;
               msg_num_filtered_list[num_prod_disp] = msg_num_total_list[k+1];
            }         
        }
        
   } else if( (strlen(c_mnemon) != 0) && (fvol==0) ) { 
/* fprintf(stderr,"DEBUG - in third, filter mnemonic branch\n");  */
        for(k=0; k<*prod_list_size; k++) {
            
            sscanf(product_list[k],"%12c%d%d%d%7c%d",
                   dtime, &vol, &ele, &pid, mnemonic, &pcode);
/* DEBUG */
/* if(k <= 10) */
/* fprintf(stderr,"DEBUG mnemoninc - time='%s', vol=%d, pid=%d, menmonic='%s', pcode=%d\n", */
/*                  dtime, vol, pid, mnemonic, pcode); */

            token = strtok (mnemonic, " ");
            if(token == NULL) {
                mnemonic_p[0] = '\0';
                
            } else {
                strcpy(mnemonic_p, token);
            }
/* DEBUG */
/* if(k <= 10) */
/* fprintf(stderr,"DEBUG mnemoninc - token = '%s', mnemonic is '%s'\n", token, mnemonic_p); */
                
            if( (len = strlen(mnemonic_p)) != 3 ) {
                for(j=len ; j<3; j++)
                    mnemonic_p[j] = ' ';
                mnemonic_p[3] = '\0';
            }
            if(strcmp(mnemonic_p,c_mnemon)==0) {
               XmListAddItem(db_list, xmstr_prodb_list[k], 0);
               num_prod_disp++;
               msg_num_filtered_list[num_prod_disp] = msg_num_total_list[k+1];
            } 
        }     
        
   } else if( (fpid==0) && (fpcode==0) && (strlen(c_mnemon) == 0) && (fvol!=0) ) { 
/* fprintf(stderr,"DEBUG - in fourth, filter volume branch\n");  */
        for(k=0; k<*prod_list_size; k++) {
            sscanf(product_list[k],"%12c%d%d%d%7c%d",
                   dtime, &vol, &ele, &pid, mnemonic, &pcode);
            if(vol==fvol) {
               XmListAddItem(db_list, xmstr_prodb_list[k], 0);
               num_prod_disp++;
               msg_num_filtered_list[num_prod_disp] = msg_num_total_list[k+1];
            } 
        }

   } else if( (strlen(c_mnemon) != 0) && (fvol!=0) ) { 
/* fprintf(stderr,"DEBUG - in fifth, filter mnemonic & volume branch\n");  */
        for(k=0; k<*prod_list_size; k++) {
            sscanf(product_list[k],"%12c%d%d%d%7c%d",
                   dtime, &vol, &ele, &pid, mnemonic, &pcode);
            token = strtok (mnemonic, " ");
            if(token == NULL) {
                mnemonic_p[0] = '\0';
                
            } else {
                strcpy(mnemonic_p, token);
            }
            if( (len = strlen(mnemonic_p)) != 3 ) {
                for(j=len ; j<3; j++)
                    mnemonic_p[j] = ' ';
                mnemonic_p[3] = '\0';
            }
            if( (strcmp(mnemonic_p,c_mnemon)==0) && (vol==fvol)  ) {
               XmListAddItem(db_list, xmstr_prodb_list[k], 0);
               num_prod_disp++;
               msg_num_filtered_list[num_prod_disp] = msg_num_total_list[k+1];
            } 
        }         
        
   } else { 
/* fprintf(stderr,"DEBUG - in last, filter prodid/code & volume branch\n");     */
        for(k=0; k<*prod_list_size; k++) {

            sscanf(product_list[k],"%12c%d%d%d%7c%d",
                   dtime, &vol, &ele, &pid, mnemonic, &pcode);
            if( ((fpid!=0) && (pid==fpid)) ||
                ((fpcode!=0) && (pcode==fpcode)) )
                if(vol==fvol) {
                   XmListAddItem(db_list, xmstr_prodb_list[k], 0);
                   num_prod_disp++; 
                   msg_num_filtered_list[num_prod_disp] = msg_num_total_list[k+1];
                }
        }  
    }      

/* DEBUG */
/* for(i=0;i<10;i++) */
/*     fprintf(stderr,"DEBUG - msg number item %d is %d\n", i,  msg_num_filtered_list[i]);             */
/* fprintf(stderr,"DEBUG - msg number item %d is %d\n",  */
/*            num_prod_disp-1,  msg_num_filtered_list[num_prod_disp-1]); */
/* fprintf(stderr,"DEBUG - msg number item %d is %d\n",  */
/*            num_prod_disp,  msg_num_filtered_list[num_prod_disp]);  */

   sprintf(buf, "%d", num_prod_disp);
   xmstr2 = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
   XtVaSetValues(list_size_label, XmNlabelString, xmstr2, NULL);
   XmStringFree(xmstr2);

XmListSetBottomPos(db_list, num_prod_disp);


/* DEBUG */
/* fprintf(stderr,"DEBUG finished filtering list\n\n"); */

} /* end filter_prod_list */






/****************************************************************************/
void build_msg_list(int err) {

   int j;
   XmString *xmstr_error_list=NULL;
   
   char buf[8];
   XmString xmstr1, xmstr2;

   
    char *err3_dialog[NUM_LINES] =
        {"Unable to open the selected product database file:                                      ",
         product_database_filename,
         "    This could reflect an incorrectly configured database in CVG prefs.\n    Double check preferences.\n",
         "    Confirm that the selected product database file exists and that you have\n    both read and write privileges.\n",
         "Take appropriate corrective action and press the 'Update List & Filter' button."};

    char *err2_dialog[NUM_LINES] =
        {"                                                                                       ",
         " Product database contains no products.                                                ",
         "    This may be normal if the ORPG is running with no input data or an ORPG\n    clean start has been performed.  Start data ingest.\n",
         "                                                                                       ",
         "Take appropriate corrective action and press the 'Update List & Filter' button."};


    char *err0_dialog[NUM_LINES] =
        {"Current Database Product List is Empty.  Press the 'Update List & Filter' button.\n",
         "    This may be normal if the ORPG is running with no input data or an ORPG\n    clean start has been performed.  Start data ingest.\n",
         "    This could reflect an incorrectly configured database in CVG prefs.\n    Double check preferences.\n",
         "    Confirm that the selected product database file exists and that you have\n    both read and write privileges.\n",
         "Take appropriate corrective action and press the 'Update List & Filter' button."};


    /* allocate memory for the composite string list */
    xmstr_error_list = (XmString*) XtMalloc(sizeof(XmString) * (NUM_LINES));


    if(err==0) {
    /* build error xmstr string list */
        for(j=0; j<NUM_LINES; j++) {
          xmstr_error_list[j] = XmStringCreateLtoR(err0_dialog[j], "tabfont");
        } /* end for */
    }
    
    if(err==-2) {
    /* build error xmstr string list */
        for(j=0; j<NUM_LINES; j++) {
          xmstr_error_list[j] = XmStringCreateLtoR(err2_dialog[j], "tabfont");
        } /* end for */
    }

    if(err==-3) {
    /* build error xmstr string list */
        for(j=0; j<NUM_LINES; j++) {
          xmstr_error_list[j] = XmStringCreateLtoR(err3_dialog[j], "tabfont");         
        } /* end for */
    }
    
    if(err==-1) {
    /* build error xmstr string list */
        for(j=0; j<NUM_LINES; j++) {
          xmstr_error_list[j] = XmStringCreateLtoR(init_dialog[j], "tabfont");         
        } /* end for */         
        
    }
    
        
        sprintf(buf, "0");
        xmstr1 = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);    
        XtVaSetValues(num_prod_label, XmNlabelString, xmstr1, NULL); 
        XmStringFree(xmstr1);
        
        sprintf(buf, "0");
        xmstr2 = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(list_size_label, XmNlabelString, xmstr2, NULL);
        XmStringFree(xmstr2);

/*-----------------------*/


    XmListDeleteAllItems(db_list);
    
    for(j=0; j<NUM_LINES; j++) { 
 
         XmListAddItem(db_list, xmstr_error_list[j], 0);
         
    }
    
    
    XmListSetBottomPos(db_list, NUM_LINES);


/*  FREE PREVIOUS LIST  */
    if (xmstr_error_list != NULL) {          

        for(j=0; j<NUM_LINES; j++) 
            XmStringFree(xmstr_error_list[j]);
        
        XtFree((char *)xmstr_error_list); 
        xmstr_error_list=NULL;        
    }


} /* end build_msg_list */











/* Reads the selected ICD product in from the product database linear buffer
 * and determines what types of packets and such can be displayed
 *
 * RESULT:  icd_product is loaded and layer_info is populated
 */
void listselection_Callback(Widget w, XtPointer client_data, XtPointer call_data) 
{


  int db_position; /* location in the product LB to read */
  
  int *selected_postions;
 
  int setup_result = CONFIG_ERROR;


  char *saved_icd_product = NULL, *saved_lb_filename = NULL;
  /*  GENERIC_PROD */
  void *saved_generic_prod_data = NULL;
  int rv;

    
  /* set up the main active ive screen */    
    if(selected_screen == SCREEN_1) {
        sd = sd1;       
    }

    if(selected_screen == SCREEN_2) {
        sd = sd2;
    }

    if(selected_screen == SCREEN_3) {
        sd = sd3;
    }



    /*  new method allows removal of message number component from list items */
    XtVaGetValues(db_list,
        XmNselectedPositions,         &selected_postions,
        NULL);


    /*  Nothing was selected so return */
    if(selected_postions==NULL) {
        fprintf(stderr,"Product not selected\n");
        return;
    }

/* DEBUG */
/* fprintf(stderr,"DEBUG SELECT CALLBACK - selected pos is %d\n", */
/*             selected_postions[0]); */


  db_position = msg_num_filtered_list[selected_postions[0]];

  /* zero is not a valid position, should not do anything */
  if(db_position == 0)
    return;



  saved_icd_product = sd->icd_product;
  sd->icd_product = NULL;      
  /*  GENERIC_PROD */
  saved_generic_prod_data = sd->generic_prod_data;
  sd->generic_prod_data = NULL;
  
   
  /* access the product located at db_position - load into memory and
     return to icd_product */
  
  sd->icd_product = (char*)Load_ORPG_Product(db_position, product_database_filename);


  if(sd->icd_product == NULL) {
      sd->icd_product = saved_icd_product;
  /*  GENERIC_PROD */
      sd->generic_prod_data = saved_generic_prod_data;
      return;
  }

  /* save the location of where the file was loaded from (i.e. the filename of the PDLB) */
  saved_lb_filename = sd->lb_filename;
  sd->lb_filename = NULL;  

  sd->lb_filename = (char *) malloc(strlen(product_database_filename)+1);
  if(sd->lb_filename == NULL) {
      fprintf(stderr,"ERROR allocating memory for filename\n");
      
      return;
  }
  strcpy(sd->lb_filename, product_database_filename);
  
  setup_result = product_post_load_setup();


  if(setup_result == CONFIG_ERROR) {
      /* leave prior product displayed */
      if(sd->icd_product != NULL)
          free(sd->icd_product);
      sd->icd_product = saved_icd_product;
      /*  GENERIC_PROD */
      if(sd->generic_prod_data != NULL)
          rv = cvg_RPGP_product_free((void *)sd->generic_prod_data);
      sd->generic_prod_data = saved_generic_prod_data;
      
      if(sd->lb_filename != NULL)
          free(sd->lb_filename);
      sd->lb_filename = saved_lb_filename;    
      return;
  }
  
  if( (setup_result == PARSE_ERROR) || 
      (setup_result == BLK_LEN_ERROR)  ) {
      /* // FUTURE IMPROVEMENT - restore all screen data modified by  */
      /* //     parse_packet_numbers() and replot the previous image */
      clear_screen_data(selected_screen, TRUE);
      if(saved_icd_product != NULL) {
          free(saved_icd_product);
          saved_icd_product = NULL;
        }
      /*  GENERIC_PROD */
      if(saved_generic_prod_data != NULL) {
          rv = cvg_RPGP_product_free((void *)saved_generic_prod_data);
          saved_generic_prod_data = NULL;
        }
      if(saved_lb_filename != NULL) {
          free(saved_lb_filename);
          saved_lb_filename = NULL;
        }
      return;    
  }
  
  if(saved_icd_product != NULL) {
      free(saved_icd_product);
      saved_icd_product = NULL;
    }
  /*  GENERIC_PROD */
  if(saved_generic_prod_data != NULL) {
      rv = cvg_RPGP_product_free((void *)saved_generic_prod_data);
      saved_generic_prod_data = NULL;
    }

  if(saved_lb_filename != NULL) {
      free(saved_lb_filename);  
      saved_lb_filename = NULL;
    }

  packet_selection_menu();

} /*  end listselection_Callback */










void browse_select_Callback(Widget w, XtPointer client_data, XtPointer call_data)
{
/*  Problem worked-around: while we can get item_position, item_length, and  */
/*  selected_item_count from the ListCallback_Struct, we cannot get the item */
/*  string.  This works in two callbacks in prefs.c but not here for some reason */
/*  Instead, lists of database message numbers are maintained.  This has a by  */
/*  product of allowing us to remove the internal message number component  */
/*  from all list items.  */
  XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;

 
   int lbfd=-1;
   int listSize = maxProducts+1;
   LB_info list[listSize]; 
   int num_products;

   typedef char param_type[13];  
   param_type param[6] = {"NOTSET", "NOTSET", "NOTSET", "NOTSET", "NOTSET", "NOTSET"};
   
   char prod_info[100], lb_data[96];
   XmString prod_info_xmstr;   
   Prod_header *hdr;   
   int i, len, msg_num=1;
   

/* //------method 1 ------------------------------------ */
/* // StringGet returns null and we crash with sscanf */
/*   XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &sbuf); */
/* // try new function, this retuns blank string */
/* // use ->item with listcallbackstruct and ->value with selectboxstruct */
/*   sbuf = (char *) XmStringUnparse(cbs->item, XmFONTLIST_DEFAULT_TAG, */
/*                                   XmCHARSET_TEXT, XmCHARSET_TEXT, */
/*                                   NULL, 0, */
/*                                   XmOUTPUT_ALL);   */
/* DEBUG */
/* fprintf(stderr,"DEBUG - selected item is %s, length is %d\n", */
/*    sbuf, strlen(sbuf));  */
/* fprintf(stderr,"DEBUG - selected item is '%s'\n", */
/*    sbuf);   */
  
/*   sscanf(sbuf, "%d", &msg_num);  */
/*   free(sbuf);   */
/* --------end method 1 ---------------------------------- */

/* //-------method 2 ---------------------------------- */
/* XtVaGetValues(db_list, */
/*     XmNselectedItems,       &sel_item_list, */
/*     XmNitems,               &the_list, */
/*     XmNitemCount,           &num_items, */
/*     XmNselectedItemCount,   &num_sel, */
/*     NULL); */
/* fprintf(stderr,"DEBUG SELECT M 2 - number selected is %d\n", num_sel);  */
/* fprintf(stderr,"DEBUG SELECT M 2 - size of displayed list is %d\n", num_items);    */
/* fprintf(stderr,"DEBUG SELECT - position of selected item is %d DEBUG \n", cbs->item_position);     */
/* if( ( XmStringGetLtoR(sel_item_list[0], XmFONTLIST_DEFAULT_TAG, &str) ) == TRUE) */
/* {         */
/*     fprintf(stderr,"DEBUG SELECT M 2 - list item is %s\n", str); */
/*     test_num = atoi(str); */
/*     fprintf(stderr,"DEBUG        TEST NUMBER IS %d\n", test_num); */
/*     free(str); */
/* } */
/* //---------end method 2 ----------------------------- */
/* //---------method 3 ----------------------------- */
/*  */
/*  XmStringGetLtoR(the_list[num_sel], XmFONTLIST_DEFAULT_TAG, &str); */
/*          */
/*     fprintf(stderr,"DEBUG SELECT M 2 - list item is %s\n", str); */
/*     test_num = atoi(str); */
/*     fprintf(stderr,"DEBUG        TEST NUMBER IS %d\n", test_num); */
/*     free(str); */



/* fprintf(stderr,"DEBUG - position of selected item is %d, length is %d, number selected is %d\n", */
/*    cbs->item_position, cbs->item_length, cbs->selected_item_count);  */
   
      msg_num = msg_num_filtered_list[cbs->item_position];
      
/* fprintf(stderr,"DEBUG - browseselect, message number is %d\n", msg_num); */

      /* if nothing was selected, we get zero back, and should not do anything */
      if(msg_num == 0) {
        
           sprintf(prod_info,"     ");
           prod_info_xmstr = XmStringGenerate((XtPointer)prod_info, 
                               "tabfont", XmCHARSET_TEXT, NULL);
           XtVaSetValues(prod_info_label,
               XmNlabelString,     prod_info_xmstr,
               NULL);    
           XmStringFree(prod_info_xmstr);
                     
           return;
      }



      /* open the product linear buffer */
      lbfd = LB_open(product_database_filename, LB_READ, NULL);
      if(lbfd < 0) {
          if(verbose_flag)
           printf("ERROR opening the following linear buffer: %s\n",
                  product_database_filename);
                  
           build_msg_list(-3);

           sprintf(prod_info,"     ");
           prod_info_xmstr = XmStringGenerate((XtPointer)prod_info, 
                               "tabfont", XmCHARSET_TEXT, NULL);
           XtVaSetValues(prod_info_label,
               XmNlabelString,     prod_info_xmstr,
               NULL);    
           XmStringFree(prod_info_xmstr);                               

          return;
      }

      /* fix reading wrapped portion of database?? */
      num_products = LB_list(lbfd, list, maxProducts+1);
      len = LB_read(lbfd, lb_data, 96, list[msg_num-1].id);

/* DEBUG */
/* fprintf(stderr,"DEBUG - PRODINFO - message length is %d\n",len); */
      
      if(msg_num==1) {
          strcpy(prod_info, "          *** INTERNAL DATABASE MESSAGE ***");

      } else if(len == LB_EXPIRED) { 
          strcpy(prod_info, "          *** EXPIRED PRODUCT MESSAGE ***");  
          
      } else if( (len <= 0) && (len != LB_BUF_TOO_SMALL) ) {
          strcpy(prod_info, "          *** INTERNAL DATABASE ERROR ***");   
          
      } else {  /*  readable product message       */
      
          hdr = (Prod_header *)lb_data;
          
          for(i=0;i<=5;i++) {
              if( hdr->g.req_params[i] == -32768 )
                  sprintf(param[i], "UNUSED    ");
              else if( hdr->g.req_params[i] == -32767 )
                  sprintf(param[i], "ANY_VALUE ");
              else if( hdr->g.req_params[i] == -32766 )
                  sprintf(param[i], "ALG_SET   ");
              else if( hdr->g.req_params[i] == -32765 )
                  sprintf(param[i], "ALL_VALUES");
              else if( hdr->g.req_params[i] == -32764 )
                  sprintf(param[i], "ALL_EXIST ");
              else
                  sprintf(param[i], "%- 10d", hdr->g.req_params[i]); 
          } 

/* TEST vol_time_list */
/* fprintf(stderr, "TEST VOL TIME LIST - vol date-time is %d  for message %d\n", */
/*                              sort_time_filtered_list[cbs->item_position], msg_num);  */
                                            
          sprintf(prod_info,"%4d  %2d  %4d    %s %s %s %s %s %s",
                        msg_num, hdr->g.vol_num, hdr->g.prod_id,  param[0],
                        param[1], param[2], param[3], param[4], param[5] );

/* TEST */
/* fprintf(stderr,"TESTING ELEV NUM LIST - elev_num id %d\n", */
/*                                   elev_num_total_list[cbs->item_position]); */

    } /*  end readable product message */
    

      prod_info_xmstr = XmStringGenerate((XtPointer)prod_info, 
                               "tabfont", XmCHARSET_TEXT, NULL);
                                     
      /*  SET VALUE HERE */
      XtVaSetValues(prod_info_label,
          XmNlabelString,     prod_info_xmstr,
          NULL);
    
       XmStringFree(prod_info_xmstr); 

       LB_close(lbfd);
           
} /*  end browse_select_Callback */










/****************************************************************************/
/* lets the user select a file containing displayable information to 
 * be read off disk for display
 */
void diskfile_select_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

    Widget auto_detect_radio, detect_auto_but, detect_manual_but;
    Widget form;
    static int detect_auto = AUTO_MODE;
    static int detect_manual = MANUAL_MODE;
    
    XmString xmdir;
    static char *helpfile = HELP_FILE_DISK_SELECT;


    prod_disk_sel = XmCreateFileSelectionDialog(w, "choose_diskfile_dialog", NULL, 0);
    
    XtVaSetValues(XtParent(prod_disk_sel), XmNtitle, "Choose Product Binary File...", 
    
         /* force increased width to improve usability of Motif file dialogs */
         XmNwidth,                560,
         XmNheight,               380,
         XmNallowShellResize,     FALSE,
    
    NULL);
    
    XtVaSetValues(prod_disk_sel, 
    
         XmNdialogStyle,       XmDIALOG_FULL_APPLICATION_MODAL, 
/* // why not set to XmDIALOG_FILE_SELECTION, the default? */
         XmNmarginHeight,      0,
         XmNmarginWidth,       0,
/*  WARning: the following causes CVG to crash when changing directories, why? */
/* //         XmNpathMode,              XmPATH_MODE_RELATIVE, */
         
         /* with increased width forced, this places labels in a reasonable location */
         XmNdirListLabelString,     
              XmStringCreateLtoR
                   ("-- Directories ------------                                             ", 
                                                         XmFONTLIST_DEFAULT_TAG),
         XmNfileListLabelString,    
              XmStringCreateLtoR
                   ("                                             ------------------ Files --", 
                                                         XmFONTLIST_DEFAULT_TAG),
         XmNlistVisibleItemCount,  16,
         
         NULL);

    /* if we've saved a directory that we've loaded something from,
     * use that as the current directory
     */
    if(prod_disk_last_filename[0] != '\0') {
        xmdir = XmStringCreateLtoR(prod_disk_last_filename, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(prod_disk_sel, XmNdirectory, xmdir, NULL);
        XmStringFree(xmdir);
    }


    /* ------------------------------------------------------ */
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, prod_disk_sel,
           NULL);

    /* new header detection mode radio box, default is auto */
    auto_detect_radio = XmCreateRadioBox(form, "radioselect", NULL, 0);
    XtVaSetValues(auto_detect_radio,
                XmNorientation,      XmVERTICAL,
                XmNpacking,          XmPACK_TIGHT,
                XmNtopAttachment,    XmATTACH_FORM,
                XmNtopOffset,        5,
                XmNleftAttachment,   XmATTACH_FORM,
                XmNleftOffset,       25,
                XmNbottomAttachment, XmATTACH_NONE,
                XmNrightAttachment,  XmATTACH_NONE,
                NULL);

    detect_auto_but = XtVaCreateManagedWidget("Automatically Detect Header Type (Default)", 
       xmToggleButtonWidgetClass, auto_detect_radio, 
       XmNset, True, NULL);
    XtAddCallback(detect_auto_but, XmNarmCallback, auto_detect_callback, 
                                                             (XtPointer) &detect_auto );
    
    detect_manual_but = XtVaCreateManagedWidget("Manually Select Header Type: --------->", 
       xmToggleButtonWidgetClass, auto_detect_radio, NULL);
    XtAddCallback(detect_manual_but, XmNarmCallback, auto_detect_callback, 
                                                             (XtPointer) &detect_manual );
    XtManageChild(auto_detect_radio); 

    /*----------------------------------------------------------*/
    diskfile_radio = XmCreateRadioBox(form, "radioselect", NULL, 0);
    XtVaSetValues(diskfile_radio,
                  XmNorientation,      XmHORIZONTAL,
                  XmNpacking,          XmPACK_COLUMN,
                  XmNnumColumns,       2,
                  XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
                  XmNtopWidget,        auto_detect_radio,
                  XmNtopOffset,        28,
                  XmNleftAttachment,   XmATTACH_WIDGET,
                  XmNleftWidget,       detect_manual_but,
                  XmNleftOffset,       0,
                  XmNrightAttachment,  XmATTACH_NONE,
                  XmNbottomAttachment, XmATTACH_NONE,                  
                  NULL);

    diskfile_icd_but = XtVaCreateManagedWidget("ICD Product (no hdr)", 
                  xmToggleButtonWidgetClass, diskfile_radio, 
                  XmNset, True, 
                  NULL);
    diskfile_icdwmo_but = XtVaCreateManagedWidget("ICD w/ WMO Header", 
                  xmToggleButtonWidgetClass, diskfile_radio, NULL);
    diskfile_preicdheaders_but = XtVaCreateManagedWidget("ICD w/ Pre-ICD Header", 
                  xmToggleButtonWidgetClass, diskfile_radio, NULL);
    diskfile_raw_but = XtVaCreateManagedWidget("CVG Raw Data (no hdr)", 
                  xmToggleButtonWidgetClass, diskfile_radio, NULL);
                  
                  
    XtManageChild(diskfile_radio);

    /*----------------------------------------------------*/
        header_type = AUTO_DETECT;
        XtVaSetValues(diskfile_icd_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        XtVaSetValues(diskfile_icdwmo_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        XtVaSetValues(diskfile_preicdheaders_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
    /* THIS FORMAT WILL BE REPLACED WITH SOMETHING ELSE, */
    XtVaSetValues(diskfile_raw_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
    /*----------------------------------------------------*/

    XtAddCallback(prod_disk_sel, XmNokCallback, diskfile_ok_callback, NULL);
    XtAddCallback(prod_disk_sel, XmNcancelCallback, diskfile_cancel_callback, NULL);
    XtAddCallback(prod_disk_sel, XmNhelpCallback, help_window_callback, helpfile);

    XtManageChild(prod_disk_sel);

    
} /*  end diskfile_select_callback() */





/*************************************************************************/
void auto_detect_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int *mode_selected = (int *)client_data; 
    
    if(*mode_selected == AUTO_MODE) {
        XtVaSetValues(diskfile_icd_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        XtVaSetValues(diskfile_icdwmo_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        XtVaSetValues(diskfile_preicdheaders_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        XtVaSetValues(diskfile_raw_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        header_type = AUTO_DETECT;
        
    } else if(*mode_selected == MANUAL_MODE) {
        XtVaSetValues(diskfile_icd_but,
               XmNset,           True, 
               XmNsensitive,     True,
               NULL);
        XtVaSetValues(diskfile_icdwmo_but,
               XmNset,           False, 
               XmNsensitive,     True,
               NULL);
        XtVaSetValues(diskfile_preicdheaders_but,
               XmNset,           False, 
               XmNsensitive,     True,
               NULL);
        /* the following currently disabled */
        XtVaSetValues(diskfile_raw_but,
               XmNset,           False, 
               XmNsensitive,     False,
               NULL);
        header_type = HEADER_NONE;
    }
    
    
    
    
}






/****************************************************************************/
/* if they say ok, then load the file and do the typical processing on it */
void diskfile_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    XmFileSelectionBoxCallbackStruct *cbs = (XmFileSelectionBoxCallbackStruct *)call_data;
    char *filename;
    FILE *data_file;

    int len;

    XmString xmstr;
    Widget d;

    int setup_result = CONFIG_ERROR;

    int rv = GOOD_LOAD;

    struct stat st;
    int size;


    Graphic_product *gp;
    short product_code;

/*  DEBUG */
/*  Prod_header *hdr; */
    
    
#ifdef LITTLE_ENDIAN_MACHINE
    unsigned char temp;
#endif


    
  /* set up the main active screen */    
    if(selected_screen == SCREEN_1) {
        sd = sd1;       
    }

    if(selected_screen == SCREEN_2) {
        sd = sd2;
    }

    if(selected_screen == SCREEN_3) {
        sd = sd3;
    }


    /* first, get the file that we want to read from */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);    
    if((data_file = fopen(filename,"r")) == NULL) {

        if(filename != NULL) {
            free(filename);
            filename = NULL;
        }
        do_fnf_error_box(w);
        
        return;        
    }

    /*  get filesize to be used for loading product */
    stat(filename,&st);
    size = (int) st.st_size;

    diskfile_saved_icd_product = sd->icd_product;
    sd->icd_product = NULL;      
  
    /*  GENERIC_PROD */
    diskfile_saved_generic_prod_data = sd->generic_prod_data;
    sd->generic_prod_data = NULL;
                 
      


        /* now, load the product into memory */
        if((sd->icd_product = load_icd_product_disk(data_file, size)) == NULL) {

            sd->icd_product = diskfile_saved_icd_product;
            /*  GENERIC_PROD */
            sd->generic_prod_data = diskfile_saved_generic_prod_data;
/*  NEEDED CHANGE, THE FOLLOWING DIALOG SHOULD ONLY BE OPENED IF THE TEST */
/*                 FOR ICD PRODUCT WAS NOT THE CAUSE OF THE PROBLEM */
/*             do_fnl_error_box(w); */

            if(filename != NULL) {
                free(filename);
                filename = NULL;
            }
            
            return;
        }
        
    

    /* save the location of where the file was loaded from */

    diskfile_saved_lb_filename = sd->lb_filename;
    sd->lb_filename = NULL;  

    fclose(data_file);

    /* save the directory where this file is in */
    len = strlen(filename);
    while( (filename[len] != '/') && (len > 0) ) 
        len--;
    strncpy(prod_disk_last_filename, filename, len+2);  /* include the '/' */
    prod_disk_last_filename[len+1] = '\0';


    if(filename != NULL) {
        free(filename);
        filename = NULL;
    }
            
    /* if the file has a pre-icd header, then we can go on.  otherwise, ask them
     * if they want to specify a product ID so that we can display
     * everything correctly
     */


    /*  this logic to be moved up to line 1427 as extra logic */
    if(header_type != HEADER_PRE_ICD) { /*  no Pre-ICD header */
           
        /*  read the product code from the product */
        gp = (Graphic_product *)(sd->icd_product + 96);
    
        product_code=gp->prod_code;
#ifdef LITTLE_ENDIAN_MACHINE
        product_code=SHORT_BSWAP(gp->prod_code);
#endif  
        
        /*  lookup the product ID if Legacy code (130 or less) */
        if(product_code<=130)
            diskfile_pid = code_to_id[(int)product_code];
        else
            diskfile_pid = (int)product_code;

/*  TEST */
/* if(product_code == 19) */
/* diskfile_pid = 0; */

/*  DEBUG */
/* fprintf(stderr,"DEBUG code to id in diskfile_ok, ID is %d from code %d\n",  */
/* diskfile_pid, product_code);    */


        /*  if product ID not a valid value then create the dialog prompt */
        if( (diskfile_pid <= 0) ||
            (diskfile_pid > 1999)  ) { /*  must ask for a product id */

            fprintf(stderr, "Detected Product ID (%d) not valid.\n"
                            "This can be caused by an error in the product or\n"
                            "CVG does not support the selected product.\n", 
                                                                   diskfile_pid);
                            
            d = XmCreatePromptDialog(w, "pid_spec", NULL, 0);
            XtVaSetValues(XtParent(d), XmNtitle, "Specify Product ID", NULL);
            xmstr = XmStringCreateLtoR("Detected Product ID not valid.\n"
                                       "This can be caused by an error in the\n"
                                       "product or CVG does not support\n"
                                       "the selected product.\n\n"
                                       "Enter the Product ID:\n"
                                                  , XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(d, XmNselectionLabelString, xmstr, 
                      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, NULL);
            XtAddCallback(d, XmNcancelCallback, product_specify_cancel_callback, NULL);
            XtAddCallback(d, XmNokCallback, product_specify_ok_callback, NULL);
            XmStringFree(xmstr);

            XtManageChild(d);
            
            /*   XtUnmanageChild(w); this is accomlished by  */
            /*                        product_specify_ok_callback() */

            return;
        } /*  end product ID not within range */
        
        /*  stuff the id into the pre-icd header */
        /* changed for LINUX */
        sd->icd_product[0] = (diskfile_pid >> 8) & 0xff;
        sd->icd_product[1] = diskfile_pid & 0xff;
#ifdef LITTLE_ENDIAN_MACHINE
        temp =sd->icd_product[1];
        sd->icd_product[1]=sd->icd_product[0];
        sd->icd_product[0]=temp;
#endif
        
/*  DEBUG */
/* hdr = (Prod_header *)(sd->icd_product); */
/* fprintf(stderr,"DEBUG diskfile_ok_callback - pre=icd prod id set to %d\n",  */
/*    hdr->g.prod_id); */
        
    } /*  end not pre-icd header    */
        
/* /////////////////////////////////////////////////////         */
    setup_result = product_post_load_setup();

/*  DEBUG */
/* fprintf(stderr,"DEBUG diskfile_ok - PRE_ICD setup_result is %d\n", setup_result); */



    if(setup_result != GOOD_LOAD) {
        rv = handle_load_error(setup_result);
        XtUnmanageChild(w);
        
        return;
    }
    
    
    if(diskfile_saved_icd_product != NULL) {
        free(diskfile_saved_icd_product);
        diskfile_saved_icd_product = NULL;
     }
/*  GENERIC_PROD */
     if(diskfile_saved_generic_prod_data != NULL) {
        rv = cvg_RPGP_product_free((void *)diskfile_saved_generic_prod_data);
        diskfile_saved_generic_prod_data = NULL;
     }

     if(diskfile_saved_lb_filename != NULL) {
        free(diskfile_saved_lb_filename); 
        diskfile_saved_lb_filename = NULL;
     }
       
                
   packet_selection_menu();
 
   XtUnmanageChild(w);
        
    
}  /*  end diskfile_ok_callback() */







/****************************************************************************/ 
void diskfile_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}







/****************************************************************************/
void product_specify_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

int rv;

    if(sd->icd_product != NULL)
        free(sd->icd_product);
    sd->icd_product = diskfile_saved_icd_product;
    /*  GENERIC_PROD */
    if(sd->generic_prod_data != NULL)
        rv = cvg_RPGP_product_free((void *)sd->generic_prod_data);
    sd->generic_prod_data = diskfile_saved_generic_prod_data;
    
    if(sd->lb_filename != NULL)
        free(sd->lb_filename);
    sd->lb_filename = diskfile_saved_lb_filename;    

    XtUnmanageChild(XtParent(w));
    XtUnmanageChild(XtParent(XtParent(w)));
}




/****************************************************************************/
void product_specify_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;
    char *pid_str;

    Widget d;
    XmString xmstr;

    char sub[25];
    int k, j;
        
    int setup_result = CONFIG_ERROR;
    
    int rv = GOOD_LOAD;

    
#ifdef LITTLE_ENDIAN_MACHINE
unsigned char temp;
#endif

    /* get the new pid in string form */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &pid_str);

    /* find a number and complain if it isn't one */
    /* skip over any initial spaces */
    k=0;
    while(pid_str[k] == ' ') k++;

    /* read in an integer (the prod id) */
    j=0;
    while(isdigit((int)(pid_str[k]))) 
        sub[j++] = pid_str[k++];
    sub[j] = '\0';
    free(pid_str);
    
    /* check if we got digits */
    if(j==0) {
        d = XmCreateErrorDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR("The entered value is not a number.\nPlease try again.",
                   XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
        
    } else 
        diskfile_pid = atoi(sub);



    if(verbose_flag)
        printf("new pid = %d\n", diskfile_pid);

    /* if it is, we just stuff it into its position on the top of the pre-ICD header */
/* changed for LINUX */
    sd->icd_product[0] = (diskfile_pid >> 8) & 0xff;
    sd->icd_product[1] = diskfile_pid & 0xff;
#ifdef LITTLE_ENDIAN_MACHINE
    temp =sd->icd_product[1];
    sd->icd_product[1]=sd->icd_product[0];
    sd->icd_product[0]=temp;
#endif
    
    /* now, do the normal set up */
    setup_result = product_post_load_setup();

/*  DEBUG */
/* fprintf(stderr,"DEBUG product_specify_ok - no PRE_ICD setup_result is %d\n", setup_result);     */

    if(setup_result != GOOD_LOAD) {
        rv = handle_load_error(setup_result);
        XtUnmanageChild(XtParent(w));
        XtUnmanageChild(XtParent(XtParent(w)));        
        return;
    }


    if(diskfile_saved_icd_product != NULL) {
        free(diskfile_saved_icd_product);
        diskfile_saved_icd_product = NULL;
    }
     /*  GENERIC_PROD */
    if(diskfile_saved_generic_prod_data != NULL) {
        rv = cvg_RPGP_product_free((void *)diskfile_saved_generic_prod_data);
        diskfile_saved_generic_prod_data = NULL;
    }

    if(diskfile_saved_lb_filename != NULL) {
        free(diskfile_saved_lb_filename);
        diskfile_saved_lb_filename = NULL;
    }


    
    packet_selection_menu();


    XtUnmanageChild(XtParent(w));
    XtUnmanageChild(XtParent(XtParent(w)));
    
} /*  end product_specify_ok_callback() */








int handle_load_error(int load_error)
{

int ret_val = 0;


    if(load_error == CONFIG_ERROR) {
        /* eave prior product displayed */
        if(sd->icd_product != NULL)
            free(sd->icd_product);
        sd->icd_product = diskfile_saved_icd_product;
        /* GENERIC_PROD */
        if((sd->generic_prod_data != NULL)) 
            ret_val = cvg_RPGP_product_free((void *)sd->generic_prod_data);
        sd->generic_prod_data = diskfile_saved_generic_prod_data;
        
        if(sd->lb_filename != NULL)
            free(sd->lb_filename);
        sd->lb_filename = diskfile_saved_lb_filename;    
        /*  do we need XtUnmanageChild(w) ? AND */
        /*  XtUnmanageChild(XtParent(XtParent(w))) */
        return (ret_val);
    }
    if( (load_error == PARSE_ERROR) || 
        (load_error == BLK_LEN_ERROR) ) {
        /* // FUTURE IMPROVEMENT - restore all screen data modified by  */
        /* //     parse_packet_numbers() and replot the previous image */
        clear_screen_data(selected_screen, TRUE);
        if(diskfile_saved_icd_product != NULL) {
            free(diskfile_saved_icd_product);
            diskfile_saved_icd_product=NULL;
        }
        /* GENERIC_PROD */
        if((diskfile_saved_generic_prod_data != NULL)) {
            ret_val = cvg_RPGP_product_free((void *)diskfile_saved_generic_prod_data);
            diskfile_saved_generic_prod_data = NULL;
        }
            
        if(diskfile_saved_lb_filename != NULL) {
            free(diskfile_saved_lb_filename);
            diskfile_saved_lb_filename=NULL;
        }
        return (ret_val);    
    }         
   
    return (ret_val);
    
} /*  end handle_load_error */







/****************************************************************************/
/* pops up a dialog to choose an intermediate linear buffer to
 * list the contents of for perusal
 */
void ilb_file_select_callback(Widget w,XtPointer client_data, XtPointer call_data)
{
    Widget d;
    XmString xmdir;
    char static *helpfile = HELP_FILE_FILE_SELECT;

    XmString xmstr;


    /*  function not yet ported to Linux */

    d = XmCreateInformationDialog(shell, "Intermediate Product", NULL, 0);
    xmstr = XmStringCreateLtoR(
    "The capability to select an intermediate product for display\n"
    "was a demonstration feature for CVG.  Until there are \n"
    "standards for the structure of intermediate products this \n"
    "feature is of no use to algorithm developers.\n\n"
    
    "A special sample product structure called 'CVG Raw Data' was\n"
    "was demonstrated in CODE sample algorithm 3 but is no longer\n"
    "supported by CVG.\n\n"
    
    "Loading products from intermediate product linear buffers\n"
    "will be available when useful.",                        
                         XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(XtParent(d), XmNtitle, 
                 "Intermediate Product Demonstration Feature", 
                 NULL);
    XtVaSetValues(d, XmNmessageString, xmstr, NULL);
    XmStringFree(xmstr);
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
    XtManageChild(d);

#ifdef LITTLE_ENDIAN_MACHINE
    return;
#endif


    d = XmCreateFileSelectionDialog(w, "choose_diskfile_dialog", NULL, 0);
    
    XtVaSetValues(XtParent(d), XmNtitle, "Choose Intermediate LB File...", 
    
         XmNwidth,                560,
         XmNheight,               380,
         XmNallowShellResize,     FALSE,
    
    NULL);
    
    XtVaSetValues(d, 
          XmNdialogStyle,       XmDIALOG_FULL_APPLICATION_MODAL, 
          XmNmarginHeight,      0,
          XmNmarginWidth,       0,
          
         XmNdirListLabelString,     
              XmStringCreateLtoR
                   ("-- Directories ------------                                             ", 
                                                         XmFONTLIST_DEFAULT_TAG),
         XmNfileListLabelString,    
              XmStringCreateLtoR
                   ("                                             ------------------ Files --", 
                                                         XmFONTLIST_DEFAULT_TAG),
         XmNlistVisibleItemCount,  16,
          
          NULL);

    /* if we've saved a directory that we've loaded something from,
     * use that as the current directory
     */
    if(prod_disk_last_filename[0] != '\0') {
        xmdir = XmStringCreateLtoR(prod_disk_last_filename, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNdirectory, xmdir, NULL);
        XmStringFree(xmdir);
    }
    else {
      /* set the directory name to $ORPGDIR */
        char *orpg_dir;
        orpg_dir = getenv("ORPGDIR");
        prod_disk_last_filename[0]=0;
        sprintf(prod_disk_last_filename,"%s/",orpg_dir);
        fprintf(stderr,"ORPGDIR: %s\n",prod_disk_last_filename);
        xmdir = XmStringCreateLtoR(prod_disk_last_filename, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNdirectory, xmdir, NULL);
        XmStringFree(xmdir);
      
    }

    XtAddCallback(d, XmNokCallback, ilb_file_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, ilb_file_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, help_window_callback, helpfile);

    XtManageChild(d);
    
}  /*  end ilb_file_select_callback() */








/****************************************************************************/
void ilb_file_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}






Widget ilb_radiobox, ilb_raw_but, ilb_icd_but;

/* once we choose the file, we list the contents of the linear buffer so that
 * the user can pick a product out of them
 */
void ilb_file_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
   XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;
   Widget dialog, label, d;
   char *charval, *p_desc, *lb_filename=NULL;
   int lbfd=-1;
   int listSize = maxProducts+1;

   LB_info list[listSize]; 
   int len,i;
   char *lb_data;
 
   Prod_header *hdr;
   Arg args[20];
   int n=0;
   char prod_desc[80];
   XmString *xmstr_fileselect, xmstr;
   static char *helpfile = HELP_FILE_ILB_SELECT;
   int error_found=FALSE;

   XtUnmanageChild(w);
   
   /* get the selected file name */
   XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &lb_filename);

   if(lb_filename==NULL) {
      fprintf(stderr,"ERROR getting filname\n");
      return;
   }
   /* save the directory where this file is in */
   len = strlen(lb_filename);
   while( (lb_filename[len] != '/') && (len > 0) ) len--;
   strncpy(prod_disk_last_filename, lb_filename, len+2);  /* include the '/' */
   prod_disk_last_filename[len+1] = '\0';
   
   /* open the product linear buffer */
   lbfd = LB_open(lb_filename, LB_READ, NULL);
   if(lbfd < 0) {
      if(verbose_flag)
          printf("ERROR opening the following linear buffer: %s\n",
                 lb_filename);
      return;
   }

   /* retrieve information from the linear buffer */
   num_products = LB_list(lbfd, list, maxProducts+1);
   /*fprintf(stderr,"number of products in lb: %d\n",num_products);*/
   if(verbose_flag) 
       printf("num_products available=%i\n", num_products);
   if(num_products <= 0) {
     if(verbose_flag)
         printf("ERROR: the selected linear buffer contains no products\n");
 

     d = XmCreateInformationDialog(w, "Note", NULL, 0);
     xmstr = XmStringCreateLtoR("The selected linear buffer contains no products.",
                                XmFONTLIST_DEFAULT_TAG);
     XtVaSetValues(XtParent(d), XmNtitle, "NOTE", NULL);
     XtVaSetValues(d, XmNmessageString, xmstr, NULL);
     XmStringFree(xmstr);
     XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
     XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
     XtManageChild(d);
     return;
   }

   /* allocate memory for the composite string list */
   xmstr_fileselect = (XmString*) XtMalloc(sizeof(XmString) * (num_products+1));

   if(verbose_flag)
       fprintf(stderr, "creating list of availible products");

   /* note: the listing index is included in the first line. hence the
      smstr index is incremented by one */
   for(i=0; i<num_products; i++) {
      char entry_string[150];
      len = LB_read(lbfd, &lb_data, LB_ALLOC_BUF, list[i].id);

      if(lb_data==NULL) {
          fprintf(stderr,"ERROR reading intermediate LB message\n");
          return;
      }
      
      hdr = (Prod_header *)lb_data;
      

      /* this checks for a negative product length which indicates
      an abort message has been issued */
      if(hdr->g.len<=0) {  
                    strcpy(prod_desc, "EMPTY PRODUCT MESSAGE - PRODUCT ABORTED");         
      } else {  
         /* if the product has a description, use it, otherwise use a default */
         p_desc = assoc_access_s(product_names, hdr->g.prod_id);
         if(p_desc == NULL)
            strcpy(prod_desc, "Description Not Configured");
         else
            strcpy(prod_desc, p_desc);
      }
      
      /* create a string to be placed into the list */
      if(hdr->g.len<=0) 
        sprintf(entry_string,
              "%03d          N/A           N/A           N/A        %s\n",
                i+1, prod_desc);
      else      
               sprintf(entry_string,
              "%03d          %03hd            %02d              %02d         %s\n",
                i+1, hdr->g.prod_id, hdr->g.vol_num, get_elev_ind(lb_data,orpg_build_i), prod_desc);
      
            xmstr_fileselect[i]=XmStringCreateLtoR(entry_string,XmFONTLIST_DEFAULT_TAG);
      
            free(lb_data);
            lb_data=NULL;
            
   } /* end for */

   LB_close(lbfd);
   fprintf(stderr, "linear buffer closed\n");
   
   /* the next if statement is used to handle a break in the
   processing loop caused by abort messages populating the intermediate lb */
   if(error_found==TRUE)
      num_products=i;

   /* create dialog window */
   n=0;
   XtSetArg(args[n], XmNlistItems, xmstr_fileselect); n++;
   XtSetArg(args[n], XmNlistItemCount, num_products); n++;
   XtSetArg(args[n], XmNvisibleItemCount, 15); n++;
   XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
   XtSetArg(args[n], XmNlistLabelString,
   XmStringCreateLtoR("Num    LBuffer ID   Volume    Elevation     Product Type", 
                               XmFONTLIST_DEFAULT_TAG)); n++;
   XtSetArg(args[n], XmNmarginHeight,      0); n++;
   XtSetArg(args[n], XmNmarginWidth,       0); n++; 
   dialog = XmCreateSelectionDialog(shell, "product_selectd", args, n);

   XtVaSetValues(XtParent(dialog), XmNtitle, "Intermediate LB - Product Select", NULL);   

   if(verbose_flag)
       printf("number of products in the list = %d\n", num_products);

   /* get rid of buttons 'n' stuff we don't want */
   XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_SELECTION_LABEL));
   XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT));
   XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON));
 
   /* default select first item in list */
   XmListSelectPos(XmSelectionBoxGetChild(dialog, XmDIALOG_LIST), 1, True);
 
   /* add stuff we do - radio buttons to say what type of input we have */
   ilb_radiobox = XmCreateRadioBox(dialog, "radioselect", NULL, 0);
   XtVaSetValues(ilb_radiobox,
                 XmNorientation,      XmHORIZONTAL,
                 XmNpacking,          XmPACK_TIGHT,
                 NULL);
   label = XtVaCreateManagedWidget("Input Type:", xmLabelWidgetClass, ilb_radiobox, NULL);
   ilb_icd_but = XtVaCreateManagedWidget("ICD Product", 
       xmToggleButtonWidgetClass, ilb_radiobox, 
       XmNset, True, NULL);
   ilb_raw_but = XtVaCreateManagedWidget("CVG Raw Data                                  ", 
       xmToggleButtonWidgetClass, ilb_radiobox, NULL);
   XtManageChild(ilb_radiobox);

   /* save the lb filename so that we can load stuff from it */
   charval = malloc(strlen(lb_filename));
   strcpy(charval, lb_filename);
   XtAddCallback(dialog, XmNokCallback, ilb_product_selection_callback, charval);
   XtAddCallback(dialog, XmNhelpCallback, help_window_callback, helpfile);
   XtManageChild(dialog);
   
       

   /* free allocated memory for the xmstring */
   /* ML FIX - it looks like we should free one more but don't try */
   for(i=0; i<num_products; i++) 
       XmStringFree(xmstr_fileselect[i]);

   XtFree((char *)xmstr_fileselect);
   
   
   free(lb_filename);

} /*  end ilb_file_ok_callback() */





/****************************************************************************/
void ilb_product_selection_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    
  XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;
  char *lb_filename, *cbuf;
  int db_position; /* location in the product LB to read */

  Boolean is_set;

  int setup_result = CONFIG_ERROR;


  char *saved_icd_product = NULL, *saved_lb_filename = NULL;    
  /*  GENERIC_PROD */
  void *saved_generic_prod_data = NULL;
  int rv;
    
    /* set up the main active screen */    
    if(selected_screen == SCREEN_1) {
        sd = sd1;       
    }

    if(selected_screen == SCREEN_2) {
        sd = sd2;
    }

    if(selected_screen == SCREEN_3) {
        sd = sd3;
    }
    

    /* get back the filename of the lb we're reading from */
    lb_filename = (char *)client_data;
    
    /* figure out where in the lb the message we're looking for is */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &cbuf);
    sscanf(cbuf, "%d", &db_position); 
    printf("item selected is %i\n",db_position);
    free(cbuf);  

    /* if nothing was selected, we get zero back, and should not do anything */
    if(db_position == 0) {
        if(lb_filename != NULL) {
            free(lb_filename);
            lb_filename = NULL;
        }
        return;
    }


    saved_icd_product = sd->icd_product;
    sd->icd_product = NULL;      
    /* GENERIC_PROD */
    saved_generic_prod_data = sd->generic_prod_data;
    sd->generic_prod_data = NULL;
          

    if(verbose_flag)
      printf("Linear buffer filename: %s\n", lb_filename);
    
    /* since we're dealing with radio buttons, only one of them should be set */
    XtVaGetValues(ilb_raw_but, XmNset, &is_set, NULL);
    
    /* if the input is in the CVG raw format, then ingest that and
     * stick it in an icd-ish product
     */
    if(is_set == True) { /*  NOTE: this option not currently supported */
      sd->icd_product = load_cvg_raw_lb(lb_filename, db_position);
    } else {
      /* else, we can just grab the ICD product */
      sd->icd_product = (char*)Load_ORPG_Product(db_position, lb_filename);
    }    

    if(verbose_flag)
        fprintf(stderr, "Loaded product...\n");

    if(sd->icd_product == NULL) {
        sd->icd_product = saved_icd_product;
        /* GENERIC_PROD */
        sd->generic_prod_data = saved_generic_prod_data;
        if(lb_filename != NULL) {
            free(lb_filename);
            lb_filename = NULL;
        }
        return;
    }


    if(verbose_flag)
        fprintf(stderr, "Set up product pointers...\n");

    /* save the location of where the file was loaded from */
    /* (i.e. the filename of the LB) */

    saved_lb_filename = sd->lb_filename;
    sd->lb_filename = NULL;  


    if(verbose_flag)
        fprintf(stderr, "Setting up new saved file name...\n");

    if((sd->lb_filename = (char *) malloc(strlen(lb_filename)+1)) != NULL)
        strcpy(sd->lb_filename, lb_filename);

    if(verbose_flag)
        fprintf(stderr, "Doing product post processing...\n");
 
    setup_result = product_post_load_setup();
    
    if(setup_result == CONFIG_ERROR) {
        /* leave prior product displayed */
        if(sd->icd_product != NULL)
            free(sd->icd_product);
        sd->icd_product = saved_icd_product;
        /* GENERIC_PROD */
        if(sd->generic_prod_data != NULL)
            rv = cvg_RPGP_product_free((void *)sd->generic_prod_data);
        sd->generic_prod_data = saved_generic_prod_data;
        
        if(sd->lb_filename != NULL)
            free(sd->lb_filename);
        sd->lb_filename = saved_lb_filename;    
        if(lb_filename != NULL) {
            free(lb_filename);
            lb_filename = NULL;
        }
        return;
    }
    if( (setup_result == PARSE_ERROR) || 
        (setup_result == BLK_LEN_ERROR) ) {
        /* // FUTURE IMPROVEMENT - restore all screen data modified by  */
        /* //     parse_packet_numbers() and replot the previous image */
        clear_screen_data(selected_screen, TRUE);
        if(saved_icd_product != NULL) {
            free(saved_icd_product);
            saved_icd_product = NULL;
        }
        /* GENERIC_PROD */
        if(saved_generic_prod_data != NULL) {
            rv = cvg_RPGP_product_free((void *)saved_generic_prod_data);
            saved_generic_prod_data = NULL;
        }
        if(saved_lb_filename != NULL) {
          free(saved_lb_filename);
          saved_lb_filename = NULL;
        }
        if(lb_filename != NULL) {
            free(lb_filename);
            lb_filename = NULL;
        }
        return;    
    }     

    if(saved_icd_product != NULL) {
        free(saved_icd_product);
        saved_icd_product = NULL;
    }
    /* GENERIC_PROD */
    if(saved_generic_prod_data != NULL) {
        rv = cvg_RPGP_product_free((void *)saved_generic_prod_data);
        saved_generic_prod_data = NULL;
    }
    if(saved_lb_filename != NULL) {
        free(saved_lb_filename);
        saved_lb_filename = NULL;
    }
       
    packet_selection_menu();

    /* finally, free the filename string that we malloced before */
    if(lb_filename != NULL) {
        free(lb_filename);
        lb_filename = NULL;
    }
        
} /*  end ilb_product_selection_callback() */




/****************************************************************************/
/* does an error dialog in case of a file that cannot be opened */
void do_fnf_error_box(Widget w)
{
    Widget d;
    XmString xmstr;

    d = XmCreateErrorDialog(w, "File Error", NULL, 0);
    xmstr = XmStringCreateLtoR("The specified file could not be opened.",
                               XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(XtParent(d), XmNtitle, "File Error", NULL);
    XtVaSetValues(d, XmNmessageString, xmstr, NULL);
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
    XtManageChild(d);
    XmStringFree(xmstr);
    
}




/****************************************************************************/
/* does an error dialog in case of a file that cannot be loaded */
void do_fnl_error_box(Widget w)
{
    Widget d;
    XmString xmstr;

    d = XmCreateErrorDialog(w, "File Error", NULL, 0);
    xmstr = XmStringCreateLtoR("The specified product file could not be loaded.",
                               XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(XtParent(d), XmNtitle, "Product Load Error", NULL);
    XtVaSetValues(d, XmNmessageString, xmstr, NULL);
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
    XtManageChild(d);
    XmStringFree(xmstr);
}



/****************************************************************************/
/* takes the loaded icd_product and 
 *     - checks for some configuration errors
 *     - checks for block length errors
 *     - accomplishes byte-swap of product
 *     - populates a whole bunch of structures and fields with it's data
 * RETURNS -1 ON CONFIGURATION ERROR, -2 ON BLK LENGTH ERROR, 
 * -3 ON PARSE PACKETS ERROR, and 1 ON SUCCESS
 */
int product_post_load_setup()
{

    int result = FALSE;
    
    Prod_header *hdr;
    char pdesc[150], *desc_ptr;
    int *msg_type_ptr, *dl_flag_ptr, *prod_res_ptr;
    int msg_type, dl_flag, prod_res;
    char*leg_units_ptr, *legend_filename_ptr, *legend2_filename_ptr;
    Widget d;
    XmString xmstr;
    int icd_format, pre_icd;
    Graphic_product *gp;
    short threshold_1, threshold_2, divider;

    int rv;
    
    
/* DEBUG */
/* fprintf(stderr,"DEBUG - entering product_post_load_setup \n"); */
    
    
    if(selected_screen == SCREEN_1) {
        sd = sd1;       
    }

    if(selected_screen == SCREEN_2) {
        sd = sd2;
    }

    if(selected_screen == SCREEN_3) {
        sd = sd3;
    }


/* BEGIN CONFIGURATION CHECK SECTION */
    /* If product not configured, we stop here. */
    /* This can prevent an endless loop that may occur in the function */
    /* skip_over_packet() in symbology_block.c                         */
    hdr = (Prod_header *)(sd->icd_product);
    gp = (Graphic_product *)(sd->icd_product + 96);
    msg_type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);


/*  TEST ERROR HANDLING BY CALLER */
/* if(hdr->g.prod_id == 2) */
/* msg_type_ptr = NULL; */

   
   threshold_1=gp->level_1;
   threshold_2=gp->level_2;
   divider=gp->divider;
#ifdef LITTLE_ENDIAN_MACHINE
    threshold_1=SHORT_BSWAP(gp->level_1);
    threshold_2=SHORT_BSWAP(gp->level_2);
    divider=SHORT_BSWAP(gp->divider);
#endif   

   
    if (msg_type_ptr == NULL) {
       fprintf(stderr,"\nCONFIGURATION ERROR \n");
       fprintf(stderr,"This product has not been configured.\n");
       fprintf(stderr,"Enter preferences for product id %d\n", hdr->g.prod_id);
       fprintf(stderr,"using the CVG Product Specific Info Menu.\n\n");
       
       d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
       xmstr = XmStringCreateLtoR("The selected product has not been configured.\nEnter preferences for this product using the CVG Product Specific Info Menu.",
                               XmFONTLIST_DEFAULT_TAG);
       XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
       XtVaSetValues(d, XmNmessageString, xmstr, NULL);
       XmStringFree(xmstr);
       XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
       XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
       XtManageChild(d);

       
       return CONFIG_ERROR;
    }

    if (*msg_type_ptr == RADAR_CODED_MESSAGE) {
       fprintf(stderr,"\nNON-SUPPORTED PRODUCT \n");
       fprintf(stderr,"The Radar Coded Message Type is not \n");
       fprintf(stderr,"currently displayed by CVG\n\n");
       
       d = XmCreateErrorDialog(shell, "Non-Supported Product Type", NULL, 0);
       xmstr = XmStringCreateLtoR("The Radar Coded Message Type is not \ncurrently displayed by CVG.",
                               XmFONTLIST_DEFAULT_TAG);
       XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
       XtVaSetValues(d, XmNmessageString, xmstr, NULL);
       XmStringFree(xmstr);
       XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
       XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
       XtManageChild(d);

       
       return CONFIG_ERROR;
    }

    if (*msg_type_ptr == TEXT_MESSAGE) {
       fprintf(stderr,"\nNON-SUPPORTED PRODUCT \n");
       fprintf(stderr,"The Text Message Type is not \n");
       fprintf(stderr,"currently displayed by CVG\n\n");
       
       d = XmCreateErrorDialog(shell, "Non-Supported Product Type", NULL, 0);
       xmstr = XmStringCreateLtoR("The Text Message Type is not \ncurrently displayed by CVG.",
                               XmFONTLIST_DEFAULT_TAG);
       XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
       XtVaSetValues(d, XmNmessageString, xmstr, NULL);
       XmStringFree(xmstr);
       XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
       XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
       XtManageChild(d);

       
       return CONFIG_ERROR;
    }
    
 
   /* Here we check for a few common product configuration errors. */
   /* Currently this checks for the presence of invalid initial values. */
   /* The initial value for default packet is a common default and is not checked */
   dl_flag_ptr = assoc_access_i(digital_legend_flag, hdr->g.prod_id);
   leg_units_ptr = assoc_access_s(legend_units, hdr->g.prod_id);
   prod_res_ptr = assoc_access_i(product_res, hdr->g.prod_id);
   msg_type = *msg_type_ptr;
   dl_flag = *dl_flag_ptr;
   prod_res = *prod_res_ptr;



    
   if (msg_type == -1 || dl_flag == -1 || prod_res == -1) {
      fprintf(stderr,"\nCONFIGURATION ERROR \n");
      fprintf(stderr,"Configuration of this product is incomplete or in error.\n");
      fprintf(stderr,"Edit preferences for product id %d\n", hdr->g.prod_id);
      fprintf(stderr,"using the CVG Product Specific Info Menu.\n\n");
      fprintf(stderr,"Ensure that Message Type, Resolution, Digital Legend Flag\n"); 
      fprintf(stderr,"and Units have been edited correctly.\n\n");
      if (msg_type == -1)
          fprintf(stderr,"Message Type has not been configured.\n");
      if (dl_flag == -1)
          fprintf(stderr,"Digital Legend Flag has not been configured.\n");
      if (prod_res == -1)
          fprintf(stderr,"Product Resolution has not been configured.\n");
      if (leg_units_ptr == NULL)
          fprintf(stderr,"The Legend Units string NULL, configuration error.\n");
      else if (strcmp(leg_units_ptr, " units") == 0)
          fprintf(stderr,"The Legend Units string has not been configured.\n");
          
      d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
      xmstr = XmStringCreateLtoR("Product configuration is incomplete.\nEdit preferences for this product using the CVG Product Specific Info Menu.",
                               XmFONTLIST_DEFAULT_TAG);
      XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
      XtVaSetValues(d, XmNmessageString, xmstr, NULL);
      XmStringFree(xmstr);
      XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
      XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
      XtManageChild(d);
      
      return CONFIG_ERROR;
   }

   /*CVG 9.0 - DON'T TEST FOR NULL PTR UNLESS DIGITAL */
   if(dl_flag != 0) {
   
      legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);
      
      if( (legend_filename_ptr == NULL) ) {
          fprintf(stderr,"\nCONFIGURATION ERROR, legend filename string is NULL\n");
          
          d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
          xmstr = XmStringCreateLtoR("Legend file string is NULL for this digital product.\nEdit preferences for this product using the CVG Product Specific Info Menu.",
              XmFONTLIST_DEFAULT_TAG);
          XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
          XtVaSetValues(d, XmNmessageString, xmstr, NULL);
          XmStringFree(xmstr);
          XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
          XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
          XtManageChild(d);
          
          return CONFIG_ERROR;
          
      } else if( (strcmp(legend_filename_ptr, ".")==0) ||
                 (strcmp(legend_filename_ptr, ".lgd")==0) ) {
          fprintf(stderr,"\nCONFIGURATION ERROR\n");
          fprintf(stderr,
              "This product is configured as a Digital Product\n");
          fprintf(stderr,
              "A legend file name must be entered in the Product Specific Preferences\n");
          
          d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
          xmstr = XmStringCreateLtoR("No legend file is specified for this digital product.\nEdit preferences for this product using the CVG Product Specific Info Menu.",
              XmFONTLIST_DEFAULT_TAG);
          XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
          XtVaSetValues(d, XmNmessageString, xmstr, NULL);
          XmStringFree(xmstr);
          XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
          XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
          XtManageChild(d);
      
          return CONFIG_ERROR;
          
      } /* end else */

   } /*  end if dl_flag != 0 */


    /*  TEST DIGITAL FLAG 3 HERE */
    if(dl_flag == 3) {       

       legend2_filename_ptr = assoc_access_s(dig_legend_file_2, hdr->g.prod_id);
       
       if( (legend2_filename_ptr == NULL) ) {
           fprintf(stderr,"\nCONFIGURATION ERROR, legend filename string is NULL\n");
           
           d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
           xmstr = XmStringCreateLtoR("Legend file string is NULL for this digital product.\nEdit preferences for this product using the CVG Product Specific Info Menu.",
               XmFONTLIST_DEFAULT_TAG);
           XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
           XtVaSetValues(d, XmNmessageString, xmstr, NULL);
           XmStringFree(xmstr);
           XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
           XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
           XtManageChild(d);
       
           return CONFIG_ERROR;
           
       } else if( ( (strcmp(legend2_filename_ptr, ".")==0) ||
                (strcmp(legend2_filename_ptr, ".lgd")==0) ) &&
                dl_flag != 0                    ) {
           fprintf(stderr,"\nCONFIGURATION ERROR\n");
           fprintf(stderr,
               "This product is configured as a Digital Velocity Product\n");
           fprintf(stderr,
               "A second legend file name must be entered in the Product Specific Preferences\n");
           
           d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
           xmstr = XmStringCreateLtoR("No second legend file is specified for this digital velocity product.\nEdit preferences for this product using the CVG Product Specific Info Menu.",
               XmFONTLIST_DEFAULT_TAG);
           XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
           XtVaSetValues(d, XmNmessageString, xmstr, NULL);                         
           XmStringFree(xmstr);                                 
           XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));          
           XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));        
           XtManageChild(d);                                 
                                             
           return CONFIG_ERROR;                                                                         
       }                                 
                                         
       if( (threshold_1 != -635) && (threshold_1 != -1270 ) ) {        

           fprintf(stderr,"\nCONFIGURATION OR PRODUCT ERROR\n");
           fprintf(stderr,
               "This product is configured as a Digital Velocity Product\n");
           fprintf(stderr,
               "The Product Descritpion Block level 1 field contains an unexpected value\n");
           fprintf(stderr,
               "The Product may not be correctly displayed correctly.\n");
           fprintf(stderr,
               "The first legend definition and color palette will always be used.\n");                          

           d = XmCreateErrorDialog(shell, "Configuration or Product Error", NULL, 0);
           xmstr = XmStringCreateLtoR("This digital velocity product has an unexpected value in the PDB level 1 field.\nThe display may not be correct (first legend and palette always used).",
               XmFONTLIST_DEFAULT_TAG);
           XtVaSetValues(XtParent(d), XmNtitle, "Configuration or Product Error", NULL);
           XtVaSetValues(d, XmNmessageString, xmstr, NULL);                         
           XmStringFree(xmstr);                                 
           XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));          
           XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));        
           XtManageChild(d);  
                                                    
       }
                                                             
    } /*  end if dl_flag==3 */



    /* if a non-digital product has a digital legend specified, we display anyway */
  
    if(dl_flag == 0) {
  
       legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);
      
       if( (legend_filename_ptr != NULL) ) {
           
          if( (strcmp(legend_filename_ptr, ".")!=0) &&
              (strcmp(legend_filename_ptr, ".lgd")!=0) ) {
             fprintf(stderr,"\nPRODUCT CONFIGURATION NOTE:\n");
             fprintf(stderr,"Possible configuration error, this non-digital product\n");
             fprintf(stderr,"has a digital legend file configured.\n");
             
             d = XmCreateInformationDialog(shell, "Error", NULL, 0);
             xmstr = XmStringCreateLtoR("Possible configuration error.\nThis non-digital product has a digital legend file configured.",
                                  XmFONTLIST_DEFAULT_TAG);
             XtVaSetValues(XtParent(d), XmNtitle, "Check Configuration", NULL);
             XtVaSetValues(d, XmNmessageString, xmstr, NULL);
             XmStringFree(xmstr);
             XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
             XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
             XtManageChild(d);          
          }
          
       } /* end if not NULL */

    } /* end if dl_flag == 0  */

    /* WE TEST FOR PALETTE FILE BEING CONFIGURED DURING PRODUCT DISPLAY       */
    /*       WHICH REQUIRES EITHER A DEFAULT PALETTE OR A CONFIGURED PALETTE  */

    /* If the legend unit string is not configured, we display the product anyway */
    if (leg_units_ptr != NULL) {
        if (strcmp(leg_units_ptr, " units") == 0) {
           fprintf(stderr,"\nPRODUCT CONFIGURATION NOTE:\n");
           fprintf(stderr,"The Legend Units string has not been configured.\n");
        
        }
    }

/* END CONFIGURATION CHECK SECTION */


    /* loaded products will NOT be parsed with the  */
    /* screens linked.  Must select either screen 1 or screen 2 and load  */
    /* a single product.  Value of sd now set at beginning of function    */
    
    /* NOTE - The following function checks for a block length error and */
    /*        accomplishes a product byte-swap while parsing the packets */
   
    result = parse_packet_numbers(sd->icd_product);

    /*  Currently we treat PARSE_ERROR and BLK_LEN_ERROR the same     */
    if(result == PARSE_ERROR) {
        
        if(sd->icd_product != NULL) {
            free(sd->icd_product);
            sd->icd_product = NULL;
        }
        /*  GENERIC_PROD */
        if((sd->generic_prod_data != NULL)) {
              rv = cvg_RPGP_product_free((void *)sd->generic_prod_data);
              sd->generic_prod_data = NULL;
        }
        return PARSE_ERROR;
    }

    if(result == BLK_LEN_ERROR) {
        
        if(sd->icd_product != NULL) {
            free(sd->icd_product);
            sd->icd_product = NULL;
        }
        /*  GENERIC_PROD */
        if((sd->generic_prod_data != NULL)) {
              rv = cvg_RPGP_product_free((void *)sd->generic_prod_data);
              sd->generic_prod_data = NULL;
        }
        return BLK_LEN_ERROR;
    }

    /* we haven't plotted this product yet */
    /* NOTE: we may no longer use this variable */
 
    if(selected_screen == SCREEN_1)
        sd1->last_plotted = FALSE;
    if(selected_screen == SCREEN_2)
        sd2->last_plotted = FALSE;
    if(selected_screen == SCREEN_3)
        sd3->last_plotted = FALSE;

    if(verbose_flag)
        fprintf(stderr, "getting product description\n");

    /* get the info from the current product */
    hdr = (Prod_header *)(sd->icd_product);
    gp = (Graphic_product *)(sd->icd_product + 96);
    
    desc_ptr = assoc_access_s(product_names, hdr->g.prod_id);
    if(desc_ptr == NULL)
      strcpy(pdesc, "Description Not Configured");
    else
      strcpy(pdesc, desc_ptr);
  
    if(verbose_flag)
        fprintf(stderr, "updating label\n");
  
    /* update the volume label */

    
    /* TEST FOR ICD STRUCTURE */
    /*  this is retained for future support of intermediate products   */
    /*  the initial test is accomplished during product load  */
    if(test_for_icd(gp->divider, gp->elev_ind, gp->vol_num, TRUE)==FALSE) 
           icd_format = FALSE;
    else
           icd_format = TRUE;
    
    /* TEST FOR PRESENCE OF PRE-ICD HEADER */
    /* this only works because of the way we loaded the product */

    if(hdr->g.vol_num==0 && get_elev_ind(sd->icd_product,orpg_build_i)==0)
           pre_icd = FALSE;
    else
           pre_icd = TRUE;
    
    if(pre_icd == FALSE) { /* no pre-ICD header */
           if(icd_format == FALSE) { /* not icd format and no header */
              sprintf(sel_prod_buf, "%d  Vol # N/A  Elev # N/A  %s", hdr->g.prod_id, 
                       pdesc);
           } else { /* icd format and no header */
              sprintf(sel_prod_buf, "%d  Vol # N/A  Elev #%d  %s", hdr->g.prod_id, 
                      gp->elev_ind, pdesc);
           }
    } else { /* there is a pre-ICD header */
               sprintf(sel_prod_buf, "%d  Vol #%d  Elev #%d  %s", hdr->g.prod_id, hdr->g.vol_num, 
                      get_elev_ind(sd->icd_product,orpg_build_i), pdesc);
    }


return GOOD_LOAD;

} /*  END post_load_setup */




