/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:44:55 $
 * $Id: anim.c,v 1.12 2008/03/13 22:44:55 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/* anim.c */
/* functions related to the animation of images on the screen */

#include "anim.h"
#include "byteswap.h"



/***********************************************************************/
/* SECTION 1.  MAJOR ANIMATION CALLBACKS AND TOP LEVEL LOGIC FUNCTIONS */
/***********************************************************************/
               
/* displays the previous set of images in the chosen series */
void back_one_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    /*  the auxiliary screen (screen3) is not linked */
    if( (XtParent(XtParent(w)) == dshell3) ) {        
        
        if(anim3.anim_type==FILE_SERIES)        
            animate_prod_file(SCREEN_3, REVERSE);
        else
            animate_prod_database(SCREEN_3, REVERSE);

        return;
    }


    /*  act if button pressed on this screen, or if all four are true: */
    /*      this screen is open, screens are linked and  */
    /*      animation modes are the same. */
    if( (XtParent(XtParent(w)) == dshell1) ||
        ( (dshell1 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        if(anim1.anim_type==FILE_SERIES)
            animate_prod_file(SCREEN_1, REVERSE);
        else
            animate_prod_database(SCREEN_1, REVERSE);
        
    }

    if( (XtParent(XtParent(w)) == dshell2) ||
        ( (dshell2 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        if(anim2.anim_type==FILE_SERIES)
            animate_prod_file(SCREEN_2, REVERSE);
        else
            animate_prod_database(SCREEN_2, REVERSE);
    }



}





/* continuously displays the previous set of images in the chosen series */
void back_play_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    /* these are static so that we can pass them as parameters */
    static int screen_num_1, screen_num_2, screen_num_3;

    /*  the auxiliary screen (screen3) is not linked */
    if( (XtParent(XtParent(w)) == dshell3) ) {           
        
        anim3.stop_anim = FALSE;
        screen_num_3 = SCREEN_3;
        back_play_cont((XtPointer)(&screen_num_3), NULL);
        return;
    }

    /*  act if button pressed on this screen, or if all four are true: */
    /*      this screen is open, screens are linked,  */
    /*      animation modes are the same. */
    if( (XtParent(XtParent(w)) == dshell1) ||
        ( (dshell1 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) {
        
        anim1.stop_anim = FALSE;
        screen_num_1 = SCREEN_1;
        back_play_cont((XtPointer)(&screen_num_1), NULL);
    }

    if( (XtParent(XtParent(w)) == dshell2) ||
        ( (dshell2 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        anim2.stop_anim = FALSE;
        screen_num_2 = SCREEN_2;
        back_play_cont((XtPointer)(&screen_num_2), NULL);
    }


}





/* effects continuous backwards play by displaying the previous image,
 * then setting up the function to be called again at a certain point
 * in the future. 
 */
void back_play_cont(XtPointer client_data, XtIntervalId *id)
{
    int screen_num = *((int *)client_data);
    int delay;   /* in seconds - on top of the normal rendering delay */
    int auto_ud_delay; /* can increase delay in auto update mode */
    int stop_flag=FALSE;
    int anim_type=1;



    if (screen_num==1) {
        stop_flag = anim1.stop_anim;
        anim_type = anim1.anim_type;
    }
    if (screen_num==2) {
        stop_flag = anim2.stop_anim;
        anim_type = anim2.anim_type;        
    }
    if (screen_num==3) {
        stop_flag = anim3.stop_anim;
        anim_type = anim3.anim_type;        
    }


    /* a slider on the main window is used to control the length of the delay */

/* for now we disable delay slider by overriding the time */
    delay=1;


    if (anim_type==AUTO_UPDATE)
        auto_ud_delay = 5000;
    else
        auto_ud_delay = 1;


    if(stop_flag != TRUE) {

        if(anim_type==FILE_SERIES)
            animate_prod_file(screen_num, REVERSE);
        else
            animate_prod_database(screen_num, REVERSE);
        
        XtAppAddTimeOut(app, MIN_PAUSE_TIME + delay*1000 + auto_ud_delay, 
            back_play_cont, client_data);
    }
}




/*************************************************************************/


/* when the stop button is clicked, we simply set a flag, and let the 
 * delayed function call handle stopping at that point 
 */
void stop_anim_callback(Widget w,XtPointer client_data, XtPointer call_data)
{


    if(XtParent(XtParent(w)) == dshell1) {
        anim1.stop_anim = TRUE;
        if(linked_flag==TRUE)
             anim2.stop_anim = TRUE;
    }
    
        

    if(XtParent(XtParent(w)) == dshell2) {
        anim2.stop_anim = TRUE;
        if(linked_flag==TRUE)
             anim1.stop_anim = TRUE;
    }


    if(XtParent(XtParent(w)) == dshell3) {
        anim3.stop_anim = TRUE;
        
    }    

}



/*************************************************************************/


/* same as above, just fowards */
void fwd_one_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

    /*  the auxialiary screen (screen3) is not linked */
    if( (XtParent(XtParent(w)) == dshell3) ) { 
        
        if(anim3.anim_type==FILE_SERIES)
            animate_prod_file(SCREEN_3, FORWARD);
        else
            animate_prod_database(SCREEN_3, FORWARD);
        
        return;
    }


    /*  act if button pressed on this screen, or if all four are true: */
    /*      this screen is open, screens are linked,  */
    /*      animation modes are the same, and not file series. */
    if( (XtParent(XtParent(w)) == dshell1) ||
        ( (dshell1 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        if(anim1.anim_type==FILE_SERIES)
            animate_prod_file(SCREEN_1, FORWARD);
        else
            animate_prod_database(SCREEN_1, FORWARD);
            
    }

    if( (XtParent(XtParent(w)) == dshell2) ||
        ( (dshell2 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        if(anim2.anim_type==FILE_SERIES)
            animate_prod_file(SCREEN_2, FORWARD);
        else
            animate_prod_database(SCREEN_2, FORWARD);
        
    }


}





/* same as above, just fowards */
void fwd_play_callback(Widget w,XtPointer client_data, XtPointer call_data)
{
    static int screen_num_1, screen_num_2, screen_num_3;
    
    /*  the auliliary screen (screen3) is not linked */
    if( (XtParent(XtParent(w)) == dshell3) ) { 
        
        anim3.stop_anim = FALSE;
        screen_num_3 = SCREEN_3;
        fwd_play_cont((XtPointer)(&screen_num_3), NULL);
        return;
    }


    /*  act if button pressed on this screen, or if all four are true: */
    /*      this screen is open, screens are linked and  */
    /*      animation modes are the same. */
    if( (XtParent(XtParent(w)) == dshell1) ||
        ( (dshell1 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        anim1.stop_anim = FALSE;
        screen_num_1 = SCREEN_1;
        fwd_play_cont((XtPointer)(&screen_num_1), NULL);
    } 

    if( (XtParent(XtParent(w)) == dshell2) ||
        ( (dshell2 != NULL) && (linked_flag==TRUE) && 
          (anim1.anim_type==anim2.anim_type) ) ) { 
        
        anim2.stop_anim = FALSE;
        screen_num_2 = SCREEN_2;
        fwd_play_cont((XtPointer)(&screen_num_2), NULL);
    }


}





/* same as above, just fowards */
void fwd_play_cont(XtPointer client_data, XtIntervalId *id)
{
    int screen_num = *((int *)client_data);
    int delay;   /* in seconds */
    int auto_ud_delay; /* can increase delay in auto update mode */
    int stop_flag=FALSE;
    int anim_type=1;


    if (screen_num==1) {
        stop_flag = anim1.stop_anim;
        anim_type = anim1.anim_type;
    }
    if (screen_num==2) {
        stop_flag = anim2.stop_anim;
        anim_type = anim2.anim_type;
    }
    if (screen_num==3) {
        stop_flag = anim3.stop_anim;
        anim_type = anim3.anim_type;
    }


/* NEED TO HANDLE THE DELAY SLIDERS SEPAARATELY */
/*     XmScaleGetValue(delay_slider, &delay); */

/* for now we disable delay slider by overriding the time */

    delay=1;


    if (anim_type==AUTO_UPDATE)
        auto_ud_delay = 5000;
    else 
        auto_ud_delay = 1;

    if(stop_flag != TRUE) {

        if(anim_type==FILE_SERIES)
            animate_prod_file(screen_num, FORWARD);
        else
            animate_prod_database(screen_num, FORWARD);

        XtAppAddTimeOut(app, MIN_PAUSE_TIME + delay*1000 + auto_ud_delay, 
            fwd_play_cont, client_data);
    }
}



/* support future animation of single product binary files
 * for now simply sets the animation stop flag
 *
 * Until this capability is provided, simply stop.
 */
void animate_prod_file(int screen_num, int direction)
{
    
    /*  Linking screens will not be applied to future file animation, */
    /*  so (linked_flag==TRUE) not a test! */
    if ((screen_num==1) ) {
        anim1.stop_anim = TRUE;
    }
    if ((screen_num==2) ) {
        anim2.stop_anim = TRUE;
    }
    if ((screen_num==3) ) {
        anim3.stop_anim = TRUE;
    }


/*  FUTURE CHANGE: when file series is implemented, reset all database  */
/*                 modes for applicable screen: */
    /* reset_elev_series(screen_num, ANIM_FULL_INIT); */
    /* reset_time_series(screen_num, ANIM_FULL_INIT); */
    /* reset_auto_update(screen_num, ANIM_FULL_INIT); */

}




/*  CVG 6.5 NEW LOGIC */
 /* Uses the volume number and volume time of the base product.
  * Aborts if no product displayed on the screen
  * Initialiazes time series and elevation series animation and
  *      Re reads the history arrays if needed
  * Uses the appropriate saved history arrays for product id and elevation
  * Finds the next product(s) to display based upon the animation mode
  */
 /* Uses the new sorted product list to find the next product.  As
  * currently written, animation is completely reset if the product
  * list is updated.  This requires reinitialization of the time
  * series animation.  POSSIBLE IMPROVEMENT: instead of resetting
  * animation when the product list is updated, time series could be 
  * automatically reinitialized to the same parameters withOUT  re
  * reading the history
  */
  
/*  CVG 7.4 CHANGES */
 /* 1. provides an option to sort database list using internal sequence
  *    number rather than volome date/time.  In this case animation also
  *    uses the sequence number instead of the date/time.  
  *
  * 2. when matching elevations in products for animation, CVG uses the
  *    elevation number in the product, which is 0 for all volume products.
  *    This prevents volume products from dropping out of time series 
  *    animation when the VCP changes.
  */
void animate_prod_database(int screen_num, int direction)
{
    int buf_num,k;
    short elev_num;
    unsigned int vol_num;
    unsigned int vol_time_sec, vol_times;
    int prod_index = -1;
    Prod_header *hdr;
    Graphic_product *gp;
    char filename[128];

    int listSize=maxProducts+1;
    LB_info list[listSize];
    int num_products;
    int lbfd=-1, len;

    int anim_hist_size = 0;
    int continue_ts, continue_es, continue_au;
    int reset_ts_flag;
        
    short *prod_ids = NULL, *elev_nums = NULL;
    short vol_nums;
    int anim_type;

/*  new logic */
anim_data *ad=NULL;



/***DEBUG***/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "\n***************************************************\n");
fprintf(stderr, "DATABASE ANIMATION - entering animate_prod_database\n");
}


/*  new logic */
    if(screen_num == SCREEN_1) {
        ad = &anim1;
        sd = sd1;
    } else if(screen_num == SCREEN_2) {
        ad = &anim2;
        sd = sd2;
    } else if(screen_num == SCREEN_3) {
        ad = &anim3;
        sd = sd3;
    } else {
        return;
    }

    anim_type = ad->anim_type;
    reset_ts_flag = ad->reset_ts_flag;


/*  FUTURE CHANGE: when implemented, reset file series mode for applicable screen: */
    /* reset_file_series(screen_num, ANIM_FULL_INIT); */

    if(sd->history==NULL) {
        fprintf(stderr,
              "*DATABASE ANIMATION: Screen %d empty, nothing to display.*\n",
                 screen_num);
        fprintf(stderr,
              "*        Display a product before attempting animation.*\n");

/*  new logic: */
        ad->stop_anim = TRUE;
        
        return; 
    }

    /* if the product was not loaded from a linear buffer, don't */
    if(sd->lb_filename == NULL) {
        fprintf(stderr,
             " Cannot animate product loaded from individual disk file\n.");
        return;
    }
   
    if(verbose_flag)
        fprintf(stderr,"Linear Buffer: %s\n", sd->lb_filename);
 
        

    /* open the linear buffer from which the product was loaded */
    lbfd = LB_open(sd->lb_filename, LB_READ, NULL);
    if(lbfd < 0) {
        fprintf(stderr,"ERROR opening the following linear buffer: %s\n", 
                filename);
        return;
    }    
        
    /* get the product inventory */
    /* I believe we need the inventory list to use the msg_num */
    /* to read a message from the database                     */
     
    num_products = LB_list(lbfd, list, maxProducts+1);



    /*  CVG 6.5 */
    /* read just the current history volume number and volume date-time */
    /* the other history components: product ids and elevation numbers */
    /* at each history level are read during initialization only       */
    /* this permits skipping overlay products that may not be present  */
    hdr = (Prod_header *)(sd->history[0].icd_product);
    gp  = (Graphic_product *)(sd->history[0].icd_product + 96);
    vol_nums = hdr->g.vol_num;
            
    vol_time_sec = ( ((int)(gp->vol_time_ms) <<16) |
                             ((int)(gp->vol_time_ls) & 0xffff) );                                               
    vol_times = (unsigned int) _88D_unix_time( (unsigned short)gp->vol_date, 
                                               (vol_time_sec * 1000) );  
       
    /* volume time note: the easiest volume time to use would be the volume time 
     * within the internal 96 byte header.  We do not use this volume time because 
     * the TDWR products modify the volume in the product in order to provide a 
     * reference for animating the repeated base scans and for dividing the TDWR 
     * scanning strategy into two WSR-88D like volumes.
     * Using the product volume time does not work in two cases:
     *        1. intermediate products (which we currently do not list) 
     *        2. the RCS (84) and VCS (83) products which have incorrect product 
     *           volume times on Linux (a bug)
     */              
    /*   if not an icd product then the following could be used for volume time */
    /* temp fix for ICD vol_time bug on Linux for RCS(84) and VCS(83) */
    /* use the internal header volume time */
    /*  SHOULD BE FIXED IN BUILD 8! */
    if( (hdr->g.prod_id==83) || (hdr->g.prod_id==84) ) {
        vol_times = (unsigned int)hdr->g.vol_t;                  
    }

    /*  CVG 7.4 */
    if(sort_method == 2)   
        vol_times = hdr->g.vol_num;  /*  the vol sequence number */

/*DEBUG*/ 
if(ANIM_DEBUG == TRUE) { 
fprintf(stderr, "DEBUG: Animation Type is %d (1-T,2-E,3-up) \n", anim_type);
/*  new logic: */
fprintf(stderr, "DEBUG: Animation Type S1 is %d (1-T,2-E,3-up) \n", anim1.anim_type);
fprintf(stderr, "DEBUG: Animation Type S2 is %d (1-T,2-E,3-up) \n", anim2.anim_type);
fprintf(stderr, "DEBUG: Animation Type S3 is %d (1-T,2-E,3-up) \n", anim3.anim_type);
}


    if(anim_type==TIME_SERIES) {
        reset_elev_series(screen_num, ANIM_CHANGE_MODE_INIT);
        reset_auto_update(screen_num, ANIM_CHANGE_MODE_INIT);
    }
    
    if(anim_type==ELEV_SERIES) {
        if(reset_ts_flag != ANIM_CHANGE_MODE_NEW_TS_INIT)
            reset_time_series(screen_num, ANIM_CHANGE_MODE_INIT);
        reset_auto_update(screen_num, ANIM_CHANGE_MODE_INIT);
    }

    if(anim_type==AUTO_UPDATE) {
        reset_elev_series(screen_num, ANIM_CHANGE_MODE_INIT);
        reset_time_series(screen_num, ANIM_CHANGE_MODE_NEW_TS_INIT);
    }
    
    
    
       
    
    if(anim_type==ELEV_SERIES) {

/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "ES_ONE - INITIAL STATE IS: \n");
fprintf(stderr, "Animation Type is %d (1-T,2-E,3-up) \n", anim_type);
/*  new logic: */
fprintf(stderr, "SCR 1 first & last index num %d & %d\n",
        anim1.es_first_index, anim1.es_last_index);
fprintf(stderr, "SCR 2 first & last index num %d & %d\n",
        anim2.es_first_index, anim2.es_last_index); 
fprintf(stderr, "SCR 3 first & last index num %d & %d\n",
        anim3.es_first_index, anim3.es_last_index); 
}
   
         continue_es = init_elev_series(screen_num, vol_times, vol_nums); 
         if(continue_es==FALSE) {
/*  new logic: */
            ad->stop_anim = TRUE; 
            LB_close(lbfd);
            return;
         }
    }


    if(anim_type==TIME_SERIES) {
    
/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
/*  new logic: */
fprintf(stderr, "TS_ONE - INITIAL STATE IS: ts reset flag 1 and 2: %d and %d, loop sizes %d %d\n", 
        anim1.reset_ts_flag, anim2.reset_ts_flag, anim1.loop_size, anim2.loop_size);
fprintf(stderr, "Animation Type is %d (1-T,2-E,3-up) \n", anim_type);
fprintf(stderr, "SCR 1 lower vol num %d, elevation num %d, first & last index num %d & %d\n",
        anim1.lower_vol_num, anim1.elev_nums[0], anim1.first_index_num, anim1.last_index_num);
fprintf(stderr, "SCR 2 lower vol num %d, elevation num %d, first & last index num %d & %d\n",
        anim2.lower_vol_num, anim2.elev_nums[0], anim2.first_index_num, anim2.last_index_num);  
fprintf(stderr, "SCR 3 lower vol num %d, elevation num %d, first & last index num %d & %d\n",
        anim3.lower_vol_num, anim3.elev_nums[0], anim3.first_index_num, anim3.last_index_num); 
}
    
         continue_ts = init_time_series(screen_num, vol_times, vol_nums); 
         if(continue_ts==FALSE) {
/*  new logic: */
              ad->stop_anim = TRUE; 
              LB_close(lbfd);
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"STOPPING ANIMATION, init_time_series returned FALSE\n");
}
              return;
         }
    }




    if (anim_type==AUTO_UPDATE) {

        continue_au = init_auto_update(screen_num);

    }


/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
/*  new logic: */
fprintf(stderr, "ANIMATE_ONE - CONTINUE STATE IS: ts reset flag 1 and 2: %d and %d, loop sizes %d %d\n", 
        anim1.reset_ts_flag, anim2.reset_ts_flag, anim1.loop_size, anim2.loop_size);
fprintf(stderr, "SCR 1 lower vol num %d, elevation num %d, first & last index num %d & %d\n",
        anim1.lower_vol_num, anim1.elev_nums[0], anim1.first_index_num, anim1.last_index_num);
fprintf(stderr, "SCR 2 lower vol num %d, elevation num %d, first & last index num %d & %d\n",
        anim2.lower_vol_num, anim2.elev_nums[0], anim2.first_index_num, anim2.last_index_num); 
fprintf(stderr, "SCR 3 lower vol num %d, elevation num %d, first & last index num %d & %d\n",
        anim3.lower_vol_num, anim3.elev_nums[0], anim3.first_index_num, anim3.last_index_num);     
}


/*  new logic: */
    anim_hist_size = ad->anim_hist_size;
    prod_ids = ad->prod_ids;
    elev_nums =  ad->elev_nums;  

    /*  vol_num retained because it is used in printf's */
    vol_num = vol_nums;    

     
    /*  THE LOOP */
       
    for(k=0; k<anim_hist_size; k++) {


/***DEBUG***/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "DATABASE ANIMATION - in for loop\n");
}
        
        buf_num = prod_ids[k];
        elev_num = elev_nums[k];

/***DEBUG***/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"vol num = %d  buf_num = %d\n", vol_num, buf_num);
}
        
        if(verbose_flag)
            fprintf(stderr,"vol num = %d  buf_num = %d\n", vol_num, buf_num);

         
         
        /*  CVG 6.5 */
        /*  AUTO_UPDATE - find_last_prod() searches the database and reads the  */
        /*                product into the anim_prod_data variable */
        /*  */
        /*  TIME_SERIES and ELEV_SERIES - the find product functions search the  */
        /*                sorted list and return the list index for the product      */
 
        anim_prod_data = NULL;
                 
        if (anim_type==AUTO_UPDATE) {
         
             find_last_prod(lbfd,num_products,list,buf_num,elev_num); 
                              
        } else if (anim_type==TIME_SERIES) {
            
            if(k==0) {               
                if(direction==FORWARD)
                    prod_index = ts_next_base_prod(buf_num, elev_num,
                                                   vol_times, screen_num);
                else if(direction==REVERSE)
                    prod_index = ts_prev_base_prod(buf_num, elev_num,
                                                   vol_times, screen_num);
                            
            } else { /*  not a base product */
                prod_index = ts_find_ovly_prod(buf_num, elev_num,
                                                              screen_num);
            }
         
        } else if (anim_type==ELEV_SERIES) {
            
            if(direction==FORWARD)
                prod_index = es_find_next_prod(buf_num, k, screen_num);
            else if(direction==REVERSE)
                prod_index = es_find_prev_prod(buf_num, k, screen_num);
            
        } else { /* oops! */
            LB_close(lbfd);
            return;
        }
        
        /* end find product message logic */ 
    
    
        /*  CVG 6.5 NEW LOGIC */
        /*  Using the index number retrurned, attempt to load the */
        /*  Product.  If expired, break out ot the animation with */
        /*  a message to update the product list. */
        /*  */

        /* ////////////////////////////////////////////////////// */
        /*  TIME_SERIES and ELEV_SERIES use the sorted list */
        if ( (anim_type==TIME_SERIES) || (anim_type==ELEV_SERIES) ) {

            /*  CVG 6.5 - currently only used by ELEV_SERIES animation */
/*  new logic: */
            ad->list_index[k] = prod_index;
        
            /*  this should not happen (a bad list), but in case it does */
            if( (prod_index == -1) && (k == 0) ) {
                fprintf(stderr,
                         "********** CVG PRODUCT ANIMATION ERROR ***********\n");
                fprintf(stderr,
                         "*DATABASE ANIMATION: base product not in sorted product list.*\n");
/* CVG 7.4 DEBUG */
/* fprintf(stderr,"DEBUG ts_next_base_prod() returned prod_index of %d, k is %d\n", prod_index, k); */
                LB_close(lbfd);
                return;                           
            } else if(prod_index == -1) { /*  missing overlay product, not necessarily an error */
                continue;
            }
            
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "DATABASE ANIMATION - reading product message %d from the database\n",
                                                       msg_num_total_list[prod_index+1]);
}         
/*             len = LB_read(lbfd, &anim_prod_data, LB_ALLOC_BUF, msg_num_total_list[prod_index+1]); */
            len = LB_read(lbfd, &anim_prod_data, LB_ALLOC_BUF, 
                              (list)[msg_num_total_list[prod_index+1] - 1].id);
            
