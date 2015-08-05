/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:07 $
 * $Id: assoc_array_s.h,v 1.4 2008/03/13 22:45:07 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* assoc_array_s.h
 * implements an associative array, i.e. an array
 * indexed by arbitrary data items, mainly for storing
 * product description information
 *
 * Why? Because we like oddball data structures that
 * save us time and make our code look neater.
 * (and easier to maintain)
 *
 * Future:  change this to a hash table (?)
 */

#ifndef _ASSOC_ARRAY_S_H_
#define _ASSOC_ARRAY_S_H_


typedef struct
{
    int    size;  /* the number of elements in the array */
    int   *keys;  /* the key corresponding to each data element */
    char **data;  /* here, we store description strings */
} assoc_array_s;  /* ... LOTS of description strings */

/* prototypes */
void  assoc_init_s(assoc_array_s **a);
void  assoc_insert_s(assoc_array_s *a, int k, const char *d);
char* assoc_access_s(assoc_array_s *a, int k);
void  assoc_clear_s(assoc_array_s *a);

void  assoc_delete_s(assoc_array_s *a); /*  currently not used */

#endif







