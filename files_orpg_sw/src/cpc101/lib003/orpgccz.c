#include <orpg.h>
#include <orpgccz.h>
#include <orpgedlock.h>
#include <orpgsmi.h>

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Get the Clutter Censor Zone data identified by "id".

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES
      buf - pointer to pointer to char
      baseline - should be either ORPGCCZ_DEFAULT or ORPGCCZ_BASELINE

   Outputs:
       buf - the address of the buffer holding the data  

   Returns:
      On success, the len of the censor zone data.  On error, a 
      negative number is returned.

   Notes:
      The user is responsible for freeing the buffer pointed to 
      by *buf.
       
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_get_censor_zones( char *id, char **buf, int baseline ){

   int len;
   extern SMI_info_t *ORPG_smi_info (char *type_name, void *data);

   /* Get the requested data.  If return value < 0, return error. */
   if( (len = DEAU_get_binary_value( id, buf, baseline )) <= 0 )
      return( len );

#ifdef LITTLE_ENDIAN_MACHINE 

   /* Perform necessary byte-swapping. */
   if (strcmp( id, ORPGCCZ_LEGACY_ZONES ) == 0) {
      SMIA_set_smi_func( ORPG_smi_info );
      SMIA_bswap_input( "RPG_clutter_regions_msg_t", *buf, len );
   }
   else if (len == sizeof (ORPG_clutter_regions_msg_t)) {/* B10 message */
      SMIA_set_smi_func( ORPG_smi_info );
      SMIA_bswap_input( "ORPG_clutter_regions_msg_t", *buf, len );
   }
   else if (len == sizeof (B9_ORPG_clutter_regions_msg_t)) {
      SMIA_set_smi_func( ORPG_smi_info );
      SMIA_bswap_input( "B9_ORPG_clutter_regions_msg_t", *buf, len );
  }
  else {
      LE_send_msg (GL_ERROR, 
			"Unexpected censor_zone message size (%d)\n", len);
      return (-1);
  }

#endif 

   /* convert to B10 */
   if (strcmp( id, ORPGCCZ_ORDA_ZONES ) == 0 &&
		len == sizeof (B9_ORPG_clutter_regions_msg_t)) {
      char *buf10;
      int i, k;
      ORPG_clutter_regions_msg_t *msg10;
      B9_ORPG_clutter_regions_msg_t *msg9;

      buf10 = malloc (sizeof (ORPG_clutter_regions_msg_t));
      if (buf10 == NULL) {
	 LE_send_msg (GL_ERROR, "malloc failed in get_censor_zones\n");
	 return (-1);
      }
      memset (buf10, 0, sizeof (ORPG_clutter_regions_msg_t));
      msg10 = (ORPG_clutter_regions_msg_t *)buf10;
      msg9 = (B9_ORPG_clutter_regions_msg_t *)(*buf);
      msg10->last_dwnld_time = msg9->last_dwnld_time;
      msg10->last_dwnld_file = msg9->last_dwnld_file;
      for (i = 0; i < MAX_CLTR_FILES; i++) {
	 msg10->file[i].file_id = msg9->file[i].file_id;
	 msg10->file[i].regions.regions = msg9->file[i].regions.regions;
	 for (k = 0; k < msg10->file[i].regions.regions; k++) {
	    msg10->file[i].regions.data[k] = msg9->file[i].regions.data[k];
	 }
      }
      free (*buf);
      *buf = buf10;
      len = sizeof (ORPG_clutter_regions_msg_t);
   }

   return len;

/* End of ORPGCCZ_get_censor_zones() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Get the Clutter Censor Zone data identified by "id".

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES
      buf - pointer to buffer holding the data.
      size - size, in bytes, of buf.
      baseline - should be either ORPGCCZ_DEFAULT or ORPGCCZ_BASELINE

   Returns:
      On success, the len of the censor zone data written.  On error, a 
      negative number is returned.

////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_set_censor_zones( char *id, char *buf, int size, int baseline ){

   int ret;
   char *tbuf = buf;
   extern SMI_info_t *ORPG_smi_info (char *type_name, void *data);

#ifdef LITTLE_ENDIAN_MACHINE

   tbuf = malloc( size );
   if( tbuf == NULL ){

      LE_send_msg( GL_ERROR, "ORPGCCZ_set_censor_zones: malloc Failed for %d Bytes\n",
                   size );
      return(-1);

   }

   memcpy( tbuf, buf, size );

   /* Perform necessary byte-swapping. */
   SMIA_set_smi_func( ORPG_smi_info );
   if( strcmp( id, ORPGCCZ_LEGACY_ZONES ) == 0 )
      SMIA_bswap_output( "RPG_clutter_regions_msg_t", tbuf, size );
   else if( strcmp( id, ORPGCCZ_ORDA_ZONES ) == 0 )
      SMIA_bswap_output( "ORPG_clutter_regions_msg_t", tbuf, size );

#endif 

   /* Set the requested data.  If return value < 0, return error. */
   ret = DEAU_set_binary_value( id, (void *) tbuf, size, baseline );

   if( tbuf != buf )
      free(tbuf);

   return ret;

/* End of ORPGCCZ_set_censor_zones() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Get info of last downloaded Clutter Censor Zone data.

   Inputs:
      buf - pointer to pointer to char

   Outputs:
       buf - the address of the buffer holding the data  

   Returns:
      On success, the len of the censor zone data.  On error, a 
      negative number is returned.

   Notes:
      The user is responsible for freeing the buffer pointed to 
      by *buf.
       
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_get_download_info( char **buf ){

   /* Get the requested data. */
   return DEAU_get_binary_value( ORPGCCZ_DOWNLOAD_INFO, buf, ORPGCCZ_DEFAULT );

/* End of ORPGCCZ_get_censor_zones() */
}

