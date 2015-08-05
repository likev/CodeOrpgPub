/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2014/03/12 17:39:17 $ $Id:
 * ps_handle_prod.c,v 1.70 1997/12/19 14:21:18 dodson Exp $ $Revision: 1.161 $
 * $State: Exp $
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <infr.h>
#include <prod_request.h>
#include <orpg.h>
#include <orpgadpt.h>
#include <rpg_vcp.h>

#include <rpg_port.h>
#include <a309.h>

#define PS_HANDLE_PROD
#include <ps_globals.h>
#undef PS_HANDLE_PROD
#include <ps_def.h>


/* Constant Definitions/Macro Definitions/Type Definitions. */

/*
Structure USER defines a list node containing information
such as RPS list for each connected user.  The information
contained within the structure is used in controlling
product generation on behalf of that user.
*/
typedef struct USER {

   short line_ind;                     /* User id is actually line_id. */
   unsigned short rps_len;             /* Routine Product Set length, in bytes. */
   char *rps_list;                     /* User's original Routine Product Set */
   Prod_gen_status_pr_req *prod_list;  /* Product list from narrowband user. */
   struct USER *next;                  /* Pointer of type struct USER
                                          to next user in list */
}  Line;


/* Static Globals. */
static Line *Line_list = NULL;     /* Internal user routine-product request list */
static Pd_prod_gen_tbl_entry *Default_prod_gen_list = NULL;
                                   /* Default Product Generation List. */

/* Output list to the gen_control. */
static Prod_gen_status_pr_req *Output_gen_control_list;
static Prod_gen_status_pr_req *Backup_output_gen_control_list;

/* Last time one time request list.  Backup the one-time-list */
static Prod_gen_status_pr_req *Back_one_time_list;


/* Static Function Prototypes.  NOTE:  Public function prototypes are
   defined in header file ps_def.h. */
static void Get_line_ind_list_index( unsigned int line_id, int *line_list_index, 
                                     unsigned int *line_bit );
static int Release_id_line(int line_id);

#define PARAM_LENGTH       7
static void Parm_to_macro( char str[][PARAM_LENGTH], short *parm );


/* Public Functions Start Below ...*/
/**************************************************************************
   Description: 
      Adds one-time requests to the product generation status for the 
      current volume scan.

   Input: 

   Output: 

   Returns: 

   Notes:

 **************************************************************************/
void PD_add_back_one_time_list_to_cur_vol(void){

   Prod_gen_status_pr_req *tmp;

   tmp = Back_one_time_list;
   while (tmp != NULL){

      /* Automatic variables ... */
      Prod_gen_status_pr tmp_prod;

      (void) memset( &tmp_prod, 0, sizeof(Prod_gen_status_pr) );

      tmp->gen_status.from.source = PS_DEF_FROM_ONE_TIME;
      tmp->gen_status.vol_time = 0;
      tmp->gen_status.vol_num = 0;
      tmp->gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
      tmp->gen_status.msg_ids = PGS_SCHEDULED;

      memcpy( (void *) &tmp_prod, (void *) &tmp->gen_status, 
              sizeof(Prod_gen_status_pr) );
      PSVPL_add_prod_gen_status(&tmp_prod, PS_DEF_CURRENT_VOLUME);

      tmp = tmp->next;

   /* while entries in the Backup OTR List */
   }        

/* END of PD_add_back_one_time_list_to_cur_vol() */
}

/**************************************************************************

   Description: 
      Make a backup copy of the Output Generation Control List.

      This should be done prior to processing a new default generation
      list, a new RPS list from the user, or new one-time requests.  

      This backup version contains the products which were scheduled
      prior to processing the new requests.  All the products in this
      list have the "act_this_time" set to deactivated.  

   Input: 
      There are no inputs.

   Output:
      Rebuilds the backup output generation control list 
      Backup_output_gen_control_list.

   Returns:
      There is no return value for this module.

   Notes:

**************************************************************************/
void PD_backup_output_gen_control_list( ){

   Line *tmp_line_list = NULL;
   Prod_gen_status_pr_req *tmp_req = NULL, t_req;
   Prod_gen_status_pr_req *tmp_one_time = NULL;
   Pd_prod_gen_tbl_entry  *tmp_default = NULL;
   int j;

   /* Start with a clean slate. */
   if (Backup_output_gen_control_list != NULL)
      PD_free_gen_control_list( BACKUP_GEN_CONTROL ); 

   /* Line_list is a pointer to the product request lists (RPS) for each
      user. */
   tmp_line_list = Line_list;

   /* Do for each user request (RPS) list. */
   while (tmp_line_list != NULL){

      tmp_req = tmp_line_list->prod_list;

      /* Do for each request for this user. */
      while (tmp_req != NULL){

         PD_add_output_gen_control_list( tmp_req, BACKUP_GEN_CONTROL);
         tmp_req = tmp_req->next;

      }
      tmp_line_list = tmp_line_list->next;

   }

   tmp_one_time = Back_one_time_list;

   /* Do for all one-time requests. */
   while (tmp_one_time != NULL){

      memcpy( (void *) &t_req.gen_status, (void *) &tmp_one_time->gen_status,
              sizeof( Gen_status_t ) );
      PD_add_output_gen_control_list(&(t_req), BACKUP_GEN_CONTROL);
      tmp_one_time = tmp_one_time->next;

   }

   /* If the default product generation list is empty, pointer should
      be NULL (i.e., undefined ). */
   tmp_default = Default_prod_gen_list;

   /* Initialize the product generation status for request to zeroes. */
   memset( &t_req, 0, sizeof( Prod_gen_status_pr_req ) );

   /* Do for all default requests. */
   while (tmp_default != NULL){

      /* Extract product information and add product to backup 
         generation control list. */
      t_req.gen_status.prod_id = tmp_default->prod_id;
      for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++)
         t_req.gen_status.params[j] = tmp_default->params[j];

      t_req.gen_status.elev_index = tmp_default->elev_index;
     
      t_req.gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_DEFAULT );
      t_req.gen_status.gen_pr = tmp_default->gen_pr;
      t_req.gen_status.req_num = tmp_default->req_num;

      t_req.gen_status.from.source = PS_DEF_FROM_DEFAULT;
      PD_add_output_gen_control_list(&(t_req), BACKUP_GEN_CONTROL);

      /* Go to next product in the default generation list. */
      tmp_default = tmp_default->next;

   }

   /* Set the active this time flag to deactivated. */
   tmp_req = Backup_output_gen_control_list;
   while (tmp_req != NULL){

      tmp_req->act_this_time = PS_DEF_PROD_DEACTIVATED;
      tmp_req = tmp_req->next;

   }

/* END of PD_backup_output_gen_control_list() */
}

/**************************************************************************
 Description: 
    This module merges the Backup_output_gen_control_list and the 
    Output_gen_control_list.  This merged copy is ultimately used to
    generate the product requests for this volume scan.

    The entries in Backup_output_gen_control_list not in the 
    Output_gen_control_list are not scheduled for generation.

 Input:
    There are not inputs to this module.

 Output: 
    The merger of the Backup_output_gen_control_list and the 
    Output_gen_control_list are saved as the Output_gen_control_list.

 Returns: 
    There is no return value for this module.

 Notes:

**************************************************************************/
void PD_merge_cur_backup_output_gen_control_list(void){

   int flag;
   Prod_gen_status_pr_req *tmp_cur;
   Prod_gen_status_pr_req *tmp_back;

   tmp_back = Backup_output_gen_control_list;
   while (tmp_back != NULL){

      tmp_cur = Output_gen_control_list;
      flag = 0;
      while (tmp_cur != NULL){

         if (PD_tell_same_prod( &tmp_back->gen_status, 
                                &tmp_cur->gen_status ) == PS_DEF_SUCCESS){

            flag = 1;
            break;

         }

         tmp_cur = tmp_cur->next;
      }

      if ( !flag ){

         /* Add product to output generation control list. */
         PD_add_output_gen_control_list(tmp_back, CUR_GEN_CONTROL);

      }
       
      tmp_back = tmp_back->next;

   }

   /* Free memory associated with Backup_output_gen_control_list. */
   PD_free_gen_control_list( BACKUP_GEN_CONTROL );

/* END of PD_merge_cur_backup_output_gen_control_list() */
}

/*************************************************************************
    Description: 
        The request identified by "req" is added to the Generation Control
        list.  Depending on the value of "call_from", it is either added to
        the Output_gen_control_list or the Backup_output_gen_control_list.

        The Generation Control List is used to set product requests.

    Input: 
        req - pointer to product request
        call_from - Either CUR_GEN_CONTROL or BACKUP_GEN_CONTROL

    Output: 

    Returns: 
        Returns PS_DEF_SUCCESS if request is already in the Generation 
        control list or has been successfully added to it.  Returns
        PS_DEF_FAILED if product id of the request is not valid.

    Notes:
        If memory allocation of generation status product request structure
        fails, task exits.
 
**************************************************************************/
int PD_add_output_gen_control_list(Prod_gen_status_pr_req *req, int call_from){

   Prod_gen_status_pr_req *adding_req = NULL;
   Prod_gen_status_pr_req *prev_req = NULL;
   Prod_gen_status_pr_req *tmp_req = NULL;
   Prod_wth_only_id_next *gen_prod_list = NULL;
   Task_prod_chain *task_entry = NULL;

   int task_status, elev_parm, type;

   /* Assign Generation Control List dependent on the "call_from" argument. */
   if( call_from == CUR_GEN_CONTROL )
      tmp_req = Output_gen_control_list;
   else
      tmp_req = Backup_output_gen_control_list;

   /* Check if this product is already defined in the Generation Control List.
      A match is based on product id and product dependent parameters.  If match,
      return PS_DEF_SUCCESS. */
   while( tmp_req != NULL ){

      if( PD_tell_same_prod( &req->gen_status, 
                             &tmp_req->gen_status ) == PS_DEF_SUCCESS )
         return( PS_DEF_SUCCESS );
       
      tmp_req = tmp_req->next;

   }

   /* Product not already in Generation Control List so must add it. */
   adding_req = MISC_malloc( sizeof(Prod_gen_status_pr_req) );

   /* Move product generation status information. Validate product id. */
   memcpy( (void *) &adding_req->gen_status, (void *) &req->gen_status,
            sizeof( Gen_status_t ) );
   if( ORPGPAT_prod_in_tbl( (int) adding_req->gen_status.prod_id ) < 0 ){

      if( adding_req != NULL )
         free(adding_req);

      adding_req = NULL;
      return( PS_DEF_FAILED );

   }

   /* If product is elevation based, assign elevation index and angle product 
      dependent parameter, if defined, according to current VCP. */
   if( (type = ORPGPAT_get_type( (int) adding_req->gen_status.prod_id )) == TYPE_ELEVATION ){

      if ((elev_parm = ORPGPAT_elevation_based((int) adding_req->gen_status.prod_id)) >= 0 )
         adding_req->gen_status.params[elev_parm] = 
                                RRS_get_elevation_angle( adding_req->gen_status.elev_index );

   }
   else
      adding_req->gen_status.elev_index = REQ_ALL_ELEVS;

   /* Fill in the remaining field of the request. */
   adding_req->act_this_time = req->act_this_time;
   adding_req->cnt = req->cnt;
   adding_req->num_products = req->num_products;
   adding_req->next = NULL;

   /* Check the task status.  If the task is not running for some reason (i.e.,
      task not configured or task failure), set the scheduling and msg id fields 
      appropriately.  This will prevent product from being scheduled for 
      generation. */
   task_entry = PSTS_is_gen_task_running( adding_req->gen_status.prod_id, &task_status );
   if( task_status != PGS_UNKNOWN ){
  
      /* Task is not running for some reason.  Set product scheduling to NOT
         SCHEDULED. */
      adding_req->gen_status.schedule = PGS_SCH_NOT_SCHEDULED;
 
      /* Set message id to the task state. */
      adding_req->gen_status.msg_ids = task_status;
  
   }

   /* Check the product scheduling status.   If the product scheduling has been
      disabled, set the scheduling field appropriately. */
   if( task_entry != NULL ){

      gen_prod_list = task_entry->gen_prod_list;
      while( gen_prod_list != NULL ){

         if( gen_prod_list->prod_id == adding_req->gen_status.prod_id ){

            if( gen_prod_list->schedule == PRODUCT_DISABLED ){

               adding_req->gen_status.schedule = PGS_SCH_NOT_SCHEDULED;
               adding_req->gen_status.msg_ids = PGS_PRODUCT_DISABLED;

            }

            break;
         }

         gen_prod_list = gen_prod_list->next;

      } /* End of while( gen_prod_list != NULL ) */

   } /* End of if( task_entry != NULL ) */

   /* Add the request to the Generation Control List. */
   if( call_from == CUR_GEN_CONTROL )
      prev_req = tmp_req = Output_gen_control_list;
   else
      prev_req = tmp_req = Backup_output_gen_control_list;
    
   while( tmp_req != NULL ){

      if( adding_req->gen_status.prod_id <= tmp_req->gen_status.prod_id ){

         if( call_from == CUR_GEN_CONTROL ){

            if( tmp_req == Output_gen_control_list ){

               adding_req->next = Output_gen_control_list;
               Output_gen_control_list = adding_req;

            }
            else{

               prev_req->next = adding_req;
               adding_req->next = tmp_req;

            }

         }
         else{

            if( tmp_req == Backup_output_gen_control_list ){

               adding_req->next = Backup_output_gen_control_list;
               Backup_output_gen_control_list = adding_req;

            }
            else{

               prev_req->next = adding_req;
               adding_req->next = tmp_req;

            }

         }

         return( PS_DEF_SUCCESS );

      }

      prev_req = tmp_req;
      tmp_req = tmp_req->next;

   }

   if( call_from == CUR_GEN_CONTROL ){

      if( Output_gen_control_list == NULL ){

         Output_gen_control_list = adding_req;
         return( PS_DEF_SUCCESS );

      }

   }
   else{

      if( Backup_output_gen_control_list == NULL ){

         Backup_output_gen_control_list = adding_req;
         return( PS_DEF_SUCCESS );

      }

   }

   prev_req->next = adding_req;

   return( PS_DEF_SUCCESS );

/* END of PD_add_output_gen_control_list() */
}

