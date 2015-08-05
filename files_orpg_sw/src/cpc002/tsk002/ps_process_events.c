/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2014/12/09 22:34:38 $ $Id:
 * ps_process_events.c,v 1.47 1997/12/19 14:21:24 dodson Exp $ $Revision:
 * 1.49 $ $State: Exp $
 */


/* System Include Files/Local Include Files */
#include <stdlib.h>
#include <string.h>

#include <gen_stat_msg.h>
#include <prod_request.h>

#include <otr_main.h>
#include <rpg_vcp.h>

#define PS_PROCESS_EVENTS
#include <ps_globals.h>
#undef PS_PROCESS_EVENTS

/* Constant Definitions/Macro Definitions/Type Definitions */

/* Static Globals */

/* Current volume volume scan number. */
static unsigned int Cur_vol_num = 0;
static unsigned int Last_start_of_volume_processed = 0xffffffff;

/* Current and previous volume VCP number. */
static int Cur_vcp_num = 0;
static int Prev_vcp_num = 0;

/***** Public Functions Start Below ... *****/
/**************************************************************************
   Description: 
      One time requests are received which must be scheduled on real-time
      data stream.

   Input: 
      buf - buffer containing one-time requests.
      num_reqs - number of requests in one-time request list.

   Output: 
  
   Returns: 
      Terminates if MISC_malloc fails, otherwise returns PS_DEF_SUCCESS.

   Notes:

 **************************************************************************/
int PSPE_handle_one_time_req_list(char *buf, int len ){

   Prod_gen_status_pr_req *one_time_list = NULL;
   int num_reqs, i;

   /* Inform operator that we are processing One_time Request list. */
   LE_send_msg( GL_INFO, "Processing One-Time Request List\n" );

   /* Build the Prod_gen_status_pr_req list from one-time request list. */
   num_reqs = PD_build_request_list( len, buf, PS_DEF_FROM_ONE_TIME,
                                     -1, &one_time_list );

   /* Inform the operator if there are no products in request. */
   if( num_reqs == 0 )
      LE_send_msg( GL_INFO, "There Are No Products In Request\n" );

   /* Backup output generation control list (master generation control list). */
   PD_backup_output_gen_control_list();

   /* Free the previous One-time generation control list, then turn the "one_time_list" 
      into the One-time generation control list. */
   PD_free_gen_control_list( ONETIME_GEN_CONTROL );
   PD_backup_one_time_list(one_time_list, num_reqs);

   /* For each one-time requested product to be generated on the real-time data stream,
      determine if there are any dependent products which need to be scheduled. */
   for (i = 0; i < num_reqs; i++)
      PSPTT_through_dep_list_of_this_prod( one_time_list[i].gen_status.prod_id,
                                           PS_DEF_FROM_ONE_TIME, 0,
                                           (void *) &one_time_list[i] );

   /* Free the one time list. */
   if (one_time_list != NULL){

      free(one_time_list);
      one_time_list = NULL;

   }

   /* Add into vol_list[0] */
   PD_add_back_one_time_list_to_cur_vol();

   /* Mark all last one time requests which were not scheduled as simply 
      not scheduled. */
   PSVPL_mark_one_time_req_last_vol_not_used_cur_vol();

   /* Generate current volume's output generation control list. */
   PD_gen_output_gen_control_list(7);

   /* Merge current volumes and backup output generation control list. */
   PD_merge_cur_backup_output_gen_control_list();

   /* Update the product requests for products which are scheduled. */
   PSPE_prod_list_to_prod_request( PD_get_output_gen_control_list(),
                                   PD_get_output_gen_control_list_len());
   PD_free_gen_control_list( CUR_GEN_CONTROL );

   return PS_DEF_SUCCESS;

/* END of PSPE_handle_one_time_req_list() */
}

/**************************************************************************
   Description: 
      Driver module for initialization of the Product Request LB.  For 
      each product defined in the product attributes table, initialize
      a product request.  The actual initialization is performed within module 
      PD_init_prod_list_to_gen_control.

   Input:

   Output:

   Returns: 
      Always returns PS_DEF_SUCCESS.
 
   Notes:
      Process termination if ORPGPAT_num_tbl_items call fails.

 **************************************************************************/
