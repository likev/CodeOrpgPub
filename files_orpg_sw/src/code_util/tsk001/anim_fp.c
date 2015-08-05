


/* anim_fp.c */

#include "anim_fp.h"
#include "byteswap.h"

 
 

/**********************************************************************/
/*  CVG 6.5 LOGIC */
/*This function finds the last product in the database by cycling through
 * the database recording the location largest volume time found for
 * that product.
 *
 * Only a fixed number of bytes is read during the search to improve
 * performance.  
 *
 * The whole database must be searched because after
 * running for a while, no assumptions can be made about the order
 * of data and the volumes get increasingly interlaced.
 */

void find_last_prod(int lbfd,int num_products,LB_info *list,int buf_num,
                    int elev_num) {

int i, len = 0; 

char fl_lb_data[216]; 

Prod_header *hdr;
Graphic_product *gp;

short elev;
unsigned int vol_time_sec, vol_time, curr_time;
short prod_id;
int curr_msg_num = 0;
/*  CVG 7.4 */
unsigned short vol_t_ms, vol_t_ls, vol_date;


    vol_time = curr_time = 0;
   
    /* find the location of the most current version of this product */
    for(i=1; i<num_products; i++) {
        /* read in product */
        len = LB_read(lbfd, fl_lb_data, 216, (list)[i].id);
        hdr = (Prod_header *)(fl_lb_data);
        gp = (Graphic_product *)(fl_lb_data + 96);
          
        /* the following test allows us to modify the logic to
           only read the first 96 bytes in order to improve performance */
        /* if not the simple test of len <=0 would suffice */

        if( (len == LB_EXPIRED) || 
            ( (len <= 0) && (len != LB_BUF_TOO_SMALL) ) ) { 
            continue;       
        }

        vol_t_ms = (unsigned short)gp->vol_time_ms;
        vol_t_ls = (unsigned short)gp->vol_time_ls;
        vol_date = (unsigned short)gp->vol_date;
#ifdef LITTLE_ENDIAN_MACHINE
        vol_t_ms = SHORT_BSWAP(vol_t_ms);
        vol_t_ls = SHORT_BSWAP(vol_t_ls);
        vol_date = SHORT_BSWAP(vol_date);
#endif

        vol_time_sec = ( ((int)(vol_t_ms) <<16) |
                                 ((int)(vol_t_ls) & 0xffff) );                                               
        vol_time = (unsigned int) _88D_unix_time( (unsigned short)vol_date,
                                               (vol_time_sec * 1000) );  
        /* volume time note: the easiest volume time to use would be the volume time 
         * within the internal 96 byte header. We do not use this volume time because 
         * the TDWR products modify the volume in the product in order to provide a 
         * reference for animating the repeated base scans and for dividing the TDWR 
         * scanning strategy into two WSR-88D like volumes.
         * Using the product volume time does not work in two cases:
         *        1. intermediate products (which we currently do not list) 
         *        2. the RCS (84) and VCS (83) products which have incorrect product 
         *           volume times on Linux (a bug)
         */        /* temp fix for ICD vol_time bug on Linux for RCS(84) and VCS(83) */
        /* use the internal header volume time */
        /*  SHOULD BE FIXED IN BUILD 8! */
        if( (hdr->g.prod_id==83) || (hdr->g.prod_id==84) ) {
            vol_time = (unsigned int)hdr->g.vol_t;                  
        }
        
    /*  CVG 7.4 */
    if(sort_method == 2)   
        vol_time = hdr->g.vol_num;  /*  the vol sequence number */
 
/*  CVG 7.4         */
/*         elev = get_elev_ind( (char *)hdr, orpg_build_i); */
        elev = gp->elev_ind;
#ifdef LITTLE_ENDIAN_MACHINE
	     elev = SHORT_BSWAP(elev);
#endif        
        prod_id = hdr->g.prod_id;

/* TEMP TEST */
/* fprintf(stderr,"DEBUG FIND LAST - message is %d --volume is  %d ------\n", i+1, vol_time); */
/* fprintf(stderr,"prod_id is %d, looking for %d; elev num is %d, looking for %d\n", */
/*                                                prod_id, buf_num, elev, elev_num); */


        if( (prod_id == buf_num) && (elev == elev_num) && 
                                    (vol_time > curr_time) ) {
            curr_time = vol_time;
            curr_msg_num = i;                        
        }
        

    } /*  end for */

    if(curr_msg_num > 0) {
        len = LB_read(lbfd, &anim_prod_data, LB_ALLOC_BUF, (list)[curr_msg_num].id);
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, " AUTO UPDATE - last product is at %d\n", curr_msg_num+1);
}                
    } else {
if(ANIM_DEBUG == TRUE) {
fprintf(stderr, " AUTO UPDATE - product no longer in the database\n");
}                
    }

    if( (len <= 0) ) {  /* ALMOST IMPOSSIBLE */
/*  CVG 8.5 */
        if(anim_prod_data!=NULL)
            free(anim_prod_data);
        anim_prod_data = NULL;
    }     

        
} /* end find last prod */





