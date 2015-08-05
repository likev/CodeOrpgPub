/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2012/09/27 14:43:47 $ $Id: ps_task_status_list.c,v 1.48 2012/09/27 14:43:47 steves Exp $
 * $Revision: 1.48 $ $State: Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <infr.h>
#include <orpg.h>
#include <gen_stat_msg.h>
#include <mrpg.h>
#include <prod_sched.h>

#define PS_TASK_STATUS_LIST
#include <ps_globals.h>
#undef PS_TASK_STATUS_LIST

#include <ps_def.h>


/* Constant Definitions/Macro Definitions/Type Definitions */
typedef struct text_t {
 
   char *str;
   struct text_t *next;

} Text_t;


/* Static Globals */
Prod_wth_only_id_next *Unavailable_list = NULL;
Prod_wth_only_id_next *Available_list = NULL;

static double RDA_latitude;
static double RDA_longitude;
static char *ICAO = NULL;

/* Static (Local) Function Prototypes */
static void Report_prod_status();
static int Set_internal_task_status( int status );
static void Format_product_status_msg( Prod_wth_only_id_next *list, 
                                       int status );
static void Add_to_prod_list( Prod_wth_only_id_next **list,
                              Task_prod_wth_only_id_next *dep_task_prods );
static void Free_prod_list( Prod_wth_only_id_next **list );
static int Check_scheduling_info( prod_id_t prod_id, char *task_name );
static unsigned int Prod_valid_for_wx_mode(int prod_id);
static int Wx_mode_matches_wx_mode_attr( int weather_mode, int wx_mode_attr );


/*** Public Functions Start Below ...  ***/
/**************************************************************************
 Description:
    Initializes this module.

 Inputs:

 Outputs:

 Returns:

 Notes:
**************************************************************************/
void PSTS_initialize(){

   char *buf;

   Psg_task_status_changed = 0;
   Psg_product_scheduling_changed = 0;

   /* Get the radar latitude and longitude. */
   DEAU_get_values( "site_info.rda_lat", &RDA_latitude, 1 );
   DEAU_get_values( "site_info.rda_lon", &RDA_longitude, 1 );

   /* Get the ICAO. */
   if( DEAU_get_string_values( "site_info.rpg_name", &buf ) > 0 )
         ICAO = strdup( buf );

/* End of PSTS_initialize() */
}

