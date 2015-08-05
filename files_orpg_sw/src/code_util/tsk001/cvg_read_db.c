/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:35 $
 * $Id: cvg_read_db.c,v 1.6 2009/05/15 17:52:35 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* cvg_read_db.c */
#include "cvg_read_db.h"

#include "byteswap.h"

/*  globals */
    int build_num;
 
    int sort_method;
    int max_list_size;  
      
    /*  for testing internal data list */
    int printed=0, printed2=0;



/* /// SIGNAL HANDLERS - to be moved to exec'd executable */
void read_db_SIGUSR1()
{
    readFlag = CHILD_P_USR1;
}


void read_db_SIGUSR2()
{
    continueFlag = CHILD_P_USR2;    
}


void read_db_SIGTERM()
{
    continueFlag = CHILD_P_TERM;    
}




/*  EXEC'D EXECUTABLE ////////////////////////////// */
/*  */
/*  included product_names.c, helpers.c, assoc_array_s.c, and */
/*  byteswap.c in the sourc code list.  moved a few  */
/*  functions into helpers.c (read_to_eol, get_elev_ind,  */
/*  check_for_directory_existence)  */
/*  */
/* IMPROVEMENT: modified the load_product_names function so that */
/*  product_names.c can be used and eliminate redundant code */
/*   */
/*  only need libinfr,  */
/*  */
/*  the following function became 'main' */
/* //////////////////////////////////////////////////////// */


/*  MAIN FUNCTION ////////////////////////////////////// */
int main(int argc, char* argv[]) 
{


    int debug_child=FALSE;
    
    int parent_id;

    /*  setup singal action structures */
    struct sigaction act1, act2, actT;
    
    act1.sa_handler = read_db_SIGUSR1;
    act2.sa_handler = read_db_SIGUSR2;
    actT.sa_handler = read_db_SIGTERM;
    sigemptyset(&act1.sa_mask);
    sigemptyset(&act2.sa_mask);
    sigemptyset(&actT.sa_mask);
    act1.sa_flags = act2.sa_flags = actT.sa_flags = 0;
    
    /*  catch the indicated signals */
    sigaction(SIGUSR1, &act1, 0);
    sigaction(SIGUSR2, &act2, 0);
    sigaction(SIGTERM, &actT, 0); 
    

    /* // exec'd executable, read preferences */
    
    /*  1. get config_dir  ///////////////////////////////////// */
    init_read_db_prefs();
    
    /*  2. Get name string arrays ////////////////////////////// */

    use_cvg_list_flag = read_descript_source();
    
    assoc_init_s(&product_names);
    assoc_init_s(&short_prod_names);
    assoc_init_s(&product_mnemonics);
    
    load_product_names(TRUE,FALSE); /*  from child process, not first read */
    
    /*  3. Get maxProducts /////////////////////////////////////// */
    read_db_size();

    max_list_size = maxProducts+1;  

    
    /*  INTERNAL PRODUCT LIST */

    /*  4. Initialize Internal Lists /////////////////////////////// */

    db_sort_list = (int *) malloc(max_list_size * sizeof(int) );
                                         
    data_element_list = (Prod_data_element_t *) malloc( 
                                    max_list_size * sizeof(Prod_data_element_t) );

/* DEBUG */
/* fprintf(stderr,"DEBUG READ DB, Sizeof data element list item is %d bytes.\n",  */
/*                                          sizeof(Prod_data_element_t) ); */
                                                                             
     
     
    /*  NOTE: 112 is based upon the db_entry_string[112] in global2.h */
    
    /*  PRODUCT LIST MESSAGE written to the LB ////////////// */
    
    
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
 


    list_message_size = 8 + (max_list_size*112) + (max_list_size*sizeof(int)) + 
                     (max_list_size*sizeof(short)) + (max_list_size*sizeof(short));
                                                    
    descript_offset = 8;
    sort_time_offset = descript_offset + (max_list_size*112);
    msg_num_offset = sort_time_offset + (max_list_size*sizeof(int));    
    elev_num_offset = msg_num_offset + (max_list_size*sizeof(short));
    
    prod_list_message = (char *) malloc( list_message_size );
    
/*  DEBUG */
/* fprintf(stderr,"DEBUG CHILD - total size of list message is %d\n", */
/*                                list_message_size);  */
/* fprintf(stderr, */
/*          "DEBUG CHILD - offset to description is %d, to message num is %d\n", */
/*                                descript_offset, msg_num_offset);  */

    list_header = (Prod_list_hdr *) prod_list_message;
    
    /*  the list of 112 character list entries */
    product_listP = (db_entry_string *)(prod_list_message + descript_offset);

    /*  the sort time list (vol time with method 1 or sequence num with method 2) */
    sort_time_total_listP = (unsigned int *)(prod_list_message + sort_time_offset);
    
    /*  the message number list */
    msg_num_total_listP = (short *) (prod_list_message + msg_num_offset);

    /*  the elevation number list (used by animation) */
    elev_num_total_listP = (short *) (prod_list_message + elev_num_offset);



    /*  5. Create the List LB File /////////////////////////////////// */
    create_list_file();
    
    
    /*  6. Get Remaining Preferences ///////////////////////////////// */
    
    /*  Get database filename */
    get_db_filename();    

   /*  Get orpg build number */
   build_num = read_orpg_build();
   
   /*  Get the desired sort method */
   sort_method = read_sort_method();    


/*  DEBUG  */
if(debug_child) {         
fprintf(stderr,"DEBUG CHILD - maxProducts is %d, database filname is \n %s\n",
                               maxProducts, prod_db_filename);
fprintf(stderr,"DEBUG CHILD - build_num is %d, verbose flag is %d\n",
                               build_num, verbose_f);   
}

    /*  7.Get parent ID */
    parent_id = getppid();
    
/*  DEBUG */
if(debug_child)
fprintf(stderr," DEBUG CHILD parent_id is %d\n", parent_id);


    /*  CHILD PROCESS LOOP */
    while(1) {
        
        if(readFlag==CHILD_P_USR1) {
/*  DEBUG */
if(debug_child)
fprintf(stderr,"DEBUG CHILD READING PREFERENCES, read signal flag is %d\n",
       readFlag );
       
            readFlag = CHILD_P_CONT; 
            /*  READ PREFERENCES: HERE database filename, orpg build, sort method  */
            /*                    and build new description list */
            get_db_filename();
            build_num = read_orpg_build();
            sort_method = read_sort_method();
            
            use_cvg_list_flag = read_descript_source();
            assoc_clear_s(product_names);
            assoc_clear_s(short_prod_names);
            assoc_clear_s(product_mnemonics);
            load_product_names(TRUE,FALSE); /*  from child process, not first read */
/*  DEBUG */
if(debug_child) {
fprintf(stderr,"DEBUG CHILD - maxProducts is %d, database filname is %s\n",
                        maxProducts, prod_db_filename);
fprintf(stderr,"DEBUG CHILD - orpg build number is %d, verbose flag is %d\n",
                        build_num, verbose_f); 
}                        
           /*  changed database so list_message should be empty */
           /*  send initialize flag of -1 */
           blank_prod_list();
           write_empty_message(-1);
                                                    
        } /*  end if CHILD_P_USR1 */


        
/*  DEBUG */
if(debug_child)
fprintf(stderr,"DEBUG CHILD - BUILDING NEW DATABASE LIST\n");  
        
         build_database_list();             


        
        if(continueFlag==CHILD_P_USR2 || continueFlag==CHILD_P_TERM) {
/*  DEBUG */
if(debug_child)
fprintf(stderr,"DEBUG CHILD STOP HERE, continue signal flag is %d\n",
       continueFlag );
       
            continueFlag = CHILD_P_CONT; 
            break;
            
        } /*  end if CHILD_P_USR2 || CHILD_P_TERM */
        
        
        sleep(5);   /*  don't spend all of the time building lists */
        
        
        /*  has my parent crashed? */
        if( kill(parent_id, 0) == -1 ) {
/*  DEBUG */
if(debug_child)
fprintf(stderr, "DEBUG CHILD STOP HERE, pid of parent task not valid\n");

            free(db_sort_list);
            free(data_element_list);   
            free(prod_list_message);
            /*  may modify to open and close as required */
            LB_close(list_lbfd);
            exit(0);
            
        } /*  end if parent has crashed (not valid pid) */
        

    } /*  end while */



    free(db_sort_list);
    free(data_element_list);   
    free(prod_list_message);
    /*  may modify to open and close as required */
    LB_close(list_lbfd); 
    return 0;


} /*  end main */



