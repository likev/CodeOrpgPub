/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/02/07 19:34:36 $
 * $Id: grow_array.h,v 1.2 2003/02/07 19:34:36 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/* grow_array.h */

#ifndef _GROW_ARRAY_H_
#define _GROW_ARRAY_H_

/* these arrays automagically resize themseleves as you add data
 * items on to the end of them -- right now, they only do ints */

#define GROW_DEFAULT_SIZE      20
#define GROW_DEFAULT_INCREMENT 10

struct _grow_array
{
  int  size;        /* the size of the data array */
  int  numitems;    /* the number of items stored in the array */
  int *data;        /* the array in which stuff is stored */
};

typedef struct _grow_array grow_array;


/* prototypes */
void grow_init(grow_array **ga);
void grow_append(grow_array *ga, int x);
int grow_access(grow_array *ga, int i);
void grow_delete(grow_array *ga);

#endif



