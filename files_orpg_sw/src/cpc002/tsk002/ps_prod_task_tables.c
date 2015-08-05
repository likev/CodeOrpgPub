/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/12 17:39:18 $
 * $Id: ps_prod_task_tables.c,v 1.39 2014/03/12 17:39:18 steves Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */

/* System Include Files/Local Include Files */
#include <stdlib.h>
#include <string.h>

#include <gen_stat_msg.h>
#include <prod_request.h>
#include <ps_def.h>
#include <orpgalt.h>
#include <rpg_vcp.h>

#define PS_PROD_TASK_TABLES
#include <ps_globals.h>
#undef PS_PROD_TASK_TABLES

/* Constant Definitions/Macro Definitions/Type Definitions */

/* Static Globals */

/* Task Products List. */
static Task_prod_chain *Task_prod_list = NULL;    
static int Task_prod_list_size = 0;

/* Product Dependency List. 

   There is an entry for each product.   Each product entry starts a
   linked-list of Prod_wth_only_id_next nodes, each node representing
   a product that needs to be generated in order for the product to 
   be generated.   Optional products are not included in this list. */
static Prod_depend_chain *Prod_depend_list = NULL; 
static int Prod_depend_list_size = 0; 

/* Dependent Products List. 

   There is an entry for each product.   Each product entry starts a
   linked-list of Task_prod_wth_only_id_next nodes, each node representing
   a product/task that depends on this product for generation.  */
static Depend_prod_chain *Depend_prod_list = NULL;
static int Depend_prod_list_size = 0;

/*
 * Static Function Prototypes
 */
static int Add_task_prod_list( Task_prod_chain **task_prod_list, char *task_name );
static int Free_prod_depend_list();
static int Free_task_prod_list();
static int Free_depend_prod_list();
static int Generate_task_prod_list(void);
static int Build_dep_list_of_this_prod( Prod_depend_chain *this_prod,
                                        prod_id_t prod_id );

static void Append_prod_to_dependency_list( Prod_depend_chain *this_prod, 
                                            prod_id_t prod_id );
static int Generate_dep_prod_list(void);
static int Build_dep_prod_list_for_task( Depend_prod_chain *this_task,
                                         prod_id_t prod_id );
static void Fill_prod_gen_table_entry( prod_id_t prod_id, 
                                       Pd_prod_gen_tbl_entry *aux_data );

/***** Public Functions Start Below ... *****/
/**************************************************************************
   Description: 
      Initialize all global static variables for this module.

   Input:
      None

   Output: 

   Returns: 
      There is no return value define for this module.

   Notes:

**************************************************************************/
void PSPTT_initialize(void){

   /* Initialize pointers to Task Products List, Product Dependency List
      and Dependent Products List to NULL. */
   Task_prod_list = NULL;
   Task_prod_list_size = 0;

   Prod_depend_list = NULL;
   Prod_depend_list_size = 0;

   Depend_prod_list = NULL;
   Depend_prod_list_size = 0;

/* END of PSPTT_initialize() */
}

