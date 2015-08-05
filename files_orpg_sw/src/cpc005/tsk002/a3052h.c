/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/06/25 18:46:19 $
 * $Id: a3052h.c,v 1.43 2007/06/25 18:46:19 ccalvert Exp $
 * $Revision: 1.43 $
 * $State: Exp $
 */
#define A3052H_C
#include <pcipdalg.h>
#include <epre_main.h>
#include <epreConstants.h>

#define REFMIN               2
#define REFMAX             255
#define PI                   3.14159
#define ALLAZ              360.0
#define BINCNST1            10.0
#define BINCNST2     (BINCNST1*BINCNST1) 
#define BINCNST3     (2.0*BINCNST1)

#define NUMBINS            230

#define EVERY_8_HOURS        8

/* Static global variables. */
static Mode_select_entry_t Prctbl;
static double Pcparea;
static float Bintbl[NUMBINS];

/* Global variables. */
extern int Previous_wxstatus;

/* Status Function Prototypes() */
static void Format_MSF_SETUP();
static void Format_MSF_STATUS( int area, float zthresh, float total_hours, 
                               int vcp, char *mode );
static void Force_wxmode_change( int mode, int vcp, time_t delta_time, 
                                 time_t current_time, int *wxstatus_deselect );

/*\///////////////////////////////////////////////////////////////////////

   Description:
      This function makes a local copy of the "Mode Selection" adaptation
      data and beamwidth from the Hydromet Prep adaptation data.

      This function also builds the range bin look-up table when necessary.

   Returns:
      Currently the return value is unused.  This function always returns
      0.

///////////////////////////////////////////////////////////////////////\*/
int A3052E_init_get_adapt( ){

   int changed, year, month, day, hour, minute, second, n;
   time_t curr_time;
   static int last_hour = 0, first_time = 1, time_to_update = 0;
   float rng, bincnst, bincnst4, bincnst5;
   static float wdthbeam = 0.0;
 
   /* Tell user what the adaptation data values are: */
   changed = 0;
   if( (Newtbl.precip_mode_zthresh != Mode_select.precip_mode_zthresh) 
                            ||
       (Newtbl.precip_mode_area_thresh != Mode_select.precip_mode_area_thresh)
                            ||
       (Newtbl.auto_mode_A != Mode_select.auto_mode_A) 
                            ||
       (Newtbl.auto_mode_B != Mode_select.auto_mode_B) 
                            ||
       (Newtbl.mode_B_selection_time != Mode_select.mode_B_selection_time) 
                            ||
       (Newtbl.ignore_mode_conflict != Mode_select.ignore_mode_conflict)
                            ||
       (Newtbl.mode_conflict_duration != Mode_select.mode_conflict_duration) ){

      RPGC_log_msg( GL_INFO, "Mode Selection Algorithm Adaptable Parameters:\n" );
      RPGC_log_msg( GL_INFO, "--->Precip Mode Z Threshold: %6.1f  Precip Mode Area Threshold: %6d\n",
                    Mode_select.precip_mode_zthresh, Mode_select.precip_mode_area_thresh );

      RPGC_log_msg( GL_INFO, "--->Auto Mode A: %2d  Auto Mode B: %2d\n",
                    Mode_select.auto_mode_A, Mode_select.auto_mode_B );

      RPGC_log_msg( GL_INFO, "--->Ignore Mode Conflict: %2d  Mode Conflict Duration: %6d\n",
                    Mode_select.ignore_mode_conflict, 
                    Mode_select.mode_conflict_duration );

      RPGC_log_msg( GL_INFO, "--->Mode B Selection Time: %6d\n",
                    Mode_select.mode_B_selection_time );

      changed = 1;

   }
 
   /* Make local copy of VCP select adaptation data. */
   Newtbl.precip_mode_zthresh = Mode_select.precip_mode_zthresh;
   Newtbl.precip_mode_area_thresh = Mode_select.precip_mode_area_thresh;
   Newtbl.auto_mode_A = Mode_select.auto_mode_A;
   Newtbl.auto_mode_B = Mode_select.auto_mode_B;
   Newtbl.mode_B_selection_time = Mode_select.mode_B_selection_time;
   Newtbl.ignore_mode_conflict = Mode_select.ignore_mode_conflict;
   Newtbl.mode_conflict_duration = Mode_select.mode_conflict_duration;

   /* Determine if it is time to output a MSF SETUP message. */
   curr_time = time(NULL);
   unix_time( &curr_time, &year, &month, &day, &hour, &minute, &second );
   if( ((hour % EVERY_8_HOURS) == 1) && (last_hour != hour) ){

      if( time_to_update == 0 ){

         time_to_update = 1;
         last_hour = hour;

      }

   }
   else
      time_to_update = 0;

   /* Format MSF SETUP message if adaptable parameters changed or data
      needs to be output. */
   if( (changed) || (first_time) || (time_to_update) ){

      first_time = 0;

      if( time_to_update )
         time_to_update = 0;

      Format_MSF_SETUP();

   }
 
   /* If the beam width has changed, recalculate Bintbl. */
   if( wdthbeam != Hydromet_prep.beam_width ){

      wdthbeam = Hydromet_prep.beam_width;
      RPGC_log_msg( GL_INFO, "--->Beam Width: %6.1f\n", wdthbeam );
 
      /* Compute Bin Area table (in tenths of kilometers squared). */
      bincnst = (PI*wdthbeam)/ALLAZ;
      bincnst4 = bincnst * BINCNST2;
      bincnst5 = bincnst * BINCNST3;
      rng = 0.0;
 
      /* For all bins out to 230 km.... NOTE:  Since the hybrid scan
         product is 1km, we assume 1 km also, regardless of the native
         resolution of the base reflectivity. */
      for( n = 0; n < NUMBINS; n++ ){

         Bintbl[n] = bincnst4 + (rng*bincnst5);
         rng += BINCNST1;

      }

   }

   return 0;

} /* End of A3052E_init_get_adapt(). */

