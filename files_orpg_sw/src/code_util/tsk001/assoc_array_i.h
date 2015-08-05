/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:04 $
 * $Id: assoc_array_i.h,v 1.4 2008/03/13 22:45:04 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* assoc_array_i.h
 * implements an associative array, i.e. an array
 * indexed by arbetrary data items, mainly for storing
 * product description information
 *
 * Why? Because we like oddball data structures that
 * save us time and make our code look neater.
 *
 * Future:  change this to a hash table
 *
 * A short note is in order here:
 * We have two copies of this data structure here mainly because
 * strings need special handling, and I prefer to keep that
 * hidden from the calling procedure.  Hopefully, these two
 * should take care of all of our needs.
 */

#ifndef _ASSOC_ARRAY_I_H_
#define _ASSOC_ARRAY_I_H_

typedef struct
{
    int    size;  /* the number of elements in the array */
    int   *keys;  /* the key corresponding to each data element */
    int   *data;  /* here, store whatever integer data you want */
} assoc_array_i;  


/* prototypes */
void  assoc_init_i(assoc_array_i **a);
void  assoc_insert_i(assoc_array_i *a, int k, int d);
int*  assoc_access_i(assoc_array_i *a, int k);
void  assoc_clear_i(assoc_array_i *a);

void  assoc_delete_i(assoc_array_i *a); /*  currently not used */

#endif