/**************************************************************************
   Description: 
      Transfers one-time request to the Back_one_time_list request list.
      These are one-time requests which couldn't be satisfied which now
      must be scheduled in the real-time data stream.

   Input:
      one_time_list - pointer to one-time request list.
      num - number of requests in the list.

   Output: 

   Returns:
      PS_DEF_SUCCESS if successful or there are no requests in list.  Returns
      PS_DEF_FAILED if "one_time_list" is empty but "num" is not zero or 
      vice versa.

   Notes:
      Task terminates on MISC_malloc failure.

 **************************************************************************/
int PD_backup_one_time_list(Prod_gen_status_pr_req *one_time_list, int num){

   int i;
   Prod_gen_status_pr_req *prev_req = NULL;

   /* If one_time_list or the number of requests is NULL, free memory
      for Baco_one_time_list, then return. */
   if (one_time_list == NULL && num == 0){

      PD_free_gen_control_list( ONETIME_GEN_CONTROL );
      return PS_DEF_SUCCESS;

   }

   /* Check for inconsistencies in the list and the number of requests in the 
      list. */
   if( (one_time_list != NULL && num == 0) 
                      ||
       (one_time_list == NULL && num != 0) ){

      LE_send_msg( GL_INFO, "PD_backup_one_time_list: one_time_list/num inconsistent!\n" );
      return PS_DEF_FAILED;

   }
    
   /* Allocate space for the list head node. */
   Back_one_time_list = (Prod_gen_status_pr_req *) 
                                  MISC_malloc( sizeof(Prod_gen_status_pr_req) );

   memset( Back_one_time_list, 0, sizeof(Prod_gen_status_pr_req) );
   prev_req = Back_one_time_list;

   /* Move product generation status info. */
   for (i = 0; i < num; i++){

      memcpy( (void *) &prev_req->gen_status, (void *) &one_time_list[i].gen_status, 
              sizeof( Gen_status_t ) );

      /* Allocate space for the next node. */
      prev_req->next = (Prod_gen_status_pr_req *) 
                                 MISC_malloc( sizeof(Prod_gen_status_pr_req) );
      memset( prev_req->next, 0, sizeof(Prod_gen_status_pr_req) );
      prev_req = prev_req->next;

   }

   /* Set the last link pointer to NULL. */
   prev_req->next = NULL;

   /* Return successful. */
   return PS_DEF_SUCCESS;

/* END of PD_backup_one_time_list() */
}

/**************************************************************************
   Description: 
      This module changes the "active this time" flag for each product 
      defined the the users routine product set. 

      If requests "num_products" field is -1, this means continuous 
      product transmission so product is "active this time".

      If requests "cnt" field is greater or equal to "num_products"
      field, make product "inactive this time".

      Based on "gen_pr", make product "active this time" or "inactive
      this time".    

   Input:

   Output: 

   Returns: 
      If Line_list is empty (no RPS lists for any users), return 
      PS_DEF_FAILED.  Otherwise, return PS_DEF_SUCCESS.

   Notes:

      Refer to RPG/PUP ICD for meanings associated with "num_products" 
      (number of products) and "gen_pr" (request interval) fields.  

**************************************************************************/
int PD_change_act_each_line(void){

   Line *tmp_line;
   Prod_gen_status_pr_req *tmp_req;

   if (Line_list == NULL)
      return PS_DEF_FAILED;
    
   tmp_line = (Line *) Line_list;
   while (tmp_line != NULL){

      tmp_req = tmp_line->prod_list;
      while (tmp_req != NULL){

         /* If product requested for continuous product transmission, then
            make active. */
         if (tmp_req->num_products == -1)
            tmp_req->act_this_time = 0;

         else{

            /* If generation count greater than number of continuous volume scans
               requested, deactivate product generation. */
            if (tmp_req->cnt >= tmp_req->num_products)
               tmp_req->act_this_time = PS_DEF_PROD_DEACTIVATED;

            else{

               /* Determine if product is to be activated this volume scan.  
                  This is determined by the request interval (gen_pr). */ 
               if( tmp_req->gen_status.gen_pr <= 0 )
                  LE_send_msg( GL_INFO, "Generation Period <= 0\n" );

               tmp_req->act_this_time = 
                        (tmp_req->act_this_time+1) % (tmp_req->gen_status.gen_pr);
               if (tmp_req->act_this_time == 0)
                  tmp_req->cnt++;
                
            }

         }

         /* Go to next product. */
         tmp_req = tmp_req->next;

      }

      /* Go to next user line list (i.e., RPG list) */
      tmp_line = tmp_line->next;

   /* While line list is not empty. */
   }      

   /*
     Return value undefined or unused. 
   */ 
   return (PS_DEF_SUCCESS);

/* END of PD_change_act_each_line() */
}

/**************************************************************************
   Description: 
      This module rebuilds the users product requests.  On VCP change, 
      this is done since some of the elevation based products may not be
      generated at the correct elevation index values.  On task failure 
      this is necessary to unschedule any upstream task which generate
      products in support of the product whose task failed. 
      

   Input:

   Output: 

   Returns: 
      If Line_list is empty (no RPS lists for any users), return 
      PS_DEF_FAILED.  Otherwise, return PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
int PD_rps_update( ){

   Line *tmp_line;

   /* Line list is empty ... must not be any connected users. */
   if (Line_list == NULL)
      return PS_DEF_FAILED;
    
   /* Process each non-empty RPS list. */
   tmp_line = (Line *) Line_list;
   while (tmp_line != NULL){

      if( (tmp_line->rps_list != NULL) && (tmp_line->rps_len > 0) )
         PD_process_request( (int) tmp_line->rps_len, (char *) tmp_line->rps_list,
                             PS_DEF_FROM_ONE_TIME + 100 );

      /* Go to next user line list (i.e., RPS list) */
      tmp_line = tmp_line->next;

   /* While line list is not empty. */
   }      

   /* Return success. */ 
   return (PS_DEF_SUCCESS);

/* END of PD_rps_update() */
}

