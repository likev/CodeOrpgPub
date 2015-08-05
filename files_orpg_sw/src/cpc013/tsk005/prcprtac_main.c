/* rcs info
 * $author$
 * $locker$
 * $date$
 * $id$
 * $revision$
 * $state$
 *
*/
/***********************************************************************
   Filename: prcprtac_main.c
   Author:   Cham Pham
   Created:  11/25/04

   Description
   ===========
	This file contains the main function for the precipitation rate 
  and accumulation algorithms.
  Input:   none
  Output:  none
  Returns: none
  Globals: none
  Notes:   All input and output for this function are provided through
           ORPG API services calls.

   Change History
   ==============
   01/05/05       000          Cham Pham          CCR# NA05-01303 
   10/26/05       001          Cham Pham          CCR# NA05-21401

************************************************************************/
/* ORPG include files */
#include <rpg_globals.h>
#include <rpgc.h>
#include <rpg.h>
#include <math.h>
#include <rpgcs.h>
#include <orpgctype.h>

/* Global include files */
#include <prcprtac_main.h>
#include <a3146.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

#define PCODE 0			/* The product code                           */
#define RHALF_DENOM 2.0		/* Constants representing the value 2.0       */

/* Declare global variables */
static char *beg_a314c1 = ( (char *)&a314c1 );
double zr_pwr_coeff, zr_mlt_coeff, min_dbz, max_dbz;

int main( int argc, char *argv[] )
{
/* Variable declarations */
double ar_const;
int   i, rc;
int   algProcess = 1; 
int   abort_flg;
int   iostatus;
/* ----------------------------------------------------------------------- */
   fprintf( stderr,"\nBegin CP013 Task 5: Rate Accumulation Algorithm\n" );

/* Initialize log_error services. */
   RPGC_init_log_services( argc, argv );

/* Register input */
   RPGC_in_data( HYBRSCAN, VOLUME_DATA );

/* Register output */
   RPGC_out_data( HYACCSCN, VOLUME_DATA, PCODE );

/* Register itc output */
   RPGC_itc_out( A314C1, (char *)beg_a314c1, sizeof(a314c1_t), ITC_ON_CALL );

/* Register for site info adaptation data */
   rc = RPGC_reg_site_info( &sadpt );

   if ( rc < 0 ) 
   {
     RPGC_log_msg( GL_ERROR, 
        "SITE INFO: cannot register adaptation data callback function\n");
   }

/* ORPG task initialization routine. Input parameters argc/argv are
   not used in this algorithm 
 */ 
   RPGC_task_init( VOLUME_BASED, argc, argv );

   if (DEBUG) {fprintf(stderr,"Initialized task............\n");}

/* Initialize hydromet file */
   initialize_file_io( &iostatus );

   if ( iostatus == IO_OK ) 
   {

   /* Build lfm lookup tables*/
     build_lfm_lookup( );

   /* Calculate constant values for bin areas along radial
      first initialize area constant 
    */
     ar_const = RHALF_DENOM * PI * r_bin_size / MAX_AZMTHS;

    /* Do for each bin along a radial */
     for ( i=0; i<MAX_RABINS; i++ ) 
     {
       /* Calculate bin range */
       a313hlfm.bin_range[i] = (i+1) * r_bin_size - r_mid_ofs;

       /* Calculate bin area */
       a313hlfm.bin_area[i] = ar_const * a313hlfm.bin_range[i];
     }

   }/* End if block iostatus equals to IO_OK */

/* Unsuccessful i/o... remove task from rpg processing */
   else 
   {
     RPGC_hari_kiri( );
   }

/* Initialize Z-R relationship and min/max reflectivity variables */ 
   zr_pwr_coeff = -99.;
   zr_mlt_coeff = -99.;
   min_dbz = -99.;
   max_dbz = -99.;
   if (DEBUG) {ivol = 1;}

   while ( algProcess )
   {
     RPGC_wait_act( WAIT_DRIVING_INPUT ); 

     if (DEBUG) 
       {fprintf(stderr,"=============== VOL %d  ============\n",ivol);}

     Rate_Buffer_Control( &abort_flg );

   /* If there was not an abort conditon in rate, then ...*/
     if ( abort_flg == FLAG_CLEAR )
     {
       Accum_Buffer_Control( );
     }

     if ( DEBUG ) {ivol = ivol + 1;}

   }/* End while(algProcess) loop */

  return (0);
}
