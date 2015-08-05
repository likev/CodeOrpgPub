/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/08/28 17:35:41 $
 * $Id: clutprod_rda_funcs.c,v 1.5 2008/08/28 17:35:41 cmn Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include <clutprod.h>
#include <clutter.h>


#ifdef CMD_TEST
/* The following data structures are used to construct a dummy
   clutter map for CMD testing. */
typedef struct {

    unsigned short      num_zones;    /* Number of range zones */
    ORDA_clutter_map_filter_t filter [1];

} Dummy_clutter_map_segment_t;

typedef struct {

    Dummy_clutter_map_segment_t segment [NUM_AZIMUTH_SEGS_ORDA];

} Dummy_clutter_map_data_t;
#endif


/* Function prototypes. */
static int Clutprod_get_cmd_status();


/****************************************************************

   Description:
      This module returns the configuration of the RDA. It's
      either the old legacy or the new ORDA.

   Returns:
      The current configuration.  The value can take one of 
      three macros: ORPGRDA_LEGACY_CONFIG, ORPGRDA_ORDA_CONFIG,
      or ORPGRDA_DATA_NOT_FOUND.

*****************************************************************/
int Clutprod_get_rda_config( ){

   int config_value;

   /* Get rda config. */
   config_value = ORPGRDA_get_rda_config( NULL );

   if( config_value == ORPGRDA_DATA_NOT_FOUND )
      LE_send_msg( GL_INFO, "Clutprod_get_rda_config Failed\n" );

   return config_value;

/* End of Clutprod_get_rda_config() */
}


/*****************************************************************

   Description:
      This module returns whether wideband is connected.

   Inputs:
      status_value - Value of wideband line status. 

   Outputs:
      status_value - 1 = CONNECTED, 0 = NOT CONNECTED.

   Returns:
      0 on success, < 0 on failure. 

*****************************************************************/
int Clutprod_is_wb_connected( int *status_value ){

   int get_status;

   /* Get the wideband line status. */
   get_status = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

   if( get_status == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "cfcprd_is_wb_connected Failed\n" );
      *status_value = 0;
      return ORPGRDA_DATA_NOT_FOUND;

   }

   /* Set wideband line status to "status_value" */
   if( get_status == RS_CONNECTED ){

      LE_send_msg( GL_INFO, "Wideband Line is CONNECTED\n" );
      *status_value = 1;

   }
   else{

      LE_send_msg( GL_INFO, "Wideband Line is NOT CONNECTED\n" );
      *status_value = 0;

   }

   /* Operation was successful. */
   return 0;

/* End of Clutprod_is_wb_connected() */
}


/****************************************************************

   Description:
      This module returns notchwidth map generation date and time. 

   Outputs:
      gen_date - generation date from RDA status.
      gen_time - generation time from RDA status.

   Returns:
      0 on success, < 0 on failure. 

*****************************************************************/
int Clutprod_nw_gen_date_time( int *gen_time, int *gen_date ){

   int gt, gd;

   /* Initialize the "gen_time" and "gen_date" to 0. */
   *gen_time = 0;
   *gen_date = 0;
 
   /* Get the notchwidth map generation time. */
   gt = ORPGRDA_get_status( RS_NWM_GEN_TIME );

   if( gt == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Clutprod_nw_gen_date_time() Failed\n" );
      return -1;

   }

   /* Get the notchwidth map generation date. */
   gd = ORPGRDA_get_status( RS_NWM_GEN_DATE );

   if( gd == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Clutprod_nw_gen_date_time() Failed\n" );
      return -1;

   }

   *gen_time = gt;
   *gen_date = gd;

   /* Operation was successful. */
   return 0;

/* End of Clutprod_nw_gen_date_time() */
}


/****************************************************************

   Description:
      This module returns bypass map generation date and time. 

   Outputs:
      gen_date - generation date from RDA status.
      gen_time - generation time from RDA status.

   Returns:
      0 on success, < 0 on failure. 

*****************************************************************/
int Clutprod_bp_gen_date_time( int *gen_time, int *gen_date ){

   int gt, gd;

   /* Initialize the "gen_time" and "gen_date" to 0. */
   *gen_time = 0;
   *gen_date = 0;
 
   /* Get the notchwidth map generation time. */
   gt = ORPGRDA_get_status( RS_BPM_GEN_TIME );

   if( gt == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Clutprod_bp_gen_date_time() Failed\n" );
      return -1;

   }

   /* Get the notchwidth map generation date. */
   gd = ORPGRDA_get_status( RS_BPM_GEN_DATE );

   if( gd == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Clutprod_bp_gen_date_time() Failed\n" );
      return -1;

   }

   *gen_time = gt;
   *gen_date = gd;

   /* Check if the Bypass Map was CMD generated.   This can only
      be true if ORDA. */
   Clutprod_get_cmd_status();

   /* Operation was successful. */
   return 0;

/* End of Clutprod_bp_gen_date_time() */
}