/**************************************************************************
 Description: 
    This module sets the task status field in the Task Product List. 
    Task status is obtained via ORPGSTAT function calls.
 
 Input: 
    task_prod_list - pointer to start of Task Product List.

 Output: 

 Returns:

 Notes:

**************************************************************************/
int PSTS_update_task_status(){

   Task_prod_chain *task_prod_list, *next_node, *end_node;
   Task_prod_wth_only_id_next *dep_tasks_prods;
   int ret, task_prod_list_size = 0;

   Mrpg_process_table_t *tskstat_entry = NULL;

   /* Set flag indicating the task status has not changed.  Set flag 
      indicating product scheduling has not changed. */
   Psg_task_status_changed = 0;
   Psg_product_scheduling_changed = 0;

   /* Get pointer to Task Product List. */
   task_prod_list = PSPTT_get_task_prod_list( &task_prod_list_size );
   if( task_prod_list == NULL ){

      LE_send_msg( GL_ERROR, "Unable to Update Task Status\n" );
      return (PS_DEF_FAILED );

   }


   /* Create a Mrpg_process_table_t entry if not already created. */
   tskstat_entry = 
      (Mrpg_process_table_t *) MISC_malloc( ALIGNED_SIZE(sizeof(Mrpg_process_table_t))+ORPG_TASKNAME_SIZ );

   /* Initialize all task statuses to PGS_UNKNOWN. */
   next_node = task_prod_list;
   end_node = task_prod_list + task_prod_list_size;
   while( next_node < end_node ){

      next_node->status = PGS_UNKNOWN;
      next_node++;

   }

   /* Do For All Tasks in the Task Product List.  The Task Product List
      is assumed a linear array of Task_prod_chain entries. */
   next_node = task_prod_list;
   end_node = task_prod_list + task_prod_list_size;
   while ( next_node < end_node  ){

      char *cpt;

      tskstat_entry->instance = 0; 
      tskstat_entry->name_off = ALIGNED_SIZE(sizeof(Mrpg_process_table_t));

      cpt = (char *) tskstat_entry + tskstat_entry->name_off;
      strcpy( cpt, (char *) &(next_node->task_name[0]) ); 

      tskstat_entry->size = ALIGNED_SIZE(sizeof(Mrpg_process_table_t)) +
                            strlen( (char *) &next_node->task_name[0] ) + 1;

      /* Read the task status. */
      if( (ret = ORPGMGR_read_task_status( tskstat_entry )) < 0 ){

         LE_send_msg( GL_ERROR, "ORPGMGR_read_task_status Failed (%d)\n", ret );
         next_node->status = PGS_UNKNOWN;

      }
      else{

         /* Set the task status. */
         next_node->status = Set_internal_task_status( tskstat_entry->status );

      }
    
      /* Prepare for next node in the Task Product List. */
      next_node++;

   /* End of "while" loop. */
   }

   /* No longer need the Mrpg_process_table_t entry. */
   if( tskstat_entry != NULL )
      free( tskstat_entry );

   /* If any of the status values indicates the task is not running or
      has failed, mark those tasks which are dependent on this task's
      execution with the same task status. */
   next_node = task_prod_list;
   end_node = task_prod_list + task_prod_list_size;
   while( next_node < end_node ){

      if( (next_node->status == PGS_TASK_FAILED)
                            ||
          (next_node->status == PGS_TASK_NOT_RUNNING) ){ 

          LE_send_msg( GL_INFO, "Task %s is Not Running ....\n", next_node->task_name );

          /* Determine all tasks which are dependent on this task. */
          dep_tasks_prods = PSPTT_find_entry_in_depend_prod_list( next_node->task_name );
          if( dep_tasks_prods == NULL ){

             if( Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL )
                LE_send_msg( GL_INFO, "There is No Depend Product List Entry For Task %s\n", 
                             next_node->task_name );

          }
          else{

             Task_prod_wth_only_id_next *next_task;
 
             /* Set new status for all dependent tasks. */
             next_task = dep_tasks_prods;
             while( next_task != NULL ){
                
                /* Only set the generating task status if the product is mandatory. */
                if( next_task->mand_or_opt == MANDATORY )
                   PSTS_set_task_prod_list_status( next_task->gen_task, 
                                                   next_node->status );
                next_task = next_task->next;

             }

          }

      }
      else{

         Prod_wth_only_id_next *next_prod = next_node->gen_prod_list;
         int prev_schedule;

         /* Check product scheduling adaptation data for this task. */
         while( next_prod != NULL ){

            /* See if the product is enabled/disabled. */
            prev_schedule = next_prod->schedule;
            next_prod->schedule = Check_scheduling_info( next_prod->prod_id, 
                                                         next_node->task_name );

            if( prev_schedule != next_prod->schedule )
               Psg_product_scheduling_changed = 1;

            next_prod = next_prod->next;

         }
          
      }

      /* Prepare for next task in the Task Product List. */
      next_node++; 

   /* End of "while" loop. */
   }

   /* List products (codes) which cannot be generated or have become
      generatable. */
   Report_prod_status();

   /* Return to caller. */
   return (PS_DEF_SUCCESS);

/* END of PSTS_update_task_status() */
}

/**************************************************************************
   Description: 
      Gets the task id from the Product Attributes Table, then reads the
      task status based on the task id.  Returns task status if available,
      otherwise returns PGS_UNKNOWN.

   Input: 
      prod_id - Product ID

   Output: 

   Returns: 
      Task status.  Possible status values are:
         PGS_UNKNOWN (also used to indicate NORMAL)
         PGS_TASK_NOT_RUNNING/PGS_TASK_NOT_CONFIG
         PGS_TASK_FAILED

   Notes:
      prod_status.h lists macro definitions for the task status values
      used.

**************************************************************************/
Task_prod_chain* PSTS_is_gen_task_running( short prod_id, int *status ){

   char *task_name;

   /* Validate product ID.  If invalid, return status unknown. */
   if (prod_id < 0){

      LE_send_msg( GL_ERROR, "Invalid Product ID: %d\n", prod_id );
      *status = PGS_UNKNOWN;
      return( NULL );

   }

   task_name = ORPGPAT_get_gen_task((int) prod_id);
   if( task_name == NULL ){

      LE_send_msg( GL_ERROR, "Can Not Find Generating Task For Product ID %d\n",
                   prod_id );
      *status =  PGS_UNKNOWN;
      return( NULL );

   }

   return( PSTS_what_is_task_status( task_name, status ) );

/* END of PSTS_is_gen_task_running() */
}

