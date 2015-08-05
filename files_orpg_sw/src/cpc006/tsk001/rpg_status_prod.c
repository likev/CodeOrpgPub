/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/08/21 16:48:30 $
 * $Id: rpg_status_prod.c,v 1.8 2007/08/21 16:48:30 cmn Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#define RPG_STATUS_PROD_C
#include <rpg_status_prod.h>

/* Macro definitions. */
#define SECS_IN_HOUR       3600
#define DEFAULT_GEN_PERIOD    8

/* Global variables. */
static time_t Current_time;
static int    Current_hour;
static int    Previous_hour;
static int    Read_status_log = 0;
static int    Num_messages = 0;
static Status_prod_t Adapt;
static char  *Month_label [] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* Function prototypes. */
void Handle_notification( int data_id, LB_id_t msg_id );
static int Process_log_entry();
static int Add_message( char *msg, unsigned int code );
static void Free_list();
static int Get_status_prod_adapt( void *struct_addr );

/*\///////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Main routine for building the Archive III status product.
//
///////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int ret;

   /* Register output.  The output is DEMAND_DATA so no requests for this
      product are required for product generation. */
   RPGC_reg_outputs( argc, argv );

   /* Register UN events so as to be notified whenever a new System Status
      Log message is written. */
   ret = RPGC_UN_register( ORPGDAT_SYSLOG_SHADOW, LB_ANY, Handle_notification );
   if( ret < 0 ){

      /* This is a fatal error!!!!! */
      RPGC_log_msg( GL_INFO, "RPGC_UN_register Failed (%d)\n", ret );
      RPGC_hari_kiri();

   }

   /* Register Event Timeout. */
   RPGC_reg_for_internal_event( EVT_WAIT_FOR_EVENT_TIMEOUT, NULL, 60 );

   /* Do task initialization. */
   RPGC_task_init( TASK_EVENT_BASED, argc, argv );

   /* Position the read pointer for the shadow Status Log.  This will
      be the first unread message in this log. */
   ORPGDA_seek( ORPGDAT_SYSLOG_SHADOW, 0, LB_FIRST, NULL );

   /* Data needed to determine when to generate a product. */
   Current_time = time( NULL );
   Current_hour = Current_time / SECS_IN_HOUR;
   Previous_hour = Current_hour;

   /* Get the generation period from adaptation data. */
   Get_status_prod_adapt( &Adapt );
   RPGC_log_msg( GL_INFO, "The Status Product Will Be Generated Every %d hrs\n",
                 Adapt.gen_period );
   
   /* Process status log events. */
   while(1){

      RPGC_wait_for_event();

      /* If we need to read the system status log, do it now. */
      if( Read_status_log ){

         Read_status_log = 0;

         /* Any negative return value from Process_log_entry is 
            considered a fatal error (i.e., they are either malloc
            failures or ORPGDA_read failures). */
         if( Process_log_entry() < 0 )
            RPGC_hari_kiri();

      }

      /* Determine if it is time to produce the next product. */
      Current_time = time( NULL );
      Current_hour = Current_time / SECS_IN_HOUR;

      if( (Current_hour != Previous_hour) 
                        && 
          ((Current_hour % Adapt.gen_period) == 0) ){

         if( Num_messages > 0 ){

            RPGC_log_msg( GL_INFO, "Time to build new product.\n" );

            /* A negative return value from Build_status_product
               is non-fatal.  We just won't build a product this
               time around. */
            if( Build_status_product( Num_messages ) < 0 )
               RPGC_abort();

            /* Clear Num_messages from the shadow file since these messages
               have already been processed. */
            ORPGDA_clear( ORPGDAT_SYSLOG_SHADOW, Num_messages );
     
            /* Reset the number of messages processed. */
            Num_messages = 0;

            /* Free memory allocated to the linked list of status messages. */
            Free_list();

         }

         Previous_hour = Current_hour;

      }

   /* Wait for the system status log to be undated again. */
   }
    
   return 0;

} /* End of main() */

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Notification handler called when data_id and msg_id have been 
      updated.  When called, Read_status_log flag is set.

   Inputs:
      data_id - Data ID of LB (ORPGDAT_SYSLOG_SHADOW)
      msg_id - Message ID within LB.

   
