/**************************************************************************

   Module:  ps_vol_prod_list.c

   Description:
      This file provides the source for the Schedule Routine Products
      routines associated with the Volume Product List (Vol_list).

      Functions that are public are defined at the top of this file and 
      are identified with a prefix of "PSVPL_".

      Functions that are private to this file are defined following the 
      definition of the public functions.  (Search for pattern "Private 
      Functions").


   Assumptions:

**************************************************************************/

/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2012/06/19 20:23:44 $ $Id: ps_vol_prod_list.c,v 1.51 2012/06/19 20:23:44 steves Exp $
 * $Revision: 1.51 $ $State: Exp $
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <infr.h>

#define PS_VOL_PROD_LIST
#include <ps_globals.h>
#undef PS_VOL_PROD_LIST

#include <ps_def.h>

/* Constant Definitions/Macro Definitions/Type Definitions */

#define FOUND_LINE       -204
#define NOT_FOUND_LINE   -205
#define TIMEOUT            60

/* Real-time generation status of a product. Contains PGS_LIST_LENGTH of 
   generation status history (associated with the msg_ids field.) */
typedef struct GEN_STATUS_PS_NEXT{ 

   prod_id_t prod_id;                       /* product buffer id */

   short params[NUM_PROD_DEPENDENT_PARAMS]; /* product dependent parameter */

   short elev_index;                        /* elevation index corresponding to
                                               elevation parameter, if defined. */

   short gen_pr;                             /* generation period in number 
                                               of volumes */

   short spare;                              /* pad character. */

   unsigned int schedule;                   /* PGS_SCHEDULED, PGS_NOT_SCHEDULED or
                                               PGS_TASK_NOT_RUNNING */

   LB_id_t msg_ids[PGS_LIST_LENGTH];        /* list of the latest product message IDs;
                                               Each entry corresponds to a volume with
                                               the first entry conresponding to the 
                                               latest (the current) volume. If a product
                                               is not available, a special value (PGS_UNKNOWN...
                                               as defined above) is used to indicate the
                                               possible cause. */

   struct GEN_STATUS_PS_NEXT *next;

} Prod_gen_status_next;


/* Static Globals */

/*  Volume Product List (VPL) */
typedef struct VOL_PROD_LIST{

   int wx_mode;                     /* weather mode. */
   time_t vol_time;                 /* volume scan (RDA) time. */
   unsigned int vol_num;            /* volume number. */
   time_t local_vol_time;           /* local volume (UNIX) time. */
   int vcp_num;                     /* volume coverage pattern number. */
   Prod_gen_status_pr *prod_list;   /* Product Generation Status list. */

} vol_prod_list_t;

#define MAX_NUM_DEPTH 8

/* This list tracks product generation status for the current volume
   scan and the seven previous volume scans. */
static vol_prod_list_t Vol_list[MAX_NUM_DEPTH]; 

/* Describes how Vol_list got initialized. */
#define VOL_LIST_INIT_DEFAULTS          1
#define VOL_LIST_INIT_PROD_STATUS       2
static int Vol_list_initialized;

/* Flag for writing into prod_status.lb. Need_update==1 if write into
   prod_status.lb. Need_update==0 if NO writing into prod_status.lb. */
static int Need_update = 0;

/*
 * Last output time for output prod_status.lb. The purpose of using this:
 * Lower the frequency of writing into prod_status.lb. The hightest frequency
 * is to write to prod_status.lb is once every second.
 */
static time_t Last_output_time = 0;

/*
 * Static Function Prototypes
 */
static int Shift_one_upper_vol_prod_list(void);
static int Add_in_output_list( Prod_gen_status_pr *prod,
                               int vol_num, Prod_gen_status_next **list );
static int Add_to_volume_list( Prod_gen_status_pr *prod, int vol_num );
static unsigned int Failed_reason( Prod_gen_status_pr *prod_status );
static int Free_prod_list( Prod_gen_status_pr **prod_list );
static int Get_prod_status_len( Prod_gen_status_next *list );
static void Put_msg_id_pd_gen_status( Prod_gen_status_pr *prod,
                                      Prod_gen_status_pr *prod_new,
                                      int from );
static void Put_timeout_prev_vol_list( Prod_gen_status_pr *prod );


/***** Public Functions Start Below ... *****/

/**************************************************************************
   Description:
      Initializes the product generation status for the current volume
      scan.  Uses the default product generation table to build Vol_list[0].
      Vol_list[0] is the product generation status for the current volume
      scan.

   Input:

   Output: 
      Initialized Vol_list[0].

   Returns:
      Always returns PS_DEF_SUCCESS.

   Notes:
      Process termination if Default Product Generation List is undefined.
      This condition should never happen.

**************************************************************************/
int PSVPL_init_vol_list_for_cur_vol( void ){

   Pd_prod_gen_tbl_entry *Default_prod_gen_list = NULL;
   Pd_prod_gen_tbl_entry *tmp_default = NULL;
   Task_prod_chain *task_entry = NULL;
   Prod_wth_only_id_next *gen_prod_list = NULL;
   int j;

   /* Get address of Default Product Generation List. */
   Default_prod_gen_list = PD_get_default_prod_gen_list();

   /* Check if the Default Product Generation List has been defined. */
   if (Default_prod_gen_list == NULL){

      /* The Default Product Generation List is empty.  This should never
         happend. */
      LE_send_msg(GL_ERROR, "Default_prod_gen_list == NULL\n");
      ORPGTASK_exit( GL_ERROR );

   }

   /* Set temporary pointer to start of Default Product Generation List */
   tmp_default = Default_prod_gen_list;

   /* Step through each entry in the Default Product Generation List */
   while (tmp_default != NULL){

      /* Automatic variables ... */
      Prod_gen_status_pr tmp_gen_status_pr;
      int task_state;

      /* Initialize product generation status to zeroes (0) */
      (void) memset(&tmp_gen_status_pr, 0, sizeof(Prod_gen_status_pr));

      /* Move product id, generation period fields, product dependent
         parameters, and elevation index. */
      tmp_gen_status_pr.gen_status.prod_id = tmp_default->prod_id;
      tmp_gen_status_pr.gen_status.gen_pr = tmp_default->gen_pr;
      tmp_gen_status_pr.gen_status.elev_index = tmp_default->elev_index;
     
      for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++)
         tmp_gen_status_pr.gen_status.params[j] = tmp_default->params[j];

      tmp_gen_status_pr.gen_status.from.source = PS_DEF_FROM_DEFAULT;
      tmp_gen_status_pr.gen_status.vol_time = 0;
      tmp_gen_status_pr.gen_status.req_num = tmp_default->req_num;
      tmp_gen_status_pr.next = NULL;

      /* Get the status of this task. */
      task_entry = PSTS_what_is_task_status( tmp_default->gen_task, &task_state );

      /* For now, it is assumed a task state of PGS_UNKNOWN means 
         the task is running. */
      if( task_state == PGS_UNKNOWN ){

         /* Set the product scheduling to both SCHEDULED and SCHEDULED BY DEFAULT. */
         tmp_gen_status_pr.gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_DEFAULT);

         /* If this is the initial volume scan, set message id to indicate
            product was requested but not scheduled.  No products are 
            scheduled in initial volume scan, regardless of reason. */
         if( RRS_initial_volume() )
            tmp_gen_status_pr.gen_status.msg_ids = PGS_REQED_NOT_SCHED;

         else
            tmp_gen_status_pr.gen_status.msg_ids = PGS_SCHEDULED;

         /* Check for product scheduling disabled. */
         if( task_entry != NULL ){

            gen_prod_list = task_entry->gen_prod_list;
            while( gen_prod_list != NULL ){

               if( gen_prod_list->prod_id == tmp_gen_status_pr.gen_status.prod_id ){

                  if( gen_prod_list->schedule == PRODUCT_DISABLED ){
                     
                     tmp_gen_status_pr.gen_status.schedule = PGS_SCH_NOT_SCHEDULED;
                     tmp_gen_status_pr.gen_status.msg_ids = PGS_PRODUCT_DISABLED;

                  }

                  break;
               }

               gen_prod_list = gen_prod_list->next;

            } /* End of while( gen_prod_list != NULL ) */

         } /* End of if( task_entry != NULL ) */

      }
      else{

         /* Task is not running for some reason.  Set product scheduling to NOT 
            SCHEDULED. */
         tmp_gen_status_pr.gen_status.schedule = PGS_SCH_NOT_SCHEDULED;

         /* If this is the initial volume scan, set message id to indicate
            product was requested but not scheduled.  No products are 
            scheduled in initial volume scan, regardless of reason. 
            Otherwise, set message id to the task state. */
         if( RRS_initial_volume() )
            tmp_gen_status_pr.gen_status.msg_ids = PGS_REQED_NOT_SCHED;

         else
            tmp_gen_status_pr.gen_status.msg_ids = task_state;

      }

      /* Add this product to Volume Status for the current volume. */
      PSVPL_add_prod_gen_status(&tmp_gen_status_pr, PS_DEF_CURRENT_VOLUME);

      /* Go to next product. */
      tmp_default = tmp_default->next;

   /* While entries in Default Product Generation List */
   }      

   PSVPL_init_vpl_curvol_wxmode_voltime();

   return( PS_DEF_SUCCESS );