/**************************************************************************
 Description: 
    Given the task id, return the task status (Product Generation Status).

 Input: 
    task_name - Task name

 Output: 

 Returns: 
    Returns task status.

 Notes:

**************************************************************************/
Task_prod_chain* PSTS_what_is_task_status( char *task_name, int *status ){

   Task_prod_chain *task;

   /* Find task in Task Product List. */
   if( (task = PSTS_find_task_in_task_prod_list( task_name )) == NULL ){

      *status = PGS_UNKNOWN;
      return( NULL );

   }

   *status = task->status;
   return( task );

/* END of PSTS_what_is_task_status() */
}

/*******************************************************************
   Description:
      This function sets the task status for task "task_name".

   Inputs:
      task_name - the task name find.
      int status - the new task status value.

   Returns:
      Returns PS_DEF_SUCCESS if operation successful, otherwise
      return PS_DEF_FAILED.

******************************************************************/  
int PSTS_set_task_prod_list_status( char *task_name, int status ){

   Task_prod_chain *task;
   
   /* Get pointer to the task in the Task Products List. */ 
   if( (task = PSTS_find_task_in_task_prod_list( task_name )) == NULL )
      return( PS_DEF_FAILED ); 
   
   /* Set the task status. */
   if( (task->status != PGS_TASK_FAILED)
                     &&
       (task->status != PGS_TASK_NOT_RUNNING) )
      task->status = status;

   return( PS_DEF_SUCCESS );

/* End of PSTS_set_task_prod_list_status( ) */
}

/*******************************************************************
   Description:
      This function performs a binary search of the Task Products 
      table for a match on task ID.  If a match found, returns
      pointer to task entry.

   Inputs:
      task_name - the task name to find.

   Returns:
      Returns pointer to task entry if operation successful, 
      otherwise returns NULL.

******************************************************************/  
Task_prod_chain* PSTS_find_task_in_task_prod_list( char *task_name ){

   int top, middle, bottom;
   int task_prod_list_size = 0;
   Task_prod_chain *task_prod_list;
   
   
   /* Get pointer to and size of Task Products List. */ 
   task_prod_list = PSPTT_get_task_prod_list( &task_prod_list_size ); 
   
   /* Set the list top and bottom bounds. */
   top = task_prod_list_size - 1;
   if( top == -1 )
      return NULL;

   bottom = 0;

   /* Do Until task_name found or task_name not in list. */
   while( top > bottom ){

      middle = (top + bottom)/2;
      if( strcmp( task_name, (task_prod_list + middle)->task_name ) > 0 )
         bottom = middle + 1;
      
      else if( strcmp( task_name, (task_prod_list + middle)->task_name ) == 0 )
         return ( task_prod_list + middle );

      else
         top = middle;


   }

   /* Modify the task status. */
   if( strcmp( (task_prod_list + top)->task_name, task_name ) == 0 )
      return ( task_prod_list + top );

   else
      return NULL; 

/* End of PSTS_find_task_in_task_prod_list( ) */
}

/*** Private Functions Start Below ...  ***/