/*  TEMP DEBUG */
/* fprintf(stderr,"DEBUG ANIMATE DATABASE - product message length is %d\n", len);    */

            if( (len <= 0) ) {
/*  CVG 8.5 */
                if(anim_prod_data!=NULL)
                    free(anim_prod_data);
                anim_prod_data = NULL;
            }
        
        }  /*  end if TIME_SERIES or ELEV_SERIES */
        /* /////////////////////////////////////////////////////// */


/***DEBUG***/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "DATABASE ANIMATION - finished looking for product\n");
}       


        /*  handle missing products */
        if(anim_prod_data == NULL) {           
            if(k == 0) { /*  can't find / read the base product */
                fprintf(stderr,
                     "********** CVG PRODUCT ANIMATION ERROR ***********\n");
                fprintf(stderr,
                     "*DATABASE ANIMATION: base product not in product database.*\n");
                LB_close(lbfd);
                return;
                
            /*  for TIME_SERIES and ELEV_SERIES, not finding an overlay this is an error  */
            /*  if(prod_index!=-1 and anim_prod_data==NULL)  */
            } else if( (anim_type==TIME_SERIES) || (anim_type==ELEV_SERIES) ) {
                if( (prod_index!=-1) && (anim_prod_data==NULL) ) {
                    fprintf(stderr,
                         "********** CVG PRODUCT ANIMATION ERROR ***********\n");
                    fprintf(stderr,
                         "*DATABASE ANIMATION: overlay product not in product database.*\n");
                    continue;
                }
            }
        } /*  end if anim_prod_data NULL */


        /* DISPLAY THE SELECTED PRODUCT */              
        display_animation_product(k,screen_num);

    } /* end for k<anim_hist_size */


    LB_close(lbfd);


} /* end animate_prod_database() */




