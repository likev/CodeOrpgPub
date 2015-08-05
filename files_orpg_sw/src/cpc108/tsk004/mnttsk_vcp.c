/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/02/11 17:10:13 $
 * $Id: mnttsk_vcp.c,v 1.33 2014/02/11 17:10:13 steves Exp $
 * $Revision: 1.33 $
 * $State: Exp $
 */

#include <mnttsk_vcp.h>
#include <ctype.h>

#define CFG_NAME_SIZE		128
#define AZI_RATE_SCALE          (22.5/16384)
#define DEFAULT_NUM_PRFS        5
#define INITIAL_PRF             4
#define MAX_DOP_PRI             8
#define MIN_SNR_THRESHOLD       -12.0

/* Static Global Variables */
static char Cfg_dir            [CFG_NAME_SIZE];
					/* config directory pathname */
static char Vcp_dir_name[CFG_NAME_SIZE];
					/* directory name where VCPs are stored */
static char Vcp_basename[CFG_NAME_SIZE];
                                        /* base file name for standard VCPs */
static char Vcp_site_basename[CFG_NAME_SIZE];
                                        /* base file name for site-specific VCPs */

static int Verbose_mode;		/* Verbose mode. */

static Vcp_struct Vcp;
static Vcp_alwblprf_t Allowable_prfs;
static int Allowable_prfs_n_ele = 0;
static short Rdccon[ECUTMAX];
static int  unambiguous_range[DELPRI_MAX][PRFMAX] = { { 460,332,230,173,146,135,125,115 },
                                                      { 463,334,232,174,147,136,126,116 },
                                                      { 466,336,233,175,148,137,127,117 },
                                                      { 468,338,234,176,149,138,128,118 },
                                                      { 471,340,236,177,150,139,129,119 } };
static float prfvalue[PRFMAX] = { 321.888,446.428,643.777,857.143,1013.51,1094.89,
                                  1181.00,1282.05 };

static int delta_pri = 3;



/* Static Function Prototypes */
static int    Init_VCP_tbl( int wx_mode, int where_defined, short vcp_flags );
static int    Read_VCP_attrs( int *wx_mode, int *where_defined, short *vcp_flags );
static int    Read_elev_cut_attrs( int num_elevs );
static int    Read_alwb_prf_pulse_counts( int ele_num );
static char*  Set_elev_key( int elev_cut );
static char*  Set_sector_key( int sector_num );
static int    Verify_rdccon();
static int    Set_config_file_names( );
static int    Get_next_file_name( char *dir_name, char *basename,
                                  char *buf, int buf_size );
static int    Get_command_line_options( int argc, char *argv[], int *startup_type,
                                        int *vcp_num );
static int Get_trans_tbl_config_file_name( char *tmpbuf );

