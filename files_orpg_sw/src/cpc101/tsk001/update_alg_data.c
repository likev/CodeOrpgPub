/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/15 15:29:47 $
 * $Id: update_alg_data.c,v 1.22 2014/07/15 15:29:47 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <orpg.h>
#include <infr.h>
#include <rpg_port.h>
#include <rpgcs_model_data.h>
#include <rpgcs_latlon.h>
#include <itc.h>
#include <orpgred.h>
#include <mrpg.h>
#include "sta_def.h"
#include <build_model_grids.h>

#define PIOVER2         (3.14159/2.0)
#define PI3OVER2        (3*3.14159/2.0)
#define MISSING_HGT     -999999.0f

/* The legacy color tables. Reflectivity tables are not currently editable. I
   put here for completeness. "VAD 84 Reflectivity Data Levels (Precip/8)" is
   not defined in DEA. So I comment it out. */
static Color_table_t C_tabs[] = {

    {"Vel_data_level_precip_16_97.code",	4,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_precip_16_194.code",	6,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_precip_8_97.code",		5,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"Vel_data_level_precip_8_194.code",	7,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"Vel_data_level_clear_16_97.code",		26,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_clear_16_194.code",	28,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_clear_8_97.code",		27,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"Vel_data_level_clear_8_194.code",		29,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"STP_data_levels.code",			23,	0,
	"ND 0. .3 .6 1. 1.5 2. 2.5 3. 4. 5. 6. 8. 10. 12. 15.",	1},
    {"OHP/THP_data_levels.code",		22,	0,
	"ND 0. .1 .25 .5 .75 1. 1.25 1.5 1.75 2. 2.5 3. 4. 6. 8.", 1},
    {"RCM_reflectivity_data_levels.code",	20,	0,
	"ND 15 30 40 45 50 55 BLANK",				1},
    {"Reflectivity_data_levels_clear_16.code",	2,	0,
	"ND -28 -24 -20 -16 -12 -8 -4 0 4 8 12 16 20 24 28", 	1},
    {"Reflectivity_data_levels_precip_8.code",	3,	0,
	"ND 5 18 30 41 46 50 57", 				1},
    {"Reflectivity_data_levels_precip_16.code",	1,	0,
	"ND 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75", 	1},
    {"VAD_84_reflectivity_data_levels_precip_8.code",	25,	0,
	"5 5 18 30 41 46 50", 					1}

};

static int N_upd_tables  = 11;
		/* number of tables need to be updated for the moment */

static color_table_t Ctbl;

static int  Env_data_msg_fd = -1;
static int  Env_data_msg_updated = 1;
static int  Use_legacy_interpolation_method = 0;

static int  Terminate( int signal, int sigtype );
static void Deau_cb(int fd, LB_id_t msgid, int msg_info, void *arg );
static void Lb_cb( int fd, LB_id_t msgid, int msg_info, void *arg );
static void Init_deau();
static void Init_lb_notification();
static void Update_color_tables();
static int  Update_ewt();
static int  Update_grids();
static int  Update_temps(int model, char *buf,
                         RPGCS_model_grid_data_t *grid_h_data,
                         int i_index, int j_index, int update_flag, double basehgt);
static int  Read_options( int argc, char *argv[] );
static void Print_usage( char *argv[] );
static A3cd97* Interpolate_enw( double *u, double *v, double *height,
                                int num_levels, double radar_height );
static A3cd97* Interpolate_enw_legacy( double *u, double *v, double *height,
                                       int num_levels, double radar_height );
static double Find_height( double temp1, double temp2, double height1,
                           double height2, double target );
static char * Format_message( time_t model_run_time, time_t valid_time,
                              time_t forecast_period, int model );


/******************************************************************

   Description:
      The main function of update_alg_data.

******************************************************************/
int main(int argc, char *argv[] ){

    int ret;

    /* Read options.  Exit on failure. */
    if (Read_options (argc, argv) != 0)
	exit (1);

    /* Initialize the LE service */
    ret = ORPGMISC_init( argc, argv, 500, LB_SINGLE_WRITER, -1, 0 );
    if( ret < 0 ){

	LE_send_msg( GL_ERROR, "ORPGMISC_init Failed (%d)", ret );
	exit (1);

    }

    ret = ORPGTASK_reg_term_handler( Terminate );
    if( ret < 0 ){

	LE_send_msg( GL_ERROR, "ORPGTASK_reg_term_hdlr Failed: (%d)", ret );
	exit (1);

    }

    /* Register for DEA updates to the color tables. */
    Init_deau();
    Update_color_tables();

    /* Open LB data stores for Freezing Height and CIP grids. */
    Open_lb_data_store();

    /* Register for LB notification for Environmental Data. */
    Init_lb_notification ();
    Update_ewt();
    Update_grids();

    /* Report this process is ready. */
    ORPGMGR_report_ready_for_operation();
    LE_send_msg( GL_INFO, "Report ready for RPG operation" );

    /* Wait for operational mode before continuing with initialization. */
    if( ORPGMGR_wait_for_op_state( (time_t) 120 ) < 0 )
       LE_send_msg( GL_ERROR, "Waiting For RPG Operational State TIMED-OUT\n" );

    else
       LE_send_msg( GL_INFO, "The RPG is Operational\n" );

    /* The main loop */
    while( 1 ){

        /* Block events while updating:
           1 - Color tables
           2 - Environmental Wind Table.
           3 - Model Derived Grids
        */
	EN_control( EN_BLOCK );
	Update_color_tables();
        Update_ewt();
	Update_grids();
	EN_control( EN_WAIT, 2000 );

    }

    exit (0);

/* End of main() */
}

/******************************************************************

   Description:
      DEAU LB update callback function.

******************************************************************/
static void Update_color_tables () {

    int upd, i, ret;

    upd = 0;
    for (i = 0; i < N_upd_tables; i++) {
	if (C_tabs[i].updated) {
	    upd = 1;
	    break;
	}
    }
    if (!upd)
	return;

    ORPGDA_open (ORPGDAT_ADAPTATION, LB_WRITE);
    ret = ORPGDA_read (ORPGDAT_ADAPTATION, (char *)&Ctbl,
					sizeof (Ctbl), COLRTBL);
    if (ret != sizeof (Ctbl)) {
	LE_send_msg (GL_ERROR,
		"ORPGDA_read ORPGDAT_ADAPTATION failed (%d)", ret);
	return;
    }
    for (i = 0; i < N_upd_tables; i++) {
	if (C_tabs[i].updated) {
	    char *thr;
	    int n_thrs, ind;

	    C_tabs[i].updated = 0;
	    n_thrs = DEAU_get_string_values (C_tabs[i].de_id, &thr);
	    if (n_thrs <= 0) {
		LE_send_msg (GL_ERROR,
		"DEAU_get_string_values (%s) failed (%d)",
						C_tabs[i].de_id, n_thrs);
		continue;
	    }
	    ind = C_tabs[i].ind - 1;
	    ret = STA_update_color_table (ind + 1, thr, n_thrs,
				    Ctbl.colors[ind], Ctbl.code[ind]);
	    if (ret < 0)
		LE_send_msg (GL_ERROR, "%s not updated", C_tabs[i].de_id);
	    else
		LE_send_msg (GL_INFO, "%s updated", C_tabs[i].de_id);
	}
    }

    ret = ORPGDA_write (ORPGDAT_ADAPTATION, (char *)&Ctbl,
					sizeof (Ctbl), COLRTBL);
    if (ret != sizeof (Ctbl)) {
	LE_send_msg (GL_ERROR,
		"ORPGDA_write ORPGDAT_ADAPTATION failed (%d)", ret);
	return;
    }

/* End of Update_color_tables() */
}

#define MAX_MODEL_HEIGHTS	100