/* ////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////// */
/*  CREATING AND WRITING THE DATABASE LIST */


/* ///////////////////////////////////////////////////////////////////////// */
void build_database_list()
{
   int db_lbfd=-1;

   int listSize = maxProducts+1;

   LB_info list[listSize]; 
   char lb_data[216];
   int result;   
   int num_products;
   
   int i=0, j=0, len;
   int internal_list_size=0;
   Prod_header *hdr;
   Graphic_product *gp;
   

   
   /* parameters in product description block */   
   short divider, elev_ind, vol_num; 

 
   unsigned int vol_time_sec, cur_sort_time;
   int vol_begin, vol_end;
   
   short cur_elev_ind;
   int elev_begin, elev_end;   
        

    
   int debug_build=FALSE;


  /* used for testing */
  /* for creating short vol time */
/*   time_t tval; */
/*   struct tm *t1; */
/*   char time_string[40]; // volume date-time */

/* DEBUG */
/* fprintf(stderr,"DEBUG child - building database list \n");    */

      /* if we have previously written an empty product message with the */
      /* init flag (-1) then replace it with a normal empty message      */
      
      if(list_header->prod_list_size  == -1) {
          
          write_empty_message(0);
      }
            
      /* open the product linear buffer */
      db_lbfd = LB_open(prod_db_filename, LB_READ, NULL);
      if(db_lbfd < 0) {
          if(verbose_f)
            fprintf(stderr, "ERROR opening the following linear buffer: %s\n",
                  prod_db_filename);
          blank_prod_list();
          write_empty_message(-3);
          sleep(2);
          return;
      } /*  end error opening database lb */

       
       list_header->prod_list_size=0;   
       msg_num_total_listP[0]=0;

        
       /* retrieve information from the linear buffer */
       num_products = LB_list(db_lbfd, list, maxProducts+1);
       if(debug_build) 
           fprintf(stderr, "num_products available=%i\n", num_products);
       if(num_products <= 0) {
           if(verbose_f)
           fprintf(stderr, "NOTE: the products database linear buffer contains "
                           "no products\n");
	       blank_prod_list();
	       write_empty_message(-2);
	              
           LB_close(db_lbfd); 
           sleep(2);
           return;
       }

/*  SHOULD WE BLANK THE PRODUCT LIST EVERY TIME? */
/*        blank_prod_list(); */

/*  SHOULD WE BLANK THE DATA LIST EVERY TIME? */
       blank_data_list();

/*  SHOULD WE BLANK THE SORT LIST EVERY TIME? */
       blank_sort_list();

/*  IF WE ALWAYS DO A BLANK HERE, WE SHOULD ELIMINATE THE  */
/*  BLANKS ACCOMPLISHED ON ERROR! */
              
       if(debug_build)
           fprintf(stderr, "DEBUG CHILD creating list of availible products\n");



       j=0;
       
       for(i=1; i<=num_products; i++) {
         
         len = LB_read(db_lbfd, lb_data, 216, list[i].id);
         hdr = (Prod_header *)lb_data;
         gp = (Graphic_product *)(lb_data+96);


       /* ////// begin create new internal product list logic ///////////////////// */
       

      /* the db_sort_list contains the internal database message       */
      /* numbers which will be ordered by sorting on the volume time   */
      /* (or the alternate sequence number) then subsequently sorted   */
      /* on the elevation index then prod_id all data associated with  */
      /* the product used for sorting and forcreating the list is in a */
      /* separate array of structures havingthe index of the internal  */  
      /* message number                                                */
         /*  used to test for a final (icd) product          */
         divider = gp->divider;
         elev_ind = gp->elev_ind;
         vol_num = gp->vol_num;
         

#ifdef LITTLE_ENDIAN_MACHINE
	     divider = SHORT_BSWAP(divider);
	     elev_ind = SHORT_BSWAP(elev_ind);
	     vol_num = SHORT_BSWAP(vol_num);
#endif

/* TEMP TEST ////////////////// */
if(printed2==0) {
/* fprintf(stderr,"DEBUGGING BUILD DATA LIST=====i = %d ====== j = %d =======\n",  */
/*                                                                           i, j); */
/* fprintf(stderr,"divider is %d,  elev_ind is %d,  vol_num is %d, TEST is %d\n", */
/*                 divider, elev_ind, vol_num,  */
/*                               test_for_icd(divider, elev_ind, vol_num, FALSE) ); */
if(i==num_products)
    printed2 = 1;
} 
/* END TEST //////////////////// */


         /*  THE INTERNAL LIST INCLUDES BLANK ENTRIES FOR EXPIRED PRODUCTS, */
         /*  AND INTERMEDIATE PRODUCTS INTERLACED WITH GOOD ENTRIES. The  */
         /*  db_sort_list[] only contains indeces for good final products */

         /*  only add valid final products, not include EXPIRED or intermediat  */
         /*  products with modified logic we could include intermediat products */
         if( (len != LB_EXPIRED) && 
             ((len > 0) || (len == LB_BUF_TOO_SMALL)) &&
              (test_for_icd(divider, elev_ind, vol_num, TRUE)==TRUE) ) {
                
              /*  sort index list index is a basic sequence */
              db_sort_list[j]=i;              
;
              /* element list index is the current message number i */

              data_element_list[i].prod_id=hdr->g.prod_id;
                             
              /*  the product elevation index is 0 for volume products */
              /*          used by product animation */
              data_element_list[i].elev=elev_ind;

              /* using the internal elevation index places volume products */
              /* into the elevation they were produced( typically 4 or max elev ) */
              data_element_list[i].elev_index=get_elev_ind(lb_data,build_num);
              
              /* will contain empty (meaningless entries) for expired products etc. */
              data_element_list[i].divider=divider;
              
              data_element_list[i].vol_num=vol_num;
              /*  if not an icd then vol_num would have to be determined  */
              /*     from an icd product having the same volume time */
              
              data_element_list[i].prod_code=gp->prod_code;
              data_element_list[i].vol_date=(unsigned short)gp->vol_date;
              data_element_list[i].vol_time_ms=(unsigned short)gp->vol_time_ms;
              data_element_list[i].vol_time_ls=(unsigned short)gp->vol_time_ls;
                          
#ifdef LITTLE_ENDIAN_MACHINE
              data_element_list[i].prod_code = 
                                    SHORT_BSWAP(data_element_list[i].prod_code);
              data_element_list[i].vol_date = 
                                    SHORT_BSWAP(data_element_list[i].vol_date); 
              data_element_list[i].vol_time_ms = 
                                    SHORT_BSWAP(data_element_list[i].vol_time_ms);
              data_element_list[i].vol_time_ls = 
                                    SHORT_BSWAP(data_element_list[i].vol_time_ls);
#endif
         
              vol_time_sec = ( ((int)(data_element_list[i].vol_time_ms) <<16) |
                             ((int)(data_element_list[i].vol_time_ls) & 0xffff) );

              data_element_list[i].sort_time = 
                   (unsigned int) _88D_unix_time(( data_element_list[i].vol_date),
                                               (vol_time_sec*1000) );
                                            
/* same as _88D_unix_time */
/*               data_element_list[i].sort_time = (unsigned int)   */
/*                       ( (data_element_list[i].vol_date-1)*86400 + vol_time_sec); */

              /* volume time note: the easiest volume time to use would be the  */
              /* volume time within the internal 96 byte header.  We do not use */
              /* this volume time because the TDWR products modify the volume   */
              /* in the product in order to provide a reference for animating   */
              /* the repeated base scans and for dividing the TDWR scanning     */
              /* strategy into two WSR-88D like volumes                         */
              /* Using the product volume time does not work in two cases on a  */
              /* Linux platform (has been fixed in Build 8):                    */
              /*      1. intermediate products (which we currently do not list) */
              /*      2. the RCS (84) and VCS (83) products which have incorrect*/
              /*         product volume times on Linux (a bug)                  */
              
              /*  if not an icd product then the following could be used for volume  */
              /*  time but this does not work if using sort method 2, see below... */
              data_element_list[i].hdr_vol_time=(unsigned int)hdr->g.vol_t;

/* /////////////////////////////////////////////////////////////////////////// */
/* Added the following work-around  back in in order to support saved */
/*         databases from ORPG's prior to Build 8. */
#ifdef LITTLE_ENDIAN_MACHINE
              /* fix for ICD vol_time bug on pre-Build 8 Linux for RCS(84) and VCS(83) */
              /* must use the internal header volume time */
              /* HAS BEEN FIXED IN BUILD 8! */
              
              if(build_num < 8) { 
                
                  if( data_element_list[i].prod_id==83 || 
                      data_element_list[i].prod_id==84 ) {
    /*  DEBUG   */
    /* fprintf(stderr,"DEBUG read_database prodid %d,  vol_time: %d, hdr_vol_time: %d.\n", */
    /*             data_element_list[i].prod_id, data_element_list[i].sort_time,  */
    /*                                         data_element_list[i].hdr_vol_time ); */
                        
                        data_element_list[i].sort_time = data_element_list[i].hdr_vol_time;
              
                  } /* end if id is 83 or 84 */
              
              } /*  ind if before build 8 */
#endif
/* ////////////////////////////////////////////////////////////////////////// */

              /*  WHEN THE sort_method == 2, put the product vol time in the  */
              /*  hdr_vol_time and replace the date/time in sort_time with sequence  */
              /*  number LIMITATION, cannot use sort method 2 if list contains  */
              /*  intermediate products. */
              if(sort_method == 2) {
                  data_element_list[i].hdr_vol_time = 
                                   data_element_list[i].sort_time; /*  prod vol time */
                  data_element_list[i].sort_time = 
                                   hdr->g.vol_num; /*  vol sequence number */
              }
    
               /* NOTE, WE NEED THE ORIGINAL VOL_TIME WHEN CREATING 
                * THE TIME STRING LATER! 
                */
              
              j++; 
         } /*  end if this is an ICD product that has not expired */
         
            
              
         internal_list_size = j;
              
 /* /////// end new internal list logic  ///////////////////////////  */
 
/* TEMP TEST /////////////////////// */
if(printed==0) {
/* fprintf(stderr,"DEBUGGING BUILD DATA LIST===== i = %d ====== j == %d =======\n", */
/*                         i, j); */
/* fprintf(stderr,"sort_time=%d,  hdr_vol_time=%d, elev_index=%d, prod_id=%d\n", */
/*             data_element_list[i].sort_time,  data_element_list[i].hdr_vol_time,   */
/*             data_element_list[i].elev_index,  data_element_list[i].prod_id ); */
/* fprintf(stderr,"  vol_num = %d,            new elev number=%d, prod_code=%d\n", */
/*                 data_element_list[i].vol_num,  data_element_list[i].elev,   */
/*                 data_element_list[i].prod_code ); */
if(i==num_products)
    printed = 1;
}
/* END TEST /////////////// */
                          
       } /*  end for i <= num_products */
              
              

/* ////// begin internal list test logic //////////////////////////////// */
/*  */
/* //DEBUG // */
/* //fprintf(stderr, "\nDEBUG INTERNAL LIST: list size is %d items\n",j); */
/*    */
/* //      for(i=0;i<internal_list_size;i++) */
/* //           print_new_list_item(data_element_list[db_sort_list[i]],  */
/* //                                                  db_sort_list[i], i);  */
/*  */
/*  */
/* //for(i=0;i<internal_list_size;i++) {  */
/* //  fprintf(stderr,"Data Element[%4d] ", i ); */
/* //  fprintf(stderr,"VOL_TIME ICD:%9d  HDR:%9d\n",  */
/* //        data_element_list[i].sort_time, data_element_list[i].hdr_vol_time);  */
/* //}             */
/* /////  end internal list test logic  ////////////////////////////////// */


/* //  begin internal list sort logic ////////////////////////////////////// */

      heapsort_vol(db_sort_list, internal_list_size);

      vol_begin=0;
      vol_end=0;
      cur_sort_time=data_element_list[db_sort_list[0]].sort_time;
      /*  now within each volume time, sort on elevation index */
      for(i=0; i<internal_list_size; i++) {
              
          /*  short cut for end of list */
          if( i==internal_list_size-1 ) {
              if(data_element_list[db_sort_list[i]].sort_time > cur_sort_time ) 
                  vol_end=i-1;
              else 
                  vol_end=i;
/* DEBUG */
/* fprintf(stderr,"\nCalling 'dbSort_ele' with begin %4d,  end %4d,  size %d\n",  */
/*         vol_begin, vol_end,  vol_end-vol_begin+1 );    */
                 
              heapsort_ele( (db_sort_list+vol_begin), (vol_end-vol_begin+1) ); 
              
              break;
                            
          } else {
            
              if( data_element_list[db_sort_list[i]].sort_time > cur_sort_time ) {
                  vol_end=i-1;

/* DEBUG */
/* fprintf(stderr,"\nCalling 'heapsort_ele' with begin %4d,  end %4d,  size %d\n",  */
/*         vol_begin, vol_end,  vol_end-vol_begin+1 );  */
                  
                  heapsort_ele( (db_sort_list+vol_begin), (vol_end-vol_begin+1) );

                  vol_begin=i;
                  cur_sort_time=data_element_list[db_sort_list[i]].sort_time;
              } /*  end if */
           
          } /*  end else */
                              
      } /*  end for  */


      /* / now sort for product id within each elevation index */
      /* / must account for same/different volumes */
      elev_begin=0;
      elev_end=0;
      cur_sort_time=data_element_list[db_sort_list[0]].sort_time;
      cur_elev_ind=data_element_list[db_sort_list[0]].elev_index;
      
      for(i=0; i<internal_list_size; i++) {
              
          /*  short cut for end of list */
          if( i==internal_list_size-1 ) {
              if( (data_element_list[db_sort_list[i]].sort_time > cur_sort_time) ||
                  (data_element_list[db_sort_list[i]].elev_index != cur_elev_ind) )
                  elev_end=i-1;
              else 
                  elev_end=i;
/*             future enhancement, provide a switch between product id and  */
/*                                 product code  */
/*               heapsort_pcd( (db_sort_list+elev_begin), (elev_end-elev_begin+1) ); */
              heapsort_pid((db_sort_list+elev_begin), (elev_end-elev_begin+1));
              
              break;
                            
          } else {
            
              if( (data_element_list[db_sort_list[i]].sort_time>cur_sort_time) ||
                  (data_element_list[db_sort_list[i]].elev_index!=cur_elev_ind) ) {
                  elev_end=i-1;
                  
/*                 future enhancement, provide a switch between product id and  */
/*                                     product code */
/*                   heapsort_pcd((db_sort_list+elev_begin), (elev_end-elev_begin+1)); */

                  heapsort_pid((db_sort_list+elev_begin), (elev_end-elev_begin+1));
                  
                  elev_begin=i;
                  cur_sort_time=data_element_list[db_sort_list[i]].sort_time;
                  cur_elev_ind=data_element_list[db_sort_list[i]].elev_index;
                  
              } /*  end if */
           
          } /*  end else */
                              
      } /*  end for  */


      /* / end sort for product ID */

/* //  end internal list sort logic /////////////////////////////////////// */




/* ////// begin sorted list test logic //////////////////////////////// */
/*  */
/* //DEBUG // */
/* fprintf(stderr, "\nDEBUG SORTED LIST: list size is %d items\n",j); */
/*  */
/*  */
/* //      for(i=2080;i<2100;i++)  */
/* //           print_new_list_item(data_element_list[db_sort_list[i]],  */
/* //                                                   db_sort_list[i], i);  */
/*  */
/*  */
/* //      for(i=2080;i<2170;i++)  */
/* //           print_new_list_item(data_element_list[db_sort_list[i]],  */
/* //                                                   db_sort_list[i], i);    */
/*  */
/*       for(i=0;i<internal_list_size;i++)  */
/*            print_new_list_item(data_element_list[db_sort_list[i]],  */
/*                                                      db_sort_list[i], i);    */
/*      */
/*       fprintf(stderr,"\n"); */
/*  */
/* //      for(i=(internal_list_size-100); i<internal_list_size-20; i++)  */
/* //           print_new_list_item(data_element_list[db_sort_list[i]],  */
/* //                                                   db_sort_list[i], i);  */
/*  */
/* /////  end sorted list test logic  ////////////////////////////////// */

        
       LB_close(db_lbfd);    


/* //// new create message list logic ///////////////////////////////////// */

       /*  create message from internal sorted list */
       for(i=0; i<num_products; i++) {  
        
           process_list_item(db_sort_list[i]); 
             
       } 

/* //// end new create message list logic ///////////////////////////////// */


       /*  write the linear buffer message. */

       list_header->max_list_size = listSize;

/* DEBUG */
/* fprintf(stderr, "DEBUG CREATE LIST - maximum list size is %d, "  */
/*                 "number of products is %d\n", */
/*              list_header->max_list_size, list_header->prod_list_size); */

       result=LB_write(list_lbfd, prod_list_message, list_message_size, 208);

       if(result<0) {
       fprintf(stderr,"ERROR Writing Database List Message, code %d\n",result);
                
        
       } /*  end if */
        
    
} /*  end build_database_list */





