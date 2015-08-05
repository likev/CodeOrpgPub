/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2014/03/12 17:39:17 $ $Id:
 * ps_process_msg.c,v 1.34 1997/10/30 17:20:18 dodson Exp dodson $ $Revision:
 * 1.36 $ $State: Exp $
 */

#include <stdlib.h>
#include <string.h>

#include <orpgda.h>
#include <orpgdat.h>
#include <orpgerr.h>

#include <prod_distri_info.h>
#include <prod_gen_msg.h>
#include <prod_user_msg.h>
#include <prod_status.h>

#define PS_PROCESS_MSG
#include <ps_globals.h>
#undef PS_PROCESS_MSG


/*
 * Static Globals
 */
static int Aligned_Buf1[PS_DEF_BUF_SIZE_WDS];
static char *Buf1 = (char *) Aligned_Buf1;

/*
 * Static Function Prototypes
 */
static int Change_gen_msg_to_pr_format(Prod_gen_msg * gen_msg,
                      Prod_gen_status_pr * prod_status);


/**************************************************************************

  Description: 
      Search Prod_gen_msg LB to see if any product newly generated. If
      yes, newly generated product is added to Vol_list via a call to 
      PSVPL_add_prod_gen_status.  After adding, the product status is 
      written to the product status LB via a call to 
      PSVPL_output_product_status. 

  Inputs:

  Outputs:

  Returns:
     PS_DEF_FAILURE on error, otherwise PS_DEF_SUCCESS.

  Notes:

***************************************************************************/
int PSPM_chk_new_prods(void){

   Prod_gen_msg *pr_gen_msg;
   int len, vol_index, output_this_time;

   /* Flag, when set, indicates the Product Status is to be updated. */
   output_this_time = 0;

   /* Do Forever ..... */
   while (1){

      /* Automatic variables ... */
      static Prod_gen_status_pr prod_status;

      /* Read Prod_Gen_Msg LB to see if there is any product newly generated. */
      len = ORPGDA_read(ORPGDAT_PROD_GEN_MSGS, Buf1, PS_DEF_BUF_SIZE, LB_NEXT);
      
      /* Check for negative length.  All negative values other than LB_TO_COME
         and LB_EXPIRED are errors. */
      if( len < 0 ){

         if (len == LB_TO_COME){

            /* No new product generated. */
            if (output_this_time == 1)
               break;

            return( len );
         }

         /* If the return value is LB_EXPIRED, seek to the first available
         message in hopes we can catch up. */
         else if (len == LB_EXPIRED){

            ORPGDA_seek( ORPGDAT_PROD_GEN_MSGS, 0, LB_FIRST, NULL );
            continue;

         }

         /* Must be an error. */
         else{     

            LE_send_msg(GL_ORPGDA(len), "ORPGDAT_PROD_GEN_MSGS Read Failed (%ld).\n",
                        len);
            return( len );

         }

      }

      /* Length less than size of product generation mesage indicates coding error. */
      if (len < sizeof(Prod_gen_msg)){

         LE_send_msg(GL_ERROR, "Bad Prod Gen Msg (len %d < sizeof(Prod_gen_msg) %d)\n",
                     len, sizeof(Prod_gen_msg));
         return( PS_DEF_FAILED );

      }

      pr_gen_msg = (Prod_gen_msg *) Buf1;

      /* Check if this product generated from real-time stream or replay stream.
         Ignore products generation off the replay stream. */
      if( pr_gen_msg->input_stream == PGM_REPLAY_STREAM )
         continue;
   
      /* There is a new product just generated.  Initialize product status.*/
      (void) memset(&prod_status, 0, sizeof(Prod_gen_status_pr));

      /* Change Buf1 into Prod_gen_msg format.  If fails, return error. */
      if( Change_gen_msg_to_pr_format(pr_gen_msg, &prod_status) < 0){

         LE_send_msg( GL_ERROR, "Product Generation Status To Request Failed\n" );
         return( PS_DEF_FAILED );

      }

      /* Return the index into Vol_list for which this product belongs. */
      if (Psg_cur_wx_mode == PS_DEF_WXMODE_UNKNOWN){ 

         /* Before 1st vol scan starts. */
         vol_index = PS_DEF_CURRENT_VOLUME;

      }
      else
         vol_index = PSVPL_get_vol_list_index( &prod_status );

      if( vol_index >= PS_DEF_CURRENT_VOLUME ){

         /* If verbose mode, tell user about generated product */
         if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL){

            LE_send_msg( GL_INFO,
                         "Product Generated For Volume %d (msg_ids %ld)\n",
                         prod_status.gen_status.vol_num, prod_status.gen_status.msg_ids);
            PD_write_prod( prod_status.gen_status.prod_id, 
                           prod_status.gen_status.params, 
                           prod_status.gen_status.elev_index );

         }

         /* Set the schedule to PGS_SCH_NOT_SCHEDULED.  If this product exists
            in the Vol_list, the correct schedule information will be retained. */
         prod_status.gen_status.schedule = PGS_SCH_NOT_SCHEDULED;

         /* Add this product to the product status. */
         PSVPL_add_prod_gen_status(&prod_status, vol_index);
         output_this_time = 1;

      }
      else{

         LE_send_msg( GL_ERROR, 
                      "Invalid Volume Index For Product Generation Status %d\n",
                      vol_index );
         return( PS_DEF_FAILED );

      }

   /* End of "while" loop. */
   }     

   /* Flag, if set, indicates the product generation status LB needs updating. */
   if (output_this_time){

      /* Update ORPGDAT_PROD_STATUS LB. */
      Psg_output_prod_status_flag = 1;

      PSVPL_output_product_status(PS_DEF_NEW_PRODUCT);
   }

   return( PS_DEF_SUCCESS );