/**************************************************************************
   Description:
      Activated by Product Attribute Table change.  This module performs
      the following actions:

        1.  Frees space allocated to the Product Dependency List, 
            Task Product List, and Dependent Products List.
        2.  Checks if the number of products in the Product Attributes
            Table (PAT) is > 0.  If not, ps_routine exits.
        3.  Calls Generate_task_prod_list function to generate the 
            Task_prod_list and the Prod_depend_list.
        4.  Calls Generate_dep_prod_list function to generate the 
            Depend_prod_list. 

   Input: 
      None.

   Output:
      None

   Returns: 
      There is no return value for this function.

   Notes:
      If the number of products in the Product Attributes List is <= 0,
      ORPGTASK_exit is called to "kill" this process.

**************************************************************************/
void PSPTT_build_prod_task_tables(void){

   int num_prod;

   /* If pointer to Product Dependency list not NULL, free memory allocated
      for it. */
   if( Prod_depend_list != NULL )
      Free_prod_depend_list();

   /* If pointer to Task Product list not NULL, free memory allocated
      for it. */
   if( Task_prod_list != NULL )
      Free_task_prod_list();

   /* If pointer to Dependent products list not NULL, free memory allocated
      for it. */
   if( Depend_prod_list != NULL )
      Free_depend_prod_list();

   /* Determine the number of products in the Product Attributes Table. If no 
      products in the list, aborting processing. */
   if( (num_prod = ORPGPAT_num_tbl_items()) <= 0 ){

      LE_send_msg( GL_ERROR, "Generate Product Attribute List Failed\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }
   else{

      /* Inform user about success of operation. */
      if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
         LE_send_msg( GL_INFO, 
                      "There Are %d Products In Product Attribute Table.\n", 
                      num_prod);
       
   }

   /* The following module generates the Task_prod_list and the 
      Prod_depend_list. */
   Generate_task_prod_list();

   /* The following module generates the Depend_prod_list. */
   Generate_dep_prod_list();

   /* The following module sets the task status field in the Task_prod_list. */
   PSTS_update_task_status();

/* END of PSPTT_build_prod_task_tables() */
}

/**************************************************************************
 Description:
    This module schedules those products which must be generated regardless
    of whether they are requested or not.  These products support the 
    alerting function and one-time requests.

 Inputs:

 Outputs:

 Returns:

 Notes:
    The basic products scheduled are assumed to be volume based with 
    no product dependent parameters.  If this ever becomes not true,
    then product dependent parameters will need to become available 
    (possibly from the Product Attribute Table) and this module will
    need to change.

***************************************************************************/
void PSPTT_schedule_basic_products(){

   int num_cats, index, num_prods, priority;
   prod_id_t prod_id;
   char *gen_task;

   /* Schedule all products (and those these products depend on)
      which are inputs to alerting. */
   gen_task = ORPGPAT_get_gen_task( ALRTMSG );
   if( gen_task != NULL ){

      Task_prod_chain *task;

      task = PSTS_find_task_in_task_prod_list( gen_task );

      /* If entry found, then ... */
      if( task != NULL ){

         Prod_wth_only_id_next *prod;

         prod = task->gen_prod_list;
         while( prod != NULL ){

            Pd_prod_gen_tbl_entry aux_data;

            /* Fill in the request fields. */
            Fill_prod_gen_table_entry( prod->prod_id, &aux_data );

            if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
               LE_send_msg( GL_INFO, 
                   "Alerting Forced Generation of Prod %d And Dependent Prods\n",
                   prod->prod_id );

            /* Schedule this product and all dependent products if not 
               already scheduled. */
            PSPTT_through_dep_list_of_this_prod( prod->prod_id, 
                                                 PS_DEF_FROM_DEFAULT,
                                                 0, (void *) &aux_data );

            /* Go to next product. */
            prod = prod->next;

         }

      }
      else
         LE_send_msg( GL_ERROR, 
                      "Unable To Find Alerting Task (%s) In Task Products List\n", gen_task );

   }
   else
      LE_send_msg( GL_ERROR, "Unable To Get Alerting Generating Task From PAT\n" );

   /* Schedule alert-paired products which can not be generated off replay
      and which do not have product dependent parameters. 

      Read alert-paired products. */
   if( (num_cats = ORPGALT_categories( )) > 0 ){

      int cat, prod_code;

      /* Do for each alert-paired product. */
      index = 0;
      while( index < num_cats ){ 

         /* Get the alert category. */
         if( (cat = ORPGALT_get_category( index )) != ORPGALT_INVALID_INDEX ){

            /* Get the alerted-paired product product code. */
            prod_code = ORPGALT_get_prod_code( cat );
            if( prod_code != ORPGALT_INVALID_CATEGORY ){

               Pd_prod_gen_tbl_entry aux_data;

               /* Need the product ID. */
               prod_id = ORPGPAT_get_prod_id_from_code( prod_code );
               if( prod_id != ORPGPAT_ERROR ){

                  /* Normal case .... */
                  if( prod_id > 0 ){

                     /* If alert-paired product does not have product dependent 
                        parameters, then schedule it. */
                     if( ORPGPAT_get_num_parameters( prod_id ) == 0 ){

                        /* Fill in request fields. */
                        Fill_prod_gen_table_entry( prod_id, &aux_data );

                        if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
                           LE_send_msg( GL_INFO, 
                              "Forced Generation of Alert-Paired Prod %d And Dependent Prods\n", 
                              prod_id );

                        /* Schedule this product and all dependent products if not 
                           already scheduled. */
                        PSPTT_through_dep_list_of_this_prod( prod_id, PS_DEF_FROM_DEFAULT,
                                                             0, (void *) &aux_data );

                     }

                  }
                  else{ 

                     /* This is the case where the product ID is negative.  The dependent
                        products lists the associated product codes. */
                     /* Check the number of dependent products. */
                     int i, pid, n_dep_prods = ORPGPAT_get_num_dep_prods( prod_id );
        
                     /* Loop through all the dependent products. */
                     for( i = 0; i < n_dep_prods; i++ ){

                        int p_code = ORPGPAT_get_dep_prod( prod_id, i ); 
                        if( p_code != ORPGPAT_ERROR ){

                           /* If alert-paired product does not have product dependent
                              parameters, then schedule it. */
                           if( ((pid = ORPGPAT_get_prod_id_from_code( p_code)) != ORPGPAT_ERROR) 
                                                        &&
                                (ORPGPAT_get_num_parameters( pid ) == 0) ){

                              /* Fill in request fields. */
                              Fill_prod_gen_table_entry( pid, &aux_data );

                              if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
                                 LE_send_msg( GL_INFO,
                                    "Forced Generation of Alert-Paired Prod %d And Dependent Prods\n",
                                    prod_id );

                              /* Schedule this product and all dependent products if not
                                 already scheduled. */
                              PSPTT_through_dep_list_of_this_prod( pid, PS_DEF_FROM_DEFAULT,
                                                                   0, (void *) &aux_data );

                           }

                        }

                     }

                  }

               }
               else
                  LE_send_msg( GL_ERROR, "ORPGPAT_get_prod_id_from_code Failed (%d)\n",
                               prod_code );

            }
            else
               LE_send_msg( GL_ERROR, "ORPGALT_get_prod_code Failed For %d\n", cat );

         }
         else
            LE_send_msg( GL_ERROR, "ORPGALT_get_category Failed For %d\n", index );

         /* Go to next alert category. */
         index++;

      /* End of "while" loop. */
      }

   }

   /* Get number of products in the Product Attributes Table. */
   num_prods = ORPGPAT_num_tbl_items();

   /* Go through all products in the product attribute table and 
      schedule those products (and those these products dependent on)
      whose priority is the maximum priority. */
   index = 0;
   while (index < num_prods ){

      /* Get priority for next product in Product Attribute Table. */
      prod_id = ORPGPAT_get_prod_id( index );
      priority = ORPGPAT_get_priority( prod_id, ORPGVST_get_mode()+1 );

      if( priority == ORPGPAT_MAX_PRIORITY ){

         Pd_prod_gen_tbl_entry aux_data;
         int num_defined_params, el_parm, ind, j;

         /* Fill in request fields. */
         Fill_prod_gen_table_entry( prod_id, &aux_data );
 
         if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
            LE_send_msg( GL_INFO, 
               "Priority Forced Generation of Prod %d And Dependent Prods\n", prod_id );

         /* For forced-priority products, assign defaults from the PAT to 
            the defined product parameters. */
         num_defined_params = ORPGPAT_get_num_parameters( prod_id );           
         for (j = 0; j < num_defined_params; ++j){
      
            ind = ORPGPAT_get_parameter_index( prod_id, j );
            if( ind >= 0 ){
      
               /* If product is final product, get from parameters from PAT. */
               aux_data.params[ind] = ORPGPAT_get_parameter_default( (int) prod_id, j );
 
            }
 
         }
  
         /* Set elevation angle parameter, if defined for this product,
            to the elevation angle from the current VCP. */
         if( (el_parm = ORPGPAT_elevation_based((int) prod_id)) >= 0 ){
  
            int vcp, vs_num, num_elevs;
            short vcp_elev_inds[MAX_ELEVATION_CUTS];
            short vcp_elev_angles[MAX_ELEVATION_CUTS];

            /* Get the elevation angles associated with the elevation request. */
            vcp = RRS_get_current_vcp_num();
            vs_num = RRS_get_volume_num(NULL);
            if( (num_elevs = ORPGPRQ_get_requested_elevations( vcp, aux_data.params[el_parm],
                                                               (int) MAX_ELEVATION_CUTS,
                                                               vs_num, vcp_elev_angles,
                                                               vcp_elev_inds )) <= 0 ){

               /* If there was a problem getting elevation request information,
                  assign elevation based on closest elevation match to elevation
                  parameter. */
               int elev_ind, elev_angle;

               elev_ind = RRS_get_elevation_index( aux_data.params[el_parm], NULL );

               if( (elev_angle = RRS_get_elevation_angle( elev_ind )) >= 0 )
                  aux_data.params[el_parm] = elev_angle;
  
               else
                  aux_data.params[el_parm] = 0;
  
               aux_data.elev_index = elev_ind;

               /* Schedule this product and all dependent products if not 
                  already scheduled. */
               PSPTT_through_dep_list_of_this_prod( prod_id, PS_DEF_FROM_DEFAULT,
                                                    0, (void *) &aux_data );

            }
            else{

               int elev_ind;

               for( elev_ind = 0; elev_ind < num_elevs; elev_ind++ ){

                  /* Set the elevation angle and elevation index in the request. */
                  aux_data.params[el_parm] = vcp_elev_angles[elev_ind];
                  aux_data.elev_index = vcp_elev_inds[elev_ind];

                  if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
                     LE_send_msg( GL_INFO, "Forcing Generation of %d with Elev: %d\n",
                                  prod_id, aux_data.params[el_parm] );

                  /* Schedule this product and all dependent products if not 
                     already scheduled. */
                  PSPTT_through_dep_list_of_this_prod( prod_id, PS_DEF_FROM_DEFAULT,
                                                       0, (void *) &aux_data );

               }

            }
             
         }
         else{

            /* Schedule this product and all dependent products if not 
               already scheduled. */
            PSPTT_through_dep_list_of_this_prod( prod_id, PS_DEF_FROM_DEFAULT,
                                                 0, (void *) &aux_data );

         }

      }

      /* Prepare for next product in Product Attributes Table. */
      index++;

   /* End of "while" loop. */
   }

}

/**************************************************************************
 Description: 
    This module searches the Product Dependency List for "prod_id" and 
    schedules any dependent products.

 Input: 
    prod_id - product ID.
    source - where the request came from.
    line_id - line index for narrowband user if request from a narrowband
              user.
    aux_data - auxilliary data needed for the product scheduling.

 Output: 

 Returns:
    Return PS_DEF_SUCCESS if successful, or PD_DEF_FAILED if "prod_id" is 
    not found in the Product Dependency List.
 
 Notes:
    If Product Dependency List in undefined, report an error and terminate.

 **************************************************************************/
int PSPTT_through_dep_list_of_this_prod( prod_id_t prod_id, unsigned int source,
                                        unsigned int line_id, void *aux_data ){

   Prod_depend_chain *prod_list = NULL;
   Prod_wth_only_id_next *depend_list = NULL;

   int task_status;
   int prod_elev_based = 0;
   int type;
   char  *gen_task;

   short elevs[VCP_MAXN_CUTS], elev_inds[VCP_MAXN_CUTS];

   prod_list = Prod_depend_list;

   /* Make sure the product dependency list has been defined. */
   if( prod_list == NULL ){

      LE_send_msg( GL_ERROR, "Prod_depend_list == NULL\n" );
      ORPGTASK_exit( GL_ERROR );

   }

   /* Find a match on product id. */ 
   while (prod_list->prod_id != prod_id){

      prod_list = prod_list->next;
      if( prod_list == NULL ){

         LE_send_msg( GL_ERROR, "Product %d Not Found In Product Dependency List\n",
                      prod_id );
         return PS_DEF_FAILED;

      }

   /* End of "while" loop. */
   }

   /* This test should never fail (famous last words!). */
   if( prod_list->prod_id == prod_id ){

      short *params = NULL;
      short *elev_index = NULL;

      /* Check to see if the product is elevation based. */
      if( (type = ORPGPAT_get_type( (int) prod_id )) == TYPE_ELEVATION )
         prod_elev_based = 1;

      /* Schedule this product. */
      PD_schedule_this_prod( prod_id, source, line_id, aux_data );

      /* Check the status of the task just scheduled.   If not running owing to
         either task failure or task was not loaded/started, then do not schedule
         and products/tasks which this task depends on. */
      gen_task = ORPGPAT_get_gen_task(prod_id);
      PSTS_what_is_task_status( gen_task, &task_status );
      if( (task_status == PGS_TASK_FAILED) || (task_status == PGS_TASK_NOT_CONFIG) )
         return PS_DEF_SUCCESS;

      /* Check the product scheduling information.  If product disabled, do
         not schedule product or products which depend on it. */

      if( source == PS_DEF_FROM_DEFAULT ){

         Pd_prod_gen_tbl_entry *aux = (Pd_prod_gen_tbl_entry *) aux_data;
         params = &aux->params[0];
         elev_index = &aux->elev_index;

      }
      else{

         Prod_gen_status_pr_req *aux = (Prod_gen_status_pr_req *) aux_data;
         params = &aux->gen_status.params[0];
         elev_index = &aux->gen_status.elev_index;

      }

      /* Schedule all products for which this product depends */
      depend_list = prod_list->prod_depend_list;
      while( depend_list != NULL ){

         /* Check if the dependent product is elevation based. */
         if( ((type = ORPGPAT_get_type( (int) depend_list->prod_id )) == TYPE_ELEVATION )
                        &&
             (!prod_elev_based) ){
         
            int vcp, vs_num, ncuts, i;
            short save_param = -1, save_elev_index = -1;
            int elev_ind = ORPGPAT_elevation_based( depend_list->prod_id );

            /* The downstream product may be volume data with no elevation parameter,
               but the upstream product may be elevation data.  If this is the case, 
               then we assume the product will be generated for all elevations.  */
            if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
               LE_send_msg( GL_INFO, "Must Schedule All Elevations of %d on behalf of %d\n",
                            depend_list->prod_id, prod_id );
      
            vcp = RRS_get_current_vcp_num();
            vs_num = RRS_get_volume_num(NULL);
            ncuts = ORPGPRQ_get_requested_elevations( vcp, (int) ORPGPRQ_ALL_ELEVATIONS,
                                                      VCP_MAXN_CUTS, vs_num, elevs, 
                                                      elev_inds ); 

            /* Save the parameter and elevation index so it can later be restored. */
            if( elev_ind >= 0 )
               save_param = params[elev_ind];
            save_elev_index = *elev_index;

            /* Do For All elevation cuts in the VCP. */
            for( i = 0; i < ncuts; i++ ){

               if( elev_ind >= 0 )
                  params[elev_ind] = elevs[i];
               *elev_index = elev_inds[i];

               /* Schedule product in dependency list. */
               PD_schedule_this_prod( depend_list->prod_id, source, 
                                      line_id, aux_data );

            }

            /* Restored the saved parameter and elevation index. */
            if( elev_ind >= 0 )
               params[elev_ind] = save_param;
            *elev_index = save_elev_index;
                
         }
         else{

            short save_elev_index = *elev_index;

            /* If the data type is not TYPE_ELEVATION, ensure the elevation index
               is set to all elevations.  In some cases, the downstream product might
               be elevation-based but a dependent product is not. */

            if( type != TYPE_ELEVATION )
               *elev_index = REQ_ALL_ELEVS; 

            /* Schedule product in dependency list. */
            PD_schedule_this_prod( depend_list->prod_id, source, 
                                   line_id, aux_data );

            /* Restore the elevation index. */
            *elev_index = save_elev_index;

         }

         depend_list = depend_list->next;

      /* End of "while" loop. */
      }

      return PS_DEF_SUCCESS;

   }
   else{

      LE_send_msg( GL_ERROR, 
                   "Product %d Not Found In Product Dependency List\n",
                   prod_id );
      return PS_DEF_FAILED;

   }


/* END of PSPTT_through_dep_list_of_this_prod() */
}

/*********************************************************************
   Description:
      Return address of Task_prod_list.
 
   Inputs:
      size - pointer to int to receive size of Task Products List.

   Outputs:
      size - contains the number of entries in Task Products List.

   Returns:
      Returns address of Task_prod_list or NULL if list is not
      defined.

*********************************************************************/
Task_prod_chain* PSPTT_get_task_prod_list( int *size ){

   *size = Task_prod_list_size;
   return( Task_prod_list );

}

/*********************************************************************
   Description:
      Return address of Depend_prod_list.

   Inputs:
      size - pointer to int to receive size of Dependent Products List.

   Outputs:
      size - contains the number of entries in Dependent Products List.

   Returns:
      Returns address of Depend_prod_list or NULL if list is not
      defined.

*********************************************************************/
Depend_prod_chain* PSPTT_get_depend_prod_list( int *size ){

   *size = Depend_prod_list_size;
   return( Depend_prod_list );

}

/*********************************************************************
   Description:
      Return address of Prod_depend_list.

   Inputs:
      size - pointer to int to receive size of Product Dependency List.

   Outputs:
      size - contains the number of entries in Product Dependency List.

   Returns:
      Returns address of Prod_depend_list or NULL if list is not
      defined.

*********************************************************************/
Prod_depend_chain* PSPTT_get_prod_depend_list( int *size ){

   *size = Prod_depend_list_size;
   return( Prod_depend_list );

}

/*******************************************************************
   Description:
      This function performs a binary search of the Dependent 
      Products List for a match on task ID.  

   Inputs:
      task_name - the task name to find.

   Returns:
      Returns pointer to dependent products list for this task ID
      if task ID found in the list.  Otherwise, returns NULL;

   Notes:
      If Dependent Products List is empty, process terminates.

******************************************************************/  
Task_prod_wth_only_id_next* 
     PSPTT_find_entry_in_depend_prod_list( char *task_name ){

   int top, middle, bottom;
   
   /* Set the list top and bottom bounds. */
   top = Depend_prod_list_size - 1;
   if( top == -1 ){
     
      LE_send_msg( GL_ERROR, "Dependent Products List Is Empty\n" );
      ORPGTASK_exit( GL_ERROR );

   }

   bottom = 0;

   /* Do Until task_name found or task_name not in list. */
   while( top > bottom ){

      middle = (top + bottom)/2;
      if( strcmp( task_name, (char *) &((Depend_prod_list + middle)->task_name[0]) ) > 0 )
         bottom = middle + 1;
      else if( strcmp( task_name, (char *) &((Depend_prod_list+middle)->task_name[0]) ) == 0 )
         return( (Depend_prod_list + middle)->depend_prod_list );
      else
         top = middle;

   /* End of "while" loop. */
   }

   if( strcmp( (char *) &((Depend_prod_list + top)->task_name[0]), task_name ) == 0 )
      return( (Depend_prod_list + top)->depend_prod_list );

   else
      return NULL; 

/* End of PSPTT_find_entry_in_depend_prod_list( ) */
}

/*****************************************************************

   Description:
      This modules writes the Task_prod_chain to the ps_routine
      log file in human-readable format.
   Inputs:

   Outputs:

   Returns:
      There is no return value define for this function.

   Notes:
      This function should only be called at very high verbosity
      level.  It generates alot of output.  It is intended to be
      used mainly for debugging.

*****************************************************************/
void PSPTT_dump_task_prod_list(){

   Task_prod_chain *current_task;
   Prod_wth_only_id_next *current_prod;
   int list_index;

   LE_send_msg( GL_INFO, "....... Task_prod_chain ........\n" );
   
   /* Get the task product list. */
   current_task = Task_prod_list;

   /* Do For each Task in the task product list. */
   list_index = 0;
   while( list_index < Task_prod_list_size ){
   
      LE_send_msg( GL_INFO, "Task: %s\n", current_task->task_name );
      LE_send_msg( GL_INFO, "   >>>IDs of Products Generated By This Task\n" );
      current_prod = current_task->gen_prod_list;

      /* List products generated by this task. */
      while( current_prod != NULL ){

         LE_send_msg( GL_INFO, "      %d\n", current_prod->prod_id );
         current_prod = current_prod->next;

      /* End of inner "while" loop. */
      }
   
      LE_send_msg( GL_INFO, "   <<<IDs of Products Needed By This Task\n" );
      current_prod = current_task->support_prod_list;

      /* List products directly used by this task. */
      while( current_prod != NULL ){

         LE_send_msg( GL_INFO, "      %d\n", current_prod->prod_id );
         current_prod = current_prod->next;

      /* End of inner "while" loop */
      }
   
      current_task++;
      list_index++;
  
   /* End of outer "while" loop. */
   }

/* End of PSPTT_dump_task_prod_list() */
}

/*****************************************************************

   Description:
      Dumps the product dependency list in human-readable 
      format.  The product dependency list contains for each
      product those products which must be generated in 
      for the product to be generated.

   Inputs:

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes: 
      This function should only be called at very high verbosity
      level.  It generates alot of output.  It is intended to be
      used mainly for debugging.
*****************************************************************/
void PSPTT_dump_prod_depend_list(){

   Prod_depend_chain *current_prod;
   Prod_wth_only_id_next *prod_depend;

   LE_send_msg( GL_INFO, "....... Prod_depend_list ........\n" );
   
   current_prod = Prod_depend_list;
   while( current_prod != NULL ){
   
      LE_send_msg( GL_INFO, "Prod ID: %d\n", current_prod->prod_id );
  
      LE_send_msg( GL_INFO, "   >>>IDs of Products Required For This Product\n" );
      prod_depend = current_prod->prod_depend_list;
      while( prod_depend != NULL ){

            LE_send_msg( GL_INFO, "      %d\n", prod_depend->prod_id );
            prod_depend = prod_depend->next;

      /* End of inner "while" loop. */
      }
   
      current_prod = current_prod->next;
  
   /* End of outer "while" loop. */
   }

/* End of PSPTT_dump_prod_depend_list() */
}

/*************************************************************************

   Description:
      Dumps the Dependent product list in human-readable 
      format.  The dependent product list contains for each
      task those products which cannot be generated if the
      task in question does not execute.

   Inputs:

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes: 
      This function should only be called at very high verbosity
      level.  It generates alot of output.  It is intended to be
      used mainly for debugging.

*************************************************************************/
void PSPTT_dump_depend_prod_list(){

   Depend_prod_chain *current_task;
   Task_prod_wth_only_id_next *depend_prod;
   int list_index;

   LE_send_msg( GL_INFO, "....... Depend_prod_list ........\n" );
   
   current_task = Depend_prod_list;
   list_index = 0;
   while( list_index < Depend_prod_list_size ){
   
      LE_send_msg( GL_INFO, "Task ID: %s\n", current_task->task_name );
  
      LE_send_msg( GL_INFO,
          "   >>>IDs of Products (Gen Tasks) Dependent On This Task\n" );
      depend_prod = current_task->depend_prod_list;
      while( depend_prod != NULL ){

            if( depend_prod->mand_or_opt == OPTIONAL )
               LE_send_msg( GL_INFO, "      %6d (opt: T, gen_task: %s)\n", 
                            depend_prod->prod_id, depend_prod->gen_task );

            else
               LE_send_msg( GL_INFO, "      %6d (opt: F, gen_task: %s)\n", 
                            depend_prod->prod_id, depend_prod->gen_task );

            depend_prod = depend_prod->next;

      /* End of inner "while" loop. */
      }
   
      current_task++;
      list_index++;
  
   /* End of outer "while" loop. */
   }

/* End of PSPTT_dump_depend_prod_list() */
}

/* Private Functions Start Below ... */

/**************************************************************************

   Description: 
      The modules checks the task product list for a match on task_name.  If
      match, simply return.  If no match, reads the task attribute table 
      for entry associated with task_name.  The task attribute list input data
      and output data are used to fill the new task product list entry.

   Input: 
      task_prod_list - pointer to pointer to Task_prod_chain structure.
                       This is the task product list.
      task_name - The task's name

   Output: 

   Returns: 
      PS_DEF_SUCCESS if entry already in task product list associated with
      task_name or if entry is successfully entered into the task product
      list.  PS_DEF_FAILED is returned is taskid is not found in the
      task attributes table. 

   Notes:
      If memory allocation of task product list entry fails, this process
      exits.  

**************************************************************************/
static int Add_task_prod_list( Task_prod_chain **task_prod_list, 
                               char *task_name ){

   Task_prod_chain *tmp_task = NULL, *prev_task = NULL, *next_task = NULL;
   Orpgtat_entry_t *task_entry = NULL;
   int inserted = 0;

   /* Go the head of task product list. */
   tmp_task = *task_prod_list;

   /* Search the task product list for match on task id  */
   while (tmp_task != NULL){

      /* Match found.  Return. */
      if( strcmp( tmp_task->task_name, task_name ) == 0 )
         return PS_DEF_SUCCESS;

      /* Check next task is list for match on task id. */
      tmp_task = tmp_task->next;

   /* End of "while" loop. */
   }

   /* If here, no match found.  Allocate space for task product list
      entry.  Exit on MISC_malloc failure. */
   tmp_task = MISC_malloc(sizeof(Task_prod_chain));

   /* Initialize fields in the task product list for this task. */
   strcpy( tmp_task->task_name, task_name );
   tmp_task->gen_prod_list = NULL;
   tmp_task->support_prod_list = NULL;
   tmp_task->status = PGS_UNKNOWN;
   tmp_task->reported = 0;
   tmp_task->next = NULL;

   /* Read the task attribute table for this task id. */
   task_entry = ORPGTAT_get_entry( task_name );

   if( task_entry == NULL ){

      LE_send_msg( GL_INFO, "Task %s Not In Task Attribute Table.\n", task_name );
      free( tmp_task );
      return( PS_DEF_FAILED );

   }

   /* Build generation product list for this task.   This is the list of
      products this task generates. */
   if( task_entry->num_output_dataids > 0 ){

      Prod_wth_only_id_next *next_prod = NULL, *prev_prod = NULL;
      char *tat_entry = NULL;
      int  *product_ids = NULL, i;

      tat_entry = (char *) task_entry;
      product_ids = (int *) (tat_entry + task_entry->output_data); 

      /* Do For Each Output Product in task attribute list for this task. */
      for( i = 0; i < task_entry->num_output_dataids; i++ ){

         next_prod = MISC_malloc( sizeof(Prod_wth_only_id_next) );

         /* Set the product id and next pointer. */
         next_prod->prod_id = *product_ids;
         next_prod->schedule = PRODUCT_ENABLED;
         next_prod->reported = 0;
         next_prod->next = NULL;

         /* Append to generation product list. */
         if( tmp_task->gen_prod_list == NULL ){

            prev_prod = next_prod;
            tmp_task->gen_prod_list = prev_prod;

         }
         else if( prev_prod != NULL )
            prev_prod->next = next_prod; 

         else{

            LE_send_msg( GL_INFO, "Coding Error in Building Task Attribute Table\n" );
            ORPGTASK_exit(GL_ERROR);

         }
            
         /* Go to next product in output list of task attribute table. */
         product_ids++;

         prev_prod = next_prod;

      /* End of "for" loop. */
      }

   }

   /* Build support product list for this task.  This is the list of products
      this task inputs. */
   if( task_entry->num_input_dataids > 0 ){

      Prod_wth_only_id_next *next_prod = NULL, *prev_prod = NULL;
      char *tat_entry = NULL;
      int  *product_ids = NULL, i;

      tat_entry = (char *) task_entry;
      product_ids = (int *) (tat_entry + task_entry->input_data); 

      /* Do For Each Output Product in task attribute list for this task. */
      for( i = 0; i < task_entry->num_input_dataids; i++ ){

         next_prod = MISC_malloc( sizeof(Prod_wth_only_id_next) );

         /* Set the product id and next pointer. */
         next_prod->prod_id = *product_ids;
         next_prod->schedule = PRODUCT_ENABLED;
         next_prod->reported = 0;
         next_prod->next = NULL;

         /* Append to generation product list. */
         if( tmp_task->support_prod_list == NULL ){

            prev_prod = next_prod;
            tmp_task->support_prod_list = prev_prod;

         }
         else if( prev_prod != NULL )
            prev_prod->next = next_prod; 

         else{

            LE_send_msg( GL_INFO, "Coding Error in Building Task Attribute Table\n" );
            ORPGTASK_exit(GL_ERROR);

         }
            
         /* Go to next product in output list of task attribute table. */
         product_ids++;

         prev_prod = next_prod;

      /* End of "for" loop. */
      }

   }

   /* Insert this task list into the task product list. */
   if( *task_prod_list == NULL )
      *task_prod_list = tmp_task;

   else{

      /* The task list entry is inserted into list in 
         lexicographical order. */
      inserted = 0;
      prev_task = next_task = *task_prod_list;
      while( next_task != NULL ){

         if( strcmp( tmp_task->task_name, next_task->task_name) > 0 ){

            /* Move to the next entry. */
            prev_task = next_task;
            next_task = next_task->next;
            continue;

         }
         else{

            /* Do the addresses match? */
            if( prev_task == next_task ){

               /* Inserting task prior to head of list. */
               next_task = *task_prod_list;
               *task_prod_list =  tmp_task;
               tmp_task->next = next_task;

            }
            else{

               /* Inserting task somewhere in the middle of the list. */
               tmp_task->next = next_task;
               prev_task->next = tmp_task;

            }
          
            /* Set flag indicating entry was inserted in list and
               break out of "while" loop. */
            inserted = 1;
            break;

         }

      /* End of "while" loop. */
      }
            
      /* Task list entry inserted at tail of list. */
      if( !inserted )
         prev_task->next = tmp_task;

   }

   /* Increment the number of Task Product List entries. */
   Task_prod_list_size++;

   /* Free the task entry. */
   free(task_entry);

   return PS_DEF_SUCCESS;

/* End of Add_task_prod_list() */
}

/**************************************************************************
   Description:
      Free space allocated to Product Dependency List.

   Input:

   Output:

   Returns:
      Always returns PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
static int Free_prod_depend_list(void){

   Prod_depend_chain *tmp_prod, *next_entry;
   Prod_wth_only_id_next *next_prod;

   tmp_prod = (Prod_depend_chain *) Prod_depend_list;
   while (tmp_prod != NULL){

      while( tmp_prod->prod_depend_list != NULL ){

         next_prod = tmp_prod->prod_depend_list->next;
         free( tmp_prod->prod_depend_list );
         tmp_prod->prod_depend_list = next_prod;

      /* End of inner "while" loop. */
      }

      next_entry = tmp_prod->next;
      free( tmp_prod );
      tmp_prod = next_entry;

   /* End of outer "while" loop. */
   }

   Prod_depend_list = NULL;
   Prod_depend_list_size = 0;

   return( PS_DEF_SUCCESS );

