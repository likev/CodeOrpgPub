/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/01/22 15:59:15 $
 * $Id: BDE_elev_prod_main.c,v 1.5 2013/01/22 15:59:15 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
#include <BDE_elev_prod.h>

/*******************************************************************************

   Description:
      Builds the Base Data Elevation Product.   This process is used to support
      both legacy resolution and super resolution products.

   Inputs:
      argc - the number of command line arguments.
      argv - the command line arguments.

   Returns:
      There is no return value defined for this function.

   Notes:
      The input type of data is BASEDATA.  The TAT dicates what type of data
      BASEDATA corresponds to.

*******************************************************************************/
int main(int argc,char* argv[]) {
  
   Base_data_header *hdr = NULL;           
   Compact_basedata_elev *elev_prod = NULL;
   int size, opstatus;
   
   /* Register task */
   RPGC_reg_and_init( ELEVATION_BASED, argc, argv );

   /* Main processing loop. */
   while(1) {

      /* Suspend until driving input available. */
      RPGC_wait_act(WAIT_DRIVING_INPUT);
      
      /* Get the output buffer to store the product. */
      elev_prod = (Compact_basedata_elev *) RPGC_get_outbuf_by_name( "BASEDATA_ELEV", 
                                                                     MAX_PRODUCT_SIZE,
                                                                     &opstatus ); 

      /* If unable to acquire the output buffer, abort processing and 
         return to waiting for activation. */
      if( elev_prod == NULL ){

         RPGC_log_msg( GL_INFO, "Unable to Acquire Output Buffer of Size: %d\n",
                       MAX_PRODUCT_SIZE );
         RPGC_abort_because( opstatus );
         continue;

      }

      /* Loop through all radials of this elevation scan. */
      size = 0;
      while(1){

         short status;

         /* Get the driving input ... BASEDATA */
         hdr = (Base_data_header *) RPGC_get_inbuf_by_name( "BASEDATA", &opstatus );
      
         /* If acquisition of the input buffer fails, release all buffers
            and abort.  Then go back to waiting for activation. */
         if( opstatus != NORMAL ){

              RPGC_log_msg( GL_INFO, "RPGC_get_inbuf Returned Error (%d)\n", opstatus );
              RPGC_cleanup_and_abort( opstatus );
              break;

         }

         /* Build the base data elevation product. */
         BDE_build_product( (void *) hdr, (void *) elev_prod, &size );

         /* Get the radial status from radial header. */
         status = hdr->status;

         /* Release the input buffer. */
         RPGC_rel_inbuf( (void *) hdr );

         /* If the status indicates "end of elevation" or "end of volume",
            release the output buffer, then go back to waiting for 
            activation. */
         if( (status == GENDEL) || (status == GENDVOL) ){

            RPGC_rel_outbuf( elev_prod, FORWARD | RPGC_EXTEND_ARGS, size );
            elev_prod = NULL;
            break;

         }

      }
            
   }  
   
   return 0;

/* End of main() */
}  