/*****************************************************************
   Description:
      Report the products (codes) which cannot be
      generated because of a faulted or non-running task. 

   Inputs:

   Outputs:

   Returns:
      There is no return value for this function.

******************************************************************/
static void Report_prod_status(){

   Task_prod_chain *task_prod_list = NULL, *task_entry = NULL;
   Prod_wth_only_id_next *gen_prod_list = NULL;
   Task_prod_wth_only_id_next *dep_task_prods = NULL, temp;
   int list_index, status, task_prod_list_size = 0;

   /* Get address of the Task Product List. */
   task_prod_list = PSPTT_get_task_prod_list( &task_prod_list_size );
   if( task_prod_list == NULL ){

      LE_send_msg( GL_ERROR, 
                   "PSTS_report_prod_status: Task Product List Is NULL\n" );
      return;

   }

   /* Do For Each task in the Task Products List. */
   list_index = 0;
   while( list_index < task_prod_list_size ){

      status = (task_prod_list + list_index)->status;

      switch( status ){

         /* Tasks not running. */
         case PGS_TASK_NOT_CONFIG:
         case PGS_TASK_FAILED:
         {

            /* If task not running and status has not been reported,
               add all products with product codes to list of unavailable
               products. */
            if( !((task_prod_list + list_index)->reported) ){

               /* Get the list of products which can not be generated 
                  because this task failed or not configured. */
               dep_task_prods =
                  PSPTT_find_entry_in_depend_prod_list( (task_prod_list + list_index)->task_name );
               if( dep_task_prods != NULL )
                  Add_to_prod_list( &Unavailable_list, dep_task_prods );

               /* Set reported flag so this task doesn't get reported again. */
               (task_prod_list + list_index)->reported = 1;

               /* Set the flag indicting the task status has updated. */
               Psg_task_status_changed = 1;

            }
            break;

         }

         /* Tasks which are running. */
         default:
         {
            if( ((task_prod_list + list_index)->reported) ){

               /* Get the list of products which can now be generated 
                  because this task which had not been running is now restored. */
               dep_task_prods =
                  PSPTT_find_entry_in_depend_prod_list( (task_prod_list + list_index)->task_name );
               while( dep_task_prods != NULL ){

                  memcpy( &temp, dep_task_prods, sizeof( Task_prod_wth_only_id_next ) );
                  temp.next = NULL;

                  /* Do not report products which are disabled. */
                  task_entry = PSTS_find_task_in_task_prod_list( dep_task_prods->gen_task );
                  if( task_entry != NULL ){

                     gen_prod_list = task_entry->gen_prod_list;
                     while( gen_prod_list != NULL ){
                  
                        if( gen_prod_list->prod_id == dep_task_prods->prod_id ){

                           /* Product is currently enabled so add to the list. */
                           if( gen_prod_list->schedule == PRODUCT_ENABLED ){

                              Add_to_prod_list( &Available_list, &temp );
                              break;

                           } /* End of if( gen_prod_list->schedule .... */

                        } /* End of if( gen_prod_list->prod_id ... */

                        gen_prod_list = gen_prod_list->next;

                     } /* End of while( gen_prod_list .... */

                  }

                  dep_task_prods = dep_task_prods->next;

               } /* End of while( dep_task_prods .... */

               /* Set reported flag so this task doesn't get reported again. */
               (task_prod_list + list_index)->reported = 0;

               /* Set the flag indicting the task status has updated. */
               Psg_task_status_changed = 1;

            }
            break;

         }

      /* End of "switch" */
      }

      /* Add the products which have been disabled. */
      if( Psg_product_scheduling_changed ){

         /* Only report status if task is configured and running ..... */
         if( ((task_prod_list + list_index)->status != PGS_TASK_NOT_CONFIG)
                                     &&
             ((task_prod_list + list_index)->status != PGS_TASK_FAILED) ){

            gen_prod_list = (task_prod_list + list_index)->gen_prod_list;

            while( gen_prod_list != NULL ){

               status = gen_prod_list->schedule;
               switch( status ){

                  case PRODUCT_DISABLED:
                  {

                     if( !gen_prod_list->reported ){

                        /* If product DISABLED and status has not been reported, 
                           add all products with product codes to list of unavailable
                           products. */
                        Task_prod_wth_only_id_next dep_task_prods;

                        dep_task_prods.prod_id = gen_prod_list->prod_id;
                        dep_task_prods.next = NULL;

                        Add_to_prod_list( &Unavailable_list, &dep_task_prods );

                        /* Set reported flag so this task doesn't get reported again. */
                        gen_prod_list->reported = 1;

                     } /* End of if( !gen_prod_list->reported .... */

                     break;
                  } /* End of case PRODUCT_ENABLED: */

                  case PRODUCT_ENABLED:
                  {

                     if( gen_prod_list->reported ){

                        /* If product DISABLED and status has not been reported, 
                           add all products with product codes to list of unavailable
                           products. */
                        Task_prod_wth_only_id_next dep_task_prods;

                        dep_task_prods.prod_id = gen_prod_list->prod_id;
                        dep_task_prods.next = NULL;

                        Add_to_prod_list( &Available_list, &dep_task_prods );

                        /* Set reported flag so this task doesn't get reported again. */
                        gen_prod_list->reported = 0;

                     } /* End of if( gen_prod_list->reported .... */

                     break;
                  } /* End of case PRODUCT_DISABLED: */

               } /* End of switch( status ) */

               gen_prod_list = gen_prod_list->next;

            } /* End of while( gen_prod_list != NULL ) */

         } /* End of if( ((task_prod_list + list_index)->status != .... */

      } /* End of if( Psg_product_scheduling_changed ) */

      /* Go to next task in the list. */
      list_index++;

   /* End of "while" loop. */
   }

   /* Reset flag so next pass only report products whose scheduling changed. */
   Psg_product_scheduling_changed = 0;

   /* Free the Available and Unavailable product lists. */
   if( Unavailable_list != NULL ){

      Format_product_status_msg( Unavailable_list, PGS_TASK_FAILED );
      Free_prod_list( &Unavailable_list );
      
   }

   if( Available_list != NULL ){

      Format_product_status_msg( Available_list, PGS_UNKNOWN );
      Free_prod_list( &Available_list );
      
   }

/* End of PSTS_report_prod_status() */
}