/* END of PSVPL_init_vol_list_for_cur_vol() */
}
/**************************************************************************
   Description: 
      Add prod_status into Vol_list[vol_num].prod_list.

   Input: 
      prod_status - pointer to product status data.
      vol_num_ind - index into Vol_list to place product status data.

   Output: 

   Returns: 
      Returns 
         PS_DEF_FAILED if pointer to product status data is not valid, or
         PS_DEF_SUCCESS if product associated with the product status data 
                        is already found in Vol_list, or
         PS_DEF_FOUND_IT if product associated with the product status 
                         data was not found in Vol_list but was successfully
                         added to it, or
         PS_DEF_NOT_FOUND if product associcated with the product status
                          could not be successfully added to Vol_list.
      
   Notes:

**************************************************************************/
int PSVPL_add_prod_gen_status(Prod_gen_status_pr *prod_status, 
                              int vol_num_ind){

   Prod_gen_status_pr *tmp_prod;
   int flag, r_bit;

   /* Verify the product status pointer is valid. */
   if (prod_status == NULL){

      LE_send_msg(GL_ERROR, "Attempting To Add NULL Product Status To Vol_list.\n");
      return( PS_DEF_FAILED );

   }

   /* Get product list data from Vol_list */
   tmp_prod = (Prod_gen_status_pr *) Vol_list[vol_num_ind].prod_list;

   /* Initialize flag to product not found in Vol_list. */
   flag = PS_DEF_NOT_FOUND;

   /* Do for all product in the product list */
   while (tmp_prod != NULL){

      /* Go to next product in Vol_list if product IDs do not match.  This
         short-cuts PD_tell_same_prod(). */
      if( tmp_prod->gen_status.prod_id != prod_status->gen_status.prod_id ){

            tmp_prod = tmp_prod->next;
            continue;

      }

      /* PD_tell_same_prod returns PS_DEF_SUCCESS if current product is 
         alredy defined in the product status.  The match is based on product 
         ID, product dependent parameters, and elevation index. */
      if (PD_tell_same_prod(&tmp_prod->gen_status,
                            &prod_status->gen_status) == PS_DEF_SUCCESS){

         /* If here, the product is already in the product generation status */

         /* Check if product was scheduled by user request. */
         r_bit = prod_status->gen_status.schedule & PGS_SCH_BY_REQUEST;

         /* If scheduled by user request or is a one-time request, then ... */
         if ((r_bit != 0) || (prod_status->gen_status.from.source == PS_DEF_FROM_ONE_TIME)){

            /* Set the PGS_SCH_BY_REQUEST bit */
            tmp_prod->gen_status.schedule |= (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
            tmp_prod->gen_status.from.source |= prod_status->gen_status.from.source;
            PD_merge_from_line_ind_list( &tmp_prod->gen_status.from, &prod_status->gen_status.from );

            /* If product list indicates the product was requested but not scheduled, 
               then set the msg_id to PGS_SCHEDULED. */
            if (tmp_prod->gen_status.msg_ids == PGS_REQED_NOT_SCHED
                                  &&
                prod_status->gen_status.msg_ids == PGS_SCHEDULED){
          
               /* Only set to schedule if not the initial volume scan. */
               if( Vol_list[vol_num_ind].vol_num != RRS_initial_volume_number() )
                  tmp_prod->gen_status.msg_ids = PGS_SCHEDULED;

               else
                  tmp_prod->gen_status.msg_ids = PGS_REQED_NOT_SCHED;

            }

            /* Exit module. */
            return ( PS_DEF_FOUND_IT );

         }

         /* If falls through to here, check if product is scheduled only 
            by default, then .... */         
         if (prod_status->gen_status.from.source == PS_DEF_FROM_DEFAULT){

            /* Set the PGS_SCH_BY_DEFAULT bit */
            tmp_prod->gen_status.schedule |= (PGS_SCH_SCHEDULED | PGS_SCH_BY_DEFAULT);
            tmp_prod->gen_status.from.source |= prod_status->gen_status.from.source;
            PD_merge_from_line_ind_list( &tmp_prod->gen_status.from, 
                                         &prod_status->gen_status.from );

            /* Exit module. */
            return ( PS_DEF_FOUND_IT );

         }

         /* If from indicates the product is from a product generation message, 
            then ... */
         if (prod_status->gen_status.from.source == PS_DEF_FROM_GEN_PROD){

            Put_msg_id_pd_gen_status( tmp_prod, prod_status, PS_DEF_FROM_GEN_PROD);
            Put_timeout_prev_vol_list(prod_status);

            /* Exit module. */
            return (PS_DEF_FOUND_IT);

         }
         else{

            /* If the task is not configured, don't report .... otherwise messages fill
               up the Error Log. */
            if( prod_status->gen_status.msg_ids != PGS_TASK_NOT_CONFIG ){

               LE_send_msg( GL_ERROR, "Can Not Add Product ID (%d) To Product Status.\n",
                            prod_status->gen_status.prod_id );
               LE_send_msg( GL_ERROR, "--->vol_num: %d, elev_index: %d, from.source: %d, msg_id: %d\n",
                            prod_status->gen_status.vol_num, prod_status->gen_status.elev_index,
                            prod_status->gen_status.from.source, prod_status->gen_status.msg_ids );
            }

            /* Go to next product in Vol_list. */
            tmp_prod = tmp_prod->next;

         }

      }
      else{

         /* Go to next product in Vol_list. */
         tmp_prod = tmp_prod->next;

      }
       
   /* while products in product list for a given volume */
   }  

   /* If product not found in volume list, then ....  */
   if (flag == PS_DEF_NOT_FOUND){

      if( Psg_cur_wx_mode == PS_DEF_WXMODE_UNKNOWN ){

         Vol_list[PS_DEF_CURRENT_VOLUME].wx_mode = Psg_wx_mode_beginning;
         Vol_list[PS_DEF_CURRENT_VOLUME].vol_time = 
                  RRS_get_volume_time( &(Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time));
         Vol_list[PS_DEF_CURRENT_VOLUME].vol_num = 
                  RRS_get_volume_num( &(Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time));
         Vol_list[PS_DEF_CURRENT_VOLUME].vcp_num = RRS_get_current_vcp_num( );

      }

      /* Add this product to Vol_list. */
      if( Add_to_volume_list(prod_status, vol_num_ind) == PS_DEF_FAILED)
         return PS_DEF_FOUND_IT;
       
      return flag;

   }

   return( PS_DEF_FOUND_IT );

/* END of PSVPL_add_prod_gen_status() */
}

/**************************************************************************
   Description: 
       Removes the narrowband users requests from the Vol_list product list.
       By "remove" we mean the products originally scheduled on behalf
       of this user are no longer scheduled.

   Input:
       ind - line index for user.

   Output: 

   Returns: 
       Always returns PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
int PSVPL_delete_line_ind_vol_list_latest(int ind){

   Prod_gen_status_pr *tmp_pr = Vol_list[PS_DEF_CURRENT_VOLUME].prod_list;

   /* Go through product list for current volume, looking for line ind. */
   while (tmp_pr != NULL){

      /* "from" field indicates product scheduled on behalf of this user. */
      if( (PD_test_line_from_line_ind_list(&tmp_pr->gen_status.from, ind)) != 0){

         /* If "from" field indicates product was scheduled based only on 
            this user, mark product as not scheduled in Volume list product
            list. */
         PD_clear_line_from_line_ind_list( &tmp_pr->gen_status.from, ind );

         if( tmp_pr->gen_status.from.source < PS_DEF_FROM_DEFAULT ){

            /* Mark product as not scheduled. */
            tmp_pr->gen_status.schedule = PGS_SCH_NOT_SCHEDULED;

         }
         else{

            /* Product must be scheduled owing to some other user (as well) 
               or by default. */
            if (tmp_pr->gen_status.from.source >= PS_DEF_FROM_ONE_TIME)
               tmp_pr->gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_REQUEST);
             
            if( (tmp_pr->gen_status.from.source & PS_DEF_FROM_DEFAULT) != 0){

               /* If we remove this user from the "from" list, if the "from"
                  list only indicates PS_DEF_FROM_DEFAULT, we need to change 
                  the scheduling information to exclude scheduling by request. */
               if( (tmp_pr->gen_status.from.source & PS_DEF_FROM_DEFAULT) == PS_DEF_FROM_DEFAULT )
                  tmp_pr->gen_status.schedule = (PGS_SCH_SCHEDULED | PGS_SCH_BY_DEFAULT);

               else
                  tmp_pr->gen_status.schedule |= PGS_SCH_BY_DEFAULT;

            }
             
            if( tmp_pr->gen_status.from.source < PS_DEF_FROM_DEFAULT)
               tmp_pr->gen_status.schedule = PGS_SCH_NOT_SCHEDULED;
             
         }

      }

      /* Prepare for next product in product list. */ 
      tmp_pr = tmp_pr->next;

   /* End of "while" loop. */
   }      

   return PS_DEF_SUCCESS;