/******************************************************************

   Description:
      New environmental model data received.   Build environmental
      wind table..

   Returns:
      -1 on error, non-negative number on success.

******************************************************************/
static int Update_ewt(){

   char *buf = NULL;
   int status, model, update_flag = MODEL_UPDATE_DISALLOWED;
   int active, i_index, j_index, model_error = 1;

   double basehgt, height[MAX_MODEL_HEIGHTS];
   double u[MAX_MODEL_HEIGHTS], v[MAX_MODEL_HEIGHTS];

   RPGCS_model_attr_t *attrs = NULL;
   RPGCS_model_grid_data_t *grid_u_data = NULL;
   RPGCS_model_grid_data_t *grid_v_data = NULL;
   RPGCS_model_grid_data_t *grid_h_data = NULL;

   A3cd97 *enw = NULL;

   if( (ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE )) == ORPGSITE_FAA_REDUNDANT )
      active = ORPGRED_channel_state( ORPGRED_MY_CHANNEL );
   else
      active = ORPGRED_CHANNEL_ACTIVE;

   /* If the data has not been updated, do nothing. */
   if( (!Env_data_msg_updated) || (active != ORPGRED_CHANNEL_ACTIVE) )
      return 0;

   /* Get the model data. */
   if( (model = RPGCS_get_model_data( ORPGDAT_ENVIRON_DATA_MSG, RUC_ANY_TYPE, &buf )) < 0 ){

      if( model != LB_NOT_FOUND )
         LE_send_msg( GL_STATUS | GL_ERROR,
                      "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );
      return -1;

   }

   /* Get the model attributes. */
   attrs = RPGCS_get_model_attrs( model, buf );
   if( attrs == NULL ){

      LE_send_msg( GL_STATUS | GL_ERROR,
                   "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );

      if( buf != NULL )
         RPGP_product_free( buf );

      return -1;

   }

   /* Get the U, V and Geopotential Height fields. */
   grid_u_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_UCOMP );
   grid_v_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_VCOMP );
   grid_h_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_GH );

   /* Find the base height of the wind table.  This is the same as the
      radar height.  We express this value in Kft MSL*/
   basehgt = (double) ORPGSITE_get_int_prop( ORPGSITE_RDA_ELEVATION ) / 1000.0;  ;
   if( ORPGSITE_error_occurred() ){

      basehgt = 0;
      LE_send_msg( GL_INFO, "ORPGSITE_get_int_prop Of ORPGSITE_RDA_ELEVATION Failed\n" );
      LE_send_msg( GL_INFO, "Setting Default Base Height To 0\n" );

   }
   else
      LE_send_msg( GL_INFO, "Radar Height: %6.2f (Kft MSL)\n", basehgt );


   while(1){

      double grid_lat, grid_lon;

      /* All the necessary data is available ...... */
      if( (grid_u_data != NULL)
                  &&
          (grid_v_data != NULL)
                  &&
          (grid_h_data != NULL) ){


         Siteadp_adpt_t site;
         int ret, num_levels = 0, i, j;

         /* Find the closest grid point to the radar location. */
         if( (ret = ORPGSITE_get_site_data( &site )) < 0 ){

            LE_send_msg( GL_INFO, "ORPGSITE_get_site_data() Failed: %d\n", ret );
            break;

         }

         LE_send_msg( GL_INFO, "Updating the Model-derived EWT\n" );

         /* This initializes modules used to convert map projected data to
            meteorological coordinates. */
         RPGCS_lambert_init( attrs );

         /* Convert the site lat/lon to model grid point. */
         if( ( ret = RPGCS_lambert_latlon_to_grid_point( ((double) site.rda_lat)/1000.0,
                                                         ((double) site.rda_lon)/1000.0,
                                                         &i_index, &j_index )) < 0 ){

            if( ret == RPGCS_DATA_POINT_NOT_IN_GRID )
               LE_send_msg( GL_INFO, "Site Lat/Lon Not Within Model Grid\n" );

            else
               LE_send_msg( GL_INFO, "RPGCS_lambert_latlon_to_grid_point Failed\n" );

            break;

         }

         /* Convert the grid index to lat/lon. */
         if( RPGCS_lambert_grid_point_latlon( i_index, j_index,
                                              &grid_lat, &grid_lon ) < 0 ){

            LE_send_msg( GL_INFO, "RPGCS_lambert_grid_point_latlon Failed\n" );
            break;

         }

         LE_send_msg( GL_INFO, "Miscellaneous Information ....\n" );
         LE_send_msg( GL_INFO, "--->Site Latitude: %f, Site Longitude: %f\n",
                      site.rda_lat/1000.0, site.rda_lon/1000.0 );
         LE_send_msg( GL_INFO, "--->Grid I Index: %d, Grid J Index: %d\n",
                      i_index, j_index );
         LE_send_msg( GL_INFO, "--->Grid Point Latitude: %f, Grid Point Longitude: %f\n",
                      grid_lat, grid_lon );

         /* Initialize the height table, and u and v components. */
         for( j = 0; j < MAX_MODEL_HEIGHTS; j++ ){

            height[j] = MTTABLE;
            u[j] = MTTABLE;
            v[j] = MTTABLE;

         }

         /* There should be height data at at least one level. */
         if( grid_h_data->num_levels <= 0 ){

            LE_send_msg( GL_INFO, "Error: grid_h_data->num_levels <= 0 (%d)\n",
                         grid_h_data->num_levels );
            break;

         }

         /* Set the model error flag is 0 (no errors). */
         model_error = 0;

         /* Populate the wind table. */
         LE_send_msg( GL_INFO, "Model Data:\n" );
         for( j = 0; j < grid_h_data->num_levels; j++ ){

            double h_level_value, geo_hgt;
            int h_units;

            h_level_value = grid_h_data->params[j]->level_value;
            geo_hgt = RPGCS_get_data_value( grid_h_data, j, i_index, j_index,
                                            &h_units );

            /* Convert the height in meters to 1000s of feet. */
            if( h_units == RPGCS_METER_UNITS )
               geo_hgt *= M_TO_KFT;

            else if( h_units == RPGCS_KILOMETER_UNITS )
               geo_hgt *= M_TO_FT;

            /* We assume the u and v components exist on matching levels.  Make sure
               geopotential height levels match u and v components levels. */
            for( i = 0; i < grid_u_data->num_levels; i++ ){

               double ucomp, vcomp;
               double u_level_value = grid_u_data->params[i]->level_value;
               double v_level_value = grid_v_data->params[i]->level_value;
               float windspeed, direction;
               int u_units, v_units;

               if( h_level_value != u_level_value )
                  continue;

               if( u_level_value != v_level_value )
                  LE_send_msg( GL_INFO, "u_level_value: %d != v_level_value: %d\n",
                               u_level_value, v_level_value );

               ucomp = RPGCS_get_data_value( grid_u_data, i, i_index, j_index,
                                             &u_units );
               vcomp = RPGCS_get_data_value( grid_v_data, i, i_index, j_index,
                                             &v_units );

               if( (ucomp == MODEL_BAD_VALUE) || (vcomp == MODEL_BAD_VALUE) )
                  LE_send_msg( GL_INFO, "ucomp: %d or vcomp: %d MODEL_BAD_VALUE: %d\n",
                               ucomp, vcomp, MODEL_BAD_VALUE );

               if( u_units != v_units )
                  LE_send_msg( GL_INFO, "u_units: %d != v_units: %d\n", u_units, v_units );

               /* Save the height for interpolation purposes.  Here, height is
                  Kft MSL. */
               height[num_levels] = geo_hgt;

               /* Convert the U and V components to meteorological coordinates. */
               RPGCS_lambertuv_to_uv( ucomp, vcomp, grid_lon, &u[num_levels], &v[num_levels] );

               windspeed = sqrt( u[num_levels]*u[num_levels] + v[num_levels]*v[num_levels] );
               if( u[num_levels] < 0 )
                  direction = (PIOVER2 - atan(v[num_levels]/u[num_levels]))/DEGTORAD;

               else if( u[num_levels] > 0 )
                  direction = (PI3OVER2 - atan(v[num_levels]/u[num_levels]))/DEGTORAD;

               else{

                  if( v[num_levels] < 0 )
                     direction = 0.0;

                  else
                     direction = 180.0;

               }

               LE_send_msg( GL_INFO, "--->Speed: %6.2f (m/s), Dir: %6.2f (Deg), H: %6.2f (Kft MSL)\n",
                            windspeed, direction, geo_hgt );

               /* Increment the number of height levels. */
               num_levels++;

            }

         }

         /* Interpolate the Environmental Wind Table between levels. */
         if( Use_legacy_interpolation_method )
            enw = Interpolate_enw_legacy( u, v, height, grid_u_data->num_levels, basehgt );

         else
            enw = Interpolate_enw( u, v, height, grid_u_data->num_levels, basehgt );

         /* Check for NULL pointer ... If NULL (which we never expect to happen),
            set the model_error flag and break out of "while(1)" loop. */
         if( enw == NULL ){

            LE_send_msg( GL_ERROR, "Model Interpolation Error ....\n" );
            model_error = 1;

            break;

         }

         /* Set additional fields in the table. */
         enw->envwndflg = 1;

     int vtime,year,month,day,hour,minute,second,julian_date;
     ret = RPGCS_unix_time_to_ymdhms(attrs->valid_time, &year, &month,  &day,
                                         &hour, &minute, &second);

     ret = RPGCS_date_to_julian(year, month, day, &julian_date);

     vtime = (julian_date-1)*1440 + hour*60 + minute;

     enw->sound_time = vtime;

/*         enw->sound_time = time( NULL ) / 60;
*/
         enw->basehgt = basehgt;

         LE_send_msg( GL_INFO, "Environmental Wind Table:" );
         for( j = 0; j < LEN_EWTAB; j++ ){

            if( enw->ewtab[WNDSPD][j] != MTTABLE )
               LE_send_msg( GL_INFO, "--->Level: %d, Speed: %6.2f, Direction: %6.2f\n",
                            j, enw->ewtab[WNDSPD][j], enw->ewtab[WNDDIR][j] );

         }

         /* Write the data. */
         status = ORPGDA_write( (MODEL_EWT/ITC_IDRANGE)*ITC_IDRANGE,
                                (char *) enw, sizeof( A3cd97 ),
                                LBID_MODEL_EWT );

         if( status < 0 )
            LE_send_msg( GL_INFO, "ORPGDA_write MODEL_EWT Failed (%d)\n", status);

         else{

            EWT_update_t update;
            A3cd97 vad;

            time_t model_run_time = attrs->model_run_time;
            time_t valid_time = attrs->valid_time;
            time_t forecast_period = attrs->forecast_period;
            int model = attrs->model;

            char *text = Format_message( model_run_time, valid_time, forecast_period, model );

            /* Determine if the VAD/Model Environmental Wind Table Needs to be updated. */
            status = ORPGDA_read( (EWT_UPT/ITC_IDRANGE)*ITC_IDRANGE,
                                  (char *) &update, sizeof( EWT_update_t ),
                                  LBID_EWT_UPT );

            if( (status > 0) && ((update_flag = update.flag) == MODEL_UPDATE_ALLOWED) ){

               /* Must read A3CD97.  The "envwndflg" value needs to be transferred
                  to "enw". */
               status = ORPGDA_read( (A3CD97/ITC_IDRANGE)*ITC_IDRANGE,
                                     (char *) &vad, sizeof( A3cd97 ),
                                     A3CD97 % ITC_IDRANGE );

               if( status > 0 ){

                  enw->envwndflg = vad.envwndflg;

                  /* Write the data. */
                  status = ORPGDA_write( (A3CD97/ITC_IDRANGE)*ITC_IDRANGE,
                                         (char *) enw, sizeof( A3cd97 ),
                                         LBID_A3CD97 );

                  if( status < 0 )
                     LE_send_msg( GL_STATUS | GL_ERROR,
                                  "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );

                  else{

                     /* Post event indicating A3CD97 has been updated. */
                     EN_post_event( ORPGEVT_ENVWND_UPDATE );

                     LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG,
                                  "MODEL DATA: %s Being Used By RPG\n", text );

                  }

               }
               else
                  LE_send_msg( GL_STATUS | GL_ERROR,
                               "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );

            }
            else
               LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG,
                            "MODEL DATA: %s Not Being Used By RPG\n", text );

         }

      }

      /* Break out of "while(1) loop. */
      break;

   }

   /* Update the Hail Temperature Data. */
   if( !model_error ){

      RPGC_log_msg( GL_INFO, "Updating Hail Temps ....\n" );
      Update_temps( model, buf, grid_h_data, i_index, j_index, update_flag, basehgt);

   }
   else
      RPGC_log_msg( GL_INFO, "model_error .... Temps NOT Updated\n" );

   /* Free the data */
   if( grid_u_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_u_data );

   if( grid_v_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_v_data );

   if( grid_h_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_h_data );

   if( attrs != NULL )
      free( attrs );

   if( buf != NULL ){

      RPGP_product_free( buf );
      return 0;

   }

   return 0;