/******************************************************************
   Description:
      Given the task status returned from the ORPGSTAT functions,
      convert into a Product Generation Status.

   Inputs:
      status - task status from ORPGSTAT function call.

   Outputs:

   Returns:
      Product Generation Status.

******************************************************************/
static int Set_internal_task_status( int status ){

   /* Set the orpgstat task status to ps_routine internal value. */
   switch( status ){

      case MRPG_PS_NOT_STARTED:
         return (PGS_TASK_NOT_CONFIG);

      case MRPG_PS_ACTIVE:
      case MRPG_PS_STARTED:
         return (PGS_UNKNOWN);

      case MRPG_PS_FAILED:
         return (PGS_TASK_FAILED);

      default:
         return (PGS_UNKNOWN);

   /* End of "switch" */
   }

/* Set_internal_task_status( ) */
}

/*******************************************************************

   Description:
      Given "list", format system status messages depending on 
      "status".  If status indicates unavailable, list unavailable
      products.  If status indicates available, list available
      products.

   Inputs:
      list - list of product codes.
      status - indicates available or unavailable at the current
               time.

   Outputs:

   Returns:
      There is no return value define for this function.

   Notes:
      Assumes product codes are not more than 3 digits.

      The system status messages are written in reverse order so that
      they appear in the status log in the correct order.

*******************************************************************/
static void Format_product_status_msg( Prod_wth_only_id_next *list, 
                                       int status ){

   Text_t *text, *new_text;
   char char_code[6]; 
   int prod_code, products_reported;

   /* Allocate buffer to store system status message. */
   text = malloc( sizeof(Text_t) );
   if( text == NULL )
      return;

   text->str = calloc( 1, 65 );
   if( text->str == NULL )
      return;

   text->next = NULL;

   /* Do for all products in the "list". */
   products_reported = 0;
   while( list != NULL ){

      products_reported = 1;

      if( (strlen( text->str ) + 5) >= 65 ){

         /* Allocate buffer to store system status message. */
         new_text = malloc( sizeof(Text_t) );
         if( new_text == NULL )
            break;

         new_text->str = calloc( 1, 65 );
         if( new_text->str == NULL )
            break;

         new_text->next = text;
         text = new_text;

      }
  
      prod_code = ORPGPAT_get_code( list->prod_id );
      sprintf( char_code, "%5d", prod_code );   
      strcat( text->str, char_code );

      list = list->next;

   } 

   /* Do for all messages. */
   while( text != NULL ){

      /* Write system status message if there are products still to report. */
      if( (text->str != NULL) && (strlen( text->str ) != 0) ){

         if( (status == PGS_TASK_NOT_CONFIG)
                     ||
             (status == PGS_TASK_FAILED) )
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "%s\n", text->str );

         else
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", text->str );

      }

      /* Go to next text string ... free the previous string. */
      new_text = text;
      text = text->next;
      if( new_text->str != NULL )
         free( new_text->str );

      free( new_text );

   /* End of "while" loop. */
   } 

   /* If there are available or unavailable products, write header message. */
   if( products_reported ){

      if( (status == PGS_TASK_NOT_CONFIG)
                  ||
          (status == PGS_TASK_FAILED) )
         LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "The Following Product Codes Are Unavailable:\n" );

      else
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "The Following Product Codes Are Now Available:\n" );

   }

/* End of Format_product_status_msg( ) */
}

