/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/10 21:30:43 $
 * $Id: rpgc_blockage_data_funcs.c,v 1.2 2012/01/10 21:30:43 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <orpg.h>

#define NUM_GATES_BLOCKAGE_DATA 	(3600*230)

/* Global Variables. */
static int Last_msg_num = -1;		/* Last message read. */

static int Initialized = 0;		/* Flag used to indicate the blockage
                                           file has been ORPGDA_stat()'d */

static char *Blockage_data = NULL;	/* Decompressed blockage data for one
                                           elevation (corresponds to the message
                                           Last_msg_num). */

static int Num_msgs = 0;		/* Number of messages in the blockage LB. */

/* Function Prototypes. */
static int Initialize_blockage_LB_stat();

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Given elevation angle and azimuth angle, return the blockage array.

   Inputs:
      elev_deg - elevation angle, in degrees.
      az_deg - azimuth angle, in degrees.

   Returns:
      Pointer to character array containing blockage values along a radial 
      specified in percent blockage [0 - 100].  On error, NULL is returned.

///////////////////////////////////////////////////////////////////////////\*/
unsigned char* RPGC_blockage_data_lookup( float elev_deg, float az_deg ){

   int msg_num_req, opstatus, size;
   int az_index, loc_in_buf;
 
   /* Validate the azimuth. */
   while( az_deg < 0.0 )
      az_deg += 360.0;

   while( az_deg >= 360.0 )
      az_deg -= 360.0;
 
   /* Check and see if we have this data already in memory and 
      can skip the actual lookup. */
   msg_num_req = (int) roundf( elev_deg*10.0f ) + 10;

   /* Is the elevation of interest the last elevation read? */
   if( msg_num_req != Last_msg_num ){

      /* Need to read the elevation.  Make sure this module is
         initialized first. */
      if( !Initialized ){

         opstatus = Initialize_blockage_LB_stat();
         if( opstatus < 0 )
            return NULL;

      }

      if( Num_msgs <= msg_num_req ){

         /* Data request is above available blockage data.   Therefore there
            is no blockage. */
         return NULL; 

      }

      /* Free memory from previous read. */
      if( Blockage_data != NULL ){

         free( Blockage_data );
         Blockage_data = NULL;

      }
   
      /* Read the data from the linear buffer. */
      size = ORPGDA_read( ORPGDAT_BLOCKAGE, &Blockage_data, LB_ALLOC_BUF, 
                          msg_num_req );

      if( size < 0 ){

         LE_send_msg( GL_INFO, "ORPGDA_read( ORPGDAT_BLOCKAGE ) Failed: %d\n", 
                      size );
         return NULL;

      }

      /* Set Last_msg_num so that we know which data is already in the buffer. */
      Last_msg_num = msg_num_req;  

   }

   if( Blockage_data == NULL )
      return NULL;

   /* Use current Blockage_data array for lookup.  Find the azimuth index that we 
      are looking for.  The data is stored in 10th of degrees in azimuth. */
   az_index = (int) roundf( az_deg*10 );
   loc_in_buf = az_index*230;
   return ( (unsigned char *) &Blockage_data[loc_in_buf] );

}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Initializes the blockage LB status information.

/////////////////////////////////////////////////////////////////////////\*/
static int Initialize_blockage_LB_stat( ){

   LB_status lbstatus;
   int opstatus;

   /* Initialize lB_status data structure elements. */
   lbstatus.attr = NULL;
   lbstatus.check_list = NULL;
   lbstatus.n_check = 0;

   opstatus = ORPGDA_stat( ORPGDAT_BLOCKAGE, &lbstatus );
   if( opstatus < 0 ){

      LE_send_msg( GL_INFO, "ORPGDA_stat( ORPGDAT_BLOCKAGE ) Failed: %d\n",
                   opstatus );
      return( opstatus );

   }

   /* Set the number of messages in the blockage LB.  Each message
      corresponds to an elevation angle in 10ths of a degree. */
   Num_msgs = lbstatus.n_msgs;

   /* Set flag indicating the data has been read ... doesn't need to
      be read again. */
   Initialized = 1;

   return 0;

/* End of Initialize_blockage_LB_stat(). */
}