/**************************************************************************
   Description:
       Form the default product generation list. This is done during 
       ps_routine initializaton or whenever the default product generation 
       list changes.

   Input: 
       None.

   Output: 
       default_pgt_num_prods - the number of products in default pgt.

   Returns: 
      Returns number of products generated by default on success,
      otherwise some failure occurred.  The number of products
      in the product generation table is not necessarily the number of 
      products generated by default.  If failure returned, then 
      "default_pgt_num_prods" is undefined.

    Notes:

**************************************************************************/
int PD_form_default_prod_gen_list( int *default_pgt_num_prods ){

   int prods_gen_by_default = 0, prod_id, ind, i, j;
   int vcp, vs_num, elev_param, num_elevs;
   short elev_req, vcp_elev_angles[MAX_ELEVATION_CUTS];
   short vcp_elev_inds[MAX_ELEVATION_CUTS];
   char *gen_task;

   Pd_prod_gen_tbl_entry *prod_gen_list = NULL;
   Pd_prod_gen_tbl_entry *prod_gen_list_prvptr = NULL;

   /* Read the Current Product Generation Table.  This function returns the
      number of products in the table. */
   if( (*default_pgt_num_prods = ORPGPGT_get_tbl_num( ORPGPGT_CURRENT_TABLE )) < 0 ){

      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Unable To Read Default Product Generation Table\n" );
      *default_pgt_num_prods = prods_gen_by_default = 0;

   }
   else{
      
      /* Set the default number of products to the number of products in the 
         default product generation table. */
      prods_gen_by_default = *default_pgt_num_prods;

   }

   /* Free storage for internal Default Product Generation List. */
   if( Default_prod_gen_list != NULL ){

      Pd_prod_gen_tbl_entry *next_node, *this_node;

      next_node = Default_prod_gen_list;
      while( next_node != NULL ){

         this_node = next_node;
         next_node = this_node->next;
         free( this_node );

      }

      Default_prod_gen_list = NULL;

   }

   /* Get the current VCP and volume scan number.  This may be needed to 
      expand elevation requests. */
   vcp = RRS_get_current_vcp_num();
   vs_num = RRS_get_volume_num(NULL);

   /* Do For All Products in the Current Product Generation Table. */
   ind = 0;
   for( i = 0; i < prods_gen_by_default; i++ ){

      /* Get the product ID of the next product in the current table. */
      prod_id = ORPGPGT_get_prod_id( ORPGPGT_CURRENT_TABLE, i );
     
      /* Check if product ID not valid. */ 
      if( ORPGPAT_prod_in_tbl( prod_id ) < 0  )
         continue;

      /* Save pointer to previous entry if entry pointer is set. */
      if( prod_gen_list != NULL )
         prod_gen_list_prvptr = prod_gen_list;

      /* Allocate storage for new internal product generation list entry. */
      prod_gen_list = (Pd_prod_gen_tbl_entry *) 
                               MISC_malloc( sizeof( Pd_prod_gen_tbl_entry ) ); 

      /* Is this elevation-based and does it have an elevation angle
         product dependent parameter? */
      elev_req = num_elevs = 0;
      if( (elev_param = ORPGPAT_elevation_based( prod_id )) >= 0 ){

         elev_req = ORPGPGT_get_parameter( ORPGPGT_CURRENT_TABLE, i, elev_param );

         /* Get the elevation angles associated with elevation request.  On error
            or no matching elevations, ignore this request. */
         if( (num_elevs = ORPGPRQ_get_requested_elevations( vcp, elev_req, 
                                                            (int) MAX_ELEVATION_CUTS,
                                                            vs_num, vcp_elev_angles, 
                                                            vcp_elev_inds )) <= 0 ){

            /* Write appropriate error message. */
            if( num_elevs < 0 )
               LE_send_msg( GL_ERROR, "Bad Elev Param (%x) For Prod ID %d\n",
                            elev_req, prod_id );

            else
               LE_send_msg( GL_ERROR, "No Elevs For Param (%x) For Prod ID %d\n",
                            elev_req, prod_id );

            /* Free space allocated for product. */
            free( prod_gen_list );
            prod_gen_list = NULL;

            /* Go to next product in default generation list. */
            continue;

         }

      }

      /* Set the list head pointer or link this entry with previous entry. */
      if( Default_prod_gen_list == NULL ){

         Default_prod_gen_list = prod_gen_list;
         prod_gen_list_prvptr = prod_gen_list;

      }
      else
         prod_gen_list_prvptr->next = prod_gen_list;

      /* Product is a default for the current weather mode.  Increment the 
         number of products in Default Product Generation List. */
      ind++;

      /* Transfer Default Product Generation message to Default Product 
         Generation List. */
      prod_gen_list->prod_id = prod_id;
      gen_task = ORPGPAT_get_gen_task( (int) prod_id );
      if( gen_task != NULL )
          strcpy( (char *) &(prod_gen_list->gen_task[0]), gen_task );

      else
          prod_gen_list->gen_task[0] = '\0';
      prod_gen_list->gen_pr = 
                   ORPGPGT_get_generation_interval( ORPGPGT_CURRENT_TABLE, i );
      prod_gen_list->req_num = 0;
      prod_gen_list->next = NULL;

      /* Move product dependent parameters. */
      for( j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++ )
         prod_gen_list->params[j] = 
                       ORPGPGT_get_parameter( ORPGPGT_CURRENT_TABLE, i, j );

      /* Process elevation parameter if product is elevation-based. */
      if( elev_param >= 0 ){

         /* If this is a multiple elevation request, .... */
         if( (elev_req & ORPGPRQ_ELEV_FLAG_BITS) || (num_elevs > 1) ){

            int elev_ind; 

            /* If here, the elevation angle denotes either 1) the lowest N 
               elevation slices, or all elevation angles at or below a specified 
               angle. */
            for( elev_ind = 0; elev_ind < num_elevs; elev_ind++ ){

               /* If this is not the first time through loop, the product must be
                  accounted for. */
               if( elev_ind != 0 ){

                  /* Increment the number of products. */
                  ind++;

                  /* Save pointer to previous list entry. */
                  prod_gen_list_prvptr = prod_gen_list;

                  /* Allocate storage for internal product generation list entry. */
                  prod_gen_list = (Pd_prod_gen_tbl_entry *) 
                                           MISC_malloc( sizeof( Pd_prod_gen_tbl_entry ) ); 

                  /* Copy the previous entry into "tmp". */
                  memcpy( prod_gen_list, prod_gen_list_prvptr, 
                          sizeof( Pd_prod_gen_tbl_entry ) );

                  /* Link new list entry into list and NULL the next pointer for this 
                     new list entry.*/
                  prod_gen_list_prvptr->next = prod_gen_list;
                  prod_gen_list->next = NULL;

               }

               /* Set the elevation angle based on the current VCP. */
               prod_gen_list->params[elev_param] = vcp_elev_angles[elev_ind];
               prod_gen_list->elev_index = vcp_elev_inds[elev_ind];

            }

         }
         else{

            prod_gen_list->params[elev_param] = vcp_elev_angles[0];
            prod_gen_list->elev_index = vcp_elev_inds[0];

         }
 
      }
      else{

         prod_gen_list->elev_index = REQ_ALL_ELEVS;

         /* For any product with special request requirements. */
         PD_special_request_processing( prod_id, &prod_gen_list->elev_index );

      }

   }

   /* Set the number of products to be generated by default. */
   prods_gen_by_default = ind;
   *default_pgt_num_prods = prods_gen_by_default;

   /* If there are product to be scheduled by default for this weather mode, 
      then ... */
   if( prods_gen_by_default >= 0 ){

      Pd_prod_gen_tbl_entry *entry = Default_prod_gen_list;

      /* Build the dependency list for each product in the default generation list,
         then schedule the product (and all dependent products) for generation. */
      for (i = 0; i < prods_gen_by_default; i++){

         PSPTT_through_dep_list_of_this_prod( entry->prod_id, PS_DEF_FROM_DEFAULT, 
                                              0, (void *) entry );
         entry = entry->next;

      }

      /* Determine the basic products (those which must be generated at all times)
         and schedule them. */
      PSPTT_schedule_basic_products();

      if( Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL )
         PSPTT_dump_task_prod_list();

   }

   /* There are no products to be scheduled by default for this weather mode. */
   else{

      /* Free memory allocated to default product generation list. */
      if( Default_prod_gen_list != NULL )
         free( Default_prod_gen_list );

      Default_prod_gen_list = NULL;

   }

   /* Inform user of number of valid products in Default Product Generation List. */
   {
      Pd_prod_gen_tbl_entry *entry = Default_prod_gen_list;

      prods_gen_by_default = 0;
      while( entry->next != NULL ){

         if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
            PD_write_prod( entry->prod_id, entry->params, entry->elev_index );

         prods_gen_by_default++;
         entry = entry->next;

      }

      if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
         LE_send_msg( GL_INFO,
                      "%d Products In Default Product Generation List.\n",
                      prods_gen_by_default );

   }

   return( prods_gen_by_default );

/* END of PD_form_default_prod_gen_list() */
}

/**************************************************************************
   Description:
      Returns address of Default Product Generation List.

   Input:

   Output: 

   Returns:

   Notes:

**************************************************************************/
Pd_prod_gen_tbl_entry* PD_get_default_prod_gen_list(void){


   /* Return Default Product Generation List address. */
   return (Default_prod_gen_list );

/* End of PD_get_default_prod_gen_list() */
}

/**************************************************************************
 Description: 
    Frees memory allocated to the one of the output generation control 
    lists.  The list is one of the following:

        Output_gen_control_list
        Backup_output_gen_control_list
        Back_one_time_list.

 Input: 
    list_type - describes which list needs to be freed.

 Output: 
    The memory allocated to output generation control list is freed and 
    the pointer is set to NULL.

 Returns: 
    void

 Notes:

**************************************************************************/
void PD_free_gen_control_list( int list_type ){

   Prod_gen_status_pr_req *tmp_req = NULL;

   /* Set the list pointer. */
   if( list_type == CUR_GEN_CONTROL )
      tmp_req = Output_gen_control_list;

   else if( list_type == BACKUP_GEN_CONTROL )
      tmp_req = Backup_output_gen_control_list;

   else if( list_type == ONETIME_GEN_CONTROL )
       tmp_req = Back_one_time_list;

   else{

      LE_send_msg( GL_ERROR, "Unknown Generation Control List Type (%d)\n",
                   list_type );
      ORPGTASK_exit( GL_ERROR );

   }

   /* Free the memory. */
   if( tmp_req != NULL ){

      Prod_gen_status_pr_req *next_req;

      while (tmp_req != NULL){

         next_req = tmp_req->next;
         free(tmp_req);
         tmp_req = next_req;

      }

      /* Set head pointer to NULL. */
      if( list_type == CUR_GEN_CONTROL )
         Output_gen_control_list = NULL;

      else if( list_type == BACKUP_GEN_CONTROL )
         Backup_output_gen_control_list = NULL;

      else if( list_type == ONETIME_GEN_CONTROL )
         Back_one_time_list = NULL;

   }

   return;

/* END of PD_free_output_gen_control_list() */
}

/**************************************************************************

   Description:
      This function generates the Output Generation and Control List for 
      the current volume scan.  This list is ultimately used to schedule
      products.

   Input: 
      option - used in setting activate this time field.

   Output: 

   Returns: 

   Notes:

**************************************************************************/
void PD_gen_output_gen_control_list(int option){

   Line *tmp_line_list = NULL;
   Prod_gen_status_pr_req *tmp_req = NULL, t_req;
   Prod_gen_status_pr_req *tmp_one_time = NULL;
   Pd_prod_gen_tbl_entry *tmp_default = NULL;
   int j;

   /* If output generation control list pointer is defined, free memory
      associated with it. */
   PD_free_gen_control_list( CUR_GEN_CONTROL );

   /* Do for all Routine Product Set (RPS) lists (user requests) */
   tmp_line_list = Line_list;
   while (tmp_line_list != NULL){

      tmp_req = tmp_line_list->prod_list;
 
      while (tmp_req != NULL){

         if( (option & 2) == 0)
            tmp_req->act_this_time = PS_DEF_PROD_DEACTIVATED;

         PD_add_output_gen_control_list(tmp_req, CUR_GEN_CONTROL);
         tmp_req = tmp_req->next;

      }

      tmp_line_list = tmp_line_list->next;
   }

   tmp_one_time = Back_one_time_list;
   while (tmp_one_time != NULL){

      memcpy( (void *) &t_req.gen_status, (void *) &tmp_one_time->gen_status,
               sizeof( Gen_status_t ) );
      t_req.cnt = 0;
      t_req.num_products = 0;
      if( (option & 1) == 0)
         t_req.act_this_time = PS_DEF_PROD_DEACTIVATED;

      else
         t_req.act_this_time = 0;
       
      PD_add_output_gen_control_list(&(t_req), CUR_GEN_CONTROL);
      tmp_one_time = tmp_one_time->next;

   }

   tmp_default = Default_prod_gen_list;
   while (tmp_default != NULL){

      t_req.gen_status.prod_id = tmp_default->prod_id;
      for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++)
         t_req.gen_status.params[j] = tmp_default->params[j];

      t_req.gen_status.elev_index = tmp_default->elev_index;
       
      t_req.gen_status.gen_pr = tmp_default->gen_pr;
      t_req.gen_status.req_num = tmp_default->req_num;

      /* Scheduled product by default. */
      t_req.gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_DEFAULT);

      t_req.gen_status.from.source = PS_DEF_FROM_DEFAULT;
      PD_clear_from_line_ind_list( &t_req.gen_status.from );

      t_req.gen_status.vol_time = 0;
      t_req.gen_status.vol_num = 0;
      t_req.gen_status.msg_ids = 0;
      t_req.cnt = 0;
      t_req.num_products = 0;
      if (option < 4)
         t_req.act_this_time = PS_DEF_PROD_DEACTIVATED;
      
      else
         t_req.act_this_time = 0;

      PD_add_output_gen_control_list(&(t_req), CUR_GEN_CONTROL);
      tmp_default = tmp_default->next;

   }

