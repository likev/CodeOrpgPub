/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2001/04/24 14:39:44 $
 * $Id: crda_queue_services.c,v 1.10 2001/04/24 14:39:44 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#include <crda_control_rda.h>

/* This is the definition of the request queue.  It is implemented
   as an arry with count, and front and rear pointers.  It is a
   priority queue in that high priority requests can be inserted
   at the front of the queue, low priority requests are inserted 
   at the rear of the queue. */
typedef struct{

   int count;
   int front;
   int rear;
   Request_list_t *entry[ MAXN_OUTSTANDING ];

} request_queue_t;

/* File scope global variables. */
/* Request queue. */
static request_queue_t Request_queue;


/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Creates the outstanding request queue by initializing 
//      front and rear pointers and queue count.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
void QS_create_request_queue(){

   /* Perform initialization of request queue. */
   Request_queue.count = 0;
   Request_queue.front = 0;
   Request_queue.rear = -1;

/* End of QS_create_request_queue() */
}

/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Checks whether the outstanding request queue is 
//      empty.  
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      QS_QUEUE_EMPTY if the queue is empty; QS_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
int QS_request_queue_empty(){

   if ( Request_queue.count <= 0 )
      return ( QS_QUEUE_EMPTY );

   else
      return ( QS_SUCCESS );

/* End of QS_request_queue_empty() */
}

/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Checks whether the outstanding request queue is full.  
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      QS_QUEUE_FULL if the queue is full; QS_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
int QS_request_queue_full(){

   if( Request_queue.count >= MAXN_OUTSTANDING )
      return ( QS_QUEUE_FULL );

   else
      return ( QS_SUCCESS );

/* End of QS_request_queue_full() */
}

/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Inserts outstanding request index at rear of the 
//      outstanding request queue.  No insertion is 
//      performed if the request queue is full.
//
//   Inputs:
//      list_ptr - pointer to an outstanding request.
//
//   Outputs:
//
//   Returns:
//      QS_QUEUE_FULL if the queue is full; QS_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
int QS_insert_rear_request_queue( Request_list_t *list_ptr ){

   int ret;

   /* Check if queue is full. */
   if( (ret = QS_request_queue_full()) == QS_QUEUE_FULL )
      return ( QS_QUEUE_FULL );

   /* Queue not full.  Insert pointer to request at rear of 
      queue. */
   Request_queue.count++;

   Request_queue.rear = (Request_queue.rear + 1)%MAXN_OUTSTANDING;
   Request_queue.entry[ Request_queue.rear ] = list_ptr;

   return ( QS_SUCCESS );

/* QS_insert_rear_request_queue() */
}

/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Inserts outstanding request index at front of the 
//      outstanding request queue.  No insertion is 
//      performed if the request queue is full.
//
//   Inputs:
//      list_ptr - pointer to an outstanding request.
//
//   Outputs:
//
//   Returns:
//      QS_QUEUE_FULL if the queue is full; QS_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
int QS_insert_front_request_queue( Request_list_t *list_ptr ){

   int ret;

   /* Check if request queue is full. */
   if( (ret = QS_request_queue_full()) == QS_QUEUE_FULL )
      return ( QS_QUEUE_FULL );

   /* Request queue is not full.  Insert pointer to request at 
      front of queue. */
   Request_queue.count++;

   Request_queue.front--;
   if( Request_queue.front < 0 )
      Request_queue.front = MAXN_OUTSTANDING - 1;

   Request_queue.entry[ Request_queue.front ] = list_ptr;

   return ( QS_SUCCESS );

/* QS_insert_front_request_queue() */
}

/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Removes outstanding request index at front of the 
//      outstanding request queue.  No removal is 
//      performed if the request queue is empty.
//
//   Inputs:
//      list_ptr - pointer to pointer to an outstanding 
//                 request.
//
//   Outputs:
//      list_ptr - assigned pointer to an outstanding 
//                 request.  Request if from top of 
//                 queue.
//
//   Returns:
//      QS_QUEUE_EMPTY if the queue is empty; QS_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
int QS_remove_request_queue( Request_list_t **list_ptr ){

   int ret;

   /* Check if queue is empty. */
   if( (ret = QS_request_queue_empty()) == QS_QUEUE_EMPTY )
      return ( QS_QUEUE_EMPTY );

   /* Queue is not empty.  Remove request from front of queue. */
   Request_queue.count--;

   *list_ptr = Request_queue.entry[ Request_queue.front ];
   Request_queue.front = (Request_queue.front+1)%MAXN_OUTSTANDING;

   return ( QS_SUCCESS );

/* End of QS_remove_request_queue() */
}

/*\///////////////////////////////////////////////////////////
//
//   Description:
//      Removes all outstanding requests from the outstanding 
//      request queue.  No removal is performed once the 
//      request queue is empty.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      There is no return values defined for this function. 
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
void QS_clear_request_queue(){

   Request_list_t *list_ptr;
   int ret;

   /* Do until queue is empty. */
   while( (ret = QS_remove_request_queue( &list_ptr )) != QS_QUEUE_EMPTY )

   /* Re-initialize the Outstanding Request list. */
   SWM_init_outstanding_requests();

/* End of QS_clear_request_queue() */
}