/* END of PSVPL_delete_line_ind_vol_list_latest() */
}

/**************************************************************************
   Description: 
      Free Vol_list[ind].prod_list.  This is a linked list so each 
      list node needs to be freed separately.

   Input: 
      ind - volume scan index.  Index 0 corresponds to the current 
            volume scan.

   Output: 

   Returns: 

   Notes:
 **************************************************************************/
int PSVPL_free_prod_list_according_to_ind(int ind){

   Prod_gen_status_pr *tmp_prod;

   /* First validate the index into Vol_list */
   if (ind < PS_DEF_CURRENT_VOLUME || ind > MAX_NUM_DEPTH - 1)
      return( PS_DEF_FAILED );

   /* If product list is empty, there is nothing to do so return. */
   if (Vol_list[ind].prod_list == NULL)
      return( PS_DEF_SUCCESS );
    
   /* If here, index was valid and product list contains entries.
      Remove them. */
   tmp_prod = Vol_list[ind].prod_list;
   while (tmp_prod->next != NULL){

      /* Free current entry. */
      Vol_list[ind].prod_list = tmp_prod->next;
      free(tmp_prod);
      tmp_prod = NULL;

      /* Go to next entry in list. */
      tmp_prod = Vol_list[ind].prod_list;

   }

   /* At end of list, free last entry. */
   if (Vol_list[ind].prod_list != NULL){

      free(Vol_list[ind].prod_list);
      Vol_list[ind].prod_list = NULL;

   }

   return( PS_DEF_SUCCESS );

/* END of PSVPL_free_prod_list_according_to_ind() */
}

/**************************************************************************
   Description:
      Initialize the Volume Product List (VPL) current volume
      weather mode, volume time, and volume number values.

   Input:

   Output: 

   Returns: 

   Notes:

**************************************************************************/
void PSVPL_init_vpl_curvol_wxmode_voltime(){

   Vol_list[PS_DEF_CURRENT_VOLUME].wx_mode = RRS_get_current_weather_mode();
   Vol_list[PS_DEF_CURRENT_VOLUME].vol_time = 
               RRS_get_volume_time(&(Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time));
   Vol_list[PS_DEF_CURRENT_VOLUME].vol_num = 
               RRS_get_volume_num(&(Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time));
   Vol_list[PS_DEF_CURRENT_VOLUME].vcp_num = RRS_get_current_vcp_num();

   return;

/* END of PSVPL_init_vpl_curvol_wxmode_voltime() */
}

