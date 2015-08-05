/* RCS info */
/* $Author: ryans $ */
/* $Locker:  $ */
/* $Date: 2007/06/26 16:10:54 $ */
/* $Id: radcdmsg_main.c,v 1.1 2007/06/26 16:10:54 ryans Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <rcm.h>

/*******************************************************************************
* Description:
*    Main function for the Radar Coded Message (radcdmsg) Task.
*
* Inputs:
*
* Outputs:
*
* Returns:
*    int
*
* Globals:
*
* Notes:
*******************************************************************************/
int main( int argc, char *argv[] )
{
   int ret;  /* generic function call return value */

   /* Register inputs and outputs (automatically done using the Task Attributes
      Table) */
   RPGC_reg_io( argc, argv );

   /* Register optional inputs */
   RPGC_in_opt( COMBATTR, RCM_IN_OPT_WAIT );
   RPGC_in_opt( ETTAB, RCM_IN_OPT_WAIT );
   RPGC_in_opt( VADTMHGT, RCM_IN_OPT_WAIT );

   /* Register ITC's for update. */
   ret=RPGC_itc_in( A314C1, &Lfm_parms.grid_lat, sizeof(a314c1_t), HYBRSCAN );

   /* Register adaptation blocks */
   RPGC_reg_RDA_control( &RDA_cntl.rdacnt_start, BEGIN_VOLUME );
   RPGC_reg_site_info( &Siteadp.rda_lat );

   /* Register Adaptation Data callback functions */
   ret = RPGC_reg_ade_callback( vad_rcm_heights_callback_fx,
      (char*)&Prod_sel.vad_rcm_heights, VAD_RCM_HEIGHTS_DEA_NAME, BEGIN_VOLUME);
   if( ret < 0 )
   {
      RPGC_log_msg( GL_ERROR,
         "RCM main: cannot register VAD_RCM_HEIGHTS adapt data callback fx.\n");
   }

   ret = RPGC_reg_ade_callback( mda_callback_fx, &Mda_adapt, MDA_DEA_NAME,
      BEGIN_VOLUME );
   if( ret < 0 )
   {
      RPGC_log_msg( GL_ERROR,
         "RCM main: cannot register MDA adaptation data callback function.\n");
   }
   ret = RPGC_reg_ade_callback( rcm_callback_fx, &Rcm_adapt, RCM_PROD_DEA_NAME,
      BEGIN_VOLUME );
   if( ret < 0 )
   {
      RPGC_log_msg( GL_ERROR,
         "RCM main: cannot register RCM adaptation data callback function.\n");
   }

   /* Register for scan summary */
   RPGC_reg_scan_summary();

   /* Register color table adaptation block. */
   if((ret = RPGC_reg_color_table((void *) &Colrtbl, BEGIN_VOLUME) < 0))
   {
      RPGC_log_msg( GL_ERROR, "Cannot Register Color Table\n");
      RPGC_hari_kiri();
   }

   /* Initialize this task.  It is volume based. */
   RPGC_task_init( VOLUME_BASED, argc, argv );

   /* Waiting for activation. */
   while(1)
   { 
      RPGC_wait_act( WAIT_DRIVING_INPUT );
      a30821_buffer_control();
   }

   return 0;

} /* end main */