/**********************************************************************/
/*                    FUNCTIONS FOR TIMES SERIES                      */
/**********************************************************************/

/**********************************************************************/
/* CVG 6.5 LOGIC DESCRIPTION
 * This function finds the next base product to display for a time series
 * animation.  It uses the index number of the of the first and last 
 * messages of volume containing the currently displayed product, and the 
 * index numbers of the first and last messages encompasing the loop.  
 *
 * Unlike previous version of CVG, each successive volume is searched 
 * until the product is found. The list index of the next product is
 * returned.  Previous versions of CVG returned the product message in 
 * anim_prod_data and left anim_prod_data == NULL if a product was not found in the 
 * next volume.
 *
 * No test for product expiration is performed (caller must check this). 
 * The logic now also handles complete volumes missing from the list 
 * (vol time is greater - rather than vol num one greater).
 *
 * LINKED SCREENS NOTE: Since volumes not containing the product are 
 * skipped, if both screens are animated in linked mode they will get
 * out of sync if a product is missing on one screen and not the other.
 */
int ts_next_base_prod(int buf_num, int elev_num, unsigned int vol_time, 
                                                          int screen_num) {
                                                            
int first_message = 0, last_message = 0, i; 

/*  CVG 6.5 */
    int prod_idx = -1;
    int new_curr_first = -1, new_curr_last = -1;
    unsigned int curr_time = 0;
    int new_vol_num = 0;
    int  vol, ele, pid;  
    int ele_n;  /* CVG 7.4 added */
    char dtime[20];
    int current_prod_index = -1;
    
Boolean found_next_vol_begin = FALSE, found_next_vol_end = FALSE;
Boolean found_next_prod = FALSE;

/* volume limits of currently displayed product */ 
int current_first_index = 0, current_last_index = 0; 

/*  new logic */
anim_data *ad=NULL;

    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;
        
    first_message = ad->first_index_num;
    last_message = ad->last_index_num;
    current_first_index = ad->ts_current_first_index;
    current_last_index = ad->ts_current_last_index;    


if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * FORWARD TS - INITIAL STATUS FINDING NEXT PRODUCT \n");
fprintf(stderr," * first_message in loop is %d, last_message in loop is %d \n", 
        first_message, last_message);
fprintf(stderr," * current_first_index is %d, current_last_index is %d \n", 
        current_first_index, current_last_index); 
fprintf(stderr," * product index is %d, vol time is %d\n", current_prod_index, curr_time);
}

/*  CVG 6.5 NEW LOGIC */
    /*  1. Starting at current volume until at end of loop */
    /*     (at the end of the list for entire database) */
    /*      a. get and set beginning of next vol */
    /*      b. find base product, set found=TRUE */
    /*      c. get and set end of next vol */
    /*      d. break if found */
    /*      e. reset flags, time, & index and continue if not found */
    /*  2. if not found, go to beginning of loop until original volume */
    /*     (at the beginning of the list for entire database) */
    /*      a. get and set beginning of next vol */
    /*      b. find base product, set found=TRUE */
    /*      c. get and set end of next vol */
    /*      d. break if found */
    /*      e. reset flags, time, & index and continue if not found */

/*  NOTE: ARRAY INDEXs DIFFER BY 1. */
/*        The sort_time_total_list[] begins with index 1 and ends with  */
/*        The product_list[] begins with index 0 and ends with */