/* END of PD_gen_output_gen_control_list() */
}

/**************************************************************************
   Description: 
       Returns pointer to Output_gen_control_list.  This function would
       be used by functions outside this file scope who wish to access
       the Output_gen_control_list.

   Input:

   Output: 

   Returns: 
       Returns pointer to Output_gen_control_list.

   Notes:

**************************************************************************/
Prod_gen_status_pr_req* PD_get_output_gen_control_list(void){

   return (Output_gen_control_list);

/* END of PD_get_output_gen_control_list() */
}

/**************************************************************************
   Description: 
       Determines the number of products in Output_gen_control_list.
  
   Input: 

   Output:

   Returns: 
       The number of products in Output_gen_control_list.

   Notes:
       An undefined Output_gen_control_list returns 0 products.

**************************************************************************/ 
int PD_get_output_gen_control_list_len(void){

   int num;
   Prod_gen_status_pr_req *tmp_req;

   tmp_req = Output_gen_control_list;
   num = 0;

   /* Do Until no more products in Output_gen_control_list. */
   while (tmp_req != NULL){

      /* Increment the number of products in list. */
      ++num;

      /* Prepare for next product. */
      tmp_req = tmp_req->next;

   }

   /* Return the number of products. */
   return (num);

/* END of PD_get_output_gen_control_list_len() */
}

/**************************************************************************

   Description: 
      Initialize global variables defined in this source file or in the 
      global header file.

   Input:

   Output:

   Returns:
      There is no return value defined for this function.

   Notes:

**************************************************************************/
void PD_initialize(void){

   Back_one_time_list = NULL; 
   Default_prod_gen_list = NULL; 
   Line_list = NULL; 
   Output_gen_control_list = NULL;  

/* END of PD_initialize() */
}

