/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:03 $
 * $Id: assoc_array_i.c,v 1.4 2008/03/13 22:45:03 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* assoc_array_i.c
 * The only functions implemented so far are very basic ones
 * If you need more, feel free to make them
 */

#include <stdlib.h>
#include "assoc_array_i.h"

/* Initializes an associative array.  Must be called before
 * the array is otherwise used.
 *
 * SIDEFFECTS: allocates memory for assoc_array
 * RESULT: a bright, shiny assoc_array, ready for use
 */
void assoc_init_i(assoc_array_i **a) 
{
    *a = (assoc_array_i *)malloc(sizeof(assoc_array_i));
    (*a)->size = 0;
    (*a)->keys = NULL;
    (*a)->data = NULL;
}

/* Adds a (key,data) pair to the array.  Right now, we simply
 * search through the list to see if the key is already there.
 * if not, we append the pair to the end of the array
 *
 * SIDEFFECTS: increases both arrays by 1, and allocates enough
 * memory for a copy of the data
 * RESULT: the array has a new value in it!
 */
void  assoc_insert_i(assoc_array_i *a, 
		     int k,            /* the key/data pair to insert */ 
		     int d)
{
  int i;

  for(i=0; i<a->size; ++i) 
      if(a->keys[i] == k) {
	  a->data[i] = d;
	  return;
      }

  /* if we can't find the key */
  a->size++;
  a->keys = (int *)realloc(a->keys, a->size*sizeof(int));
  a->keys[a->size-1] = k;
  a->data = (int *)realloc(a->data, a->size*sizeof(int));
  a->data[a->size-1] = d;
}

/* Linearly searches the array for the element associated with a
 * certain key
 *
 * RESULT: returns a pointer to the int associated with k, 
 * if k exists in the array, otherwise it returns NULL
 */
int *assoc_access_i(assoc_array_i *a, 
		    int k)            /* the key of the element we want */
{
  int i;
  for(i=0; i<a->size; i++)
      if(a->keys[i] == k)
	  return &(a->data[i]);

  return NULL;
}



/*  new CVG 8.0 */
/* Frees up all the memory associated with the associative array
 * The array must be reloaded
 *
 * RESULT: the array is emptied
 */
void  assoc_clear_i(assoc_array_i *a)
{
    
   
    a->size = 0;
    a->keys = NULL;
    a->data = NULL;  
    
}



/*  CURRENTLY NOT USED!!  NEEDS MODIFICATION!! */
/* Frees up all the memory associated with the associative array
 * The array must not be used after this call unless reinited
 *
 * RESULT: the array is completely wiped from exsistance
 */
void  assoc_delete_i(assoc_array_i *a)
{


  free(a->keys);
  free(a->data);
  free(a);
}
