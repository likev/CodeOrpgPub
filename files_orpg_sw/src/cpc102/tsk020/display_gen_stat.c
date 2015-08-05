/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/20 14:42:17 $
 * $Id: display_gen_stat.c,v 1.9 2014/08/20 14:42:17 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <orpg.h>
#include <misc.h>
#include <gen_stat_msg.h>
#include <rdacnt.h>
#include <orpgsite.h>

/* Macros for writing VCP data. */
#define FROM_VOLUME_STATUS	1
#define FROM_RDA_RDACNT		2
#define STATUS_WORDS		26
#define MAX_STATUS_LENGTH	64
#define MAX_PWR_BITS		5
#define COMSWITCH_BIT		4


static  char            status[][32]            = { "Start-Up",
                                                    "Standby",
                                                    "Restart",
                                                    "Operate",
                                                    "Playback",
                                                    "Off-Line Operate",
                                                    "      " };
static  char            *moments[]              = { "None",
                                                    "All ",
                                                    "R",
                                                    "V",
                                                    "W",
                                                    "      " };
static  char            orda_mode[][32]         = { "Operational",
                                                    "Test",
                                                    "Maintenance",
                                                    "      " };
static  char            authority[][32]         = { "No Action",
                                                    "Local Control Requested",
                                                    "Local Control Released",
                                                    "      " };
static  char            channel_status[][32]    = { "Ctl",
                                                    "Non-Ctl",
                                                    "      " };
static  char            spot_blanking[][32]     = { "Not Installed",
                                                    "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            operability[][32]       = { "On-Line",
                                                    "MAR",
                                                    "MAM",
                                                    "CommShut",
                                                    "Inoperable",
                                                    "WB Disc",
                                                    "      " };
static  char            control[][32]           = { "RDA",
                                                    "RPG",
                                                    "Eit",
                                                    "      " };
static  char            *set_aux_pwr[]          = { " Aux Pwr=On",
                                                    " Util Pwr=Yes",
                                                    " Gen=On",
                                                    " Xfer=Manual",
                                                    " Cmd Pwr Switch",
                                                    "      " };
static  char            *reset_aux_pwr[]        = { " Aux Pwr=Off",
                                                    " Util Pwr=No",
                                                    " Gen=Off",
                                                    " Xfer=Auto",
                                                    "",
                                                    "      " };
static  char            tps[][32]               = { "Off",
                                                    "Ok",
                                                    "      " };

static  char            perf_check[][32]        = { "Auto",
                                                    "Pending",
                                                    "      " };