/* ///////////////////////////////////////////////////////////////////// */
void process_list_item(int msg_num)
{


  char  *p_desc;
  char *mnemonic;


  char prod_desc[80]; /*  hold product description, only first 71 char used */

  char prod_mnemon[4]; /*  hold 3 chararacter mnemonic, padded with blanks  */

  /* for creating short vol time */
  time_t tval;
  struct tm *t1;
  char time_string[40]; /*  volume date-time */
  short pcode;
  int volume;
  static int last_vol_num=0;
  /* parameters in product description block */




  int item_debug_flag = FALSE;


/* DEBUG */
if(item_debug_flag)
fprintf(stderr, "\nDEBUG CHILD process_list_item - entered, index %d\n", msg_num);


    /*  currently the sorted list does not include intermediate products */
    /*  but this is included if we change our mind */
    /*   this logic works if there is an icd product from the same volume */
    /*   previously in the list */
    if(test_for_icd(data_element_list[msg_num].divider, 
                         data_element_list[msg_num].elev_index, 
                         data_element_list[msg_num].vol_num,
                                                    TRUE)==TRUE) {
        last_vol_num = data_element_list[msg_num].vol_num;
        volume = data_element_list[msg_num].vol_num; 
    } else { /* not an icd product */
        volume = last_vol_num;
        pcode = 0;
    }

    

/* DEBUG */
if(item_debug_flag)
fprintf(stderr, "DEBUG CHILD process_list_item - header info complete, "
                "volume is %d, pcode is %d\n",
                            volume, data_element_list[msg_num].prod_code);

    /*  Note: for all outputs, we add one to the message number  */
    /*        so list begins with 1 */

/* DEBUG */
if(item_debug_flag)
fprintf(stderr, "DEBUG CHILD looking up p_desc and mnemonic for prod id %d\n",
                                          data_element_list[msg_num].prod_id); 
         
    /* The first message in not a product message (Build 3 change) */
    if(msg_num==0) {
    /*  we currently do not display first message   */
    
/*  DEBUG */
if(item_debug_flag)
fprintf(stderr,"CHILD DEBUG creating internal message p_desc\n");

       
       strcpy(prod_desc, "*** INTERNAL DATABASE MESSAGE ***");
       strcpy(prod_mnemon, "   ");
  
 
    } else {

/*  DEBUG */
if(item_debug_flag)
fprintf(stderr,"CHILD DEBUG creating product message p_desc\n");

        /* if the product has a p_desc use it, otherwise use a default */

        p_desc = assoc_access_s(product_names, data_element_list[msg_num].prod_id);

/* TEST */
/* fprintf(stderr,"TEST CHILD prod %d description is %s\n", */
/*                 data_element_list[msg_num].prod_id, p_desc); */


/*  DEBUG */
if(item_debug_flag)              
fprintf(stderr,"CHILD DEBUG finished reading p_desc\n");

        mnemonic = assoc_access_s(product_mnemonics, 
                                   data_element_list[msg_num].prod_id);
        
/*  DEBUG */
if(item_debug_flag)
fprintf(stderr,"CHILD DEBUG finished reading mnemonic\n");  
      
        if(p_desc == NULL) {

/*  DEBUG */
if(item_debug_flag)
fprintf(stderr,"CHILD DEBUG unknown product, p_desc was NULL\n");

            strcpy(prod_desc, "--- Description Not Configured ---");
            strcpy(prod_mnemon, "   ");

        } else { 
            /* CVG 9.0 - NEED TO KEEP prod_desc to a length of 69 plus '\0' */
            if( strlen(p_desc) > 69 ) {
                strncpy(prod_desc, p_desc, 69);
                prod_desc[69] = '\0';
            } else {
                strcpy(prod_desc, p_desc);
            }

          if(mnemonic==NULL)
              strcpy(prod_mnemon, "---");
          else
              strcpy(prod_mnemon, mnemonic);                    
        } /*  end else not NULL */
        
    }  /*  end else a normal product */

/* DEBUG */
if(item_debug_flag) {
fprintf(stderr, "DEBUG CHILD process_list_item - p_desc & mnemonic complete\n");
fprintf(stderr, "            p_desc is %s, mnemonic is %s\n", 
                                    prod_desc, prod_mnemon);
fprintf(stderr,"DEBUG CHILD - prod_list_size is %d\n", 
                                    list_header->prod_list_size);
}

    
    /* create a string to be placed into the list */
    /* The first message in not a product message (Build 3 change) */
    if(msg_num==0) {
    /*  we currently do not display first message */
        ;
/*         sprintf(product_listP[list_header->prod_list_size], */
/*        "     N/A     N/A N/A   N/A    N/A  N/A   %s", */
/*         prod_desc); */
/*         (list_header->prod_list_size)++; */
/* // CVG 6.5 */
/*         sort_time_total_listP[list_header->prod_list_size] =  */
/*                                        data_element_list[msg_num].sort_time; */
/*         msg_num_total_listP[list_header->prod_list_size] = msg_num+1; */
/* DEBUG */
/* if(item_debug_flag) */
/* fprintf(stderr,"DEBUG CHILD - finished first message, prod_list_size  %d\n",  */
/*             list_header->prod_list_size);   */

            
/*  an example of eliminating certain products from the list     */
/* //    else if(data_element_list[msg_num].prod_id == 119) */
/* //        // we currently do not list the CFC product */
/* //        ;      */
/* // */
    
    } else {  /* a normal product listing */
   /* create short date */
         if(sort_method == 1)
              tval=(time_t)data_element_list[msg_num].sort_time;
         else
              tval=(time_t)data_element_list[msg_num].hdr_vol_time;
         t1 = gmtime(&tval);
      /* YYYY is t1->tm_year+1900, MM is t1->tm_mon+1, DD is t1->tm_mday, 
       * HH is tm_hour, MM is t1->tm_min, SS is t1->tm_sec 
       */
         sprintf(time_string,"%02d/%02d-%02d:%02d", 
                 t1->tm_mon+1, t1->tm_mday, t1->tm_hour, t1->tm_min);
        /* end create short date */
        
/* DEBUG */
if(item_debug_flag)
fprintf(stderr,"DEBUG CHILD finished creating the time string\n");
        

        /*  total string length is the same for both methods */
        if(sort_method==1) /*  the original volume date-time */
            /* CVG 9.0 - improve alignment of labels and list on some X-servers */
            /*             added space before pcode                               */
            sprintf(product_listP[list_header->prod_list_size],
                " %s  %02d  %02d  %4d    %s  %4d  %s", 
                time_string, volume,
                data_element_list[msg_num].elev_index, 
                data_element_list[msg_num].prod_id, prod_mnemon, 
                data_element_list[msg_num].prod_code, prod_desc);
        else  /*  use the volume sequence number */

            /* CVG 9.0 - improve alignment of labels and list on some X-servers */
            /*             added space before mnemonic and pcode                  */
            sprintf(product_listP[list_header->prod_list_size],
                "%s  %05d  %02d  %4d   %s  %4d  %s", 
                time_string, data_element_list[msg_num].sort_time,
                data_element_list[msg_num].elev_index, 
                data_element_list[msg_num].prod_id, prod_mnemon, 
                data_element_list[msg_num].prod_code, prod_desc);

/* DEBUG */
if(item_debug_flag) {
fprintf(stderr,"DEBUG CHILD finished creating the list entry string:\n");
fprintf(stderr,"            %s\n", product_listP[list_header->prod_list_size]);
fprintf(stderr," the internal message number is %d\n", 
                             msg_num_total_listP[list_header->prod_list_size]);
}
        (list_header->prod_list_size)++;
       
        sort_time_total_listP[list_header->prod_list_size] = 
                                         data_element_list[msg_num].sort_time;
        msg_num_total_listP[list_header->prod_list_size] = msg_num+1;
        elev_num_total_listP[list_header->prod_list_size] = 
                                              data_element_list[msg_num].elev;
/* TEST */
/* fprintf(stderr,"TEST PROCESS LIST ITEM - msg_num is %d, elev_num is %d, " */
/*                "index is %d\n", */
/*         msg_num_total_listP[list_header->prod_list_size],  */
/*         elev_num_total_listP[list_header->prod_list_size], */
/*         data_element_list[msg_num].elev_index ); */


/*  DEBUG               */
if(item_debug_flag)              
fprintf(stderr," the internal message number is %d\n", 
                            msg_num_total_listP[list_header->prod_list_size]);
        
    }  /*  end else a normal product */
                    
} /*  end process_list_item */