/*\////////////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Adds/removes VCP data from RPG adaptation data.   The VCPs are assumed defined
//      in configuration file located in $CFGDIR/vcp
//
//   Inputs:
//      argc - number of command line arguments
//      argv - the command line arguments
//
//   Notes:
//      Normal termination requires exit(0).  Abnormal termination requires non-zero
//      exit code.
//
////////////////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int retval;
   int startup_type;
   int vcp_num;

   /* Initialize log-error services. */
   (void) ORPGMISC_init(argc, argv, 5000, 0, -1, 0) ;

   /* Get the command line options. */
   if( (retval = Get_command_line_options( argc, argv, &startup_type,
                                           &vcp_num )) < 0 )
      exit(1) ;

   /* Process startup command line option. */
   if( MNTTSK_VCP_tables( startup_type, vcp_num ) < 0 )
      exit(1);

   if( MNTTSK_VCP_translation_tables( startup_type ) < 0 )
      exit(2);

   if( MNTTSK_PRF_commands( startup_type ) < 0 )
      exit(3);

   if( MNTTSK_init_RDA_RDACNT( startup_type ) < 0 )
      exit(4);

   /* Normal termination. */
   exit(0);

} /* End of main() */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Reads and initializes the PRF Command message.
//
//   Inputs:
//      startup_action - start up type.
//
//   Outputs:
//
//   Returns:
//      Negative value on error, or 0 on success.
//      
//   Notes:
//
//////////////////////////////////////////////////////////////////\*/
static int MNTTSK_PRF_commands( int startup_action ){

   Prf_command_t Prf_command;
   Prf_status_t Prf_status;
   int i, ret;

   /* Open LB for write access. */
   ORPGDA_write_permission( ORPGDAT_PRF_COMMAND_INFO );

   /* Read the PRF Command LB. */
   ret = ORPGDA_read( ORPGDAT_PRF_COMMAND_INFO, &Prf_command,
                      sizeof(Prf_command_t), ORPGDAT_PRF_COMMAND_MSGID );

   /* A return value of 0 or negative number implies the message needs 
      to be initialized. */
   if( ret == sizeof(Prf_command_t) ){

      LE_send_msg( GL_INFO, "PRF Command Exists\n" );
      LE_send_msg( GL_INFO, "--->command:  %d\n", Prf_command.command );
      LE_send_msg( GL_INFO, "--->storm_id: %s\n", &Prf_command.storm_id[0] );
      return 0;

   }

   /* Initialize the PRF Command buffer. */
   Prf_command.command = PRF_COMMAND_STORM_BASED;
   LE_send_msg( GL_INFO, "Setting PRF Command to PRF_COMMAND_STORM_BASED\n" );

   memset( &Prf_command.storm_id[0], 0, MAX_CHARS );
      
   /* Write out the PRF Command buffer. */
   ret = ORPGDA_write( ORPGDAT_PRF_COMMAND_INFO, (char *) &Prf_command,
                       sizeof(Prf_command_t), ORPGDAT_PRF_COMMAND_MSGID );

   /* On error, report error code and return error. */
   if( ret < sizeof(Prf_command_t) ){

      LE_send_msg( GL_ERROR, "ORPGDA_write( ORPGDAT_PRF_COMMAND_INFO ) Failed: %d\n", ret );
      return -1;

   }

   /* Read the PRF Status. */
   ret = ORPGDA_read( ORPGDAT_PRF_COMMAND_INFO, &Prf_status,
                      sizeof(Prf_status_t), ORPGDAT_PRF_STATUS_MSGID );

   /* A return value of 0 or negative number implies the message needs to be
      initialized. */
   if( ret == sizeof(Prf_status_t) ){

      LE_send_msg( GL_INFO, "PRF Status Exists\n" );
      LE_send_msg( GL_INFO, "--->state:              %d\n", Prf_status.state );
      LE_send_msg( GL_INFO, "--->error_code:         %d\n", Prf_status.error_code );
      LE_send_msg( GL_INFO, "--->radius:             %d\n", Prf_status.radius );
      LE_send_msg( GL_INFO, "--->num_storms:         %d\n", Prf_status.num_storms );
      LE_send_msg( GL_INFO, "--->num_storms_tracked: %d\n", Prf_status.num_storms_tracked );

      return 0;

   }

   /* Initialize the PRF Status. */
   Prf_status.state = Prf_command.command;
   Prf_status.error_code = PRF_STATUS_NO_ERRORS;
   Prf_status.num_storms = 0;
   Prf_status.radius = 20;
   memset( &Prf_status.storm_data[0], 0, MAX_STORMS*sizeof(Storm_data_t) );

   Prf_status.num_storms_tracked = 0;
   for( i = 0; i < MAX_TRACKED; i++ )
      Prf_status.ids_storms_tracked[i] = -1;

   /* Write out the PRF Status buffer. */
   ret = ORPGDA_write( ORPGDAT_PRF_COMMAND_INFO, (char *) &Prf_status,
                       sizeof(Prf_status_t), ORPGDAT_PRF_STATUS_MSGID );

   /* On error, report error code and return error. */
   if( ret < sizeof(Prf_status_t) ){

      LE_send_msg( GL_ERROR, "ORPGDA_write( ORPGDAT_PRF_STATUS_INFO ) Failed: %d\n", ret );
      return -1;

   }

   /* Write the initialization values to log file. */
   LE_send_msg( GL_INFO, "Initialized ORPGDAT_PRF_STATUS_INFO (ret: %d)\n", ret );
   LE_send_msg( GL_INFO, "--->State:           %d\n", Prf_status.state );
   LE_send_msg( GL_INFO, "--->Error Code:      %d\n", Prf_status.error_code );
   LE_send_msg( GL_INFO, "--->Num Storms:      %d\n", Prf_status.num_storms );
   LE_send_msg( GL_INFO, "--->Radius:          %d\n", Prf_status.radius );
   LE_send_msg( GL_INFO, "--->Storms Tracked:  %d\n", Prf_status.num_storms_tracked );

   /* Return normal. */
   return 0;

} /* End of MNTTSK_PRF_commands(). */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Initializes the RDA_RDACNT data. 
//
//   Inputs:
//      startup_action - start up type.
//
//   Outputs:
//
//   Returns:
//      Negative value on error, or 0 on success.
//      
//   Notes:
//
//////////////////////////////////////////////////////////////////\*/
static int MNTTSK_init_RDA_RDACNT( int startup_action ){

   int ret, pos, def_vcp, ind;

   RDA_rdacnt_t rda_vcp;
   Siteadp_adpt_t site_data;
   VCP_ICD_msg_t *c_vcp = NULL;

   short *rdcvcpta = NULL, *rdccon = NULL;
   float first_angle = -1.0;

   /* Initialize the data. */
   memset( &rda_vcp, 0, sizeof(RDA_rdacnt_t) );
   memset( &site_data, 0, sizeof(Siteadp_adpt_t) );

   /* Open LB for write access. */
   ORPGDA_write_permission( ORPGDAT_PRF_COMMAND_INFO );

   /* Read the PRF Command LB. */
   ret = ORPGDA_read( ORPGDAT_ADAPTATION, &rda_vcp, sizeof(RDA_rdacnt_t), 
                      RDA_RDACNT );

   /* A return value of 0 or negative number implies the message needs 
      to be initialized. */
   if( ret == sizeof(RDA_rdacnt_t) ){

      LE_send_msg( GL_INFO, "RDA_RDACNT Exists\n" );
      return 0;

   }

   /* The data does not exist.  Initialize the first element with info
      on the default VCP for the default weather mode. */
   ret = ORPGSITE_get_site_data( &site_data );
   if( ret < 0 ){

      /* This should not happen ... if it does, report it and move on. */
      LE_send_msg( GL_ERROR, "ORPGSITE_get_site_data Failed: %d\n", ret );
      return 0;

   }

   /* Get the default wxmode and for the default, get the default VCP. */
   if( site_data.wx_mode == CLEAR_AIR_MODE )
      def_vcp = site_data.def_mode_B_vcp;

   else
      def_vcp = site_data.def_mode_A_vcp;

   /* Get the VCP data for the default VCP. */
   pos = ORPGVCP_index( def_vcp );
   if( pos < 0 ){

      /* This should not happend ... if it does, report it and move on. */ 
      LE_send_msg( GL_ERROR, "ORPGVCP_index(%d) Failed: %d\n", def_vcp, pos );
      return 0;

   }

   rdcvcpta = ORPGVCP_ptr( pos );
   rdccon = ORPGVCP_elev_indicies_ptr( pos );

   if( (rdcvcpta == NULL) || (rdccon == NULL) ){

      /* This should not happend ... if it does, report it and move on. */ 
      LE_send_msg( GL_ERROR, "ORPGVCP_ptr/ORPGVCP_elev_indicies_ptr Failed\n" );
      return 0;

   }
   
   /* Start initializing. */
   rda_vcp.data[0].volume_scan_number = 0;
   memcpy( &rda_vcp.data[0].rdcvcpta[0], rdcvcpta, VCPSIZ*sizeof(short) );
   memcpy( &rda_vcp.data[0].rdccon[0], rdccon, ECUTMAX*sizeof(short) );

   rda_vcp.last_entry = 0;
   rda_vcp.last_entry_time = time(NULL);
   rda_vcp.last_entry_time = time(NULL);

   /* If there are supplemental cuts, then the elevation angle should 
      match the first cut angle. */
   c_vcp = (VCP_ICD_msg_t *) rdcvcpta;
   first_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                      c_vcp->vcp_elev_data.data[0].angle );

   /* Do For All elevation cuts in the VCP ... */
   for( ind = 0; ind < c_vcp->vcp_elev_data.number_cuts; ind++ ){

      int waveform = (int) c_vcp->vcp_elev_data.data[ind].waveform;
      int phase  = (int) c_vcp->vcp_elev_data.data[ind].phase;
      int super_res = (int) c_vcp->vcp_elev_data.data[ind].super_res;
      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                              c_vcp->vcp_elev_data.data[ind].angle );

      /* Set waveform bit. */
      if( waveform == VCP_WAVEFORM_CS )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_CS;

      else if( waveform == VCP_WAVEFORM_CD )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_CD;

      else if( waveform == VCP_WAVEFORM_CDBATCH )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_CDBATCH;

      else if( waveform == VCP_WAVEFORM_BATCH )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_BATCH;

      else if( waveform == VCP_WAVEFORM_STP )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_SPRT;

      /* Set phase bit. */
      if( phase == VCP_PHASE_SZ2 )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_SZ2;

      /* Set super resolution  bit. */
      if( super_res & VCP_HALFDEG_RAD )
         rda_vcp.data[0].suppl[ind] |= RDACNT_IS_SR;

      /* Until I figure out a better way to determine supplemental
         cuts, if the elevation angle matches the lowest cut, it
         is supplemental. */
      if( (rdccon[ind] > 1) && (elev_angle == first_angle) )
         rda_vcp.data[0].suppl[ind] |= RDACNT_SUPPL_SCAN;

   }

   /* Write out the data to LB. */
   ret = ORPGDA_write( ORPGDAT_ADAPTATION, (char *) &rda_vcp, 
                       sizeof(RDA_rdacnt_t), RDA_RDACNT );
   if( ret < 0 )
      LE_send_msg( GL_ERROR, "Failed Writing RDA_RDACNT Data: %d\n", ret );

   LE_send_msg( GL_INFO, "Initialized ORPGDAT_ADAPTATION, RDA_RDACNT (ret: %d)\n", ret );
   LE_send_msg( GL_INFO, "--->Default VCP (%d) for Default Wx Mode Used.\n", def_vcp );

   return 0;

} /* End of MNTTSK_init_RDA_RDACNT() . */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Reads and parsers the VCP translation table. 
//
//   Inputs:
//      startup_action - start up type.
//
//   Outputs:
//
//   Returns:
//      Negative value on error, or 0 on success.
//      
//   Notes:
//
//////////////////////////////////////////////////////////////////\*/
int MNTTSK_VCP_translation_tables( int startup_action ){

   char table_name[CFG_NAME_SIZE];
   int incoming_vcp, translate_to_vcp, elev_cut_map_size;
   int cnt, ret, elev_cut_map[ VCP_MAXN_CUTS ];

   Trans_tbl_inst_t *table_installed = NULL;
   Trans_list_t *List_head = NULL, *list = NULL, *node = NULL;
   int num_nodes = 0;

   /* Initialially indicate the VCP Translation Table is not installed.  
      If table exists and not errors are detected, then the table will 
      be indicated as installed. */

   table_installed = (Trans_tbl_inst_t *) calloc( sizeof( Trans_tbl_inst_t ), 1 );
   if( table_installed == NULL ){

      LE_send_msg( GL_ERROR, "calloc Failed for %d bytes\n", sizeof(Trans_tbl_inst_t) );
      return -1;

   }

   if( (ret = ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO, (char *) &table_installed, 
                            sizeof(Trans_tbl_inst_t), TRANS_TBL_MSG_ID ) < 0) ){

      LE_send_msg( GL_ERROR, 
              "ORPGDA_write(ORPGDAT_SUPPL_VCP_INFO) Failed: %d\n", ret );
      free(table_installed);

      return -1;

   }

   /* Initialize the list head. */
   List_head = NULL;

   /* Set the name of the CS file containing the translation definitions. */
   Get_trans_tbl_config_file_name( table_name );
   LE_send_msg( GL_INFO, "VCP Translation Table: %s\n", table_name );

   /* Open the CS file.  File existence is not mandatory. */
   CS_cfg_name ( table_name );
   CS_control (CS_COMMENT | '#');

   /* Parse the CS file ....... */

   /* Verify the VCP translation section exist. */
   if( (CS_entry (TRNS_CS_ATTR_KEY, 0, 0, NULL) < 0 ) || (CS_level( CS_DOWN_LEVEL ) < 0 )){

      LE_send_msg( GL_ERROR, "Could Not Find %s Key\n", TRNS_CS_ATTR_KEY );
      free(table_installed);
      return 0;

   }

   /* Initialize return code. */
   ret = 0;

   do {

      /* Step down into the VCP Translation Table ... */
      if (CS_level(CS_DOWN_LEVEL) < 0) 
         continue ;

      /* Parse the "incoming_vcp", "translate_to_vcp" and "elev_cut_map_size" keys. */
      if( (CS_entry( TRNS_CS_INCOMING_KEY, TRNS_CS_INCOMING_TOK, 0, (void *) &incoming_vcp ) <= 0)
                                          ||
          (CS_entry( TRNS_CS_MAP_SIZE_KEY, TRNS_CS_MAP_SIZE_TOK, 0, (void *) &elev_cut_map_size  ) <= 0)
                                          ||
          (CS_entry( TRNS_CS_TRANSLATE_KEY, TRNS_CS_TRANSLATE_TOK, 0, (void *) &translate_to_vcp  ) <= 0) ){

         LE_send_msg( GL_ERROR, "Error parsing incoming_vcp: %di, translate_to_vcp: %d\n",
                      incoming_vcp, translate_to_vcp );
         ret = -1;
         break;

      }

      /* Parse the elevation cut mapping table. */
      cnt = 0;
      while( CS_entry( TRNS_CS_CUT_MAP_KEY, ((cnt + 1) | CS_INT), 0,
                       (void *) &elev_cut_map[ cnt ] ) > 0 )
         cnt++;

      if( cnt != elev_cut_map_size ){

         LE_send_msg( GL_ERROR, "Error parsing for elev_cut_map\n" );
         ret = -1;
         break;

      }

      /* Allocate a new list member and fill in the list entries. */
      list = (Trans_list_t *) calloc( 1, sizeof( Trans_tbl_t ) );
      if( list == NULL ){

         ret = -1;
         break;

      }

      list->table.incoming_vcp = incoming_vcp;
      list->table.translate_to_vcp = translate_to_vcp;
      list->table.elev_cut_map_size = elev_cut_map_size;
      memcpy( list->table.elev_cut_map, elev_cut_map, elev_cut_map_size*sizeof(int) );
      list->next = NULL;

      LE_send_msg( GL_INFO, "Incoming VCP %d is to be translated to %d\n",
                   list->table.incoming_vcp, list->table.translate_to_vcp );

      /* Add the node to the list. */
      num_nodes++;
      if( List_head == NULL )
         List_head = list;

      else{

         node = List_head;
         while( node->next != NULL )
            node = node->next;

          node->next = list;

      }

      /* Go to next translation entry. */
      (void) CS_level(CS_UP_LEVEL) ;

   } while (CS_entry(CS_NEXT_LINE, 0, 0, (char *) NULL) >= 0); 

   /* Publish the translation table. */
   if( num_nodes > 0 ){

      int table_size = (num_nodes-1)*sizeof(Trans_tbl_t) + sizeof(Trans_tbl_inst_t);
      table_installed = (Trans_tbl_inst_t *) realloc( table_installed, table_size );
      if( table_installed == NULL ){

         LE_send_msg( GL_ERROR, "realloc Failed for %d bytes\n", table_size );
         return -1;

      }

      table_installed->installed = 1;
      table_installed->num_tbls = num_nodes;
      num_nodes = 0;
      for( cnt = 0; cnt < table_installed->num_tbls; cnt++ ){

         char map[256];
         char cut[4];
         int i;

         /* By requirement, a VCP cannot be tranlatable unless it is 
            experimental. */
         int retval = ORPGVCP_is_vcp_experimental( (int) List_head->table.incoming_vcp );
 
         /* A value other than 1 indicates either error (-1) or VCP is not
            experimental (0). */
         if( retval < 1 ){

            if( retval < 0 )
               LE_send_msg( GL_ERROR, "ORPGVCP_is_vcp_experimental(%d) Failed\n", retval );

            continue;

         }

         LE_send_msg( GL_INFO, "Experimental VCP To be Translated: %d\n", List_head->table.incoming_vcp );
         LE_send_msg( GL_INFO, "--->VCP translated to:      %d\n", List_head->table.translate_to_vcp );
         LE_send_msg( GL_INFO, "--->Elevaton Cut Map size:  %d\n", List_head->table.elev_cut_map_size );
         
         memset( map, 0, 256 );
         memset( cut, 0, 4 );
         for( i = 0; i < List_head->table.elev_cut_map_size; i++ ){

            sprintf( cut, "%-3d", List_head->table.elev_cut_map[i] );
            strcat( map, cut );

         }

         LE_send_msg( GL_INFO, "--->Elevaton Cut Map:       %s\n", map );
        
         /* VCP is experimental ...... */
         memcpy( (char *) &table_installed->table[num_nodes], (char *) List_head, 
                 sizeof( Trans_tbl_t ) );
         num_nodes++;
         node = List_head->next;
         free( List_head );
         List_head = node;

      }

      if( num_nodes <= 0 )
         table_installed->installed = 0;

      table_installed->num_tbls = num_nodes;
      ret = ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO, (char *) table_installed, 
                          table_size, TRANS_TBL_MSG_ID );

   }

   if( table_installed != NULL )
      free(table_installed);

   return ret;
}
/* End of MNTTSK_VCP_translation_tables() */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Read VCP configuration files and installs the VCP data.
//
//   Inputs:
//      startup_action - start up type.
//      vcp - volume coverage pattern number.
//
//   Outputs:
//
//   Returns:
//      Negative value on error, or 0 on success.
//      
//   Notes:
//      vcp configuration file names are either of the form "vcp_xxx"
//      where xxx is the VCP number, or "ICAO_vcp_xxx" where ICAO
//      is the 4-letter site name or ICAO.   The ICAO must be 
//      either all uppercase or all lowercase.
//
//////////////////////////////////////////////////////////////////\*/
int MNTTSK_VCP_tables( int startup_action, int vcp_num ){

   char ext_name[ CFG_NAME_SIZE ], *call_name;
   int ret, wx_mode, where_defined;
   short vcp_flags = 0;

   /* Initialize the configuration file names. */
   Set_config_file_names(); 

   /* Clear start.  If VCP number is not specified, then each VCP 
      defined by configuration file is removed.  If VCP number is
      specified, only that VCP is removed. */
   if( startup_action == CLEAR ){

      if( vcp_num == 0 ){

         /* Read all the standard VCP files. */
         call_name = Vcp_dir_name;
         while( Get_next_file_name( call_name, Vcp_basename,
                                    ext_name, CFG_NAME_SIZE ) == 0 ){

            /* Remove any previous definition. */
            memset( (void *) &Vcp, 0, sizeof(Vcp_struct) );

            LE_send_msg( GL_INFO, "Processing File: %s\n", ext_name );

            /* Specifies the CS configuration file name and which 
               character is interpreted as comment. */
            CS_cfg_name ( ext_name );
            CS_control (CS_COMMENT | '#');

            /* Read the VCP attributes section.  We need to do this
               to get the VCP number. */
            if( Read_VCP_attrs( &wx_mode, &where_defined, &vcp_flags ) < 0 )
               continue;

            /* Remove this VCP from RPG adaptation data. */
            LE_send_msg( GL_INFO, "Deleting VCP %d\n", Vcp.vcp_num );
            ORPGVCP_delete( Vcp.vcp_num );

            call_name = NULL;

         }

         /* Looking for site-specific files is only necessary if
            Vcp_basename and Vcp_site_basename are not identical
            strings. */
         if( strcmp( Vcp_basename, Vcp_site_basename ) != 0 ){

            /* Read all the site-specific VCP files. */
            call_name = Vcp_dir_name;
            while( Get_next_file_name( call_name, Vcp_site_basename,
                                       ext_name, CFG_NAME_SIZE ) == 0 ){

               /* Remove any previous definition. */
               memset( (void *) &Vcp, 0, sizeof(Vcp_struct) );

               LE_send_msg( GL_INFO, "Processing File: %s\n", ext_name );

               /* Specifies the CS configuration file name and which
                  character is interpreted as comment. */
               CS_cfg_name ( ext_name );
               CS_control (CS_COMMENT | '#');

               /* Read the VCP attributes section.  We need to do this
                  to get the VCP number. */
               if( Read_VCP_attrs( &wx_mode, &where_defined, &vcp_flags ) < 0 )
               continue;

               /* Remove this VCP from RPG adaptation data. */
               LE_send_msg( GL_INFO, "Deleting VCP %d\n", Vcp.vcp_num );
               ORPGVCP_delete( Vcp.vcp_num );

               call_name = NULL;
            }

         }

      } /* End of while() loop. */
      else
         ORPGVCP_delete( vcp_num );

      /* Now make the changes permanent. */
      if( (ret = ORPGVCP_write( )) < 0 ){

         LE_send_msg( GL_ERROR, "ORPGVCP_write Failed: %d\n", ret );
         return( -1 );

      }

   }
   else if( (startup_action == STARTUP) || (startup_action == RESTART) ){

      /* On startup or restart, then the VCP configuration files are read
         and all defined VCPs are installed. */
      call_name = Vcp_dir_name;
      while( Get_next_file_name( call_name, Vcp_site_basename,
                                 ext_name, CFG_NAME_SIZE ) == 0 ){

         /* Do some initialization ...... */
         memset( (void *) &Vcp, 0, sizeof(Vcp_struct) );
         memset( (void *) &Allowable_prfs, 0, sizeof(Vcp_alwblprf_t) );
         memset( (void *) &Rdccon[0], 0, sizeof(Rdccon) );
         Allowable_prfs_n_ele = 0;

         LE_send_msg( GL_INFO, "Reading and processing standard VCP %s\n", ext_name );

         CS_cfg_name ( ext_name );
         CS_control (CS_COMMENT | '#');

         call_name = NULL;

         /* Read and process the VCP_attrs */
         if( Read_VCP_attrs( &wx_mode, &where_defined, &vcp_flags ) < 0 )
            continue;

         /* If VCP is already defined, do nothing.  This forces the use of "clear". */ 
  
         /* If this call returns non-negative number, the VCP is 
            already defined. */ 
         if( ORPGVCP_index( Vcp.vcp_num ) >= 0 ){

            LE_send_msg( GL_INFO, "Skipping VCP %d .... It is already defined.\n",
                         Vcp.vcp_num );
            continue;

         }

         /* Read and process the elevation cuts. */
         if( Read_elev_cut_attrs( (int) Vcp.n_ele ) < 0 )
            continue;

         /* Use ORPGVCP API to install VCP information. */
         if( Init_VCP_tbl( wx_mode, where_defined, vcp_flags ) < 0 )
            continue;

      } /* End of "while( Get_next_file_name( ... ))" loop. */

      /* Looking for non site-specific files is only necessary if
         Vcp_basename and Vcp_site_basename are not identical
         strings. */
      if( strcmp( Vcp_basename, Vcp_site_basename ) != 0 ){

         call_name = Vcp_dir_name;
         while( Get_next_file_name( call_name, Vcp_basename,
                                    ext_name, CFG_NAME_SIZE ) == 0 ){

            /* Do some initialization ...... */
            memset( (void *) &Vcp, 0, sizeof(Vcp_struct) );
            memset( (void *) &Allowable_prfs, 0, sizeof(Vcp_alwblprf_t) );
            memset( (void *) &Rdccon[0], 0, sizeof(Rdccon) );
            Allowable_prfs_n_ele = 0;

            LE_send_msg( GL_INFO, "Reading and processing site-specific VCP %s\n", ext_name );

            CS_cfg_name ( ext_name );
            CS_control (CS_COMMENT | '#');

            call_name = NULL;

            /* Read and process the VCP_attrs */
            if( Read_VCP_attrs( &wx_mode, &where_defined, &vcp_flags ) < 0 )
               continue;

            /* If VCP is already defined, do nothing.  This forces the use of "clear". */ 

            /* If this call returns non-negative number, the VCP is
               already defined. */
            if( ORPGVCP_index( Vcp.vcp_num ) >= 0 ){

               LE_send_msg( GL_INFO, "Skipping VCP %d .... It is already defined.\n",
                            Vcp.vcp_num );
               continue;

            }

            /* Read and process the elevation cuts. */
            if( Read_elev_cut_attrs( (int) Vcp.n_ele ) < 0 )
               continue;

            /* Use ORPGVCP API to install VCP information. */
            if( Init_VCP_tbl( wx_mode, where_defined, vcp_flags ) < 0 )
               continue;

         } /* End of "while( Get_next_file_name( ... ))" loop. */

      }

   }

   return(0) ;

/*END of MNTTSK_VCP_tables()*/
}

