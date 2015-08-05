/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/04/20 21:32:39 $
 * $Id: orpgsum_scan_sum.c,v 1.5 2006/04/20 21:32:39 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*\//////////////////////////////////////////////////////////////////////
 Description:
        This file provides ORPG library routines for accessing the Scan
        Summary Data in ORPGDAT_SCAN_SUMMARY.

/////////////////////////////////////////////////////////////////////\*/
#include <orpgsum.h>

/* Static Global Variables */
static Summary_Data *Summary = NULL;
static int Summary_size = 0;

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Returns the scan summary data associated with volume scan or 
      volume sequence number vol_num.

   Returns:
      Pointer to Scan Summary data for volume scan "vol_num" or
      NULL on error.

//////////////////////////////////////////////////////////////////////\*/
Scan_Summary* ORPGSUM_get_scan_summary( int vol_num ){

   int vol_scan_num;

   /* Valididate the volume scan number. */
   vol_scan_num = ORPGMISC_vol_scan_num( (unsigned int) vol_num );

   /* Get the scan summary array. */
   if( (Summary = ORPGSUM_get_summary_data()) == NULL )
      return( NULL );

   /* Return a pointer to the requested data. */
   return( (Scan_Summary *) &(Summary->scan_summary[vol_scan_num]) );

/* END of ORPGSUM_get_scan_summary() */
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Returns the scan summary data.

   Returns:
      A pointer to the scan summary data, or NULL on error.

//////////////////////////////////////////////////////////////////////\*/
Summary_Data* ORPGSUM_get_summary_data( ){

   /* Free data from previous read. */
   if( Summary != NULL ){

      free(Summary);
      Summary = NULL;

   }

   if( (Summary_size = ORPGDA_read( ORPGDAT_SCAN_SUMMARY, (char **) &Summary, 
                                    LB_ALLOC_BUF,
                                    (LB_id_t) SCAN_SUMMARY_ID)) <= 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_read(ORPGDAT_SCAN_SUMMARY) failed: %d", 
                   Summary_size );
      return( NULL );

   }

   /* Return a pointer to the requested data. */
   return( Summary );

/* END of ORPGSUM_get_summary_data() */
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Set the scan summary data.

   Inputs:
      summary_data - pointer to the Summary_Data.

   Returns:
      Return value from ORPGDA_write.

//////////////////////////////////////////////////////////////////////\*/
int ORPGSUM_set_summary_data( Summary_Data *summary_data ){

   int retval;

   if( (retval = ORPGDA_write( ORPGDAT_SCAN_SUMMARY, (char *) summary_data,
                               sizeof( Summary_Data ),
                               (LB_id_t) SCAN_SUMMARY_ID)) <= 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_write(ORPGDAT_SCAN_SUMMARY) failed: %d",
                   retval );
      return( retval );

   }

   Summary_size = retval;

   /* Return value from ORPGDA_write. */
   return( Summary_size );

/* END of ORPGSUM_set_summary_data() */
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Copies the the scan summary data to user-supplied buffer.

   Inputs:
      buf - user-suppled buffer to hold scan summary data.

   Returns:
      0 on success, or -1 on error.

   Notes:   
      There are no bounds checking on size of buf.

//////////////////////////////////////////////////////////////////////\*/
int ORPGSUM_read_summary_data( void *buf ){

   /* The buffer is assumed allocated .... */
   if( buf == NULL ){

      LE_send_msg( GL_INFO, "ORPGSUM_read_summary_data( buf == NULL )\n" );
      return( -1 );

   }
 
   /* If the scan summary data has been updated or this is the first time accessing
      the data, read the data. */
  if( (Summary = ORPGSUM_get_summary_data()) == NULL )
     return( -1 ); 

   /* Copy the summmay data to user buffer. */
   memcpy( buf, Summary, Summary_size );

   /* Normal return. */
   return( 0 );

/* END of ORPGSUM_read_summary_data() */
}