/* ///////////////////////////////////////////////////////////////// */

void create_list_file()
{
 
    static LB_attr attr;   


    /* define the list filename*/
    sprintf(list_filename, "%s/cvg_db_list.lb", config_dir); 
       
      LB_remove (list_filename);

      
      attr.mode = 0666;
      
      attr.msg_size = 0;
      attr.maxn_msgs = 1; 
     
      attr.types = LB_FILE | LB_REPLACE | LB_SINGLE_WRITER;      
           
      list_lbfd = LB_open (list_filename, LB_CREATE, &attr);
      
      if (list_lbfd < 0) {
         fprintf (stderr,"Failure to create list buffer file: '%s' (ret = %d)\n",
                  list_filename, list_lbfd);
         fprintf (stderr,"lb message size is %d, attribute types is 0x%x \n", 
         attr.msg_size, attr.types);
	     exit (0);
	  }
	  
	  
	  /*  initially write an empty list */
	  blank_prod_list();
	  write_empty_message(0);
	  	  
	  
/* CONSIDER CLOSING HERE AND OPENING AND CLOSING AS REQUIRED	     */
/*       LB_close(list_lbfd);  */
    
} /*  end create_list_file */






/* ////////////////////////////////////////////////////////// */

void write_empty_message(int num_prod)
{
    int result;        
	  /*  write an empty list */

       list_header->prod_list_size=num_prod; 
       list_header->max_list_size = maxProducts+1;	
         
       result=LB_write(list_lbfd, prod_list_message, list_message_size, 208);

       if(result<0) {
           fprintf(stderr,"ERROR Writing Database List Message, code %d\n", 
                                                                     result);
	   } 
	           
} /*  end write_empty_message */