#define  CNVRATIO   2.0
#define  ZBIAS     66.0

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Computes derived adaptation data.

///////////////////////////////////////////////////////////////////////\*/
int A3052F_comp_tables( ){

   static int old_refthr;

   /* Precip Threshold (converted to reflectivity in scaled biased units ). */
   Prctbl.precip_mode_zthresh = CNVRATIO*Newtbl.precip_mode_zthresh + ZBIAS;

   /* Check for max and min reflec. values out of range. */
   if( Prctbl.precip_mode_zthresh > REFMAX ) Prctbl.precip_mode_zthresh = REFMAX;
   if( Prctbl.precip_mode_zthresh < REFMIN ) Prctbl.precip_mode_zthresh = REFMIN;

   if( Newtbl.precip_mode_zthresh != old_refthr ){ 

      RPGC_log_msg( GL_INFO, "Reflectivity Threshold: %6.1f (%6.1f)", 
                    Newtbl.precip_mode_zthresh, Newtbl.precip_mode_zthresh );
      old_refthr = Newtbl.precip_mode_zthresh;

   }
 
   /* Create new threshold for significant rain area testing (units: 10ths of kms sq). */
   Prctbl.precip_mode_area_thresh =  Newtbl.precip_mode_area_thresh*100;
 
   /* Copy remaining adaptation data. */
   Prctbl.auto_mode_A = Newtbl.auto_mode_A;
   Prctbl.auto_mode_B = Newtbl.auto_mode_B;
   Prctbl.mode_B_selection_time = Newtbl.mode_B_selection_time;
   Prctbl.ignore_mode_conflict = Newtbl.ignore_mode_conflict;
   Prctbl.mode_conflict_duration = Newtbl.mode_conflict_duration;

   return 0;

} /* End of A3052F_comp_tables(). */

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Processes the hybrid scan data.   Sums up the bin area where 
      reflectivity meets or exceeds dBZ threshold. 

   Inputs:
      hybrscan - hybrid scan data.

   Outputs:
      area - precipitation area, in km^2.

   Returns:
      The return value is currently unused.  This function always returns
      0.