/**************************************************************************
   Description: 
      Initialize global variables defined in this source file
      or in the global header file.  This is the initialization
      routine for ps_vol_prod_list module.

   Input: 

   Output:

   Returns: 

   Notes:

**************************************************************************/
void PSVPL_initialize(void){

   int ret, vol_num_ind, i;
   int *buf = NULL;

   Psg_check_failed_ones_for_last_vol = 0; 	/* defined in ps_globals.h */

   ret = ORPGDA_read( ORPGDAT_PROD_STATUS, &buf, LB_ALLOC_BUF, 
                      PROD_STATUS_MSG );
   if( ret <= 0 ){

      if( Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL )
         LE_send_msg( GL_INFO, "Initializing Product Status With Defaults\n" );
 
      /* Do for all possible entries in Vol_list. */
      for (i = PS_DEF_CURRENT_VOLUME; i < MAX_NUM_DEPTH; ++i){

         /* Initialize Vol_list entry. */
         Vol_list[i].wx_mode = -1;
         Vol_list[i].vol_time = 0;
         Vol_list[i].vol_num = 0;
         Vol_list[i].vcp_num = -1;
         Vol_list[i].local_vol_time = 0;
         Vol_list[i].prod_list = NULL;

      }

      Vol_list_initialized = VOL_LIST_INIT_DEFAULTS;
      return;

   }
   else{

      Prod_gen_status_header *hdr = NULL;
      Prod_gen_status *array = NULL;

      if( Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL )
         LE_send_msg( GL_INFO, "Initializing Product Status From LB\n" );

      hdr = (Prod_gen_status_header *) buf;

      /* Do for all possible entries in Vol_list. */
      for (i = PS_DEF_CURRENT_VOLUME; i < MAX_NUM_DEPTH; ++i){

         /* Initialize Vol_list entry. */
         Vol_list[i].wx_mode = -1;
         Vol_list[i].vol_time = 0;
         Vol_list[i].vol_num = 0;
         Vol_list[i].vcp_num = -1;
         Vol_list[i].local_vol_time = 0;
         Vol_list[i].prod_list = NULL;

      }

      /* Construct the Volume List header. */
      for( i = 0; i < hdr->vdepth; i++ ){

         Vol_list[i].wx_mode = (int) hdr->wx_mode[i];

         /* Make the local and RDA volume times the same.  The product status
            does not carry the local volume time.  */
         Vol_list[i].vol_time = (int) hdr->vtime[i];
         Vol_list[i].local_vol_time = (int) hdr->vtime[i];

         Vol_list[i].vol_num = (int) hdr->vnum[i];
         Vol_list[i].vcp_num = (int) hdr->vcpnum[i];

      /* End of "for" loop. */
      }

      /* Construct the Volume List ..... Do For All products in product
         status and all volume scans. */
      array = (Prod_gen_status *) ( ((char*) buf) + hdr->list );
      for( i = 0; i < hdr->length; i++ ){

         Prod_gen_status_pr gen_status;

         /* Initialize all elements to 0. */
         memset( &gen_status, 0, sizeof(Prod_gen_status_pr) );

         /* Set pertinent information from product status. */
         gen_status.gen_status.prod_id = (prod_id_t) array[i].prod_id;
         gen_status.gen_status.gen_pr = (char) array[i].gen_pr;
         gen_status.gen_status.schedule = (unsigned int) array[i].schedule;
  
         memcpy( (void *) gen_status.gen_status.params, (void *) array[i].params, 
                 NUM_PROD_DEPENDENT_PARAMS*sizeof(short) );

         gen_status.gen_status.elev_index = array[i].elev_index;
             
         gen_status.gen_status.from.source = PS_DEF_FROM_INIT;

         /* Fill in volume time, volume number, and message ID information. */
         for( vol_num_ind = 0; vol_num_ind < hdr->vdepth; vol_num_ind++ ){

            gen_status.gen_status.vol_time = (time_t) hdr->vtime[vol_num_ind];
            gen_status.gen_status.vol_num = (unsigned int) hdr->vnum[vol_num_ind];

            gen_status.gen_status.msg_ids = (LB_id_t) array[i].msg_ids[vol_num_ind];

            /* If the message ID is not a valid ID, then since this is the initial
               volume scan, set the message ID to requested but not scheduled. 

               NOTE:  Message ID is unsigned int so need to convert and check if
                      sign bit is set. */
            if( (vol_num_ind == 0) && ( ((int) gen_status.gen_status.msg_ids) <= 0) )
               gen_status.gen_status.msg_ids = PGS_REQED_NOT_SCHED;
            
            /* The function adds "gen_status" structure to Volume List
               for volume number "vol_num". */
            Add_to_volume_list( &gen_status, (int) vol_num_ind );

         /* End of "for" loop. */
         }

      /* End of "for" loop. */
      }

      Vol_list_initialized = VOL_LIST_INIT_PROD_STATUS;

      /* Free allocated memory. */
      if( buf != NULL )
         free(buf);

      return;
   }

/* END of PSVPL_initialize() */
}

/**************************************************************************
    Description:

       Goes through all products in the product list for the current volume.
       If the source indicates the request was only from one-time and 
       the msg_id indicates the product was requested but not scheduled,
       mark product as not scheduled.

    Input: 

    Output: 

    Returns: 

    Notes:
 **************************************************************************/
void PSVPL_mark_one_time_req_last_vol_not_used_cur_vol(void){

   Prod_gen_status_pr *tmp_pr_prod;

   tmp_pr_prod = Vol_list[PS_DEF_CURRENT_VOLUME].prod_list;
   while (tmp_pr_prod != NULL){

      if ((tmp_pr_prod->gen_status.from.source == PS_DEF_FROM_ONE_TIME)
                             && 
          (tmp_pr_prod->gen_status.msg_ids == PGS_REQED_NOT_SCHED))
         tmp_pr_prod->gen_status.msg_ids = PGS_NOT_SCHEDULED;


      /* Prepare for next product in list. */
      tmp_pr_prod = tmp_pr_prod->next;

   }

   return;

/* END of PSVPL_mark_one_time_req_last_vol_not_used_cur_vol() */
}