/* //////////////////////////////////////////////////////////// */
void blank_prod_list()
{
    int i;

    list_header->prod_list_size=0;
    
    for(i=0;i<max_list_size; i++) {
        sprintf(product_listP[i], "%s","");
        msg_num_total_listP[i] = 0;
        sort_time_total_listP[i] = 0;
        elev_num_total_listP[i] =0;
    }
    
}




void blank_data_list()
{
    int i;


    for(i=0; i<max_list_size; i++) {
        data_element_list[i].sort_time = 0;
        data_element_list[i].hdr_vol_time = 0;
        data_element_list[i].elev_index = 0;
        data_element_list[i].divider = 0;
        data_element_list[i].vol_num = 0;
        data_element_list[i].vol_date = 0;
        data_element_list[i].vol_time_ms = 0;
        data_element_list[i].vol_time_ls = 0;
        data_element_list[i].elev = 0;
        
    }

}



void blank_sort_list()
{
    int i;
    
    for(i=0; i<max_list_size; i++) 
        db_sort_list[i] = 0;
        
}





/* ////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////// */
/*  These functions must be used in the correct order! */
/*  */
/*  Sort first on sort time */
/*  Sort second on elevation index */
/*  Sort third on either product code or product id */
/*  */
/* ////////////////////////////////////////////////////////////////////// */
/* BEGIN  HEAPSORT */