/* END of Free_prod_depend_list() */
}

/**************************************************************************
   Description:
      Free space allocated to Dependent Products List.

   Input:

   Output:

   Returns:
      Always returns PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
static int Free_depend_prod_list(void){

   Depend_prod_chain *current_task;
   Task_prod_wth_only_id_next *next_node, *current_node;
   int list_index;

   current_task = Depend_prod_list;
   list_index = 0;
   while( list_index < Depend_prod_list_size ){
   
      next_node = current_task->depend_prod_list;
      while( next_node != NULL ){

         current_node = next_node;
         next_node = current_node->next;
         free( current_node );

      /* End of inner "while" loop. */
      }
   
      current_task++;
      list_index++;
  
   /* End of outer "while" loop. */
   }

   free( Depend_prod_list );
   Depend_prod_list = NULL;
   Depend_prod_list_size = 0;

   return( PS_DEF_SUCCESS );

/* END of Free_depend_prod_list() */
}

/**************************************************************************
   Description:
      Free space allocated to Task Product List.

   Input:

   Output:

   Returns:
      Always returns PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
static int Free_task_prod_list(void){

   Task_prod_chain *tmp_task, *next_entry;
   Prod_wth_only_id_next *next_prod;

   /* Free the generated products list. */
   tmp_task = (Task_prod_chain *) Task_prod_list; 
   while (tmp_task != NULL){

      while( tmp_task->gen_prod_list != NULL ){

         next_prod = tmp_task->gen_prod_list->next;
         free( tmp_task->gen_prod_list );
         tmp_task->gen_prod_list = next_prod;

      /* End of inner "while" loop. */
      }

      tmp_task = tmp_task->next;

   /* End of outer "while" loop. */
   }

   /* Free the support products list. */
   tmp_task = (Task_prod_chain *) Task_prod_list; 
   while (tmp_task != NULL){

      while( tmp_task->support_prod_list != NULL ){

         next_prod = tmp_task->support_prod_list->next;
         free( tmp_task->support_prod_list );
         tmp_task->support_prod_list = next_prod;

      /* End of inner "while" loop. */
      }

      next_entry = tmp_task->next;
      tmp_task = next_entry;

   /* End of outer "while" loop. */
   }

   free( Task_prod_list );
   Task_prod_list = NULL;
   Task_prod_list_size = 0;

   return( PS_DEF_SUCCESS );