/************************************************************

   Description:
      This module reads RDA the specified clutter map from 
      the Clutter Map Linear Buffer.  The message header
      is stripped off.

   Inputs:
      type - Data type(READ_BYPASS_MAP or READ_CLUTTER_MAP).

   Returns:
      Status of the LB read operation.

*************************************************************/
int Clutprod_read_map( int type ){

   int msg_id, data_id, size, ret_size, *avail = NULL;
   char *buf = NULL; 
   short *map = NULL;

   /* Determine if RDA is legacy or ORDA */
   Rda_config = Clutprod_get_rda_config( );
   if( (Rda_config != ORPGRDA_ORDA_CONFIG) 
                   && 
       (Rda_config != ORPGRDA_LEGACY_CONFIG) ){

      RPGC_log_msg( GL_INFO | GL_ERROR, "RDA configuration unknown (%d).\n", 
                    Rda_config);
      return -1;

   }

   data_id = ORPGDAT_CLUTTERMAP;

   /* Set the message ID for the data we want to read. */
   if( type == READ_BYPASS_MAP ){

      if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

         msg_id = LBID_BYPASSMAP_LGCY;
         map = (short *) &Cbpm_legacy.clby_map_legacy.date;
         size = sizeof(Cbpm_legacy_t) - sizeof(int);
         avail = (int *) &Cbpm_legacy.bpm_avail_legacy;

      }
      else{
 
         msg_id = LBID_BYPASSMAP_ORDA;
         map = (short *) &Cbpm_orda.clby_map_orda.date;
         size = sizeof(Cbpm_orda_t) - sizeof(int);
         avail = (int *) &Cbpm_orda.bpm_avail_orda;

      }

   } 
   else if( type == READ_CLUTTER_MAP ){

      if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

         msg_id = LBID_CLUTTERMAP_LGCY;
         map = (short *) &Cnwm.nwm_map_legacy.date;
         size = sizeof(Notchwidth_map_t) - sizeof(int);
         avail = (int *) &Cnwm.nwm_avail_legacy;

      }
      else{

         msg_id = LBID_CLUTTERMAP_ORDA;
         map = (short *) &Clm.clm_map_orda.date;
         size = sizeof(Clutter_map_t) - sizeof(int);
         avail = (int *) &Clm.clm_avail_orda;

      }

   }
   else{

      /* Invalid type.  Return error. */
      RPGC_log_msg( GL_INFO | GL_ERROR, "Invalid Message ID Specified.\n" );
      return -1;

   }

   /* Read in data from LB. */
   ret_size = RPGC_data_access_read( data_id, (void *) &buf, LB_ALLOC_BUF, msg_id );
   if( ret_size > 0 ){

      int status;
      char *cpt = buf + sizeof(RDA_RPG_message_header_t);

      /* Transfer the data just read to the map array. */
      ret_size -= sizeof(RDA_RPG_message_header_t);
      if( size > ret_size )
         size = ret_size;
      
      memcpy( map, cpt, size );

      /* Validate the date/time field.  If date is <= 1, the map 
         is not valid. */
      if( map[0] <= 1 ){

         LE_send_msg( GL_INFO, "Map date/time Invalid\n" );
         status = -1;

      }
      else{

         status = 0; 

         /* If ORDA Bypass Map, get the CMD status. */
         if( msg_id == LBID_BYPASSMAP_ORDA )
            Clutprod_get_cmd_status();

      }

      /* Free memory associated with buf */
      if( buf != NULL )
         free( buf );

      /* Indicate the the map is available. */
      if( status >= 0 )
         *avail = 1;

      return status;

   }
   else{

      /* Bad return value from ORPGDA_read. */
      RPGC_log_msg( GL_INFO | GL_ERROR, "RDA Status Read Failed (%d)\n", ret_size );
      return -1;

   }

} /* End of Clutprod_read_map() */

/************************************************************

   Description:
      This module reads the latest RDA Status Message to
      determine the CMD status.

   Returns:
      0 on success or -1 on error.

*************************************************************/
static int Clutprod_get_cmd_status(){

   unsigned int cmd;

   /* Check if the Bypass Map was CMD generated.   This can only
      be true if ORDA. */
   Cbpm_orda.cmd_generated = 0;

   cmd = ORPGRDA_get_status( RS_CMD );
   if( cmd == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Unable to get CMD Status\n" );
      return -1;

   }
   else{

      Cbpm_orda.cmd_generated = cmd >> 1;
      LE_send_msg( GL_INFO, "CMD Generated Value: %d\n", Cbpm_orda.cmd_generated );
      return 0;

   }

   return 0;

} /* End of Clutprod_get_cmd_status(). */

#ifdef CMD_TEST
/************************************************************

   Description:
      This module generates a clutter map with bypass map
      in control everywhere.   Used to test CMD in a
      non-operational environment.

   Returns:
      0 on success or -1 on error.

*************************************************************/
int Clutprod_gen_clutter_map( ){

    int i, j;
    char *clm_ptr = NULL;
    Dummy_clutter_map_data_t *clm = NULL;

    /* Clutter Map generation not supported in Legacy configuration. */
    if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

        LE_send_msg( GL_INFO, "Clutter Map Generation Not Supported in Legacy Configuration\n" );
        return -1;

    }

    /* Save fields read from Bypass Map. */
    Clm.clm_avail_orda = 1;
    Clm.clm_map_orda.date = Cbpm_orda.clby_map_orda.date;
    Clm.clm_map_orda.time = Cbpm_orda.clby_map_orda.time;

    Clm.clm_map_orda.num_elevation_segs = Cbpm_orda.clby_map_orda.num_segs;

    clm_ptr = (char *) &Clm.clm_map_orda.data[0];
    for( i = 0; i < Clm.clm_map_orda.num_elevation_segs; i++ ){

        clm = (Dummy_clutter_map_data_t *) clm_ptr;

        for( j = 0; j < NUM_AZIMUTH_SEGS_ORDA; j++ ){

            clm->segment[j].num_zones = 1;
            clm->segment[j].filter[0].op_code = 1;
            clm->segment[j].filter[0].range = 511;
  
            clm_ptr += sizeof(Dummy_clutter_map_segment_t);

        } 

    }

    return 0;

/* End of Clutprod_gen_clutter_map(). */
}
#endif