/******************************************************************/
    /*  1. Starting at current volume until at end of loop */
    curr_time = vol_time;
    
    for(i=current_first_index; i<=last_message; i++) {
        
        /*      a. get and set beginning of next vol */
        if(found_next_vol_begin == FALSE) {
            if(sort_time_total_list[i+1] > curr_time) {
                curr_time = sort_time_total_list[i+1];
                new_curr_first = i;
                sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
                new_vol_num = vol;
                found_next_vol_begin = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - found VOL BEGIN in first loop, i = %d\n", i);
}
            }
        }
                
        /*      b. find base product, set found=TRUE         */
        if( (found_next_vol_begin == TRUE) &&
            (found_next_prod == FALSE) ) {
            sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
/* TEMP DEBUG */
/* fprintf(stderr,"DEBUG 1ST LOOP, ele=%d, elev_num=%d,  pid=%d, buf_num=%d,  vol=%d, new_vol_num=%d\n", */
/*                 ele, elev_num, pid, buf_num, vol, new_vol_num); */

/* CVG 7.4 */
            ele_n = elev_num_total_list[i+1];

/*             if( ( ele == elev_num ) && ( pid == buf_num ) &&  */
            if( ( ele_n == elev_num ) && ( pid == buf_num ) && 
                                       ( vol == new_vol_num ) ) {
                current_prod_index = i;
                found_next_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - found product in first loop, i = %d\n", i);
}
            }           
        }
        
        /*      c. get and set end of next vol */
        if(found_next_vol_begin == TRUE) {
            if(sort_time_total_list[i+1] > curr_time) {
                new_curr_last = i-1;
                found_next_vol_end = TRUE;   
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - found VOL END in first loop, i = %d\n", i);
}        
            }
        }
        
        /*      d. break if found         */
        /*  found vol begin and at end of loop */
        if( (found_next_vol_begin == TRUE) &&   
            (i == last_message) ) {
            new_curr_last = i;
            found_next_vol_end = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - set vol end to end of loop, in first loop, i = %d\n", i);
}
        }
                
        /*  found vol begin and prod and vol end */
        if( (found_next_vol_begin == TRUE) && 
            (found_next_prod == TRUE) &&  
            (found_next_vol_end == TRUE) ) {
/*  new logic: */
            ad->ts_current_first_index = new_curr_first;
            ad->ts_current_last_index = new_curr_last;

            prod_idx = current_prod_index;
            
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - break out of first loop, i = %d\n", i);
    fprintf(stderr,"             prod index is %d, begin vol is %d, end vol is %d\n",
                                 prod_idx, new_curr_first, new_curr_last);
}
            break;
        }  
         
        /*      e. reset flags, time, & index and continue if not found             */
        /*  handle volume missing base product and continue */
        if( (found_next_vol_begin == TRUE) && 
            (found_next_prod == FALSE) &&  
            (found_next_vol_end == TRUE) ) {
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - volume missing base product, continue.., i = %d\n", i);
}
            if(i == last_message) { 
            /*  CVG 7.4 ELIMINATE READ BEYOND INDEXES */
            /*     curr_time = sort_time_total_list[i+2]; */
            /*     new_curr_first = i + 1; */
            /*     sscanf(product_list[i+1],"%13c%d%d%d", */
            /*                dtime, &vol, &ele, &pid); */
            /*     new_vol_num = vol; */
                ;
            } else {
                curr_time = sort_time_total_list[i+1];
                new_curr_first = i;
                sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
                new_vol_num = vol;
                /*  CVG 7.4 BUG FIX: re-read the first product in the volume */
                i--;
            }
            found_next_vol_begin = TRUE;          
            found_next_vol_end = FALSE;
        }
                    
    } /*  end for */
    

/******************************************************************/
    /*  2. if not found, go to beginning of loop until original volume */
    if(found_next_prod == FALSE) {
        /*  beginning of loop is beginning of volume */
        curr_time = sort_time_total_list[first_message + 1];        
        new_curr_first = first_message;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - set VOL BEGIN to begin of loop in second loop\n");
}

        sscanf(product_list[first_message],"%13c%d%d%d",
                   dtime, &vol, &ele, &pid);
        new_vol_num = vol;
        found_next_vol_begin = TRUE;
        found_next_vol_end = FALSE;

        for(i=first_message; i<=current_last_index; i++) {
            
            /*      a. get and set beginning of next vol */
            /*  this test not actually needed with new sorted list */
            if(found_next_vol_begin == FALSE) {
                if(sort_time_total_list[i+1] > curr_time) {
                    curr_time = sort_time_total_list[i+1];
                    new_curr_first = i;
                    sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
                    new_vol_num = vol;
                    found_next_vol_begin = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - found VOL BEGIN in second loop, i = %d\n", i);
}
                }
            }
            
            /*      b. find base product, set found=TRUE         */
            if( (found_next_vol_begin == TRUE) &&
            (found_next_prod == FALSE) ) {
                sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
/* TEMP DEBUG */
/* fprintf(stderr,"DEBUG 2ND LOOP, ele=%d, elev_num=%d,  pid=%d, buf_num=%d,  vol=%d, new_vol_num=%d\n", */
/*                 ele, elev_num, pid, buf_num, vol, new_vol_num); */

/* CVG 7.4 */
                ele_n = elev_num_total_list[i+1];

/*                 if( ( ele == elev_num ) && ( pid == buf_num ) &&  */
                if( ( ele_n == elev_num ) && ( pid == buf_num ) && 
                                           ( vol == new_vol_num ) ) {
                    current_prod_index = i;
                    found_next_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - found product in second loop, i = %d\n", i);
}
                }
            }
            
            /*      c. get and set end of next vol */
            if(found_next_vol_begin == TRUE) {
                if(sort_time_total_list[i+1] > curr_time) {
                    new_curr_last = i-1;
                    found_next_vol_end = TRUE; 
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - found VOL END in second loop, i = %d\n", i);
}          
                }
            }

            /*      d. break if found             */
            /*  found vol begin and at end of orig volume */
            if( (found_next_vol_begin == TRUE) &&   
                (i == current_last_index) ) {
                new_curr_last = i;
                found_next_vol_end = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - set vol end to original vol, in second loop, i = %d\n", i);
}
            }
            /*  found vol begin and prod and vol end */
            if( (found_next_vol_begin == TRUE) && 
                (found_next_prod == TRUE) &&  
                (found_next_vol_end == TRUE) ) {
/*  new logic: */
                ad->ts_current_first_index = new_curr_first;
                ad->ts_current_last_index = new_curr_last;

                prod_idx = current_prod_index;
                
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - break out of second loop, i = %d\n", i);
    fprintf(stderr,"             prod index is %d, begin vol is %d, end vol is %d\n",
                                 prod_idx, new_curr_first, new_curr_last);
}
                break;
            } 
                       
            /*      e. reset flags, time, & index and continue if not found             */
            /*  handle volume missing base product and continue */
            if( (found_next_vol_begin == TRUE) && 
                (found_next_prod == FALSE) &&  
                (found_next_vol_end == TRUE) ) {
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"FORWARD TS - volume missing base product, continue.., i = %d\n", i);
}
                if(i == current_last_index) { 
                 /*  CVG 7.4 ELIMINATE READ BEYOND INDEXES */
                 /*    curr_time = sort_time_total_list[i+2]; */
                 /*    new_curr_first = i + 1; */
                 /*    sscanf(product_list[i+1],"%13c%d%d%d", */
                 /*               dtime, &vol, &ele, &pid); */
                 /*    new_vol_num = vol;  */
                     ;
                } else {
                    curr_time = sort_time_total_list[i+1];
                    new_curr_first = i;
                    sscanf(product_list[i],"%13c%d%d%d",
                               dtime, &vol, &ele, &pid);
                    new_vol_num = vol;
                    /*  CVG 7.4 BUG FIX: re-read the first product in the volume */
                    i--;
                }
                found_next_vol_begin = TRUE;          
                found_next_vol_end = FALSE;
            }
            
        } /*  end for (second loop)    */
    
    } /*  end if product not found do second loop */

  