///////////////////////////////////////////////////////////////////////\*/
int A3052G_chekrate_sumareas( int *hybrscan, float *area ){

   int i, j;

   EPRE_buf_t *refl = (EPRE_buf_t *) hybrscan;
 
   /* Initialize area detected to 0. */
   Pcparea = 0.0;
 
   /* Do For Each radial in hybrid scan. */
   for( j = 0; j < MAX_AZM; j++ ){
 
      /* Do For Each reflectivity bin in hybrid scan. */
      for( i = 0; i < MAX_RNG; i++ ){
 
         if( (refl->HyScanZ[j][i] >= Prctbl.precip_mode_zthresh)
                               &&
             (refl->HyScanZ[j][i] != RDMSNG) ){
 
            /* Add current bin's area to the total precip
               area over this threshold. */
            Pcparea += Bintbl[i];

         }

      }

   }

   /* Tell the operator what detected area. */
   *area = (float) (Pcparea / 100.0);
   RPGC_log_msg( GL_INFO, "Precipitation Area Detected: %8.1f (km^2) ", *area );

   return 0;

} /* End of A3052G_chekrate_sumareas(). */

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Assigns precipitation category based on thresholds.  Updates ITC 
      a3052t.

   Inputs:
      catflag - the current precipitation category.

///////////////////////////////////////////////////////////////////////\*/
int A3052H_precip_cats( int *catflag ){

   int add_time, time_diff;

   /* Initialize Precip-Category-Found flag. */
   *catflag = CTGRY0;
 
   /* Check for actual precip area exceeding the threshold area. */
   if( Pcparea >= (float) Prctbl.precip_mode_area_thresh ){
 
      /* Actual precip area for this precip rate exceeds area threshold.
         Set flag indicating Precip Category 1 found and set time & date for
         it to be in effect to be DESELECT_TIME minutes past the present time 
         & date.  If the Auto Mode A flag is not set (the Wx Selection algorithm
         will not automatically select Mode A when significant rain occurs), 
         then set time and date to be the present time and date. */
      if( Prctbl.auto_mode_A == 0 )
         add_time = 0;

      else
         add_time = Prctbl.mode_B_selection_time * SEC_IN_MIN;

      *catflag = CTGRY1;
      Mode_select_status.a3052t.time_to_cla = Mode_select_status.a3052t.curr_time + add_time;
 
      /* Set last time & date precip mode area exceeded to present time & date. */
      Mode_select_status.a3052t.last_time = Mode_select_status.a3052t.curr_time;

      RPGC_log_msg( GL_INFO, "Precipitation Area >= Threshold Area\n" );
 
   }
   else{

      /* Reset the time to Clear Air if the "Mode_B_selection_time" has changed or
         the time to Clear Air is greater than the current time plus "Mode_B_selection_time".
         In both cases, set the time to Clear Air to the current time plus 
         "Mode_B_selection_time". */
      if( Prctbl.auto_mode_A ){ 

         if( (Mode_B_selection_time != Prctbl.mode_B_selection_time)
                                    ||
             (Mode_select_status.a3052t.time_to_cla > (Mode_select_status.a3052t.curr_time +
                                                      (Prctbl.mode_B_selection_time * SEC_IN_MIN))) ){

            /* Mode_B_selection_time has changed. */
            if( Mode_B_selection_time != Prctbl.mode_B_selection_time )
               RPGC_log_msg( GL_INFO, "auto_mode_A and mode_B_selection_time Changed\n" );

            else
               RPGC_log_msg( GL_INFO, "auto_mode_A and time_to_cla Too Far in the Future\n" );
         
            RPGC_log_msg( GL_INFO, ">>> Original: Current Time: %d, Time to Clear Air: %d\n",
                          Mode_select_status.a3052t.curr_time, Mode_select_status.a3052t.time_to_cla ); 

            /* Only set a new time to Clear Air if the current time to Clear Air is greater
               than the current time. */
            if( Mode_select_status.a3052t.time_to_cla > Mode_select_status.a3052t.curr_time ){

               Mode_select_status.a3052t.time_to_cla = Mode_select_status.a3052t.curr_time +
                                                       (Prctbl.mode_B_selection_time * SEC_IN_MIN);
               RPGC_log_msg( GL_INFO, ">>> New: Current Time: %d, Time to Clear Air: %d\n",
                              Mode_select_status.a3052t.curr_time, Mode_select_status.a3052t.time_to_cla ); 

            }

         }

      }

      /* Save the mode_B_selection_time */
      Mode_B_selection_time = Prctbl.mode_B_selection_time;

      /* If the time to Clear Air is prior to current time, set the time to Clear Air to 
         the current time.  If the Auto Mode A flag is not set, then the user can go to 
         Clear Air mode now. */
      if( (Mode_select_status.a3052t.time_to_cla < Mode_select_status.a3052t.curr_time)
                                     ||
                            (Prctbl.auto_mode_A == 0) ){

         Mode_select_status.a3052t.time_to_cla = Mode_select_status.a3052t.curr_time;
         RPGC_log_msg( GL_INFO, "Setting Time to Clear Air to Current Time: %d\n",
                        Mode_select_status.a3052t.time_to_cla );

      }

      RPGC_log_msg( GL_INFO, "Precipitation Area < Threshold Area\n" );

   }

   time_diff = Mode_select_status.a3052t.time_to_cla - Mode_select_status.a3052t.curr_time;
   RPGC_log_msg( GL_INFO, "--->Wx Status B Can Be Selected in %3d minutes. \n", 
                 time_diff / SEC_IN_MIN );

   return 0;

} /* A3052H_precip_cats(). */

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Sends a RDA control command to change weather mode.  
      Fills in the Precipitation Status Message elements.
      Fills in the VCP Selection Status elements.

   Inputs:
      outbuf - Output buffer to hold Precipitation Status Message.
      catflfag - the current precipitation category.
      vcp_info - Contains various volume scan/VCP information.

   Outputs:
      wxstatus_deselect - if set, indicates Wx Mode deselect