/* // original sorts ints */
/* // */
/* //void heapsort(int a[], int array_size) { */
/* //  int i; */
/* //  for (i = (array_size/2 -1); i >= 0; --i) { */
/* //    downHeap(a, i, array_size-1); */
/* //  } */
/* //  for (i = array_size-1; i >= 0; --i) { */
/* //    int temp; */
/* //    temp = a[i]; */
/* //    a[i] = a[0]; */
/* //    a[0] = temp; */
/* //    downHeap(a, 0, i-1); */
/* //  } */
/* //} */
/* // */
/* // */
/* //void downHeap(int a[], int root, int bottom) { */
/* //  int maxchild, temp, child; */
/* //  while (root*2 < bottom) { */
/* //    child = root * 2 + 1; */
/* //    if (child == bottom) */
/* //      maxchild = child; */
/* //    else */
/* //    if (a[child] > a[child + 1]) */
/* //      maxchild = child; */
/* //    else    maxchild = child + 1; */
/* //    if (a[root] < a[maxchild]) { */
/* //      temp = a[root]; */
/* //      a[root] = a[maxchild]; */
/* //      a[maxchild] = temp; */
/* //    } else return; */
/* //    root = maxchild; */
/* //  } */
/* //} */


/*  sorts ints but uses comparison in another list */
void heapsort_vol(int a[], int array_size) {
  int i;
  for (i = (array_size/2 -1); i >= 0; --i) {
    downHeap_vol(a, i, array_size-1);
  }
  for (i = array_size-1; i >= 0; --i) {
    int temp;
    temp = a[i];
    a[i] = a[0];
    a[0] = temp;
    downHeap_vol(a, 0, i-1);
  }
}


void downHeap_vol(int a[], int root, int bottom) {
  int maxchild, temp, child;
  while (root*2 < bottom) {
    child = root * 2 + 1;
    if (child == bottom)
        maxchild = child;
    else
/*     if (a[child] > a[child + 1]) */
    if ( data_element_list[a[child]].sort_time > 
                                   data_element_list[a[child + 1]].sort_time )
        maxchild = child;
    else    maxchild = child + 1;
/*     if (a[root] < a[maxchild]) { */
    if ( data_element_list[a[root]].sort_time < 
                                   data_element_list[a[maxchild]].sort_time ) {
        temp = a[root];
        a[root] = a[maxchild];
        a[maxchild] = temp;
    } else return;
    root = maxchild;
  }
}




