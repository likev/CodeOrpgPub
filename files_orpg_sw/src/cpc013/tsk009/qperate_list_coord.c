/*
 */

/*** Local Include Files  ***/

#include "qperate_list_coord.h"
#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_list_coord.c

    Description:
    ============
       add_coord_to_list() function adds a list item to the beginning of the 
    list and adjusts all the necessary pointers.

    Input: 
       listitem_t** mylist  - the list
       int          new_az  - azimuth index.
       int          new_rng - range index.  

    Change History
    ==============
    DATE          VERSION    PROGRAMMER           NOTES
    ----          -------    ----------           -----
    12/08/06      0000       D. Stein & C. Pham   Initial implementation for 
                                                  dual-polarization project
                                                  (ORPG Build 11). 
******************************************************************************/

void add_coord_to_list (struct listitem_t **mylist, int new_az, int new_rng)
{
   struct listitem_t *newitem;

   static unsigned int item_size = sizeof (struct listitem_t);

   if(item_size > 0)
      newitem = (struct listitem_t *) malloc (item_size);
   else
      newitem = NULL;

   if(newitem == NULL)
   {
      #ifdef QPERATE_DEBUG
         fprintf (stderr, "ERROR - malloc failed in add_coord. \n");
      #endif
      return; 
   }
   
   newitem->az = new_az;
   newitem->rng = new_rng;
   newitem->Next = *mylist; /* Put newitem at the head of the list */
   
   if ( !*mylist ) /* if this isn't 1st item on list */
   {
      newitem->Prev = NULL; /* 1st Prev always points to NULL */
   }
   else
   {
      newitem->Prev = (*mylist)->Prev;
      (*mylist)->Prev = newitem;
   }
   
   *mylist = newitem;

} /* end add_coord_to_list() ------------------------------- */

/******************************************************************************
    Filename: qperate_list_coord.c

    Description:
    ============
       get_coord_from_list() function will remove the "current" list item from
    the list and connect all the necessary links.  The function will also free
    the memory occupied by the list item.  Finally, the list item's az value  
    will be placed in new_az and the item's rng value will be placed in 
    "new_rng".  The static variable "curr_pos" keeps track of the current  
    position within the list.  Once curr_pos has reached the end of the list, 
    get_coord will return FALSE indicating that there are no more unchecked
    items on the list.  Otherwise, a value of TRUE will be returned indicating
    that there are more list items waiting to be checked.

    Input: 
       listitem_t** mylist   - the current list item from the list. 
       listitem_t** curr_pos - the current position within the list.

    Output:
       int* new_az  - radial index
       int* new_rng - sample bin index
    
    Return:
       The FALSE flag - no more unchecked items on the list
           TRUE       - more list items waiting to be checked

    Change History
    ==============
    DATE          VERSION    PROGRAMMER           NOTES
    ----          -------    ----------           -----
    12/08/06      0000       D. Stein & C. Pham   Initial implementation for 
                                                  dual-polarization project
                                                  (ORPG Build 11). 
******************************************************************************/

int get_coord_from_list ( struct listitem_t **mylist,
                          struct listitem_t **curr_pos, 
                          int *new_az, int *new_rng )
{
   struct listitem_t *listitem = NULL;
   
   if ( *curr_pos != NULL ) /* if we're not at the end of the list */
   {
      *new_az = (*curr_pos)->az;
      *new_rng = (*curr_pos)->rng;

      /* Now we want to remove this item from the list and update all the
       * pointers.
       */
      if ( (*curr_pos)->Prev != NULL ) /* If this isn't the first item */
      {
         #ifdef QPERATE_DEBUG
            fprintf (stderr, "Not the 1st item\n");
         #endif

         listitem = *curr_pos;

         ((*curr_pos)->Prev)->Next = (*curr_pos)->Next; 

         #ifdef QPERATE_DEBUG
            fprintf (stderr, "Just set the Next pointer\n");
         #endif

         if ( (*curr_pos)->Next != NULL ) /* If this isn't the last item */
         {
            ((*curr_pos)->Next)->Prev = (*curr_pos)->Prev;
         }

         #ifdef QPERATE_DEBUG
            fprintf (stderr, "Just set the Prev pointer\n");
         #endif

         *curr_pos = (*curr_pos)->Next;

         if(listitem != NULL)
            free(listitem);
      }
      else /* This is the first item */
      {
         listitem = *curr_pos;

         #ifdef QPERATE_DEBUG
            fprintf (stderr, "This IS the 1st Item\n");
         #endif

         if ( (*curr_pos)->Next != NULL ) /* If this isn't the last item */
         {
           ((*curr_pos)->Next)->Prev = NULL; /* Now the new 1st item */
         }

         #ifdef QPERATE_DEBUG
            fprintf (stderr, "Just set the Prev pointer to NULL\n");
         #endif

	 *mylist = (*curr_pos)->Next;

         #ifdef QPERATE_DEBUG
            fprintf (stderr, "Set mylist to the Next\n");
         #endif

         *curr_pos = (*curr_pos)->Next;

         #ifdef QPERATE_DEBUG
            fprintf (stderr, "Set curr_pos to the Next\n");
         #endif

         if(listitem != NULL)
            free (listitem);
      }

      return ( TRUE );
         
   } 
   else /* we're at the end of the list */
   {
      #ifdef QPERATE_DEBUG
         fprintf (stderr,"ERROR - get_coord curr_pos at the end of the list\n");
      #endif

      /* Set curr_pos back to the beginning for next time */
      *curr_pos = *mylist;
      return ( FALSE );
   }

} /* end get_coord_from_list() ----------------------------- */

/******************************************************************************
    Filename: qperate_list_coord.c

    Description:
    ============
       destroy_list() function destroys the linked list.

    Input: 
       listitem_t** mylist - the list to destroy

    Change History
    ==============
    DATE          VERSION    PROGRAMMER           NOTES
    ----          -------    ----------           -----
    12/08/06      0000       D. Stein & C. Pham   Initial implementation for 
                                                  dual-polarization project
                                                  (ORPG Build 11). 
******************************************************************************/

void destroy_list ( struct listitem_t **mylist )
{
   struct listitem_t  *tmplist = *mylist;

   while(*mylist != NULL) /* while we haven't reached the end of the list */
   {
      *mylist = (*mylist)->Next;
      if(tmplist != NULL)
         free (tmplist);
      tmplist = *mylist;
   }

} /* end destroy_list() ------------------------------------ */