/* END of Free_task_prod_list() */
}

/**************************************************************************
   Description: 
      This is the driver module for generating the Task Product List.  The
      Task Product List contains an entry for each schedulable task.  For 
      each task, the input product IDs the task consumes and the output
      product IDs the task produces are determined and stored in the List. 
 
      This is also the driver module for generating the Product Dependency
      List.  The Product Dependency List contains for each product ID, 
      those products which must be generated in order that this product
      can be produced.  That is, those products which the product ID is 
      not only directly dependent on, but also indirectly dependent on.

   Input: 
      None.

   Output:
      None.

   Returns:
      Currently, this module always returns PS_DEF_SUCCESS. 

   Notes:
      Task exit occurs on MISC_malloc failure.
  
**************************************************************************/
static int Generate_task_prod_list(void){

   int num_prods, index, prod_id, type;
   Prod_depend_chain *tmp_prod = NULL, *prev_prod = NULL;

   static Task_prod_chain *task_prod_list = NULL;

   /* Get the number of products in the Product Attribute Table. */
   num_prods = ORPGPAT_num_tbl_items();

   /* Do For All products in the Product Attribute Table. */
   Prod_depend_list_size = 0;
   index = 0;
   while( index < num_prods ){

      /* Get the product ID. */
      if( (prod_id = ORPGPAT_get_prod_id( index )) == ORPGPAT_ERROR ){

         LE_send_msg( GL_ERROR, "ORPGPAT_get_prod_id Returned Error For Index %d\n",
                      index );
         ORPGTASK_exit(GL_ERROR);

      }

      /* Ignore products which are generated on demand.  These products
         are not scheduled.  There is a special case for alerting. */
      if( (type = ORPGPAT_get_type( prod_id )) != ORPGPAT_ERROR ){

         if( (type != TYPE_ON_DEMAND)
                        ||
             ((prod_id == ALRTMSG) || (prod_id == ALRTPROD)) ){

            char *task_name = ORPGPAT_get_gen_task( prod_id );

            if( (task_name != NULL) && (strlen(task_name) > 0) ){

               /* Add task, and its inputs and outputs to Task_prod_chain. */
               Add_task_prod_list( &task_prod_list, task_name );

            }
            else
               LE_send_msg( GL_INFO, 
                  "Gen Task For Prod ID %d Not Added To Task Product List\n",
                  prod_id );

         }

      }
      else{

         LE_send_msg( GL_INFO, "ORPGPAT_get_type Returned Error For Prod ID %d\n",
                      prod_id );

         ORPGTASK_exit( GL_ERROR );

      }

      /* Prepare for next entry in Product Attribute Table. */
      index++;

   /* End of "while" loop. */
   }

   /* Transform Task Product List into a linear array.  We do this to make
      binary searches easier. */
   Task_prod_list = (Task_prod_chain *) 
                    MISC_malloc( sizeof(Task_prod_chain)*Task_prod_list_size );

   index = 0;
   while( task_prod_list != NULL ){

      Task_prod_chain *next_entry;

      /* Copy Task Product List entry data. */
      memcpy( (void *) (Task_prod_list + index), (void *) task_prod_list, 
              sizeof(Task_prod_chain) );

      /* Move to next entry in list. */
      next_entry = task_prod_list->next;
      free( task_prod_list );
      task_prod_list = next_entry;
      (Task_prod_list + index)->next = NULL;
      index++;

   /* End of "while" loop. */
   }

   /* Form Prod_depend_list for each product. */

   /* Do For Each Product in Product Attribute List. */
   index = 0;
   while( index < num_prods ){

      /* Allocate a new entry for Product Dependency Chain. */
      tmp_prod = (Prod_depend_chain *) MISC_malloc( sizeof( Prod_depend_chain ) );

      /* Insert this node into the list. */
      if( Prod_depend_list == NULL )
         Prod_depend_list = tmp_prod;

      else if( prev_prod != NULL )
         prev_prod->next = tmp_prod;

      else{
       
         LE_send_msg( GL_INFO, 
                      "Coding Error In Building Dependent Products list\n" );
         ORPGTASK_exit( GL_ERROR );

      } 

      /* Set fields in node. */
      tmp_prod->prod_id = ORPGPAT_get_prod_id( index ); 
      tmp_prod->prod_depend_list = NULL;
      tmp_prod->next = NULL;

      /* Build Product Dependency Chain entry for this product. */
      Build_dep_list_of_this_prod( tmp_prod, tmp_prod->prod_id );
       
      /* Prepare for next product in Product Attributes List. */
      index++;
      prev_prod = tmp_prod;

      /* Increment the size of the Product Dependency List. */
      Prod_depend_list_size++;

   /* End of "while" loop. */
   }

   /* Write the Product Dependency Chain to Log file. */
   if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
      PSPTT_dump_prod_depend_list(); 

   return( PS_DEF_SUCCESS );

/* END of Generate_task_prod_list() */
}