/*\/////////////////////////////////////////////////////////////////////
    
   Description:
      Copy clutter censor zones baseline to default.

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES

   Returns:
      0 on success, or negative error returned.
      
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_baseline_to_default( char *id ){

   int ret;

   /* Get the requested data.  If return value < 0, return error. */
   if( (ret = DEAU_move_baseline( id, 0 )) < 0 )
      return( ret ); 

   return 0;

/* End of ORPGCCZ_baseline_to_default() */
}

/*\/////////////////////////////////////////////////////////////////////
    
   Description:
      Copy clutter censor zones default to baseline.

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES

   Returns:
      0 on success, or negative error returned.
      
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_default_to_baseline( char *id ){

   int ret;

   /* Get the requested data.  If return value < 0, return error. */
   if( (ret = DEAU_move_baseline( id, 1 )) < 0 )
      return( ret );

   return 0;

/* End of ORPGCCZ_default_to_baseline() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Clears the edit lock.  See orpgedlock man page for details on 
      locking.

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES

   Returns:
      0 on success, or negative error returned.
   
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_clear_edit_lock( char *id ){

   int msg_id;

   /* Get the requested data element message ID.  If return value < 0, 
      return error. */
   if( (msg_id = DEAU_get_msg_id( id )) < 0 )
      return( msg_id );

   /* Clear the edit lock. */
   return( ORPGEDLOCK_clear_edit_lock( ORPGDAT_ADAPT_DATA, msg_id ) );

/* End of ORPGCCZ_clear_edit_lock() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Determines if another application has the edit lock.  See 
      orpgedlock man page for details on locking.

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES

   Returns:
      0 on success, or negative error returned.
   
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_get_edit_status( char *id ){

   int msg_id;

   /* Get the requested data element message ID.  If return value < 0, 
      return error. */
   if( (msg_id = DEAU_get_msg_id( id )) < 0 )
      return( msg_id );

   /* Get edit status. */
   return( ORPGEDLOCK_get_edit_status( ORPGDAT_ADAPT_DATA, msg_id ) );

/* End of ORPGCCZ_get_edit_status() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Sets the edit lock.  See orpgedlock man page for details on 
      locking.

   Inputs:
      id - should be either ORPGCCZ_LEGACY_ZONES or ORPGCCZ_ORDA_ZONES

   Returns:
      0 on success, or negative error returned.
   
////////////////////////////////////////////////////////////////////\*/
int ORPGCCZ_set_edit_lock( char *id ){

   int msg_id;

   /* Get the requested data element message ID.  If return value < 0, 
      return error. */
   if( (msg_id = DEAU_get_msg_id( id )) < 0 )
      return( msg_id );

   /* Set the edit lock. */
   return( ORPGEDLOCK_set_edit_lock( ORPGDAT_ADAPT_DATA, msg_id ) );

/* End of ORPGCCZ_set_edit_lock() */
}
