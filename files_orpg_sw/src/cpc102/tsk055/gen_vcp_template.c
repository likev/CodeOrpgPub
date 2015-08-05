/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/23 21:24:42 $
 * $Id: gen_vcp_template.c,v 1.4 2007/02/23 21:24:42 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/


#include <orpgvcp.h>
#include <orpg.h>
#include <pulse_counts.h>
#include <write_vcp_template.h>

int Verbose_mode = 0;

/* Function prototypes. */
static int General_info( int *pattern_num, int *wx_mode, 
                         int *num_elev_cuts, int *where_defined );
static int Get_command_line_options( int argc, char *argv[] );
static void Get_elevation_info( int cut_num, float *elev_ang_deg, int *waveform, 
                                int *phase, float *scan_rate_deg, int *def_surv_pri, 
                                int *def_surv_p_cnt, int *def_dop_pri, int *def_dop_p_cnt );

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      This is a simple tool to assist in defining a VCP template to be 
//      read by mnttsk_vcp.  
//
//      This tools queries the operator for basic things like:
//  
//         -- VCP number
//         -- Number of elevation scans
//         -- Weather Mode
//         -- Where is this VCP going to be defined at
//
//      For each elevation cut defined, the operator must provide the following:
//
//         -- Elevation angle
//         -- Waveform type
//         -- Scan Rate or scan period
//
//      For waveform type CS or BATCH, the operator must provide the surveillance
//      PRI.  For CS waveform, this program automatically computes the number of 
//      surveillance pulses based on predefined PRT values and operator supplied
//      scan rate.  For BATCH, the operator must provide the number of surveillance
//      pulses.  
//
//      For waveform type CD/W, CD/WO, and BATCH the operator must provide the 
//      Doppler PRI.  This program automatically computes the number of Doppler 
//      pulses based on predefined PRT values, operator supplied scan rate, and 
//      operator supplied surveillance PRI and number of pulses in the case of
//      BATCH.
//
//      The VCP template file is written to file vcp_xx in the current directory
//      where xx is the VCP number supplied to the program by the operator.
//
//////////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char* argv[] ){

   int pattern_num;
   int wx_mode;
   int num_elev_cuts;
   int where_defined;
   int waveform, phase;
   int i, ret;
   int num_alwb_prfs = 5;
   int alwb_prfs[] = { 4, 5, 6, 7, 8 };

   int where_defined_trans[] = { ORPGVCP_RPG_DEFINED_VCP, ORPGVCP_RPG_DEFINED_VCP, 
                                 (ORPGVCP_RPG_DEFINED_VCP + ORPGVCP_RDA_DEFINED_VCP) };
   Cut_info_t cut_info;
   Vol_info_t vol_info;


   /* Get command line options, if any. */
   if( (ret = Get_command_line_options( argc, argv )) < 0 ){

      fprintf( stderr, "Command Line Options Failed\n" );
      exit(0);

   }

   /* Prompt the user for inputs to the VCP_attr section. */
   General_info( &pattern_num, &wx_mode, &num_elev_cuts, &where_defined );

   /* Construct and open the file to contain the VCP information. */
   if( Get_filename( pattern_num ) == NULL ){

      fprintf( stderr, "Unable to Open File to hold VCP data\n" );
      exit(1);

   }

   /* Write out the VCP_attr section. */
   vol_info.pattern_num = pattern_num;
   vol_info.wx_mode = wx_mode;
   vol_info.num_elev_cuts = num_elev_cuts;
   vol_info.where_defined = where_defined_trans[ where_defined ];
   vol_info.pulse_width = ORPGVCP_SHORT_PULSE;
   vol_info.cluttermap_group = 1;
   vol_info.vel_reso = 0.5;

   Write_VCP_attr( &vol_info );

   /* Do For All elevation cuts. */
   for( i = 0; i < num_elev_cuts; i++ ){

      int flag;

      /* Initialize the elevation attributes. */
      Init_elevation_attr( &cut_info );

      /* Get the Elevation Attributes. */
      Get_elevation_info( i+1, &cut_info.elev_ang_deg, &waveform, &phase,  
                          &cut_info.scan_rate_deg, &cut_info.def_surv_prf, 
                          &cut_info.def_surv_p_cnt, &cut_info.def_dop_prf,
                          cut_info.def_dop_p_cnt );

      cut_info.phase = phase;
      cut_info.waveform = waveform;
      cut_info.cut_num = i;

      /* Write the Elevation Attributes. */
      if( i == 0 )
         flag = START_ELEV_ATTR;

      else if( i == num_elev_cuts-1 )
         flag = END_ELEV_ATTR;

      else
         flag = NO_ACTION;

      Write_elevation_attr( flag, &cut_info, num_alwb_prfs, alwb_prfs ); 

   } /* End of "For All Elevation Cuts" loop. */

   return 0;

}

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Gathers general VCP information.
//
//   Outputs:
//      pattern_num - VCP number.
//      wx_mode - Weather Mode (1 - Clear Air, 2 - Precipitation).
//      num_elev_cuts - Number of RDA elevation cuts.
//      where_defined - Where the VCP is defined (1-RPG Only, 2-RPG and RDA).
//
//////////////////////////////////////////////////////////////////////////\*/
static int General_info( int *pattern_num, int *wx_mode, int *num_elev_cuts, int *where_defined ){

   int response;
   int ret;

   fprintf( stdout, "\n\nYou will be asked some general VCP questions\n\n" );

   while(1){

      /* Prompt the user for inputs to the VCP_attr section. */
      fprintf( stdout, "--->Enter the Pattern Number\n" );
      scanf( "%d", pattern_num );

      fprintf( stdout, "--->Enter Wx Mode (1 - Clear Air, 2 - Precipitation)\n" );
      scanf( "%d", wx_mode );

      fprintf( stdout, "--->Enter the Number of Elevation Cuts\n" );
      scanf( "%d", num_elev_cuts );

      fprintf( stdout, "--->Enter Where the VCP Will be Defined (1 - RPG only, 2 - RPG and RDA)\n" );
      scanf( "%d", where_defined );

      /* Validate the user inputs. */
      if( (*pattern_num < 1) || (*pattern_num > 255) ){

         fprintf( stderr, "Pattern # %d Must Be in Range 0 < VCP <= 255\n",
                  *pattern_num );
         continue;

      }

      if( (*wx_mode < 1) || (*wx_mode > 2) ){

         fprintf( stderr, "Wx Mode Must be Either 1 (Clear Air) or 2 (Precipitation)\n" );
         continue;

      }

      if( (*num_elev_cuts < 1) || (*num_elev_cuts > 25) ){

         fprintf( stderr, "Number Elevation Cuts %d Must be Between 1 and 25\n",
                  *num_elev_cuts );
         continue;

      }

      if( (*where_defined < 1) || (*where_defined > 2) ){

         fprintf( stderr, "Where Defined %d Must be Either 1 (RPG Only) or 2 (RPG and RDA)\n",
                  *where_defined );
         continue;

      }

      fprintf( stdout, "\nYou Entered the Following Information:\n" );
      fprintf( stdout, "--->Pattern Number: %d\n", *pattern_num );

      if( *wx_mode == 1 )
         fprintf( stdout, "--->Weather Mode:   1 - Clear Air\n" );
      else
         fprintf( stdout, "--->Weather Mode:   2 - Precipitation\n" );

      fprintf( stdout, "---># Elevations:   %d\n", *num_elev_cuts );

      if( *where_defined == 1 )
         fprintf( stdout, "--->Where Defined:  1 - RPG Only\n" );
      else
         fprintf( stdout, "--->Where Defined:  2 - RPG and RDA\n" );

      fprintf( stdout, "\nEnter 1 if Correct, 0 if Not Correct\n" );
      ret = scanf( "%d", &response );

      if( response == 1 )
         break;

   } /* End of while() loop. */

   return (0);

} /*End of General_info() */

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Process command line arguments.
//
//   Inputs:
//      argc - number of command line arguments.
//      argv - the command line arguments.
//
//   Returns:
//      exits on error, or returns 0 on success.
//
///////////////////////////////////////////////////////////////////////////\*/
static int Get_command_line_options( int argc, char *argv[] ){

   extern char *optarg;
   extern int optind;
   int c, err;
  
   Verbose_mode = 0;

   err = 0;
   while( (c = getopt (argc, argv, "hv")) != EOF ){

      switch (c) {

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

   if( err == 1 ){              /* Print usage message */

      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h Help (print usage msg and exit)\n");
      printf ("\t\t-v Verbose Mode\n");
      exit (1);

   }              
                     
   return(0);
                  
} /* End of Get_command_line_options() */

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Gathers elevation information.
//
//   Inputs:
//      cut_num - Elevation cut number.
//
//   Outputs:
//      elev_ang_deg - Elevation angle, in degrees.
//      waveform - Waveform type (1-CS, 2-CD/W, 3-CD/WO, 4-BATCH).
//      phase - Constant or SZ2.
//      scan_rate_deg - Antenna scan rate, deg/s.
//
//////////////////////////////////////////////////////////////////////////\*/
static void Get_elevation_info( int cut_num, float *elev_ang_deg, int *waveform, 
                                int *phase, float *scan_rate_deg, int *def_surv_pri, 
                                int *def_surv_p_cnt, int *def_dop_pri, int def_dop_p_cnt[] ){

   int response, ret, i, option;
   float scan_rate_s, value;

   *def_surv_pri = 0;
   *def_surv_p_cnt = 0;
   *def_dop_pri = 0;

   for( i = 0; i <= MAX_ALWB_PRI; i++ )
      def_dop_p_cnt[i] = 0;

   if( cut_num == 1 )
      fprintf( stdout, "\n\nYou will be asked to enter general information\nabout the elevation cut data for the VCP\n" );

   while(1){
      
      /* Prompt the user for inputs. */
      fprintf( stdout, "\n--->Enter Elevation Angle of Cut %d (deg)\n", cut_num );
      scanf( "%f", elev_ang_deg );
         
      fprintf( stdout, "--->Enter Waveform (1 - CS, 2 - CD/W, 3 - CD/WO, 4 - BATCH)\n" );
      scanf( "%d", waveform );

      *phase = 0;
      if( (*waveform == 1) || (*waveform == 2) ){

         fprintf( stdout, "--->Enter Phase (0 - Constant, 2 - SZ2)\n" );
         scanf( "%d", phase );

      }

      while(1){

         fprintf( stdout, "--->Enter 1,Rate (deg/s) or 2,Scan Period (s)\n" );
         ret = scanf( "%d,%f", &option, &value );
         if( ret < 2 ){

           fprintf( stderr, "scanf returned %d\n", ret );
           fflush( stdin );
           continue;

         }

         if( option == 1 ){

            *scan_rate_deg = value;
            scan_rate_s = 360.0 / *scan_rate_deg;
            break;

         }
         else if( option == 2 ){

            scan_rate_s = value;
            *scan_rate_deg = 360.0 / scan_rate_s;
            break;

         }
         else{

            fprintf( stdout, "Only options 1 or 2 are accepted\n" );
            continue;

         }

      }

      /* Validate all user inputs. */
      if( (*elev_ang_deg < -1.0) || (*elev_ang_deg > 45.0) ){

         fprintf( stderr, "Elevation Angle %f Must be Between -1.0 <= Elev <= 45.0\n",
                  *elev_ang_deg );
         continue;
      
      }
      
      if( (*waveform < 1) || (*waveform > 4) ){

         fprintf( stderr,
              "Waveform %d Must be 1 - CS, 2 - CD/W, 3 - CD/WO or 4 - BATCH\n", *waveform );
         continue;

      }

      if( (*phase != 0) && (*phase != 2) ){

         fprintf( stderr,
              "Phase %d Must be 0 - CONSTANT, 2 - SZ2\n", *phase );
         continue;

      }

      if( (*scan_rate_deg <= 0.0) || (*scan_rate_deg > 30.0) ){

         if( option == 1 ){

            fprintf( stderr, "Scan Rate %f (deg/s) Must be Between 0.0 < Rate <= 30.0\n",
                     *scan_rate_deg );
            continue;

         }

         if( option == 2 ){

            fprintf( stderr, "Scan Period  %f (s) Must be Between 0 < Period <= 12\n",
                     scan_rate_s );
            continue;

         }

      }

      fprintf( stdout, "\nYou Entered the Following Information:\n" );
      fprintf( stdout, "--->Elevation Angle: %5.2f deg\n", *elev_ang_deg );

      if( *waveform == 1 )
         fprintf( stdout, "--->Waveform:     1 - CS\n" );
      else if( *waveform == 2 )
         fprintf( stdout, "--->Waveform:     2 - CD/W\n" );
      else if( *waveform == 3 )
         fprintf( stdout, "--->Waveform:     3 - CD/WO\n" );
      else if( *waveform == 4 )
         fprintf( stdout, "--->Waveform:     4 - BATCH\n" );

      if( *phase == 0 )
         fprintf( stdout, "--->Phase:        0 - CONSTANT\n" );
      else if( *phase == 2 )
         fprintf( stdout, "--->Phase:        2 - SZ2\n" );
   
      if( option == 1 )
         fprintf( stdout, "--->Scan Rate:    %6.3f deg/s\n", *scan_rate_deg );
      else if( option == 2 )
         fprintf( stdout, "--->Scan Period:  %6.3f s\n", scan_rate_s );
   
      fprintf( stdout, "\nEnter 1 if Correct, 0 if Not Correct\n" );
      ret = scanf( "%d", &response );

      if( response == 1 )
         break;

   }

   if( (*waveform == 1 /* CS */) || (*waveform == 4 /* BATCH */) ){

      while(1){

         fprintf( stdout, "--->Enter the Default Surveillance PRI\n" );
         scanf( "%d", def_surv_pri ); 

         if( (*def_surv_pri < MIN_SURV_PRI) || (*def_surv_pri > MAX_SURV_PRI) ){

            fprintf( stderr, "Surveillance PRI Must be in Range %d <= PRI <= %d\n",
                     MIN_SURV_PRI, MAX_SURV_PRI );
            continue;

         }

         break;

      } /* End of "while()" loop. */

      if( *waveform == 4 ){

         fprintf( stdout, "--->Enter the Number of Surveillance Pulses\n" );
         scanf( "%d", def_surv_p_cnt );

      }
      else
         *def_surv_p_cnt = Pulse_cnt_CS( *scan_rate_deg, *def_surv_pri );
      

   }

   if( (*waveform == 2 /* CS/W */) 
                  || 
       (*waveform == 3 /* CS/WO */)
                  || 
       (*waveform == 4 /* BATCH */) ){

      while(1){

         fprintf( stdout, "--->Enter the Default Doppler PRI\n" );
         scanf( "%d", def_dop_pri ); 

         if( (*def_dop_pri < MIN_DOP_PRI) || (*def_dop_pri > MAX_DOP_PRI) ){

            fprintf( stderr, "Doppler PRI Must be in Range %d <= PRI <= %d\n",
                     MIN_DOP_PRI, MAX_DOP_PRI );
            continue;

         }

         break;

      } /* End of "while()" loop. */

      for( i = MIN_DOP_PRI; i <= MAX_DOP_PRI; i++ ){

         if( (*waveform == 2) || (*waveform == 3) )
            def_dop_p_cnt[i] = Pulse_cnt_CD( *scan_rate_deg, i );
         else
            def_dop_p_cnt[i] = Pulse_cnt_BATCH( *scan_rate_deg, *def_surv_pri,
                                                *def_surv_p_cnt, i );

      }

   }

} /* End of Get_elevation_info() */