if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * FORWARD TS - FINAL STATUS FINDING NEXT PRODUCT \n");
fprintf(stderr," * product index is %d, vol time is %d\n", prod_idx, curr_time);
}  
  

    return prod_idx;

} /* end ts_next_base_prod */






/**********************************************************************/
/* CVG 6.5 LOGIC DESCRIPTION
 * This function finds the previous base product to display for a time series
 * animation.  It uses the index number of the of the first and last 
 * messages of volume containing the currently displayed product, and the 
 * index numbers of the first and last messages encompasing the loop.  
 *
 * Unlike previous version of CVG, each successive volume is searched 
 * until the product is found. The list index of the previous product is
 * returned.  Previous versions of CVG returned the product message in 
 * anim_prod_data and left anim_prod_data == NULL if a product was not found in the 
 * previous volume.
 *
 * No test for product expiration is performed (caller must check this). 
 * The logic now also handles complete volumes missing from the list 
 * (vol time is greater - rather than vol num one greater).
 *
 * LINKED SCREENS NOTE: Since volumes not containing the product are 
 * skipped, if both screens are animated in linked mode they will get
 * out of sync if a product is missing on one screen and not the other.
 */
int ts_prev_base_prod(int buf_num, int elev_num, unsigned int vol_time, 
                                                          int screen_num) {
                                                            
int first_message = 0, last_message = 0, i; 

/*  CVG 6.5 */
    int prod_idx = -1;
    int new_curr_first = -1, new_curr_last = -1;
    unsigned int curr_time = 0;
    int new_vol_num = 0;
    int  vol, ele, pid;
    int ele_n;  /*  CVG 7.4 added */
    char dtime[20];
    int current_prod_index = -1;
    
Boolean found_prev_vol_begin = FALSE, found_prev_vol_end = FALSE;
Boolean found_prev_prod = FALSE;

/* volume limits of currently displayed product */ 
int current_first_index = 0, current_last_index = 0; 

/*  new logic */
anim_data *ad=NULL;

    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    first_message = ad->first_index_num;
    last_message = ad->last_index_num;
    current_first_index = ad->ts_current_first_index;
    current_last_index = ad->ts_current_last_index;


if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * REVERSE TS - INITIAL STATUS FINDING NEXT PRODUCT \n");
fprintf(stderr," * first_message in loop is %d, last_message in loop is %d \n", 
        first_message, last_message);
fprintf(stderr," * current_first_index is %d, current_last_index is %d \n", 
        current_first_index, current_last_index); 
fprintf(stderr," * product index is %d, vol time is %d\n", current_prod_index, curr_time);
}

