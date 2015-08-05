/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/03/21 20:59:09 $
 * $Id: mda2d_list.c,v 1.3 2005/03/21 20:59:09 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda2d_list.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        Oct. 30, 2002                                         *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This file provides all functions that make            *
 *                      the data structure linked-list usage convenient       *
 *      notes:          none                                                  *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mda2d_acl.h" 

/*====================================================================*/
/*
 * make a new node, inserting "value". Return a pointer to the node.
 * Return NULL when out of dynamic storage space. Assumes the defination
 * in "list.h".
 */

NODEPTR make_node(Shear_vect_2d value)
{
	NODEPTR newptr;

	if ((newptr = (NODEPTR) malloc(sizeof(struct note))) != NULL)
	 {
	  newptr->data = value;
	  newptr->next = NULL;
         }
	return newptr;
} /* END of the function make_node(Shear_vect_2d) */

/* =====================================================================*/
/*
 * Print a list pointed by "L"
 */

void print_list(NODEPTR L)
{
	for (; L != NULL; L = L->next)
         fprintf(stderr, "%f   %f   %f   %f   %f\n", L->data.range, 
	   	L->data.beg_azm, L->data.end_azm, L->data.beg_vel, L->data.end_vel);
 
        fprintf(stderr, "\n");
}/*END of print_list(NODEPTR) function */

/* =================================================================== */
/*
 * Insert "val" into already sorted linked lis "L". return nonzaro
 * if space is vailable. Zero otherwise. Uses make_node to create node.
 */

int insert_list_in_order (Shear_vect_2d val, NODEPTR *L)
{
        NODEPTR curr = *L,
                prev = NULL,
                temp;

        /* match until correct place, or until end of list */
	for (; curr != NULL && ((val.range < curr->data.range) ||
          ((val.range == curr->data.range) && (val.beg_azm < curr->data.beg_azm))); 
			curr = curr->next)
            prev = curr;

        /* Get new node and insert into proper place ( if space is available). */
        if ((temp =  make_node(val)) != NULL)
         {
          temp->next = curr;
          if (prev == NULL)
                *L = temp;
          else
                prev->next = temp;
         }
        return temp !=NULL;
} /* END of int insert_list */


/* ======================================================================*/
/*
 * Return each element in "L" to the FSP, then set "L" to the empty list.
 */


void free_list(NODEPTR *L)
{
	NODEPTR curr = *L,
		temp;

	for (; curr != NULL; curr = temp)		
         {
          temp = curr->next;
          free((char *) curr); 
         }
	*L = NULL;          
} /* END of free_list function */