/***************************************************************/
/* SECTION 2.     ANIMATION HELPER FUNCTIONS                   */
/***************************************************************/


/*************************************************/
int get_history(int screen)
{
   int hist_size, k; 
   Prod_header *hdr=NULL;
   Graphic_product *gp=NULL; 
    /* reads the current histories */

/*  new logic */
anim_data *ad=NULL;

    if(screen == SCREEN_1) 
        ad = &anim1;
    else if(screen == SCREEN_2)
        ad = &anim2;
    else if(screen == SCREEN_3)
        ad = &anim3;



    hist_size = sd->history_size;

    if(hist_size > MAX_HIST_SZ) {
        fprintf(stderr,"ERROR, History size exceeded maximum of %d\n", MAX_HIST_SZ);
        
        return -1;   
    }



    /*  CVG 6.5 We always read the volume number and volume time just before */
    /*  looking for the next product.  We also read them here so they are available */
    /*  for the ANIM_UPDATE_LIST_INIT mode of initialization when the history is not */
    /*  updated and the original base volume needs to be used to reestablish the */
    /*  indexes for the loop limits */

/*  new logic: */
    for(k=0;k<hist_size;k++) {

        hdr = (Prod_header *)(sd->history[k].icd_product);
        gp = (Graphic_product *)(sd->history[k].icd_product+96);
            
          
        ad->prod_ids[k] = hdr->g.prod_id;
/*  CVG 7.4 CHANGE  */
/*          Instead of using the internal elevation index, animation now uses the */
/*          elevation number in the products (0 for volume products) */
/*             elev_nums_s1[k] = get_elev_ind(sd->history[k].icd_product,orpg_build_i); */
        ad->elev_nums[k] = gp->elev_ind;
               
if(ANIM_DEBUG) {
    fprintf(stderr,"ANIM GET HISTORY - get history vol %d, prod %d, elev %d\n",
            hdr->g.vol_num, ad->prod_ids[k], ad->elev_nums[k]);
}
    } /*  end for */

    return hist_size;   
        
}