/**************************************************************************
   Description:
      Add all the routine (RPS) requests to Vol_list[0].  Vol_list maintains
      a history of the product generation status.  Index 0 is always the 
      current volume scan. 

   Input: 
      Void.

   Output: 

   Returns:
      There is no return value define for this function. 

   Notes:
      This is the function that sets the message IDs to PGS_SCHEDULED.

**************************************************************************/
void PD_keep_routine_req_vol_list(void){

   Line *tmp_line_list = NULL;
   Prod_gen_status_pr_req *tmp_pr_prod = NULL;

   tmp_line_list = Line_list;

   /* Do for all user's line lists (i.e., RPS lists. ) */
   while (tmp_line_list != NULL){

      tmp_pr_prod = tmp_line_list->prod_list;

      /* Do while there are products in the RPS list. */
      while (tmp_pr_prod != NULL){

         /* Automatic variables ... */
         Prod_gen_status_pr prod_status;
         int task_state;
         char *gen_task;

         /* Process this product. */
         if( (tmp_pr_prod->act_this_time >= 0)
                         ||
             (tmp_pr_prod->act_this_time < tmp_pr_prod->gen_status.gen_pr) ){

            /* Get the task status or state. */
            gen_task = ORPGPAT_get_gen_task( tmp_pr_prod->gen_status.prod_id );
            if( gen_task == NULL ){

               LE_send_msg( GL_INFO, "Unknown Generating Task For Product %d\n",
                            tmp_pr_prod->gen_status.prod_id );
               tmp_pr_prod = tmp_pr_prod->next;
               continue;

            }

            /* Get the task state.  Used to determine if a task is to be scheduled
               or not. */
            PSTS_what_is_task_status( gen_task, &task_state );

            /* For now, a task state of PGS_UNKNOWN means task is running. */
            if( task_state == PGS_UNKNOWN ){

               /* Set scheduled by request. */
               tmp_pr_prod->gen_status.schedule = 
                                      (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
          
               /* If active this time, set msg id to PGS_SCHEDULED.  Otherwise, set 
                  to PGS_REQED_NOT_SCHED which means the product was requested, just
                  not for this volume scan. */
               if( tmp_pr_prod->act_this_time == 0 ) 
                  tmp_pr_prod->gen_status.msg_ids = PGS_SCHEDULED;

               else
                  tmp_pr_prod->gen_status.msg_ids = PGS_REQED_NOT_SCHED;

            }
            else{

               /* Task is not running for some reason.  Set product scheduling to 
                  NOT SCHEDULED. */
               tmp_pr_prod->gen_status.schedule = PGS_SCH_NOT_SCHEDULED;
   
               /* If task was not loaded, set message id to PGS_NOT_CONFIGURED. 
                  If task failed, set message id to PGS_TASK_FAILED. */
               tmp_pr_prod->gen_status.msg_ids = task_state;

            }

            /* Add this product to the product generation status for the 
               current volume scan. */
            memcpy( (void *) &prod_status, (void *) &tmp_pr_prod->gen_status, 
                    sizeof(Gen_status_t) );
            prod_status.next = NULL;
            PSVPL_add_prod_gen_status(&prod_status, PS_DEF_CURRENT_VOLUME);

         }

         /* Go to next product in the RPS list. */
         tmp_pr_prod = tmp_pr_prod->next;

      /* While there are prods in a given Line List entry (RPS List) */
      }    

      /* Go to next RPS list */
      tmp_line_list = tmp_line_list->next;

   /* While there are entries in the Line List */
   }       

/* END of PD_keep_routine_req_vol_list() */
}

/**************************************************************************
   Description: 
      Add all the one-time requests to be generated on real-time data
      stream to Vol_list[0].  Vol_list maintains a history of the 
      product generation status.  Index 0 is always the current volume 
      scan. 

   Input: 

   Output:

   Returns: 

   Notes:

 **************************************************************************/
void PD_keep_one_time_req_vol_list(void){

   Prod_gen_status_pr_req *tmp_prod = NULL;

   tmp_prod = Back_one_time_list;
   while (tmp_prod != NULL){

      /* Automatic variables ... */
      Prod_gen_status_pr add_prod;
      int task_state;
      char *gen_task;

      /* Get the task status or state. */
      gen_task = ORPGPAT_get_gen_task( tmp_prod->gen_status.prod_id );
      if( gen_task == NULL ){

         LE_send_msg( GL_INFO, "Unknown Generating Task For Product %d\n",
                      tmp_prod->gen_status.prod_id );
         tmp_prod = tmp_prod->next;
         continue;

      }

      /* Get the task state.  Used to determine if a task is to be scheduled
         or not. */
      PSTS_what_is_task_status( gen_task, &task_state );

      /* For now, a task state of PGS_UNKNOWN means task is running. */
      if( task_state == PGS_UNKNOWN ){

         /* Set scheduled by request. */
         tmp_prod->gen_status.schedule = 
                                (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
         /* If active this time, set msg id to PGS_SCHEDULED.  Otherwise, set 
            to PGS_REQED_NOT_SCHED which means the product was requested, just
            not for this volume scan. */
         if (tmp_prod->act_this_time == 0)
            tmp_prod->gen_status.msg_ids = PGS_SCHEDULED;

         else
            tmp_prod->gen_status.msg_ids = PGS_REQED_NOT_SCHED;
             
      }
      else{

         /* Task is not running for some reason.  Set product scheduling to 
            NOT SCHEDULED. */
         tmp_prod->gen_status.schedule = PGS_SCH_NOT_SCHEDULED;
   
         /* If task was not loaded, set message id to PGS_NOT_CONFIGURED. 
            If task failed, set message id to PGS_TASK_FAILED. */
         tmp_prod->gen_status.msg_ids = task_state;

      }

      /* Copy generation status info. */
      memcpy( (void *) &add_prod.gen_status, (void *) &tmp_prod->gen_status,
              sizeof( Gen_status_t ) );

      add_prod.gen_status.msg_ids = PGS_SCHEDULED;
      add_prod.next = NULL;

      PSVPL_add_prod_gen_status(&add_prod, PS_DEF_CURRENT_VOLUME);

      tmp_prod = tmp_prod->next;

   /* While there are entries in the Backup OTR List */
   }        

/* END of PD_keep_one_time_req_vol_list() */
}

/**************************************************************************
   Description:

      This module services a new product request message.  The message
      can come from either a user or from ps_onetime.

      1. put the user requests from Buf into Line_list.
      2. add these requests into Vol_list[0].
      3. write into prod_gen_control.lb.
   
      If a new line comes in, create a new Line, if all req got canceled
      on a line, delete this Line.

   Input: 
      len - length of the product request message.
      buf - product request message.
      call_from - who made the product request.

   Output: 

   Returns: 
      PS_DEF_FAILURE on failure, otherwise PS_DEF_NORMAL.

   Notes:

**************************************************************************/
int PD_process_request(int len, char *buf, int call_from){

   Pd_msg_header *tmp_hdr = NULL;
   Prod_gen_status_pr_req *tmp_pr_prod = NULL, *next_pr_prod = NULL;
   Prod_gen_status_pr_req *prod_list = NULL; 
   char *rps_list = NULL;

   Line *tmp_line_list = NULL, *tmp_line = NULL;
   int i, line_already_defined, ret, msg_len, dep_num_reqs, num_reqs;
 
   /* Cast buffer to message header structure. */
   tmp_hdr = (Pd_msg_header *) buf;

   /* Validate the product request message. */
   if( PD_validate_request_message( buf, len, call_from ) == PS_DEF_FAILED )
      return( PS_DEF_FAILED );

   /* If request from ps_onetime, service it here. */
   if (tmp_hdr->line_ind == -1 && call_from == PS_DEF_FROM_ONE_TIME){

      LE_send_msg(GL_INFO, "Processing One-time Product Request(s).\n");
      ret = PSPE_handle_one_time_req_list( buf, len );
      return( ret );

   }

   /* This must be a routine request. */
   msg_len = len;

   /* Try and find line index in line_list.  If match found, modify line_list.  
      If no match found, must be a new line.  Append to line_list. */
   tmp_line_list = (Line *) Line_list;
   line_already_defined = 0;

   while (tmp_line_list != NULL){

      /* Check for line index match. */
      if (tmp_line_list->line_ind == tmp_hdr->line_ind){

         /* First discard the original list of this line_ind.  
            Make the tmp_line_list_>prod_list empty. */
         PD_backup_output_gen_control_list();

         if( tmp_line_list->prod_list != NULL ){

            Prod_gen_status_pr_req *tmp_req = tmp_line_list->prod_list;
            Prod_gen_status_pr_req *next_req = NULL;

            while( tmp_req != NULL ){

               next_req = tmp_req->next;
               free(tmp_req); 
               tmp_req = next_req;

            }

         }

         tmp_line_list->prod_list = NULL;

         /* Set line found flag and break out of loop. */
         line_already_defined = 1;
         break;

      }
      else
         tmp_line_list = tmp_line_list->next;

   /* End of while loop */
   }

   /* If line index not found, then .... */
   if( !line_already_defined ){

      /* This is a new line. If there are not requests in message,
         just return. */
      if (msg_len == ALIGNED_SIZE(sizeof(Pd_msg_header)))
         return 1;
      
      /* Allocate space for new line list.  Terminate on MISC_malloc failure. */ 
      tmp_line_list = MISC_malloc( sizeof(Line) );

      tmp_line_list->line_ind = tmp_hdr->line_ind;
      tmp_line_list->rps_len = 0;
      tmp_line_list->rps_list = NULL;
      tmp_line_list->prod_list = NULL;
      tmp_line_list->next = NULL;

   }

   /* Save the RPS list if not the same one as already have.  If the RPS
      list is the same as already have (i.e., the addresses are the same),
      it is assumed this module was called from PD_rps_update().
      Otherwise, this module was assumed called from PSPE_proc_rt_request_event(). */
   if( (tmp_line_list->rps_list == NULL) 
                     ||  
       (tmp_line_list->rps_list != buf) ){

      if( tmp_line_list->rps_list != NULL ){

         LE_send_msg( GL_INFO, "Freeing Old RPS List For Line %d\n", 
                      tmp_line_list->line_ind );
         free( tmp_line_list->rps_list );

      }

      /* Save the new RPS list. */
      rps_list = (char *) MISC_malloc( (size_t) len );

      memcpy( (void *) rps_list, (void *) buf, len );
      tmp_line_list->rps_len = (unsigned short) len;
      tmp_line_list->rps_list = (char *) rps_list;

   }

   /* Inform operator that we are processing RPS list. */
   LE_send_msg( GL_INFO, "Processing RPS List For Line Index %d\n", tmp_hdr->line_ind );

   /* Build the Pd_gen_status_pr_req list from product request message. */
   num_reqs = PD_build_request_list( msg_len, buf, call_from, tmp_line_list->line_ind,
                                     &prod_list );

   LE_send_msg( GL_INFO, "There are %d Valid Requests for Line: %d\n", 
                num_reqs, tmp_line_list->line_ind );

   /* If this is a new line, then ... */
   if( !line_already_defined ){

      /* If this is the first line, set Line_list to this line list. */
      if (Line_list == NULL) 
         Line_list = tmp_line_list;

      else{

         /* Append this line list to Line_list */
         tmp_line = Line_list;
         while (tmp_line->next != NULL)
            tmp_line = tmp_line->next;
          
         tmp_line->next = tmp_line_list;

      }

   }


   /* Set up the prod_list linked list.   PD_build_request_list returns prod_list 
      as an array. */
   if( num_reqs > 0 ){

      tmp_pr_prod = (Prod_gen_status_pr_req *) MISC_malloc( sizeof(Prod_gen_status_pr_req) ); 
      memcpy( tmp_pr_prod, &prod_list[0], sizeof(Prod_gen_status_pr_req ) );
      tmp_line_list->prod_list = tmp_pr_prod;
      for( i = 1; i < num_reqs; i++ ){

         next_pr_prod = (Prod_gen_status_pr_req *) MISC_malloc( sizeof(Prod_gen_status_pr_req) );
         memcpy( next_pr_prod,  &prod_list[i], sizeof(Prod_gen_status_pr_req) );
         tmp_pr_prod->next = next_pr_prod;
         tmp_pr_prod = next_pr_prod;

      }
   
      /* Set the last element "next" pointer to NULL. */
      tmp_pr_prod->next = NULL;

      /* Free memory allocated to "prod_list". */
      free(prod_list);

   }

   /* If there are dependent products to schedule, then schedule them. */
   dep_num_reqs = num_reqs;
   if( dep_num_reqs > 0 ){

      tmp_pr_prod = tmp_line_list->prod_list;
      while (tmp_pr_prod != NULL){

         /* If product generation not deactivated, schedule dependent
            products. */
         if (tmp_pr_prod->act_this_time != PS_DEF_PROD_DEACTIVATED)
            PSPTT_through_dep_list_of_this_prod( tmp_pr_prod->gen_status.prod_id,
                                                 tmp_pr_prod->gen_status.from.source,
                                                 tmp_line_list->line_ind,
                                                 tmp_pr_prod );

         tmp_pr_prod = tmp_pr_prod->next;

      }

   }

   /* Deal with Vol_list, 1. delete original, 2. add this new list. */
   PSVPL_delete_line_ind_vol_list_latest(tmp_hdr->line_ind);

   /* Build product generation status. */
   tmp_pr_prod = tmp_line_list->prod_list;

   /* While there are products in the Product List of the Line List */
   while (tmp_pr_prod != NULL){

      /* Automatic variables ... */
      static Prod_gen_status_pr prod_status;

      /* Move fields from real-time generation status to product
         status. */
      memcpy( (void *) &prod_status, (void *) &tmp_pr_prod->gen_status, 
              sizeof(Gen_status_t) );
      prod_status.next = NULL;

      /* Add this product status to status array. */
      ret = PSVPL_add_prod_gen_status(&prod_status, PS_DEF_CURRENT_VOLUME);

      tmp_pr_prod = tmp_pr_prod->next;

   }   

   /* If this was an empty product request list, then remove 
      requests for this line. */
   if (msg_len == ALIGNED_SIZE(sizeof(Pd_msg_header)))
      Release_id_line(tmp_line_list->line_ind);

   /* Generate the output generation control list.  Merge
      with backup output generation control list.  */
   PD_gen_output_gen_control_list(7);
   PD_merge_cur_backup_output_gen_control_list();

   /* Build the product request list for all scheduled products. */
   PSPE_prod_list_to_prod_request( Output_gen_control_list, 
                                   PD_get_output_gen_control_list_len() );

   /* Free memory associated with Output_gen_control_list */
   PD_free_gen_control_list( CUR_GEN_CONTROL );

   /* Return value undefined or unused.  Return 0. */
   return 0;

/* END of PD_process_request() */
}

/**************************************************************************
   Description:
     This module initializes the Product Request List. All products in the 
     Product Atrribute Table have a Product Request List entry which indicates
     that the product is not to be generated.


    Input: 
       list - pointer to Prod_gen_status_pr structure containing product
              information.
 
    Output: 

    Returns:
       Returns PS_DEF_FAILED if product ID in Prod_gen_status_pr entry
       is not found in the Product Attribute Table.  Otherwise, 
       PS_DEF_SUCCESS is returned.

    Notes:
       If memory allocation fails, this process terminates.

**************************************************************************/
int PD_init_prod_list_to_gen_control( int prod_id ){

   Prod_request *prod_req = NULL, *term_req = NULL;
   int len, cur_id;

   /* Allocate storage for a single product request and a request
      terminator. */
   prod_req = MISC_malloc( 2*sizeof(Prod_request) );

   /* Verify that the product ID is valid.  Must be defined in
      the Product Attributes Table. Negative product IDs are special
      products and can not be directly scheduled. */
   prod_req->pid = prod_id;
   if( (prod_req->pid < 0) 
                ||
       (ORPGPAT_prod_in_tbl( (int) prod_req->pid ) < 0) ){

      if( prod_req->pid >= 0 )
         LE_send_msg( GL_INFO,
                      "Product ID %d Not is Product Attributes Table.\n",
                      prod_req->pid );

      else
         LE_send_msg( GL_INFO,
                      "Product ID %d Not is Product Attributes Table.\n",
                      prod_req->pid );


      /* Ignore this unknown product. */
      if( prod_req != NULL )
         free( prod_req );

      return( PS_DEF_FAILED );

   }

   /* Initialize product request data. */
   prod_req->param_1 = -2;
   prod_req->param_2 = -2;
   prod_req->param_3 = -2;
   prod_req->param_4 = -2;
   prod_req->param_5 = -2;
   prod_req->param_6 = -2;
   prod_req->elev_ind = REQ_NOT_SCHEDLD;

   prod_req->req_num = 0;
   prod_req->vol_seq_num = 0;
   cur_id = prod_req->pid;

   /* Add a request termination.  A request termination is denoted by a 
      Product ID (pid) of -1. */
   term_req = prod_req;
   term_req++;
   memset( term_req, 0, sizeof(Prod_request) );
   term_req->pid = -1;

   /* Write product requests to product requests LB. */
   len = ORPGDA_write( ORPGDAT_PROD_REQUESTS, (char *) prod_req, 
                       2*sizeof(Prod_request), cur_id );

   /* A negative length indicates an error. */
   if (len < 0)
      LE_send_msg( GL_INFO,
                   "ORPGDA_write ORPGDAT_PROD_REQUESTS (id %d) failed: %d",
                   cur_id, len);
          
   /* Free memory associated with product request. */
   if( prod_req != NULL )
      free(prod_req);

   return( PS_DEF_SUCCESS );

/* END of PD_init_prod_list_to_gen_control() */
}

/**************************************************************************
   Description:
      Schedules product "prod_id" with auxilliary data "aux_data".  How
      the product gets scheduled will depend on "source" which is either
      PS_DEF_FROM_DEFAULT (i.e., a default product generation request),
      PS_DEF_FROM_ONE_TIME (i.e., one-time request scheduled on realtime
      stream), or PS_DEF_FROM_ROUTINE (i.e., RPS list request).

   Input: 
      prod_id - Product ID
      source - either PS_DEF_FROM_DEFAULT, PS_DEF_FROM_ONE_TIME, or 
               PS_DEF_FROM_ROUTINE
      line_id - line index if RPS list request
      aux_data - auxilliary data (include such things are product
                 dependent parameters, scheduling information, ... )

   Output: 

   Returns: 
      PS_DEF_SUCCESS is product successfully scheduled, or PS_DEF_FAILED

   Notes:

**************************************************************************/
int PD_schedule_this_prod( prod_id_t prod_id, unsigned int source,
                           unsigned int line_id, void *aux_data ){

   Line *tmp_line_list = NULL;
   Pd_prod_gen_tbl_entry *prev_default = NULL, *tmp_default = NULL;
   Pd_prod_gen_tbl_entry *adding_default = NULL;
   Prod_gen_status_pr_req *prev_req = NULL, *tmp_pr_req = NULL;
   Prod_gen_status_pr_req *adding_req = NULL, *routine_aux_data;
   Prod_gen_status_pr_req *prev_onetime = NULL, *tmp_one_time = NULL; 
   Prod_gen_status_pr_req *adding_onetime = NULL;
   int i, ind, num_params;
   char *gen_task;


   /* If product ID is REFLDATA_ELEV or COMBBASE_ELEV, then set to BASEDATA_ELEV.  
      We do this is REFLDATA_ELEV and COMBBASE_ELEV are really special cases of 
      BASEDATA_ELEV.  */
   if( (prod_id == REFLDATA_ELEV) || (prod_id == COMBBASE_ELEV) )
      prod_id = BASEDATA_ELEV;

   /* If request the result of a default request, then.... */
   if (source == PS_DEF_FROM_DEFAULT){

      Pd_prod_gen_tbl_entry *def_aux_data;

      /* Go through default generation list.  If product found, then product
         should already be scheduled. */
      tmp_default = Default_prod_gen_list;
      while (tmp_default != NULL){

         if (tmp_default->prod_id == prod_id){

            Pd_prod_gen_tbl_entry *aux = (Pd_prod_gen_tbl_entry *) aux_data;

            /* If COMBBASE, REFLDATA, or BASEDATA do not have to schedule again.
               We ignore the parameter check. */
            if( (prod_id == COMBBASE) || (prod_id == REFLDATA) || (prod_id == BASEDATA) )
               return( PS_DEF_SUCCESS );

            /* If RAWDATA do not have to schedule again. */
            if( prod_id == RAWDATA )
               return( PS_DEF_SUCCESS );

            /*Some products may have special request processing. */
            PD_special_request_processing( prod_id, &aux->elev_index );

            /* If product dependent parameters and elevation index match, 
               do not have to schedule again. */
            if( (tmp_default->params[0] == aux->params[0])
                                  &&
                (tmp_default->params[1] == aux->params[1])
                                  &&
                (tmp_default->params[2] == aux->params[2])
                                  &&
                (tmp_default->params[3] == aux->params[3])
                                  &&
                (tmp_default->params[4] == aux->params[4])
                                  &&
                (tmp_default->params[5] == aux->params[5])
                                  &&
                (tmp_default->elev_index == aux->elev_index) ){

               return( PS_DEF_SUCCESS );

            }

         }
       
         tmp_default = tmp_default->next;

      }

      /* Need to schedule product. */
      adding_default = MISC_malloc( sizeof(Pd_prod_gen_tbl_entry) );

      def_aux_data = (Pd_prod_gen_tbl_entry *) aux_data;
      adding_default->prod_id = prod_id;
      gen_task = ORPGPAT_get_gen_task( prod_id );
      if( gen_task != NULL )
         strcpy( (char *) &(adding_default->gen_task[0]), gen_task );
      else
         adding_default->gen_task[0] = '\0';
         
      adding_default->gen_pr = def_aux_data->gen_pr;
      adding_default->req_num = 1;
      adding_default->next = NULL;

      /* Initialize all product dependent parameters to PARAM_UNUSED. */
      for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++)
         adding_default->params[i] = PARAM_UNUSED;
       
      /* Propagate the product dependent parameters and elevation index
         to upstream processes. */
      num_params = ORPGPAT_get_num_parameters( prod_id );
      for (i = 0; i < num_params; ++i){
   
         ind = ORPGPAT_get_parameter_index( prod_id, i );
         if( ind >= 0 )
            adding_default->params[ind] = def_aux_data->params[ind];
   
      }

      adding_default->elev_index = def_aux_data->elev_index;

      prev_default = tmp_default = Default_prod_gen_list;
      while (tmp_default != NULL){

         prev_default = tmp_default;
         tmp_default = tmp_default->next;

      }

      if (Default_prod_gen_list == NULL)
         Default_prod_gen_list = adding_default;

      else{

         prev_default->next = adding_default;
         adding_default->next = tmp_default;

      }

      return( PS_DEF_SUCCESS );

   }

   /* from is something other than PS_DEF_FROM_DEFAULT. */
   tmp_line_list = Line_list;
   while (tmp_line_list != NULL){

      /* If COMBBASE, REFLDATA, or BASEDATA do not have to schedule again.
         We ignore the parameter check. */
      if( (prod_id == COMBBASE) || (prod_id == REFLDATA) || (prod_id == BASEDATA) )
         return( PS_DEF_SUCCESS );

      /* If RAWDATA do not have to schedule again. */
      if( prod_id == RAWDATA )
         return( PS_DEF_SUCCESS );

      tmp_pr_req = tmp_line_list->prod_list;
      while (tmp_pr_req != NULL){

         if (tmp_pr_req->gen_status.prod_id == prod_id){

            Prod_gen_status_pr_req *aux = (Prod_gen_status_pr_req *) aux_data;

            /* Check parameters .... if parameters match, then return.  Otherwise,
               product is not already in list. */
            if( (tmp_pr_req->gen_status.params[0] == aux->gen_status.params[0])
                                  &&
                (tmp_pr_req->gen_status.params[1] == aux->gen_status.params[1])
                                  &&
                (tmp_pr_req->gen_status.params[2] == aux->gen_status.params[2])
                                  &&
                (tmp_pr_req->gen_status.params[3] == aux->gen_status.params[3])
                                  &&
                (tmp_pr_req->gen_status.params[4] == aux->gen_status.params[4])
                                  &&
                (tmp_pr_req->gen_status.params[5] == aux->gen_status.params[5])
                                  &&
                (tmp_pr_req->gen_status.elev_index == aux->gen_status.elev_index) )
               return PS_DEF_SUCCESS;

         }
          
         tmp_pr_req = tmp_pr_req->next;

      }

      tmp_line_list = tmp_line_list->next;

   }

   tmp_one_time = Back_one_time_list;
   while (tmp_one_time != NULL){

      /* If match on product ID, check for match on parameters. */
      if (tmp_one_time->gen_status.prod_id == prod_id){

         Prod_gen_status_pr_req *aux = (Prod_gen_status_pr_req *) aux_data;

         /* Check parameters .... if parameters match, then return.  Otherwise,
            product is not already in list. */
         if( (tmp_one_time->gen_status.params[0] == aux->gen_status.params[0])
                               &&
             (tmp_one_time->gen_status.params[1] == aux->gen_status.params[1])
                               &&
             (tmp_one_time->gen_status.params[2] == aux->gen_status.params[2])
                               &&
             (tmp_one_time->gen_status.params[3] == aux->gen_status.params[3])
                               &&
             (tmp_one_time->gen_status.params[4] == aux->gen_status.params[4])
                               &&
             (tmp_one_time->gen_status.params[5] == aux->gen_status.params[5])
                               &&
             (tmp_one_time->gen_status.elev_index == aux->gen_status.elev_index) )
            return PS_DEF_SUCCESS;

      }
       
      tmp_one_time = tmp_one_time->next;

   }

   /* Need to be added. */
   if (source == PS_DEF_FROM_ONE_TIME){

      Prod_gen_status_pr_req *onetime_aux_data;

      adding_onetime = MISC_malloc( sizeof(Prod_gen_status_pr_req) );

      onetime_aux_data = (Prod_gen_status_pr_req *) aux_data;
      memset( &adding_onetime->gen_status, 0, sizeof(Gen_status_t) );
      adding_onetime->gen_status.prod_id = prod_id;
      adding_onetime->gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
      adding_onetime->gen_status.gen_pr = onetime_aux_data->gen_status.gen_pr;
      adding_onetime->num_products = onetime_aux_data->num_products;
      adding_onetime->act_this_time = onetime_aux_data->act_this_time;
      adding_onetime->cnt = 0;
      adding_onetime->next = NULL;

      /* Initialize all product dependent parameters to PARAM_UNUSED. */
      for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++)
         adding_onetime->gen_status.params[i] = PARAM_UNUSED;
       
      /* Propagate the product dependent parameters to upstream processes. */
      num_params = ORPGPAT_get_num_parameters( prod_id );
      for (i = 0; i < num_params; ++i){
   
         ind = ORPGPAT_get_parameter_index( prod_id, i );
         if( ind >= 0 )
            adding_onetime->gen_status.params[ind] = onetime_aux_data->gen_status.params[ind];
   
      }

      /* Set the elevation index */
      adding_onetime->gen_status.elev_index = onetime_aux_data->gen_status.elev_index;

      adding_onetime->gen_status.from.source = PS_DEF_FROM_ONE_TIME;

      prev_onetime = tmp_one_time = Back_one_time_list;
      while (tmp_one_time != NULL){

         prev_onetime = tmp_one_time;
         tmp_one_time = tmp_one_time->next;

      }

      if (Back_one_time_list == NULL)
         Back_one_time_list = adding_onetime;

      else{

         prev_onetime->next = adding_onetime;
         adding_onetime->next = tmp_one_time;

      }

      return PS_DEF_SUCCESS;

   }

   /* Add the product request. */
   adding_req = MISC_malloc( sizeof(Prod_gen_status_pr_req) );

   routine_aux_data = (Prod_gen_status_pr_req *) aux_data;
   memset( &adding_req->gen_status, 0, sizeof(Gen_status_t) );
   adding_req->gen_status.prod_id = prod_id;
   adding_req->gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
   adding_req->gen_status.gen_pr = routine_aux_data->gen_status.gen_pr;
   adding_req->num_products = routine_aux_data->num_products;
   adding_req->cnt = 0;
   adding_req->act_this_time = routine_aux_data->act_this_time;
   adding_req->next = NULL;

   /* Initialize all product dependent parameters to PARAM_UNUSED. */
   for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++)
      adding_req->gen_status.params[i] = PARAM_UNUSED;
    
   /* Propagate the product dependent parameters to upstream processes. */
   num_params = ORPGPAT_get_num_parameters( prod_id );
   for (i = 0; i < num_params; ++i){
   
      ind = ORPGPAT_get_parameter_index( prod_id, i );
      if( ind >= 0 )
         adding_req->gen_status.params[ind] = routine_aux_data->gen_status.params[ind];
   
   }

   /* Set the elevation index for this product. */
   adding_req->gen_status.elev_index = routine_aux_data->gen_status.elev_index;

   /* Insert this product request into the product list for this user. */
   tmp_line_list = Line_list;
   while (tmp_line_list != NULL){

      if (tmp_line_list->line_ind == line_id){

         prev_req = tmp_pr_req = tmp_line_list->prod_list;
         while (tmp_pr_req != NULL){

            prev_req = tmp_pr_req;
            tmp_pr_req = tmp_pr_req->next;

         }

         adding_req->gen_status.from.source = source;
         PD_set_line_from_line_ind_list( &adding_req->gen_status.from, line_id );
         if (tmp_line_list->prod_list == NULL)
            tmp_line_list->prod_list = adding_req;

         else{

            prev_req->next = adding_req;
            adding_req->next = tmp_pr_req;

         }

         return PS_DEF_SUCCESS;

      }

      tmp_line_list = tmp_line_list->next;

   }

   /* If falls through to here, product was unable to be scheduled. 
      Clean-up and return failure. */
   if( adding_req != NULL )
      free(adding_req);

   adding_req = NULL;

   if( adding_default != NULL )
      free(adding_default);

   adding_default = NULL;
   return PS_DEF_FAILED;