/**************************************************************************
   Description: 
      Tell to which volume in the volume list this newly generated 
      product belongs.

   Input: 
      prod_status - Product status structure for product in question.

   Output: 

   Returns: 
      Returns index into volume list for which the product belongs.
      If volume number for product is not one of the volumes in the 
      volume list, then PS_DEF_FAILED is returned.

   Notes:

***************************************************************************/
int PSVPL_get_vol_list_index( Prod_gen_status_pr *prod_status ){

   int type, i;

   if (prod_status->gen_status.vol_time == 0)
      return PS_DEF_CURRENT_VOLUME;

   if (prod_status->gen_status.vol_num == 0)
      return PS_DEF_CURRENT_VOLUME;

   /*
     If volume number in product matches current volume number, return
     index of current volume in volume list.
   */
   if (prod_status->gen_status.vol_num == Vol_list[PS_DEF_CURRENT_VOLUME].vol_num)
      return PS_DEF_CURRENT_VOLUME;

   /*
      Find match on volume sequence number. 
   */
   for (i = PS_DEF_CURRENT_VOLUME; i < MAX_NUM_DEPTH; i++){

      if (Vol_list[i].vol_num == 0)
         break;

      if (prod_status->gen_status.vol_num == Vol_list[i].vol_num)
         return i;

   }

   /*
     Check if this product is generated ON_DEMAND or ON_REQUEST.   If either,
     then allow the product to be published in the current volume scan if and
     only if the volume number in the product status is the upcoming volume scan.
     This might happen if the product generation is not tied to a volume scan 
     and the product was generated after Scan Summary data was published by pbd
     but before ps_routine serviced the START_OF_VOLUME event for the current volume.
   */
   type = ORPGPAT_get_type( prod_status->gen_status.prod_id );
   if( (type == TYPE_ON_DEMAND) || (type == TYPE_ON_REQUEST) ){

      if( (prod_status->gen_status.vol_num - 1) == Vol_list[PS_DEF_CURRENT_VOLUME].vol_num ){

         LE_send_msg( GL_INFO, "Prod %d Generated in Volume %d But Status Added to Volume %d\n",
                      prod_status->gen_status.prod_id, prod_status->gen_status.vol_num,
                      prod_status->gen_status.vol_num - 1 );
         return PS_DEF_CURRENT_VOLUME;

      }

   }

   /* If here, we cannot assign this product to a volume scan that we are currently
      tracking.  Report Error. */
   LE_send_msg( GL_ERROR, 
                "No Match Error (Prod ID (%d) Vol Num (%d) Not In Vol List.)\n",
                prod_status->gen_status.prod_id, prod_status->gen_status.vol_num);
   LE_send_msg( GL_ERROR, 
                "--->Vol_list[PS_DEF_CURRENT_VOLUME].vol_num: %d\n",
                Vol_list[PS_DEF_CURRENT_VOLUME].vol_num );

   return PS_DEF_FAILED;

/* END of PSVPL_get_vol_list_index() */
}

/**************************************************************************
   Description:
      Change the first PGS_LIST_LENGTH layers of internal history table into:
      Prod_gen_status in prod_status.h
   
   Input: 
      from - why this routine is called.

   Output: 

   Returns: 

   Notes:

**************************************************************************/
int PSVPL_output_product_status(int from){

   Prod_gen_status_header *prod_status_array_header;
   Prod_gen_status *prod_status_array;
   Prod_gen_status_next *output_prod_status_list, *tmp;
   Prod_gen_status_pr *tmp_prod;
   int i, j, prod_status_list_len, ret, vdepth, len;

   if (PGS_LIST_LENGTH > MAX_NUM_DEPTH){

      LE_send_msg(GL_ERROR, "PGS_LIST_LENGTH > MAX_PROD_DEPENDENT_PARAMS ? \n");
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* No two or more output within 1 second of each other. */
   if (Last_output_time == 0){

      if (from == PS_DEF_FROM_TIMER)
         return 1;

      Last_output_time = MISC_systime(NULL);
      Need_update = 0;

   }
   else{

      time_t cur_time;

      cur_time = MISC_systime(NULL);
      if (cur_time - Last_output_time < 1){

         if (from == PS_DEF_NEW_PRODUCT)
            Need_update = 1;

         return 1;

      }
      else{

         if (from == PS_DEF_NEW_PRODUCT){

            Last_output_time = cur_time;
            Need_update = 0;

         }
         else if (Need_update == 1){

            Last_output_time = cur_time;
            Need_update = 0;

         }
         else
            return 1;

      }

   }

   /* If called from PS_DEF_FROM_TIMER in main loop(ps_main.c), if
      something changed, output, otherwise return. */
   if (from == PS_DEF_FROM_TIMER){

      if (Psg_output_prod_status_flag == 0)
         return 1;
       
   }

   /* Fill in product status data from Vol_list information. */
   output_prod_status_list = NULL;
   vdepth = 0;
   for (i = PS_DEF_CURRENT_VOLUME; i < PGS_LIST_LENGTH; i++ ){

      /* Ignore when the weather mode is invalid.  Invalid weather
         modes will occur for Vol_list volumes which have not occurred
         as of yet.  Invalid weather mode is the initialized value
         placed in this list. */
      if (Vol_list[i].wx_mode == -1)
         break; 

      vdepth++;
      tmp_prod = Vol_list[i].prod_list;

      /* For each product in the product list .... */
      while (tmp_prod != NULL){

         /* Add product and associated information to output_prod_stat_list. */
         Add_in_output_list(tmp_prod, i, &output_prod_status_list);
         tmp_prod = tmp_prod->next;

      }

   }

   /* Get the length, in number of status entries, of output_prod_status_list */
   prod_status_list_len = Get_prod_status_len(output_prod_status_list);

   if (prod_status_list_len <= 0)
      output_prod_status_list = NULL;

   else{

      /* Allocate space for product status buffer, includes status header. */
      prod_status_array_header = MISC_malloc( ALIGNED_SIZE(sizeof(Prod_gen_status_header)) +
                                              (prod_status_list_len) * sizeof(Prod_gen_status) );

      /* Fill product status header with length and volume depth 
         information of the product status. */
      prod_status_array_header->length = prod_status_list_len;
      prod_status_array_header->vdepth = vdepth;
      prod_status_array_header->list =
                         ALIGNED_SIZE(sizeof(Prod_gen_status_header));

      /* Transfer Vol_list header data to the product status header. */
      for (i = PS_DEF_CURRENT_VOLUME; i < prod_status_array_header->vdepth; i++){

         prod_status_array_header->vtime[i] = Vol_list[i].vol_time;
         prod_status_array_header->vnum[i] = Vol_list[i].vol_num;
         prod_status_array_header->vcpnum[i] = Vol_list[i].vcp_num;
         prod_status_array_header->wx_mode[i] = Vol_list[i].wx_mode;
      }

      prod_status_array = (Prod_gen_status *) ((char *) prod_status_array_header +
                           ALIGNED_SIZE(sizeof(Prod_gen_status_header)));

      /* Transfer the product status data to product status buffer. */
      tmp = output_prod_status_list;
      for (i = 0; i < prod_status_list_len; i++){

         if (tmp == NULL)
            break;

         prod_status_array[i].prod_id = tmp->prod_id;

         for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++)
            prod_status_array[i].params[j] = tmp->params[j];

         prod_status_array[i].elev_index = tmp->elev_index;
          
         prod_status_array[i].gen_pr = tmp->gen_pr;
         prod_status_array[i].schedule = tmp->schedule;

         for (j = 0; j < PGS_LIST_LENGTH; j++)
            prod_status_array[i].msg_ids[j] = tmp->msg_ids[j];

         output_prod_status_list = tmp->next;
         free(tmp);

         tmp = output_prod_status_list;

      }

      output_prod_status_list = NULL;

      /* Write the product status data to the product status LB. */
      len = ORPGDA_write( ORPGDAT_PROD_STATUS, (char *) prod_status_array_header,
                          ALIGNED_SIZE(sizeof(Prod_gen_status_header)) +
                          (prod_status_list_len) * sizeof(Prod_gen_status),
                          PROD_STATUS_MSG );

      if( len < 0 ){

         LE_send_msg( GL_ORPGDA(len),
                      "ORPGDA_write Product Status Failed (%d)\n", len);
         ORPGTASK_exit(GL_EXIT_FAILURE);

      }

      /* Post event indicating the product status has been updated. */
      if( (ret = EN_post( ORPGEVT_PROD_STATUS, NULL, 0, 
                         EN_POST_FLAG_DONT_NTFY_SENDER )) < 0){

         LE_send_msg( GL_EN(ret),
                      "Post ORPGEVT_PROD_STATUS evnt failed (ret = %d)\n", ret);
         ORPGTASK_exit(GL_EXIT_FAILURE);

      }

      /* Free product status buffer. */
      free(prod_status_array_header);
      prod_status_array_header = NULL;


      Psg_output_prod_status_flag = 0;

   }

   /* Do not know what to return, so return PS_DEF_SUCCESS (SDS 8/25/98) */
   return PS_DEF_SUCCESS;

/* END of PSVPL_output_product_status() */
}