///////////////////////////////////////////////////////////////////////\*/
void Handle_notification( int data_id, LB_id_t msg_id ){

   /* Verify the data ID .... */
   if( data_id == ORPGDAT_SYSLOG_SHADOW )
      Read_status_log = 1;

} /* End of Handle_notification(). */

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      This function reads each unread RPG Status Log message.  It
//      formats the time string, constructs a message with the text and
//      time string prepended, then calls Add_message() to add this
//      message to a linked list of messages for later processing.
//
//   Returns:
//      -1 on error, or 0 otherwise.  
//
///////////////////////////////////////////////////////////////////////\*/
static int Process_log_entry(){

   int ret, year, month, day, hour, minute, second, code;
   char *buf, *ptr, prod_msg[256];
   LE_critical_message *msg;

   while(1){

      /* Exhaust all unread messages. */
      ret = ORPGDA_read( ORPGDAT_SYSLOG_SHADOW, &buf, LB_ALLOC_BUF, LB_NEXT );
      if( ret == LB_TO_COME )
         return 0;

      /* On error return.  For expired messages, we need to seek to the first
         unread message. */
      if( ret < 0 ){

         RPGC_log_msg( GL_ERROR, "ORPGDA_read( ORPGDAT_SYSLOG_SHADOW ) Failed (%d)\n",
                       ret );

         if( ret == LB_EXPIRED ){

            ORPGDA_seek( ORPGDAT_SYSLOG_SHADOW, 0, LB_FIRST, NULL );
            RPGC_log_msg( GL_ERROR, "--->Seeking to first unread message\n" );
            continue;

         }

         return(-1);

      }

      msg = (LE_critical_message *) buf;

      /* Format the time to read month day, year hh:mm:ss */
      unix_time (&msg->time, &year, &month, &day, &hour, &minute, &second);
      year = year%100;

      /* If the task name: string is prepended, strip it off. */
      ptr = strstr( msg->text, ":" );
      if( ptr == NULL ) 
         ptr = msg->text;

      else
         ptr = ptr + 1;

      sprintf( prod_msg, "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
               Month_label [month-1], day, year, hour,
               minute, second, ptr);

      /* Extract the code information.   This will be used for setting 
         parameter data in the RPGP_text_t structure. */
      code = msg->code;

      free(buf);

      RPGC_log_msg( GL_INFO, "--->%s\n", prod_msg );   
      if( (ret = Add_message( prod_msg, code )) < 0 )
         return( ret );

   }

   return(0);

} /* End of Process_log_entry() */

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Creates a Text_node_t structure and populates this structure 
//      with "msg" and "code".
//
//   Inputs:
//      msg - the formatted System Status Log message.
//      code - a code corresponding to type of message.
//
//   Returns:
//      -1 on error, or 0 otherwise.
//
///////////////////////////////////////////////////////////////////////\*/
static int Add_message( char *msg, unsigned int code ){

   Text_node_t *node = NULL;
   char *text;

   /* Allocate space for RPGP_text_node_t structure. */
   if( (node = (Text_node_t *) malloc( sizeof( Text_node_t ) ))   
                                       == (Text_node_t *) NULL ){

      RPGC_log_msg( GL_ERROR, "malloc Failed for %d Bytes.\n",
                    sizeof( Text_node_t ) );
      return(-1);

   }

   node->next = (Text_node_t *) NULL;
   if( (text = (char *) calloc( 1, strlen( msg ) + 1 )) == (char *) NULL ){

      RPGC_log_msg( GL_ERROR, "malloc Failed for %d Bytes.\n",
                    strlen(msg) + 1 );
      return(-1);

   }

   /* Copy the text and append a line feed. */
   strcpy( text, msg );
   node->text = text;
   node->code = code;

   /* If List is empty, start the list. */
   if( List_head == NULL )
      List_head = node;

   /* Else add node to end of list. */
   else
      List_tail->next = node;   

   List_tail = node;

   /* Increment the number of status messages so far. */
   Num_messages++;

   return(0);

/* End of Add_message() */
}

/*\///////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Frees all the nodes in the linked list of nodes
//
///////////////////////////////////////////////////////////////////////////////\*/
static void Free_list(){

   Text_node_t *prev_node, *next_node;

   /* Loop through the linked list of Text_node_t structures and free each node. */
   prev_node = List_head;
   if( prev_node != NULL ){

      while(1){

         next_node = prev_node->next;
         free( prev_node );
         if( next_node == NULL )
            break;

         prev_node = next_node;

      }

   }

   Num_messages = 0;
   List_head = NULL;
   List_tail = NULL;

   return;

} /* End of Free_list() */

/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Function to read in the status product adaptation data.
//
//   Inputs:
//      struct_addr - address to receive adaptation data.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////\*/
int Get_status_prod_adapt( void *struct_addr ){

   int ret = -1;
   double value;
   char *ds_name = NULL;

   Status_prod_t *status_prod = (Status_prod_t *) struct_addr;

   ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
   if( ds_name != NULL )
      DEAU_LB_name( ds_name );

   ret = DEAU_get_values( "status_prod.gen_period", &value, 1 );
   if( ret >= 0 )
      status_prod->gen_period = (int) value;
   
   else{

      RPGC_log_msg( GL_ERROR, "STATUS PROD: gen_period unavailable!!!!!\n" );
      RPGC_log_msg( GL_ERROR, "--->Setting gen_period to %d hrs\n", DEFAULT_GEN_PERIOD );
     
      status_prod->gen_period = DEFAULT_GEN_PERIOD;

   }

   return 0;

}
