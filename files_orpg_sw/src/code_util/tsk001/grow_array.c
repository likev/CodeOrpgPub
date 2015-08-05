/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:46:02 $
 * $Id: grow_array.c,v 1.4 2008/03/13 22:46:02 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* grow_array.c */

#include "grow_array.h"
#include <stdio.h>
#include <stdlib.h>

/* This module is only used by parse_packet_numbers() in symbology_block.c */
/* for building an array of packet codes and offsets to packets that is    */
/* contained in the layer information portion of the screen data.          */


/* sets the array up so that we can work with it */
void grow_init(grow_array **ga)
{
  *ga = (grow_array *)malloc(sizeof(grow_array));
  (*ga)->size = GROW_DEFAULT_SIZE;
  (*ga)->numitems = 0;
  (*ga)->data = (int *)malloc(GROW_DEFAULT_SIZE*sizeof(int));
}



/* adds an item to the end of the array */
void grow_append(grow_array *ga, int x)
{
  /* if it looks like we're going to overflow the array, make it bigger */
  if(ga->size == ga->numitems) {
      ga->size += GROW_DEFAULT_INCREMENT;
      if((ga->data = realloc(ga->data, ga->size * sizeof(int))) == NULL) {
	      fprintf(stderr, "Out of memory in grow_append()\n");
	      exit(0);
      }
  }

  ga->data[ga->numitems++] = x;
}



/* gets an item from the array */
/* returns 0 if array index is exceeded */
/* currently only used in printf statements */
int grow_access(grow_array *ga, int i)
{
  /* a little bit of bounds checking, because we can */
  if( (i<0) || (i>=ga->numitems) ) {
/*     fprintf(stderr, "invalid index: %d\n", i); */
      return 0;
  }

  return ga->data[i];
}



/* clears up an array */
void grow_delete(grow_array *ga)
{
  free(ga->data);
  free(ga);
}