/*\////////////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function initializes the VCP table structure from 
//      information read from a CS file.  If all goes well, the
//      VCP information is written.
//
//   Input:
//      file_name - name of the CS file containing the VCP information.
//      where defined - specifies if this is an RPG or RPG/RDA defined
//                      VCP.
//      vcp_flags - supplemental VCP information.
//
//   Output: 
//
//   Return:
//      -1 on error, 0 otherwise.
//
////////////////////////////////////////////////////////////////////////\*/
static int Init_VCP_tbl( int wx_mode, int where_defined, short vcp_flags ){

   int ret, pos;
   int i, j, *delta_pri_p = NULL;
   int *unambiguous_range_p = NULL;
   float vcp_time, *prfvalue_p = NULL;
   Ele_attr *Elev = NULL;

   /* Define the VCP data through the VCP API. */
   if( (pos = ORPGVCP_add( (int) Vcp.vcp_num, (int) wx_mode,
                           where_defined )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGVCP_add( %d, %d ) Failed: %d\n",
                   Vcp.vcp_num, wx_mode, pos );
      return( -1 );

   }

   /* Set fields in the VCP attributes section. */
   if( (ORPGVCP_set_num_elevations( Vcp.vcp_num, Vcp.n_ele ) < 0 )
                                ||
       (ORPGVCP_set_pulse_width( Vcp.vcp_num, Vcp.pulse_width ) < 0 )
                                ||
       (ORPGVCP_set_vel_resolution( Vcp.vcp_num, Vcp.vel_resolution ) < 0 ) ){

      LE_send_msg( GL_ERROR, "Error setting # elevs/pulse width/vel resolution.\n" );
      LE_send_msg( GL_ERROR, "--->VCP: %d, # Elevs: %d, Pulse Width: %d, Vel Resol: %d\n",
                   Vcp.vcp_num, Vcp.n_ele, Vcp.pulse_width, Vcp.vel_resolution );
      return( -1 );

   }

   /* Set the fields for each elevation cut. */
   for( i = 0; i < Vcp.n_ele; i++ ){

      Elev = (Ele_attr *) Vcp.vcp_ele[ i ];

      if( (ORPGVCP_set_elevation_angle( Vcp.vcp_num, i, 
                                        ((float) Elev->ele_angle)/100.0 ) < -1.0 )
                                    ||
          (ORPGVCP_set_azimuth_rate( Vcp.vcp_num, i,
                                     (float) (Elev->azi_rate * AZI_RATE_SCALE) ) < 0.0 )
                                    ||
          (ORPGVCP_set_waveform( Vcp.vcp_num, i, Elev->wave_type ) < 0 ) 
                                    ||
          (ORPGVCP_set_phase_type( Vcp.vcp_num, i, Elev->phase ) < 0 ) ){
       
         LE_send_msg( GL_ERROR, "Error setting elev angle/azm rate/waveform\n" );
         LE_send_msg( GL_ERROR, "--->VCP: %d, Elev Angle: %f, Azm Rate: %f, Wave Type: %d, Phase: %d\n",
                      Vcp.vcp_num, ((float) Elev->ele_angle)/100.0, 
                      (float) (Elev->azi_rate * AZI_RATE_SCALE), 
                      Elev->wave_type, Elev->phase );
         return( -1 );

      }

      /* Set the super resolution word. */
      if( ORPGVCP_set_super_res( Vcp.vcp_num, i, (Elev->super_res & VCP_SUPER_RES_MASK) ) < 0 ){

         LE_send_msg( GL_ERROR, "Error setting super resolution.\n" );
         return(-1);

      }

      /* Set the Dual Pol word. */
      if( Elev->super_res & VCP_DUAL_POL_ENABLED ){

         if( ORPGVCP_set_dual_pol( Vcp.vcp_num, i, VCP_DUAL_POL_ENABLED ) < 0 ){

            LE_send_msg( GL_ERROR, "Error setting dual pol enabled.\n" );
            return(-1);

         }

      }

      /* Set the surveillance PRF number and pulse counts. */
      if( (Elev->wave_type == VCP_WAVEFORM_CS) 
                               ||
          (Elev->wave_type == VCP_WAVEFORM_BATCH)
                               ||
          (Elev->wave_type == VCP_WAVEFORM_STP) ){

         if( (ORPGVCP_set_prf_num( Vcp.vcp_num, i, ORPGVCP_SURVEILLANCE, 
                                   Elev->surv_prf_num ) < 0)
                                     ||
             (ORPGVCP_set_pulse_count( Vcp.vcp_num, i, ORPGVCP_SURVEILLANCE, 
                                       Elev->surv_pulse_cnt ) < 0) ){

            LE_send_msg( GL_ERROR, "Error setting surveillance prf/pulse count\n" );
            LE_send_msg( GL_ERROR, "--->VCP: %d, PRF #: %d, Pulse Count: %d\n",
                         Vcp.vcp_num, Elev->surv_prf_num, Elev->surv_pulse_cnt );
            return( -1 );

         }

      }

      /* Set the Doppler PRF number and pulse counts. */
      if( (Elev->wave_type == VCP_WAVEFORM_CD) 
                               ||
          (Elev->wave_type == VCP_WAVEFORM_BATCH)
                               ||
          (Elev->wave_type == VCP_WAVEFORM_CDBATCH) ){

         if( (ORPGVCP_set_edge_angle( Vcp.vcp_num, i, ORPGVCP_DOPPLER1,
                                      ((float) Elev->azi_ang_1)/10.0 ) < 0.0 )
                                    ||
             (ORPGVCP_set_prf_num( Vcp.vcp_num, i, ORPGVCP_DOPPLER1,
                                   Elev->dop_prf_num_1 ) < 0 )
                                    ||
             (ORPGVCP_set_pulse_count( Vcp.vcp_num, i, ORPGVCP_DOPPLER1,
                                       Elev->pulse_cnt_1 ) < 0 ) ){

            LE_send_msg( GL_ERROR, "Error setting Doppler Sector 1 prf/pulse count/sector angle\n" );
            LE_send_msg( GL_ERROR, "--->VCP:  PRF #: %d, Edge Angle: %f, Pulse Count: %d\n",
                         Vcp.vcp_num, Elev->dop_prf_num_1, Elev->azi_ang_1/10.0, Elev->pulse_cnt_1 );
            return( -1 );

         }
                                    
         if( (ORPGVCP_set_edge_angle( Vcp.vcp_num, i, ORPGVCP_DOPPLER2,
                                      ((float) Elev->azi_ang_2)/10.0 ) < 0.0 )
                                    ||
             (ORPGVCP_set_prf_num( Vcp.vcp_num, i, ORPGVCP_DOPPLER2, 
                                   Elev->dop_prf_num_2 ) < 0 )
                                    ||
             (ORPGVCP_set_pulse_count( Vcp.vcp_num, i, ORPGVCP_DOPPLER2, 
                                       Elev->pulse_cnt_2 ) < 0 ) ){

            LE_send_msg( GL_ERROR, "Error setting Doppler Sector 2 prf/pulse count/sector angle\n" );
            LE_send_msg( GL_ERROR, "--->VCP:  PRF #: %d, Edge Angle: %f, Pulse Count: %d\n",
                         Vcp.vcp_num, Elev->dop_prf_num_2, Elev->azi_ang_2/10.0, Elev->pulse_cnt_2 );
            return( -1 );

         }
                                    
         if( (ORPGVCP_set_edge_angle( Vcp.vcp_num, i, ORPGVCP_DOPPLER3, 
                                      ((float) Elev->azi_ang_3)/10.0 ) < 0.0 )
                                    ||
             (ORPGVCP_set_prf_num( Vcp.vcp_num, i, ORPGVCP_DOPPLER3, 
                                   Elev->dop_prf_num_3 ) < 0 )
                                    ||
             (ORPGVCP_set_pulse_count( Vcp.vcp_num, i, ORPGVCP_DOPPLER3, 
                                       Elev->pulse_cnt_3 ) < 0 ) ){

            LE_send_msg( GL_ERROR, "Error setting Doppler Sector 3 prf/pulse count/sector angle\n" );
            LE_send_msg( GL_ERROR, "--->VCP:  PRF #: %d, Edge Angle: %f, Pulse Count: %d\n",
                         Vcp.vcp_num, Elev->dop_prf_num_3, Elev->azi_ang_3/10.0, Elev->pulse_cnt_3 );
            return( -1 );

         }

      }

      /* Set the Signal-to_Noise thresholds. */
      if( (ORPGVCP_set_threshold( Vcp.vcp_num, i, ORPGVCP_REFLECTIVITY, 
                                  ( (float) Elev->surv_thr_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( Vcp.vcp_num, i, ORPGVCP_VELOCITY, 
                                  ( (float) Elev->vel_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( Vcp.vcp_num, i, ORPGVCP_SPECTRUM_WIDTH, 
                                  ( (float) Elev->spw_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD ) 
                                       ||
          (ORPGVCP_set_threshold( Vcp.vcp_num, i, ORPGVCP_DIFFERENTIAL_Z, 
                                  ( (float) Elev->zdr_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( Vcp.vcp_num, i, ORPGVCP_CORRELATION_COEF, 
                                  ( (float) Elev->corr_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( Vcp.vcp_num, i, ORPGVCP_DIFFERENTIAL_PHASE,
                                  ( (float) Elev->phase_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD ) ){

         LE_send_msg( GL_ERROR, "Error setting moment thresholds\n" );
         LE_send_msg( GL_ERROR, "--->VCP: %d, SNR Thresholds (Z/V/W): %5.1f/%5.1f/%5.1f dB\n",
                      Vcp.vcp_num, ((float) Elev->surv_thr_parm)/8.0, 
                      ((float) Elev->vel_thrsh_parm)/8.0, ((float) Elev->spw_thrsh_parm)/8.0 );
         LE_send_msg( GL_ERROR, "--->VCP: %d, SNR Thresholds (ZDR/CC/PHI): %5.1f/%5.1f/%5.1f dB\n",
                      Vcp.vcp_num, ((float) Elev->zdr_thrsh_parm)/8.0, 
                      ((float) Elev->corr_thrsh_parm)/8.0, ((float) Elev->phase_thrsh_parm)/8.0 );
         return( -1 );

      }

   }

   /* Set the allowable PRF information. */
   if( ((ret = ORPGVCP_set_allowable_prf_vcp_num( pos, Vcp.vcp_num )) < 0 )
                                  ||
       ((ret = ORPGVCP_set_allowable_prfs( Vcp.vcp_num, 
                                           Allowable_prfs.num_alwbl_prf )) < 0 ) ){

      LE_send_msg( GL_ERROR, "Error setting allowable prfs: %d\n", ret );
      LE_send_msg( GL_ERROR, "--->VCP: %d, Pos: %d, Number Allowable PRFs: %d\n", 
                   pos, Vcp.vcp_num, Allowable_prfs.num_alwbl_prf );
      return( -1 );

   }

   for( i = 0; i < Allowable_prfs.num_alwbl_prf; i++ ){

      if( (ret = ORPGVCP_set_allowable_prf( Vcp.vcp_num, i, Allowable_prfs.prf_num[i] )) < 0 ){

         LE_send_msg( GL_ERROR, "Error setting allowable prf: %d\n", ret );
         LE_send_msg( GL_ERROR, "--->VCP: %d, PRF Index: %d, PRF: %d\n", 
                      Vcp.vcp_num, i, Allowable_prfs.prf_num[i] );
         return( -1 );

      }


   } /* End of "For All Allowable PRFs" loop. */

   for( i = 0; i < Allowable_prfs_n_ele; i++ ){

      for( j = 1; j <= PRFMAX; j++ ){

         if( (ret = ORPGVCP_set_allowable_prf_pulse_count( Vcp.vcp_num, i, j, 
                                                           Allowable_prfs.pulse_cnt[i][j-1] )) < 0 ){
 
            LE_send_msg( GL_ERROR, "Error setting allowable prf pulse count: %d\n", ret );
            LE_send_msg( GL_ERROR, "--->VCP: %d, Elev: %d, PRF: %d, Pulse Cnt: %d\n", 
                         Vcp.vcp_num, i, j, Allowable_prfs.pulse_cnt[i][j] );
            return( -1 );

         }

      } /* End of "For All Allowable PRF Elevation Cuts" loop. */

      if( (ORPGVCP_set_allowable_prf_default( Vcp.vcp_num, i, 
                                              Allowable_prfs.pulse_cnt[i][NAPRFELV-1] ) < 0 )){

         LE_send_msg( GL_ERROR, "Error setting allowable prf default\n" );
         return( -1 );

      }

   } /* End of "For All Elevation Cuts" loop. */

   /* Compute the VCP time (duration). Add a 1 second of overhead for each 
      elevation transition. */
   vcp_time = (float) (Vcp.n_ele - 1);
   for( i = 0; i < Vcp.n_ele; i++ ){

      Elev = (Ele_attr *) Vcp.vcp_ele[ i ];
      vcp_time += 360.0/((Elev->azi_rate*AZI_RATE_SCALE));

   }
   
   /* Set the VCP time. */
   if( ORPGVCP_set_vcp_time( Vcp.vcp_num, (int) (vcp_time + 0.5) ) < 0 ){

      LE_send_msg( GL_ERROR, "Error setting VCP time\n" );
      return( -1 );

   }

   /* Set VCP flags. */
   if( ORPGVCP_set_vcp_flags( Vcp.vcp_num, vcp_flags ) < 0 ){

      LE_send_msg( GL_ERROR, "Error setting VCP Flags\n" );
      return( -1 );

   }

   /* Verify the RDA to RPG elevation mapping. */
   if( Verify_rdccon() < 0 ){

      LE_send_msg( GL_ERROR, "Rdccon Could Not Be Verified\n" );
      return( -1 );

   }

   /* Set the RDA to RPG elevation mapping. */
   for( i = 0; i < Vcp.n_ele; i++ ){

      if( ORPGVCP_set_rpg_elevation_num( Vcp.vcp_num, i, Rdccon[i] ) < 0 ){

         LE_send_msg( GL_ERROR, "Error setting VCP %d Rdccon[%d]: %d\n",
                      Vcp.vcp_num, i, Rdccon[i] );
         return( -1 );

      }

   }/* End of "For All Elevation Cuts" loop. */

   if( Verbose_mode ){

      char temp_buf[ VCP_CS_MAXLEN ], temp_buf1[ VCP_CS_MAXLEN];

      LE_send_msg( GL_INFO, "--->VCP %d Time: %d secs\n", 
                   Vcp.vcp_num, (int) (vcp_time + 0.5) );

      LE_send_msg( GL_INFO, "--->Rdccon (RDA -> RPG Elevation Cut Mapping)\n" );

      memset( temp_buf, 0, sizeof(temp_buf) );
      memset( temp_buf1, 0, sizeof(temp_buf1) );

      sprintf( temp_buf, "------>" );
      for( i = 0; i < Vcp.n_ele; i++ ){

         sprintf( temp_buf1, " %d", Rdccon[ i ] );
         strcat( temp_buf, temp_buf1 );

      } /* End of "For All Elevation Cuts" loop. */

      LE_send_msg( GL_INFO, "%s\n", temp_buf );

   }

   /* Write the VCP to adaptation data through the VCP API. */
   if( (ret = ORPGVCP_write( )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGVCP_write( ) Failed: %d\n",
                   ret );
      return( -1 );

   }

   /* Set the unambiguous range table. */
   unambiguous_range_p = ORPGVCP_unambiguous_range_ptr( );
   if( unambiguous_range_p == NULL ){

      LE_send_msg( GL_ERROR, "ORPGVCP_unambiguous_range_ptr() Failed\n" );
      return( -1 );

   }

   memcpy( unambiguous_range_p, unambiguous_range, sizeof(unambiguous_range) );

   /* Set the PRF value table. */
   prfvalue_p = ORPGVCP_prf_ptr();

   if( prfvalue_p == NULL ){

      LE_send_msg( GL_ERROR, "ORPGVCP_prf_ptr() Failed\n" );
      return( -1 );

   }

   memcpy( prfvalue_p, prfvalue, sizeof(prfvalue) );

   /* Set the PRF value table. */
   delta_pri_p = ORPGVCP_delta_pri_ptr();

   if( delta_pri_p == NULL ){

      LE_send_msg( GL_ERROR, "ORPGVCP_delta_pri_ptr() Failed\n" );
      return( -1 );

   }

   *delta_pri_p = delta_pri;

   /* Write the VCP to adaptation data through the VCP API. */
   if( (ret = ORPGVCP_write( )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGVCP_write( ) Failed: %d\n",
                   ret );
      return( -1 );

   }

   return( 0 );
}

/*\////////////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function initializes the VCP table structure ().
//
//   Input: 
//      wx_mode - pointer to int to receive weather mode.
//      where_defined - where the VCP is defined (RPG, BOTH, EXP) 
//      vcp_flags - VCP flags (included support/non support for SAILS)
//
//   Output: 
//      wx_mode - stores weather mode.
//      where_defined - where the VCP is defined (RPG, BOTH, EXP) 
//      vcp_flags - VCP flags (included support/non support for SAILS)
//
//   Return:
//      -1 on error, 0 otherwise.
//
////////////////////////////////////////////////////////////////////////\*/
static int Read_VCP_attrs( int *wx_mode, int *where_defined, short *vcp_flags ){

   /* Definitions for the VCP_attr section. */
   short msg_size;
   short vcp_num;
   short num_elevs;
   short clut_map_grp = 1;
   short pat_type = ORPGVCP_PATTERN_TYPE_CONSTANT;
   unsigned char vel_reso = ORPGVCP_VEL_RESOLUTION_HIGH;
   unsigned char pulse_width = ORPGVCP_SHORT_PULSE;

   float vel_reso_ms;
   int   local_where_defined = ORPGVCP_RPG_DEFINED_VCP;
   int   local_allow_sails = 0;
   int   flag_value = 0;
   char  buf1[ VCP_CS_MAXLEN ], buf2[ VCP_CS_MAXLEN ], buf3[ VCP_CS_MAXLEN ];
   char  buf4[ VCP_CS_MAXLEN ];

   /* Clear the Vcp data, the vcp_flags value, and the read buffers. */
   memset( (void *) &Vcp, 0, sizeof( Vcp_struct ) );
   memset( (void *) buf1, 0, sizeof( buf1 ) );
   memset( (void *) buf2, 0, sizeof( buf2 ) );
   memset( (void *) buf3, 0, sizeof( buf3 ) );
   memset( (void *) buf4, 0, sizeof( buf4 ) );
   *vcp_flags = 0;

   /* Verify the VCP attributes section exist. */
   if( (CS_entry (VCP_CS_ATTR_KEY, 0, 0, NULL) < 0 ) || (CS_level( CS_DOWN_LEVEL ) < 0 )){

      LE_send_msg( GL_ERROR, "Could Not Find %s Key\n", VCP_CS_ATTR_KEY );
      return (-1);

   }

   /* Parse the "pattern_num", "wx_mode" and "num_elev_cuts" keys. */
   if( (CS_entry( VCP_CS_PAT_NUM_KEY, VCP_CS_PAT_NUM_TOK, 0, (void *) &vcp_num ) <= 0)
                                          ||
       (CS_entry( VCP_CS_WX_MODE_KEY, VCP_CS_WX_MODE_TOK, VCP_CS_MAXLEN, (void *) buf1  ) <= 0)
                                          ||
       (CS_entry( VCP_CS_NUM_ELEVS_KEY, VCP_CS_NUM_ELEVS_TOK, 0, (void *) &num_elevs ) <= 0) ){

      LE_send_msg( GL_ERROR, "Error parsing pattern_num: %d, wx_mode: %s or num_elev_cuts: %d\n",
                   vcp_num, buf1, num_elevs );
      return( -1 );

   }

   /* The size of the message must be VCP_ATTR_SIZE + num_elevs*ELE_ATTR_SIZE. */
   msg_size = (VCP_ATTR_SIZE + num_elevs*ELE_ATTR_SIZE);

   /* Do some validation. */
   /* The number of elevations must be in the valid range  */
   if( (num_elevs <= 0) || (num_elevs > VCP_MAXN_CUTS) ){

      LE_send_msg( GL_ERROR, "Number of Elev Cuts Error:  0 < Cuts (%d) <= %d\n",
                   num_elevs, VCP_MAXN_CUTS );
      return( -1 );

   }

   /* The following validation will set values to defaults if the entered values are not valid. */
   /* If the weather mode is not valid, assume Mode A. */
   if( strstr( buf1, "A" ) )
      *wx_mode = PRECIPITATION_MODE;

   else if( strstr( buf1, "B" ) )
      *wx_mode = CLEAR_AIR_MODE;
    
   else{

      *wx_mode = PRECIPITATION_MODE;
      LE_send_msg( GL_ERROR, "Invalid Wx Mode.  Setting to Precipitation Mode\n" );

   }

   /* Process all optional keys. */
   CS_control( CS_KEY_OPTIONAL );

   /* Parse the velocity resolution. */
   if( CS_entry( VCP_CS_DOP_RESO_KEY, VCP_CS_DOP_RESO_TOK, 0, (void *) &vel_reso_ms ) > 0 ){

      /* If the velocity resolution is not valid, set to 0.5 m/s */
      if( vel_reso_ms == 0.5 )
         vel_reso = ORPGVCP_VEL_RESOLUTION_HIGH;

      else if( vel_reso_ms == 1.0 )
         vel_reso = ORPGVCP_VEL_RESOLUTION_LOW;

      else{ 

         LE_send_msg( GL_ERROR, "Invalid Velocity Resolution:  Setting to 0.5 m/s\n" );
         vel_reso = ORPGVCP_VEL_RESOLUTION_HIGH;

      } 

   }

   /* Parse the pulse width. */
   if( CS_entry( VCP_CS_PULSE_WID_KEY, VCP_CS_PULSE_WID_TOK, VCP_CS_MAXLEN, (void *) buf2 ) > 0 ){

      /* If the pulse width is not valid, assume short pulse. */
      if( strstr( buf2, "SHORT" ) )
         pulse_width = ORPGVCP_SHORT_PULSE;

      else if( strstr( buf2, "LONG" ) )
         pulse_width = ORPGVCP_LONG_PULSE;
    
      else{

         pulse_width = ORPGVCP_SHORT_PULSE;
         LE_send_msg( GL_ERROR, "Invalid Pulse Width.  Setting to Short Pulse\n" );

      }

   }

   /* Parse the clutter map group number. */
   if( CS_entry ( VCP_CS_CLUT_GRP_KEY, VCP_CS_CLUT_GRP_TOK, 0, (void *) &clut_map_grp ) > 0){

      if( (clut_map_grp < 0) || (clut_map_grp > 5) ){

         LE_send_msg( GL_ERROR, "Invalid Clutter Map Group:  Setting to 1\n" );
         clut_map_grp = 1;

      }

   }

   /* Parse the where-defined value */
   if( CS_entry( VCP_CS_WHERE_DEF_KEY, VCP_CS_WHERE_DEF_TOK, VCP_CS_MAXLEN, (void *) buf3  ) > 0 ){

      /* If the where-defined value is not valid, assume RPG defined VCP only. */
      if( strstr( buf3, "RPG" ) )
         local_where_defined = ORPGVCP_RPG_DEFINED_VCP;

      else if( strstr( buf3, "BOTH" ) )
         local_where_defined = ORPGVCP_RPG_DEFINED_VCP | ORPGVCP_RDA_DEFINED_VCP;
    
      else if( strstr( buf3, "EXP" ) )
         local_where_defined = ORPGVCP_RPG_DEFINED_VCP | ORPGVCP_EXPERIMENTAL_VCP;

      else
         local_where_defined = ORPGVCP_RPG_DEFINED_VCP;

   }

   *where_defined = local_where_defined;

   /* Parse the Site Specific VCP flag. */
   if( CS_entry ( VCP_CS_SITE_VCP_KEY, VCP_CS_SITE_VCP_TOK, VCP_CS_MAXLEN, (void *) buf4 ) > 0){

      /* If the where-defined value is not valid, assume RPG defined VCP only. */
      if( strstr( buf4, "RPG" ) ){

         *where_defined |= ORPGVCP_SITE_SPECIFIC_RPG_VCP;
         LE_send_msg( GL_INFO, "RPG Site-Specific VCP\n" );

      }
      else if( strstr( buf4, "RDA" ) ){

         *where_defined |= ORPGVCP_SITE_SPECIFIC_RDA_VCP;
         LE_send_msg( GL_INFO, "RDA Site-Specific VCP\n" );

      }
      else if( strstr( buf4, "BOTH" ) ){

         *where_defined |= (ORPGVCP_SITE_SPECIFIC_RDA_VCP | ORPGVCP_SITE_SPECIFIC_RPG_VCP);
         LE_send_msg( GL_INFO, "BOTH (RDA/RPG) Site-Specific VCP\n" );
    
      }
      else{

         *where_defined |= ORPGVCP_SITE_SPECIFIC_RPG_VCP;
         LE_send_msg( GL_INFO, "Default (RPG) Site-Specific VCP\n" );

      }

   }

   /* Parse the allow SAILS flag. */
   if( CS_entry ( VCP_CS_ALLOW_SAILS_KEY, VCP_CS_ALLOW_SAILS_TOK, 0, (void *) &local_allow_sails ) > 0){

      if( (local_allow_sails < 0) || (local_allow_sails > 3) ){

         LE_send_msg( GL_ERROR, "Invalid Allow Sails Flag:  Setting to 0\n" );
         local_allow_sails = 0;

      }

   }

   flag_value = 0;
   if( local_allow_sails ){

      flag_value = local_allow_sails << VCP_MAX_SAILS_CUTS_SHIFT;
      *vcp_flags |= (VCP_FLAGS_ALLOW_SAILS | flag_value);

   }
   else
      *vcp_flags &= ~VCP_FLAGS_ALLOW_SAILS;

   /* Assign fields in the VCP attributes section. */
   Vcp.msg_size = msg_size;
   Vcp.type = pat_type;
   Vcp.vcp_num = (short) vcp_num;
   Allowable_prfs.vcp_num = (short) vcp_num;
   Vcp.n_ele = num_elevs;
   Vcp.clutter_map_num = clut_map_grp;
   Vcp.vel_resolution = vel_reso;
   Vcp.pulse_width = pulse_width;

   /* Write out attributes. */
   if( Verbose_mode ){

      LE_send_msg( GL_INFO, "VCP %d Attributes (Size %d shorts)\n", 
                   vcp_num, msg_size );
      LE_send_msg( GL_INFO, "---># Elevation Cuts: %d\n", num_elevs );

      if( vel_reso == ORPGVCP_VEL_RESOLUTION_HIGH )
         LE_send_msg( GL_INFO, "--->Velocity Resolution: 0.5 m/s\n" );
      else
         LE_send_msg( GL_INFO, "--->Velocity Resolution: 1.0 m/s\n" );

      if( pulse_width == ORPGVCP_SHORT_PULSE )
         LE_send_msg( GL_INFO, "--->Pulse Width: SHORT\n" );
      else 
         LE_send_msg( GL_INFO, "--->Pulse Width: LONG\n" );

      if( local_allow_sails )
         LE_send_msg( GL_INFO, "--->Allow SAILS: Yes (%d)\n", *vcp_flags  );
     
      else 
         LE_send_msg( GL_INFO, "--->Allow SAILS: No\n" );

   }

   CS_level (CS_UP_LEVEL);

   return( 0 );
    
/* End of Read_VCP_attrs() */
}

/*\/////////////////////////////////////////////////////////////////////
//
//   Description:
//      Parser for the elevation cut data.
//
//   Input:
//      num_elevs - number of elevations to parse.
//
//   Returns:
//      -1 on error, 0 on success. 
//
/////////////////////////////////////////////////////////////////////\*/
static int Read_elev_cut_attrs( int num_elevs ){

   Ele_attr *Elev = NULL;

   char *elev_key;
   int ele_cut;
   int scan_rate_set;
   int ret;
   int cnt;

   unsigned char waveform_type;
   unsigned char phase;

   short scan_period_s;
   unsigned short super_res;
   unsigned short dual_pol;
   short prf;
   short pulses;
   short rpg_elev_num;

   float scan_rate_deg;
   float Z_snr_dB, V_snr_dB, W_snr_dB, DZ_snr_dB, P_snr_dB, C_snr_dB;
   float elev_ang_deg;

   char buf[ VCP_CS_MAXLEN ];
   char temp_buf[ VCP_CS_MAXLEN ], temp_buf1[ VCP_CS_MAXLEN];

   /* Verify the elevation cut section exists. */
   if( (CS_entry (VCP_CS_ELEV_ATTR_KEY, 0, 0, NULL) < 0 ) || (CS_level( CS_DOWN_LEVEL ) < 0 )){

      LE_send_msg( GL_ERROR, "Could Not Find %s Key\n", VCP_CS_ELEV_ATTR_KEY );
      return (-1);
   
   }

   /* The number of allowable PRFs and the allowable PRFs are optional.  
      By default, it is assumed the number of allowable PRFs is 5 and 
      they are PRF numbers 4, 5, 6, 7, and 8. */
   CS_control( CS_KEY_OPTIONAL );

   cnt = 0;
   while( CS_entry( VCP_CS_ALWB_PRF_KEY, ((cnt + 1) | CS_SHORT), 0,
                    (void *) &Allowable_prfs.prf_num[ cnt ] ) > 0 )
      cnt++;        
             
   if( cnt > 0 )
      Allowable_prfs.num_alwbl_prf = cnt;
            
   else{

      /* Set defaults for number of allowable PRFs and allowable PRFs. */
      Allowable_prfs.num_alwbl_prf = DEFAULT_NUM_PRFS;
      for( cnt = 0; cnt < Allowable_prfs.num_alwbl_prf; cnt++ )
         Allowable_prfs.prf_num[ cnt ] = INITIAL_PRF + cnt;

   }

   if( Verbose_mode ){

      LE_send_msg( GL_INFO, "--->Number of Allowable PRFs: %d\n", Allowable_prfs.num_alwbl_prf );
      sprintf( temp_buf, "------>Allowable PRFS:" );
      for( cnt = 0; cnt < Allowable_prfs.num_alwbl_prf; cnt++ ){

         sprintf( temp_buf1, " %d", Allowable_prfs.prf_num[ cnt ] ); 
         strcat( temp_buf, temp_buf1 );

      }

      LE_send_msg( GL_INFO, "%s\n", temp_buf );

   }

   /* Do For All elevation cuts in the VCP. */
   for( ele_cut = 1; ele_cut <= num_elevs; ele_cut++ ){

      elev_key = Set_elev_key( ele_cut );
      if( (ret = CS_entry( elev_key, 0, 0, NULL )) < 0 ){

         LE_send_msg( GL_ERROR, "Could Not Find Key %s (%d)\n", elev_key, ret );
         return( -1 );  

      }

      CS_level( CS_DOWN_LEVEL );
      CS_control( CS_KEY_REQUIRED );

      /* Clear out the read buffer. */
      memset( buf, 0, sizeof(buf) );

      /* Parse the elevation angle and waveform type fields. */
      if( (CS_entry( VCP_CS_ELEV_ANG_KEY, VCP_CS_ELEV_ANG_TOK, 0, (void *) &elev_ang_deg ) <= 0) 
                                              ||
          (CS_entry( VCP_CS_WF_KEY, VCP_CS_WF_TOK, VCP_CS_MAXLEN, (void *) buf ) <= 0) ){

         LE_send_msg( GL_ERROR, "Required Elevation Fields %s or %s Not Found For Elev %s\n",
                      VCP_CS_ELEV_ANG_KEY, VCP_CS_WF_KEY, elev_key );
         return( -1 );

      }

      /* Set the data in the VCP structure. */
      Elev = (Ele_attr *) Vcp.vcp_ele[ ele_cut-1 ];

      /* But it must be validated first. */
      /* The elevation angle must be between -1.0 and 45.0 degrees. */
      if(elev_ang_deg < -1.0){

         LE_send_msg( GL_ERROR, "Elevation Angle Error:  %f < -1.0\n", elev_ang_deg );
         return( -1 );

      }
 
      Elev->ele_angle = (short) ((elev_ang_deg * 100.0) + 0.5);

      /* Validate the waveform type. */
      waveform_type = VCP_WAVEFORM_UNKNOWN;
      if( strstr( buf, "CS" ) )
         waveform_type = VCP_WAVEFORM_CS;

      else if( strstr( buf, "CDBATCH" ) )
         waveform_type = VCP_WAVEFORM_CDBATCH;

      else if( strstr( buf, "BATCH" ) )
         waveform_type = VCP_WAVEFORM_BATCH;

      else if( strstr( buf, "CD" ) )
         waveform_type = VCP_WAVEFORM_CD;

      else if( strstr( buf, "STP" ) )
         waveform_type = VCP_WAVEFORM_STP;

      if( waveform_type == VCP_WAVEFORM_UNKNOWN ){

         LE_send_msg( GL_ERROR, "Invalid Waveform Type (%s) For Elev Angle: %f\n",
                      buf, elev_ang_deg );
         return( -1 );

      }

      Elev->wave_type = waveform_type;
          
      /* Parse the SNR field for all 3 moments. */
      CS_control( CS_KEY_OPTIONAL );
      if( CS_entry( VCP_CS_SNR_KEY, VCP_CS_SNRZ_TOK, 0, (void *) &Z_snr_dB ) <= 0 ){

         if( waveform_type == VCP_WAVEFORM_CS )
            Z_snr_dB = 2.0;

         else
            Z_snr_dB = 3.5;

      }

      if( CS_entry( VCP_CS_SNR_KEY, VCP_CS_SNRV_TOK, 0, (void *) &V_snr_dB ) <= 0 )
         V_snr_dB = 3.5;

      if( CS_entry( VCP_CS_SNR_KEY, VCP_CS_SNRW_TOK, 0, (void *) &W_snr_dB ) <= 0 )
         W_snr_dB = 3.5;

      if( CS_entry( VCP_CS_SNR_KEY, VCP_CS_SNRDZ_TOK, 0, (void *) &DZ_snr_dB ) <= 0 )
         DZ_snr_dB = Z_snr_dB;

      if( CS_entry( VCP_CS_SNR_KEY, VCP_CS_SNRP_TOK, 0, (void *) &P_snr_dB ) <= 0 )
         P_snr_dB = Z_snr_dB;

      if( CS_entry( VCP_CS_SNR_KEY, VCP_CS_SNRC_TOK, 0, (void *) &C_snr_dB ) <= 0 )
         C_snr_dB = Z_snr_dB;

      /* Validate the SNR data.  Any data outside of the limits, set to the limits. */
      if( Z_snr_dB < -12.0 )
         Z_snr_dB = -12.0;

      else if( Z_snr_dB > 20.0 ) 
          Z_snr_dB = 20.0;

      if( V_snr_dB < -12.0 )
         V_snr_dB = -12.0;

      else if( V_snr_dB > 20.0 ) 
          V_snr_dB = 20.0;

      if( W_snr_dB < -12.0 )
         W_snr_dB = -12.0;

      else if( W_snr_dB > 20.0 ) 
          W_snr_dB = 20.0;

      if( DZ_snr_dB < -12.0 )
         DZ_snr_dB = -12.0;

      else if( DZ_snr_dB > 20.0 ) 
          DZ_snr_dB = 20.0;

      if( P_snr_dB < -12.0 )
         P_snr_dB = -12.0;

      else if( P_snr_dB > 20.0 ) 
          P_snr_dB = 20.0;

      if( C_snr_dB < -12.0 )
         C_snr_dB = -12.0;

      else if( C_snr_dB > 20.0 ) 
          C_snr_dB = 20.0;

      Elev->surv_thr_parm = (short) (Z_snr_dB * 8.0);
      Elev->vel_thrsh_parm = (short) (V_snr_dB * 8.0);
      Elev->spw_thrsh_parm = (short) (W_snr_dB * 8.0);
      Elev->zdr_thrsh_parm = (short) (DZ_snr_dB * 8.0);
      Elev->phase_thrsh_parm = (short) (P_snr_dB * 8.0);
      Elev->corr_thrsh_parm = (short) (C_snr_dB * 8.0);

      CS_control( CS_KEY_OPTIONAL );
      scan_rate_set = 0;

      if( CS_entry( VCP_CS_SCAN_RATE_KEY, VCP_CS_SCAN_RATE_TOK, 0, (void *) &scan_rate_deg ) > 0 )
         scan_rate_set = 1;                                 

      else if( CS_entry( VCP_CS_SCAN_PERIOD_KEY, VCP_CS_SCAN_PERIOD_TOK, 0, (void *) &scan_period_s ) > 0 ){ 
      
         scan_rate_deg = 360.0 / (float) scan_period_s;
         scan_rate_set = 1;                                 

      }

      if( scan_rate_set )
         Elev->azi_rate = scan_rate_deg/AZI_RATE_SCALE + 0.5;

      else{

         LE_send_msg( GL_ERROR, "Scan Rate (Period or Degrees) Not Set For Elev Angle %f\n",
                      elev_ang_deg );
         return( -1 );

      }

      /* Check to see if the RPG elevation number is set. */
      if( CS_entry( VCP_CS_RPG_ELEV_NUM_KEY, VCP_CS_RPG_ELEV_NUM_TOK, 0, (void *) &rpg_elev_num ) > 0 )
         Rdccon[ ele_cut - 1 ] = rpg_elev_num;

      /* Check to see if the phase field is set. */
      phase = VCP_PHASE_CONSTANT;
      if( CS_entry( VCP_CS_PHASE_KEY, VCP_CS_PHASE_TOK, VCP_CS_MAXLEN, (void *) &buf ) > 0 ){

         if( strstr( buf, "SZ2" ) )
            phase = VCP_PHASE_SZ2;
      }

      Elev->phase = phase;

      /* Check if super resolution bits are set. */
      Elev->super_res = super_res = 0;
      if( CS_entry( VCP_CS_RPG_SUPER_RES_KEY, VCP_CS_RPG_SUPER_RES_TOK, 0, (void *) &super_res ) > 0 ){

         if( super_res & VCP_HALFDEG_RAD )
            Elev->super_res |= VCP_HALFDEG_RAD;

         if( super_res & VCP_QRTKM_SURV )
            Elev->super_res |= VCP_QRTKM_SURV;

         if( super_res & VCP_300KM_DOP )
            Elev->super_res |= VCP_300KM_DOP;

      }

      /* Check if Dual Pol bit is set. */
      dual_pol = 0;
      if( (CS_entry( VCP_CS_RPG_DUAL_POL_KEY, VCP_CS_RPG_DUAL_POL_TOK, 0, (void *) &dual_pol ) > 0)
                                         &&
                                     (dual_pol) )
         Elev->super_res |= VCP_DUAL_POL_ENABLED;

      CS_control( CS_KEY_REQUIRED );

      /* Process the various waveform types. */

      /* For CS and BATCH, need the surveillance PRF number and number of pulses. */
      if( (waveform_type == VCP_WAVEFORM_CS) 
                         || 
          (waveform_type == VCP_WAVEFORM_BATCH) 
                         || 
          (waveform_type == VCP_WAVEFORM_STP) ){

         if( (CS_entry( VCP_CS_SURV_PRF_KEY, VCP_CS_SURV_PRF_TOK, 0, (void *) &prf ) <= 0) 
                                                 ||
             (CS_entry( VCP_CS_SURV_PULSES_KEY, VCP_CS_SURV_PULSES_TOK, 0, (void *) &pulses ) <= 0) ){

            LE_send_msg( GL_ERROR, "Required Elevation Fields %s or %s Not Found For Elevation %s\n",
                         VCP_CS_SURV_PRF_KEY, VCP_CS_SURV_PULSES_KEY, elev_key );
            return( -1 );

         }

         Elev->surv_prf_num = prf;
         Elev->surv_pulse_cnt = pulses;

      }

      /* For CD and BATCH and CDBATCH, need the Doppler PRF number for each PRF sector
         and the number of pulses within the sector. */
      if( (waveform_type == VCP_WAVEFORM_CD) 
                         || 
          (waveform_type == VCP_WAVEFORM_BATCH) 
                         ||
          (waveform_type == VCP_WAVEFORM_CDBATCH) ){

         int sector_num, min = MAX_DOP_PRI;
         float sector_ang;
         unsigned char *sector_key = NULL;

         /* Process the allowable PRF pulse counts. */
         if( Read_alwb_prf_pulse_counts( Allowable_prfs_n_ele ) < 0 ){
               
            LE_send_msg( GL_ERROR, "Read_alwb_prf_pulse_cnts Failed For Elev Angle %f\n",
                         elev_ang_deg );
            return(-1);
            
         }


         /* For Each PRF Sector ...... */
         for( sector_num = 1; sector_num <= 3; sector_num++ ){

            /* Get the sector key and check if the sector is defined.  If not, then we
               assume the default PRF is set. */
            sector_key = (unsigned char *) Set_sector_key( sector_num );

            if( (ret = CS_entry( (char *) sector_key, 0, 0, NULL )) >= 0 ){

               CS_level( CS_DOWN_LEVEL );

               if( (CS_entry( VCP_CS_SECT_ANG_KEY, VCP_CS_SECT_ANG_TOK, 0, (void *) &sector_ang ) <= 0) 
                                                 ||
                   (CS_entry( VCP_CS_DOP_PRF_KEY, VCP_CS_DOP_PRF_TOK, 0, (void *) &prf ) <= 0) ){ 

                  LE_send_msg( GL_ERROR, "Required PRF Sector Info For %s Incorrect For Elev Angle %f\n",
                               sector_key, elev_ang_deg );
                  return( -1 );

               }

               /* Set the PRF sector information in the VCP struct. */
               switch( sector_num ){

                  case 1:
                  {
                     Elev->azi_ang_1 = sector_ang*10;
                     Elev->dop_prf_num_1 = prf;
                     Elev->pulse_cnt_1 = Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele ][ prf-1 ];

                     break;

                  } /* End of "case 1:" */

                  case 2:
                  {
                     Elev->azi_ang_2 = sector_ang*10;
                     Elev->dop_prf_num_2 = prf;
                     Elev->pulse_cnt_2 = Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele ][ prf-1 ];

                     break;

                  } /* End of "case 2" */
   
                  case 3:
                  {
                     Elev->azi_ang_3 = sector_ang*10;
                     Elev->dop_prf_num_3 = prf;
                     Elev->pulse_cnt_3 = Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele ][ prf-1 ];

                     break;

                  } /* End of "case 3" */
   
               } /* End of "switch" */

               CS_level( CS_UP_LEVEL );

               if( prf < min )
                  min = prf;

               Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele ][ NAPRFELV-1 ] = min;

            } /* End of CS_entry( sector_key ..... ) */
            else{

               short default_prf;

               /* If no PRF sector information is provided, then a default PRF must be 
                  specified. */
               if( CS_entry( VCP_CS_DOP_PRF_KEY, VCP_CS_DOP_PRF_TOK, 0, 
                             (void *) &default_prf ) <= 0 ){

                  LE_send_msg( GL_ERROR, "Error setting Default PRF For Elev Angle %f\n",
                               elev_ang_deg );
                  return( -1 );

               }

               Elev->dop_prf_num_1 = Elev->dop_prf_num_2 = Elev->dop_prf_num_3 = default_prf;
               Elev->azi_ang_1 = 300;
               Elev->azi_ang_2 = 2100;
               Elev->azi_ang_3 = 3350;

               Elev->pulse_cnt_1 = Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele ][ default_prf-1 ];
               Elev->pulse_cnt_2 = Elev->pulse_cnt_3 = Elev->pulse_cnt_1;
               Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele ][ NAPRFELV-1 ] = default_prf;
         
               /* Break out of "For Each PRF Sector" loop. */
               break;

            }

         } /* End of "For Each Sector" loop. */

         /* Increment the number of elevations in the Allowable_prfs table. */
         Allowable_prfs_n_ele++;

      } /* End of "waveform_type" processing. */

      CS_level( CS_UP_LEVEL );

      /* Write out information about this elevation cut. */
      if( Verbose_mode ){

         int sector;

         LE_send_msg( GL_INFO, "--->Elevation Angle: %6.3f\n", elev_ang_deg );
         if( waveform_type == VCP_WAVEFORM_CS )
            LE_send_msg( GL_INFO, "------>Waveform Type: CS\n" );

         else if( waveform_type == VCP_WAVEFORM_CD )
            LE_send_msg( GL_INFO, "------>Waveform Type: CD\n" );

         else if( waveform_type == VCP_WAVEFORM_CDBATCH )
            LE_send_msg( GL_INFO, "------>Waveform Type: CDBATCH\n" );

         else if( waveform_type == VCP_WAVEFORM_BATCH )
            LE_send_msg( GL_INFO, "------>Waveform Type: BATCH\n" );

         else if( waveform_type == VCP_WAVEFORM_STP )
            LE_send_msg( GL_INFO, "------>Waveform Type: STP\n" );

         LE_send_msg( GL_INFO, "------>Scan Rate: %6.3f deg/s\n", scan_rate_deg );

         if( super_res != 0 ){

            LE_send_msg( GL_INFO, "------>Super Res Information\n" );

            if( super_res == VCP_HALFDEG_RAD )
               LE_send_msg( 0, "--------->1/2 Degree Radials\n" );

            if( super_res == VCP_QRTKM_SURV )
               LE_send_msg( 0, "--------->1/4 km Reflectivity\n" );

            if( super_res == VCP_300KM_DOP )
               LE_send_msg( 0, "--------->300 km Doppler\n" );

            if( super_res == (VCP_HALFDEG_RAD | VCP_QRTKM_SURV) )
               LE_send_msg( 0, "--------->1/2 Degree Radials, 1/4 Km Reflecitivity\n" );

            if( super_res == (VCP_HALFDEG_RAD | VCP_QRTKM_SURV | VCP_300KM_DOP) )
               LE_send_msg( 0, "--------->1/2 Degree Radials, 1/4 Km Reflecitivity, 300 Km Doppler\n" );

            if( super_res == (VCP_HALFDEG_RAD | VCP_300KM_DOP) )
               LE_send_msg( 0, "--------->1/2 Degree Radials, 300 Km Doppler\n" );

            if( super_res == (VCP_QRTKM_SURV | VCP_300KM_DOP) )
               LE_send_msg( 0, "--------->1/4 Km Reflecitivity, 300 Km Doppler\n" );

         }

         if( dual_pol != 0 )
            LE_send_msg( GL_INFO, "------>Dual Pol Enabled\n" );

         if( waveform_type != VCP_WAVEFORM_CS ){

            sprintf( temp_buf, "------>Pulse Counts:" );
            for( cnt = 0; cnt < Allowable_prfs.num_alwbl_prf; cnt++ ){

               int index;

               index = Allowable_prfs.prf_num[ cnt ];

               sprintf( temp_buf1, " %d", Allowable_prfs.pulse_cnt[ Allowable_prfs_n_ele-1 ][ index ] ); 
               strcat( temp_buf, temp_buf1 );

            }

            LE_send_msg( GL_INFO, "%s\n", temp_buf );

            for( sector = 1; sector <= 3; sector++ ){

               LE_send_msg( GL_INFO, "------>PRF Sector %d\n", sector );

               if( sector == 1 ){

                  LE_send_msg( GL_INFO, "--------->Edge Angle: %6.3f\n", Elev->azi_ang_1/10.0 );
                  LE_send_msg( GL_INFO, "--------->PRF: %d\n", Elev->dop_prf_num_1 );
                  LE_send_msg( GL_INFO, "--------->Pulse Count: %d\n", Elev->pulse_cnt_1 );

               }
               else if( sector == 2 ){
   
                  LE_send_msg( GL_INFO, "--------->Edge Angle: %6.3f\n", Elev->azi_ang_2/10.0 );
                  LE_send_msg( GL_INFO, "--------->PRF: %d\n", Elev->dop_prf_num_2 );
                  LE_send_msg( GL_INFO, "--------->Pulse Counts: %d\n", Elev->pulse_cnt_2 );

               }
               else if( sector == 3 ){

                  LE_send_msg( GL_INFO, "--------->Edge Angle: %6.3f\n", Elev->azi_ang_3/10.0 );
                  LE_send_msg( GL_INFO, "--------->PRF: %d\n", Elev->dop_prf_num_3 );
                  LE_send_msg( GL_INFO, "--------->Pulse Counts: %d\n", Elev->pulse_cnt_3 );

               }

            } /* End of "For Each PRF Sector" loop. */

         }

      } /* End of "Verbose_mode" */

   } /* End of "For All Elevations" loop. */

   return( 0 );

} /* End of Read_elev_cut_attrs() */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Parser for allowable PRF information.
//
//   Inputs:
//      ele_num - elevation number for which the allowable PRF
//                information applies.
//
//   Returns:
//      -1 on error, 0 otherwise.
//
///////////////////////////////////////////////////////////////////\*/
static int Read_alwb_prf_pulse_counts( int ele_num ){

   int cnt = 0;
   short pulse_cnt, prf;

   if( Allowable_prfs.num_alwbl_prf > 0) { 

      cnt = 0;
      while( (cnt < Allowable_prfs.num_alwbl_prf) 
                             &&
             (CS_entry( VCP_CS_DOP_PULSES_KEY, ((cnt + 1) | CS_SHORT), 0,
                       (void *) &pulse_cnt ) > 0) ){

         prf = Allowable_prfs.prf_num[ cnt ];
         Allowable_prfs.pulse_cnt[ ele_num ][ prf-1 ] = pulse_cnt;
         cnt++;

      }

      if( cnt != Allowable_prfs.num_alwbl_prf ){

         LE_send_msg( GL_ERROR, "Incorrect number of pulse counts (%d != %d)\n",
                      cnt, Allowable_prfs.num_alwbl_prf );
         return(-1);

      }


   }

   return (0);

} /* End of Read_alwb_prfs() */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Convenience function for setting the key used to parse the 
//      elevation cut data.
//
//   Input:
//      elev_cut - elevation cut number (unit indexed).
//
//   Returns:
//      Pointer to string holding the key.
//
///////////////////////////////////////////////////////////////////\*/
static char* Set_elev_key( int elev_cut ){

   static char elev_key[8];

   elev_key[0] = 0;
   sprintf( elev_key, "Elev_%0d", elev_cut );

   return( elev_key );

} /* End of Set_elev_key() */

/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Convenience function for setting the key used to parse the 
//      PRF sector data.
//
//   Input:
//      elev_cut - elevation cut number (unit indexed).
//
//   Returns:
//      Pointer to string holding the key.
//
///////////////////////////////////////////////////////////////////\*/
static char* Set_sector_key( int sector_num ){

   static char sector_key[9];

   sector_key[0] = 0;
   sprintf( sector_key, "Sector_%0d", sector_num );

   return( sector_key );

} /* End of Set_sector_key() */

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function initializes the configuration file names.
//
//   Return: 
//      It returns 0 on success or -1 on failure.
//
////////////////////////////////////////////////////////////////////////\*/
static int Set_config_file_names( ){

    int err = 0;                /* error flag */
    int len;
    char tmpbuf [CFG_NAME_SIZE];
    char *buf, *site_name = NULL;

    memset( tmpbuf, 0, sizeof(tmpbuf) );

    if( DEAU_get_string_values( "site_info.rpg_name", &buf ) > 0 ){

       site_name = calloc( 1, strlen(buf) + 1 );
       if( site_name != NULL ){

          /* Make sure the site_name is always uppercase. */
          strcpy( site_name, buf );
          site_name = MISC_toupper( site_name );

       }

    }

    /* get the configuration source directory. */
    len = MISC_get_cfg_dir (Cfg_dir, CFG_NAME_SIZE);
    if (len > 0)
       strcat (Cfg_dir, "/");

    strcpy (Vcp_basename, "vcp");

    if( site_name != NULL ){

       strcpy (Vcp_site_basename, site_name);
       strcat (Vcp_site_basename, "_vcp");

    }
    else
       strcpy (Vcp_site_basename, "vcp");

    /* Append the filename to the CFG_DIR.  If CFG_DIR not defined, 
       return error. */
    if( len <= 0 ){

       err = -1;
       LE_send_msg (GL_INFO, "CFG Directory Undefined\n");

     }
     else{

       /* Process the standard VCPs and the site-specific VCPs. */
       strcpy (tmpbuf, Cfg_dir);
       strcat (tmpbuf, Vcp_basename);
       strcpy (Vcp_dir_name, tmpbuf);

    }

    LE_send_msg( GL_INFO, "Vcp_basename: %s, Vcp_site_basename: %s, Vcp_dir_name: %s\n",
                 Vcp_basename, Vcp_site_basename, Vcp_dir_name );

    free(site_name);

    return (err);

} /* End of Set_config_file_names() */

/*\/////////////////////////////////////////////////////////////////
//
//   Description:
//
//      Returns the name of the first (dir_name != NULL) or the next 
//      (dir_name = NULL) file in directory "dir_name" whose name matches 
//      "basename".*. The caller provides the buffer "buf" of size 
//      "buf_size" for returning the file name. 
//
//   Inputs:  
//      dir_name - directory name or NULL
//      basename - product table base name
//      buf - receiving buffer for next file name
//      buf_size - size of receiving buffer
//
//   Outputs:
//      buf - contains next file name
//
//   Returns:
//      It returns 0 on success or -1 on failure.
//
/////////////////////////////////////////////////////////////////\*/
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size ){

    static DIR *dir = NULL;     /* the current open dir */
    static char saved_dirname[CFG_NAME_SIZE] = "";
    struct dirent *dp;
    char *basename_lower = NULL;
    int ret = -1;

    /* If directory is not NULL, open directory. */
    if( dir_name != NULL ){

        int len;

        len = strlen (dir_name);
        if (len + 1 >= CFG_NAME_SIZE) {
            LE_send_msg (GL_ERROR,
                "dir name (%s) does not fit in tmp buffer\n", dir_name);
            return (-1);
        }
        strcpy (saved_dirname, dir_name);
        if (len == 0 || saved_dirname[len - 1] != '/')
            strcat (saved_dirname, "/");
        if (dir != NULL)
            closedir (dir);
        dir = opendir (dir_name);
        if (dir == NULL)
            return (-1);
    }

    if (dir == NULL)
        return (-1);

    /* Make a copy of the basename and make all lower case. */
    basename_lower = strdup( basename );
    basename_lower = MISC_tolower( basename_lower );

    /* Read the directory. */
    while ((dp = readdir (dir)) != NULL) {

        struct stat st;
        char fullpath[2 * CFG_NAME_SIZE];

        if( (strncmp (basename, dp->d_name, strlen (basename)) != 0)
                                  &&
            (strncmp (basename_lower, dp->d_name, strlen (basename)) != 0) )
            continue;
        if (strlen (dp->d_name) >= CFG_NAME_SIZE) {
            LE_send_msg (GL_ERROR,
                "file name (%s) does not fit in tmp buffer\n", dp->d_name);
            continue;
        }
        strcpy (fullpath, saved_dirname);
        strcat (fullpath, dp->d_name);

        if (stat (fullpath, &st) < 0) {
            LE_send_msg (GL_ERROR,
                "stat (%s) failed, errno %d\n", fullpath, errno);
            continue;
        }
        if (!(st.st_mode & S_IFREG))    /* not a regular file */
            continue;

        if (strlen (fullpath) >= buf_size) {
            LE_send_msg (GL_ERROR,
                "caller's buffer is too small (for %s)\n", fullpath);
            continue;
        }
        strcpy (buf, fullpath);
        ret = 0;
        break;
    }

    if( basename_lower != NULL )
       free(basename_lower);

    return (ret);

/* End of Get_next_file_name() */
}

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Process command line arguments.
//
//   Inputs:
//      argc - number of command line arguments.
//      argv - the command line arguments.
//      vcp_num - for CLEAR, the VCP to remove.   For STARTUP/RESTART, the
//                VCP to add.
//
//   Outputs:
//      startup_action - start up action (STARTUP or RESTART)
//
//   Returns:
//      exits on error, or returns 0 on success.
//
///////////////////////////////////////////////////////////////////////////\*/
static int Get_command_line_options( int argc, char *argv[], int *startup_action,
                                     int *vcp_num ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];
   
   /* Initialize startup_action to RESTART and vcp_num to 0. */
   *startup_action = RESTART;
   *vcp_num = 0;

   Verbose_mode = 0;

   err = 0;
   while ((c = getopt (argc, argv, "hvt:p:")) != EOF) {

      switch (c) {

         case 't':
            if( strlen( optarg ) < 255 ){

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" ) ;
                  err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else
                     *startup_action = RESTART;

               }

            }
            else
               err = 1;

            break;

         case 'p':
            ret = sscanf( optarg, "%d", vcp_num );
            if( ret == EOF ){

               LE_send_msg( GL_ERROR, "VCP number needs to be specified\n" );
               err = 1;

            }
            break;

         case 'v':
            Verbose_mode = 1;
            break;

         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if (err == 1) {              /* Print usage message */
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h Help (print usage msg and exit)\n");
      printf ("\t\t-v Verbose Mode\n");
      printf ("\t\t-t Startup Mode/Action (optional - default: restart)\n" );
      printf ("\t\t-p VCP Number (optional - default: ALL)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Verifies the contents of the Rdccon array.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      -1 on error, 0 otherwise.
// 
//////////////////////////////////////////////////////////////////////////\*/
static int Verify_rdccon(){

   int cnt = 0;
   int ele_angle = -200;
   int i;
   Ele_attr *Elev;

   /* Check the first element of Rdccon.  If 0, then the entire table needs
      to be built. */
   if( Rdccon[0] == 0 ){

      for( i = 0; i < Vcp.n_ele; i++ ){

         Elev = (Ele_attr *) Vcp.vcp_ele[ i ];
         if( Elev->ele_angle != ele_angle ){

            cnt++;
            ele_angle = Elev->ele_angle;

         }

         Rdccon[i] = cnt;

      }

   } /* End of "For All Elevation Cuts" loop. */

   /* The entries in Rdccon need to be monotonically increasing and the total number
      of entries must equal the number of elevations in the VCP. */  
   for( i = 1; i < Vcp.n_ele; i++ ){

      if( (Rdccon[i] < Rdccon[i-1]) || (Rdccon[i] == 0) ){

         LE_send_msg( GL_ERROR, "(Rdccon[%d]: %d !< Rdccon[%d]: %d) || (Rdccon[%d]: %d == 0)\n",
                      i, Rdccon[i], i-1, Rdccon[i-1], i, Rdccon[i] );     
         return( -1 );

      }

   } /* End of "For All Elevation Cuts" loop. */

   return( 0 );

} /* End of Verify_rdccon() */

/*\///////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Gets the CS configuration file name.  The file is assume to be located
//      in the configuration directory, in subdirectory vcp_translations with
//      filename translation_table.
//
//   Inputs:
//      tmpbuf - holds the CS configuration file name.
//
//   Returns:
//      Negative number on error, 0 on success.
//
//////////////////////////////////////////////////////////////////////////////\*/
static int Get_trans_tbl_config_file_name( char *tmpbuf ){

   char cfg_dir[CFG_NAME_SIZE];
   int err = 0;                /* error flag */
   int len;

   memset( tmpbuf, 0, sizeof(tmpbuf) );

   /* Get the configuration source directory. */
   len = MISC_get_cfg_dir (cfg_dir, CFG_NAME_SIZE);
   if (len > 0)
      strcat (cfg_dir, "/");

   /* Append the filename to the CFG_DIR.  If CFG_DIR not defined,
      return error. */
   if( len <= 0 ){

      err = -1;
      LE_send_msg (GL_INFO, "CFG Directory Undefined\n");

    }
    else{

       /* Construct the VCP Translation Table name. */
       strcpy (tmpbuf, cfg_dir);
       strcat (tmpbuf, "vcp_translations/translation_table");

    }

    return (err);

} /* End of Get_trans_tbl_config_file_name() */

