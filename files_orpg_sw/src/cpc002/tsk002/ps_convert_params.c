/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2004/06/04 16:53:21 $ $Id:
 * ps_convert_params.c,v 1.21 1997/11/24 20:43:00 dodson Exp dodson $
 * $Revision: 1.39 $ $State: Exp $
 */

/* System Include Files/Local Include Files */
#include <prod_gen_msg.h>
#include <ps_def.h>
#include <a309.h>

#define PS_CONVERT_PARAMS
#include <ps_globals.h>
#undef PS_CONVERT_PARAMS

/* Constant Definitions/Macro Definitions/Type Definitions */

/* Note: Public function prototypes are defined in header file ps_def.h. */

/**************************************************************************
   Description: 
       Convert from a 6-product-dependent-parameter legacy format 
       to a 6-product-dependent-parameter orpg format.

       Preconditions:
          A properly-initialized ORPGDAT_PROD_GEN_MSGS message
          Storage for an internal product generation status

       Postconditions:
          NUM_PROD_DEPENDENT_PARAMS parameters in the internal product
          generation status are set based on the corresponding
          ORPGDAT_PROD_GEN_MSGS values

   Inputs:
      gen_msg - pointer to product generation message.

   Outputs:
      prod_status - pointer to product status message.

   Returns: 
      PS_DEF_SUCCESS upon success; otherwise returns PS_DEF_FAILED.

   Notes:
     Refer to the Interface Control Document (ICD) for RPG/Associated PUP.

     File scope global variables have first character capitalized and are
     defined at the top of this file.  Process scope globals variables
     begin with Psg_ and are define in ps_globals.h

 **************************************************************************/
int PSCV_convert_p6( Prod_gen_msg *gen_msg, 
                     Prod_gen_status_pr *prod_status ){

   int ind, i, num_defined_params;

   /* Debug statements ..... */
   if (Psg_verbose_level >= PS_DEF_DEBUG_VERBOSE_LEVEL){

      LE_send_msg( GL_INFO, "PSCV_convert_p6 gen_msg->prod_id: %d\n",
                   gen_msg->prod_id);

      LE_send_msg( GL_INFO, "  %d  %d  %d  %d  %d  %d",
                   gen_msg->req_params[0],
                   gen_msg->req_params[1],
                   gen_msg->req_params[2],
                   gen_msg->req_params[3],
                   gen_msg->req_params[4],
                   gen_msg->req_params[5] );

   }

   /* Initialize all product dependent parameters to PARAM_UNUSED. */
   for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; ++i)
      prod_status->gen_status.params[i] = PARAM_UNUSED;

   /* Verify this is a valid product id. */
   if( ORPGPAT_prod_in_tbl( (int) gen_msg->prod_id ) >= 0 ){

      /* Set only those parameters that are used by this product. */
      num_defined_params = ORPGPAT_get_num_parameters( (int) gen_msg->prod_id );
      for (i = 0; i < num_defined_params; ++i){

         ind = ORPGPAT_get_parameter_index( gen_msg->prod_id, i );
         if( ind >= 0 )

            /* If product is final product, get from parameters in generation
               message. */
            prod_status->gen_status.params[ind] = gen_msg->req_params[ind];

      }

   }
   else{

      LE_send_msg( GL_ERROR, "Product ID (%d) Not In Product Attributes Table\n",
                   gen_msg->prod_id );
      return( PS_DEF_FAILED );

   }

   return( PS_DEF_SUCCESS );

/* END of PSCV_convert_p6() */
}