/**************************************************/
/* animation product display logic                */
void display_animation_product(hist_lyr, screen_num)
{
    short pcode;
    Graphic_product *gp;
    int prev_ovly_flag;
    int rv;
    
/*  CVG 8.5 */
    int msg_len;
    short param_8;

    /* ML FIX  */
    if((sd->icd_product != NULL)) {
        free(sd->icd_product);
        sd->icd_product = NULL;
    }   
/*  GENERIC_PROD */
    if((sd->generic_prod_data != NULL)) {
        rv = cvg_RPGP_product_free((void *)sd->generic_prod_data);
        sd->generic_prod_data = NULL;
    }    
    
    sd->icd_product = anim_prod_data;
    

    /* Since we only support Animation of Final Products (or pseudo    */
    /* final product created from CVG Raw structure), there is no need */
    /* to confirm valid ICD structure before testing for compression.  */

/*  CVG 8.5 */
    gp  = (Graphic_product *)(sd->icd_product + 96);
    msg_len = gp->msg_len;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    msg_len = INT_BSWAP(msg_len);
#endif 
/*  CVG 8.5 */
    /* if a product is empty (length == 120 for just MHB and PDB) do NOT */
    /* test for compression                                              */
        param_8=gp->param_8;
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
        param_8=SHORT_BSWAP(param_8);
#endif

    if(msg_len>120) { /*  test for compression, otherwize assume uncompressed */

        /* check to see if the body of the product is compressed */
/*  CVG 8.5 */
/*         if( ((sd->icd_product[100+96] != 0) || (sd->icd_product[101+96] != 0)) */
/*               && ((sd->icd_product[120+96]&0xff) != 0xff)  */
/*               && ((sd->icd_product[121+96]&0xff) != 0xff) ) */
        if( ( (param_8 == 1) || (param_8 == 2) ) &&
            ( ((sd->icd_product[96+120]&0xff) != 0xff) ||
              ((sd->icd_product[96+121]&0xff) != 0xff) ) ) 

            product_decompress(&(sd->icd_product));

    } /*  end not an empty product */

   prev_ovly_flag = overlay_flag; 

   if(hist_lyr==0)
     overlay_flag = FALSE;
     /*  causes history to be cleared (and screen cleared)  */
     /*  when plotting base image */
   else
     overlay_flag = TRUE;


   /* Usually we SELECT_ALL packets to be displayed for animation. 
    * However, one product requires special consideration.
    *     1. DPA can only have one layer selected at a time.   
    *     2. 
    */
   gp  = (Graphic_product *)(sd->icd_product + 96);
   pcode = (gp->prod_code);
   
#ifdef LITTLE_ENDIAN_MACHINE
   pcode=SHORT_BSWAP(pcode);
#endif   

    /*  CVG 8.0 Fixed overlay issues with 56 USP, 57 DHR, and 108 DPA */
    /*          DPA still requires special handling due to multiple  */
    /*          2D array packets */
   if(
/* cvg 8.0   pcode == 31 ||  */
/* cvg 8.0   pcode == 32 ||  */
       pcode == 81) {
       sd->packet_select_type = SELECT_LAYER;
       sd->layer_select = 0;
/* cvg 8.0 */
/*        fprintf(stderr, "****** CVG ANIMATION PROBLEM *****************\n"); */
/*        fprintf(stderr, "*   CVG cannot animate this product.         *\n"); */
/*        fprintf(stderr, "**********************************************\n"); */
/*        return; */
       
   } else {   
      sd->packet_select_type = SELECT_ALL;         

   }



    /* following normally called from post_load_setup */
    parse_packet_numbers(sd->icd_product);

/***DEBUG***/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "FWD ANIMATION - returned from parse_packet_numbers\n");
}


    /* following normally called from packet selection callbacks */
    plot_image(screen_num, TRUE);