/*  CVG 6.5 NEW LOGIC */
    /*  1. Starting at current volume until at beginning of loop */
    /*     (at the beginning of the list for entire database) */
    /*      a. get and set end of previous vol */
    /*      b. find base product, set found=TRUE */
    /*      c. get and set beginning of previous vol */
    /*      d. break if found */
    /*      e. reset flags, time, & index and continue if not found */
    /*  2. if not found, go to end of loop until original volume */
    /*     (at the end of the list for entire database) */
    /*      a. get and set end of previous vol */
    /*      b. find base product, set found=TRUE */
    /*      c. get and set beginning of previous vol */
    /*      d. break if found */
    /*      e. reset flags, time, & index and continue if not found */

/*  NOTE: ARRAY INDEXs DIFFER BY 1. */
/*        The sort_time_total_list[] begins with index 1 and ends with  */
/*        The product_list[] begins with index 0 and ends with */

/******************************************************************/
    /*  1. Starting at current volume until at beginning of loop */
    curr_time = vol_time;
    
    for(i=current_last_index; i>=first_message; i--) {
        
        /*      a. get and set end of previous vol */
        if(found_prev_vol_end == FALSE) {
            if(sort_time_total_list[i+1] < curr_time) {
                curr_time = sort_time_total_list[i+1];
                new_curr_last = i;
                sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
                new_vol_num = vol;
                found_prev_vol_end = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - found VOL END in first loop, i = %d\n", i);
}
            }
        }
        
        /*      b. find base product, set found=TRUE         */
        if( (found_prev_vol_end == TRUE) &&
            (found_prev_prod == FALSE) ) {
            sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
/* TEMP DEBUG */
/* fprintf(stderr,"DEBUG 1ST LOOP, ele=%d, elev_num=%d,  pid=%d, buf_num=%d,  vol=%d, new_vol_num=%d\n", */
/*                 ele, elev_num, pid, buf_num, vol, new_vol_num); */
                
/* CVG 7.4 */
            ele_n = elev_num_total_list[i+1];
                
/*             if( ( ele == elev_num ) && ( pid == buf_num ) &&  */
            if( ( ele_n == elev_num ) && ( pid == buf_num ) && 
                                       ( vol == new_vol_num ) ) {
                current_prod_index = i;
                found_prev_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - found product in first loop, i = %d\n", i);
}
            }           
        }
        
        /*      c. get and set beginning of previous vol */
        if(found_prev_vol_end == TRUE) {
            if(sort_time_total_list[i+1] < curr_time) {
                new_curr_first = i+1;
                found_prev_vol_begin = TRUE;   
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - found VOL BEGIN in first loop, i = %d\n", i);
}        
            }
        }
        
        /*      d. break if found         */
        /*  found vol end and at beginning of loop */
        if( (found_prev_vol_end == TRUE) &&   
            (i == first_message) ) {
            new_curr_first = i;
            found_prev_vol_begin = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - set vol begin to begin of loop, in first loop, i = %d\n", i);
}
        }
        /*  found vol begin and prod and vol end */
        if( (found_prev_vol_end == TRUE) && 
            (found_prev_prod == TRUE) &&  
            (found_prev_vol_begin == TRUE) ) {
/*  new logic: */
            ad->ts_current_first_index = new_curr_first;
            ad->ts_current_last_index = new_curr_last;
/* // old logic: */
/*             if(screen_num == SCREEN_1) { */
/*                 ts_current_first_index_s1 = new_curr_first; */
/*                 ts_current_last_index_s1 = new_curr_last; */
/*             } else if(screen_num == SCREEN_2) { */
/*                 ts_current_first_index_s2 = new_curr_first; */
/*                 ts_current_last_index_s2 = new_curr_last; */
/*             } else if(screen_num == SCREEN_3) { */
/*                 ts_current_first_index_s3 = new_curr_first; */
/*                 ts_current_last_index_s3 = new_curr_last; */
/*             } */

            prod_idx = current_prod_index;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - break out of first loop, i = %d\n", i);
    fprintf(stderr,"             prod index is %d, begin vol is %d, end vol is %d\n",
                                 prod_idx, new_curr_first, new_curr_last);
}
            break;
        }  
         
        /*      e. reset flags, time, & index and continue if not found             */
        /*  handle volume missing base product and continue */
        if( (found_prev_vol_end == TRUE) && 
            (found_prev_prod == FALSE) &&  
            (found_prev_vol_begin == TRUE) ) {
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - volume missing base product, continue.., i = %d\n", i);
}
            if(i == first_message) { 
            /*  CVG 7.4 ELIMINATE READ BEYOND INDEXES */
            /*     curr_time = sort_time_total_list[i]; */
            /*     new_curr_last = i - 1; */
            /*     sscanf(product_list[i-1],"%13c%d%d%d", */
            /*                dtime, &vol, &ele, &pid); */
            /*     new_vol_num = vol;  */
                ;
            } else {
                curr_time = sort_time_total_list[i+1];
                new_curr_last = i;
                sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
                new_vol_num = vol;
                /*  CVG 7.4 BUG FIX: re-read the last product in the volume */
                i++;
            }
            found_prev_vol_end = TRUE;          
            found_prev_vol_begin = FALSE;
        }
                    
    } /*  end for */
    

