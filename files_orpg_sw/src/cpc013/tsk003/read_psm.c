/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/27 00:55:46 $
 * $Id: read_psm.c,v 1.2 2004/01/27 00:55:46 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/************************************************************************
 copy_precip_status_msg copies the precip status message from the linear
 buffer and puts it into an array pointed to by precip_status.
*************************************************************************/

/*** System includes ****************************************************/
#include <stdio.h>
#include <stdlib.h>

/*** ORPG includes ******************************************************/
#include <orpg.h>
#include <gagedata.h>
#include <assert.h>
#include <prod_gen_msg.h>

/* reads the precip. status message from the Gage database linear buffer */

void copy_precip_status_msg( int *precip_status )
{
   gagedata *gagedb;
   static int last_avail = 1;
   int num_items;

   int buflen, len, hdr_offset;
   int *buf;

   LB_id_t data_time;
   LB_info list;

   int debugit = 0;

/* Get some information about this Linear Buffer. */
   num_items = ORPGDA_list( GAGEDATA, &list, last_avail );
   if ( num_items <= 0 ){
       fprintf( stderr,
               "There are no GAGEDATA entries. num_items = %d\n",
               num_items );
      exit( -1 );
   }

/* Get the Linear Buffer ID. */
   data_time = list.id;

/* Calculate the size of the ORPG product header. */
   hdr_offset = sizeof( Prod_header );

/* Calculate the size of the product data with header. */
   buflen = sizeof( gagedata ) + hdr_offset;

/* Allocate space to accommodate this buffer. */
   buf = (int *) calloc( (size_t) 1,
                            (size_t) buflen );
   assert( buf != NULL );

/* Read the Gage Database Linear Buffer. */

   len = ORPGDA_read( GAGEDATA,
                      (char *) buf,
                      buflen,
                      data_time );

   if ( len <= 0 ){
      fprintf( stderr,
               "Bad len from GAGEDATA read. len = %d\n",
               len );
      exit(-1);
   }

/* Copy precip_status_info. */

   gagedb = (gagedata *) ( buf + hdr_offset / sizeof( int ) );

   precip_status[0] = gagedb->precip_status_info.date_detect_ran;

   precip_status[1] = gagedb->precip_status_info.time_detect_ran;

   precip_status[2] = gagedb->precip_status_info.date_last_precip;

   precip_status[3] = gagedb->precip_status_info.time_last_precip;

   precip_status[4] = gagedb->precip_status_info.cur_precip_cat;

   precip_status[5] = gagedb->precip_status_info.last_precip_cat;

   if ( debugit ) {
   fprintf( stderr, "date_detect_ran = %d\n",
                gagedb->precip_status_info.date_detect_ran );
   fprintf( stderr, "time_detect_ran = %d\n",
                gagedb->precip_status_info.time_detect_ran );
   fprintf( stderr, "date_last_precip = %d\n",
                gagedb->precip_status_info.date_last_precip );
   fprintf( stderr, "time_last_precip = %d\n",
                gagedb->precip_status_info.time_last_precip );
   fprintf( stderr, "cur_precip_cat = %d\n",
                gagedb->precip_status_info.cur_precip_cat );
   fprintf( stderr, "last_precip_cat = %d\n",
                gagedb->precip_status_info.last_precip_cat );
               } /* end of debugit */

/* Free memory regardless of income of read. */
   free( buf );

   return;

}
