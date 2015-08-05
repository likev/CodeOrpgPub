/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2010/10/29 15:24:33 $
 * $Id: translate_read.c,v 1.2 2010/10/29 15:24:33 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <translate_main.h>

/* Static Global Variables. */
static Trans_tbl_inst_t *Translation_table;

/*******************************************************************

   Description:   
      Given the VCP number, find this VCP in the translation table.

   Inputs:        
      vcp_num - VCP number
      entry - possible values are TRNS_INCOMING_VCP or 
              TRNS_TRNS_TO_VCP

   Outputs:       
      The Current_table address is set to the appropriate
      Translation table entry.

   Notes:
      For messages from the RDA, the VCP information is translated
      if there is a translation table entry for the VCP.  

      For messages from the RPG, the decision to translate is based
      on the last VCP commanded by the operator.  Once a translatable
      VCP is selected, VCP translation will occur until a VCP is
      commanded by the operator that doesn't require translation.

*********************************************************************/
Trans_tbl_t* Find_in_translation_table( int vcp_num, int entry ){

   int retval, i;
   char *buf = NULL;
   User_commanded_vcp_t *cmded_vcp = NULL;

   static Trans_tbl_t No_translation;

   /* Initialize ... VCP not found in list. */
   No_translation.incoming_vcp = abs( vcp_num );
   No_translation.translate_to_vcp = abs( vcp_num );

   /* If the vcp_num is negative, this is a locally defined RDA VCP number
      and we assume only RPG defined VCPs can be translated. */
   if( vcp_num < 0 )
      return( &No_translation );

   /*  If this is a VCP from the RPG, does it need to be translated? */
   if( entry == TRNS_TRNS_TO_VCP ){

      if( (retval = ORPGDA_read( ORPGDAT_SUPPL_VCP_INFO, &buf, LB_ALLOC_BUF, 
                                 USER_COMMANDED_VCP_MSG_ID )) < 0 ){

         /* If the message hasn't been initialized, assume no translation. */
         if( retval == LB_NOT_FOUND )
            return( &No_translation );

         /* All other errors are fatal. */
         LE_send_msg( GL_ERROR, "ORPGDA_read( ORPGDAT_SUPPLE_VCP_INFO ) Failed: %d\n",
                      retval );
         ORPGTASK_exit( GL_EXIT_FAILURE );

      }

      cmded_vcp = (User_commanded_vcp_t *) buf;

      for( i = 0; i < Translation_table->num_tbls; i++ ){

         /* If the last commanded VCP is one which needs to be translated, assume
            the current VCP also needs to be translated .... regardless of whether
            the current VCP is the last VCP commanded. */
         if( cmded_vcp->last_vcp_commanded == Translation_table->table[i].incoming_vcp )
            break;

      }

      free( buf );

      /* If no translation table entry found, then no translation is to be performed. */
      if( i >= Translation_table->num_tbls )
         return( &No_translation );

   }

   /* See if VCP in the translation list. */
   for( i = 0; i < Translation_table->num_tbls; i++ ){

      if( (entry == TRNS_INCOMING_VCP) 
                 && 
          (Translation_table->table[i].incoming_vcp == vcp_num) ){

         /* VCP found. */
         return( &Translation_table->table[i] );

      }
      else if( (entry == TRNS_TRNS_TO_VCP)
                      &&
               (Translation_table->table[i].translate_to_vcp == vcp_num) ){

         /* VCP found. */
         return( &Translation_table->table[i] );

      }

   }

   return( &No_translation );
   
}

/*************************************************************************************

   Description:
      Reads the translation table.  This only needs to be done once since it is 
      assumed the table is not dynamic.

   Returns:
      Negative number on error, 0 on success.

**************************************************************************************/
int Read_translation_table( ){

   int ret;
   char *buf;

   ret = ORPGDA_read( ORPGDAT_SUPPL_VCP_INFO, &buf, LB_ALLOC_BUF, TRANS_TBL_MSG_ID ); 
   if( ret < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_read( ORPGDAT_SUPPL_VCP_INFO ) Failed: %d\n", ret );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   Translation_table = (Trans_tbl_inst_t *) buf;

   return 0;
}

/*************************************************************************************

   Description:
      Return 1 if translation table installed, or 0 if no table is installed.

**************************************************************************************/
int Is_translation_table_installed(){

   return( Translation_table->installed );

}