/******************************************************************/
    /*  2. if not found, go to end of loop until original volume */
    if(found_prev_prod == FALSE) {
        /*  end of loop is end of volume */
        curr_time = sort_time_total_list[last_message + 1];        
        new_curr_last = last_message;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - set VOL END to end of loop in second loop\n");
}

        sscanf(product_list[last_message],"%13c%d%d%d",
                   dtime, &vol, &ele, &pid);
        new_vol_num = vol;
        found_prev_vol_end = TRUE;
        found_prev_vol_begin = FALSE;
        
        for(i=last_message; i>=current_first_index; i--) {
            
            /*      a. get and set end of previous vol */
            /*  this test not actually needed with new sorted list */
            if(found_prev_vol_end == FALSE) {
                if(sort_time_total_list[i+1] < curr_time) {
                    curr_time = sort_time_total_list[i+1];
                    new_curr_last = i;
                    sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
                    new_vol_num = vol;
                    found_prev_vol_end = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - found VOL END in second loop, i = %d\n", i);
}
                }
            }
            
            /*      b. find base product, set found=TRUE         */
            if( (found_prev_vol_end == TRUE) &&
                (found_prev_prod == FALSE) ) {
                sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
/* TEMP DEBUG */
/* fprintf(stderr,"DEBUG 2ND LOOP, ele=%d, elev_num=%d,  pid=%d, buf_num=%d,  vol=%d, new_vol_num=%d\n", */
/*                 ele, elev_num, pid, buf_num, vol, new_vol_num); */
                
/* CVG 7.4 */
                ele_n = elev_num_total_list[i+1];
            
/*                 if( ( ele == elev_num ) && ( pid == buf_num ) &&  */
                if( ( ele_n == elev_num ) && ( pid == buf_num ) && 
                                           ( vol == new_vol_num ) ) {
                    current_prod_index = i;
                    found_prev_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - found product in second loop, i = %d\n", i);
}
                }
            }
            
            /*      c. get and set beginning of previous vol */
            if(found_prev_vol_end == TRUE) {
                if(sort_time_total_list[i+1] < curr_time) {
                    new_curr_first = i+1;
                    found_prev_vol_begin = TRUE; 
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - found VOL BEGIN in second loop, i = %d\n", i);
}          
                }
            }

            /*      d. break if found             */
            /*  found vol end and at beginning of orig volume */
            if( (found_prev_vol_end == TRUE) &&   
                (i == current_first_index) ) {
                new_curr_first = i;
                found_prev_vol_begin = TRUE;
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - set vol begin to original vol, in second loop, i = %d\n", i);
}
            }
            /*  found vol begin and prod and vol end */
            if( (found_prev_vol_end == TRUE) && 
                (found_prev_prod == TRUE) &&  
                (found_prev_vol_begin == TRUE) ) {
/*  new logic: */
                ad->ts_current_first_index = new_curr_first;
                ad->ts_current_last_index = new_curr_last;

                prod_idx = current_prod_index;
                
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - break out of second loop, i = %d\n", i);
    fprintf(stderr,"             prod index is %d, begin vol is %d, end vol is %d\n",
                                 prod_idx, new_curr_first, new_curr_last);
}
                break;
            } 
                       
            /*      e. reset flags, time, & index and continue if not found             */
            /*  handle volume missing base product and continue */
            if( (found_prev_vol_end == TRUE) && 
                (found_prev_prod == FALSE) &&  
                (found_prev_vol_begin == TRUE) ) {
if(ANIM_DEBUG == TRUE) {
    fprintf(stderr,"REVERSE TS - volume missing base product, continue.., i = %d\n", i);
}
                if(i == current_first_index) { 
                /*  CVG 7.4 ELIMINATE READ BEYOND INDEXES */
                /*     curr_time = sort_time_total_list[i]; */
                /*     new_curr_last = i - 1; */
                /*     sscanf(product_list[i-1],"%13c%d%d%d", */
                /*            dtime, &vol, &ele, &pid); */
                /*     new_vol_num = vol; */
                    ;
                } else {
                    curr_time = sort_time_total_list[i+1];
                    new_curr_last = i;
                    sscanf(product_list[i],"%13c%d%d%d",
                           dtime, &vol, &ele, &pid);
                    new_vol_num = vol; 
                    /*  CVG 7.4 BUG FIX: re-read the last product in the volume */
                    i++;
                }
                found_prev_vol_end = TRUE;          
                found_prev_vol_begin = FALSE;
            }
            
        } /*  end for (second loop)    */
    
    } /*  end if product not found do second loop */

  