///////////////////////////////////////////////////////////////////////\*/
int A3052I_setmode_setobuf( Prcipmsg_t *outbuf, int catflag, Vcpinfo_t *vcp_info,
                            int *wxstatus_deselect ){

   int test_mode, year, month, day, hour, minute, second;
   time_t current_time, delta_time;
   float total_hours; 

   /* Initialize the flags which indicate whether a Wx Mode change is
      commanded (or not).  The initialize value denotes "not commanded". */ 
   *wxstatus_deselect = 0;

   /* Set the current VCP in operation in the VCP Select Status. */
   Mode_select_status.current_vcp = vcp_info->rpgvcp;

   /* Determine the current time. */
   current_time = Mode_select_status.a3052t.curr_time;

   /* If current time is less than time to Clear Air, set Precip Category to 1. */
   if( Mode_select_status.a3052t.curr_time < Mode_select_status.a3052t.time_to_cla ) 
      Mode_select_status.a3052t.pcpctgry = CTGRY1;

   else
      Mode_select_status.a3052t.pcpctgry = catflag;

   /* Check if we are in test mode.  If so, we prevent automatic switchovers. */
   test_mode = ORPGINFO_is_test_mode();
 
   /* Call routine to change weather mode to `Convective' if it is `Clear Air' 
      and Precip Category is #1 and 'Auto Mode A' is active. */
   if( Mode_select_status.a3052t.pcpctgry == CTGRY1 ){
 
      Mode_select_status.wxstatus_status = MODE_SELECT_NORMAL;
      Mode_select_status.current_wxstatus = vcp_info->rpgwmode;

      if( (Mode_select_status.recommended_wxstatus != PFWXCONV)
                                 ||
          (Mode_select_status.recommended_wxstatus_start_time == 0) )
         Mode_select_status.recommended_wxstatus_start_time = current_time;

      Mode_select_status.recommended_wxstatus = PFWXCONV;

      if( vcp_info->rpgwmode == PFWXCONV ){

         Mode_select_status.conflict_start_time = 0;
         total_hours = -1.0;
         delta_time = 0;

      }
      else{

         Mode_select_status.wxstatus_status = MODE_SELECT_CONFLICT;

         /* Set the conflict start time if not already set. */
         if( Mode_select_status.conflict_start_time == 0 )
            Mode_select_status.conflict_start_time = current_time;

         total_hours = 
            ((float) (current_time - Mode_select_status.conflict_start_time)) / SEC_IN_HOUR; 
         delta_time = (int) total_hours;

      }

      /* Write out MSF Status. */
      Format_MSF_STATUS( (int) (Pcparea/100.0), Newtbl.precip_mode_zthresh, 
                         total_hours, vcp_info->rpgvcp, "PRECIP" );

      /* Try and force a weather mode change. */
      if( vcp_info->rpgwmode != PFWXCONV ){

         /* If Auto Wx Mode A is set or the time span since Wx Mode conflict started
            is greater than threshold and the RPG is not in Test Mode, force Wx Mode change. */
         if( ((Prctbl.auto_mode_A == 1) 
                                ||
             ((!Prctbl.ignore_mode_conflict) && (delta_time >= Prctbl.mode_conflict_duration)))
                  &&
             (!test_mode) )
            Force_wxmode_change( PFWXCONV, vcp_info->rpgvsnum, delta_time, 
                                 current_time, wxstatus_deselect );

      } 

   } 
 
   /* Call routine to change weather mode to 'Clear Air' if it is 'Precipitation' 
      and Precip Category is #0 and 'Auto Mode B' is active. */
   if( Mode_select_status.a3052t.pcpctgry == CTGRY0 ){

      Mode_select_status.wxstatus_status = MODE_SELECT_NORMAL;
      Mode_select_status.current_wxstatus = vcp_info->rpgwmode;

      if( (Mode_select_status.recommended_wxstatus != PFWXCLA)
                                 ||
          (Mode_select_status.recommended_wxstatus_start_time == 0) )
         Mode_select_status.recommended_wxstatus_start_time = current_time;

      Mode_select_status.recommended_wxstatus = PFWXCLA;

      if( vcp_info->rpgwmode == PFWXCLA ){

         Mode_select_status.conflict_start_time = 0;
         total_hours = -1.0;
         delta_time = 0;

      }
      else{

         Mode_select_status.wxstatus_status = MODE_SELECT_CONFLICT;

         /* Set the conflict start time if not already set. */
         if( Mode_select_status.conflict_start_time == 0 )
            Mode_select_status.conflict_start_time = current_time;

         total_hours = 
            ((float) (current_time - Mode_select_status.conflict_start_time)) / SEC_IN_HOUR; 
         delta_time = (int) total_hours;

      }

      /* Write out MSF Status. */
      Format_MSF_STATUS( (int) (Pcparea/100.0), Newtbl.precip_mode_zthresh, 
                         total_hours, vcp_info->rpgvcp, "CLEAR AIR" );

      /* Try and force a weather mode change. */
      if( vcp_info->rpgwmode != PFWXCLA ){

         /* If Auto Wx Mode B is set or the time span since Wx Mode conflict started
            is greater than threshold, force Wx Mode change. */
         if( ((Prctbl.auto_mode_B == 1)
                                ||
             ((!Prctbl.ignore_mode_conflict) && (delta_time >= Prctbl.mode_conflict_duration))) 
                  &&
             (!test_mode) )
            Force_wxmode_change( PFWXCLA, vcp_info->rpgvsnum, delta_time, 
                                 current_time, wxstatus_deselect );

      }

   }         

   RPGC_log_msg( GL_INFO, "VCP Selection Status:" );
   RPGC_log_msg( GL_INFO, "--->Status: %d, Current Wx Mode: %d, Recommended Wx Mode: %d\n",
                 Mode_select_status.wxstatus_status, Mode_select_status.current_wxstatus, 
                 Mode_select_status.recommended_wxstatus );

   if( Mode_select_status.conflict_start_time > 0 ){

      unix_time( &Mode_select_status.conflict_start_time, &year, &month, &day, &hour,
                 &minute, &second );
      RPGC_log_msg( GL_INFO, "--->Conflict Start Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   /* Fill the output buffer (i.e. for the Precip Status Message). */
   outbuf->current_date = (Mode_select_status.a3052t.curr_time / SEC_IN_DAY) + 1;
   outbuf->current_time = Mode_select_status.a3052t.curr_time - (outbuf->current_date-1)*SEC_IN_DAY;
 
   outbuf->last_precip_detect_date = (Mode_select_status.a3052t.last_time / SEC_IN_DAY) + 1;
   outbuf->last_precip_detect_time = Mode_select_status.a3052t.last_time -
                                     (outbuf->last_precip_detect_date-1)*SEC_IN_DAY;
 
   outbuf->precip_cat = Mode_select_status.a3052t.pcpctgry;
   outbuf->weather_mode = vcp_info->rpgwmode;
 
   /* Set the operative/inoperative indicator to 0 (not yet implemented). */
   outbuf->inop_flag = 0;
 
   /* If weather mode has changed, set the time when the weather mode changed. */
   if( Mode_select_status.current_wxstatus != Previous_wxstatus )
      Mode_select_status.current_wxstatus_time = current_time;

   Previous_wxstatus = Mode_select_status.current_wxstatus;

   /* Save the current category for next pass. */
   Mode_select_status.a3052t.prectgry = Mode_select_status.a3052t.pcpctgry;

   return 0;

} /* End of A3052I_setmode_setobuf(). */

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Formats the MSF SETUP status log message.

   Inputs:

   Outputs:

///////////////////////////////////////////////////////////////////////\*/
static void Format_MSF_SETUP(){

   char *line = NULL;
   char temp[80];
   int def_vcp;

   /* calloc space for status log message. */
   line = (char *) calloc( 1, 80 );
   if( line == NULL ){

      RPGC_log_msg( GL_ERROR, "calloc Failed for 80 Bytes\n" );
      return;

   }

   /* Add the line header. */
   strcat( line, "MSF SETUP: " );

   /* Add Precipitation Switching flag (A)uto or (M)anual. */
   if( Newtbl.auto_mode_A )
      strcat( line, "PRECIP:A/" );

   else
      strcat( line, "PRECIP:M/" );
 
   
   /* Add default VCP for mode A */
   def_vcp = ORPGSITE_get_int_prop( ORPGSITE_DEF_MODE_A_VCP );
   sprintf( temp, "%3d/", def_vcp );
   strcat( line, temp );

   /* Add the reflectivity threshold. */
   sprintf( temp, "%4.1fdBZ/", Newtbl.precip_mode_zthresh );
   strcat( line, temp );

   /* Add the areal threshold. */
   sprintf( temp, "%5dkm^2,", Newtbl.precip_mode_area_thresh );
   strcat( line, temp );

   /* Add Clear Air Switching flag (A)uto or (M)anual. */
   if( Newtbl.auto_mode_B )
      strcat( line, "CLEAR AIR:A/" );

   else
      strcat( line, "CLEAR AIR:M/" );

   /* Add default VCP for mode A */
   def_vcp = ORPGSITE_get_int_prop( ORPGSITE_DEF_MODE_B_VCP );
   sprintf( temp, "%3d/", def_vcp );
   strcat( line, temp );

  /* Add Mode B Selection Time. */
   sprintf( temp, "%2dmin,", Newtbl.mode_B_selection_time );
   strcat( line, temp );

  /* Add Ignore Mode Conflict flag (Y)es or (N)o. */
  if( Newtbl.ignore_mode_conflict )
      strcat( line, "IMC:Y/" );

   else
      strcat( line, "IMC:N/" );

   /* Add the Mode Conflict Duration. */
   sprintf( temp, "%2dhrs", Newtbl.mode_conflict_duration );
   strcat( line, temp );
   
   /* Write the message to the System Status Log. */
   RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, "%s", line );

   /* Free the line memory. */
   free( line );

} /* End of Format_MSF_SETUP() */


/*\///////////////////////////////////////////////////////////////////////

   Description:
      Formats the MSF STATUS status log message.

   Inputs:
      area - area in km^2 having significant reflectivity
      zthresh - significant reflectivity threshold
      total_hours - total number hours in mode conflict
      vcp - current VCP
      mode - recommended weather mode

   Outputs:

///////////////////////////////////////////////////////////////////////\*/
static void Format_MSF_STATUS( int area, float zthresh, float total_hours, 
                               int vcp, char *mode ){

   if( total_hours >= 0.0 )
      RPGC_log_msg( GL_ERROR | GL_STATUS, 
           "MSF STATUS: %6dkm^2 > %4.1fdBZ, in VCP %d, recommend %s mode for %5.1fhrs",
           area, zthresh, vcp, mode, total_hours );

   else
      RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
           "MSF STATUS: %6dkm^2 > %4.1fdBZ, in VCP %d, recommended %s mode is in use",
            area, zthresh, vcp, mode );

} /* End of Format_MSF_STATUS() */

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Forces weather mode change if wideband is connected and RDA is 
      not in Local Control..

   Inputs:
      mode - weather mode to go to.
      vcp - default VCP for this mode.
      delta_time - time difference from current to conflict start time.
      current_time - current volume scan time.

   Outputs:
      wxstatus_deselect - flag set if current weather mode is deselected.

