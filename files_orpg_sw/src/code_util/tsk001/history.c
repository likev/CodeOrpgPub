/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:39 $
 * $Id: history.c,v 1.7 2009/05/15 17:52:39 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* history.c */
/* functions for keeping track of what has been plotted */

#include "history.h"

#define FALSE 0
#define TRUE  1

/* Saves the current variables essential for re-plotting products 
 * so that we can reuse them to replot everything on the display
 *
 * The current screen data must be correctly set by the caller.
 * The first arg is a pointer to an array because we resize the array
 * using realloc(), which potentially causes the location of the array
 * to change
 */
void save_current_state_to_history(history_info **hist, int *hist_size)
{
    int ser_len, ret;
    char *serial_data;  /* offset to the serialized data in the packet */

    packet_28_t *pkt28hdr;
    Graphic_product *gp;

    if(verbose_flag)
        fprintf(stderr,"Saving to History\n");

    /* adding the current state means that we need to resize the array */
    (*hist) = (history_info *)realloc(*hist, sizeof(history_info) * 
                      ((*hist_size) + 1));

    gp = (Graphic_product *)(sd->icd_product+96);
    
    (*hist)[*hist_size].icd_product = malloc(gp->msg_len+96);
    memcpy((*hist)[*hist_size].icd_product, sd->icd_product, gp->msg_len+96);

    (*hist)[*hist_size].layers = copy_layer_info(sd->layers, sd->num_layers);
    /* GENERIC_PROD Cannot be treated like the original product because it is not */
    /* a single memory block or like layer info because there is no copy function.*/
    if(sd->generic_prod_data != NULL) { /*  copy by recreating the data */
        pkt28hdr = (packet_28_t *) (sd->icd_product + sd->packet_28_offset);
        ser_len = pkt28hdr->num_bytes;
        serial_data = &sd->icd_product[sd->packet_28_offset+8];
        ret = cvg_RPGP_product_deserialize(serial_data, ser_len, 
                             (void *)&(*hist)[*hist_size].generic_prod_data);
    } else 
        (*hist)[*hist_size].generic_prod_data = NULL;
    
    (*hist)[*hist_size].num_layers = sd->num_layers;   
    (*hist)[*hist_size].packet_select_type = sd->packet_select_type;
    (*hist)[*hist_size].layer_select = sd->layer_select;
    (*hist)[*hist_size].packet_select = sd->packet_select;
    (*hist_size)++;
}


/* Plots each product from the history list in sequence onto the given screen 
 * Set the the current screen data depending on the screen number given
 */
/* this function is only called from replot_image() in callbacks.c */
void replay_history(history_info *hist, int hist_size, int screen_num)
{
    int i;
    layer_info *old_layers;
    char *old_product;
    int old_num_layers, old_packet_select_type, old_packet_select, old_layer_select;
    /*  GENERIC_PROD */
    void *old_generic_prod_data;
    int prev_ovly_flag;

    if(verbose_flag)
        fprintf(stderr,"Replaying History\n");

    if(screen_num == SCREEN_1)
        sd = sd1;
    else if(screen_num == SCREEN_2)
        sd = sd2;
    else if(screen_num == SCREEN_3)
        sd = sd3;
    else
        return;

    /* save current data */
    old_product = sd->icd_product;
    /*  GENERIC_PROD */
    old_generic_prod_data = sd->generic_prod_data;
    old_layers = sd->layers;
    old_num_layers = sd->num_layers;
    old_packet_select_type = sd->packet_select_type;
    old_layer_select = sd->layer_select;
    old_packet_select = sd->packet_select;

    prev_ovly_flag = overlay_flag;
    overlay_flag = FALSE;

    /* plot each history item (previously displayed products) */
    for(i=0; i<sd->history_size; i++) {
        
        sd->icd_product = sd->history[i].icd_product;
        /*  GENERIC_PROD */
        sd->generic_prod_data = sd->history[i].generic_prod_data;
        sd->layers = sd->history[i].layers;
        sd->num_layers = sd->history[i].num_layers;
        sd->packet_select_type = sd->history[i].packet_select_type;
        sd->layer_select = sd->history[i].layer_select;
        sd->packet_select = sd->history[i].packet_select;
      
        if(i>=1) overlay_flag = TRUE;   

        plot_image(screen_num, FALSE);

    }

    /* restore old data */
    sd->icd_product = old_product;
    /*  GENERIC_PROD */
    sd->generic_prod_data = old_generic_prod_data;
    sd->layers = old_layers;
    sd->num_layers = old_num_layers;
    sd->packet_select_type = old_packet_select_type;
    sd->layer_select = old_layer_select;
    sd->packet_select = old_packet_select;
    
    overlay_flag = prev_ovly_flag;
    
    
}



/* deletes everything from the history list - both args are pointers
 * so that we can zero them out
 */
void clear_history(history_info **hist, int *hist_size)
{
    int i, rv;

    if(verbose_flag) {
        fprintf(stderr, "entering clear_history()...\n");
    fprintf(stderr, "history size = %d\n", *hist_size);
    }

    if((*hist) == NULL)
        return;


    for(i=0; i<(*hist_size); i++) {
        if(verbose_flag) {
            fprintf(stderr, "num layers = %d  packet select type = %d  "
                            "packet select = %d  layer select = %d\n", 
                            (*hist)[i].num_layers, (*hist)[i].packet_select_type, 
                            (*hist)[i].packet_select, (*hist)[i].layer_select);
    }

    if(verbose_flag)
        fprintf(stderr, "clearing product...\n");

    if((*hist)[i].icd_product != NULL) 
        free((*hist)[i].icd_product);

    
    if(verbose_flag)
        fprintf(stderr, "clearing layer list...\n");

    if((*hist)[i].layers != NULL)
        delete_layer_info((*hist)[i].layers, (*hist)[i].num_layers);

    /* GENERIC_PRODUCT - not included in every product (test for NULL) */
    if((*hist)[i].generic_prod_data != NULL)
        rv = cvg_RPGP_product_free((*hist)[i].generic_prod_data);

    if(verbose_flag)
        fprintf(stderr, "done\n");
    }

    if(verbose_flag)
        fprintf(stderr, "done clearing elements\n");
    if((*hist) != NULL)
        free(*hist);
    (*hist) = NULL;
    (*hist_size) = 0;
    
}


