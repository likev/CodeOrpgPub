/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/02/07 19:31:10 $
 * $Id: layer_info.c,v 1.2 2003/02/07 19:31:10 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/* layer_info.c */
/* misc helper routines associated with the layer_info struct */

#include "layer_info.h"

/* frees all malloced memory with the layer info */
void delete_layer_info(layer_info *linfo, int size)
{
    int i;

    if(linfo == NULL) return;
    for(i=0; i<size; i++) {
        free(linfo[i].codes);
	free(linfo[i].offsets);
    }

    free(linfo);
}

/* allocates memory and copies contents of layer information */
layer_info *copy_layer_info(layer_info *linfo, int size)
{
    layer_info *ret;
    int i, k;
    
    ret = NULL;
    
    ret = (layer_info *)malloc(sizeof(layer_info) * size);
    
    for(i=0; i<size; i++) {
        ret[i].num_packets = linfo[i].num_packets;
        ret[i].index = linfo[i].index;
        ret[i].codes = (int *)malloc(sizeof(int) * (linfo[i].num_packets));
        ret[i].offsets = (int *)malloc(sizeof(int) * (linfo[i].num_packets));
        for(k=0; k<linfo[i].num_packets; k++) {
            ret[i].codes[k] = linfo[i].codes[k];
            ret[i].offsets[k] = linfo[i].offsets[k];
        }
    }
    	
    return ret;	
	
}