/**************************************************************************
   Description: 
      This is the driver module for building the dependent products list.

   Input: 
      this_prod - Dependent Products List entry. 
      prod_id - product ID for which dependent products are needed.

   Output: 

   Returns:
      Returns PS_DEF_SUCCESS when dependent products list for prod_id is
      finished.  Otherwise, if some error occurs, PS_DEF_FAILED is 
      returned.

   Notes:
      This function is recursively called.

**************************************************************************/
static int Build_dep_list_of_this_prod( Prod_depend_chain *this_prod,
                                        prod_id_t prod_id ){

   int num_prods, index, skip;
   int tmp_num_prods, tmp_end_prod, dep_prod_id;
   int tmp_num_opt_prods, tmp_opt_prod, opt_prod_id;

   /* Get number of products in Product Attributes List. */
   num_prods = ORPGPAT_num_tbl_items();

   /* Do For All Product Attribute List entries. */
   index = 0;
   while( index < num_prods ){

      /* If match on product ID, then .... */
      if( ORPGPAT_get_prod_id( index ) == prod_id){

         /* Get dependent products list for this product. */
         tmp_num_prods = ORPGPAT_get_num_dep_prods( prod_id );
         tmp_num_opt_prods = ORPGPAT_get_num_opt_prods( prod_id );
         if( tmp_num_prods > 0 ){

            /* Recursively add all dependent products to dependent products
               list for "prod_id". */
            tmp_end_prod = 0;
            while( tmp_end_prod < tmp_num_prods ){
    
               dep_prod_id = ORPGPAT_get_dep_prod( prod_id, tmp_end_prod );
               if( dep_prod_id >= 0 
                               && 
                   ORPGPAT_prod_in_tbl( dep_prod_id ) >= 0  ){

                  /* If a dependent product ID is same as product ID of product
                     which the dependency list is being built, there is a
                     problem with the Product Attribute Table so terminate. */
                  if( dep_prod_id == this_prod->prod_id ){
  
                     LE_send_msg( GL_INFO,
                            "Build Depend_list: Product ID %d Dependent on Self\n",
                            dep_prod_id );
                     ORPGTASK_exit( GL_ERROR );
 
                  }

                  /* For optional input products, do not append to dependency list. */
                  skip = 0;
                  for( tmp_opt_prod = 0; tmp_opt_prod < tmp_num_opt_prods; tmp_opt_prod++ ){

                     opt_prod_id = ORPGPAT_get_opt_prod( prod_id, tmp_opt_prod );
                     if( opt_prod_id == dep_prod_id ){

                        skip = 1;
                        break;

                     } 

                  }

                  /* If we are to skip this product because it is optional, 
                     increment "tmp_end_prod" then continue to the top of the 
                     "while" loop ... i.e., go to the next dependent product. */ 
                  if( skip ){

                     tmp_end_prod++;
                     continue;

                  }
                  
                  /* Add this product to the dependency list because it is a
                     required dependency. */
                  Append_prod_to_dependency_list( this_prod, dep_prod_id );
                  Build_dep_list_of_this_prod( this_prod, dep_prod_id );

               }

               /* Go to next dependent product in list. */
               tmp_end_prod++;

            /* End of "while" loop. */
            }

            return( PS_DEF_SUCCESS );

         }
         else
            return( PS_DEF_SUCCESS );

      }

      /* Go to next Product Attributes List entry. */
      index++;

   /* End of "while" loop. */
   }

   return( PS_DEF_FAILED );

/* END of Build_dep_list_of_this_prod() */
}