/***DEBUG***/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, "FWD ANIMATION - returned from plot_image\n");
}


   overlay_flag = prev_ovly_flag;

    
} /* end display_animation_product */




/***************************************************************/
/* SECTION 3.     ANIMATION INITIALIZATION AND RESET           */
/***************************************************************/




/***************** ANIMATION MODE INITIALIZATION TYPES *********************/
/***************************************************************************/
/*  defined in global.h */
/* */ /* animation initialization type */
/* #define ANIM_NO_INIT 0   */
/* #define ANIM_FULL_INIT 1 // displaying product, clearing screen, and  */
/*                          // if ANIM_NO_INIT when setting time series */
/* #define ANIM_CHANGE_MODE_INIT 2 // set when changing animation mode */
/* #define ANIM_CHANGE_MODE_NEW_TS_INIT 3 // set if CVG user sets new new */
/*                          // base vol and loop after change mode, and */
/*                          // set when changing animation mode from AUTO_UPDATE */
/*                          // to TIME_SERIES  */
/* #define ANIM_UPDATE_LIST_INIT 4 // set when updating sorted product list */

   /*  CVG 6.5 */
   /* 1. FULL_INIT (set when new product manually displayed, screen cleared,
    *               or if NO_INIT when user resets time series animation.
    *     a. reset init flag
    *     b. reset all variables
    *     c. set base volume
    *     d. establish loop limits (time series)
    *     e. update animation history
    *
    * 2. CHANGE_MODE_INIT (set when switching animation modes)
    *     a. reset init flag
    *     b. reset all variables (ELEV_SERIES only)
    *     c. do NOT reset base volume 
    *     d. establish loop limits (ELEV_SERIES only)
    *     e. do NOT update animation history
    *
    * 3. ANIM_CHANGE_MODE_NEW_TS_INIT (set when user resets time series animation
    *                                  if not NO_INIT; and when changing out of
    *                                  AUTO_UPDATE mode)
    *     a. reset init flag
    *     b. reset all variables (ELEV_SERIES)
    *     c. reset base volume 
    *     d. establish loop limits (ELEV_SERIES)
    *     e. do NOT update animation history
    *
    * 4. UPDATE_LIST_INIT (set when updating the sorted product list)
    *     a. reset init flag
    *     b. reset all variables except loop size
    *     c. do NOT reset base volume but find new indexes
    *     d. establish loop limits (time series) using previous loop size
    *     e. do NOT update animation history
    */
          