/* END of PSPM_chk_new_prods() */
}

/**************************************************************************
    Description:
        Builds a product status message from information contained in the
        product generatation message.  

    Input: 
        gen_msg - pointer to product generation message.

    Output: 
        prod_status - pointer to product status message.

    Returns: 
        PS_DEF_SUCCESS upon success; PS_DEF_FAILURE otherwise

    Notes:

 **************************************************************************/
static int Change_gen_msg_to_pr_format(Prod_gen_msg *gen_msg,
                                       Prod_gen_status_pr *prod_status){

   int ret, type;

   /* Transfer product id from product generation message to product status
      message. */
   prod_status->gen_status.prod_id = gen_msg->prod_id;

   /* Put product dependent parameters in product status format. */
   ret = PSCV_convert_p6(gen_msg, prod_status);
   if (ret < 0){

      LE_send_msg(GL_ERROR, "PSCV_convert_p6 Failed (Ret = %d)\n", ret);
      return PS_DEF_FAILED;
   }

   /* Set the elevation index.  If data type is TYPE_ELEVATION, 
      use the elevation index from the ORPG product header.  Otherwise,
      set to "all elevations" */
   if( (type = ORPGPAT_get_type( gen_msg->prod_id )) == TYPE_ELEVATION )
      prod_status->gen_status.elev_index = gen_msg->elev_ind;

   else
      prod_status->gen_status.elev_index = REQ_ALL_ELEVS;

   /* Set fields in product status for newly generated product. */
   prod_status->gen_status.schedule = PGS_GEN_OK;

   /* The length field determines whether the product was generated
      correctly (len >= 0) or the product was not generated owing to
      some abort condition (len < 0).  If abort condition, len specifies
      the abort reason.  */
   if( gen_msg->len >= 0 )
      prod_status->gen_status.msg_ids = gen_msg->id;

   else{

      /* Product not generated.  Determine reason and place appropriate
         abort code in msg_id field of product status message. */
      switch( gen_msg->len ){

         case PGM_CPU_LOADSHED:
         case PGM_SCAN_ABORT:
         {
            prod_status->gen_status.msg_ids = PGS_VOLUME_ABORTED;
            break;
         }
         case PGM_INPUT_DATA_ERROR:
         {
            prod_status->gen_status.msg_ids = PGS_DATA_SEQ_ERROR;
            break;
         }
         case PGM_PROD_NOT_GENERATED:
         {
            prod_status->gen_status.msg_ids = PGS_PRODUCT_NOT_GEN;
            break;
         }
         case PGM_MEM_LOADSHED:
         {
            prod_status->gen_status.msg_ids = PGS_MEMORY_LOADSHED;
            break;
         }
         case PGM_DISABLED_MOMENT:
         {
            prod_status->gen_status.msg_ids = PGS_DISABLED_MOMENT;
            break;
         }
         case PGM_TASK_FAILURE:
         {
            prod_status->gen_status.msg_ids = PGS_TASK_FAILED;
            break;
         }
         case PGM_SLOT_UNAVAILABLE:
         {
            prod_status->gen_status.msg_ids = PGS_SLOT_UNAVAILABLE;
            break;
         }
         case PGM_INVALID_REQUEST:
         {
            prod_status->gen_status.msg_ids = PGS_INVALID_PARAMS;
            break;
         }
         case PGM_TASK_SELF_TERMINATED:
         {
            prod_status->gen_status.msg_ids = PGS_TASK_SELF_TERM;
            break;
         }
         default:
         {
            prod_status->gen_status.msg_ids = PGS_VOLUME_ABORTED;
         }

      /* End of "switch". */
      }

   }
             
   /* Fill pertinent fields in product status */
   prod_status->gen_status.vol_time = gen_msg->vol_t;
   prod_status->gen_status.vol_num = gen_msg->vol_num;

   prod_status->gen_status.from.source = PS_DEF_FROM_GEN_PROD;
   PD_clear_from_line_ind_list( &prod_status->gen_status.from );

   prod_status->gen_status.req_num = gen_msg->req_num;
   prod_status->gen_status.gen_pr = 0;

   return( PS_DEF_SUCCESS );

/* END of Change_gen_msg_to_pr_format() */
}