/* End of Update_ewt() */
}

/******************************************************************

   Description:
      Update the hail temperature data.

   Inputs:
      model       - model of interest.
      buf         - buffer holding the model of interest.
      grid_h_data - grid height data for the model of interest.
      i_index     - grid "i" index closest to radar.
      j_index     - grid "j" index closest to radar.
      update_flag - for deciding if adaptation data is updated.
      basehgt     - radar height (MSL)

   Returns:
      -1 on error, non-negative number on success.

   20120127 Ward CCR NA12-00024 Changed order to go from highest
                 level to lowest, added bad data checks and
                 added basehgt to calling parameters.

******************************************************************/

#define NOT_FOUND -1

#define MIN_TEMP   -100 /* C */
#define MAX_TEMP    100 /* C */

#define MIN_HEIGHT    0 /* Kft */
#define MAX_HEIGHT  100 /* Kft */

#define MIN_LEVEL     0 /* mb */
#define MAX_LEVEL  1050 /* mb */

static int Update_temps(int model, char *buf,
                        RPGCS_model_grid_data_t *grid_h_data,
                        int i_index, int j_index, int update_flag,
                        double basehgt)
{
   RPGCS_model_grid_data_t *grid_t_data = NULL;
   int ret, zerodeg_hgt_set = 0, mtwentydeg_hgt_set = 0;

   time_t current_time = time( NULL );
   int yy = 1970, mm = 1, dd = 1, hr = 0, min = 0, sec = 0;

   Hail_temps_t hail;

   int i, j;
   int t_units, h_units;

   double temp,  height;
   double temp1, height1;
   double temp2, height2;

   double t_level;
   double h_level;

   short temp_is_good[MAXIMUM_LEVELS];
   short height_is_good[MAXIMUM_LEVELS];

   short h_lt_m20_index = NOT_FOUND; /* height <  -20 C */
   short t_lt_m20_index = NOT_FOUND; /* temp   <  -20 C */

   short h_ge_m20_index = NOT_FOUND; /* height >= -20 C */
   short t_ge_m20_index = NOT_FOUND; /* temp   >= -20 C */

   short h_lt_0_index = NOT_FOUND;   /* height <    0 C */
   short t_lt_0_index = NOT_FOUND;   /* temp   <    0 C */

   short h_ge_0_index = NOT_FOUND;   /* height >=   0 C */
   short t_ge_0_index = NOT_FOUND;   /* temp   >=   0 C */

   /* Initialize the hail temperatures */

   hail.height_0        = 0.0;
   hail.height_minus_20 = 0.0;
   hail.hail_date_yy    = 1970;
   hail.hail_date_mm    = 1;
   hail.hail_date_dd    = 1;
   hail.hail_time_hr    = 0;
   hail.hail_time_min   = 0;
   hail.hail_time_sec   = 0;

   /* Assume data is bad until proven otherwise */

   memset(temp_is_good,   0, MAXIMUM_LEVELS * sizeof(short));
   memset(height_is_good, 0, MAXIMUM_LEVELS * sizeof(short));

   /* Get the Temperature field. */

   grid_t_data = RPGCS_get_model_field(model, buf, RPGCS_MODEL_TEMP);

   if(grid_t_data == NULL)
   {
      LE_send_msg(GL_INFO, "Cannot Update the Model-derived Hail Temperatures\n");
   }
   else
   {
      LE_send_msg(GL_INFO, "Updating the Model-derived Hail Temperatures\n");

      /* 1. Loop through the data, printing it out.                    *
       *                                                               *
       * 2. As we loop, validate the data. If a value is invalid,      *
       *    print it out, but continue, which removes it from further  *
       *    processing.                                                *
       *                                                               *
       * 3. MODEL_BAD_VALUE, -99999999.0, is returned by               *
       *    RPGCS_get_data_value() when the data is off the grid;      *
       *    i_index or j_index is bad.                                 *
       *                                                               *
       * 4. To be valid, the data must be between these ranges:        *
       *                                                               *
       *     -100 C   <= temp   <=  100 C                              *
       *        0 Kft <= height <=  100 Kft                            *
       *        0 mb  <= level  <= 1050 mb                             *
       *                                                               *
       *    Heights < radar height are also considered invalid.        *
       *                                                               *
       * 5. The model data is indexed from the ground, 0, to the       *
       *    highest, grid_t_data->num_levels - 1. For some reason,     *
       *    num_levels - 1 is always invalid:                          *
       *                                                               *
       *           num_levels    actual data                           *
       *           ----------    -----------                           *
       *    RUC 13    33             32                                *
       *    RUC 40    19             18                                *
       *                                                               *
       *    so we skip num_levels-1, as the previous code did.         *
       *                                                               *
       * 6. We will look for -20 C and 0 C temps from the top down.    *
       *    There may be multiple crossing points, and this will give  *
       *    us the highest level with a crossing point. The previous   *
       *    code looked from the bottom up, which gave the lowest      *
       *    level with a crossing point.                               *
       *                                                               *
       * 7. To identify crossing points, We will record the first      *
       *    (valid) crossing point >= -20 C and the first (valid)      *
       *    crossing point >= -0 C. By the way the loop is traversed,  *
       *    this crossing automatically is the highest one, and it     *
       *    also ensures that the level above it is < -20 C; or < 0 C. *
       *                                                               *
       * 8. temp and height have separate level arrays. In normal      *
       *    operation the level indices match exactly.                 *
       *    For example:                                               *
       *                                                               *
       *        temp   index 12 is (-7.75   C at 700 mb) and           *
       *        height index 12 is ( 9.82 Kft at 700 mb).              *
       *                                                               *
       *    However, the indices are not guaranteed to match.          *
       *    For example:                                               *
       *                                                               *
       *        temp   index 12 is (-7.75   C at 700 mb) and           *
       *        height index 12 is (11.70 Kft at 650 mb).              *
       *                                                               *
       *    For this reason, the temp and height indices are kept      *
       *    separately, and a (rare) mismatch is dealt with after      *
       *    the loop. The previous code also did this check.           */

      for(i = grid_t_data->num_levels - 2 /* see note 5 */; i >= 0; i--)
      {
         /* Validate temp[i] */

         temp    = RPGCS_get_data_value(grid_t_data, i, i_index, j_index, &t_units);
         height  = RPGCS_get_data_value(grid_h_data, i, i_index, j_index, &h_units);
         t_level = grid_t_data->params[i]->level_value;

         if(temp == MODEL_BAD_VALUE /* -99999999.0 */)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): MODEL BAD VALUE, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                        height, t_level);
            continue; /* skip this level */
         }

         /* Convert units. Height unit conversion moved up for debugging output. */

         if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
            temp += KELVIN_TO_C;

         if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
            height *= M_TO_KFT;

         if(temp < MIN_TEMP)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): BAD temp %6.2f < %d, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                        temp, MIN_TEMP, height, t_level);
            continue; /* skip this level */
         }

         if(temp > MAX_TEMP)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): BAD temp %6.2f > %d, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                        temp, MAX_TEMP, height, t_level);
            continue; /* skip this level */
         }

         if(t_level < MIN_LEVEL)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): BAD t_level %4.0f < %d\n",
                        temp, height, t_level, MIN_LEVEL);
            continue; /* skip this level */
         }

        if(t_level > MAX_LEVEL)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): BAD t_level %4.0f > %d\n",
                        temp, height, t_level, MAX_LEVEL);
            continue; /* skip this level */
         }

         /* If we got here, the temp is valid */

         temp_is_good[i] = TRUE;

         /* Validate height[i] */

         if(height == MODEL_BAD_VALUE)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL): MODEL BAD VALUE, Level (mb): %4.0f\n",
                        temp, t_level);
            continue; /* skip this level */
         }

         if(height < MIN_HEIGHT)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL) BAD height %5.2f < %d, Level (mb): %4.0f\n",
                        temp, height, MIN_HEIGHT, t_level);
            continue; /* skip this level */
         }

         if(height < basehgt) /* radar height */
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL) BAD height %5.2f < radar %5.2f, Level (mb): %4.0f\n",
                        temp, height, basehgt, t_level);
            continue; /* skip this level */
         }

         if(height > MAX_HEIGHT)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL) BAD height %5.2f > %d, Level (mb): %4.0f\n",
                        temp, height, MAX_HEIGHT, t_level);
            continue; /* skip this level */
         }

         h_level = grid_h_data->params[i]->level_value;

         if(h_level < MIN_LEVEL)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): BAD h_level %4.0f < %d\n",
                        temp, height, h_level, MIN_LEVEL);
            continue; /* skip this level */
         }

         if(h_level > MAX_LEVEL)
         {
            LE_send_msg(GL_INFO,
                        "Temp  (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): BAD h_level %4.0f > %d\n",
                        temp, height, h_level, MAX_LEVEL);
            continue; /* skip this level */
         }

         /* If we got here, the height is valid */

         height_is_good[i] = TRUE;

         LE_send_msg(GL_INFO,
                     "Temp  (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                     temp, height, t_level);

         /* See if it's the first temp >= -20 C */

         if((t_ge_m20_index == NOT_FOUND) && (temp >= -20.0))
         {
            t_ge_m20_index = i;

            /* Double check the height level matches
             * the temp level. This is normal behavior. */

            if(h_level == t_level)
               h_ge_m20_index = i;
         }

         /* See if it's the first temp >= 0 C */

         if((t_ge_0_index == NOT_FOUND) && (temp >= 0.0))
         {
            t_ge_0_index = i;

            /* Double check the height level matches
             * the temp level. This is normal behavior. */

            if(h_level == t_level)
               h_ge_0_index = i;
         }

      } /* end loop through all the temperature levels */

      /*  9. We are done looping through the levels. If all worked    *
       *     normally, we have:                                       *
       *                                                              *
       *       t_ge_m20_index - index of highest  temp   >= -20 C     *
       *       h_ge_m20_index - index of matching height >= -20 C     *
       *                                                              *
       *       t_ge_0_index   - index of highest  temp   >= - 0 C     *
       *       h_ge_0_index   - index of matching height >=   0 C     *
       *                                                              *
       *     Deal with the error cases. We will do the -20 C case     *
       *     first, and the 0 C processing is similar.                *
       *                                                              *
       * 10. If the t_ge_m20_index was found, but the h_ge_m20_index  *
       *     was not found, this can only happen if the temp and      *
       *     height levels are out of sync. Do a pass through the     *
       *     height levels to try and find a matching height level.   */

      /* Process >= -20 C */

      if(t_ge_m20_index != NOT_FOUND) /* have a good temp */
      {
         if(h_ge_m20_index == NOT_FOUND) /* the temp doesn't have a matching height */
         {
            t_level = grid_t_data->params[t_ge_m20_index]->level_value;

            for(i = 0; i < grid_h_data->num_levels - 1; i++)
            {
                if(height_is_good[i] == TRUE)
                {
                   h_level = grid_h_data->params[i]->level_value;

                   if(h_level == t_level)
                   {
                      h_ge_m20_index = i;
                      break;
                   }
                }
            }
         }
      }

      /* 11. If we still couldn't find t_ge_m20_index or h_ge_m20_index, *
       *     find the lowest good level, which is guaranteed to be       *
       *     above the radar height (because all the levels below the    *
       *     radar height were ignored).                                 *
       *                                                                 *
       * 12. hail heights are rounded and displayed to 0.1 Kft, their    *
       *     accuracy in hail.alg                                        */

      if((t_ge_m20_index == NOT_FOUND) || (h_ge_m20_index == NOT_FOUND))
      {
         /* 13. We will get here if there is no -20 C crossing  *
          *     or the -20 C crossing is below the radar height *
          *     (hell has frozen over).                         */

         for(i = 0; i < grid_t_data->num_levels - 1; i++)
         {
            if((temp_is_good[i] == TRUE) && (height_is_good[i] == TRUE))
            {
               temp2   = RPGCS_get_data_value(grid_t_data, i, i_index, j_index, &t_units);
               height2 = RPGCS_get_data_value(grid_h_data, i, i_index, j_index, &h_units);

               if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
               {
                  temp2 += KELVIN_TO_C;
               }

               if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
               {
                  height2 *= M_TO_KFT;
               }

               hail.height_minus_20 = height2;
               hail.height_minus_20 = RPGC_NINT(hail.height_minus_20 * 10.0) / 10.0; /* round to nearest 0.1 Kft */

               mtwentydeg_hgt_set = 1;

               LE_send_msg(GL_INFO, " ");

               LE_send_msg(GL_INFO,
                           "     hail.height_minus_20 (Kft MSL): %4.1fd\n",
                           hail.height_minus_20);

               LE_send_msg(GL_INFO,
                          "Temp2 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                          temp2, height2, grid_t_data->params[i]->level_value);

               break; /* stop looking for a good level */
            }
         }
      }
      else /* look above for < -20 C */
      {
         /* 14. If we got here, we must have a good t_ge_m20_index and a  *
          *     good h_ge_m20_index. find the < -20 C level, which is     *
          *     guaranteed to be above the radar height. We start at one  *
          *     level above the -20 C level:                              *
          *                                                               *
          *        i = t_ge_m20_index + 1;                                *
          *        j = h_ge_m20_index + 1;                                *
          *                                                               *
          *     and work up:                                              *
          *                                                               *
          *        i++;                                                   *
          *        j++;                                                   *
          *                                                               *
          *     Normally, the next good level is just 1 level up, but we  *
          *     loop in the rare case it has bad values.                  *
          *                                                               *
          * 15. If t_ge_m20_index or h_ge_m20_index is at the highest     *
          *     level, then t_lt_m20_index (a level above it) will not be *
          *     found.                                                    */

         i = t_ge_m20_index + 1;
         j = h_ge_m20_index + 1;

         while((i < grid_t_data->num_levels - 1) &&
               (j < grid_h_data->num_levels - 1))
         {
            if((temp_is_good[i]   == TRUE) &&
               (height_is_good[j] == TRUE))
            {
                t_level = grid_t_data->params[i]->level_value;
                h_level = grid_h_data->params[j]->level_value;

                if(t_level == h_level)
                {
                   t_lt_m20_index = i;
                   h_lt_m20_index = j;
                   break;
                }
            }

            i++;
            j++;
         }

         /* 16. If we couldn't find t_lt_m20_index or h_lt_m20_index,  *
          *     use t_ge_m20_index, h_ge_m20_index as the -20 C level. *
          *     No interpolation is done.                              */

         if((t_lt_m20_index == NOT_FOUND) || (h_lt_m20_index == NOT_FOUND))
         {
             /* Use >= -20 C as the height */

             temp2   = RPGCS_get_data_value(grid_t_data, t_ge_m20_index, i_index, j_index, &t_units);
             height2 = RPGCS_get_data_value(grid_h_data, h_ge_m20_index, i_index, j_index, &h_units);

             if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
             {
                temp2 += KELVIN_TO_C;
             }

             if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
             {
                height2 *= M_TO_KFT;
             }

             hail.height_minus_20 = height2;
             hail.height_minus_20 = RPGC_NINT(hail.height_minus_20 * 10.0) / 10.0; /* round to nearest 0.1 Kft */

             mtwentydeg_hgt_set = 1;

             LE_send_msg(GL_INFO, " ");

             LE_send_msg(GL_INFO,
                         "     hail.height_minus_20 (Kft MSL): %4.1fd\n",
                         hail.height_minus_20);

             LE_send_msg(GL_INFO,
                        "Temp2 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                        temp2, height2, grid_t_data->params[t_ge_m20_index]->level_value);
         }
         else /* normal case, have 2 good (temp, height) pairs */
         {
             /* 17. If we got here, we have 2 pairs, (t_lt_m20_index, h_lt_m20_index) *
              *     and (t_ge_m20_index, h_ge_m20_index). Use Find_height() to        *
              *     interpolate between them.                                         */

             temp1   = RPGCS_get_data_value(grid_t_data, t_lt_m20_index, i_index, j_index, &t_units);
             height1 = RPGCS_get_data_value(grid_h_data, h_lt_m20_index, i_index, j_index, &h_units);

             temp2   = RPGCS_get_data_value(grid_t_data, t_ge_m20_index, i_index, j_index, &t_units);
             height2 = RPGCS_get_data_value(grid_h_data, h_ge_m20_index, i_index, j_index, &h_units);

             if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
             {
                temp1 += KELVIN_TO_C;
                temp2 += KELVIN_TO_C;
             }

             if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
             {
                height1 *= M_TO_KFT;
                height2 *= M_TO_KFT;
             }

             hail.height_minus_20 = Find_height(temp1, temp2, height1, height2, -20.0);
             hail.height_minus_20 = RPGC_NINT(hail.height_minus_20 * 10.0) / 10.0; /* round to nearest 0.1 Kft */

             mtwentydeg_hgt_set = 1;

             LE_send_msg(GL_INFO, " ");

             LE_send_msg(GL_INFO,
                        "Temp1 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                        temp1, height1, grid_t_data->params[t_lt_m20_index]->level_value);

             LE_send_msg(GL_INFO,
                         "     hail.height_minus_20 (Kft MSL): %4.1f\n",
                         hail.height_minus_20);

             LE_send_msg(GL_INFO,
                        "Temp2 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                        temp2, height2, grid_t_data->params[t_ge_m20_index]->level_value);
         }

      } /* end look above for < -20 C */

      /* Repeat the -20 C logic to process 0 C */

      if(t_ge_0_index != NOT_FOUND) /* have a good temp */
      {
         if(h_ge_0_index == NOT_FOUND) /* the temp doesn't have a matching height */
         {
            /* The height and temp levels are out of sync. *
             * Try to find a matching height level         */

            t_level = grid_t_data->params[t_ge_0_index]->level_value;

            for(i = 0; i < grid_h_data->num_levels - 1; i++)
            {
                if(height_is_good[i] == TRUE)
                {
                   h_level = grid_h_data->params[i]->level_value;

                   if(h_level == t_level)
                   {
                      h_ge_0_index = i;
                      break;
                   }
                }
            }
         }
      }

      if((t_ge_0_index == NOT_FOUND) || (h_ge_0_index == NOT_FOUND))
      {
         /* We couldn't find a (temp, height) pair >= 0 C.     *
          * Use the lowest good value, which will be above     *
          * the radar height.                                  *
          *                                                    *
          * 18. We will get here if there is no 0 C crossing   *
          *     or the 0 C crossing is below the radar height. */

         for(i = 0; i < grid_t_data->num_levels - 1; i++)
         {
            if((temp_is_good[i] == TRUE) && (height_is_good[i] == TRUE))
            {
               temp2   = RPGCS_get_data_value(grid_t_data, i, i_index, j_index, &t_units);
               height2 = RPGCS_get_data_value(grid_h_data, i, i_index, j_index, &h_units);

               if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
               {
                  temp2 += KELVIN_TO_C;
               }

               if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
               {
                  height2 *= M_TO_KFT;
               }

               hail.height_0 = height2;
               hail.height_0 = RPGC_NINT(hail.height_0 * 10.0) / 10.0; /* round to nearest 0.1 Kft */

               zerodeg_hgt_set = 1;

               LE_send_msg(GL_INFO, " ");

               LE_send_msg(GL_INFO,
                           "            hail.height_0 (Kft MSL): %4.1f\n",
                           hail.height_0);

               LE_send_msg(GL_INFO,
                           "Temp2 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                           temp2, height2, grid_t_data->params[i]->level_value);

               break; /* stop looking for a good level */
            }
         }
      }
      else /* look above for < 0 C */
      {
         i = t_ge_0_index + 1;
         j = h_ge_0_index + 1;

         while((i < grid_t_data->num_levels - 1) &&
               (j < grid_h_data->num_levels - 1))
         {
            if((temp_is_good[i]   == TRUE) &&
               (height_is_good[j] == TRUE))
            {
                t_level = grid_t_data->params[i]->level_value;
                h_level = grid_h_data->params[j]->level_value;

                if(t_level == h_level)
                {
                   t_lt_0_index = i;
                   h_lt_0_index = j;
                   break;
                }
            }

            i++;
            j++;
         }

         if((t_lt_0_index == NOT_FOUND) || (h_lt_0_index == NOT_FOUND))
         {
             /* Use >= 0 C as the height */

             temp2   = RPGCS_get_data_value(grid_t_data, t_ge_0_index, i_index, j_index, &t_units);
             height2 = RPGCS_get_data_value(grid_h_data, h_ge_0_index, i_index, j_index, &h_units);

             if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
             {
                temp2 += KELVIN_TO_C;
             }

             if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
             {
                height2 *= M_TO_KFT;
             }

             hail.height_0 = height2;
             hail.height_0 = RPGC_NINT(hail.height_0 * 10.0) / 10.0; /* round to nearest 0.1 Kft */

             zerodeg_hgt_set = 1;

             LE_send_msg(GL_INFO, " ");

             LE_send_msg(GL_INFO,
                         "            hail.height_0 (Kft MSL): %4.1f\n",
                         hail.height_0);

             LE_send_msg(GL_INFO,
                         "Temp2 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                         temp2, height2, grid_t_data->params[t_ge_0_index]->level_value);
         }
         else /* normal case, have 2 good (temp, height) pairs */
         {
             temp1   = RPGCS_get_data_value(grid_t_data, t_lt_0_index, i_index, j_index, &t_units);
             height1 = RPGCS_get_data_value(grid_h_data, h_lt_0_index, i_index, j_index, &h_units);

             temp2   = RPGCS_get_data_value(grid_t_data, t_ge_0_index, i_index, j_index, &t_units);
             height2 = RPGCS_get_data_value(grid_h_data, h_ge_0_index, i_index, j_index, &h_units);

             if(t_units == RPGCS_KELVIN_UNITS) /* Kelvin to C */
             {
                temp1 += KELVIN_TO_C;
                temp2 += KELVIN_TO_C;
             }

             if(h_units == RPGCS_METER_UNITS)  /* m to Kft */
             {
                height1 *= M_TO_KFT;
                height2 *= M_TO_KFT;
             }

             hail.height_0 = Find_height(temp1, temp2, height1, height2, 0.0);
             hail.height_0 = RPGC_NINT(hail.height_0 * 10.0) / 10.0; /* round to nearest 0.1 Kft */

             zerodeg_hgt_set = 1;

             LE_send_msg(GL_INFO, " ");

             LE_send_msg(GL_INFO,
                         "Temp1 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                         temp1, height1, grid_t_data->params[t_lt_0_index]->level_value);

             LE_send_msg(GL_INFO,
                         "            hail.height_0 (Kft MSL): %4.1f\n",
                         hail.height_0);

             LE_send_msg(GL_INFO,
                         "Temp2 (C): %6.2f, Height (Kft MSL): %5.2f, Level (mb): %4.0f\n",
                         temp2, height2, grid_t_data->params[t_ge_0_index]->level_value);
         }

      } /* end look above for < 0 C */

      /* Free the data */

      RPGCS_free_model_field(model, (char *) grid_t_data);
   }

   /* 19. Code from here on is identical to the previous Update_temps(). */

   /* If the heights have not been updated, return. */

   if(!zerodeg_hgt_set && !mtwentydeg_hgt_set)
   {
      LE_send_msg( GL_INFO, "0 Deg And -20 Deg Heights Not Updated!\n" );
      return -1;
   }

   /* Set the time the hail data was updated. */



     int vtime,year,month,day,hour,minute,second,julian_date;
     RPGCS_model_attr_t *attrs = NULL;

   /* Get the model attributes. */
   attrs = RPGCS_get_model_attrs( model, buf );
   if( attrs == NULL ){

      LE_send_msg( GL_STATUS | GL_ERROR, 
                   "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );

      if( buf != NULL )
         RPGP_product_free( buf );

      return -1;

   }

     ret = RPGCS_unix_time_to_ymdhms(attrs->valid_time, &year, &month,  &day,
                                         &hour, &minute, &second);
     if( year >= 2000.0 )
       year -= 2000;     
     else
       year -= 1900;
 
     hail.hail_date_yy = year;
     hail.hail_date_mm = month;
     hail.hail_date_dd = day;
     hail.hail_time_hr = hour;
     hail.hail_time_min = minute;
     hail.hail_time_sec = second;

      fprintf(stderr,"year = %d  month = %d  day = %d  hour = %d  minute = %d  second = %d\n",
	      year,month,day,hour,minute,second);

   if( attrs != NULL )
      free( attrs );


     /**  if( unix_time( &current_time, &yy, &mm, &dd, &hr, &min, &sec ) >= 0 ){
      **/
      /* Subtract the century out of the year. */
     /**      if( yy >= 2000.0 )
         yy -= 2000;
     **/
      /* Update the ITC.  Then update adaptation data if allowed. */
     /**      hail.hail_date_yy = yy;
      hail.hail_date_mm = mm;
      hail.hail_date_dd = dd;
      hail.hail_time_hr = hr;
      hail.hail_time_min = min;
      hail.hail_time_sec = sec;

   }
   else
      LE_send_msg( GL_INFO, "unix_time Failed .... Date/Time Set to 1/1/70 0:0:0" );
     **/

   /* Update the ITC data. */

   ret = ORPGDA_write( (MODEL_HAIL/ITC_IDRANGE)*ITC_IDRANGE,
                       (char *) &hail, sizeof( Hail_temps_t ),
                       LBID_MODEL_HAIL );

   if( ret < 0 )
      LE_send_msg( GL_INFO, "ORPGDA_write HAIL_UPT Failed (%d)\n", ret );

   /* Update adaptation data if updates allowed. */

   if( update_flag == MODEL_UPDATE_ALLOWED ){

      char *ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );

      if( ds_name != NULL ){

         double yyd = (double) year;
         double mmd = (double) month;
         double ddd = (double) day;
         double hrd = (double) hour;
         double mind = (double) minute;
         double secd = (double) second;

	 /*         double yyd = (double) yy;
         double mmd = (double) mm;
         double ddd = (double) dd;
         double hrd = (double) hr;
         double mind = (double) min;
         double secd = (double) sec;
	 */
         LE_send_msg( GL_INFO, "Updating Hail Adaptation Data ....\n" );
         DEAU_LB_name( ds_name );
         if( ((ret = DEAU_set_values( "alg.hail.height_0", 0, (void*) &hail.height_0,
                                      1, 0)) >= 0)
                                 &&
             ((ret = DEAU_set_values( "alg.hail.height_minus_20", 0,
                                      (void*) &hail.height_minus_20, 1, 0)) >= 0) ){


            /* Update adaptation data. */
            if( ((ret = DEAU_set_values( "alg.hail.hail_date_yy", 0, (void *) &yyd, 1, 0 )) < 0)
                                           ||
                ((ret = DEAU_set_values( "alg.hail.hail_date_mm", 0, (void *) &mmd, 1, 0 )) < 0)
                                           ||
                ((ret = DEAU_set_values( "alg.hail.hail_date_dd", 0, (void *) &ddd, 1, 0 )) < 0)
                                           ||
                ((ret = DEAU_set_values( "alg.hail.hail_time_hr", 0, (void *) &hrd, 1, 0 )) < 0)
                                           ||
                ((ret = DEAU_set_values( "alg.hail.hail_time_min", 0, (void *) &mind, 1, 0 )) < 0)
                                           ||
                ((ret = DEAU_set_values( "alg.hail.hail_time_sec", 0, (void *) &secd, 1, 0 )) < 0) )
                LE_send_msg( GL_INFO, "Model-Derived Hail Temps Update Date/Time Failed (%d)\n", ret );

         }
         else
            LE_send_msg( GL_INFO, "Model-Derived Hail Temps Update Failed (%d)\n", ret );

      }
      else
         LE_send_msg( GL_INFO, "Adaptation Data Datastore Name Failed\n", ds_name );

   }

   return 0;

} /* End of Update_temps() */

/******************************************************************

   Description:
      Interpolate missing levels in Environmental Wind Table.

   Input:
      u - array of U (East-West) wind components, in m/s.
      v - array of V (North-South) wind components, in m/s.
      height - array of data value heights, in Kft MSL.
      num_levels - number of elements in arrays u, v and height.
      radar_height - radar height, in Kft.

   Output:
      enw - pointer to Environmental Wind Table.

   Notes:
      The interpolation scheme first converts model data, inserts
      the data into the wind table taking the closest table index
      to the data value height, then interpolates levels which
      data not have data.

******************************************************************/
static A3cd97* Interpolate_enw_legacy( double *u, double *v, double *height,
                                       int num_levels, double radar_height ){

   int hgt_index, indx, i, j, k;
   double uc1, uc2, vc1, vc2;
   double uslope, vslope, unew, vnew;

   static A3cd97 enw;

   /* Initialize the ewt to missing value. */
   for( i = 0; i < LEN_EWTAB; i++ ){

      enw.ewtab[WNDSPD][i] = MTTABLE;
      enw.ewtab[WNDDIR][i] = MTTABLE;
      enw.newndtab[i][ECOMP] = MTTABLE;
      enw.newndtab[i][NCOMP] = MTTABLE;

   }

   /* Insert the model data into the environmental wind table. */
   for( i = 0; i < num_levels; i++ ){

      /* No need to process entries below the radar height index or
         at the height ceiling index.  The environmental windd table
         levels are defined in Kft AGL so we need to subtract the
         radar height form the height[i] values which are Kft MSL . */
      hgt_index = (int) (height[i] - radar_height);
      if( (hgt_index < 0 )
                   ||
          (hgt_index >= 70) )
         continue;

      /* Insert the windspeed and direction data into the Environmental
         Wind Table. */
      enw.ewtab[WNDSPD][hgt_index] = sqrt( u[i]*u[i] + v[i]*v[i] );
      if( u[i] < 0 )
         enw.ewtab[WNDDIR][hgt_index] = (PIOVER2 - atan(v[i]/u[i]))/DEGTORAD;

       else if( u[i] > 0 )
         enw.ewtab[WNDDIR][hgt_index] = (PI3OVER2 - atan(v[i]/u[i]))/DEGTORAD;

       else{

          if( v[i] < 0 )
             enw.ewtab[WNDDIR][hgt_index] = 0.0;

          else
             enw.ewtab[WNDDIR][hgt_index] = 180.0;

       }

   }

   /* Do For All entries in the wind table (Interpolate). */
   for( i = 0; i < LEN_EWTAB-1; i++ ){

      /* Does the current entry have valid data? */
      if( (enw.ewtab[WNDDIR][i] != MTTABLE)
                             &&
          (enw.ewtab[WNDSPD][i] != MTTABLE)) {

         /* Does the next entry have invalid data? */
         if( (enw.ewtab[WNDDIR][i+1] == MTTABLE)
                             ||
             (enw.ewtab[WNDSPD][i+1] == MTTABLE)){

            /* Interpolate between valid data levels. */
            for( j = i+2; j < LEN_EWTAB; j++ ){

               if( (enw.ewtab[WNDDIR][j] != MTTABLE)
                             &&
                   (enw.ewtab[WNDSPD][j] != MTTABLE)) {

                  uc1 = (-1) * enw.ewtab[WNDSPD][i] *
                        sin((double) (enw.ewtab[WNDDIR][i]*DEGTORAD));

                  uc2 = (-1) * enw.ewtab[WNDSPD][j] *
                        sin((double) (enw.ewtab[WNDDIR][j]*DEGTORAD));

                  vc1 = (-1) * enw.ewtab [WNDSPD][i] *
                        cos((double) (enw.ewtab[WNDDIR][i]*DEGTORAD));

                  vc2 = (-1) * enw.ewtab [WNDSPD][j] *
                        cos((double) (enw.ewtab[WNDDIR][j]*DEGTORAD));

                  uslope = (uc2 - uc1) / (j-i);
                  vslope = (vc2 - vc1) / (j-i);

                  indx = 1;

                  for( k = i+1; k < j; k++ ){

                     unew = uc1 + indx*uslope;
                     vnew = vc1 + indx*vslope;

                     indx++;

                     if( unew < 0 )
                        enw.ewtab[WNDDIR][k] = (PIOVER2 -
                                   atan(vnew/unew))/DEGTORAD;

                     else if( unew > 0 )
                        enw.ewtab[WNDDIR][k] = (PI3OVER2 -
                                   atan (vnew/unew))/DEGTORAD;

                     else{

                        if( vnew < 0 )
                           enw.ewtab[WNDDIR][k] = 0.0;

                        else
                           enw.ewtab[WNDDIR][k] = 180.0;

                     }

                     enw.ewtab[WNDSPD][k] = sqrt( vnew*vnew + unew*unew );

                  }

                  break;

               }

            }

         }

      }

   }

   /* Return the wind table. */
   return( &enw);

/* End of Interpolate_enw_legacy() */
}

/******************************************************************

   Description:
      Interpolate missing levels in Environmental Wind Table.

   Input:
      u - array of U (East-West) wind components, in m/s.
      v - array of V (North-South) wind components, in m/s.
      height - array of data value heights, in Kft.
      num_levels - number of elements in arrays u, v and height.
      radar_height - radar height in Kft.

   Output:
      enw - pointer to Environmental Wind Table.

   Notes:
      The interpolation scheme first interpolates the data values
      to the midpoint height of the wind table entries.  It then
      inserts these into the wind table, then interpolates levels
      which data not have data.

******************************************************************/
static A3cd97* Interpolate_enw( double *u, double *v, double *height,
                                int num_levels, double radar_height ){

   int j;

   /* Note: ref_height is Kft AGL and represents 0.5 Kft above ground
            level. */
   double ref_height = 0.5;

   static A3cd97 enw;

   /* Initialize the wind table. */
   for( j = 0; j < LEN_EWTAB; j++ ){

      enw.ewtab[WNDSPD][j] = MTTABLE;
      enw.ewtab[WNDDIR][j] = MTTABLE;
      enw.newndtab[j][ECOMP] = MTTABLE;
      enw.newndtab[j][NCOMP] = MTTABLE;

   }

   /* Do For All midpoint heights .... 0.5 Kft, 1.5 Kft, etc.
      Ignore heights below the radar (base_height) and above
      70,000 ft. */
   while( ref_height < 70.0 ){

      /* Reference height must be between levels. */
      for( j = 0; j < num_levels-1; j++ ){

         if( (ref_height >= (height[j] - radar_height))
                         &&
             (ref_height <= (height[j+1] - radar_height)) ){

            double ratio = (ref_height-(height[j] - radar_height))/(height[j+1]-height[j]);

            /* Perform the interpolation. */
            double v_int = v[j] + (v[j+1]-v[j])*ratio;
            double u_int = u[j] + (u[j+1]-u[j])*ratio;

            /* Note: hgt_index is relative to height Kft AGL. */
            int hgt_index = ref_height;

            /* No need to process data above the height ceiling index. */
            if( hgt_index >= 70 )
               continue;

            /* Convert interpolated values (u and v) to speed and direction. */
            enw.ewtab[WNDSPD][hgt_index] = sqrt((u_int*u_int)+(v_int*v_int));
            if( u_int < 0 )
               enw.ewtab[WNDDIR][hgt_index] = (PIOVER2-atan(v_int/u_int))/DEGTORAD;

            else if( u_int > 0 )
               enw.ewtab[WNDDIR][hgt_index] = (PI3OVER2-atan(v_int/u_int))/DEGTORAD;

            else{

               if( v_int < 0 )
                  enw.ewtab[WNDDIR][hgt_index] = 0.0;

               else
                  enw.ewtab[WNDDIR][hgt_index] = 180.0;

            }

            /* Save the components. */
            enw.newndtab[hgt_index][ECOMP] = RPGC_NINTD( u_int );
            enw.newndtab[hgt_index][NCOMP] = RPGC_NINTD( v_int );

            break;

         }

      }

      /* Increment the reference height (by 1 Kft). */
      ref_height += 1.0;

   }

   /* Return the wind table. */
   return( &enw);

/* End of Interpolate_enw() */
}

/******************************************************************
   Description:
      Interpolate to find the height of the "target" temperature.

   Inputs:
      temp1 - temperature at height "height1"
      temp2 - temperature at height "height2"
      height1 and height2 - heights of temp1 and temp2, respectively
      target - target temperature.

   Returns:
      The height of the "target" temperature.

   20120127 Ward CCR NA12-00024 Added equality check. If two temps
                                are equal, it returns the higher one.
******************************************************************/

double Find_height( double temp1,   double temp2,
                    double height1, double height2,
                    double target)
{
   double height = 0.0;

   if(temp1 == temp2)
   {
      if(height1 > height2)
         height = height1;
      else
         height = height2;
   }
   else
   {
      height = height1 + ((height2-height1)*(target-temp1))/(temp2-temp1);
   }

   return(height);

} /* end Find_height() */

/******************************************************************

   Description:
      DEAU LB update callback function.

   Notes:
      See lb man page for description of function arguments.

******************************************************************/
static void Deau_cb( int fd, LB_id_t msgid, int msg_info, void *arg ){

   int i;

   /* Determine which color tables were updated. */
   for( i = 0; i < N_upd_tables; i++ ){

      if( C_tabs[i].msg_id == msgid )
         C_tabs[i].updated = 1;

   }

/* End of Deau_cb() */
}

/******************************************************************

   Description:
      LB update callback function.

   Notes:
      See lb man page for description of function arguments.

******************************************************************/
static void Lb_cb (int fd, LB_id_t msgid, int msg_info, void *arg) {

   if( (Env_data_msg_fd == fd)
             &&
       (ORPGDAT_RUC_DATA_MSG_ID == msgid) )
      Env_data_msg_updated = 1;

/* End of Lb_cb() */
}

/******************************************************************

   Description
      Initializes the DEAU LB name and register for DEA update events.

******************************************************************/
static void Init_deau () {

   char *ac_ds_name;
   int ret, i;

   ac_ds_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
   if( ac_ds_name == NULL ){

      LE_send_msg( GL_ERROR, "ORPGDA_lbname( %d ) Failed",
                   ORPGDAT_ADAPT_DATA);
      exit (1);

   }

   DEAU_LB_name (ac_ds_name);

   for( i = 0; i < N_upd_tables; i++ ){

      C_tabs[i].msg_id = DEAU_get_msg_id (C_tabs[i].de_id);
      if( C_tabs[i].msg_id == 0xffffffff ){

         LE_send_msg( GL_ERROR,
                      "DE DB Message ID for %s Not Found", C_tabs[i].de_id);
	 exit (1);

      }

   }

   ret = DEAU_UN_register( "", Deau_cb );
   if( ret < 0 ){

      LE_send_msg (GL_ERROR, "DEAU_UN_register Failed. (%d)", ret);
      exit (1);

   }

/* End of Init_deau() */
}

/******************************************************************

   Description:
      Register for LB notification.

******************************************************************/
static void Init_lb_notification () {

   int ret;

   ret = ORPGDA_UN_register (ORPGDAT_ENVIRON_DATA_MSG,
                             ORPGDAT_RUC_DATA_MSG_ID,
                             Lb_cb );
    if (ret < 0) {
        LE_send_msg (GL_ERROR, "ORGPDA_UN_register failed: %d", ret);
        exit (1);
   }

   Env_data_msg_fd = ORPGDA_lbfd( ORPGDAT_ENVIRON_DATA_MSG );

/* End of Init_lb_notification() */
}

/**************************************************************************

   Description:
      This function terminates this process.

**************************************************************************/
static int Terminate( int sig, int sigtype ){

    LE_send_msg (GL_INFO,  "Terminating with - signal %d", sig);
    return (0);

/* End of Terminate() */
}

/**************************************************************************

   Description:
      This function reads command line arguments.

   Inputs:	
      argc - number of command arguments
      argv - the list of command arguments

   Return:	
      It returns 0 on success or -1 on failure.

**************************************************************************/
static int Read_options( int argc, char **argv ){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */

   Use_legacy_interpolation_method = 0;

   err = 0;
   while ((c = getopt (argc, argv, "lh?")) != EOF) {

      switch (c) {

         case 'l':
            Use_legacy_interpolation_method = 1;
            break;

         case 'h':
	 case '?':
	    Print_usage (argv);
	    break;

      }

   }

   return (err);

/* End of Read_options() */
}

/**************************************************************************

   Description:
      This function prints the usage info.

**************************************************************************/
static void Print_usage( char **argv ){

   printf( "Usage: %s (options)\n", argv[0] );
   printf( "       synchronize RPG structs to the adaptable data\n" );
   printf( "       Options:\n" );
   printf( "       -l Legacy style Interpolation of Env. Winds (Default: Disabled)\n" );
   printf( "       -h Help\n" );
   exit(0);

/* End of Print_usage() */
}

/**************************************************************************

   Description:
      Formats System Status Log message.

**************************************************************************/
static char * Format_message( time_t model_run_time, time_t valid_time,
                              time_t forecast_period, int model ){


   int yy_mrt, mm_mrt, dd_mrt, hr_mrt, min_mrt, sec_mrt;
   int yy_vt, mm_vt, dd_vt, hr_vt, min_vt, sec_vt;
   unsigned int period;

   static char message[ 132 ];
   char model_str[6];

   /* Initialize the message contents. */
   memset( message, 0, 132 );
   memset( model_str, 0, 6 );

   /* Convert times to dd/mm/yy and hr:min:sec */
   unix_time( &model_run_time, &yy_mrt, &mm_mrt, &dd_mrt, &hr_mrt,
              &min_mrt, &sec_mrt );

   unix_time( &valid_time, &yy_vt, &mm_vt, &dd_vt, &hr_vt, &min_vt, &sec_vt );

   period = forecast_period / 3600;

   /* Remove the century from the year. */
   if( yy_mrt >= 2000 )
      yy_mrt -= 2000;

   else
      yy_mrt -= 1900;

   /* What model is being used? */
   if( model == RUC80 )
      strcpy( model_str, "RUC80" );

   else if( model == RUC40 )
      strcpy( model_str, "RUC40" );

   else if( model == RUC13 )
      strcpy( model_str, "RUC13" );

   else
      strcpy( model_str, "RUC??" );

   /* Format the string. */
   sprintf( message,
            "%02d/%02d/%02d %02d:%02d %s %2dhr Forecast Valid At %02dZ",
            mm_mrt, dd_mrt, yy_mrt, hr_mrt, min_mrt, model_str, period, hr_vt );

   return message;

/* End of Format_message() */
}

/******************************************************************

   Description:
      New environmental model data received. Build model derived
      grid structures of external type RPGP_ext_data_t.

   Returns:
      -1 on error, non-negative number on success.

******************************************************************/
static int Update_grids(){

   char *buf = NULL;
   int model;
   int active, model_error = 1;
   int ret;
   int site_i_index = -1;
   int site_j_index = -1;
   double grid_lat, grid_lon;
   Siteadp_adpt_t site;

   RPGCS_model_attr_t *attrs = NULL;
   RPGCS_model_grid_data_t *grid_h_data = NULL;

   if( (ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE )) == ORPGSITE_FAA_REDUNDANT )
      active = ORPGRED_channel_state( ORPGRED_MY_CHANNEL );
   else
      active = ORPGRED_CHANNEL_ACTIVE;

   /* If the data has not been updated, do nothing. */
   if( (!Env_data_msg_updated) || (active != ORPGRED_CHANNEL_ACTIVE) )
      return 0;

   /* Originally set in Update_ewt() but now moved here */
   Env_data_msg_updated = 0;

   /* Get the model data. */
   if( (model = RPGCS_get_model_data( ORPGDAT_ENVIRON_DATA_MSG, RUC_ANY_TYPE, &buf )) < 0 ){

      if( model != LB_NOT_FOUND )
         LE_send_msg( GL_STATUS | GL_ERROR, 
                      "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );
      return -1;

   }

   /* Get the model attributes. */
   attrs = RPGCS_get_model_attrs( model, buf );
   if( attrs == NULL ){

      LE_send_msg( GL_STATUS | GL_ERROR, 
                   "MODEL DATA: Processing Failure, Can Not Be Used By RPG\n" );

      if( buf != NULL )
         RPGP_product_free( buf );

      return -1;

   }

   /* Get the Geopotential Height field. */
   grid_h_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_GH ); 

   /* Check that the necessary data are available 
      and the radar location is within the model grid  */
   if( grid_h_data != NULL ){

      /* There should be height data for at least one level. */
      if( grid_h_data->num_levels <= 0 )
         LE_send_msg( GL_INFO, "Error: grid_h_data->num_levels <= 0 (%d)\n",
                      grid_h_data->num_levels );

      else{
         /* Find the closest grid point to the radar location. */
         if( (ret = ORPGSITE_get_site_data( &site )) < 0 )
            LE_send_msg( GL_INFO, "ORPGSITE_get_site_data() Failed: %d\n", ret );

         else{
            /* Convert the site lat/lon to model grid point. */
            if( ( ret = RPGCS_lambert_latlon_to_grid_point( ((double) site.rda_lat)/1000.0, 
                                                            ((double) site.rda_lon)/1000.0, 
                                                            &site_i_index, &site_j_index )) < 0 ){
               if( ret == RPGCS_DATA_POINT_NOT_IN_GRID )
                  LE_send_msg( GL_INFO, "Site Lat/Lon Not Within Model Grid\n" );
               else
                  LE_send_msg( GL_INFO, "RPGCS_lambert_latlon_to_grid_point Failed\n" );

            }
            else{
               /* Convert the grid index to lat/lon. */
               if( RPGCS_lambert_grid_point_latlon( site_i_index, site_j_index,
                                                    &grid_lat, &grid_lon ) < 0 )
                  LE_send_msg( GL_INFO, "RPGCS_lambert_grid_point_latlon Failed\n" );

               else
                  /* Set the model error flag to 0 (no errors). */
                  model_error = 0;
            }
         }
      }

   }

   if( !model_error ){

      /* Update the Freezing Height Grid Data. */
      RPGC_log_msg( GL_INFO, "Updating Model-derived Freezing Height Grids ...\n" );
      Create_FreezingHeight_Grid( model, buf, attrs, site_i_index, site_j_index );

      /* Update the CIP Grid Data. */
      RPGC_log_msg( GL_INFO, "Updating Model-derived CIP Grid ...\n" );
      Create_CIP_Grid( model, buf, attrs, site_i_index, site_j_index );

   }
   else{

      RPGC_log_msg( GL_INFO, "model_error .... Freezing Height and CIP Grids NOT Updated\n" );

   }

   /* Free the data */
   if( grid_h_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_h_data );

   if( attrs != NULL )
      free( attrs );

   if( buf != NULL ){
   
      RPGP_product_free( buf );
      return 0;

   }

   return 0;

/* End of Update_grids() */
}