/******************************************************************************

   Description:
      This module adds prod_id to the Product Dependency List.

   Inputs:
      this_prod - head of chain of Dependent Products List for some
                  product.
      prod_id - Product ID of product to add to list.

   Outputs:

   Returns:
      There is no return values defined for this function.

   Notes:
      If memory allocation of product dependency list list entry fails,
      this process exits.

*******************************************************************************/
static void Append_prod_to_dependency_list( Prod_depend_chain *this_prod, 
                                            prod_id_t prod_id ){

   Prod_wth_only_id_next *new_entry;
   Prod_wth_only_id_next **prod_dep;

   /* Set dep_prod to the beginning of the dependent product list. */
   prod_dep = &(this_prod->prod_depend_list);

   /* If the dependent product list is not initially empty, cycle through
      link list until the end of the list. */
   if( *prod_dep != NULL ){

      while( (*prod_dep)->next != NULL ){

         if( (*prod_dep)->next->prod_id == prod_id )
            return;

         /* Go to next entry in dependent product list. */
         prod_dep = &((*prod_dep)->next);

      /* End of "while" loop. */
      }

      /* At the end of the dependent product list.  Add new entry here! */
      prod_dep = &((*prod_dep)->next);

   }

   /* Allocate space for new entry. */
   new_entry = (Prod_wth_only_id_next *) MISC_malloc( sizeof(Prod_wth_only_id_next) );

   /* Set dependent product product ID and next pointer to NULL. */
   new_entry->prod_id = prod_id;
   new_entry->schedule = PRODUCT_ENABLED;
   new_entry->reported = 0;
   new_entry->next = NULL;

   /* Link new entry into dependent product chain */
   *prod_dep = new_entry;

   return;

/* End of Append_prod_to_dependency_list() */
}