/*******************************************************************

   Description:
      Adds dependent product codes to "list".

   Inputs:
      list - list which contains dependent product codes.
      dep_task_prods - list containing all dependent products.

   Outputs:

   Returns:

   Notes:

*******************************************************************/
static void Add_to_prod_list( Prod_wth_only_id_next **list,
                              Task_prod_wth_only_id_next *dep_task_prods ){


   Task_prod_wth_only_id_next *node = dep_task_prods;

   /* Do for all products in the "list" */
   while( node != NULL ){

      /* Check if product has a product code and is not of TYPE_ON_DEMAND.  
         If it does have a product code and is of TYPE_ON_DEMAND, the 
         product ID must be either ALRTMSG or ALRTPROD. */
      if( ORPGPAT_get_code( node->prod_id ) > 0 ){

         if( (ORPGPAT_get_type( node->prod_id ) != TYPE_ON_DEMAND) 
                                ||
             ((ORPGPAT_get_type( node->prod_id ) == TYPE_ON_DEMAND) 
                                  &&
              ((node->prod_id == ALRTMSG) || (node->prod_id == ALRTPROD))) ){

            /* If list is currently empty, allocate first node of the 
               list. */
            if( *list == NULL ){

               *list = malloc( sizeof( Prod_wth_only_id_next ) );
               if( *list == NULL )
                  return;

               /* Add the product to the list. */
               (*list)->prod_id = node->prod_id;
               (*list)->next = NULL;

            }
            else{

               Prod_wth_only_id_next *list_node = *list;
               Prod_wth_only_id_next *prev_list_node = *list;

               /* List is not empty ... put product at end of the list. */
               while( list_node != NULL ){

                  /* Do not report duplicate product codes. */
                  if( node->prod_id == list_node->prod_id )
                     break;

                  prev_list_node = list_node;
                  list_node = list_node->next;

               }

               /* Add the product to the end of the list. */
               if( list_node == NULL ){

                  list_node = malloc( sizeof( Prod_wth_only_id_next ) );
                  if( list_node == NULL )
                     return;

                  list_node->prod_id = node->prod_id;
                  list_node->next = NULL;
                  prev_list_node->next = list_node;

               }

            }

         } 

      }

      node = node->next;

   /* End of "while" loop. */
   } 

/* End of Add_to_prod_list() */
}

/*******************************************************************

   Description:
      Frees storage allocated to "list".

   Inputs:
      list - linked list to be freed.

   Outputs:

   Returns:

   Notes:

*******************************************************************/
static void Free_prod_list( Prod_wth_only_id_next **list ){

   Prod_wth_only_id_next *this_node, *next_node;

   this_node = *list;
   while( this_node != NULL ){

      next_node = this_node->next;
      free( this_node );
      this_node = next_node;

   /* End of "while" loop. */
   }

   *list = NULL;

/* End of Free_prod_list() */
}