if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * REVERSE TS - FINAL STATUS FINDING NEXT PRODUCT \n");
fprintf(stderr," * product index is %d, vol time is %d\n", prod_idx, curr_time);
}  
  

    return prod_idx;

} /* end ts_prev_base_prod */




/**********************************************************************/
/* CVG 6.5
 * Finds the next overlay product to display for a time series
 * animation.  It uses the first and last messages encompasing the volume
 * of the currently displayed base product.
 *
 * The list index of the overlay product is returned.  Previous versions 
 * of CVG returned the product message in anim_prod_data and left anim_prod_data == NULL 
 * if a product was not found in the base product volume.
 *
 * No test for product expiration is performed (caller must check this). 
 */
int ts_find_ovly_prod(int buf_num, int elev_num, int screen_num) {
                        
int i; 

/*  CVG 6.5 */
    int prod_idx = -1;
    int  vol, ele, pid;
    int ele_n; /*  CVG 7.4 added */
    char dtime[20];

Boolean found_overlay_prod = FALSE;

/* volume limits of currently displayed product */ 
int current_first_index = 0, current_last_index = 0;          


/*  new logic */
anim_data *ad=NULL;

    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    current_first_index = ad->ts_current_first_index;
    current_last_index = ad->ts_current_last_index;
 

if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * FIND OVERLAY - STATUS BEFORE FIRST SEARCH LOOP           *\n");
fprintf(stderr," * current_first_index is %d, current_last_index is %d            *\n", 
        current_first_index, current_last_index); 
fprintf(stderr," * Looking for Product ID %d, elevation %d\n", buf_num, elev_num);
}


    /*  CVG 6.5 NEW LOGIC */
    
    for(i=current_first_index; i<=current_last_index; i++) {

        sscanf(product_list[i],"%13c%d%d%d",
                   dtime, &vol, &ele, &pid);
/* CVG 7.4 */
                ele_n = elev_num_total_list[i+1];
                
/*         if( ( ele == elev_num ) && ( pid == buf_num ) ) { */
        if( ( ele_n == elev_num ) && ( pid == buf_num ) ) {
            prod_idx = i;
            found_overlay_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"FIND OVERLAY found_overlay_prod, i is %d\n", i);                     
}
        break;
        }      
    
    } /*  end for */

    return prod_idx;
    
} /* end ts_find_ovly_prod */





/**********************************************************************/
/*                  FUNCTIONS FOR ELEVATION SERIES                    */
/**********************************************************************/


/* CURRENTLY anim_prod_data is set to NULL before
 * this is called, leaving anim_prod_data = NULL tells the caller no product
 * was found.
 */

/**********************************************************************/
int es_find_next_prod(int buf_num, int hist_index, int screen_num) {

int first_message = 0, last_message = 0, i; 

short *elev_nums = NULL;

/*  CVG 6.5 */
    int prod_idx = -1;
    int  vol, ele, pid;
    int ele_n;  /* CVG 7.4 added */
    char dtime[20];
    short curr_index = -1;

    Boolean found_next_prod = FALSE;

/*  new logic */
anim_data *ad=NULL;

    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    first_message = ad->es_first_index;
    last_message = ad->es_last_index;
    elev_nums = ad->elev_nums;
    curr_index = ad->list_index[hist_index];
    


if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * FORWARE ES - STATUS BEFORE FIRST SEARCH LOOP           *\n");
fprintf(stderr," * first message is %d, last message is %d            *\n", 
        first_message, last_message); 
fprintf(stderr," * Current Product ID %d, elevation %d, list index is %d\n", buf_num, 
                                                  elev_nums[hist_index], curr_index);
}


/*  CVG 6.5 NEW LOGIC */
    /*  1. Starting at current elevation until at end of volume */
    /*      a. find next product, update elevation history, set found=TRUE */
    /*      b. break if found */
    /*  2. if not found, go to beginning of volume until original elevation */
    /*      a. find next product, update elevation history, set found=TRUE */
    /*      b. break if found */

    /*  special case to cover no product found last time */
    if(curr_index == -1)
         curr_index = first_message;
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND NEXT - entering first loop, begin %d, end %d\n", */
/*                                                     curr_index+1, last_message); */
    /*  1. Starting at current elevation until at end of volume */
    for(i=curr_index+1; i<=last_message; i++) {
        
        sscanf(product_list[i],"%13c%d%d%d",
                   dtime, &vol, &ele, &pid);
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND NEXT - pid is %d, buf_num is %d\n", */
/*                                                     pid, buf_num); */
/* CVG 7.4 */
        ele_n = elev_num_total_list[i+1];
        
        if(pid == buf_num) {
            prod_idx = i;
/*             elev_nums[hist_index] = ele; */
            elev_nums[hist_index] = ele_n;
            found_next_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"FORWARD ES - found product in first loop, i is %d\n", i);                     
}
            break;  
        
        }              
        
    } /*  end first loop */


    /*  2. if not found, go to beginning of volume until original elevation */
    if(found_next_prod == FALSE) {
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND NEXT - entering second loop\n, begin %d, end %d\n", */
/*                                                     first_message, curr_index);         */
        for(i=first_message; i<=curr_index; i++) {
            
            sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND NEXT - pid is %d, buf_num is %d\n", */
/*                                                     pid, buf_num); */
/* CVG 7.4 */
            ele_n = elev_num_total_list[i+1];
        
            if(pid == buf_num) {
                prod_idx = i;
/*                 elev_nums[hist_index] = ele; */
                elev_nums[hist_index] = ele_n;
                found_next_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"FORWARD ES - found product in second loop, i is %d\n", i);
}
                break;  
            
            }              
            
            
        } /*  end second loop  */
        
    } /*  end if not found */
           
    
   return prod_idx; 

} /* end es_find_next_prod */