/**************************************************************************
   Description: 
      This is the driver module for generating the Dependent Products Chain.
      The Dependent Products Chain contains an entry for each schedulable task.
      For each task, the products which depend on the task execution are
      listed. 
 
   Input: 
      None.

   Output:
      None. 

   Returns:
      Currently, this module always returns PS_DEF_SUCCESS. 

   Notes:
      Task exit occurs on MISC_malloc failure.
  
**************************************************************************/
static int Generate_dep_prod_list(void){

   Task_prod_chain *curr_task, *end_task;
   Prod_wth_only_id_next *gen_prod_list;
   int list_index;

   /* Form Depend Products List for each task. */
   Depend_prod_list_size = Task_prod_list_size;

   Depend_prod_list = (Depend_prod_chain *) MISC_malloc( sizeof(Depend_prod_chain) *
                                                         Depend_prod_list_size );

   /* Do For Each Task in the Task Product List. */
   curr_task = Task_prod_list;
   end_task = Task_prod_list + Task_prod_list_size;
   list_index = 0;
   while( curr_task < end_task ){

      Task_prod_wth_only_id_next *dep_prod = NULL;
      Depend_prod_chain *tmp_task;

      /* Set fields in this node. */
      tmp_task = Depend_prod_list + list_index;
      strcpy( (char *) &(tmp_task->task_name[0]), (char *) &(curr_task->task_name[0]) ); 
      tmp_task->depend_prod_list = NULL;

      /* Build Product Dependency Chain entry for this product. */
      gen_prod_list = curr_task->gen_prod_list;
      while( gen_prod_list != NULL ){

         dep_prod = (Task_prod_wth_only_id_next *) 
                    MISC_malloc( sizeof( Task_prod_wth_only_id_next ) );

         /* Set product ID. */
         dep_prod->prod_id = gen_prod_list->prod_id;
         dep_prod->mand_or_opt = MANDATORY;
         strcpy( dep_prod->gen_task, curr_task->task_name );
         dep_prod->next = NULL;

         /* Insert this node into the list of dependent products. */
         if( tmp_task->depend_prod_list == NULL )
            tmp_task->depend_prod_list = dep_prod;

         else{

            Task_prod_wth_only_id_next *tmp_node;

            tmp_node = tmp_task->depend_prod_list;
            dep_prod->next = tmp_node;
            tmp_task->depend_prod_list = dep_prod;

         }

         Build_dep_prod_list_for_task( tmp_task, gen_prod_list->prod_id );
         gen_prod_list = gen_prod_list->next;

      /* End of "while" loop. */
      }
       
      /* Prepare for next task in Task Product List. */
      list_index++;
      curr_task++;

   /* End of "while" loop. */
   }

   /* Write the Dependent Products List to Log file. */
   if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
      PSPTT_dump_depend_prod_list(); 

   return( PS_DEF_SUCCESS );

/* END of Generate_dep_prod_list() */
}