/**************************************************************************

   Description:
      Decides if the product should be scheduled based on:

         1) Disabled flag in PAT
         2) wxmode flag in PAT
         3) Product scheduling information in adaptation data.  

      For scheduling information in adaptation data, products can be 
      disabled by site, time of day, day of year, VCP, weather mode or
      latitude/longitude.

   Inputs:
      prod_id - Product ID
      task_name - Task name

   Returns:
      0 - if product is disabled.
      1 - if product is enabled.
   
**************************************************************************/
static int Check_scheduling_info( prod_id_t prod_id, char *task_name ){

   char str[64], *buf;
   double values[20];
   int disabled, current, i, len, attr, yr, mon, day, hr, min, sec, ret;
   time_t current_time;

   int num_vals = 0;

   /* First check the PAT to see if the product has been disabled. */
   if( (disabled = ORPGPAT_get_disabled( prod_id )) > 0 )
         return PRODUCT_DISABLED;

   /* Next check if the product is disabled by weather mode. */
   if( (disabled = Prod_valid_for_wx_mode( prod_id )) != PS_DEF_SUCCESS )
      return disabled;

   /* Last, check the product scheduling information in adaptation data. */
   sprintf( str, "%s.%s.", PROD_SCHED_KEY, task_name );
   attr = strlen( str );

   /* Check if task has products disabled. */
   sprintf( &str[ attr ], PROD_SCHED_DISABLED );
   if( (ret = DEAU_get_values( str, &values[0], 1 )) > 0 ){

      if( (int) values[0] == PRODUCT_ENABLED )
         return PRODUCT_ENABLED;

   }
   else
      return PRODUCT_ENABLED;

   /* If here, then the disabled attribute must be TRUE.  If no products
      specified, assume all products are disabled. */
   sprintf( &str[ attr ], PROD_SCHED_PROD_ID );
   num_vals = DEAU_get_number_of_values( str );
   if( num_vals > 0 ){

      if( (ret = DEAU_get_values( str, &values[0], num_vals )) > 0 ){

         /* Do For All values looking for wildcard or match on Product ID. */
         for( i = 0; i < num_vals; i++ ){

            /* Assume product should be generated if product ID does not match. */
            if( (values[i] == -1) || ((int) values[i] == prod_id) )
               break;

         } /* End of for( i = 0; ..... ) */

         /* If no match on product ID and product ID not wildcard, schedule
            product. */
         if( i >= num_vals )
            return PRODUCT_ENABLED;

      } /* End of if( ret = DEAU( ..... ) > 0 ) */

   } /* End of if( num_vals > 0 ) */

   /* How the following logic works ...............
  
      If PROD_SCHED_BY_VCP, PROD_SCHED_BY_WXMODE, PROD_SCHED_TIME_OF_DAY, 
      PROD_SCHED_DAY_OF_YEAR, PROD_SCHED_BY_LOCATION or PROD_SCHED_BY_ICAO
      are defined, then all the criteria that are defined for the product 
      have to be satisfied.  

      Example:  If PROD_SCHED_BY_VCP has value 21 and PROD_SCHED_DAY_OF_YEAR
                has values 0321, 0921, then product(s) is (are) disabled
                when executing VCP 21 and the day of year is in interval
                [ March, 21, September, 21]. */

   /* Check if product disabled for ICAO. */
   sprintf( &str[ attr ], PROD_SCHED_FOR_ICAO );
   num_vals = DEAU_get_number_of_values( str );
   if( num_vals > 0 ){

      if( (ICAO != NULL)
               &&
          (DEAU_get_string_values( str, &buf ) > 0) ){

         for( i = 0; i < num_vals; i++ ){

            len = strlen( buf );
            if( strncmp( ICAO, buf, strlen(ICAO)) == 0 )
               break;

            buf += (len+1);

         }

         /* If no match on ICAO, schedule product. */
         if( i >= num_vals )
            return PRODUCT_ENABLED;

      }

   }

   /* Check if product disabled for VCP. */
   sprintf( &str[ attr ], PROD_SCHED_BY_VCP );
   num_vals = DEAU_get_number_of_values( str );
   if( num_vals > 0 ){

      current = RRS_get_current_vcp_num();
      if( (ret = DEAU_get_values( str, &values[0], num_vals )) > 0 ){

         for( i = 0; i < num_vals; i++ ){

            if( values[i] == current )
               break;

         }

         /* If no match on VCP, schedule product. */
         if( i >= num_vals )
            return PRODUCT_ENABLED;

      }
   
   }

   /* Check if product disabled for Weather Mode. */
   sprintf( &str[ attr ], PROD_SCHED_BY_WXMODE );
   num_vals = DEAU_get_number_of_values( str );
   if( num_vals > 0 ){

      current = RRS_get_current_weather_mode();
      if( (ret = DEAU_get_values( str, &values[0], num_vals )) > 0 ){

         for( i = 0; i < num_vals; i++ ){

            if( values[i] == current )
               break;

         }

         /* If no match on weather mode, schedule product. */
         if( i >= num_vals )
            return PRODUCT_ENABLED;

      }

   }

   /* Check if product disabled for time of day. */
   sprintf( &str[ attr ], PROD_SCHED_TIME_OF_DAY );
   if( (ret = DEAU_get_values( str, &values[0], 2 )) > 0 ){

      current_time = time(NULL);
      unix_time( &current_time, &yr, &mon, &day, &hr, &min, &sec ); 
      current_time = hr*100 + min;

      if( values[0] < values[1] ){

         if( (current_time < (int) values[0]) || (current_time > values[1]) )
            return PRODUCT_ENABLED;

      }
      else{

         if( (current_time > (int) values[0]) || (current_time < values[1]) )
            return PRODUCT_ENABLED; 

      } /* End of if( values[0] < values[1] ) */

   }
    
   /* Check if product disabled for day of year. */
   sprintf( &str[ attr ], PROD_SCHED_DAY_OF_YEAR );
   if( (ret = DEAU_get_values( str, &values[0], 2 )) > 0 ){

      current_time = mon*100 + day;

      if( values[0] < values[1] ){

         if( (current_time < (int) values[0]) || (current_time > (int) values[1]) )
            return PRODUCT_ENABLED;

      }
      else{

         if( (current_time > (int) values[0]) || (current_time < values[1]) )
            return PRODUCT_ENABLED;

      } /* End of if( values[0] < values[1] ) */

   }

   /*Check if product disabled by latitude and longitude. */
   sprintf( &str[ attr ], PROD_SCHED_BY_LOCATION );
   if( (ret = DEAU_get_values( str, &values[0], 4 )) > 0 ){
 
      if( (RDA_latitude < values[0] || RDA_latitude > values[1])
                                  || 
          (RDA_longitude < values[2] || RDA_longitude > values[3]) )
      return PRODUCT_ENABLED;

   }

   return PRODUCT_DISABLED;

/* End of Check_scheduling_info() */
}