/* END of PD_schedule_this_prod() */
}

/**************************************************************************
   Description: 
       This functions does validity checks and product ID as well as 
       checks for matching product dependent parameters between two products.

   Input:
       prod - Product Generation Status for first product.
       prod1 - Product Generation Status for second product.

   Output: 

   Returns: 
       PS_DEF_FAILED if undefined pointer, invalid product ID, product
       ID mismatch, or in the case of matching product IDs, mismatch on
       product dependent parameters; PS_DEF_SUCCESS otherwise. 

   Notes:

**************************************************************************/
int PD_tell_same_prod(Gen_status_t *prod, Gen_status_t *prod1){

   int elev_ind, i;

   /* If either pointer undefined, return failure. */
   if( prod == NULL || prod1 == NULL )
      return( PS_DEF_FAILED );
    
   /* If product IDs do not match, return failure. */
   if( prod->prod_id != prod1->prod_id )
      return( PS_DEF_FAILED );

   /* If either product ID invalid, return failure. */
   if( ORPGPAT_prod_in_tbl( (int) prod->prod_id ) < 0)
      return( PS_DEF_FAILED );

   else{

      /* Check if product is elevation-based. */
      elev_ind = ORPGPAT_elevation_based( (int) prod->prod_id );

      /* Check all product dependent parameters. */
      for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ ){

         if( elev_ind >= 0 && elev_ind == i ){

            if( RRS_get_elevation_index( prod->params[elev_ind], NULL ) !=
                RRS_get_elevation_index( prod1->params[elev_ind], NULL ))
               return( PS_DEF_FAILED );

         }
         else{

            if( prod->params[i] != prod1->params[i] )
               return( PS_DEF_FAILED );
             
         }

      }

      /* Check the elevation index value. */
      if( prod->elev_index != prod1->elev_index )
         return( PS_DEF_FAILED );

   }
  
   return( PS_DEF_SUCCESS );