///////////////////////////////////////////////////////////////////////\*/
static void Force_wxmode_change( int mode, int vcp, time_t delta_time, 
                                 time_t current_time, int *wxstatus_deselect ){

   int wb_status, rda_control;
   char from_mode[16], to_mode[16];

   if( mode == PFWXCONV ){

      strcpy( &from_mode[0], "Clear Air (B)" );
      strcpy( &to_mode[0], "Precip (A)" );

   }
   else{

      strcpy( &to_mode[0], "Clear Air (B)" );
      strcpy( &from_mode[0], "Precip (A)" );

   }

   if( delta_time >= Prctbl.mode_conflict_duration )
      RPGC_log_msg( GL_STATUS | GL_ERROR, "MSF: Mode Conflict Duration Exceeded\n" );

   else if( current_time == Mode_select_status.conflict_start_time )
      RPGC_log_msg( GL_STATUS | GL_ERROR, "MSF: %s Mode Criteria Satisfied\n", to_mode );

   /* If the wideband is connected and the RDA is in REMOTE Control, force weather
      mode change. */
   wb_status = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
   rda_control = ORPGRDA_get_status( RS_CONTROL_STATUS );
   if( (wb_status == RS_CONNECTED) && (rda_control != CS_LOCAL_ONLY) ){

      RPGC_log_msg( GL_STATUS | GL_ERROR,
                    "MSF: Forcing Weather Mode Change %s to %s", from_mode, to_mode );

      ORPGRDA_send_cmd( COM4_WMVCPCHG, MSF_INITIATED_RDA_CTRL_CMD,
                        mode, vcp, 0, 0, 0, NULL );
      *wxstatus_deselect = 1;

   }
   else{

      /* Note:  We have to write in reverse order so that the messages appear in
                chronological order in the status log. */
      if( wb_status != RS_CONNECTED )
         RPGC_log_msg( GL_STATUS, "--->Wideband is Not Connected\n" );

      else
         RPGC_log_msg( GL_STATUS, "--->RDA is in LOCAL Control\n" );

      RPGC_log_msg( GL_STATUS, "Unable to Force Weather Mode Change %s to %s Because:\n",
                    from_mode, to_mode );

   }

} /* End of Force_wxmode_change() */

