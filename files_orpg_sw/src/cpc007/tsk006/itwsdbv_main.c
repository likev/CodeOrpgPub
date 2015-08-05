/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/02/06 20:35:02 $ */
/* $Id: itwsdbv_main.c,v 1.1 2007/02/06 20:35:02 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <itwsdbv.h>

/************************************************************************

   Description:
      Main function for the ITWS product task.

************************************************************************/
int main( int argc, char *argv[] ){

  int rc;

  /* Register inputs and outputs. */
  RPGC_reg_io( argc, argv );

  /* Register color table adaptation block. */
  if((rc = RPGC_reg_color_table((void *) &color_data, BEGIN_VOLUME) < 0)){

    RPGC_log_msg( GL_ERROR, "Cannot Register Color Table\n");
    RPGC_hari_kiri();

  }

  /* Register for scan summary updates. */
  RPGC_reg_scan_summary();

  /* Register for site info adaptation data. */
  RPGC_reg_site_info( &Siteadp.rda_lat );

  /* Tell system we are elevation-based. */
  RPGC_task_init( ELEVATION_BASED, argc, argv );

  /* Get the product id of ITWSDBV */
  if( (ITWS_prod_id = RPGC_get_id_from_name( "ITWSDBV" )) < 0 ){

    RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name() Failed\n" );
    RPGC_hari_kiri();

  }

  /* Waiting for activation. */
  while(1){
 
    RPGC_wait_act( WAIT_DRIVING_INPUT );
    A30761_buffer_control();

  }

  return 0;

/* End of main() */
}