/**************************************************************************
   Description: 
     This module searches the product attribute table for a match on 
     product ID.  If a match is found, the weather mode attribute is
     checked to see if this product should be generated for the
     current weather mode.  

     If the product can be scheduled for current weather mode, 
     PS_DEF_SUCCESS is returned.  If product should not be scheduled
     for current weather, PGS_INAPPR_WX_MODE is returned.  If product
     not found in product attributes table, PS_DEF_FAILED is returned.

   Input: 
      prod_id - the product ID or buffer number.

   Output: 

   Returns: 
      PS_DEF_SUCCESS, PGS_INAPPR_WX_MODE, or PS_DEF_FAILED.  Reasons 
      are described above.

   Notes:

**************************************************************************/
static unsigned int Prod_valid_for_wx_mode(int prod_id){

   int current_weather_mode;
   int wx_modes_attr;

   /* Get the current weather mode. */
   current_weather_mode = RRS_get_current_weather_mode();

   /* Get weather modes attribute from product attribute table. */
   wx_modes_attr = ORPGPAT_get_wx_modes( prod_id );

   /* If wx_modes_attr is valid, then check if product is valid for 
      weather mode. */
   if( wx_modes_attr > 0 ){

      if( Wx_mode_matches_wx_mode_attr ( current_weather_mode,
                                        wx_modes_attr ) == PS_DEF_FAILED ){

        /* Product not allowed for this weather mode. */
        return( PGS_INAPPR_WX_MODE );
         
      }

      /* Product allowed for this weather mode. */ 
      return( PS_DEF_SUCCESS );

   }

   /* This is the case where the product is not found in the product
      attributes list. */
   return( PS_DEF_FAILED );

/* END of Prod_valid_for_wx_mode() */
}

/**************************************************************************
   Description:

      Checks if specified weather mode matches weather mode attribute 
      for product
  
      In ATTR ( format )
         0 0 0 1 : weather mode 0  Clear Air
         0 0 1 0 : weather mode 1  Precipitation
         0 1 0 0 : weather mode 2  Maintenance

      In ps_routine ( format )
         0 : weather mode 0  Clear Air
         1 : weather mode 1  Precipitation
         2 : weather mode 2  Maintenance

   Input: 

      wx_mode - input weather mode
      wx_mode_attr - weather mode attribute for product.

   Output: 

   Returns: 

      PS_DEF_SUCCESS if product weather mode attribute matches specified
      weather mode, or PS_DEF_FAILED otherwise.

   Notes:

***************************************************************************/
static int Wx_mode_matches_wx_mode_attr( int weather_mode, int wx_mode_attr ){

  /* check if weather mode matches product attribute */
   if( weather_mode == CLEAR_AIR_MODE ){

      /* Weather mode is CLEAR AIR */
      if( (wx_mode_attr & 2 ) != 0)
         return( PS_DEF_SUCCESS );
      
      else
         return( PS_DEF_FAILED );

   }
   else if( weather_mode == PRECIPITATION_MODE ){

      /* Weather mode is PRECIPITATION */
      if( (wx_mode_attr & 4) != 0)
         return( PS_DEF_SUCCESS );

      else
         return( PS_DEF_FAILED );
       
   }

   /* If fall through, unknown weather mode. */
   return( PS_DEF_FAILED );

/* END of Wx_mode_matches_wx_mode_attr() */
}