int PSPE_init_gen_control_lb(void){

   int num_prods, prod_id, index;

   /* Check to see if the Product Attribute List is empty.  If so,
      this is a fatal error so exit. */
   if( (num_prods = ORPGPAT_num_tbl_items()) <= 0 ){

      LE_send_msg(GL_ERROR, "Product Attribute Table Empty!\n");
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Do For Each product in the Product Attributes List. */
   index = 0;
   while( index < num_prods ){

      if( (prod_id = ORPGPAT_get_prod_id( index )) == ORPGPAT_ERROR ){

         LE_send_msg( GL_ERROR, "ORPGPAT_get_prod_id Failed For Index %d\n", index );
         ORPGTASK_exit( GL_ERROR );

      }

      /* Initial product request data for this product. */
      PD_init_prod_list_to_gen_control( prod_id );

      /* Prepare for next product in Product Attributes List. */
      index++;

   /* End of "while" loop */
   }

   return (PS_DEF_SUCCESS);

/* END of PSPE_init_gen_control_lb() */
}

/**************************************************************************
   Description: 
      Initialize all global static variables for this module.

   Input:

   Output: 

   Returns: 
      There is no return value define for this module.

   Notes:

**************************************************************************/
void PSPE_initialize(){

   /* Set the current weather mode, current vcp and previous vcp to 
      whatever returned in volume status.  Initialize the current volume 
      number to invalid value. */
   Psg_cur_wx_mode = RRS_get_current_weather_mode();
   Cur_vol_num = 0;
   Cur_vcp_num = RRS_get_current_vcp_num();
   Prev_vcp_num = RRS_get_previous_vcp_num();

   /* Open LB and seek to first message. */
   ORPGDA_read(ORPGDAT_RT_REQUEST, NULL, 0, LB_NEXT);
   ORPGDA_seek(ORPGDAT_RT_REQUEST, 0, LB_FIRST, NULL);

/* END of PSPE_initialize() */
}

/**************************************************************************
   Description:
      Actived by request list sent by One-Time Scheduler process.

   Input:

   Output:

   Returns:

   Notes:

**************************************************************************/
void PSPE_proc_ot_schedule_list_event(void){

   /* Both events are generated by a write to the ORPGDAT_RT_REQUEST lb,
      so reading the LB can be handled in the same place. */
   PSPE_proc_rt_request_event();

/* END of PSPE_proc_ot_schedule_list_event() */
}

/**************************************************************************
   Description:
       Default (Current) Product Generation Table (PGT) has changed.  
       Rebuild the Default Product Generation List.

   Input:

   Output:

   Returns:

   Notes:

**************************************************************************/
void PSPE_proc_prod_list_event(void){

   int default_pgt_num_prods;

   LE_send_msg( GL_INFO, "Processing PROD LIST EVENT\n" );

   /* Make a copy of the previous volumes output generation control list. */
   PD_backup_output_gen_control_list(); 

   /* Re-build the default product generation list from the updated 
      current product generation table. */
   PD_form_default_prod_gen_list( &default_pgt_num_prods );

   /* Initialize vol_list[PS_DEF_CURRENT_VOLUME]. */
   if( PSVPL_init_vol_list_for_cur_vol() != PS_DEF_SUCCESS){
  
      LE_send_msg( GL_ERROR, "Can Not Form Product List For Current Volume.\n");
      PSVPL_free_prod_list_according_to_ind( PS_DEF_CURRENT_VOLUME );
  
      return;

   }                                 

   /* Get updated task status to determine which tasks are running/not
      running. */
   PSTS_update_task_status(); 

   /* For each RPS list, mark products as scheduled/not scheduled depending
      on task status of generating task. */
   PD_keep_routine_req_vol_list();
  
   /* For one-time requests generated on the real-time data stream, mark
      as scheduled/not scheduled depending on task status of generating task. */
   PD_keep_one_time_req_vol_list();
   
   /* Flag, if set, indicates ORPGDAT_PROD_STATUS data is to be output. */
   Psg_output_prod_status_flag = 1;
  
   /* Generate the output generation control list for this volume scan. */
   PD_gen_output_gen_control_list(7);
   
   /* Merge the backup output generation control list with the current volumes
      output generation control list. */
   PD_merge_cur_backup_output_gen_control_list();
  
   /* This call actually schedules the products by updating the product
      request LB.  The Output Generation Control List is used for product
      scheduling. */
   PSPE_prod_list_to_prod_request( PD_get_output_gen_control_list(),
                                   PD_get_output_gen_control_list_len());
  
   /* Done this the Output generation Control List. */
   PD_free_gen_control_list( CUR_GEN_CONTROL );                                       
  
   /* Output the product generation status. */
   PSVPL_output_product_status(PS_DEF_NEW_PRODUCT);

/* END of PSPE_proc_prod_list_event() */
}

/**************************************************************************
 Description:

   Activated by the request for routine product event.  Reads 
   ORPGDAT_RT_REQUEST LB and processes requests differently according to 
   whether the request came from ps_onetime or from a narrowband user.

 Input: 

 Output: 

 Returns: 

 Notes:

   If the return value from ORPGDA_read of ORPGDAT_RT_REQUEST is 
   negative (and not equal to LB_TO_COME or LB_EXPIRED) this is a fatal error 
   condition.
    
**************************************************************************/
void PSPE_proc_rt_request_event(void){

   LB_info info;
   int msgs_expired, len;
   char *rps_list = NULL;


   /* Do Until all messages exhausted. */
   msgs_expired = 0;
   while(1){

      /* Read Routine Requests. */
      len = ORPGDA_read( ORPGDAT_RT_REQUEST, &rps_list, LB_ALLOC_BUF,
                         LB_NEXT );

      if (len > 0){

         info.id = LB_previous_msgid( ORPGDA_lbfd( ORPGDAT_RT_REQUEST )); 

         if (len < ALIGNED_SIZE(sizeof(Pd_msg_header))){

            LE_send_msg( GL_ERROR,
                         "ORPGDA_read ORPGDAT_RT_REQUEST info.id %d: %d\n",
                         info.id, len );

            free( rps_list );
            continue;

         }

         /* Is the request coming via one-time or is it coming directly from 
            a user? */
         if (info.id == OTR_PSERVER_NUMBER){

            /* Came via ps_onetime */
            PD_process_request(len, rps_list, PS_DEF_FROM_ONE_TIME);

         }
	 else{

            /* Came via a narrowband user. */
            PD_process_request(len, rps_list, PS_DEF_FROM_ONE_TIME + 100);

         }
 
         /* Free read buffer. */
         if( rps_list != NULL ){

            free( rps_list );
            rps_list = NULL;

         }

      }
      else if(len == LB_TO_COME)
         break;

      else if (len == LB_EXPIRED){

         /* Increment the number of expired messages. */
         msgs_expired++;
         continue;

      }
      else{

         /* Anything negative returned from ORPGDA_read not equal to LB_TO_COME
            or LB_EXPIRED is considered fatal error! */
         LE_send_msg( GL_ORPGDA(len), 
                      "ORPGDA Read of ORPGDA_RT_REQUEST Failed (%d)\n", len );
	 ORPGTASK_exit( GL_EXIT_FAILURE );

      }

   /* End of "while" loop. */
   }

   /* If there were expired messages, then we may have potentially lost
      routine requests. */
   if( msgs_expired > 0 )
      LE_send_msg( GL_ERROR, "Possible Lost Routine Requests\n" );

   return;

/* END of PSPE_proc_rt_request_event() */
}

/**************************************************************************
   Description:
      Generates the master product generation control list for the current
      volume scan.

   Input:
      default_pgt_updated - flag, if set, indicates default product
                            generation list updated.
      force_update_all - forces update of all requests.

   Output:

   Returns:

   Notes:

      RRS_update_vol_status is called upon start of volume.  It is assumed
      that ps_routine does not have knowledge of a weather mode change until
      this function is called.  Therefore, the default product generation
      list, if defined, should be defined for the weather mode in effect 
      during the previous volume scan.

**************************************************************************/
void PSPE_proc_start_of_volume_event( int default_pgt_updated, 
                                      int force_update_all ){

   int ret, wx_mode_status = PS_DEF_WXMODE_UNCHANGED;
   int default_pgt_num_prods;
   int vcps_differ = RRS_NO_DIFFERENCES;

   /* Update local copy of volume status data.  */
   RRS_update_vol_status();

   /* Check if the angles are different between the current and previous 
      VCPs.  Currently only care about differences for the same VCP
      number. */
   vcps_differ = RRS_current_previous_vcps_differ();
   if( vcps_differ == RRS_DIFFERENT_ANGLES ){

      LE_send_msg( GL_INFO,
                   "Current/Previous VCPs Have Different Angles\n" );
      force_update_all = 1;

   }

   /* If the master generation list has already been built for this 
      volume scan, just return. */
   if( (Cur_vol_num = RRS_get_volume_num(NULL)) == Last_start_of_volume_processed )
      return;

   /* Set the Last_start_of_volume_processed for next pass. */
   Last_start_of_volume_processed = Cur_vol_num;

   /* Inform user of new volume scan */
   if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
      LE_send_msg( GL_INFO, "Processing Start Of Volume Event For Volume %ld\n",
                   Last_start_of_volume_processed );

   /* Check if the weather mode has changed.  wx_mode_status holds the
      status of the change (i.e., changed from default, changed from last
      weather mode..) */
   if( Psg_cur_wx_mode != RRS_get_current_weather_mode() ){

      /* Weather mode has changed. */
      if (Psg_cur_wx_mode == PS_DEF_WXMODE_UNKNOWN){

         wx_mode_status = PS_DEF_WXMODE_UNKNOWN;
         if (RRS_get_current_weather_mode() != Psg_wx_mode_beginning)
            wx_mode_status = PS_DEF_WXMODE_NOT_DEFAULT;
          
      }
      else
         wx_mode_status = PS_DEF_WXMODE_CHANGED;

   }

   /* Set current weather mode and volume coverage pattern number for 
      current and previous volume scans. */
   Psg_cur_wx_mode = RRS_get_current_weather_mode();
   Cur_vcp_num = RRS_get_current_vcp_num();
   Prev_vcp_num = RRS_get_previous_vcp_num();

   /* Get updated task status to determine which tasks are running/not 
      running.  This affects scheduling in that products generated by
      failed tasks will not be scheduled. */
   PSTS_update_task_status();

   /* If the VCP has changed, the RPS list of each Line user needs to be 
      reprocessed since the elevation indices corresponding to the requested 
      elevation angles may be different for the current VCP.  If the task
      status has changed, some products generated on behalf of the failed
      product generator may need to be unscheduled. */  
   if( (Cur_vcp_num != Prev_vcp_num)
                    ||
       (Psg_task_status_changed)
                    ||
       (force_update_all) ){

      if( Psg_task_status_changed )
         LE_send_msg( GL_INFO, "Updating RPS on Task Status Change.\n" );

      else if( force_update_all )
         LE_send_msg( GL_INFO, "Forcing Update of RPS\n" );

      else
         LE_send_msg( GL_INFO, "Updating RPS on VCP Change.\n",
                      Cur_vcp_num, Prev_vcp_num );

      PD_rps_update();

   }

   /* Make a copy of the previous volumes output generation control list.

      Note:  This must be done after any RPS list updates because of VCP
             change.  This is because in the RPS list processor, the output
             generation and control list is backed up, then ultimately
             deleted after an RPS list has been processed.  Consequently, for 
             the last RPS list processed, the backup output generation control
             list is deleted.  We need to back it up again. 
   */
   PD_backup_output_gen_control_list();

   /* If the weather mode has changed, re-read the default product generation
      list for the new weather mode.  Make this the current product generation
      list.  If the VCP has changed from the previous volume scan then we need 
      to reprocess the current product generation list since the angles associated
      with the elevation indices may have changed.  If the default product 
      generation table has been updated previous volume, update it now. 
      If the task status has changed (either a task failed or has been restarted),
      then rebuild the default product generation table. */
   if( (wx_mode_status != PS_DEF_WXMODE_UNCHANGED)
                       ||
       (Cur_vcp_num != Prev_vcp_num)
                       ||
              (default_pgt_updated)
                       ||
              (Psg_task_status_changed)
                       ||
              (force_update_all) ){

      if( wx_mode_status != PS_DEF_WXMODE_UNCHANGED ){

         /* Copy the appropriate default product generation table to the 
            current product generation table. */
         switch( Psg_cur_wx_mode ){

            case ORPGPGT_DEFAULT_A_TABLE:
            {
               ORPGPGT_replace_tbl( ORPGPGT_DEFAULT_A_TABLE, ORPGPGT_CURRENT_TABLE );
               LE_send_msg( GL_INFO, "Default A PGT Becomes Current PGT\n" );
               break;
            }
            case ORPGPGT_DEFAULT_B_TABLE:
            {
               ORPGPGT_replace_tbl( ORPGPGT_DEFAULT_B_TABLE, ORPGPGT_CURRENT_TABLE );
               LE_send_msg( GL_INFO, "Default B PGT Becomes Current PGT\n" );
               break;
            }

         /* End of "switch" */
         }

      }

      /* Tell user if default product generation table has been updated. */
      if( default_pgt_updated )
         LE_send_msg( GL_INFO, "Default PGT Updated Previous Volume Scan\n" );

      else if( Psg_task_status_changed )
         LE_send_msg( GL_INFO, "Task Status Has Changed This Volume Scan\n" );

      else if( force_update_all )
         LE_send_msg( GL_INFO, "Forcing update of Default PGT For This Volume Scan\n" );

      /* Either the weather mode has changed or the VCP number is different from last
         volume scan, or some task status has changed.  Need to process the (new) 
         current product generation table for the former 2.  For the latter, products
         which may have failed will no longer be scheduled.  This includes any dependent
         products. 

         Note:  This needs to be done after the RPS lists for each user are 
                re-processed owing to a VCP change.  This also needs to be
                performed after the output generation control list has been
                backed up.  This is necessary for products which are not longer
                specified for default generation are unscheduled. 
      */
      PD_form_default_prod_gen_list( &default_pgt_num_prods );
      if( default_pgt_num_prods == 0 )
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Default PGT Is Empty\n" );          

   }

   /* Initialize the product generation status for current volume scan,
      i.e., Vol_list[0].  Vol_list[0] always refers to the current 
      volume scan.  

      Note:  This needs to be done after the default product generation 
             list has been updated.  This is necessary so Vol_list[0] is 
             initialized with the "new" current default product generation
             list.
   */
   PSVPL_update_vol_list();

   /* Discard the original users requests if products are not required
      to be generated this volume scan. */
   PD_change_act_each_line();
   PD_keep_routine_req_vol_list();

   /* Keep the original one time requests as reqed not scheduled. */
   PD_keep_one_time_req_vol_list();

   /* Set flag indicating ORPGDAT_PROD_STATUS data is to be output. */
   Psg_output_prod_status_flag = 1;

   /* Generate the output generation control list for this volume scan. */
   PD_gen_output_gen_control_list(7);

   /* Merge the backup output generation control list with the current volumes
      output generation control list. */
   PD_merge_cur_backup_output_gen_control_list();

   /* This call actually schedules the products by updating the product
      request LB.  The Output Generation Control List is used for product
      scheduling. */
   PSPE_prod_list_to_prod_request( PD_get_output_gen_control_list(),
                                   PD_get_output_gen_control_list_len());

   /* Done with the Output generation Control List. */
   PD_free_gen_control_list( CUR_GEN_CONTROL );

   /* Output the product generation status. */
   PSVPL_output_product_status(PS_DEF_NEW_PRODUCT);

   /* Inform operator and other processes of weather mode change. */
   if( (wx_mode_status == PS_DEF_WXMODE_NOT_DEFAULT)
                          || 
       (wx_mode_status == PS_DEF_WXMODE_CHANGED) ){

      if (Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL){

         if (wx_mode_status == PS_DEF_WXMODE_NOT_DEFAULT)
            LE_send_msg( GL_INFO, "Wx Mode Changed From Default %d to %d",
                         Psg_wx_mode_beginning, 
                         RRS_get_current_weather_mode() );
             
         else if(wx_mode_status == PS_DEF_WXMODE_CHANGED)
            LE_send_msg( GL_INFO, "WX MODE Changed To %d", 
                         RRS_get_current_weather_mode() );
             
      }

      /* Post event about weather mode change. */
      if( (ret = EN_post( ORPGEVT_WX_UPDATED, NULL, 0, 
                          EN_POST_FLAG_DONT_NTFY_SENDER )) < 0){

         LE_send_msg( GL_EN(ret), "ORPGEVT_WX_UPDATED Failed (%d)", ret );
         ORPGTASK_exit(GL_EXIT_FAILURE);

      }

   }

   /* This flag indicates we need to check previous volume scan for products
      unexpectantly not generated. */
   Psg_check_failed_ones_for_last_vol = 1;

   /* Issue ORPGEVT_PROD_GEN_CONTROL event.  This event notifies all interested
      parties that scheduling is complete for the current volume scan. */
   if( (ret = EN_post( ORPGEVT_PROD_GEN_CONTROL, NULL, 0, 
                       EN_POST_FLAG_DONT_NTFY_SENDER ) ) < 0)
      LE_send_msg( GL_EN(ret),
                   "ORPGEVT_PROD_GEN_CONTROL Failed (%d)", ret );
   else 
      LE_send_msg( GL_INFO, "Post ORPGEVT_PROD_GEN_CONTROL OKAY" );

   return;

/* END of PSPE_proc_start_of_volume_event() */
}

/**************************************************************************
   Description:
      Change product generation list into product requests.

   Inputs:
      list -  pointer pointing to the product generation list.
      num_req - number of requests in the product generation list.

   Outputs:

   Returns: 
      Returns PS_DEF_FAILED on failure or PS_DEF_SUCCESS on success.

   Notes:
      There are several task termination conditions in this module.

***************************************************************************/
int PSPE_prod_list_to_prod_request(Prod_gen_status_pr_req *list, int num_req){

   Prod_gen_status_pr_req *tmp_prod;
   Prod_request *prod_req, *output_buf, tmp_req;
   int i, j, ind, len, index, cur_id, cur_num, cur_id_pt;
   int num_defined_params;

   /* If there are not requests, nothing to do. */
   if (num_req <= 0){

      LE_send_msg( GL_INFO, "There are No Products to Schedule\n" );
      return PS_DEF_FAILED;

   }

   /* Allocate memory for all product requests. */
   prod_req = (Prod_request *) MISC_malloc( num_req*sizeof(Prod_request) );

   tmp_prod = list;
   i = 0;
   while (tmp_prod != NULL && i < num_req){

      prod_req[i].pid = tmp_prod->gen_status.prod_id;
      if( prod_req[i].pid < 0 || 
          ORPGPAT_prod_in_tbl( (int) prod_req[i].pid ) == ORPGPAT_ERROR ){

         /* Unknown product id ... report error and skip. */
         LE_send_msg( GL_ERROR, "Unknown Product ID (%d)\n", prod_req[i].pid);
         num_req--;

         /* Prepare for next product. */
         tmp_prod = tmp_prod->next;
         continue;

      }

      /* Initialize all product dependent parameters to 0. */
      prod_req[i].param_1 = PARAM_UNUSED;
      prod_req[i].param_2 = PARAM_UNUSED;
      prod_req[i].param_3 = PARAM_UNUSED;
      prod_req[i].param_4 = PARAM_UNUSED;
      prod_req[i].param_5 = PARAM_UNUSED;
      prod_req[i].param_6 = PARAM_UNUSED;

      /* Put product dependent parameters in product request. */
      num_defined_params = ORPGPAT_get_num_parameters( tmp_prod->gen_status.prod_id );
      for( j = 0; j < num_defined_params; j++ ){

         ind = ORPGPAT_get_parameter_index( tmp_prod->gen_status.prod_id, j );
         if( ind >= 0 ){

            switch( ind ){

               case 0:
               {
                  prod_req[i].param_1 = tmp_prod->gen_status.params[ind];
                  break;
               }

               case 1:
               {
                  prod_req[i].param_2 = tmp_prod->gen_status.params[ind];
                  break;
               }

               case 2:
               {
                  prod_req[i].param_3 = tmp_prod->gen_status.params[ind];
                  break;
               }

               case 3:
               {
                  prod_req[i].param_4 = tmp_prod->gen_status.params[ind];
                  break;
               }

               case 4:
               {
                  prod_req[i].param_5 = tmp_prod->gen_status.params[ind];
                  break;
               }
        
               case 5:
               {
                  prod_req[i].param_6 = tmp_prod->gen_status.params[ind];
                  break;
               }

            /* End of "switch". */
            }

         }

      /* End of "for" loop. */
      }

      /* If product is not scheduled because of task failure or has been disabled, put 
         REQ_NOT_SCHEDLD elev_ind field. */
      if( tmp_prod->gen_status.schedule == PGS_SCH_NOT_SCHEDULED ){

         prod_req[i].elev_ind = REQ_NOT_SCHEDLD;

         if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
            LE_send_msg( GL_INFO, 
               "Product ID %3d Not Scheduled (Schedule == PGS_SCH_NOT_SCHEDULED)\n", 
               tmp_prod->gen_status.prod_id );

      }

      /* If product has been deactivated, put REQ_NOT_SCHEDLD in elev_ind field. */
      else if (tmp_prod->act_this_time == PS_DEF_PROD_DEACTIVATED){

         prod_req[i].elev_ind = REQ_NOT_SCHEDLD;
       
         if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL)
            LE_send_msg( GL_INFO, 
               "Product ID %3d Not Scheduled (Act_this_time == PS_DEF_PROD_DEACTIVATED)\n", 
               tmp_prod->gen_status.prod_id );
      }
      else
         prod_req[i].elev_ind = tmp_prod->gen_status.elev_index;

      /* Transfer the request number and initialize the volume sequence field. */
      prod_req[i].req_num = tmp_prod->gen_status.req_num;
      prod_req[i].vol_seq_num = 0;

      /* Prepare for next product. */
      tmp_prod = tmp_prod->next;
      i++;

   /* End of "while" loop. */
   }

   /* Sort the requests in increasing pid order. */
   cur_id = prod_req[0].pid;
   cur_id_pt = 0;

   /* Do For All requests. */
   while (cur_id_pt < num_req){

      cur_id = prod_req[cur_id_pt].pid;
      index = cur_id_pt;
      while (index < num_req){

         if (prod_req[index].pid < cur_id){

            memcpy(&tmp_req, &(prod_req[cur_id_pt]), sizeof(Prod_request));
            memcpy(&(prod_req[cur_id_pt]), &(prod_req[index]), sizeof(Prod_request));
            memcpy(&(prod_req[index]), &tmp_req, sizeof(Prod_request));

            cur_id = prod_req[cur_id_pt].pid;
            index++;

         }
         else
            index++;
          
      /* End of inner "while" loop. */
      }

      cur_id_pt++;

   /* End of outer "while" loop. */
   }

   /* Each group of requests for a product must be terminated.  The 
      group is terminated by a request with an invalid product id (=-1)  */
   cur_id = prod_req[0].pid;
   cur_num = 0;
   index = 0;

   /* For All Products .... */
   while (index + cur_num < num_req){

      if (cur_id == prod_req[index + cur_num].pid){

         /* If same product as previous, just continue. */
         cur_num++;

      }
      else{

         /* Allocate space for the group of product requests (all with the 
            same product ID. */
         output_buf = (Prod_request *) MISC_malloc((cur_num + 1) * sizeof(Prod_request));

         /* Copy list of products to temporary buffer. */
         memcpy(output_buf,
                (Prod_request *) ((char *) prod_req + index * sizeof(Prod_request)),
                cur_num * sizeof(Prod_request));

         /* Append termination request. */
         output_buf[cur_num].pid = -1;

         /* Write product requests to product request LB. */
         len = ORPGDA_write( ORPGDAT_PROD_REQUESTS,
                             (char *) output_buf,
                             (cur_num + 1) * sizeof(Prod_request),
                             cur_id );

         /* Report any write error. */
         if (len < 0){

            LE_send_msg( GL_ORPGDA(len),
                         "ORPGDA_write ORPGDAT_PROD_REQUESTS (id %d) Failed (%d)",
                         cur_id, len);
            LE_send_msg( GL_INFO, "Product Request Message Length %d\n",
                         (cur_num + 1) * sizeof(Prod_request) );

         }
          
         /* Tell user about product. */
         if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL){

            LE_send_msg( GL_INFO, "Product Generation Control List\n" );

            for (i = 0; i < cur_num + 1; i++){

               if( output_buf[i].pid >= 0 )
                  LE_send_msg( GL_INFO,
                            "-->Prod ID: %d   Params: %d  %d  %d  %d  %d  %d  Elev Ind: %d\n",
                            output_buf[i].pid, output_buf[i].param_1,
                            output_buf[i].param_2, output_buf[i].param_3,
                            output_buf[i].param_4, output_buf[i].param_5,
                            output_buf[i].param_6, output_buf[i].elev_ind );

            }

         }
   
         /* Free temporary storage. */
         free(output_buf);
         output_buf = NULL;

         /* Prepare for next product ID. */
         cur_id = prod_req[index + cur_num].pid;
         index = index + cur_num;
         cur_num = 0;

      }

   /* End of "while" loop */
   }

   /* Do last product in the list. */
   output_buf = MISC_malloc((cur_num + 1) * sizeof(Prod_request));

   /* Copy the product request information into a temporary buffer. */
   memcpy( output_buf,
           (Prod_request *) ((char *) prod_req + index * sizeof(Prod_request)),
           cur_num * sizeof(Prod_request) );

   /* Add request termination. */
   output_buf[cur_num].pid = -1;

   /* Write product request message. */
   len = ORPGDA_write( ORPGDAT_PROD_REQUESTS,
                       (char *) output_buf,
                       (cur_num + 1) * sizeof(Prod_request),
                       cur_id );

   /* Report any write errors. */
   if (len < 0){

      /* Report error trying to write product requests LB. */
      LE_send_msg( GL_ORPGDA(len),
                   "ORPGDA_write ORPGDAT_PROD_REQUESTS (id %d) Failed (%d)",
                   cur_id, len );

   }

   /* Inform users about products. */
   if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL){

      LE_send_msg(GL_INFO, "Product Generation Control List\n");

      for (i = 0; i < cur_num + 1; i++){

         if( output_buf[i].pid >= 0 )
            LE_send_msg( GL_INFO, 
                      "-->Prod ID: %d   Params: %d  %d  %d  %d  %d  %d  Elev Ind: %d\n",
                      output_buf[i].pid, output_buf[i].param_1,
                      output_buf[i].param_2, output_buf[i].param_3,
                      output_buf[i].param_4, output_buf[i].param_5,
                      output_buf[i].param_6, output_buf[i].elev_ind );

      }

   }

   /* Free all temporary storage. */
   free(output_buf);
   output_buf = NULL;

   free(prod_req);
   prod_req = NULL;

   return PS_DEF_SUCCESS;

/* END of PSPE_prod_list_to_prod_request() */
}