/**************************************************************************

   Description: 
      Check every product in the Volume Product List for the
      previous volume (index '1' in Vol_list[]).  If one or more
      products were not generated, publish the reasons in the
      ORPGDAT_PROD_STATUS data file.

   Input: 

   Output: 

   Returns: 

   Notes:

**************************************************************************/
int PSVPL_put_failed_ones(void){

   int flag;      /* if set: one or more products not generated */
   int first_time;
   time_t cur_time;
   Prod_gen_status_pr *tmp_prod;


   /* If it was checked in this vol, just return. */
   if (Psg_check_failed_ones_for_last_vol == 0)
      return 1;
    
   cur_time = MISC_systime(NULL);
   flag = 0;
   first_time = 1;

   /* If previous volume was the initial volume, or weather mode of previous
      volume is undefined, or volume time of previous volume is 0, do not
      time-out product. */
   if( (Vol_list[PS_DEF_PREVIOUS_VOLUME].vol_num != RRS_initial_volume_number())
                  &&
       (Vol_list[PS_DEF_PREVIOUS_VOLUME].wx_mode != -1) 
                  && 
       (Vol_list[PS_DEF_PREVIOUS_VOLUME].local_vol_time != 0)){


      if (Vol_list[PS_DEF_CURRENT_VOLUME].wx_mode == -1)
         return( PS_DEF_FAILED );
       
      if (Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time == 0)
         return( PS_DEF_FAILED );

      if (cur_time >= Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time + TIMEOUT){

         /* Step through product list of preceding volume ... */
         tmp_prod = Vol_list[PS_DEF_PREVIOUS_VOLUME].prod_list;
         while (tmp_prod != NULL){

            if (tmp_prod->gen_status.msg_ids == PGS_SCHEDULED){

               /* Get failed reason why product not generated. */
               tmp_prod->gen_status.msg_ids = Failed_reason(tmp_prod);

               if (Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL){

                  int type = ORPGPAT_get_type( tmp_prod->gen_status.prod_id );

                  if( first_time ){

                     LE_send_msg( GL_INFO,
                        "The Following Products Not Generated For Volume Scan %d\n",
                                  Vol_list[1].vol_num );
                      first_time = 0;

                  }

                  /* Don't report TYPE_RADIAL and TYPE_ON_DEMAND. */
                  if( (type != TYPE_RADIAL) 
                            && 
                      (type != TYPE_ON_DEMAND)
                            &&
                      (type != TYPE_ON_REQUEST) ){

                     /* Log information about product not generated. */
                     PD_write_prod( tmp_prod->gen_status.prod_id, 
                                    tmp_prod->gen_status.params, 
                                    tmp_prod->gen_status.elev_index );

                  }

               }

               flag = 1;   /* product was not generated */

            }

            /* Prepare for next product. */
            tmp_prod = tmp_prod->next;

         /* while products in list of previous volume */
         }  

         Psg_check_failed_ones_for_last_vol = 0;

      }

   }

   if (flag == 1){

      /* One or more products were not generated during the previous volume.
         Output the product generation status. */
      Psg_output_prod_status_flag = 1;

      PSVPL_output_product_status(PS_DEF_NEW_PRODUCT);
      return( PS_DEF_SUCCESS );
   }

   return( PS_DEF_SUCCESS );

/* END of PSVPL_put_failed_ones() */
}

/**************************************************************************
   Description:

      1. Shift Vol_list, i.e., Vol_list[n] <- Vol_list[n-1].
      2. Initialize product generation status for current volume
         scan, i.e., Vol_list[0].

   Input: 

   Output: 

   Returns:
      Returns PS_DEF_SUCCESS if Vol_list[0] formed successfully.  
      Otherwise, PS_DEF_FAILED is returned.

   Notes:

**************************************************************************/
int PSVPL_update_vol_list(void){

   /* Shift Vol_list, i.e., Vol_list[n] <- Vol_list[n-1]. */
   Shift_one_upper_vol_prod_list();

   /* Initialize vol_list[PS_DEF_CURRENT_VOLUME]. */
   if( PSVPL_init_vol_list_for_cur_vol() != PS_DEF_SUCCESS){

      LE_send_msg( GL_ERROR, "Can Not Form Product List For Current Volume.\n");
      PSVPL_free_prod_list_according_to_ind( PS_DEF_CURRENT_VOLUME );

      return( PS_DEF_FAILED );

   }

   return( PS_DEF_SUCCESS ) ;

/* END of PSPE_update_vol_list() */
}

/***** Private Functions Start Below ... *****/
/**************************************************************************
   Description:
      Moves data in vol_list up one index.  Values at table index which 
      exceeds maximum size of table are "aged out".  In essense, this
      module performs the following algorithm:
      
  
        Do For (i = PS_DEF_CURRENT_VOLUME to MAX_NUM_DEPTH - 2) { 
           vol_list[i+1]=Vol_list[i]; 
        }

        free(Vol_list[MAX_NUM_DEPTH]);

       At end of module, this entry contains initialize values.

   Input: 

   Output:

   Returns: 
      Returns PS_DEF_FAILED if the product list before shifting for index
      PS_DEF_CURRENT_VOLUME is empty, otherwise PS_DEF_SUCCESS.

   Notes:

**************************************************************************/
static int Shift_one_upper_vol_prod_list(void){

   int i;
   Prod_gen_status_pr *tmp_list;

   if (Vol_list[PS_DEF_CURRENT_VOLUME].prod_list == NULL)
      return PS_DEF_FAILED;

   if( (RRS_initial_volume())
              && 
       (Vol_list_initialized == VOL_LIST_INIT_PROD_STATUS) ){

      if( Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL )
         LE_send_msg( GL_INFO, "Bypass Volume List Shift\n" );

      return PS_DEF_SUCCESS;

   }

   /* Temporarily hold product list for last volume in list. */
   tmp_list = Vol_list[ MAX_NUM_DEPTH-1 ].prod_list;

   /* Move data up the Vol_list.  In essense, what was in Vol_list[0] is
      transferred into Vol_list[1], Vol_list[1] becomes Vol_list[2], etc. */
   for (i = MAX_NUM_DEPTH-1; i >= 1; i--){

      Vol_list[i].prod_list = Vol_list[i-1].prod_list;
      Vol_list[i].wx_mode = Vol_list[i-1].wx_mode;
      Vol_list[i].vol_time = Vol_list[i-1].vol_time;
      Vol_list[i].vol_num = Vol_list[i-1].vol_num;
      Vol_list[i].vcp_num = Vol_list[i-1].vcp_num;
      Vol_list[i].local_vol_time = Vol_list[i-1].local_vol_time;

   }
    
   /* Initialize Vol_list data for this new volume scan.  The newest 
      volume scans data always occupies Vol_list[PS_DEF_CURRENT_VOLUME]. */
   Vol_list[PS_DEF_CURRENT_VOLUME].wx_mode = -1;
   Vol_list[PS_DEF_CURRENT_VOLUME].vol_time = 0;
   Vol_list[PS_DEF_CURRENT_VOLUME].vol_num = 0;
   Vol_list[PS_DEF_CURRENT_VOLUME].vcp_num = -1;
   Vol_list[PS_DEF_CURRENT_VOLUME].local_vol_time = 0;
   Vol_list[PS_DEF_CURRENT_VOLUME].prod_list = NULL;

   /* The data which previously was at Vol_list[MAX_NUM_DEPTH-1] is
      "aged out".  Any storage allocated for that element is freed with the 
      next call. */
   if( tmp_list != NULL )
      Free_prod_list(&tmp_list);

   return PS_DEF_SUCCESS;

/* END of Shift_one_upper_vol_prod_list() */
}