/************************************************************************/ 
 /*  CVG 6.5 LOGIC DISCRIPTION */
 /* This function firsts checks to see if time series animation needs to
 * be initialized using the value of reset_ts_flag_s1 or reset_ts_flag_s2.
 *
 * If initialization has been requested via the time series set button
 * (value of user_reset_time_sN) initialization is accomplihsed.  This test
 * would be skipped for ANIM_UPDATE_LIST_INIT (not yet implemented). If not 
 * requested by the set button, a check is made to see if an initialiation 
 * has been requested by the application (time series reset).  If so, 
 * the user is prompted to reset the time series loop size via a dialog.
 *
 * FUTURE OPTION: Another option would be to always set the loop size to
 * the entire database whenever the application resets time series
 * animation. The test for user_reset_time_sN would be eliminated and
 * reset_time_series() would reset the loop size except for the future
 * implementation of ANIM_UPDATE_LIST_INIT.
 *
 * Initialization summary: The function cycles through the new sorted 
 * product list to determine the first index number (first_index_num_sN) 
 * containing a product from the lower volume limit (lower_vol_num_sN) 
 * and the last index number (last_index_num_sN) containing a product from 
 * the upper volume limit (upper_vol_num_sN).  In addition, the variables 
 * holding the first and last index number of the current volume are set.
 *
 * These index number variables are used to increase the efficiency
 * of time series animation in large product databases.
 *
 * ASSUMPTION: This logic relies on the correctness of the sorted product
 * list.  That is all product messages in a volume are contiguous 
 * (volumes not intermingled) and that the volume times are increasing.
 */
/***********************************************************************/
int init_time_series(int screen_num, unsigned int vol_t, short vol_n) 
{

int buf_num = 0; 
int elev_num = 0; 
int i; 

int my_loop_size=0;
int user_reset_time=0;
int my_reset_ts_flag=0;

    unsigned int my_lower_vol_time=0;
    short my_lower_vol_num=0;

   int continue_search;
   int  vol, ele, pid;  
   int ele_n;  /*  CVG 7.4 added  */
   char dtime[20];
   int current_prod_index = -1;
   int start_index = -1;
   unsigned int curr_time;
   int end_vol;

anim_data *ad=NULL;
   

if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"ENTER INIT TIME SERIES------------------\n");
}


   
    /*  CVG 6.5 test for empty product list */
    if(last_prod_list_size<=0) {    
        fprintf(stderr,"ANIMATION ABORTED.  DATABASE PRODUCT LIST IS EMPTY.\n");
        return FALSE;    
    }

/*  new logic: */
    if(screen_num == SCREEN_1) {
        ad = &anim1;
        user_reset_time = user_reset_time_s1;
    } else if(screen_num == SCREEN_2) {
        ad = &anim2;
        user_reset_time = user_reset_time_s2;
    } else if(screen_num == SCREEN_3) {
        ad = &anim3;
        user_reset_time = user_reset_time_s3;
    }
    my_reset_ts_flag = ad->reset_ts_flag;
    my_loop_size = ad->loop_size;


    /*  use previous loop if changing to and from ELEV_SERIES    */
    if(my_reset_ts_flag == ANIM_CHANGE_MODE_INIT) { 
        return TRUE;
    }



    if(user_reset_time == FALSE) {         
           /* User did not reset, check for application reset other than   */
           /* ANIM_UPDATE_LIST_INIT (ANIM_CHANGE_MODE_INIT checked above), */
           /* if no application reset, return TRUE                         */

/*  new logic */
        if(ad->reset_ts_flag==ANIM_FULL_INIT || ad->reset_ts_flag==ANIM_CHANGE_MODE_NEW_TS_INIT) {
            fprintf(stderr,
                "*TIME SERIES ANIMATION: Screen %d animation loop limits are not set.*\n",
                screen_num);
            open_time_opt_dialog(screen_num);
            return FALSE;   
        } else if(ad->reset_ts_flag==ANIM_NO_INIT)
            return TRUE;

    } /*  end if user did not reset... */
 

    /************************************************************************/
    /***********       need to re initialize time animation       ***********/ 

        
    /* Update all history information of the product being displayed except */
    /* base volume sequence number and time (which are always updated by */
    /* animate_prod_database(). */    

/*  new logic: */
    if(ad->reset_ts_flag==ANIM_FULL_INIT)
        ad->anim_hist_size = get_history(screen_num);
   

    /* RESET LOWER VOLUME NUMBER, THE ELEVATION INDEX, etc. */  

/*  new logic: */
    if(ad->reset_ts_flag != ANIM_UPDATE_LIST_INIT) {
        ad->lower_vol_num = vol_n;
        ad->lower_vol_time = vol_t;
    }
    my_lower_vol_num = ad->lower_vol_num;
    my_lower_vol_time = ad->lower_vol_time;
    elev_num = ad->elev_nums[0];
    buf_num = ad->prod_ids[0];
        

/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
/*  new logic: */
fprintf(stderr,"ANIMATE_TIME_SERIES- starting image has vol num %d, elev num %d, product id %d\n", 
        ad->lower_vol_num, ad->elev_nums[0], buf_num);
}        


    /* this should already be done */
/*  new logic: */
    ad->first_index_num = -1;


        /* FIND AND SET THE FIRST AND LAST MESSAGE NUMBER */
/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"INIT TIME SERIES - num products is %d, loop_size is %d\n", last_prod_list_size, my_loop_size);
}

    /*  CVG 6.5 my_loop_size == 0 : use whole list, this is */
    /*    recorded by setting either the upper and lower index */
    /*    to the first and last list index */

    /*  1. find base vol_time and set begin_vol_index */
    /*     if using not whole database, set lower index */
    /*  2. find base product; ensure it is still in list */
    /*  3. find not base vol_time and set end_vol_index */
    /*  4. if not using whole database, find and set upper index (loop size) */

    if(my_loop_size == 0) {
/*  new logic: */
        ad->first_index_num = 0;
        ad->last_index_num = last_prod_list_size - 1;
    }

    continue_search= FALSE; 
       
    for(i=0; i<last_prod_list_size; i++) {
        /*  1. find base vol_time and set begin_vol_index */
        /*  if using not whole database, set lower index */
        if(sort_time_total_list[i+1]==my_lower_vol_time) {
/*  new logic: */
            ad->ts_current_first_index = i;
            if(my_loop_size != 0)
                ad->first_index_num = i;            

if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT TS ts_current_first_index, i is %d\n", i); 
}
            start_index = i;
            continue_search=TRUE;
            break;
        } /*  end found begin vol */

    } /*  end for */

    if(continue_search==FALSE) {
        fprintf(stderr, "ANIMATION ABORTED, BASE VOLUME NOT FOUND\n");
        return FALSE;
    }
    continue_search = FALSE;   