void heapsort_ele(int a[], int array_size) {
  int i;
/* DEBUG */
/* fprintf(stderr,"\nInside 'heapsort_ele' with entry %4d,  array size %3d\n",  */
/*                                                            a[0], array_size); */
  for (i = (array_size/2 -1); i >= 0; --i) {
    downHeap_ele(a, i, array_size-1);
  }
  for (i = array_size-1; i >= 0; --i) {
    int temp;
    temp = a[i];
    a[i] = a[0];
    a[0] = temp;
    downHeap_ele(a, 0, i-1);
  }
}


void downHeap_ele(int a[], int root, int bottom) {
  int maxchild, temp, child;
  while (root*2 < bottom) {
    child = root * 2 + 1;
    if (child == bottom)
        maxchild = child;
    else
/*     if (a[child] > a[child + 1]) */
    if ( data_element_list[a[child]].elev_index > 
                                     data_element_list[a[child + 1]].elev_index )
        maxchild = child;
    else    maxchild = child + 1;
/*     if (a[root] < a[maxchild]) { */
    if ( data_element_list[a[root]].elev_index < 
                                     data_element_list[a[maxchild]].elev_index ) {
        temp = a[root];
        a[root] = a[maxchild];
        a[maxchild] = temp;
    } else return;
    root = maxchild;
  }
}




void heapsort_pid(int a[], int array_size) {
  int i;
/* DEBUG */
/* fprintf(stderr,"\nInside 'heapsort_pid' with entry %4d,  array size %3d\n",  */
/*                                                           a[0], array_size); */
  for (i = (array_size/2 -1); i >= 0; --i) {
    downHeap_pid(a, i, array_size-1);
  }
  for (i = array_size-1; i >= 0; --i) {
    int temp;
    temp = a[i];
    a[i] = a[0];
    a[0] = temp;
    downHeap_pid(a, 0, i-1);
  }
}


void downHeap_pid(int a[], int root, int bottom) {
  int maxchild, temp, child;
  while (root*2 < bottom) {
    child = root * 2 + 1;
    if (child == bottom)
        maxchild = child;
    else
/*     if (a[child] > a[child + 1]) */
    if ( data_element_list[a[child]].prod_id > 
                                      data_element_list[a[child + 1]].prod_id )
        maxchild = child;
    else    maxchild = child + 1;
/*     if (a[root] < a[maxchild]) { */
    if ( data_element_list[a[root]].prod_id < 
                                      data_element_list[a[maxchild]].prod_id ) {
        temp = a[root];
        a[root] = a[maxchild];
        a[maxchild] = temp;
    } else return;
    root = maxchild;
  }
}


void heapsort_pcd(int a[], int array_size) {
  int i;
/* DEBUG */
/* fprintf(stderr,"\nInside 'heapsort_pid' with entry %4d,  array size %3d\n",  */
/*                                                            a[0], array_size); */
  for (i = (array_size/2 -1); i >= 0; --i) {
    downHeap_pcd(a, i, array_size-1);
  }
  for (i = array_size-1; i >= 0; --i) {
    int temp;
    temp = a[i];
    a[i] = a[0];
    a[0] = temp;
    downHeap_pcd(a, 0, i-1);
  }
}


void downHeap_pcd(int a[], int root, int bottom) {
  int maxchild, temp, child;
  while (root*2 < bottom) {
    child = root * 2 + 1;
    if (child == bottom)
        maxchild = child;
    else
/*     if (a[child] > a[child + 1]) */
    if ( data_element_list[a[child]].prod_code > 
                                      data_element_list[a[child + 1]].prod_code )
        maxchild = child;
    else    maxchild = child + 1;
/*     if (a[root] < a[maxchild]) { */
    if ( data_element_list[a[root]].prod_code < 
                                      data_element_list[a[maxchild]].prod_code ) {
        temp = a[root];
        a[root] = a[maxchild];
        a[maxchild] = temp;
    } else return;
    root = maxchild;
  }
}



/* END  HEAPSORT */
/* ////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////// */






/* ////////TEST PRINT////////////////////////////////////////// */
/*  data_item is the structure at data_element_list[ db_sort_list[i] ] */
/*  msg_num is the internal message number at db_sort_list[i] */
/*  index is i */
/* ////////////////////////////////////////////////////////////////////// */
void print_new_list_item(Prod_data_element_t data_item, int msg_num, int index)
{
  /* for creating short vol time */
  time_t tval;
  struct tm *t1;
  char time_string[40]; /*  volume date-time */

      
    /* create short date */
    tval=(time_t)data_item.sort_time;
    t1 = gmtime(&tval);
    /* YYYY is t1->tm_year+1900, MM is t1->tm_mon+1, DD is t1->tm_mday, 
     * HH is tm_hour, MM is t1->tm_min, SS is t1->tm_sec 
     */
    sprintf(time_string,"%02d/%02d-%02d:%02d", 
             t1->tm_mon+1, t1->tm_mday, t1->tm_hour, t1->tm_min);
    /* end create short date */  
    
    
    fprintf(stderr,"INDEX[%4d] ", index);
    fprintf(stderr,"DATA_ELEMENT[%4d]: ", msg_num);
    fprintf(stderr,"vol time: %s  ", time_string);
    fprintf(stderr,"elev_index: %2d  ", data_item.elev_index);
    fprintf(stderr,"prod_id: %3d \n", data_item.prod_id);

/*     fprintf(stderr,"divider: %d ", data_item.divider); */
/*     fprintf(stderr,"vol_num: %2d ", data_item.vol_num);     */
/*     fprintf(stderr,"prod_code: %3d ", data_item.prod_code); */
/*     fprintf(stderr,"vol_time_ms: %hu, vol_time_ls: %hu ",  */
/*                      data_item.vol_time_ms, data_item.vol_time_ls); */
/* //debug */
/*     fprintf(stderr,"\n raw vol_time is %u, raw hdr_vol_time is %u ", */
/*                      data_item.sort_time, data_item.hdr_vol_time);    */
/*     fprintf(stderr,"\n"); */

}





/* /////////////////////////////////////////////////////////////////// */
/* /////////////////////////////////////////////////////////////////// */
/*  GETTING PREFERENCES */



/* //////////// EXTRACTED FROM init_prefs in prefs_load.c ///////// */
/*  gets config_dir from CVG_PREF_DIR_NAME and $HOME */