/* END of PD_tell_same_prod() */
}

/*\***********************************************************************

   Description:
      Takes the "from" structure and sets the line index bit in the 
      line index list corresponding to line_id.

   Inputs:
      from - pointer to From_t structure.
      line_id - line id of user.

   Outputs:

   Return:
      There is no return value for this function.

   Notes:

************************************************************************\*/
void PD_set_line_from_line_ind_list( From_t *from, unsigned int line_id ){

   unsigned int line_bit;
   int line_list_index;

   /* Get the index into the line_ind_list array and the bit number
      to set within this array. */
   Get_line_ind_list_index( line_id, &line_list_index, &line_bit );

   /* Set the bit in the line_ind_list array for this line. */
   from->line_ind_list[line_list_index] |= (1 << line_bit);

   /* If source does not have the PS_DEF_FROM_REQUEST bit set, set it. */
   if( (from->source & PS_DEF_FROM_REQUEST) == 0 )
      from->source |= PS_DEF_FROM_REQUEST;

/* End of PD_set_from_line_ind_list() */
}

/******************************************************************

   Description:
      Clears the line index field of the line index list.

   Inputs:
      dest - pointer to line index list for a particular
             product.

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes:

******************************************************************/
void PD_clear_from_line_ind_list( From_t *dest ){

   int i;

   for( i = 0; i < PS_DEF_MAX_USERS; i++ )
      dest->line_ind_list[i] = 0;

/* End of PD_clear_from_line_ind_list() */
}

/**********************************************************************

   Description:
      This function merges two line index fields.

   Inputs:
      src - pointer to source line index list for a particular product.
      dest - pointer to destination line index list for a particular
             product.

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes:

**********************************************************************/
void PD_merge_from_line_ind_list( From_t *dest, From_t *src ){

   int i;

   for( i = 0; i < PS_DEF_MAX_USERS; i++ )
      dest->line_ind_list[i] |= src->line_ind_list[i];

/* End of PD_merge_from_line_ind_list() */
}

/*************************************************************************

   Description:
      Given a line index field for a particular product, checks if the
      user corresponding to line_id has requested the product.

   Inputs:
      from - pointer to a line index list for a particular product.
      line_id - line ID associated with a particular user.

   Outputs:
    
   Returns:
      Returns 1 (TRUE) if bit set in line index field corresponding to 
      user associated with line_id, otherwise return 0 (FALSE).

   Notes:

*************************************************************************/
int PD_test_line_from_line_ind_list( From_t *from, unsigned int line_id ){

   unsigned int line_bit;
   int line_list_index;

   /* Get the index into the line_ind_list array and the bit number
      to set within this array. */
   Get_line_ind_list_index( line_id, &line_list_index, &line_bit );

   if( (from->line_ind_list[line_list_index] & (1 << line_bit)) != 0 )
      return( 1 );

   return( 0 );

/* End of PD_test_from_line_ind_list() */
}

/***************************************************************************

   Description:
      Clears the bit associated with line_id from the line index field of
      a line index list.  If by clearing this bit the list index field 
      becomes 0, clear the bit in the source field of the line index list 
      associated with user request.

   Inputs:
      from - pointer to line index list.
      line_id - line ID associated with a particular user.

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes:

***************************************************************************/
void PD_clear_line_from_line_ind_list( From_t *from, unsigned int line_id ){

   unsigned int line_bit;
   int line_list_index, i;

   /* Get the index into the line_ind_list array and the bit number
      to set within this array. */
   Get_line_ind_list_index( line_id, &line_list_index, &line_bit );
   from->line_ind_list[line_list_index] -= (1 << line_bit);

   for( i = 0; i < PS_DEF_MAX_USERS; i++ ){

      if( from->line_ind_list[i] != 0 )
         return;

   /* End of "for" loop. */
   } 

   from->source -= PS_DEF_FROM_REQUEST;

/* End of PD_clear_line_from_line_ind_list() */
}

/********************************************************************

   Description:
      This function formats the Product ID and product dependent 
      parameters for LE message.

   Inputs:
      prod_id - Product ID.
      params - pointer to product dependent parameters for product.
      elev_index - elevation index.

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes:

********************************************************************/
void PD_write_prod( short prod_id, short *params, short elev_index ){

   int prod_code;
   char str[NUM_PROD_DEPENDENT_PARAMS][PARAM_LENGTH];

   prod_code = ORPGPAT_get_code( (int) prod_id );

   /* Do for all parameters. */
   Parm_to_macro( str, params ); 

   LE_send_msg( GL_INFO, 
          "--> Prod ID: %3d (%3d),  El#: %3d  Params:  %s  %s  %s  %s  %s  %s\n",
          prod_id, prod_code, elev_index, str[0], str[1], str[2], str[3], str[4], str[5] );

}


/**************************************************************************
   Description:

      This module builds the Prod_gen_status_pr_req list from a new 
      product request message.  

   Input: 
      len - length of the product request message.
      buf - product request message.
      call_from - who made the product request.
      line_ind - the line index if from RPS list request.

   Output: 
      requests - pointer to pointer to Prod_gen_status_pr_req list.

   Returns: 
      Returns the total number of product requests in the Product 
      Request Message.

   Notes:

**************************************************************************/
int PD_build_request_list( int len, char *buf, int call_from, int line_ind,
                           Prod_gen_status_pr_req **requests ){

   Pd_msg_header *tmp_hdr = NULL;
   Pd_request_products *req = NULL;
   Prod_gen_status_pr_req *prod_list = NULL; 

   int ind, i, j, k, num_reqs, rest_len; 
   int num_defined_params, el_parm, msg_len, vcp, vs_num;
   int total_num_reqs = 0; 

   msg_len = len;

   /* Cast buffer to message header structure. */
   tmp_hdr = (Pd_msg_header *) buf;

   /* If there are any product requests in this message, then ... */
   if( msg_len > ALIGNED_SIZE(sizeof(Pd_msg_header)) ){

      /* Fill in the new requests. 

         Extract the number of requests in this message and allocate 
         space for the requests. */ 
      num_reqs = tmp_hdr->n_blocks - 1;
      prod_list = MISC_malloc( num_reqs*sizeof(Prod_gen_status_pr_req) );

      /* Go to first product request. */
      req = (Pd_request_products *) ((char *) buf +
                                     ALIGNED_SIZE(sizeof(Pd_msg_header)));

      /* Subtract message header length from message. */
      rest_len = len - ALIGNED_SIZE(sizeof(Pd_msg_header));

      /* Get the current VCP and volume scan number.  This will be needed for 
         multiple elevation requests. */
      vcp = RRS_get_current_vcp_num();
      vs_num = RRS_get_volume_num(NULL);

      /* Set the total_num_reqs to the number of requests in message (does not include 
         multiple elevation requests). */
      total_num_reqs = num_reqs;

      /* Do For All product requests */
      k = -1;
      for (i = 0; i < num_reqs; i++){

         /* Not enough space to hold a request.  Exit for loop. */
         if (rest_len < sizeof(Pd_request_products))
            break;

         /* Validate the product ID. */
         if( ORPGPAT_prod_in_tbl( (int) req[i].prod_id ) < 0 ){

            /* Bad product ID.  Skip this request ..,.. go to the next request. */
            rest_len = rest_len - sizeof(Pd_request_products);
            LE_send_msg(GL_INFO,
                   "Unrecognized Request Product ID (%d).\n", req[i].prod_id);
            
            /* Decrement the total number of requests. */
            total_num_reqs--;

            continue;

         }

         /* Validate the number of products ... A value of 0 indicates the
            request is illegal. */
         if( req[i].num_products == 0 ){

            /* Skip this request ..,.. go to the next request. */
            rest_len = rest_len - sizeof(Pd_request_products);
            LE_send_msg(GL_INFO,
                   "Illegal or Empty Request.  Prod ID ?? %d.\n", req[i].prod_id);

            /* Decrement the total number of requests. */
            total_num_reqs--;

            continue;

         }

         /* Increment the number of products in the product list for this user. */
         k++;

         /* Initialize information about this request. */
         prod_list[k].num_products = req[i].num_products;
         prod_list[k].act_this_time = PS_DEF_PROD_ACTIVATED;
         prod_list[k].cnt = 0;

         memset( &prod_list[k].gen_status.from, 0, sizeof(From_t) );
         prod_list[k].gen_status.prod_id = req[i].prod_id;
         prod_list[k].gen_status.from.source = PS_DEF_FROM_REQUEST;

         if( call_from == PS_DEF_FROM_ONE_TIME )
            PD_clear_from_line_ind_list( &prod_list[k].gen_status.from );

         else
            PD_set_line_from_line_ind_list( &prod_list[k].gen_status.from, line_ind);

         prod_list[k].gen_status.vol_time = 0;
         prod_list[k].gen_status.vol_num = 0;
         prod_list[k].gen_status.gen_pr = req[i].req_interval;
         prod_list[k].gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
         prod_list[k].gen_status.req_num = req[i].seq_number;
         prod_list[k].gen_status.msg_ids = PGS_NOT_SCHEDULED;
         prod_list[k].next = NULL;

         /* Transfer the product dependent parameters. */
         for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++)
             prod_list[k].gen_status.params[j] = PARAM_UNUSED;

         /* Set only those parameters that are used by this product. */
         num_defined_params = ORPGPAT_get_num_parameters( (int) req[i].prod_id );
         for (j = 0; j < num_defined_params; ++j){
   
            ind = ORPGPAT_get_parameter_index( req[i].prod_id, j );
            if( ind >= 0 ){
   
               /* If product is final product, get from parameters in generation
                  message. */
               prod_list[k].gen_status.params[ind] = req[i].params[ind];

            }
    
         }
             
         /* Set elevation angle parameter, if defined for this product, 
            to the elevation angle from the current VCP. */
         if( (el_parm = ORPGPAT_elevation_based((int) req[i].prod_id)) >= 0 ){

            int elev, num_elevs = 0;
            short elev_req, vcp_elev_angles[MAX_ELEVATION_CUTS];
            short vcp_elev_inds[MAX_ELEVATION_CUTS];

            /* From the elevation angle product dependent parameter, determine
               the associated elevation index. */
            elev_req = req[i].params[el_parm];

            /* Get the elevation angles associated with elevation request.  On error or
               no elevations for elevation parameter, disable the request. */
            if( (num_elevs = ORPGPRQ_get_requested_elevations( vcp, elev_req,
                                                               (int) MAX_ELEVATION_CUTS,
                                                               vs_num, vcp_elev_angles,
                                                               vcp_elev_inds )) <= 0 ){

               if( num_elevs < 0 )
                  LE_send_msg( GL_ERROR, "Bad Elev Param (%x) For Prod ID %d\n",
                               elev_req, req[i].prod_id );

               else
                  LE_send_msg( GL_ERROR, "No Elevs For Param (%x) For Prod ID %d\n",
                               elev_req, req[i].prod_id );

               
               prod_list[k].act_this_time = PS_DEF_PROD_DEACTIVATED;
               prod_list[k].gen_status.elev_index = REQ_NOT_SCHEDLD;

            }

            /* If there are more than 1 request associated with this elevation parameter,
               then need to reallocate the product list. */
            if( num_elevs > 1 ){

               total_num_reqs += (num_elevs - 1);
               prod_list = realloc( prod_list, 
                             (size_t) (total_num_reqs * sizeof(Prod_gen_status_pr_req)));

               if( prod_list == NULL ){

                  LE_send_msg( GL_ERROR, "realloc Failed for %d Bytes\n",
                               total_num_reqs*sizeof(Prod_gen_status_pr_req) );

                  ORPGTASK_exit(GL_MEMORY);

               }

            }

            /* Fill in the additional elevation requests. */
            for( elev = 0; elev < num_elevs; elev++ ){

               if( elev != 0 ){

                  /* Increment the number of products. */
                  k++;

                  /* Save all information from previous request .... everything identical
                     except elevation parameter and "next" pointer. */
                  memcpy( &prod_list[k], &prod_list[k-1], sizeof(Prod_gen_status_pr_req) );

               }

               prod_list[k].gen_status.params[el_parm] = vcp_elev_angles[elev];

               /* Store the elevation index ... */
               prod_list[k].gen_status.elev_index = vcp_elev_inds[elev];
               prod_list[k].gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
               prod_list[k].gen_status.msg_ids = PGS_NOT_SCHEDULED;
               
               /* Tell operator about product request. */
               if (Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL)
                  PD_write_prod( prod_list[k].gen_status.prod_id, 
                                 prod_list[k].gen_status.params,
                                 prod_list[k].gen_status.elev_index );

            }

         }
         else{

            /* Set the elevation index to all elevations. */
            prod_list[k].gen_status.elev_index = REQ_ALL_ELEVS; 

            /* Some products require special request processing. */
            PD_special_request_processing( req[i].prod_id, 
                               &prod_list[k].gen_status.elev_index );
    
            /* Tell operator about product request. */
            if (Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL)
               PD_write_prod( prod_list[k].gen_status.prod_id, 
                              prod_list[k].gen_status.params, 
                              prod_list[k].gen_status.elev_index );

         }

         /* Subtract the size of a product request from the message size. 
            This gives the number of bytes remaining in message. */
         rest_len = rest_len - sizeof(Pd_request_products);

      }

      /* Set all link so array can be treated as linked list. */
      for( ind = 1; ind < total_num_reqs; ind++ )
         prod_list[ind-1].next = &prod_list[ind];

      if( total_num_reqs > 0 )
         prod_list[total_num_reqs-1].next = NULL;

   }

   /* If there are no valid requests in the request message,
      set the number of requests to 0 and free the product list. */
   if( total_num_reqs <= 0 ){

      free(prod_list);
      prod_list = NULL;
      total_num_reqs = 0;

   }

   /* Prepare for return to caller. */
   *requests = prod_list;
   return(total_num_reqs);