/* CURRENTLY anim_prod_data is set to NULL before
 * this is called, leaving anim_prod_data = NULL tells the caller no product
 * was found.
 */
/**********************************************************************/
int es_find_prev_prod(int buf_num, int hist_index, int screen_num) {

int first_message = 0, last_message = 0, i; 

short *elev_nums = NULL;

/*  CVG 6.5 */
    int prod_idx = -1;
    int  vol, ele, pid;
    int ele_n; /*  CVG 7.4 added */
    char dtime[20];
    short curr_index = -1;

    Boolean found_next_prod = FALSE;

/*  new logic */
anim_data *ad=NULL;

    if(screen_num == SCREEN_1) 
        ad = &anim1;
    else if(screen_num == SCREEN_2)
        ad = &anim2;
    else if(screen_num == SCREEN_3)
        ad = &anim3;

    first_message = ad->es_first_index;
    last_message = ad->es_last_index;
    elev_nums = ad->elev_nums;
    curr_index = ad->list_index[hist_index];
    


if(ANIM_DEBUG == TRUE) {
fprintf(stderr," * FORWARE ES - STATUS BEFORE FIRST SEARCH LOOP           *\n");
fprintf(stderr," * first message is %d, last message is %d            *\n", 
        first_message, last_message); 
fprintf(stderr," * Current Product ID %d, elevation %d, list index is %d\n", buf_num, 
                                                  elev_nums[hist_index], curr_index);
}


/*  CVG 6.5 NEW LOGIC */
    /*  1. Starting at current elevation until at beginning of volume */
    /*      a. find next product, update elevation history, set found=TRUE */
    /*      b. break if found */
    /*  2. if not found, go to end of volume until original elevation */
    /*      a. find next product, update elevation history, set found=TRUE */
    /*      b. break if found */

    /*  special case to cover no product found last time */
    if(curr_index == -1)
         curr_index = last_message;
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND PREV - entering first loop, begin %d, end %d\n", */
/*                                                     curr_index-1, first_message); */
    /*  1. Starting at current elevation until at beginning of volume */
    for(i=curr_index-1; i>=first_message; i--) {
        
        sscanf(product_list[i],"%13c%d%d%d",
                   dtime, &vol, &ele, &pid);
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND NEXT - pid is %d, buf_num is %d\n", */
/*                                                     pid, buf_num); */
/* CVG 7.4 */
        ele_n = elev_num_total_list[i+1];
            
        if(pid == buf_num) {
            prod_idx = i;
/*             elev_nums[hist_index] = ele; */
            elev_nums[hist_index] = ele_n;
            found_next_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"REVERSE ES - found product in first loop, i is %d\n", i);                     
}
            break;  
        
        }              
        
    } /*  end first loop */


/*  2. if not found, go to end of volume until original elevation */
    if(found_next_prod == FALSE) {
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND PREV - entering second loop\n, begin %d, end %d\n", */
/*                                                     last_message, curr_index);         */
        for(i=last_message; i>=curr_index; i--) {
            
            sscanf(product_list[i],"%13c%d%d%d",
                       dtime, &vol, &ele, &pid);
/*  DEBUG */
/* fprintf(stderr,"DEBUG ES FIND PREV - pid is %d, buf_num is %d\n", */
/*                                                     pid, buf_num); */
/* CVG 7.4 */
            ele_n = elev_num_total_list[i+1];
        
            if(pid == buf_num) {
                prod_idx = i;
/*                 elev_nums[hist_index] = ele; */
                elev_nums[hist_index] = ele_n;
                found_next_prod = TRUE;
if(ANIM_DEBUG == TRUE) {
fprintf(stderr,"REVERSE ES - found product in second loop, i is %d\n", i);                     
}
                break;  
            
            }              
            
            
        } /*  end second loop  */
        
    } /*  end if not found */
           
    
   return prod_idx; 

} /* end es_find_prev_prod */

