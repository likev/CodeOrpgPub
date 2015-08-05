#include <orpggdr.h>
#include <string.h>
#include <infr.h>

/*****************************************************************

   Description:
      Given a pointer to the generic digital radar data and 
      the type of data block of interest, return a pointer
      to the data block.

   Inputs:
      rda_msg - pointer to generic digital radar data message. 
      type - type of data block of interest.

   Returns:
      Pointer to data block or NULL if not found.

******************************************************************/
char* ORPGGDR_get_data_block( char *rda_msg, int type ){

   Generic_basedata_t* rec = (Generic_basedata_t *) rda_msg;
   int i;

   /* Find the data block of interest. */
   for( i = 0; i < rec->base.no_of_datum; i++ ){

      Generic_any_t *data_block;
      char str_type[5];

      data_block = (Generic_any_t *)
                 (rda_msg + sizeof(RDA_RPG_message_header_t) + rec->base.data[i]);

      /* Convert the name to a string so we can do string compares. */
      memset( str_type, 0, 5 );
      memcpy( str_type, data_block->name, 4 );

      if( (type == ORPGGDR_RVOL) && (strstr( str_type, "RVOL" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_RELV) && (strstr( str_type, "RELV" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_RRAD) && (strstr( str_type, "RRAD" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DREF) && (strstr( str_type, "DREF" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DVEL) && (strstr( str_type, "DVEL" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DSW) && (strstr( str_type, "DSW" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DRHO) && (strstr( str_type, "DRHO" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DPHI) && (strstr( str_type, "DPHI" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DZDR) && (strstr( str_type, "DZDR" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DSNR) && (strstr( str_type, "DSNR" ) != NULL) )
         return( (char *) data_block );

      if( (type == ORPGGDR_DRFR) && (strstr( str_type, "DRFR" ) != NULL) )
         return( (char *) data_block );

   }

   return NULL;

/* End of ORPGGDR_get_data_block() */
}