/* END of PD_build_request_list() */
}

/**************************************************************************

   Description:
      Validates the product request message.  

   Inputs:
      buf - the buffer containing the ICD format product request message.
      len - the length of the product request message.
      call_from - whether an RPS list request or one-time request.

   Outputs:

   Returns:
      PS_DEF_FAILURE if something wrong with product request messsage,
      or PS_DEF_SUCCESS if OK.

   Notes:

**************************************************************************/
int PD_validate_request_message( char *buf, int len, int call_from ){

   Pd_msg_header *msg_hdr;

   /* Cast buffer to message header structure. */
   msg_hdr = (Pd_msg_header *) buf;

   /* Validate the number of blocks in this message.  A valid message should
      have at least 2 blocks if message contains requests.  */
   if( (len > ALIGNED_SIZE(sizeof(Pd_msg_header))) 
                      &&
       (msg_hdr->n_blocks < 2) ){

         LE_send_msg( GL_INFO, 
                      "Product Request Has Fewer Than Minimum # Of Blocks (%d)\n",
                      msg_hdr->n_blocks );
         return (PS_DEF_FAILED);

   }

   /* Validate the size of this message. */
   if (len < ALIGNED_SIZE(sizeof(Pd_msg_header)) + (msg_hdr->n_blocks - 1) *
          sizeof(Pd_request_products)){

         LE_send_msg(GL_ERROR, "Product Request Length Error\n");
         return (PS_DEF_FAILED);

   }

   /* Validate the number of products matches the correct number of blocks. */
   if ( (len - ALIGNED_SIZE(sizeof(Pd_msg_header))) / sizeof(Pd_request_products)
          != msg_hdr->n_blocks - 1){

         LE_send_msg( GL_ERROR,
                      "Product Request Length (%d) Not Consistent With # Blocks (%d)\n",
                      len, msg_hdr->n_blocks );
         return (PS_DEF_FAILED);

   }

   /* -1 is a special line index for one time request, later should be changed to 
      const define value. The following two checks verify that if line index is -1 
      that request is from ps_onetime. */
   if( (msg_hdr->line_ind != -1 )
                && 
       (call_from == PS_DEF_FROM_ONE_TIME) ){

      LE_send_msg( GL_ERROR, "Line_id Error %d\n", msg_hdr->line_ind);
      return (PS_DEF_FAILED);

   }

   if (msg_hdr->line_ind == -1 && call_from != PS_DEF_FROM_ONE_TIME){

      LE_send_msg( GL_INFO, "Call_from Error %d\n", msg_hdr->line_ind);
      return (PS_DEF_FAILED);

   }

   return(PS_DEF_SUCCESS);

}

/***** Local Functions *****/
/******************************************************************************

   Description:
      Given the line id, returns the line list index and the bit within
      the line index word.

   Inputs:
      line_id - the narrowband line id.

   Outputs:
      line_list_index - line list index.  The line list begins at index 1.
      line_bit - bit within line list element at index corresponding to
                 line id.

   Returns:
      There is no return value define for this function.

   Notes:
      We assume that an unsigned integer is 32 bits.
 
******************************************************************************/
static void Get_line_ind_list_index( unsigned int line_id, int *line_list_index, 
                                     unsigned int *line_bit ){

   *line_list_index = line_id / 32; 
   *line_bit = line_id % 32;

   if( (*line_list_index >= PS_DEF_MAX_USERS) || (*line_list_index < 0) ){

      LE_send_msg( GL_INFO, "Invalid Line List Index %d For Line %d\n", 
                   *line_list_index, line_id );
      ORPGTASK_exit( GL_ERROR );

   }

}

/**************************************************************************

   Description:
      Free memory associated with lind_id in Line List.
 
   Input: 
      line_id - Line ID

   Output:

   Returns:
      Always returns PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
static int Release_id_line(int line_id){

   Line *tmp_line = NULL, *prev_line = NULL;
   Prod_gen_status_pr_req *next_req = NULL, *tmp_req = NULL;

   if (Line_list == NULL)
      return( PS_DEF_SUCCESS );
    
   tmp_line = prev_line = (Line *) Line_list;
   while (tmp_line != NULL){

      /* Match on line ID. */
      if (tmp_line->line_ind == line_id){

         /* Is this line the first line in the list? */
         if (tmp_line == Line_list){

            Line_list = tmp_line->next;
            if( tmp_line != NULL ){

               /* Free all allocated memory. */
               tmp_req = tmp_line->prod_list;
               while (tmp_req != NULL){

                  next_req = tmp_req->next;
                  free(tmp_req);
                  tmp_req = next_req;

               }

               if( tmp_line->rps_list != NULL )
                  free(tmp_line->rps_list);

               
               free(tmp_line);

            }

            tmp_line = NULL;
            tmp_line = prev_line = Line_list;
            continue;

         }
         else{

            prev_line->next = tmp_line->next;
            if( tmp_line != NULL ){

               /* Free all allocated memory. */
               tmp_req = tmp_line->prod_list;
               while (tmp_req != NULL){

                  next_req = tmp_req->next;
                  free(tmp_req);
                  tmp_req = next_req;

               }

               if( tmp_line->rps_list != NULL )
                  free(tmp_line->rps_list);

               free(tmp_line);

            }

            tmp_line = NULL;
            tmp_line = prev_line->next;
            continue;
         }

      }

      prev_line = tmp_line;
      tmp_line = tmp_line->next;

   /* End of while loop. */
   }      

   /* Return value undefine or unused. */
   return( PS_DEF_SUCCESS );

/* END of Release_id_line() */
}

/*********************************************************************

   Description:
      Returns character string (plain language description) associated
      with input product dependent parameter.

   Inputs:
      param - product dependent parameter.

   Outputs:

   Returns:
      Character string describing input product dependent parameter.

   Notes:

*********************************************************************/
static void Parm_to_macro( char str[][PARAM_LENGTH], short *parm ){

   static char *Unused =    "   UNU";
   static char *Anyvalue =  "   ANY";
   static char *Allvalues = "   ALL";
   static char *Allexist =  "   EXT";
   static char *Algset =    "   ALG";
   int i;

   for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ ){

   
      if( parm[i] == PARAM_UNUSED )
         memcpy( str[i], Unused, PARAM_LENGTH );

      else if( parm[i] == PARAM_ANY_VALUE )
         memcpy( str[i], Anyvalue, PARAM_LENGTH );

      else if( parm[i] == PARAM_ALG_SET )
         memcpy( str[i], Algset, PARAM_LENGTH );

      else if( parm[i] == PARAM_ALL_VALUES )
         memcpy( str[i], Allvalues, PARAM_LENGTH );

      else if( parm[i] == PARAM_ALL_EXISTING )
         memcpy( str[i], Allexist, PARAM_LENGTH );

      else{ 

         sprintf( str[i], "%6d", parm[i] );
         str[i][6] = '\0';

      }

   }

   return;

}

/*******************************************************************
                                                                                                         
   Description:
      This module handles special request processing.
                                                                                                         
      For base velocity grid, schedule on elevation index 1.
                                                                                                         
   Inputs:
      prod_id - product ID
      *req_elev_ind - the requested elevation index
                                                                                                         
   Outputs:
                                                                                                         
   Returns:
                                                                                                         
   Notes:
                                                                                                         
********************************************************************/
void PD_special_request_processing( short prod_id, short *req_elev_ind ){
                                                                                                         
   /* Process according to product ID. */
   switch( prod_id ){

      case BASEVGD:
      {
                                                                                                         
         *req_elev_ind = 1;
         break;
                                                                                                         
      }
      default:
         break;
                                                                                                         
   /* End of "switch" statement. */
   }
                                                                                                         
   if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
      LE_send_msg( GL_INFO, "Special Request Processing: Prod ID %d elev_ind Set To  %d\n",
                   prod_id, *req_elev_ind );
                                                                                                         
/* End of PD_special_request_processing() */
}

