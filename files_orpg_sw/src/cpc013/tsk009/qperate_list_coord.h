/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:12:16 $
 * $Id: qperate_list_coord.h,v 1.3 2009/03/03 18:12:16 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef LIST_COORD_H   
#define LIST_COORD_H

/* Declare structure for hybrid scan coordinates list -----------------------*/

struct listitem_t
{
    int az;
    int rng;
    struct listitem_t *Next;
    struct listitem_t *Prev;
};

void add_coord_to_list ( struct listitem_t **mylist, int new_az, int new_rng );

int get_coord_from_list ( struct listitem_t **mylist, 
                          struct listitem_t **curr_pos,
                          int *new_az, int *new_rng );

void destroy_list ( struct listitem_t **mylist );

#endif /* LIST_COORD_H */