static  char            super_res[][32]         = { "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            cmd[][32]               = { "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            avset[][32]             = { "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            alarm_sum[][32]         = { "No Alarms",
                                                    "Tow/Util",
                                                    "Pedestal",
                                                    "Transmitter",
                                                    "Receiver",
                                                    "RDA Cntrl",
                                                    "Comms",
                                                    "Sig Proc",
                                                    "        " };
static  char            cmd_ack[][32]           = { "Remote VCP Received",
                                                    "Bypass Map Received",
                                                    "Clutter Censor Zones Received",
                                                    "Red Chan Cntrl Cmd Accepted",
                                                    "        " };
static  char            rms[][32]               = { "Non-RMS System",
                                                    "RMS In Control",
                                                    "RDA In Control",
                                                    "        " };
static char             wbstat[][32]            = { "Not Implemented",
                                                    "Connect Pending",
                                                    "Disconnect Pending",
                                                    "Disconnected HCI",
                                                    "Disconnected CM",
                                                    "Disconnected/Shutdown",
                                                    "Connected",
                                                    "Down",
                                                    "WB Failure",
                                                    "Disconnected RMS",
                                                    "        " };


/* Function Prototypes. */
static int Display_volume_status();
static int Display_rda_rdacnt();
static void Write_vcp_data( Vcp_struct *vcp, int from );
static void Display_previous_rda_state();
static void Display_rda_status();

/**************************************************************

   Tools for displaying the volume status data. 

**************************************************************/
int main( int argc, char *argv[] ){

   int ret, response = 0;
   char *ds_name = NULL;
 
   /* Needed for accessing adaptationd data. */
   ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
   if( ds_name != NULL )
      DEAU_LB_name( ds_name );

   while(1){

      fprintf( stderr, "Choose for the following ...\n" );
      fprintf( stderr, "1) - Display Volume Status Data\n" );
      fprintf( stderr, "2) - Display RDA RDACNT Data\n" );
      fprintf( stderr, "3) - Display Previous RDA State\n" );
      fprintf( stderr, "4) - Display RDA Status\n" );
      fprintf( stderr, "0) - Exit\n" );

      ret = scanf( "%d", &response );
      if( (ret > 0) && ((response < 1) || (response > 4)) )
         exit(0);

      switch( response ){

         case 1:

            Display_volume_status();
            break;

         case 2:

            Display_rda_rdacnt();
            break;

         case 3:
    
            Display_previous_rda_state();
            break;

         case 4:
    
            Display_rda_status();
            break;

         default:
            exit(0);

      } 
    
      response = 0;

   }

   return 0;

}

/*************************************************************

   Description:
      Function that displays the volume status.

*************************************************************/
int Display_volume_status(){

   Vol_stat_gsm_t gsm, *ret = NULL;
   time_t cv_time;
   int i, year, mon, day, hr, min, sec;
   int response;
   char text[128], str[6];

   /* Read the volume status data. */
   ret = (Vol_stat_gsm_t *) ORPGVST_read( (char *) &gsm ); 
   if( ret == NULL )
      return -1;

   fprintf( stderr, "Volume Status\n" );

   /* Volume Number. */
   fprintf( stderr, "--->Volume Number:         %ld\n", gsm.volume_number );

   /* Volume scan date/time. */
   cv_time = (gsm.cv_julian_date-1)*86400 + gsm.cv_time / 1000;
   unix_time( &cv_time, &year, &mon, &day, &hr, &min, &sec );

   year -= 2000;
   fprintf( stderr, "--->Volume Date/Time:      %02d/%02d/%02d %02d:%02d:%02d\n",
            mon, day, year, hr, min, sec );

   /* Initial volume scan? */
   if( gsm.initial_vol )
      fprintf( stderr, "--->Initial Vol:           Yes\n" );
   else
      fprintf( stderr, "--->Initial Vol:           No\n" );

   /* Previous volume status. */
   if( gsm.pv_status )
      fprintf( stderr, "--->Previous Vol:          Completed\n" );
   else 
      fprintf( stderr, "--->Previous Vol:          Aborted\n" );

   /* Expected volume duration. */
   fprintf( stderr, "--->Volume Duration:       %3d secs\n", gsm.expected_vol_dur );

   /* Expected volume scan number. */
   fprintf( stderr, "--->Volume Scan:           %2d\n", gsm.volume_scan );

   /* Mode of operation. */
   if( gsm.mode_operation == MAINTENANCE_MODE )
      fprintf( stderr, "--->Mode:                  Maintenance\n" );
   else if( gsm.mode_operation == CLEAR_AIR_MODE )
      fprintf( stderr, "--->Mode:                  Clear Air\n" );
   else if( gsm.mode_operation == PRECIPITATION_MODE )
      fprintf( stderr, "--->Mode:                  Precipitation\n" );
   else
      fprintf( stderr, "--->Mode:                  ???????\n" );

   /* Dual Pol expected? */
   if( gsm.dual_pol_expected )
      fprintf( stderr, "--->Dual Pol Expected:     Yes\n" );
   else
      fprintf( stderr, "--->Dual Pol Expected:     No\n" );

   /* VCP. */
   fprintf( stderr, "--->VCP:                   %3d\n", gsm.vol_cov_patt );

   /* RPGVCPID. */
   fprintf( stderr, "--->VCP ID:                %3d\n", gsm.rpgvcpid );

   /* Number of elevation cuts. */
   fprintf( stderr, "--->Number Cuts:           %2d\n", gsm.num_elev_cuts );

   /* Number of SAILS cuts. */
   fprintf( stderr, "--->Number SAILS Cuts:     %2d\n", gsm.n_sails_cuts );

   /* AVSET Termination Angle. */
   fprintf( stderr, "--->AVSET Term Angle:      %3d\n", gsm.avset_term_ang );

   /* Elevations. */
   memset( text, 0, 128 );
   memset( str, 0, 6 );
   for( i = 0; i < gsm.num_elev_cuts; i++ ){

      sprintf( str, "%4.1f ", (float) gsm.elevations[i]/10.0 );
      strcat( text, str ); 

   }
      
   fprintf( stderr, "--->Elevations (deg):      %s\n", text );

   /* RPG elevation index. */
   memset( text, 0, 128 );
   memset( str, 0, 6 );
   for( i = 0; i < gsm.num_elev_cuts; i++ ){

      sprintf( str, "%2d ", gsm.elev_index[i] );
      strcat( text, str );

   }
      
   fprintf( stderr, "--->RPG Elev Index:        %s\n", text );

   /* SAILs cut sequence numbers. */
   if( gsm.n_sails_cuts > 0 ){

      memset( text, 0, 128 );
      memset( str, 0, 10 );
      for( i = 0; i < gsm.num_elev_cuts; i++ ){

         sprintf( str, "%2d ", gsm.sails_cut_seq[i] );
         strcat( text, str );

      }

      fprintf( stderr, "--->SAILS Cut Seq:         %s\n", text );

   }

   /* Super Resolution cuts. */
   fprintf( stderr, "--->Super Res:             %x\n", gsm.super_res_cuts );

   /* VCP Supplemental Data. */
   fprintf( stderr, "--->VCP Supplemental:\n" );
   if( gsm.vcp_supp_data & VSS_AVSET_ENABLED )
      fprintf( stderr, "------>AVSET Enabled:         Yes\n" );
   else
      fprintf( stderr, "------>AVSET Enabled:         No\n" );
   
   if( gsm.vcp_supp_data & VSS_SAILS_ACTIVE )
      fprintf( stderr, "------>SAILS Active:          Yes\n" );
   else
      fprintf( stderr, "------>SAILS Active:          No\n" );
   
   if( gsm.vcp_supp_data & VSS_SITE_SPECIFIC_VCP )
      fprintf( stderr, "------>Site-Specific VCP:     Yes\n" );
   else
      fprintf( stderr, "------>Site-Specific VCP:     No\n" );

   /* Ask if VCP data is to be disilayed. */
   fprintf( stderr, "\n\nChoose for the following ...\n" );
   fprintf( stderr, "1) - Display Volume Coverage Pattern Data\n" );
   fprintf( stderr, "0) - Return\n" );

   scanf( "%d", &response );
   if( (response < 1) || (response > 1) )
      return (0);

   /* Current VCP. */
   Write_vcp_data( &gsm.current_vcp_table, FROM_VOLUME_STATUS );

   /* Put some blank lines .... */
   fprintf( stderr, "\n\n" );

   return 0;

}


/********************************************************************************
   
   Description:
      Writes out the VCP definition.

   Inputs:
      vcp - pointer to VCP data ... format specified in vcp.h.

********************************************************************************/
static void Write_vcp_data( Vcp_struct *vcp, int from ){

   static char *reso[] = { "0.5 m/s", "1.0 m/s" };
   static char *width[] = { "SHORT", "LONG" };
   static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
   static char *phase[] = { "CON", "RAN", "SZ2" };

   int i, expected_size;
   short wform, phse;

   /* Write out VCP data. */
   fprintf( stderr, "\n\nVCP %d Data:\n", vcp->vcp_num );
   fprintf( stderr, "--->VCP Header:\n" ); 
   fprintf( stderr, "       Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
            vcp->msg_size, vcp->type, vcp->n_ele );
   fprintf( stderr, "       Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n\n",
            vcp->clutter_map_num, reso[ vcp->vel_resolution/4 ], 
            width[ vcp->pulse_width/4 ] );

   /* Do some validation. */
   expected_size = VCP_ATTR_SIZE + vcp->n_ele*(sizeof(Ele_attr)/sizeof(short));
   if( vcp->msg_size != expected_size )
      fprintf( stderr, "VCP Size: %d Not Expected: %d\n", 
                   vcp->msg_size, expected_size ); 

   /* Do For All elevation cuts. */
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[i][0];

      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle ); 

      wform = elev->wave_type;
      phse = elev->phase;
      fprintf( stderr, "--->Elevation %d:\n", i+1 );
      fprintf( stderr, "       Elev Angle: %5.2f   Wave Type: %s   Phase: %s   Super Res Flag: %3d, Surv PRF: %2d   Surv Pulses: %4d\n",
                   elev_angle, wave_form[ wform ], phase[ phse ], 
                   elev->super_res, elev->surv_prf_num, elev->surv_pulse_cnt );
      fprintf( stderr, "       Az Rate: %5.2f (0x%4x BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f (dB)\n",
                   elev->azi_rate*ORPGVCP_AZIMUTH_RATE_FACTOR, elev->azi_rate, (float) elev->surv_thr_parm/8.0, 
                   (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0 );

      fprintf( stderr, "       PRF Sector 1:\n" );
      if( from == FROM_VOLUME_STATUS )
         fprintf( stderr, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      (float) elev->azi_ang_1 / 10.0, elev->dop_prf_num_1, elev->pulse_cnt_1 );
      else
         fprintf( stderr, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_1 ), 
                      elev->dop_prf_num_1, elev->pulse_cnt_1 );

      fprintf( stderr, "       PRF Sector 2:\n" );
      if( from == FROM_VOLUME_STATUS )
         fprintf( stderr, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      (float) elev->azi_ang_2 / 10.0, elev->dop_prf_num_2, elev->pulse_cnt_2 );
      else
         fprintf( stderr, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_2 ), 
                      elev->dop_prf_num_2, elev->pulse_cnt_2 );

      fprintf( stderr, "       PRF Sector 3:\n" );
      if( from == FROM_VOLUME_STATUS )
         fprintf( stderr, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                     (float) elev->azi_ang_3 / 10.0, elev->dop_prf_num_3, elev->pulse_cnt_3 );
      else
         fprintf( stderr, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                     ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_3 ), 
                     elev->dop_prf_num_3, elev->pulse_cnt_3 );

   }   

}