/* DEBUG */
/* fprintf(stderr,"DEBUG INIT TS - LOOKING FOR BASE PRODUCT, id:%3d, elev:%2d, vol_num:%2d\n", */
/*                                                     buf_num, elev_num, my_lower_vol_num); */
                                                    
    for(i=start_index; i<last_prod_list_size; i++) {         
        /*  2. find base product; ensure it is still in the list */
        sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
/* DEBUG */
/* if(i < (start_index + 50)) */
/* fprintf(stderr,"DEBUG INIT TS - PRODUCTS BEING COMPARED:  id:%3d, elev:%2d, vol_num:%2d\n", */
/*                                                                            pid, ele_n, vol);                        */

/*  CVG 8.0 CHANGE */
        ele_n = elev_num_total_list[i+1];                      
/*         if( ( vol == my_lower_vol_num ) && ( ele == elev_num ) && ( pid == buf_num ) ) { */
        if( ( vol == my_lower_vol_num ) && ( ele_n == elev_num ) && ( pid == buf_num ) ) {
        /*  note: could use vol_num from history rather than funct parameter  */
            current_prod_index = i;
if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT TS found_lower_base_prod i is %d\n", i);
fprintf(stderr,"DEBUG - INIT TS prod id: %d, vol num: %d, elev num: %d\n",
                                                              pid, vol, ele);
fprintf(stderr,"DEBUG - INIT TS message num: %d, vol time: %d\n",
                        msg_num_total_list[i+1]  , sort_time_total_list[i+1]);
}
            continue_search=TRUE;
            break;
        } /*  end found base product */
        
    } /*  end for */
    
    if(continue_search == FALSE) {
        fprintf(stderr, "ANIMATION ABORTED, BASE PRODUCT NOT IN LIST\n");
                /* this should already be done */
/*  new logic: */
        ad->first_index_num = -1;

        return FALSE;
    }
    continue_search= FALSE;           

        
    for(i=current_prod_index; i<last_prod_list_size; i++) { 
    /*  3. find not base vol_time (or end of list) and set end_vol_index */
        if(sort_time_total_list[i+1] > my_lower_vol_time) { 
/*  new logic: */
            ad->ts_current_last_index = i-1;

            start_index = i - 1;  /*  so next loop works */
            continue_search=TRUE;
if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT TS ts_current_last_index, i is %d\n", i-1); 
}            
            break;
        } /*  found end vol before end of list */
        
        if(i == last_prod_list_size-1) { /*  at end of list */
/*  new logic: */
            ad->ts_current_last_index = i;

if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT TS lower volume completes the list, i is %d\n", i); 
} 
            break;
        } /*  reached end of list */
        
    } /*  end for */

    if(continue_search == FALSE) {    
    /*  current volume also ends the list */
        if(my_loop_size != 0) {
        /*  not using whole database, upper index already found */
/*  new logic: */
            ad->last_index_num = ad->ts_current_last_index;          
        }
                 
/*  new logic: */
        ad->reset_ts_flag = ANIM_NO_INIT; 
        if(screen_num == SCREEN_1) 
            user_reset_time_s1 = FALSE;
        else if(screen_num == SCREEN_2) 
            user_reset_time_s2 = FALSE;
        else if(screen_num == SCREEN_3) 
            user_reset_time_s3 = FALSE;

        return TRUE;
    } /*  end if current volume ended the loop */

 
    /*  lower volume did not end the product list */
    if(my_loop_size != 0) { /*  looking for end of loop size */
    /*  4. if not using whole database, find and set upper index (loop size)      */
        end_vol = 0;
        curr_time = my_lower_vol_time;
        for(i=start_index; i<last_prod_list_size; i++) {           
            if(sort_time_total_list[i+1] > curr_time) {
                end_vol = end_vol + 1;
                curr_time = sort_time_total_list[i+1];
                if(end_vol == my_loop_size) {
/*  new logic: */
                    ad->last_index_num = i-1;

if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"DEBUG - INIT TS found_last_index, i is %d\n", i-1);                         
}           
                    break;
                }            
            } /*  found loop size */
            
            if(i == last_prod_list_size-1) { /*  at end of list */
/*  new logic: */
                ad->last_index_num = i;
                

if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"DEBUG - INIT TS loop ends prodect list, i is %d\n", i);                         
}                 
            } /*  reached end of list */
                  
        } /*  end for */

    } /*  end looking for end of loop size */

/*  new logic: */
    ad->reset_ts_flag = ANIM_NO_INIT;
    if(screen_num == SCREEN_1) 
        user_reset_time_s1 = FALSE;
    else if(screen_num == SCREEN_2) 
        user_reset_time_s2 = FALSE;
    else if(screen_num == SCREEN_3) 
        user_reset_time_s3 = FALSE;

    
    return TRUE;
        
}  /* end init_time_series() */




/************************************************************************/
/* If the fisrt message and last message of the elevation animation are
 * not valid (i.e., reset by the application) the elevation animation must 
 * be re initialized
 *
 * Then the sorted database product list is searched to determine the first 
 * and last message of the volume containing the base product displayed for 
 * elevation animation.  FALSE is returned if either the base volume or the
 * the base product cannot be found in the list.
 *
 * Then the current history list is used to update the product list_index 
 * portion of the history.
 *
 * ASSUMPTION: This logic assumes that products from the same volume are in
 * contiguous message in the database, that is volumes are not intermingled.
 */

/*******************************************/
int init_elev_series(int screen_num, unsigned int vol_t, short vol_n) {


int buf_num = -1, elev_num = -1, i; 


   int continue_search;
   int  vol, ele, pid;
   int ele_n;  /*  CVG 7.4 added */
   char dtime[20];
   int current_prod_index = -1;
   int start_index = -1;

   /*  update list_index and elev_num arrays */
   int my_hist_size = 0, k;
   int my_first_index = -1, my_last_index = -1;
   short *my_elev_nums = NULL, *my_buf_nums = NULL;

anim_data *ad=NULL;

/*  new logic: */
    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    if(ad->es_first_index != -1) {
/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"SCR %d: ELEV ANIMATION LIMITS ALREADY SET, first message is %d\n",
           screen_num, ad->es_first_index);
}
        return TRUE;
    }
    if(ad->reset_es_flag==ANIM_FULL_INIT)
        ad->anim_hist_size = get_history(screen_num);


/*  new logic: */
    elev_num = ad->elev_nums[0];
    buf_num = ad->prod_ids[0]; 
    my_hist_size = ad->anim_hist_size;
    my_buf_nums = ad->prod_ids;
    my_elev_nums = ad->elev_nums;


