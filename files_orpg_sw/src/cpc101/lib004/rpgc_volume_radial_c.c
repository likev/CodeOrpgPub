/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/22 15:42:08 $
 * $Id: rpgc_volume_radial_c.c,v 1.3 2006/09/22 15:42:08 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <basedata.h>

/* File scope global variables. */
static int Use_direct = 1;	
                        /* Use ORPGDA_direct flag.  This option only works for 
                           the LB_MEMORY LB. When LB_DIRECT flag is set, 
                           the user can use function LB_direct to get the 
                           pointer pointing to the message in the shared 
                           memory.  This increases, when used for accessing 
                           the message, the efficiency by eliminating a message
                           copy, which ORPGDA_read requires. */

static int Data_id;	/* The data ID of the data of interest. */

/***********************************************************************

    Description: 
       Register "data_id" for access to the replay data base.

    Inputs:	
       data_id - product ID

    Outputs:

    Return:	
       0 on success, or a negative error return value.

    Notes:	

***********************************************************************/
int RPGC_reg_volume_data( int data_id ){

  int warehouse_id, warehouse_acct_id, ret = -1;

  /* Register with ORPGBDR the data ID. */
  warehouse_id = ORPGPAT_get_warehouse_id( data_id ); 
  warehouse_acct_id = ORPGPAT_get_warehouse_acct_id( data_id ); 

  if( (warehouse_id > 0) && (warehouse_acct_id > 0) )
     ret = ORPGBDR_reg_radial_replay_type( data_id, warehouse_id,
                                           warehouse_acct_id );

  /* Save the data ID for future reference. */
  Data_id = data_id;

  return( ret );

/* End of RPGC_reg_volume_data() */
}

/***********************************************************************

    Description: 
       Checks to see whether the data for volume "vol_seq_num", RPG 
       elevation index "elev_num" and data type "type" is available
       in the replay data base.  Data is available if the elevation
       scan is completed.

    Inputs:	
       vol_seq_num - volume scan sequence number.
       elev_num - RPG elevation index within the volume scan.
       type - type of data (e.g., REFLDATA_TYPE, COMBBASE_TYPE, etc.)

    Outputs:

    Return:	
       0 on success, -1 on error.

    Notes:	

***********************************************************************/
int RPGC_check_volume_radial( unsigned int vol_seq_num, int elev_num, 
                              unsigned int type ){

   LB_id_t msg_id;

   msg_id = ORPGBDR_check_complete_elevation( Data_id, type, vol_seq_num, elev_num );
   if( msg_id == 0 )
      return -1;

   return( ORPGBDR_set_read_pointer( Data_id, msg_id, 0 ) );

/* End of RPGC_check_volume_radial() */
}

/***********************************************************************

    Description: 
       Reads the next radial in the elevation scan.

    Inputs:	
       vol_seq_num - volume sequence number.
       elev_num - RPG elevation index.

    Outputs:
       type - type of data read (e.g., REFLDATA_TYPE, COMBBASE_TYPE, ..)
       size - size, in bytes, of the data read.

    Return:	
      Valid pointer on success, NULL on error.

    Notes:	

***********************************************************************/
char* RPGC_read_volume_radial( unsigned int vol_seq_num, int elev_num, 
                               unsigned int *type, int *size ){

   int ret = 0, *bufptr = NULL;	

   *size = 0;

   if( Use_direct ){

      ret = ORPGBDR_read_radial_direct( Data_id, &bufptr, LB_NEXT );
      if( ret == LB_NOT_FOUND )
         PS_task_abort( "ORPGBDR_read_radial_direct Failed (data_store_id: %d, LB_NEXT)\n",
                        Data_id );

      if( (ret == LB_BAD_ACCESS) || (ret == RSS_NOT_SUPPORTED) ){

         /* Direct access not supported.  Must use ORPGBDR_read_radial. */
         Use_direct = 0;
         bufptr = NULL;

      }

   }

   /* Direct access not supported.  Use ORPGBDR_read_radial(). */
   if( !Use_direct )
      ret = ORPGBDR_read_radial( Data_id, (char *) &bufptr, LB_ALLOC_BUF, LB_NEXT );

   /* A return value greater than 0 indicates the read was successful. */
   if( ret > 0 ){

      unsigned int radial_vol_seq_num;
      Base_data_header *bhd = (Base_data_header *) bufptr;

      /* Set the sub_type */
      *type = bhd->msg_type & BASEDATA_TYPE_MASK;

      /* Verify volume sequence number and elevation number.  The data read is based
         on a read pointer set is RPGC_check_volume_radial.  The check performed here is
         to ensure the data read indeed matches that which was requested.  */
      radial_vol_seq_num = ORPGMISC_vol_seq_num( (int) bhd->vol_num_quotient,
                                                 (int) bhd->volume_scan_num ) ;
   
      if( (radial_vol_seq_num != vol_seq_num)
                          ||
          (bhd->rpg_elev_ind != elev_num) ){

         /* Write error message .... we don't expect this check to fail. */
         LE_send_msg( GL_ERROR, "Vol Seq # Mismatch (%d != %d) or Elev # Mismatch (%d != %d)\n",
                      radial_vol_seq_num, vol_seq_num, bhd->rpg_elev_ind, elev_num );
         *size = 0;
         return NULL;

      }

      /* Set the size of the data read. */
      *size = bhd->msg_len * sizeof(short);

   }
   else
      return NULL;

   /* Return address of buffer holding data read. */
   return ( (char *) bufptr );

/* End of RPGC_read_volume_radial() */
}

/***********************************************************************

    Description: 
       Frees memory associated with bufptr.

    Inputs:	

    Outputs:

    Return:	

    Notes:	

***********************************************************************/
void RPGC_rel_volume_radial( char *bufptr ){

   if( !Use_direct ){
   
      if( bufptr != NULL )
         free( bufptr );

      bufptr = NULL;

   }

/* End of RPGC_rel_volume_radial() */
}