/*************************************************************

   Description:
      Function that displays the RDA RDACNT data.

*************************************************************/
int Display_rda_rdacnt(){

   int i, response, ret, year, mon, day, hr, min, sec;
   Vcp_struct *vcp = NULL;
   unsigned short *suppl = NULL;

   static RDA_rdacnt_t rdacnt;
   static char text[128], str[128];

   /* Read the data. */
   ret = ORPGDA_read( ORPGDAT_ADAPTATION, &rdacnt,
                           sizeof(RDA_rdacnt_t), RDA_RDACNT );

   if( ret < sizeof(RDA_rdacnt_t) ){

      fprintf( stderr, "ORPGDA_read( RDA_RDACNT ) Failed: %d\n", ret );
      return -1;
  
   }

   fprintf( stderr, "RDA RDACNT Data\n" );

   /* Last index updated in rdacnt.data[]. */
   fprintf( stderr, "--->Last Entry (ind):  %d\n", rdacnt.last_entry );
   
   /* Last time (UTC) rdacnt was updated. */
   unix_time( &rdacnt.last_entry_time, &year, &mon, &day, &hr, &min, &sec );
   
   year -= 2000;
   fprintf( stderr, "--->Last Entry Time:   %02d/%02d/%02d %02d:%02d:%02d\n",
            mon, day, year, hr, min, sec );

   /* Last time (UTC) VCP message updated. */
   unix_time( &rdacnt.last_message_time, &year, &mon, &day, &hr, &min, &sec );
   
   year -= 2000;
   fprintf( stderr, "--->Last Message Time: %02d/%02d/%02d %02d:%02d:%02d\n",
            mon, day, year, hr, min, sec );

   /* Ask which index to display. */
   fprintf( stderr, "\n\nWhich index? [-1: Exit, or [0, 1]]\n" );
   scanf( "%d", &response );
   if( (response < 0) || (response > 1) )
      return (0);

   /* Volume Scan number. */
   fprintf( stderr, "--->Volume Scan #:     %2d\n",  
            rdacnt.data[response].volume_scan_number );

   /* Supplement Flags and RDA to RPG Elevation mapping. */
   fprintf( stderr, "--->VCP Supplemental Flags and RPG Elev Index:\n" );

   vcp = (Vcp_struct *) &rdacnt.data[response].rdcvcpta[0];
   suppl = (unsigned short *) &rdacnt.data[response].suppl[0];
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *ele = (Ele_attr *) &vcp->vcp_ele[i][0];
      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, ele->ele_angle ); 

      /* Initialize strings. */
      memset( text, 0, 128 );
      memset( str, 0, 128 );

      sprintf( str, "    RDA Elev %2d, RPG Elev Index %2d (Elev %5.2f): ", 
               i+1, rdacnt.data[response].rdccon[i], elev_angle );
      strcat( text, str );

      /* Check supplemental flag bits. */
      if( suppl[i] & RDACNT_IS_CS ){

         sprintf( str, "CS " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_CD ){

         sprintf( str, "CD/W " );
         strcat( text, str );

      }
      
      if( suppl[i] & RDACNT_IS_CDBATCH ){

         sprintf( str, "CD/WO " );
         strcat( text, str );

      }
      
      if( suppl[i] & RDACNT_IS_BATCH ){

         sprintf( str, "BATCH " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_SPRT ){

         sprintf( str, "SPRT " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_SR ){

         sprintf( str, "SR " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_SZ2 ){

         sprintf( str, "SZ2 " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_SUPPL_SCAN ){

         sprintf( str, "SAILS " );
         strcat( text, str );

      }

      fprintf( stderr, "%s\n", text );

   }

   /* Current VCP. */
   Write_vcp_data( vcp, FROM_RDA_RDACNT );

   /* Put some blank lines .... */
   fprintf( stderr, "\n\n" );

   return 0;

}

/*******************************************************************************
 
  Description:
     Writes Previous RDA state data to task log file.
 
  Inputs:
     none
 
  Return:
     void
*******************************************************************************/
void Display_previous_rda_state(){

   int i = 0,ret = 0;
   int stat = 0;
   int p_rda_stat = 0;
   int p_vcp = 0;
   int p_rda_contr_auth = 0;
   int p_chan_control = 0;
   int p_spot_blank_stat = 0;
   Vcp_struct p_vcp_data = {0};
   int vcp_size = 0;
   double deau_ret_val = 0.0;

   /* Read the previous state data. */
   ret = ORPGRDA_read_previous_state();
   if( ret < 0 ){

      fprintf( stderr, "Read of Previous RDA Status Data Failed\n" );
      return;

   }

   /* Retrieve and store data fields */
   p_rda_stat = ORPGRDA_get_previous_state( ORPGRDA_RDA_STATUS );
   if ( p_rda_stat == ORPGRDA_DATA_NOT_FOUND ){

      fprintf( stderr, "Problem retrieving ORPGRDA_RDA_STATUS\n" );
      p_rda_stat = 0; /* Reset value to 0 */

   }

   p_vcp = ORPGRDA_get_previous_state( ORPGRDA_VCP_NUMBER );
   if ( p_vcp == ORPGRDA_DATA_NOT_FOUND ){

      fprintf( stderr, "Problem retrieving ORPGRDA_VCP_NUMBER\n" );
      p_vcp = 0;  /* Reset value to 0 */

   }

   p_rda_contr_auth = ORPGRDA_get_previous_state( ORPGRDA_RDA_CONTROL_AUTH );
   if ( p_rda_contr_auth == ORPGRDA_DATA_NOT_FOUND ){

      fprintf( stderr, "Problem retrieving ORPGRDA_RDA_CONTROL_AUTH\n" );
      p_rda_contr_auth = 0;  /* Reset value to 0 */

   }

   p_chan_control = ORPGRDA_get_previous_state( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( p_chan_control == ORPGRDA_DATA_NOT_FOUND ){

      fprintf( stderr, "Problem retrieving ORPGRDA_CHAN_CONTROL_STATUS\n" );
      p_chan_control = 0;  /* Reset value to 0 */

   }

   p_spot_blank_stat = ORPGRDA_get_previous_state( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( p_spot_blank_stat == ORPGRDA_DATA_NOT_FOUND ){

      fprintf( stderr, "Problem retrieving ORPGRDA_SPOT_BLANKING_STATUS\n");
      p_spot_blank_stat = 0;  /* Reset value to 0 */

   }

   fprintf( stderr, "Previous RDA State:\n" );

   /* case RS_RDA_STATUS  */
   if( (p_rda_stat & RS_STARTUP) )
      i = 0;

   else if( (p_rda_stat & RS_STANDBY) )
      i = 1;

   else if( (p_rda_stat & RS_RESTART) )
      i = 2;

   else if( (p_rda_stat & RS_OPERATE) )
      i = 3;

   else if( (p_rda_stat & RS_PLAYBACK) )
      i = 4;

   else if( (p_rda_stat & RS_OFFOPER) )
      i = 5;

   else{

      /* Unknown value. */
      i = 6;
      sprintf( status[i], "%6d", p_rda_stat );

   }

   fprintf( stderr, "--->RDA Status:        %s\n", status[i] );

   /* case RS_VCP_NUMBER */
   {
      char temp[10];

      /* Clear temporary buffer. */
      memset( temp, 0, 10 );

      /* Determine if vcp is "local" or "remote" pattern. */
      if( p_vcp < 0 ){

         p_vcp = -p_vcp;
         temp[0] = 'L';

      }
      else
         temp[0] = 'R';

      /* Encode VCP number. */
      sprintf( &temp[1], "%d", p_vcp );

      fprintf( stderr, "--->RDA VCP:           %s\n", temp );

   }

   /* case RS_RDA_CONTROL_AUTH */
   if( p_rda_contr_auth == CA_NO_ACTION )
      i = 0;

   else if( p_rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
      i = 1;
      
   else if( p_rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
      i = 2;
      
   fprintf( stderr, "--->RDA Control Auth:  %s\n", authority[i] );

   /* case RS_CHAN_CONTROL_STATUS */
   if ( (stat =
      DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1)) >= 0){

      if ( (int) deau_ret_val == ORPGSITE_FAA_REDUNDANT ){

         if( p_chan_control == RDA_IS_CONTROLLING )
            i = 0;
         
         else if( p_chan_control == RDA_IS_NON_CONTROLLING )
            i = 1;
         
         else{

            /* Unknown value.  Place value in status buffer. */
            i = 2;
            sprintf( channel_status[i], "%6d", p_chan_control );

         }

         fprintf( stderr, "--->RDA Channel Cntrl: %s\n", channel_status[i] );

      }

   }
   else
      fprintf( stderr, "DEAU_get_values returned error.\n");

   /* case RS_SPOT_BLANKING_STATUS */
   /* If spot blanking not installed, break. */
   if( p_spot_blank_stat != SB_NOT_INSTALLED ){

      /* Process spot blanking status. */
      if( p_spot_blank_stat == SB_ENABLED )
         i = 1;
      
      else if( p_spot_blank_stat == SB_DISABLED )
         i = 2;
      
      else{

         /* Unknown value. Place value in status buffer. */
         i = 3;
         sprintf( spot_blanking[i], "%6d", p_spot_blank_stat );

      }

      fprintf( stderr, "--->RDA Spot Blanking: %s\n", spot_blanking[i] );

   }

   /* Is the previous state VCP data available? */
   ret = ORPGRDA_get_previous_state_vcp( (char *) &p_vcp_data, &vcp_size );
   if( (ret != ORPGRDA_DATA_NOT_FOUND)
                  &&
           (vcp_size != 0) )
      Write_vcp_data( &p_vcp_data, FROM_RDA_RDACNT );

   /* Put some blank lines .... */
   fprintf( stderr, "\n\n" );

} 

/*******************************************************************************
 
  Description:
        Writes Open RDA status data to system status log file in plain text 
        format.  Only status which is different than last reported is reported.
 
  Inputs:
        none
 
  Return:
        void
*******************************************************************************/
static void Display_rda_status(){

   int stat = 0;
   int i, hw = 0;
   int rda_stat = 0;
   int op_stat = 0;
   int control_stat = 0;
   int aux_pwr_stat = 0;
   int data_trans_enab = 0;
   int vcp = 0;
   int rda_contr_auth = 0;
   int opmode = 0;
   int chan_stat = 0;
   int spot_blank_stat = 0;
   int tps_stat = 0;
   int perf_check_status = 0;

   int avg_trans_pwr = 0;
   int h_ref_dBZ0 = 0;
   int v_ref_dBZ0 = 0;
   int rda_build_num = 0;
   int super_reso = 0;
   int cmd_status = 0;
   int avset_status = 0;
   int rda_alarm_sum = 0;
   int rda_command_ack = 0;
   int bpm_gen_date = 0;
   int bpm_gen_time = 0;
   int clm_gen_date = 0;
   int clm_gen_time = 0;
   int rms_stat = 0;

   int wblnstat = 0;
   int display_blanking = 0;
   int wb_failed = 0;

   double deau_ret_val = 0.0;

   /* Print header string. */
   fprintf( stderr, "Wideband Comms Status:\n" );

   /* Get line status information. */
   wblnstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
   if( wblnstat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_WBLNSTAT\n" );

   display_blanking = ORPGRDA_get_wb_status( ORPGRDA_DISPLAY_BLANKING );   
   if( display_blanking == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_DISPLAY_BLANKING\n" );

   wb_failed = ORPGRDA_get_wb_status( ORPGRDA_WBFAILED );
   if( wb_failed == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_WB_FAILED\n" );

   /* Process wideband line status. */
   if( (wblnstat == RS_NOT_IMPLEMENTED) )
      i = 0;

   else if( wblnstat == RS_CONNECT_PENDING )
      i = 1;

   else if( wblnstat == RS_DISCONNECT_PENDING )
      i = 2;

   else if( wblnstat == RS_DISCONNECTED_HCI )
      i = 3;

   else if( wblnstat == RS_DISCONNECTED_CM )
      i = 4;

   else if( wblnstat == RS_DISCONNECTED_SHUTDOWN )
      i = 5;

   else if( wblnstat == RS_CONNECTED )
      i = 6;

   else if( wblnstat == RS_DOWN )
      i = 7;

   else if( wblnstat == RS_WBFAILURE )
      i = 8;

   else if( wblnstat == RS_DISCONNECTED_RMS )
      i = 9;

   else{

      /* Unknown value.  Place value in status buffer. */
      i = 10;
      sprintf( wbstat[i], "%6d", wblnstat );

   }

   /* Print Wideband Line Status information. */
   fprintf( stderr, "--->WB Line Status:           %s\n", wbstat[i] );

   if( display_blanking )
      fprintf( stderr, "--->Display Blanking:         True\n" );

   else
      fprintf( stderr, "--->Display Blanking:         False\n" );

   if( wb_failed )
      fprintf( stderr, "--->WB Failed:                True\n" );

   else
      fprintf( stderr, "--->WB Failed:                False\n" );

   /* Put a blank line .... */
   fprintf( stderr, "\n" );

   /* Print RDA Status information. */

   /* Print header string. */
   fprintf( stderr, "RDA Status:\n" );

   /* Retrieve and store current rda status fields */
   rda_stat = ORPGRDA_get_status( ORPGRDA_RDA_STATUS );
   if ( rda_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_RDA_STATUS\n" );
    
   op_stat = ORPGRDA_get_status( ORPGRDA_OPERABILITY_STATUS );
   if ( op_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_OPERABILITY_STATUS\n" );
    
   control_stat = ORPGRDA_get_status( ORPGRDA_CONTROL_STATUS );
   if ( control_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_CONTROL_STATUS\n" );

   aux_pwr_stat = ORPGRDA_get_status( ORPGRDA_AUX_POWER_GEN_STATE );
   if ( aux_pwr_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_AUX_POWER_GEN_STATE\n" );

   data_trans_enab = ORPGRDA_get_status( ORPGRDA_DATA_TRANS_ENABLED );
   if ( data_trans_enab == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_DATA_TRANS_ENABLED\n" );

   vcp = ORPGRDA_get_status( ORPGRDA_VCP_NUMBER );
   if ( vcp == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_VCP_NUMBER\n" );

   rda_contr_auth = ORPGRDA_get_status( ORPGRDA_RDA_CONTROL_AUTH );
   if ( rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_RDA_CONTROL_AUTH\n" );

   opmode = ORPGRDA_get_status( ORPGRDA_OPERATIONAL_MODE );
   if ( opmode == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_OPERATIONAL_MODE\n" );

   chan_stat = ORPGRDA_get_status( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( chan_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_CHAN_CONTROL_STATUS\n" );

   spot_blank_stat = ORPGRDA_get_status( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_SPOT_BLANKING_STATUS\n" );

   tps_stat = ORPGRDA_get_status( ORPGRDA_TPS_STATUS );
   if ( tps_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_TPS_STATUS\n" );

   perf_check_status = ORPGRDA_get_status( ORPGRDA_PERF_CHECK_STATUS );
   if ( perf_check_status == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_PERF_CHECK_STATUS\n" );

   avg_trans_pwr = ORPGRDA_get_status( ORPGRDA_AVE_TRANS_POWER );
   if( avg_trans_pwr == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_AVE_TRANS_POWER" );

   h_ref_dBZ0 = ORPGRDA_get_status( ORPGRDA_REFL_CALIB_CORRECTION );
   if( h_ref_dBZ0 == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_REFL_CALIB_CORRECTION" );

   v_ref_dBZ0 = ORPGRDA_get_status( ORPGRDA_VC_REFL_CALIB_CORRECTION );
   if( v_ref_dBZ0 == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_VC_REFL_CALIB_CORRECTION" );

   rda_build_num = ORPGRDA_get_status( ORPGRDA_RDA_BUILD_NUM );
   if( rda_build_num == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_RDA_BUILD_NUM" );

   super_reso = ORPGRDA_get_status( ORPGRDA_SUPER_RES );
   if( super_reso == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_SUPER_RES" );

   cmd_status = ORPGRDA_get_status( ORPGRDA_CMD );
   if( cmd_status == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_CMD" );

   avset_status = ORPGRDA_get_status( ORPGRDA_AVSET );
   if( avset_status == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_AVSET" );

   rda_alarm_sum = ORPGRDA_get_status( ORPGRDA_RDA_ALARM_SUMMARY );
   if( rda_alarm_sum == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_RDA_ALARM_SUMMARY" );

   rda_command_ack = ORPGRDA_get_status( ORPGRDA_COMMAND_ACK );
   if( rda_command_ack == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_COMMAND_ACK" );

   bpm_gen_date = ORPGRDA_get_status( ORPGRDA_BPM_GEN_DATE );
   if( bpm_gen_date == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_BPM_GEN_DATE" );

   bpm_gen_time = ORPGRDA_get_status( ORPGRDA_BPM_GEN_TIME );
   if( bpm_gen_time == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_BPM_GEN_TIME" );

   clm_gen_date = ORPGRDA_get_status( ORPGRDA_NWM_GEN_DATE );
   if( clm_gen_date == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_NWM_GEN_DATE" );

   clm_gen_time = ORPGRDA_get_status( ORPGRDA_NWM_GEN_TIME );
   if( clm_gen_time == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_NWM_GEN_TIME" );

   rms_stat = ORPGRDA_get_status( ORPGRDA_RMS_CONTROL_STATUS );
   if( rms_stat == ORPGRDA_DATA_NOT_FOUND )
      fprintf( stderr, "Could not retrieve ORPGRDA_RMS_CONTROL_STATUS" );

   /*
      Output status data is readable format. NOTE: range for hw should start at
      the value of the macro representing the first field in the status msg
      (defined in rda_status.h)
   */
   for( hw = 1; hw <= STATUS_WORDS; hw++ ){

      switch( hw ){

         case RS_RDA_STATUS:
         {
            int i;

            /* Process status. */
            if( (rda_stat & RS_STARTUP) )
               i = 0;

            else if( (rda_stat & RS_STANDBY) )
               i = 1;

            else if( (rda_stat & RS_RESTART) )
               i = 2;

            else if( (rda_stat & RS_OPERATE) )
               i = 3;

            else if( (rda_stat & RS_PLAYBACK) )
               i = 4;

            else if( (rda_stat & RS_OFFOPER) )
               i = 5;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( status[i], "%6d", rda_stat );

            }

            fprintf( stderr, "--->RDA Status:               %s\n", status[i] );
            break;

         }

         case RS_OPERABILITY_STATUS:
         {

            unsigned short rda_operability;
            int i;

            /* Process operability status. */
            rda_operability = (unsigned short) op_stat;

            if( (rda_operability & OS_ONLINE) )
               i = 0;

            else if( (rda_operability & OS_MAINTENANCE_REQ) )
               i = 1;

            else if( (rda_operability & OS_MAINTENANCE_MAN) )
               i = 2;

            else if( (rda_operability & OS_COMMANDED_SHUTDOWN) )
               i = 3;

            else if( (rda_operability & OS_INOPERABLE) )
               i = 4;

            else if( (rda_operability & OS_WIDEBAND_DISCONNECT) )
               i = 5;

            else
            {
              /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( operability[i], "%6d", op_stat );
            }

            fprintf( stderr, "--->Operability Status:       %s\n", operability[i] );

            break;
         }

         case RS_CONTROL_STATUS:
         {

            int i;

            /* Process RDA Control Status. */
            if( (control_stat & CS_LOCAL_ONLY) )
               i = 0;

            else if( (control_stat & CS_RPG_REMOTE) )
               i = 1;

            else if( (control_stat & CS_EITHER) )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               sprintf( control[i], "%6d", control_stat );

            }

            fprintf( stderr, "--->RDA Control Status:       %s\n", control[i] );
            break;

         }

         case RS_AUX_POWER_GEN_STATE:
         {

            int i;
            short test_bit, curr_bit;
            char temp[MAX_STATUS_LENGTH];

            /* Clear out status buffer. */
            memset( temp, 0, MAX_STATUS_LENGTH );

            /* Check which power bits have changed. */
            for( i = 0; i < MAX_PWR_BITS; i++ ){

               test_bit = 1 << i;
               curr_bit = (aux_pwr_stat & test_bit);

               /* If bit is set. */
               if( curr_bit )
                  strcat( temp, set_aux_pwr[i] );

               else if( (i != COMSWITCH_BIT)
                                 &&
                        (strlen(reset_aux_pwr[i]) > 0) )
                  strcat( temp, reset_aux_pwr[i] );

            }

            fprintf( stderr, "--->RDA Aux Power:            %s\n", temp );
            break;

         }

         case RS_AVE_TRANS_POWER:
         {

            /* Process Average Transmitter Power. */
            fprintf( stderr, "--->RDA Ave Trans Pwr:        %4d\n", avg_trans_pwr );
            break;
         }

         case RS_REFL_CALIB_CORRECTION:
         {

            /* Process Horizontal dBZ0. */
            fprintf( stderr, "--->H dBZ0:                   %7.2f\n", (float) h_ref_dBZ0 / 100.0 );
            break;

         }

         case RS_DATA_TRANS_ENABLED:
         {

            char moment_string[10];

            /* Process Moments. */
            moment_string[0] = '\0';
            if( data_trans_enab == BD_ENABLED_NONE )
               strcat( moment_string, moments[0] );

            else if( data_trans_enab == (BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH) )
               strcat( moment_string, moments[1] );

            else{

               if( (data_trans_enab & BD_REFLECTIVITY) )
                  strcat( moment_string, moments[2] );

               if( (data_trans_enab & BD_VELOCITY) ){

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[3] );

               }

               if( (data_trans_enab & BD_WIDTH) ){

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[4] );

               }

            }

            fprintf( stderr, "--->Data Enabled:             %s\n", moment_string );
            break;

         }

         case RS_VCP_NUMBER:
         {
            short temp_vcp = vcp;
            char temp[10];

            /* Clear temporary buffer. */
            memset( temp, 0, 10 );

            /* Determine if vcp is "local" or "remote" pattern. */
            if( temp_vcp < 0 )
            {
               temp_vcp = -vcp;
               temp[0] = 'L';
            }
            else
            {
               temp[0] = 'R';
            }

            /* Encode VCP number. */
            sprintf( &temp[1], "%d", temp_vcp );
            fprintf( stderr, "--->VCP:                      %s\n", temp );
            break;

         }

         case RS_RDA_CONTROL_AUTH:
         {
            int i;

            if( rda_contr_auth == CA_NO_ACTION )
               i = 0;

            else if( rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
               i = 1;

            else if( rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               sprintf( authority[i], "%6d", rda_contr_auth );

            }

            fprintf( stderr, "--->Cntrl Auth:               %s\n", authority[i] );
            break;

         }

         case RS_RDA_BUILD_NUM: 
         {

            float num;

            if( (num = (float) rda_build_num / 100.0f) > 2.0f ) 
               fprintf( stderr, "--->RDA Build #:              %4.2f\n", num  );

            else{
  
               num = (float) rda_build_num / 10.0f;
               fprintf( stderr, "--->RDA Build #:              %4.2f\n", num  );

            }

            break;

         }

         case RS_OPERATIONAL_MODE:
         {

            int i;

            /* Process operational mode. */
            if( opmode == OP_MAINTENANCE_MODE )
               i = 1;

            else if( opmode == OP_OPERATIONAL_MODE )
               i = 0;

            else if( opmode == OP_OFFLINE_MAINTENANCE_MODE )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( orda_mode[i], "%6d", opmode );

            }

            fprintf( stderr, "--->Mode:                     %s\n", orda_mode[i] );
            break;

         }

         case RS_SUPER_RES:
         {

            int i = 0;

            /* Process Super Resolution. */
            if( super_reso == SR_ENABLED )
               i = 0;

            else if( super_reso == SR_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( super_res[i], "%6d", super_reso );

            }

            fprintf( stderr, "--->Super Reso:               %s\n", super_res[i] );
            break;

         }

         case RS_CMD:
         {

            int i = 0;

            /* Process CMD Status. */
            if( cmd_status & 0x1 ){

               i = 0;
               fprintf( stderr, "--->CMD:                      %s (%x)\n", 
                        cmd[0], cmd_status/2 );

            }
            else 
               fprintf( stderr, "--->CMD:                      %s\n", cmd[1] );

            break;

         }

         case RS_AVSET:
         {

            int i = 0;

            /* Process AVSET Status. */
            if( avset_status == AVSET_ENABLED )
               i = 0;

            else if( avset_status == AVSET_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( avset[i], "%6d", avset_status );

            }

            fprintf( stderr, "--->AVSET:                    %s\n", avset[i] );
            break;

         }

         case RS_RDA_ALARM_SUMMARY:
         {

            /* Process RDA Alarm Summary. */
            if( rda_alarm_sum == 0 )
               fprintf( stderr, "--->RDA Alarm Summary:        %s\n", alarm_sum[0] );

            else{ 
   
               fprintf( stderr, "--->RDA Alarm Summary (%4x):\n", rda_alarm_sum );

               if( rda_alarm_sum & 0x2 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[1] );

               if( rda_alarm_sum & 0x4 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[2] );

               if( rda_alarm_sum & 0x8 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[3] );

               if( rda_alarm_sum & 0x10 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[4] );

               if( rda_alarm_sum & 0x20 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[5] );

               if( rda_alarm_sum & 0x40 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[6] );

               if( rda_alarm_sum & 0x80 )
                  fprintf( stderr, "--->                          %s\n", alarm_sum[7] );

            }

            break;

         }

         case RS_COMMAND_ACK:
         {

            /* Process Command Acknowledgement. */
            if( rda_command_ack == 1 )
               fprintf( stderr, "--->RDA Command Ack:          %s\n", cmd_ack[0] );

            else if( rda_command_ack == 2 )
               fprintf( stderr, "--->RDA Command Ack:          %s\n", cmd_ack[1] );

            else if( rda_command_ack == 3 )
               fprintf( stderr, "--->RDA Command Ack:          %s\n", cmd_ack[2] );

            else if( rda_command_ack == 4 )
               fprintf( stderr, "--->RDA Command Ack:          %s\n", cmd_ack[3] );

            break;
         }
 
         case RS_CHAN_CONTROL_STATUS:
         {

            int i = 0;

            if ( (stat = DEAU_get_values("Redundant_info.redundant_type", 
                                         &deau_ret_val, 1)) >= 0){

               /* Process channel control status if FAA Redundant. */
               if( (int) deau_ret_val != ORPGSITE_FAA_REDUNDANT )
                  break;
               else if( chan_stat == RDA_IS_CONTROLLING )
                  i = 0;
               else if( chan_stat == RDA_IS_NON_CONTROLLING )
                  i = 1;
               else
               {
                  /* Unknown value.  Place value in status buffer. */
                  i = 2;
                  sprintf( channel_status[i], "%6d", chan_stat );
               }
            }
            else
              fprintf( stderr, "call to DEAU_get_values returned error.\n" );

            fprintf( stderr, "--->Chan Cntrl:               %s\n", channel_status[i] );
            break;

         }

         case RS_SPOT_BLANKING_STATUS:
         {

            int i;

            /* If spot blanking not installed, break. */
            if( spot_blank_stat == SB_NOT_INSTALLED )
               break;

            /* Process spot blanking status. */
            if( spot_blank_stat == SB_ENABLED )
               i = 1;

            else if( spot_blank_stat == SB_DISABLED )
               i = 2;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 3;
               sprintf( spot_blanking[i], "%6d", spot_blank_stat );

            }

            fprintf( stderr, "--->Spot Blank:               %s\n", spot_blanking[i] );
            break;

         }
         case RS_TPS_STATUS:
         {

            int i;

            /* If TPS not installed, break. */
            if( tps_stat == TP_NOT_INSTALLED )
               break;

            /* Process TPS status. */
            if( tps_stat == TP_OFF )
               i = 0;

            else if( tps_stat == TP_OK )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( tps[i], "%6d", tps_stat );

            }

            fprintf( stderr, "--->TPS:                      %s\n", tps[i] );
            break;

         }

         case RS_BPM_GEN_DATE:
         {

            int yr, mo, dy, hr, m, s;
            time_t bpm_time = (bpm_gen_date-1)*86400 + bpm_gen_time*60;
            
            unix_time( &bpm_time, &yr, &mo, &dy, &hr, &m, &s );
            if( yr >= 2000 )
               yr -= 2000;
            else
               yr -= 1900;

            fprintf( stderr, "--->BPM Gen Time:             %02d:%02d:%02d %02d/%02d/%02d\n",
                     hr, m, s, mo, dy, yr );
            break;

         }

         case RS_NWM_GEN_DATE:
         {

            int yr, mo, dy, hr, m, s;
            time_t clm_time = (clm_gen_date-1)*86400 + clm_gen_time*60;
            
            unix_time( &clm_time, &yr, &mo, &dy, &hr, &m, &s );
            if( yr >= 2000 )
               yr -= 2000;
            else
               yr -= 1900;

            fprintf( stderr, "--->CLM Gen Time:             %02d:%02d:%02d %02d/%02d/%02d\n",
                     hr, m, s, mo, dy, yr );
            break;

         }

         case RS_VC_REFL_CALIB_CORRECTION:
         {

            /* Process Vertical dBZ0. */
            fprintf( stderr, "--->V dBZ0:                   %7.2f\n", (float) v_ref_dBZ0 / 100.0 );
            break;

         }

         case RS_RMS_CONTROL_STATUS:
         {

            /* Process RMS Control status. */
            if( rms_stat == 2 )
               fprintf( stderr, "--->RMS Cntrl Status:         %s\n", rms[0] );

            else if( rms_stat == 4 )
               fprintf( stderr, "--->RMS Cntrl Status:         %s\n", rms[1] );

            break;

         }

         case RS_PERF_CHECK_STATUS:
         {

            int i;

            /* Process Performance Check status. */
            if( perf_check_status == PC_AUTO )
               i = 0;

            else if( perf_check_status == PC_PENDING )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( perf_check[i], "%6d", perf_check_status );

            }

            fprintf( stderr, "--->Perf Check:               %s\n", perf_check[i] );
            break;

         }
         default:
            break;

      /* End of "switch" statement. */
      }

   /* End of "for" loop. */
   }

   /* Put some blank lines .... */
   fprintf( stderr, "\n\n" );

} 