void init_read_db_prefs()
{

  char *home_dir;
  struct stat confdirstat;

  /*  The following errors should never occur because this program is not */
  /*  forked until after the preferences are initialized by CVG */
  
  /* if the HOME directory is not defined, EXIT */
  if((home_dir = getenv("HOME")) == NULL) {
      fprintf(stderr, "     ERROR*****************ERROR************ERROR*\n");
      fprintf(stderr, "     Environmental variable HOME not set.\n");
      fprintf(stderr, "          CVG launch aborted \n");
      fprintf(stderr, "     *********************************************\n");
      exit(0);
  }

  sprintf(config_dir, "%s/.%s", home_dir,CVG_PREF_DIR_NAME);

  /* make sure the local directory exists                        */
  /*    if the local configuration directory exists, do nothing  */
  /*                                                             */
  /*    if the directory does not exist, error message           */

  if(stat(config_dir, &confdirstat) < 0) {

      fprintf(stderr, "\n");
      fprintf(stderr, "     ERROR****************************************\n"); 
      fprintf(stderr, "     * CVG local config directory does not exist  \n"); 
      fprintf(stderr, "     *                                            \n"); 
      fprintf(stderr, "     *********************************************\n");
      fprintf(stderr, "\n");   
      exit(0); 
   }


} /*  end init_read_db_prefs */











/* ///////////// BASED UPON load_program_prefs IN PREFS_LOAD.C////////////////// */
void get_db_filename()
{
    char filename[150], *charval;
    char buf1[100], buf2[100];
    FILE *pref_file;

    /* open the program preferences data file */
    sprintf(filename, "%s/prefs", config_dir);
    if((pref_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "**********************************\n");
        fprintf(stderr, "*  Could not open preferences    *\n");
        fprintf(stderr, "*                                *\n");
        fprintf(stderr, "*  The file 'prefs' in the       *\n");
        fprintf(stderr, "*     ~/.cvg  directory is       *\n");
        fprintf(stderr, "*     either missing or corrupt  *\n");
        fprintf(stderr, "**********************************\n");
    exit(0);
    }


    /* get the path for the products database */
    charval = getenv("ORPG_PRODUCTS_DATABASE");

    /* if we can find it ... */
    if(charval == NULL) {
        prod_db_filename[0] = '\0';
/*  this is not an error for the standalone installation */
#ifdef LIB_LINK_DYNAMIC        
        if(verbose_f)
            fprintf(stderr,"WARNING: environmental variable for "
                           "ORPG_PRODUCTS_DATABASE is not set\n");
#endif
    } else {
        
   
        /* and it exists... */
        if(check_for_directory_existence(charval) == FALSE) {
            prod_db_filename[0] = '\0';
            
        } else {
            /* make it a default value */
            strcpy(prod_db_filename, charval);
        }
    } /*  end else not NULL */


    while(fscanf(pref_file, "%s %s", buf1, buf2) == 2) {
        
        if(strcmp(buf1, "verbose_output") == 0) {
            if(strcmp(buf2, "true") == 0) { 
                verbose_f = TRUE;
            } else {
                verbose_f = FALSE;
            }   
        } else if(strcmp(buf1, "product_database_lb") == 0) {
            strcpy(prod_db_filename, buf2);
            
        }       
        
    } /*  end while */
    

    fclose(pref_file);

    /* see if the database file exists*/
    if(check_for_directory_existence(prod_db_filename) == FALSE) {        
        fprintf(stderr,"NOTE: The CVG Configured Product Database File "
                       "Was Not Found at: \n");
        fprintf(stderr,"      '%s'\n",prod_db_filename);
        prod_db_filename[0] = '\0';
    } else {
        
       fprintf(stderr,"(Read Task) Reading Database File\n            '%s'\n",
                                                             prod_db_filename);
    }
    
    
    /*  initially write an empty list */
   
    write_empty_message(0);
                	  
        

} /*  end get_db_filename */






/* ///////////////////////////////////////////////////////////////// */

int read_orpg_build()
{
    char filename[150];
    FILE *build_file;
    int value;
 
    /* open the program preferences data file */
    sprintf(filename, "%s/orpg_build", config_dir);

    if((build_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "***********************************\n");
        fprintf(stderr, "*  Could not open orpg build file *\n");
        fprintf(stderr, "*                                 *\n");
        fprintf(stderr, "***********************************\n");
    exit(0);
    }

    fscanf(build_file, "%d", &value);

    fclose(build_file);

    return value;

}



/* ///////////////////////////////////////////////////////////////// */

int read_sort_method()
{
    char filename[150];
    FILE *sort_meth_file;
    int value;
 
    /* open the sort method file */
    sprintf(filename, "%s/sort_method", config_dir);

    if((sort_meth_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "************************************\n");
        fprintf(stderr, "*  Could not open sort method file *\n");
        fprintf(stderr, "*  Using default volume date-time. *\n");
        fprintf(stderr, "************************************\n");
    return 1;
    }

    fscanf(sort_meth_file, "%d", &value);

    fclose(sort_meth_file);

    return value;

}






/* ///////////////////////////////////////////////////////////////// */

int read_descript_source()
{
    char filename[150];
    FILE *desc_source_file;
    int value;
 
    /* open the sort method file */
    sprintf(filename, "%s/descript_source", config_dir);

    if((desc_source_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "**************************************\n");
        fprintf(stderr, "*  Could not open description source *\n");
        fprintf(stderr, "*  file. Using cvg description list. *\n");
        fprintf(stderr, "**************************************\n");
    return 1;
    }

    fscanf(desc_source_file, "%d", &value);

    fclose(desc_source_file);

    return value;

}







/* ////// COPIED FORM PREFS.C /////////////////////////////////////////// */
/*  commented out printf's */
/* checks to see if a file has been added to alter the 
 * product database list data structure size
 */
void read_db_size()
{
    char filename[150];
/*     FILE *radar_file; */
    FILE *db_size_file;
    int value;

    /* open the database size data file */
    sprintf(filename, "%s/prod_db_size", config_dir);
    
    if((db_size_file=fopen(filename, "r"))==NULL) {
        
/*         fprintf(stderr, "CVG Product DB List Size 16000 (default)\n"); */
        maxProducts=DEFAULT_DB_SIZE;
        
    } else {

          fscanf(db_size_file, "%d", &value);
          if(value < 7500)
              maxProducts=7500; 
          else if(value > 32000)
              maxProducts=32000;
          else
              maxProducts=value;
/*           fprintf(stderr, "CVG Product DB List Size %d\n", maxProducts); */
            
          fclose(db_size_file);
      
    }

    
} /*  end read_db_size */