/**************************************************************************
   Description: 
      This module takes the product status pointed to by prod and inserts
      this information in the product status list.  
      
   Input: 
      prod - pointer to product status data for a product.
      vol_num - the volume scan index corresponding to Vol_list.  The 
                product status pointed to by prod is the product status
                for this product for volume scan index vol_num.
      list - pointer to pointer to the product status list. 

   Output: 

   Returns: 
      If product status pointed to by prod is successfully inserted into the
      product status list, PS_DEF_SUCCESS is return.  In all other case,
      this process exits.

   Notes:

**************************************************************************/
static int Add_in_output_list( Prod_gen_status_pr *prod,
                               int vol_num, Prod_gen_status_next **list){

   Prod_gen_status_next *tmp, *tmp_spt, *pre_tmp;
   int j, i, find, equal;


   /* Allocate space for product status entry. Exit on Failure. */
   tmp = MISC_malloc( sizeof(Prod_gen_status_next) );

   /* Transfer product id, generation period, schedule info, and message ids
      to product status entry. */
   tmp->prod_id = prod->gen_status.prod_id;
   for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++)
         tmp->params[j] = prod->gen_status.params[j];

   tmp->elev_index = prod->gen_status.elev_index;
   tmp->gen_pr = prod->gen_status.gen_pr;

   /* If the current volume, transfer the scheduling information.  
      Otherwise, set scheduling information to PGS_NOT_SCHEDULED. */
   if( vol_num != PS_DEF_CURRENT_VOLUME )
      tmp->schedule = PGS_SCH_NOT_SCHEDULED;
   
   else
      tmp->schedule = prod->gen_status.schedule;

   /* If this product is added to product status after the current volume (i.e.,
      vol_num != PS_DEF_CURRENT_VOLUME), this implies that the product was not 
      scheduled for the current volume and all subsequent volumes up to vol_num.
      Therefore, set all the msg_ids to PGS_NOT_SCHEDULED.   If in a previous
      volume the product was generated, then the msg_ids will be overwritten. */
   for (i = PS_DEF_CURRENT_VOLUME; i < PGS_LIST_LENGTH; i++)
      tmp->msg_ids[i] = PGS_NOT_SCHEDULED;
    
   /* Set the message id for this volume. */
   tmp->msg_ids[vol_num] = prod->gen_status.msg_ids;
   tmp->next = NULL;

   /* If list has not been initialized yet, initialize it and return. */
   if (*list == NULL){

      *list = tmp;
      return PS_DEF_SUCCESS;

   }
   else{

      pre_tmp = tmp_spt = *list;
      find = PS_DEF_NOT_FOUND;

      /* For each product in list, do ... */
      while (tmp_spt != NULL){

         /* If match on product id, then ... */
         if (tmp_spt->prod_id == tmp->prod_id){

            equal = 1;

            /* Check on match of product dependent parameters. */
            for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++){

               if (tmp_spt->params[i] != tmp->params[i]){

                  /* No match */
                  equal = -1;
                  break;

               }
            }

            /* Check on match of elevation index. */
            if( tmp_spt->elev_index != tmp->elev_index ){

               /* No match */
               equal = -1;

            }

            /* If match, then */
            if (equal == 1){

               /* Set the msg id for this volume scan. */
               tmp_spt->msg_ids[vol_num] = tmp->msg_ids[vol_num];

               /* Free temporary buffer and return. */ 
               free(tmp);
               tmp = NULL;
               return PS_DEF_SUCCESS;

            }
            else{

               /* This is the no match on product parameters case. Go to 
                  next product in list. */
               pre_tmp = tmp_spt;
               tmp_spt = tmp_spt->next;

            }

         }
         else if (tmp->prod_id < tmp_spt->prod_id){

            /* This is the no match on product ID case.  If product ID of current
               product less than any other product currently in the product status
               list, insert this product at the beginning of the list. */
            if (pre_tmp == *list && tmp_spt == *list){

               tmp->next = tmp_spt;
               *list = tmp;

            }
            else{

               /* Insert the current product before the product pointed to in 
                  the product status list . */
               tmp->next = pre_tmp->next;
               pre_tmp->next = tmp;

            }

            return PS_DEF_SUCCESS;

         }
         else{

            /* This is the case where there is no match on product ID case but 
               the current product product ID is greater than the product ID of the
               product pointed to in the product status list.  Move to the next
               product in the product status list. */
            pre_tmp = tmp_spt;
            tmp_spt = tmp_spt->next;

         }

      }

      /* If falls through to here, the current product was not found in the 
         product status list and its product ID is greater than any product in
         the product status list. */
      if (find == PS_DEF_NOT_FOUND){

         if (pre_tmp == NULL){

            LE_send_msg( GL_MEMORY, "Memory Error\n");
            ORPGTASK_exit(GL_MEMORY);

         }

         /* Insert the current product at the end of the product status list. */
         pre_tmp->next = tmp;
         return PS_DEF_SUCCESS;

      }

   }

   /* If it falls through to here, something is very wrong so just exit.  This
      is a case that should never happen. */
   LE_send_msg( GL_ERROR, "Add_in_output_list failed \n");
   ORPGTASK_exit(GL_EXIT_FAILURE);

   /* Unnecessary return ... just here to prevent compiler warning. */
   return( PS_DEF_FAILED );

/* END of Add_in_output_list() */
}