/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"ANIMATE_ELEV_SERIES- starting image has vol num %d, elev num %d, product id %d\n", 
        vol_n, elev_num, buf_num);
}

    /*  CVG 6.5 setting elev_nums[k] and the new list_index[k] here and */
    /*          whenever a product is displayed simplifies finding */
    /*          the next elevation animation product.  Required because */
    /*          we only update the history during full initialization of */
    /*          time-series and elevation-series animation */
    
    /*  1. find base vol_time and set first_index_sN */
    /*  2. find base product; ensure it is still in list */
    /*  3. find not base vol_time and set last_index_sN */
    /*  4. loop thru history and set list_index[k] */

    continue_search= FALSE; 

       
    for(i=0; i<last_prod_list_size; i++) {
        /*  1. find base vol_time and set first_index_sN */
        if(sort_time_total_list[i+1]==vol_t) {
/*  new logic: */
            ad->es_first_index = i;

            my_first_index = i;
if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT ES es_first_index, i is %d\n", i); 
}
            start_index = i;
            continue_search=TRUE;
            break;
        } /*  end found begin vol */

    } /*  end for */

    if(continue_search==FALSE) {
        fprintf(stderr, "ANIMATION ABORTED, BASE VOLUME NOT FOUND\n");
        return FALSE;
    }
    continue_search = FALSE;   


    for(i=start_index; i<last_prod_list_size; i++) {         
        /*  2. find base product; ensure it is still in list */
        sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
/*  CVG 7.4 CHANGE */
        ele_n = elev_num_total_list[i+1];
/*         if( ( vol == vol_n ) && ( ele == elev_num ) && ( pid == buf_num ) ) { */
        if( ( vol == vol_n ) && ( ele_n == elev_num ) && ( pid == buf_num ) ) {
        /*  note: could use vol_num from history rather than funct parameter  */
            current_prod_index = i;
/*  new logic: */
            ad->list_index[0] = i;

if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT ES found base_prod i is %d\n", i);
fprintf(stderr,"DEBUG - INIT ES prod id: %d, vol num: %d, elev num: %d\n",
                                                              pid, vol, ele);
fprintf(stderr,"DEBUG - INIT ES message num: %d, vol time: %d\n",
                        msg_num_total_list[i+1]  , sort_time_total_list[i+1]);
}
            continue_search=TRUE;
            break;
        } /*  end found base product */
        
    } /*  end for */
    
    if(continue_search == FALSE) {
        fprintf(stderr, "ANIMATION ABORTED, BASE PRODUCT NOT IN LIST\n");
/*  new logic: */
            ad->es_first_index = -1;

        return FALSE;
    }
    continue_search= FALSE;           


    for(i=current_prod_index; i<last_prod_list_size; i++) { 
    /*  3. find not base vol_time and set last_index_sN */
        if(sort_time_total_list[i+1] > vol_t) { 
/*  new logic: */
            ad->es_last_index = i-1;

            my_last_index = i-1;
if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT ES es_last_index, i is %d\n", i-1); 
}            
            break;
        } /*  found end vol before end of list */
        
        if(i == last_prod_list_size-1) { /*  at end of list */
/*  new logic: */
            ad->es_last_index = i;

if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT ES base volume completes the list, i is %d\n", i); 
} 
            break;
        } /*  reached end of list */
        
    } /*  end for */


    /*  4. loop thru history and set list_index[k] */
    for(k=0; k<my_hist_size; k++) {
if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT ES setting up the list_index[%d] array entry\n", k); 
} 
        for(i=my_first_index; i<=my_last_index; i++) {

            sscanf(product_list[i],"%13c%d%d%d",
                          dtime, &vol, &ele, &pid);            
/*  CVG 7.4 CHANGE */
            ele_n = elev_num_total_list[i+1];
/*             if( ( ele == my_elev_nums[k] ) && ( pid == my_buf_nums[k] ) ) { */
            if( ( ele_n == my_elev_nums[k] ) && ( pid == my_buf_nums[k] ) ) {
            /*  note: could use vol_num from history rather than funct parameter  */
/*  new logic: */
                ad->list_index[k] = i;

if(ANIM_DEBUG == TRUE) {                        
fprintf(stderr,"DEBUG - INIT ES to the index value %d\n", i); 
}                    
            }            
            
        } /*  for my_last_index         */
                
    } /*  end for my_hist_size */



/*DEBUG*/
if(ANIM_DEBUG == TRUE) {
/*  new logic: */
    fprintf(stderr,"SCR %d ANIMATE_ELEV_SERIES- first message is %d, last message is %d\n", 
            screen_num, ad->es_first_index, ad->es_last_index);     
}


/*  new logic: */
    ad->reset_es_flag = ANIM_NO_INIT; 

    return TRUE;

}  /* end init_elev_series() */










/*  new for CVG 6.5 */
int init_auto_update(int screen_num) {

anim_data *ad=NULL;

/*  new logic: */
    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    if(ad->reset_au_flag==ANIM_FULL_INIT)
        ad->anim_hist_size = get_history(screen_num);        

    ad->reset_au_flag = ANIM_NO_INIT;
       

    return TRUE;
    
}  /* end init_auto_update */




/* RESET ANIMATION MODES */

/*******************************************/
/* reset TIME_SERIES animation base volume information */
void reset_time_series(int screen_num, int type_init) {

/*  new logic: */
anim_data *ad=NULL;

/*  DO WE EVER REALLY NEED TO RESET lower_vol_num_sN and lower_vol_time_sN? */
/*  Either they are overrwritten in init_time_series() or used if UPDATE_LIST_INIT */
    
    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;
        
    ad->reset_ts_flag = type_init;  
    
    if(type_init==ANIM_CHANGE_MODE_INIT)
            return;
    
    if(type_init==ANIM_FULL_INIT || type_init==ANIM_CHANGE_MODE_NEW_TS_INIT) {
        ad->lower_vol_num = -1;
        ad->lower_vol_time = -1;            
        ad->upper_vol_time = -1;
    }
    ad->first_index_num = -1;
    ad->last_index_num = -1;

/* DEBUG */
/* fprintf(stderr,"DEBUG reset_time_series, lower vol = %d, first_index = %d\n",  */
/*                     ad->lower_vol_num, ad->first_index_num); */

    

}




/* reset ELEV_SERIES animation base volume information */
void reset_elev_series(int screen_num, int type_init) {

/*  new logic */
anim_data *ad=NULL;

    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    ad->reset_es_flag = type_init;
    if(type_init==ANIM_FULL_INIT || type_init==ANIM_CHANGE_MODE_INIT) {
        ad->es_first_index = -1;
        ad->es_last_index = -1;
    }

   
}




/*  new for CVG 6.5 */
void reset_auto_update(int screen_num, int type_init) {

/*  new logic: */
    if(screen_num == SCREEN_1) {
        anim1.reset_au_flag = type_init;
    } else if(screen_num == SCREEN_2) {
        anim2.reset_au_flag = type_init;
    } else if(screen_num == SCREEN_3) {
        anim3.reset_au_flag = type_init;
    }

    
}




/***************************************************************/
/* SECTION 4.     ANIMATION selection callbacks                */
/***************************************************************/



void select_time_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

/*  new logic: */
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        anim1.anim_type = TIME_SERIES;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        anim2.anim_type = TIME_SERIES;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        anim3.anim_type = TIME_SERIES;
    }

    
}



void select_elev_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

/*  new logic: */
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        anim1.anim_type = ELEV_SERIES;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        anim2.anim_type = ELEV_SERIES;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        anim3.anim_type = ELEV_SERIES;
    }


}



void select_update_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

/*  new logic: */
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        anim1.anim_type = AUTO_UPDATE;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        anim2.anim_type = AUTO_UPDATE;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        anim3.anim_type = AUTO_UPDATE;
    }


}



void select_file_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

/*  new logic: */
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        anim1.anim_type = FILE_SERIES;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        anim2.anim_type = FILE_SERIES;
    }
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        anim3.anim_type = FILE_SERIES;
    }


}



