/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:05 $
 * $Id: assoc_array_s.c,v 1.4 2008/03/13 22:45:05 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* assoc_array.c
 * The only functions implemented so far are very basic ones
 * If you need more, feel free to make them
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "assoc_array_s.h"

/* Initializes an associative array.  Must be called before
 * the array is otherwise used.
 *
 * SIDEFFECTS: allocates memory for assoc_array
 * RESULT: a bright, shiny assoc_array, ready for use
 */
void assoc_init_s(assoc_array_s **a) 
{
    *a = (assoc_array_s *)malloc(sizeof(assoc_array_s));
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
void  assoc_insert_s(assoc_array_s *a, 
		     int k,            /* the key/data pair to insert */ 
		     const char *d)
{
  int i;

  for(i=0; i<a->size; ++i) 
      if(a->keys[i] == k) {
	  /* get rid of old data */
	  free(a->data[i]);
	  /* add new data */
	  if((a->data[i] = malloc(sizeof(char)*(strlen(d)+1))) == NULL) {
	      fprintf(stderr, "memory allocation error in assoc_insert_s()\n");
	      exit(0);
	  }
	  strcpy(a->data[i], d);  /* add new value for key */
	  return;
      }

  /* if we can't find the key in the array */
  a->size++;
  if((a->keys = (int *)realloc(a->keys, a->size*sizeof(int))) == NULL) {
      fprintf(stderr, "memory allocation error in assoc_insert_s()\n");
      exit(0);
  }
  a->keys[a->size-1] = k;
  if((a->data = (char **)realloc(a->data, a->size*sizeof(char*))) == NULL) {
      fprintf(stderr, "memory allocation error in assoc_insert_s()\n");
      exit(0);
  }
  if((a->data[a->size-1] = (char *)malloc((strlen(d)+1)*sizeof(char))) == NULL){
      fprintf(stderr, "memory allocation error in assoc_insert_s()\n");
      exit(0);
  }
  strcpy(a->data[a->size-1], d);
}

/* Linearly searches the array for the element associated with a
 * certain key
 *
 * RESULT: returns a pointer to the string associated with k, 
 * if k exists in the array, otherwise it returns NULL
 */
char* assoc_access_s(assoc_array_s *a, 
		     int k)            /* the key of the element we want */
{
  int i;
  for(i=0; i<a->size; i++)
      if(a->keys[i] == k)
	  return a->data[i];

  return NULL;
}


/*  new CVG 8.0 */
/* Frees up all the memory associated with the associative array
 * The array must be reloaded
 *
 * RESULT: the array is emptied
 */
void  assoc_clear_s(assoc_array_s *a)
{
  int i;
  
    for(i=0; i < a->size; i++) {
        free(a->data[i]);
/*         free(a->keys[i]); */
    }
   
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
void  assoc_delete_s(assoc_array_s *a)
{
  int i;
/*  cvg 8.0 */
/*   for(i=0; i<a->size; i++) */
/*       free(a->data[i]); */
    for(i=0; i < a->size; i++) {
        free(a->data[i]);
/*         free(a->keys[i]); */
    }

  free(a->keys);
  free(a->data);
  free(a);
}