/**************************************************************************
   Description: 
      Adds product generation status to the volume list.

   Input: 
      prod - pointer to product generation status structure.
      vol_num_ind - volume list index where product will be added.

   Output: 
      Returns PS_DEF_FAILED on failure to add, or PS_DEF_SUCCESS.

   Returns: 

   Notes:

**************************************************************************/
static int Add_to_volume_list(Prod_gen_status_pr *prod, int vol_num_ind){

   int flag;
   Prod_gen_status_pr *prev_prod;
   Prod_gen_status_pr *tmp_prod;
   Prod_gen_status_pr *new_entry;

   /* Validate the product ID. */
   if( ORPGPAT_prod_in_tbl( (int) prod->gen_status.prod_id ) < 0)
      return PS_DEF_FAILED;
    
   /* Allocate memory for this (potential) new entry in the Volume
      Product List ... Then initialize the new entry ...  */
   new_entry = MISC_malloc(sizeof(Prod_gen_status_pr));

   (void) memcpy(new_entry, prod, sizeof(Prod_gen_status_pr));

   tmp_prod = prev_prod =
      (Prod_gen_status_pr *) Vol_list[vol_num_ind].prod_list;

   /* If there are not products in product list, add to front of list. */
   if (tmp_prod == NULL){

      Vol_list[vol_num_ind].prod_list = new_entry;
      Vol_list[vol_num_ind].prod_list->next = NULL;

   }

   /* Else if this product ID is less than first product in list, add product
      to front of list. */
   else if (prod->gen_status.prod_id < tmp_prod->gen_status.prod_id){

      new_entry->next = Vol_list[vol_num_ind].prod_list;
      Vol_list[vol_num_ind].prod_list = new_entry;

   }
   else{

      /* Insert this product into the middle of the list. */
      flag = PS_DEF_NOT_FOUND;

      while (tmp_prod != NULL){

         if (new_entry->gen_status.prod_id == tmp_prod->gen_status.prod_id){

            new_entry->next = tmp_prod->next;
            tmp_prod->next = new_entry;
            flag = PS_DEF_FOUND_IT;
            break;

         }
         else if (new_entry->gen_status.prod_id < tmp_prod->gen_status.prod_id){

            new_entry->next = prev_prod->next;
            prev_prod->next = new_entry;
            flag = PS_DEF_FOUND_IT;
            break;

         }
         else if (new_entry->gen_status.prod_id > tmp_prod->gen_status.prod_id){

            prev_prod = tmp_prod;
            tmp_prod = tmp_prod->next;

         }

      }

      if (flag == PS_DEF_NOT_FOUND){

         new_entry->next = prev_prod->next;
         prev_prod->next = new_entry;
         flag = PS_DEF_FOUND_IT;

      }

   }

   return PS_DEF_SUCCESS;

/* END of Add_to_volume_list() */
}

/**************************************************************************
   Description:
      This product was not generated in the previous volume scan.
      Return the reason.

   Input:
      prod_status - product status entry. 

   Output: 

   Returns: 

   Notes:
**************************************************************************/
static unsigned int Failed_reason(Prod_gen_status_pr *prod_status){

   int reason;

   /* If product type is of type TYPE_RADIAL, return PGS_UNKNOWN.
      Radial product status is not currently tracked. */
   if( ORPGPAT_get_type( (int) prod_status->gen_status.prod_id ) == TYPE_RADIAL )
      return( PGS_UNKNOWN );

   /* Check the concerning tasks. */
   PSTS_is_gen_task_running(prod_status->gen_status.prod_id, &reason );

   return( reason );

/* END of Failed_reason() */
}

/**************************************************************************
   Description:
      Free prod_list pointed by the input parameter pointer.

   Input: 
      prod_list - pointer to product list.

   Output: 

   Returns: 

   Notes:
**************************************************************************/
static int Free_prod_list(Prod_gen_status_pr **prod_list){

   Prod_gen_status_pr *tmp_prod;

   if (*prod_list == NULL)
      return PS_DEF_SUCCESS;
    
   tmp_prod = *prod_list;
   while (tmp_prod != NULL){

      *prod_list = tmp_prod->next;
      free(tmp_prod);
      tmp_prod = *prod_list;

   }

   if (*prod_list != NULL){

      free(*prod_list);
      *prod_list = NULL;

   }

   /*
     Return value is currently undefined or unused.  (SDS 8/26/98)
   */
   return( PS_DEF_SUCCESS );

/* END of Free_prod_list() */
}

/**************************************************************************
   Description: 
      Returns the length, in number of entries, of the product generation
      status.
 
   Input: 
      list - pointer to product generation status

   Output: 

   Returns: 
      Returns the length.

   Notes:

**************************************************************************/
static int Get_prod_status_len(Prod_gen_status_next *list){

   int cnt;
   Prod_gen_status_next *tmp;

   if (list == NULL)
      return 0;
    
   tmp = (Prod_gen_status_next *) list;
   cnt = 0;
   while (tmp != NULL){

      cnt++;
      tmp = tmp->next;

   }

   return cnt;

/* END of Get_prod_status_len() */
}

/**************************************************************************
   Description: 
      Transfers msg_id, vol_time, and vol_num from one product generation 
      status to another.  

   Input: 
      prod - pointer to output product generation status
      prod_new - pointer to input product generation status
      frm - where the product generation status came from  

   Output: 

   Returns: 

   Notes:

**************************************************************************/
static void Put_msg_id_pd_gen_status( Prod_gen_status_pr *prod, 
                                      Prod_gen_status_pr *prod_new, int frm){

   if (frm == PS_DEF_FROM_GEN_PROD){

      prod->gen_status.msg_ids = prod_new->gen_status.msg_ids;
      prod->gen_status.vol_time = prod_new->gen_status.vol_time;
      prod->gen_status.vol_num = prod_new->gen_status.vol_num;

   }
   else
      prod->gen_status.msg_ids = PGS_UNKNOWN;

/* END of Put_msg_id_pd_gen_status() */
}


/**************************************************************************
   Description: 
      Checks the product generation status from all previous volume scans.
      If the msg_id field "still" indicates that the product is scheduled,
      this means we have not received a response for this request yet so
      we will mark the product as "timed-out".

   Input: 
      prod - product generation status

   Output: 

   Returns: 

   Notes:

**************************************************************************/
static void Put_timeout_prev_vol_list(Prod_gen_status_pr *prod){

   int i;
   Prod_gen_status_pr *tmp_prod;

   /* Do For All Volumes in Vol_list */
   for (i = PS_DEF_PREVIOUS_VOLUME; i < MAX_NUM_DEPTH; i++){

      if (Vol_list[i].prod_list == NULL)
         return;
       
      tmp_prod = (Prod_gen_status_pr *) Vol_list[i].prod_list;
      while (tmp_prod != NULL){

         if (PD_tell_same_prod(&tmp_prod->gen_status, 
                               &prod->gen_status) == PS_DEF_SUCCESS){

            if (tmp_prod->gen_status.msg_ids == PGS_SCHEDULED)
               tmp_prod->gen_status.msg_ids = PGS_TIMED_OUT;
             
            break;

         }

         /* Prepare for next product. */
         else
            tmp_prod = tmp_prod->next;
          
      }

   }

/* END of Put_timeout_prev_vol_list() */
}