/**************************************************************************
   Description: 
      This is the driver module for building the Dependent Products list.

   Input: 
      this_task - Dependent Products List entry. 
      prod_id - Product ID for product of interest.

   Output: 

   Returns:
      Returns PS_DEF_SUCCESS when dependent products list for prod_id is
      finished.  Otherwise, if some error occurs, PS_DEF_FAILED is 
      returned.

   Notes:

**************************************************************************/
static int Build_dep_prod_list_for_task( Depend_prod_chain *this_task,
                                         prod_id_t prod_id ){

   Prod_depend_chain *tmp_node;
   Task_prod_wth_only_id_next *new_node;
   int ind, num_opt_prods;
   char *temp;

   /* Do For All entries in Dependent Products List. */
   tmp_node = Prod_depend_list; 
   while( tmp_node != NULL ){
      
      Prod_wth_only_id_next *tmp_prod; 

      /* Do For All entries in product dependency list for this product. */
      tmp_prod = tmp_node->prod_depend_list;
      while( tmp_prod != NULL ){ 

         if( tmp_prod->prod_id == prod_id ){

            /* Add this product to the list of products which are dependent
               on this product. */
            new_node = (Task_prod_wth_only_id_next *) 
                       MISC_malloc( sizeof(Task_prod_wth_only_id_next) );

            /* Set node fields. */
            new_node->prod_id = tmp_node->prod_id;
            new_node->mand_or_opt = MANDATORY;

            /* Get the number of optional inputs for this product ID. */
            num_opt_prods = ORPGPAT_get_num_opt_prods( (int) tmp_node->prod_id );
            if( (num_opt_prods != 0) && (num_opt_prods != ORPGPAT_ERROR) ){

               int opt_prod_id;

               for( ind = 0; ind < num_opt_prods; ind++ ){
          
                  opt_prod_id = ORPGPAT_get_opt_prod( (int) tmp_node->prod_id, ind );
                  if( opt_prod_id == prod_id ){

                     new_node->mand_or_opt = OPTIONAL;
                     break;

                  } 

               }

            }

            temp = ORPGPAT_get_gen_task( tmp_node->prod_id );
            if( temp != NULL )
               strcpy( new_node->gen_task, temp );
            else
               new_node->gen_task[0] = '\0';
            new_node->next = NULL;

            /* Insert node at front of list. */
            if( this_task->depend_prod_list == NULL )
               this_task->depend_prod_list = new_node;
 
            else{

               Task_prod_wth_only_id_next *next_node, *prev_node;
               int node_needs_insertion = 1;

               /* Check to see if this product ID is already specified in
                  the list.  If so, no need to add node. */
               next_node = prev_node = this_task->depend_prod_list;
               while( next_node != NULL ){

                  if( next_node->prod_id == new_node->prod_id ){

                     /* Duplicate found. */
                     free( new_node );
                     node_needs_insertion = 0;
                     break;

                  }

                  /* Look at next product ID. */
                  prev_node = next_node;
                  next_node = next_node->next;

               /* End of "while" loop. */
               }

               if( node_needs_insertion ){

                  /* Add product.  Product is inserted at end of list. */
                  if( prev_node == next_node )
                     this_task->depend_prod_list = new_node;

                  else
                     prev_node->next = new_node; 

               }

            }

            break;

         }
         
         tmp_prod = tmp_prod->next;

      /* End of "while" loop. */
      }

      /* Go to next Product Attributes List entry. */
      tmp_node = tmp_node->next;

   /* End of "while" loop. */
   }

   return( PS_DEF_SUCCESS );

/* END of Build_dep_list_for_task() */
}

/**********************************************************************

   Description:
      Fill in the product generation table entry for default request.

   Inputs:
      prod_id - product ID.
      aux_data - pointer to product generation table entry which 
                 receives request information.

   Outputs:

   Returns:
      There is no return value define for this function.

   Notes:

**********************************************************************/
static void Fill_prod_gen_table_entry( prod_id_t prod_id, 
                                       Pd_prod_gen_tbl_entry *aux_data ){

   int i;
   char *temp;

   /* Fill in aux_data fields. */
   aux_data->prod_id = prod_id;
   temp = ORPGPAT_get_gen_task( prod_id );
   if( temp != NULL )
      strcpy( (char *) &(aux_data->gen_task[0]), temp );
   else
     aux_data->gen_task[0] = '\0';
   aux_data->gen_pr = 1;
   aux_data->req_num = 0;
   aux_data->next = NULL;

   for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ )
      aux_data->params[i] = PARAM_UNUSED;

   aux_data->elev_index = REQ_ALL_ELEVS;

/* End of Fill_prod_gen_table_entry() */
}
