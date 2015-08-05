/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 17:45:11 $
 * $Id: melting_layer.c,v 1.19 2014/10/03 17:45:11 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */

/********************************************************************
**
**	File:		melting_layer.c
**	Author:		Ning Shen
**	Date:		May 2, 2007
**	Version:	1.0
**
**	Description:
**	------------
**
**	The melting layer algorithm will take radial data generated
**	by HCA and generate a volume based product which contains
**	melting layer's top and bottom in km for each azimuth.
**      It also ingests available model-based freezing height data to
**      enhance radar-based mlda model-based calculations of mlda.
**
**      Change History:
**      --------------
**      NA11-00374 Increased search height from 6 km to 8 km.
**          Build 13.0   01/08/2012  Brian Klein
**
**      NA11-00375 Reports model obtained height if it finds or
**      the ML detection height is less than 1 km ARL.
**          Build 13.0   01/10/2012  Brian Klein
**
**      ??         Modified to handle model data ingest, merge radar and model 
**      based estimates of freezing height. Also utilizes interpolation instead 
**      of averaging for radar based missing radials. Finally, the previous 1 km
**      test added (above) for Build 13.0 is removed.
**          Build 14.0   08/31/2012  Robert Hallowell
**
********************************************************************/

#include <orpg.h>
#include <orpgred.h> 
#include <rpgc.h>
#include <rpgcs.h>
#include <rpg_port.h>
#include <hca.h> 
#include <orpgsite.h>
#include <rpgcs_model_data.h>

#define MLDA_DEBUG 0

/* Constants for model-based calculations */
#define MDL_NFILTER   8          /* Number of radials to smooth model data */
#define MDL_MIN_VALID 3          /* Minimum number of valid radials to use model */
#define MDL_ADJUST_TOP     0.0   /* Model correction (km) to radar-sensed values of melting layer tops */
#define MDL_ADJUST_BOTTOM  0.0   /* Model correction (km) to radar-sensed values of melting layer bottom */
#define MDL_MIN_HT         0.0   /* The minimum height allowed for top/bottom (km above radar feedhorn height) */

/* Flags for melting layer radials */
#define RF_None         0   /* No data for this radial */
#define RF_Radar        1   /* From radar-based MLDA */
#define RF_AvgRadar     2   /* Uses average radar for BADVAL (not used - compatability to old software) */
#define RF_CoastRadar   3   /* Gap in good radar is small enough -- interpolate but set a coast flag */
#define RF_IntRadar     4   /* Gap in good radar is too large -- interpolate and allow it to be replaced with model */
#define RF_Model        5   /* Uses Model-based MLDA */
#define RF_AvgModel     6   /* Uses average Model-based for BADVAL */
#define RF_MdlIntRadAvg 7   /* Use weighted average of top/bottom for model and radar -- wet snow thresh used for weighting */
#define RF_Mixed        9   /* Uses radar-based Top, Model-based Bottom (future use) */

#define MLDA_UseType_Default 0 
#define MLDA_UseType_Radar   1
#define MLDA_UseType_Model   2

#define MAX_NUM_AZ      360   /* Number of azimuths */
#define MAX_NUM_HEIGHTS  80   /* max number heights from 0 to 8.0 km */
#define MAX_NUM_VOL       8   /* 8 volumes */
#define OUTBUF_SIZE1  3000    /* size in bytes for ML data output */
#define OUTBUF_SIZE2  
#define ML_DEA_FILE "alg.mlda."
#define HAIL_DEA_FILE "alg.hail."
#define VCP31 31
#define VCP32 32
#define PI 3.1416
#define MIL_SEC_TO_MIN (1.0/(1000.0 * 60.0))
#define MINUTES_PER_DAY 1440   /* 24 * 60 */
#define DEG_TO_RAD (PI/180.0)
#define KM_PER_KFT 0.3048
#define METERS_PER_KM 1000.0
#define ML_NO_DATA -999.0
#define HALF_DEG_AZ 1
#define ONE_DEG_AZ  2
#define MAX_ARRAY_SIZE MAX_BASEDATA_REF_SIZE  /* 1840 */

  /* melting layer data structure */
typedef struct
{
  float top;
  float bottom;
} ML_data_t;

  /*********** local global variables added for model calculations **************/
static int ML_model_valid = 0;
static ML_data_t Model_ml_data[MAX_NUM_AZ];
static ML_data_t Raw_Radar_ml_data[MAX_NUM_AZ];
static ML_data_t Radar_ml_data[MAX_NUM_AZ];
static ML_data_t Merged_ml_data[MAX_NUM_AZ];
static int Radial_flag[MAX_NUM_AZ];
static int Model_Radial_flag[MAX_NUM_AZ];
static int radar_thr[MAX_NUM_AZ];
static float mdl_cross[MAX_NUM_AZ];
static int sum_cnt[MAX_NUM_AZ];

RPGCS_model_attr_t *Model_Attrs;
RPGCS_model_grid_data_t *grid_frz_data;
static time_t tm_model_valid = 0;
static int Frz_data_msg_updated = 0;
static int IJK = 0;  /* Internal volume counter */
static char date_time_str[30];

  /*********** local global variables **************/
static int ML_prod_generated = 0;
static int Abort_flag = 0;
static int Current_vol_index = 0;
static int ML_not_found = 0;
static int Time_gap_too_big_flag = 0;
static int Azm_reso = ONE_DEG_AZ;
static int Max_num_heights;
static float Wet_snow_weight[MAX_NUM_VOL][MAX_NUM_AZ][MAX_NUM_HEIGHTS];
static ML_data_t Default_ml_data[MAX_NUM_AZ];
static const float IR = 1.21;
static const float RE = 6371;  /* earth radius in km */
static int Num_vol;
static float Radar_height;/*Feedhorn, km above MSL from radial header*/

  /* adaptation data variables */
static double ML_depth;
static double Max_top_height;
static double Height_interval;
static double Upper_RhoHV_limit;
static double Lower_RhoHV_limit;
static double Low_RhoHV_profile;
static double Upper_Zmax_profile;
static double Lower_Zmax_profile;
static double Upper_Z_limit;
static double Lower_Z_limit;
static double Upper_ZDRmax_pro;
static double Lower_ZDRmax_pro;
static double Half_window_size;
static double Upper_elev_limit;
static double Lower_elev_limit;
static double High_percentile;
static double Low_percentile;
static double Min_wet_snow_sum;
static double Low_snow_thresh;
static double Min_rad_mod_wt;
static double Min_snr; 
static double Melting_Layer_Source;
static double Default_top;
static double Max_vol_time_gap;
static double Num_vol_clear;
static double Num_vol_precip;
static double Model_max_time_gap;
static double Model_half_window;
static double Model_min_range;
static double Model_max_range;
static double Radar_maxgap_interval;
static double Radar_radonly_thr;
static double Model_min_bot_delta;
static double Model_max_bot_delta;
static double Use_avg_top_clipping;

  /* local function prototype */
static int Process_control();
static int Get_default_ML_value(ML_data_t *);
static int Get_adapt_data();
static int Find_wet_snow(Base_data_header *, float *, float *, float *, float *, float *);
static float Compute_height_from_range(float, float);
static float Compute_range_from_height(float, float);
static int Get_height_index(float, float);
static int Get_range_index(float, float, float);
static int Calculate_melting_layer();
static float Get_last_top(ML_data_t * );
static int Send_ML_data(ML_data_t *, void *);
/* static int Set_ML_to_zero(ML_data_t *); */
static int Get_dualpol_data(Base_data_header *, float *, float *, float *, float *, float *);
static float Compute_elev_weight(float);
static int Calculate_model_melting_layer( void );
static void LB_cb_frz( int fd, LB_id_t msgid, int msg_info, void *arg );
static void Log_MLDA_Radials( char * );

/***************************** main() *******************************
**
**	Description:
**	Inputs:		n/a
**	Outputs:	n/a
**	Return:	
**	Globals:	Wet_snow_weight array
**	Notes:
**
*/

int main(int argc, char * argv[])
{
  int i, j, k;
  int retval = 0;

    /* 
    ** Register input and output based on
    ** task_attr_table and product_attr_table.
    */
  RPGC_reg_io(argc, argv);

    /* ORPG task initialization routine */
  RPGC_task_init(VOLUME_BASED, argc, argv);

  /* Open and register LB notification for Freezing Height Grid Data */
  retval = RPGC_data_access_open( MODEL_FRZ_GRID, LB_READ );
  if (retval < 0)
    RPGC_log_msg( GL_ERROR, "RPGC_data_access_open_failed for MODEL_FRZ_GRID: %d", retval);

  retval = RPGC_data_access_UN_register( MODEL_FRZ_GRID, MODEL_FRZ_ID, LB_cb_frz );
  if (retval < 0)
    RPGC_log_msg( GL_ERROR, "Unable to Register for UN Notification: %d\n", retval );
 
  /* Initialize flag values */
  for ( i=0; i<MAX_NUM_AZ; i++ )
  {
    Model_Radial_flag[i] = RF_None;
    Radial_flag[i] = RF_None;
  }

    /* initialize Wet_snow_weight */
  for(i = 0; i < MAX_NUM_VOL; i ++)
    for(j = 0; j < MAX_NUM_AZ; j++)
      for(k = 0; k < MAX_NUM_HEIGHTS; k++)
        Wet_snow_weight[i][j][k] = 0;

    /*  Algorithm Control Loop(ACL) */
  while(1)
  {
      /*
      ** system call to block the algorithm until good data is received
      ** and the product is requested
      */
    RPGC_wait_act(WAIT_DRIVING_INPUT);

      /* here is the entry point of the algorithm logic */
    Process_control();
  }
 
  return 0;
}  /* end of main() */


/************************** Process_control() ***********************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:	
**	Globals:	
**	Notes:
*/

 /* (CPT&E Label A) main process */
int Process_control()
{
  int opstat, elev_index, vcp, eov_flag;
  Base_data_header * bhdr_ptr;

  float data_z[MAX_ARRAY_SIZE];
  float data_hydro[MAX_ARRAY_SIZE];
  float data_RhoHV[MAX_ARRAY_SIZE];
  float data_ZDR[MAX_ARRAY_SIZE];
  float data_SNR[MAX_ARRAY_SIZE];

  float elev_angle = 0;
  static int vol_time = 0;
  static int pre_vol_time = -1;

  static short  rad_vol_date;
  static int    rad_vol_time;
  int    rad_hr, rad_min, rad_sec, rad_msec;
  int    year, month, day;
  int    radar_model_time_diff = 0;
  time_t rad_unix_time = 0;

    /* reset the flags at the beginning of volume */
  ML_prod_generated = 0;
  Abort_flag = 0;
  eov_flag = 0;
  ML_not_found = 0;

    /* (CPT&E Label B) get adaptation data */
  if(Get_adapt_data() < 0)
  {
    RPGC_abort();
    return -1;
  }

    /* convert default top from kft MSL to km ARL*/
  Default_top *= KM_PER_KFT;
  Radar_height = ORPGSITE_get_int_prop(ORPGSITE_RDA_ELEVATION) * FT_TO_KM;
  Default_top -= Radar_height;

  /* After adjustment, make sure Default_top is not below radar level */
  if (Default_top < 0.0) Default_top = 0.0;

  /* Verify the type of MLDA that we are running */
  if ( Melting_Layer_Source == MLDA_UseType_Radar )
    RPGC_log_msg(GL_INFO, "Melting_Layer_Source is set to Radar_Based\n");
  else if ( Melting_Layer_Source == MLDA_UseType_Model )
    RPGC_log_msg(GL_INFO, "Melting_Layer_Source is set to use both Radar and Model data (Model_Enahnced)\n");
  else
    RPGC_log_msg(GL_INFO, "Melting_Layer_Source is set to use environmental zero degree height (no radar/model) (RPGC_0C_Hgt)\n");

    /* calculate max number of heights */
  Max_num_heights = (int)(Max_top_height * METERS_PER_KM / Height_interval);

    /* calculate model melting layer if freezing height grid is updated and set flag if valid */ 
  if ( Frz_data_msg_updated == 1 && (int)Melting_Layer_Source == MLDA_UseType_Model )
  { 
    if ( Calculate_model_melting_layer() == 1 )
      ML_model_valid = 1;
  }

  /* if user changes use of MLDA from model to radar or default (via gui) set flag to not valid */
  if ( ((int)Melting_Layer_Source == MLDA_UseType_Radar || (int)Melting_Layer_Source == MLDA_UseType_Default) 
             && ML_model_valid == 1 )
    ML_model_valid = 0;

    /* ------ Start of radial data ingest loop ------ */
  while(1)
  {
    /* Get the input radial data from HCA */
    bhdr_ptr = (Base_data_header *)RPGC_get_inbuf_by_name("DP_BASE_HC_AND_ML", &opstat);

    if(opstat != NORMAL)
    {
      RPGC_log_msg(GL_INFO, ">> Process_control(): RPGC_get_inbuf_by_name() error");
      RPGC_abort();
      return -1;
    }

      /* (CPT&E Label D) Get VCP and Number of volumes needed */
    if(bhdr_ptr->status == BEG_VOL)
    {
      /* (CPT&E Label C) Get default ML data */
      Get_default_ML_value(Default_ml_data);

      /* Get the volume time and convert to unix time */
      rad_vol_time = bhdr_ptr->begin_vol_time;
      rad_vol_date = bhdr_ptr->begin_vol_date;
      RPGCS_convert_radial_time(rad_vol_time, &rad_hr, &rad_min, &rad_sec, &rad_msec);
      RPGCS_julian_to_date(rad_vol_date, &year, &month, &day);
      RPGCS_ymdhms_to_unix_time(&rad_unix_time, year, month, day, rad_hr, rad_min, rad_sec);
      sprintf(date_time_str,"volTime: %d%02d%02d_%02d%02d%02d", year, month, day, rad_hr, rad_min, rad_sec);

      /* Check the model data valid time to see if it's too old */
      if ( ML_model_valid == 1 )
      {
        int Myear=0, Mmonth=0, Mday=0, Mhour=0, Mminute=0, Msecond=0;

        RPGCS_unix_time_to_ymdhms( tm_model_valid, &Myear, &Mmonth, &Mday, &Mhour, &Mminute, &Msecond);

        radar_model_time_diff = abs(rad_unix_time - tm_model_valid) / 60;
        RPGC_log_msg(GL_INFO, "volTime: %d%02d%02d_%02d%02d%02d  modelTime: %d%02d%02d_%02d%02d%02d  time_diff: %d\n",
                     year, month, day, rad_hr, rad_min, rad_sec,
                     Myear, Mmonth, Mday, Mhour, Mminute, Msecond, radar_model_time_diff);

	if ( radar_model_time_diff > (int)Model_max_time_gap )
        {
          RPGC_log_msg(GL_INFO, ">> Model data is too old: %d minutes from volume time [Model_max_time_gap=%d]",
                       radar_model_time_diff, (int)Model_max_time_gap);
          ML_model_valid = 0; 
        }
      }
      else
        RPGC_log_msg(GL_INFO, "volTime: %d%02d%02d_%02d%02d%02d  modelTime: N/A, too old, or Melting_Layer_Source not set to Model\n",
                     year, month, day, rad_hr, rad_min, rad_sec);

      vcp = bhdr_ptr->vcp_num;

      if(vcp == VCP31 || vcp == VCP32)
        Num_vol = (int)Num_vol_clear;
      else
        Num_vol = (int)Num_vol_precip;

      if (Num_vol > MAX_NUM_VOL)
      {
        RPGC_log_msg(GL_ERROR, ">> Process_control(): Number of volumes %d > maximum %d",
                     Num_vol, MAX_NUM_VOL);
        RPGC_abort();
        exit(1);
      }

        /* 
        ** Check the volume time to see if elapsed volume
        ** time is greater than Max_vol_time_gap.
        */
      vol_time = (int)(bhdr_ptr->begin_vol_time * MIL_SEC_TO_MIN);
      if(pre_vol_time < 0)
        pre_vol_time = vol_time;
      else
      {
        int i, j, k;
        int time_diff = 0;
        
        time_diff = vol_time - pre_vol_time;
        if (time_diff < 0)
          time_diff += MINUTES_PER_DAY;

          /*
          ** If the time gap is too big, discard previous
          ** volume.
          */ 
        if(time_diff > (int)Max_vol_time_gap)
        {
          Time_gap_too_big_flag = 1; 
          Current_vol_index = 0;
           
          /* clear all the previous Wet_snow_weight */
          for(i = 0; i < MAX_NUM_VOL; i ++)
            for(j = 0; j < MAX_NUM_AZ; j++)
              for(k = 0; k < MAX_NUM_HEIGHTS; k++)
                Wet_snow_weight[i][j][k] = 0;

          RPGC_log_msg(GL_INFO, ">> Process_control(): Time gap exceeds threshold");
        }
        else {

          /* Initialize the wet snow points for this volume scan only */
          for(j = 0; j < MAX_NUM_AZ; j++)
             for(k = 0; k < MAX_NUM_HEIGHTS; k++)
                 Wet_snow_weight[Current_vol_index][j][k] = 0;

          Time_gap_too_big_flag = 0;

        }

        pre_vol_time = vol_time;
      }

    } /* end of if(bhdr_ptr->status == BEG_VOL) */

      /* Get the elevation angle at the start of elevation/volume */    
    if(bhdr_ptr->status == BEG_ELEV || bhdr_ptr->status == BEG_VOL)
    {
      elev_index = bhdr_ptr->rpg_elev_ind;
      elev_angle = (float)bhdr_ptr->target_elev * 0.1; 
      Azm_reso = (int)bhdr_ptr->azm_reso;
    }

    /*
    ** (CPT&E Label E) 
    ** Check the elevation angle:
    **   1. Do nothing for elevations that are lower than  4.0 (adaptable)
    **   2. Collect wet snow points for elevations between 4.0 and 10.0
    **   3. Calculate melting layer once elevation is greater than 10.0 (adaptable)
    ** Note: Calculate_melting_layer() should only be called once per volume.
    */ 
    if(elev_angle >= (float)Lower_elev_limit && elev_angle <= (float)Upper_elev_limit)
    {
       /* Get dual-pol data out of each radial */
       if(Get_dualpol_data(bhdr_ptr, data_z, data_hydro, 
                           data_RhoHV, data_ZDR, data_SNR))
       {
         RPGC_rel_inbuf((void *)bhdr_ptr);
         RPGC_abort_because(PGM_DISABLED_MOMENT);
         break;
       }

       /* looking for wet snow */
       Abort_flag = Find_wet_snow(bhdr_ptr, data_z, data_hydro,
                                  data_RhoHV, data_ZDR, data_SNR);

       /* 
       ** Handle the case in which volume scan ends 
       ** before reaching the upper elevation limit.
       */
       if(bhdr_ptr->status == END_VOL && Abort_flag == 0)
       {
          eov_flag = 1;
          Abort_flag = Calculate_melting_layer();
       }
    }
    else if(elev_angle > (float)Upper_elev_limit)
    {
       Abort_flag = Calculate_melting_layer();
    } 

      /* release input radial buffer */
    RPGC_rel_inbuf((void *)bhdr_ptr);

      /* exit radial ingest loop */
    if(Abort_flag)
    {
      RPGC_abort();
      break;
    }
    else if(ML_prod_generated)
    {
      if(!eov_flag)
        RPGC_abort_remaining_volscan();

      Current_vol_index = (Current_vol_index + 1) % MAX_NUM_VOL;
      break;
    } 
  }  /* ------ End of while(1) - radial data ingest loop ------ */

  return 0;
}  /* end of Process_control() */

/*************************** Get_adapt_data() ***************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:	
**	Globals:	
**	Notes:
**
*/

int Get_adapt_data()
{
  if(RPGC_ade_get_values(ML_DEA_FILE, "ml_depth", &ML_depth) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-ml_depth");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "max_top_height", &Max_top_height) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-max_top_height");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "height_interval", &Height_interval) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-height_interval");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "upper_RhoHV_limit", &Upper_RhoHV_limit) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-upper_RhoHV_limit");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "lower_RhoHV_limit", &Lower_RhoHV_limit) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-lower_RhoHV_limit");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "low_RhoHV_profile", &Low_RhoHV_profile) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-low_RhoHV_profile");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "upper_Zmax_profile", &Upper_Zmax_profile) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-upper_Zmax_profile");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "lower_Zmax_profile", &Lower_Zmax_profile) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-lower_Zmax_profile");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "upper_Z_limit", &Upper_Z_limit) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-upper_Z_limit");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "lower_Z_limit", &Lower_Z_limit) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-lower_Z_limit");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "upper_ZDRmax_pro", &Upper_ZDRmax_pro) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-upper_ZDRmax_pro");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "lower_ZDRmax_pro", &Lower_ZDRmax_pro) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-lower_ZDRmax_pro");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "half_window_size", &Half_window_size) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-half_window_size");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "upper_elev_limit", &Upper_elev_limit) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-upper_elev_limit");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "lower_elev_limit", &Lower_elev_limit) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-lower_elev_limit");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "high_percentile", &High_percentile) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-high_percentile");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "low_percentile", &Low_percentile) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-low_percentile");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "min_wet_snow_sum", &Min_wet_snow_sum) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-min_wet_snow_sum");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "low_snow_thresh", &Low_snow_thresh) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-low_snow_thresh");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "min_rad_mod_wt", &Min_rad_mod_wt) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-min_rad_mod_wt");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "min_snr", &Min_snr) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-min_snr");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "Melting_Layer_Source", &Melting_Layer_Source) != 0) 
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-Melting_Layer_Source");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "vol_time_gap", &Max_vol_time_gap) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-miss_vol_num");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "num_vol_clear", &Num_vol_clear) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-num_vol_clear");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "num_vol_precip", &Num_vol_precip) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-num_vol_precip");
    return -1;
  }

  if(RPGC_ade_get_values(HAIL_DEA_FILE, "height_0", &Default_top) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-height_0");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "mdl_time_gap", &Model_max_time_gap) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-mdl_time_gap");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "mdl_half_window_size", &Model_half_window) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-mdl_half_window_size");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "mdl_min_range", &Model_min_range) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-mdl_min_range");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "mdl_max_range", &Model_max_range) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-mdl_max_range");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "radar_maxgap_interval", &Radar_maxgap_interval) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-radar_maxgap_interval");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "radar_radonly_thr", &Radar_radonly_thr) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-radar_radonly_thr");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "mdl_min_bot_delta", &Model_min_bot_delta) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-mdl_min_bot_delta");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "mdl_max_bot_delta", &Model_max_bot_delta) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-mdl_max_bot_delta");
    return -1;
  }

  if(RPGC_ade_get_values(ML_DEA_FILE, "use_avg_top_clipping", &Use_avg_top_clipping) != 0)
  {
    RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value()-use_avg_top_clipping");
    return -1;
  }

  return 0;
}  /* end of Get_adapt_data() */

/************************** Get_dualpol_data() **************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:	
**	Globals:	
**	Notes:
*/

int Get_dualpol_data(Base_data_header * hdr_ptr, float * data_z, float * data_hydro,
                     float * data_RhoHV, float * data_ZDR, float * data_SNR)
{
  int i;
  unsigned char * in_data_ptr;
  float * z_ptr = NULL;
  float * hydro_ptr = NULL;
  float * RhoHV_ptr = NULL;
  float * ZDR_ptr = NULL;
  float * SNR_ptr = NULL;
  Generic_moment_t z_hdr, RhoHV_hdr, ZDR_hdr, SNR_hdr, hydro_hdr;

  if((in_data_ptr = (unsigned char *)RPGC_get_radar_data((void *)hdr_ptr, RPGC_DSMZ, &z_hdr)) == NULL)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGC_get_radar_data() error for Z");
    return 1;
  }

  if(RPGCS_radar_data_conversion((void *)in_data_ptr, &z_hdr, ML_NO_DATA, ML_NO_DATA, &z_ptr) != 1)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGCS_radar_data_conversion() error for Z");
    if (z_ptr != NULL) free(z_ptr);
    return 1;
  }
  else
  {
    for(i = 0; i < z_hdr.no_of_gates; i++)
      data_z[i] = z_ptr[i]; 
  }

  if((in_data_ptr = (unsigned char *)RPGC_get_radar_data((void *)hdr_ptr, RPGC_DRHO, &RhoHV_hdr)) == NULL)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGC_get_radar_data() error for RhoHV");
    return 1;
  }

  if(RPGCS_radar_data_conversion((void *)in_data_ptr, &RhoHV_hdr, ML_NO_DATA, ML_NO_DATA, &RhoHV_ptr) != 1)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGCS_radar_data_conversion() error for RhoHV");
    if (RhoHV_ptr != NULL) free(RhoHV_ptr);
    return 1;
  }
  else
  {
    for(i = 0; i < RhoHV_hdr.no_of_gates; i++)
      data_RhoHV[i] = RhoHV_ptr[i]; 
  }

  if((in_data_ptr = (unsigned char *)RPGC_get_radar_data((void *)hdr_ptr, RPGC_DZDR, &ZDR_hdr)) == NULL)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGC_get_radar_data() error for ZDR");
    return 1;
  }

  if(RPGCS_radar_data_conversion((void *)in_data_ptr, &ZDR_hdr, ML_NO_DATA, ML_NO_DATA, &ZDR_ptr) != 1)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGCS_radar_data_conversion() error for ZDR");
    if (ZDR_ptr != NULL) free(ZDR_ptr);
    return 1;
  }
  else
  {
    for(i = 0; i < ZDR_hdr.no_of_gates; i++)
      data_ZDR[i] = ZDR_ptr[i]; 
  }

  if((in_data_ptr = (unsigned char *)RPGC_get_radar_data((void *)hdr_ptr, RPGC_DSNR, &SNR_hdr)) == NULL)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGC_get_radar_data() error for SNR");
    return 1;
  }

  if(RPGCS_radar_data_conversion((void *)in_data_ptr, &SNR_hdr, ML_NO_DATA, ML_NO_DATA, &SNR_ptr) != 1)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGCS_radar_data_conversion() error for SNR");
    if (SNR_ptr != NULL) free(SNR_ptr);
    return 1;
  }
  else
  {
    for(i = 0; i < SNR_hdr.no_of_gates; i++)
      data_SNR[i] = SNR_ptr[i]; 
  }

    /* Get the hydro data from the radial data */
  memcpy((void *)&hydro_hdr.name, "DHCA", 4);
  if((in_data_ptr = (unsigned char *)RPGC_get_radar_data((void *)hdr_ptr, RPGC_DANY, &hydro_hdr)) == NULL)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGC_get_radar_data() error for DHCA");
    return 1;
  }

    /* 
    ** For HCA, the value 0 is not the flag value for "below_threshold" 
    ** and 1 is not the flag value for "range_folded". We pass 0 and 1 for
    ** this function to convert 0 to 0 and 1 to 1.
    */
  if(RPGCS_radar_data_conversion((void *)in_data_ptr, &hydro_hdr, 0, 1, &hydro_ptr) != 1)
  {
    RPGC_log_msg(GL_INFO, ">> Get_dualpol_data(): RPGCS_radar_data_conversion() error for DHCA");
    if (hydro_ptr != NULL) free(hydro_ptr);
    return 1;
  }
  else
  {
    for(i = 0; i < hydro_hdr.no_of_gates; i++)
      data_hydro[i] = hydro_ptr[i]; 
  }

  free(z_ptr);
  free(hydro_ptr);
  free(RhoHV_ptr);
  free(ZDR_ptr);
  free(SNR_ptr);
  return 0;
}  /* end of Get_dualpol_data() */

/*************************** Find_wet_snow() ***************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:	
**	Globals:	
**	Notes:
**
*/

/*(CPT&E Label J) */

int Find_wet_snow(Base_data_header * hdr_ptr, float * z_ptr, float * hydro_ptr,
                  float * RhoHV_ptr, float * ZDR_ptr, float * SNR_ptr)
{
  static float elev_weight = 0;
  static int stop_range_index = 0;
  int i, j, height_index, range_index;
  float Zmax = -100.0, ZDRmax = -100.0; 
  int Zmax_index = -1, ZDRmax_index= -1;
  int az_index = 0;
  static float bin_size = 0;

    /* calculate elev_weight and stop_range_index for each elevation scan */
  if(hdr_ptr->status == BEG_ELEV)
  {
    elev_weight = Compute_elev_weight(hdr_ptr->elevation);
    
      /* get the bin size in KM */
    bin_size = (float)hdr_ptr->surv_bin_size/METERS_PER_KM;

      /* 
      ** Based on the cut off height 6 km (adaptable), calculate
      ** stop range bin for each elevation scan.
      */
    stop_range_index = Get_range_index(hdr_ptr->elevation, 
                          (float)Max_top_height, bin_size); 
    if(stop_range_index > hdr_ptr->n_surv_bins)
      stop_range_index = hdr_ptr->n_surv_bins;

  }  /* end of if(hdr_ptr->status == BEG_ELEV) */ 
   
    /*
    ** az_index represnts 1-degree wide radial (0 to 359) 
    */
  az_index = (int)(hdr_ptr->azimuth) % MAX_NUM_AZ;

    /* radial process loop */
  for(i = 0; i < stop_range_index; i++)
  {
      /*
      **  Wet snow filter
      */
    if((int)hydro_ptr[i] != GC && (int)hydro_ptr[i] != BI &&
       (int)hydro_ptr[i] != UK && (int)hydro_ptr[i] != NE &&
       SNR_ptr[i] > (float)Min_snr)
    {
      if(z_ptr[i] > (float)Lower_Z_limit && z_ptr[i] < (float)Upper_Z_limit &&
         RhoHV_ptr[i] > (float)Lower_RhoHV_limit && 
         RhoHV_ptr[i] < (float)Upper_RhoHV_limit)
      {
          /*
          ** Calculate the height index for each radial bin
          */
        height_index = Get_height_index(hdr_ptr->elevation, i*bin_size);

        if(height_index >= 0 && height_index < Max_num_heights)
        {
            /*
            ** The wet snow is most likely to be found 0.5 km above 
            ** the current location.
            */
          float temp_height = (float)ML_depth;
          temp_height += Compute_height_from_range(hdr_ptr->elevation, 
                              i*bin_size);  

          range_index = Get_range_index(hdr_ptr->elevation, temp_height,
                                   bin_size);         
          if (range_index > hdr_ptr->n_surv_bins)
            range_index = hdr_ptr->n_surv_bins;

            /* 
            ** Initialize and Search for Zmax and ZDRmax from i to range_index
            */
          Zmax = -1000.0;
          ZDRmax = -1000.0;
          
          for (j = i; j < range_index; j++)
          {
            if(Zmax < z_ptr[j] && SNR_ptr[j] > (float)Min_snr)
            {
              Zmax = z_ptr[j];
              Zmax_index = j;
            }

            if(ZDRmax < ZDR_ptr[j] && SNR_ptr[j] > (float)Min_snr)
            {
              ZDRmax = ZDR_ptr[j];
              ZDRmax_index = j;
            }
          }  /* end of for (j = i; j < range_index; j++) */

            /* check Zmax & ZDRmax against profile values */
          if(Zmax > (float)Lower_Zmax_profile && Zmax < (float)Upper_Zmax_profile &&
             RhoHV_ptr[Zmax_index] > (float)Low_RhoHV_profile)
          {
            if(ZDRmax > (float)Lower_ZDRmax_pro && ZDRmax < Upper_ZDRmax_pro &&
               RhoHV_ptr[ZDRmax_index] > (float)Low_RhoHV_profile)
            {
               if(Azm_reso == ONE_DEG_AZ)
                 Wet_snow_weight[Current_vol_index][az_index][height_index] += 
                                                          1.0*(1 + elev_weight);
               else  /* 0.5 degree radial */
               {
                   
                   Wet_snow_weight[Current_vol_index][az_index][height_index] += 
                                                          1.0*(1 + elev_weight);
               }
            }
          }  /* end of if(Zmax > (float)Lower_Zmax_profile &&  ...) */
          
        }  /* end of if(height_index >= 0 && height_index < Max_num_heights) */
      }  /* end of if(z_ptr[i] > lower_z_limit && z_ptr[i] < upper_z_limit && ...) */
    }  /* end if(hydro_ptr[i] != GC && hydro_ptr[i] != BI && ....) */
  }  /* end of for(i = 0; i < stop_range_index; i++) */

  return 0; 
}  /* end of Find_wet_snow() */

/********************* Compute_elev_weight() ******************************
**
*/

/*(CPT&E Label I) */

float Compute_elev_weight(float elev_angle)
{
    /*
    ** First determine the Inverse Gate Ratio, which is the inverse of the
    ** ratio of gates that this elev has as compared to the 4.3 degree elevation
    ** of a beam passing through 2.0km to 2.5km.
    ** There are about 3x as many gates at 4.3 than at 10.0. To equal weight the
    ** detections at 4.3 and 10.0 you need to multiply the gates at 10 deg by 3.
    */
  float gate_ratio = 0.36 * elev_angle - 0.56;

    /*
    ** Second determine the accuracy ratio, which is the approximation of the
    ** difference in the reliability of the detection.
    ** At lower levels the beam is smeared more and it is wider than at higher levels.
    ** This equation is more of a "feel" than a hard scientific analysis and may
    ** change later when there is more data to support a specific implementation.
    ** The ratio is normalized for an elev of 10 degrees at 2km.
    */
  float acc_ratio = 1.0 - ( ((float)Upper_elev_limit - elev_angle)/(float)Upper_elev_limit);

  return (gate_ratio * acc_ratio); 
}  /* end of Compute_elev_weight() */

/********************* Compute_height_from_range() ********************
**
**	Description:
**	Inputs:		elev - elevation angle in degree
**			range - slant range in km
**	Outputs:	
**	Return:		height(km) calculated from inputs
**	Globals:	
**	Notes:
*/

float Compute_height_from_range(float elev, float range)
{
  float sin_elev;
  float height;
 
  sin_elev = sin(elev * DEG_TO_RAD); 
  height = range * sin_elev + (range * range)/(2 * IR * RE); 

  return height; 
}  /* end of Compute_height_from_range() */

/******************** Compute_range_from_height() *******************
**
**	Description:
**	Inputs:		elev - elevation angle in degree
**			height - height in km
**	Outputs:	
**	Return:		slant range(km) calculated from inputs.
**	Globals:	
**	Notes:
*/

float Compute_range_from_height(float elev, float height)
{
  float sin_elev = sin(elev * DEG_TO_RAD);
  float range;

    /* (CPT&E Label H) */
  range = IR * RE * (sqrt(sin_elev*sin_elev + 2*height/(IR * RE)) - sin_elev); 

  return range; 
}  /* end of Compute_range_from_height() */

/********************** Get_height_index() **************************
**
**	Description:
**	Inputs:		elev - elevation angle in degree
**			range - slant range in km
**	Outputs:	
**	Return:		height index
**	Globals:	
**	Notes:
*/
 
int Get_height_index(float elev, float range)
{
  float height;
  int height_index;

  height = Compute_height_from_range(elev, range);
  height_index = (int)(height/((float)Height_interval/METERS_PER_KM) + 0.5);

  return height_index;
}  /* end of Get_height_index() */

/*********************** Get_range_index() **************************
**
**	Description:
**	Inputs:		elev - elevation angle in degree
**			height - height in km
**			bin_size - radial bin size in km
**	Outputs:	
**	Return:		radial bin index
**	Globals:	
**	Notes:
*/

/* (CPT&E Label G) */

int Get_range_index(float elev, float height, float bin_size)
{
  float range;
  int range_index;
  
  range = Compute_range_from_height(elev, height);
  range_index = (int)(range/bin_size + 0.5);
  
  return range_index; 
}  /* end of Get_range_index() */

/***************************** Calculate_melting_layer() *************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:
**	Globals:	
**	Notes:
**
*/

/*(CPT&E Label K) */

int Calculate_melting_layer()
{
  int opstat, i, j, k, n;
  char * out_prod_ptr;
  float sum_heights[MAX_NUM_HEIGHTS];
  int low_az, high_az, low_index = -1, high_index = -1;
  float total_sum = 0, running_sum = 0, statistic = 0;
  float total_sum_threshold = 0;
  float last_avg_top = (float)Default_top;
  int vol_count = 0;
  float height_interval_km = (float)Height_interval/METERS_PER_KM;
  static int first_time = 1;
  ML_data_t ml_data[MAX_NUM_AZ];
  float top_avg = 0, bottom_avg = 0;
  float top_sum = 0, bottom_sum = 0;
  int count = 0;
  int ivalid = 0, ivalid_beg = 0;
  int Radar_Valid_Count = 0;
  int Rflag = RF_IntRadar;
  int Valid_radar_index[MAX_NUM_AZ];

    /* get output buffer */
  out_prod_ptr = (char *)RPGC_get_outbuf_by_name("MLDA", OUTBUF_SIZE1, &opstat);
  if(opstat != NORMAL)
  {
    ML_prod_generated = 0;
    return 1;
  }

    /* 
    ** Get last volume's melting layer average top. 
    ** If Time_gap_too_big_flag is set, all the previous
    ** volume data is discarded. It is treated as the first volume.
    */
  if(!Time_gap_too_big_flag) 
    last_avg_top = Get_last_top(Merged_ml_data);
  else  /* Time_gap_too_big_flag is set */
  {
      /* set the first_time flag and initialize the last_avg_top to Default_top */
    first_time = 1;
    last_avg_top = (float)Default_top;
  }

    /* Initialize ml_data when first_time flag is set */
  if(first_time)
  {
    for(i = 0; i < MAX_NUM_AZ; i++)
    {
      if ( ML_model_valid == 0 )
      {
        ml_data[i].top = ML_NO_DATA;
        ml_data[i].bottom = ML_NO_DATA;
      }
      radar_thr[i] = 0;
      Radar_ml_data[i].top = ML_NO_DATA;
      Radar_ml_data[i].bottom = ML_NO_DATA;
      Merged_ml_data[i].top = ML_NO_DATA;
      Merged_ml_data[i].bottom = ML_NO_DATA;
    }
			
    first_time = 0;
  } 
    /* 
    ** Loop through all the azimuth angles (0 - 359).
    ** For each azimuth angle, look 10 degree on either 
    ** side and calculate the histogram from the height
    ** information.
    */
  for(i = 0; i < MAX_NUM_AZ; i++)
  {
    for(k = 0; k < MAX_NUM_HEIGHTS; k++)
      sum_heights[k] = 0.0;

    low_az = i - (int)Half_window_size;
    high_az = i + (int)Half_window_size;

    if(low_az < 0)
      low_az += MAX_NUM_AZ;

    if(high_az >= MAX_NUM_AZ)
      high_az -= MAX_NUM_AZ;

    for(j = low_az; ; )
    {
      for(k = 0; k < MAX_NUM_HEIGHTS; k++)
      {
        n = Current_vol_index;
        for(vol_count = Num_vol; vol_count > 0; vol_count--)
        {
          sum_heights[k] += Wet_snow_weight[n][j][k];
          n--;
          if (n < 0)
            n = n + MAX_NUM_VOL;
        }
      }

      if(j == high_az)
        break;

      j = (j + 1) % MAX_NUM_AZ;
    }  /* end of for(j = low_az; ; ) */

    /* 
    ** zero out data that is more than 1km from last
    ** volume's melting layer top.
    */
    {
      int low = 0, high = 0;

      if ( Use_avg_top_clipping == 1 )
      {
        high = (int)((last_avg_top + 2 * (float)ML_depth) / height_interval_km + 0.5);
        low = (int)((last_avg_top - 2 * (float)ML_depth) / height_interval_km + 0.5);
      }
      else
      {
        /* Use previous radial-by-radial top for clipping */ 
        high = (int)((Merged_ml_data[i].top + 2 * (float)ML_depth) / height_interval_km + 0.5);
        low = (int)((Merged_ml_data[i].top - 2 * (float)ML_depth) / height_interval_km + 0.5);
      }

      if(low > 0)
        for(k = 0; k < low; k++)
          sum_heights[k] = 0.0;

      if(high < Max_num_heights)
        for(k = high+1; k < Max_num_heights; k++)
          sum_heights[k] = 0.0;
    }  /* end of zero out data block */

    total_sum = 0;
    for(k = 0; k < Max_num_heights; k++)
    {
      total_sum += sum_heights[k];
    }

      /* initialize */
    low_index = -1;
    high_index = -1;
    running_sum = 0;

    for(k = 0; k < Max_num_heights; k++)
    {
      running_sum += sum_heights[k];

      if (total_sum > 0 )
         statistic = running_sum / total_sum;

      if(statistic > (float)Low_percentile && low_index == -1)
        low_index = k;
   
      if(statistic > (float)High_percentile && high_index == -1)
        high_index = k;

      if (low_index > 0 && high_index > 0)
        break;
    }

    if(Azm_reso == HALF_DEG_AZ)
    {
        /* This needs to be justified by NSSL */
    /*  total_sum_threshold = (float)Min_wet_snow_sum * 2; */
    }
    else
      total_sum_threshold = (float)Min_wet_snow_sum;
 
    radar_thr[i] = total_sum;

    if(total_sum > total_sum_threshold)
    {
      ml_data[i].top = high_index * height_interval_km + 0.05; 
      ml_data[i].bottom = low_index * height_interval_km + 0.05; 
 
      Radial_flag[i] = RF_Radar;
      if ( ivalid_beg == 0 )
      {
        Valid_radar_index[ivalid] = i;   /* saved valid indexes to fill gaps later */
        Valid_radar_index[ivalid+1] = i; /* placeholder in case we have a single valid radial at the end */
        ivalid++;
        ivalid_beg = 1;
      }
      else
        Valid_radar_index[ivalid] = i;   /* saved valid indexes to fill gaps later */
    }
    else
    {
      ml_data[i].top = ML_NO_DATA;
      ml_data[i].bottom = ML_NO_DATA;
      if( ivalid_beg == 1 && i != 0 )
      {
        ivalid++;
        ivalid_beg = 0;
      }
    }
  }  /* end of for(i = 0; i < MAX_NUM_AZ; i++) */

  if ( ivalid == 1 && Valid_radar_index[0] == 0 )
    ivalid = 0;

  /* fill the missing top and bottom with average value */
  {
    top_sum = 0.0, bottom_sum = 0.0;

    /* Calculate the average top and bottom for radar-only */
    for(i = 0; i < MAX_NUM_AZ; i++)
    {
      if(ml_data[i].bottom > ML_NO_DATA)
      {
        top_sum += ml_data[i].top;
        bottom_sum += ml_data[i].bottom;
        count++; 
      }
    }

    if(count > 0)
    {
      top_avg = top_sum/count;
      bottom_avg = bottom_sum/count;
      Radar_Valid_Count = count;
    }
    /* If the radar thresholds aren't met AND we have no valid model data then set flag to indicate we failed */
    else if ( ML_model_valid == 0 )
    {
      ML_not_found = 1;
    } /* end of if(count > 0) */

    RPGC_log_msg(GL_INFO, ">> Radials (Radar) with ML detection= %d top_avg= %6.3f bottom_avg= %6.3f", count, top_avg, bottom_avg); 

    for(i = 0; i < MAX_NUM_AZ; i++)
    {
      Raw_Radar_ml_data[i].top = ml_data[i].top;
      Raw_Radar_ml_data[i].bottom = ml_data[i].bottom;
    }

    /* Interpolate between missing angles rather than using averages */
    int iv_start, iv_end = 0, iv_start_ind, iv_end_ind = 0, iv_rad, iv = 0;

    i = 0;
    while( i < ivalid )
    {
      iv_start_ind = iv_start = Valid_radar_index[i];

      /* Handle 0/359 wrap-around */
      if ( i == 0 && Valid_radar_index[i] != 0 )
      {
        iv_start     = Valid_radar_index[ivalid-1] - 360;
        iv_start_ind = Valid_radar_index[ivalid-1];
        iv_end_ind = iv_end = Valid_radar_index[i];
        i = i + 1;
      }
      else if ( i == 0 && Valid_radar_index[i] == 0 )
      {
        iv_start   = 0;
        iv_end     = -1;
        i = i + 1;
      }
      else if ( i == ivalid-1 && Valid_radar_index[0] == 0  )
      {
        iv_end     = 360;
        iv_end_ind = 0;
        i = ivalid;
      }
      else if ( i != ivalid-1 )
      {
        iv_end_ind = iv_end = Valid_radar_index[i+1];
        i = i + 2;
      }
      else
        i = i + 1;

      /* For short gaps set a coast flag, for longer gaps identify it as interpolated */
      if ( (iv_end - iv_start) > (int)Radar_maxgap_interval )
        Rflag = RF_IntRadar;
      else
        Rflag = RF_CoastRadar;

      for ( iv=iv_start; iv < iv_end; iv++ )
      {
        if ( iv < 0 )
          iv_rad = iv + 360;
        else if ( iv > 359 )
          iv_rad = iv - 360;
        else
          iv_rad = iv;
						
        if( ml_data[iv_rad].bottom == ML_NO_DATA )
        {
          ml_data[iv_rad].top = ml_data[iv_start_ind].top + (iv-iv_start)*(ml_data[iv_start_ind].top - 
                                ml_data[iv_end_ind].top)/(iv_start-iv_end);	

          ml_data[iv_rad].bottom = ml_data[iv_start_ind].bottom + (iv-iv_start)*(ml_data[iv_start_ind].bottom - 
                                   ml_data[iv_end_ind].bottom)/(iv_start-iv_end);
          Radial_flag[iv_rad] = Rflag; 
        }
      }
    }

    /* Save the Radar-calculated MLDA values */
    for(i = 0; i < MAX_NUM_AZ; i++)
    {
      Radar_ml_data[i].top = ml_data[i].top;
      Radar_ml_data[i].bottom = ml_data[i].bottom;
    }

    /* If we have valid model data and few enough radar-based radial then evaluate where model data can be used */
    if ( ML_model_valid == 1 && Radar_Valid_Count < (int)Radar_radonly_thr && (int)Melting_Layer_Source == MLDA_UseType_Model )  
    {
      RPGC_log_msg(GL_INFO, "Model was found, merge data with radar-based profile. \n"); 
      for(i = 0; i < MAX_NUM_AZ; i++)
      {
        if ( ml_data[i].bottom == ML_NO_DATA || Radial_flag[i] == RF_AvgRadar )
        {
          if( Model_Radial_flag[i] != RF_None )
          {
            ml_data[i].top = Model_ml_data[i].top;
            ml_data[i].bottom = Model_ml_data[i].bottom;
            /* Radial_flag[i]  Leave Radial_flag from model calculations */
            Radial_flag[i] = Model_Radial_flag[i];
          }
          /* otherwise leave the current data/flags alone */
        }
	else if ( Radial_flag[i] == RF_IntRadar )
        {
          /* Use average of model top/bottom with radar interpolated top/bottom */
          float rad_wt = Min_rad_mod_wt;
          if ( radar_thr[i] > Low_snow_thresh )
            rad_wt = radar_thr[i]/Min_wet_snow_sum;
          if ( rad_wt < Min_rad_mod_wt )
            rad_wt = Min_rad_mod_wt;

          float mdl_wt = 1 - rad_wt;
          ml_data[i].top = mdl_wt*(Model_ml_data[i].top) + rad_wt*(ml_data[i].top);
          ml_data[i].bottom = mdl_wt*(Model_ml_data[i].bottom) + rad_wt*(ml_data[i].bottom);
          Radial_flag[i] = RF_MdlIntRadAvg;
        }
      } 
    }

  }  /* end of fill missing value block */

  /* Save Merged MLDA results */
  float top_min = 99.0, top_max = -99.0;
  float bot_min = 99.0, bot_max = -99.0;
  top_sum = 0.0; bottom_sum = 0.0; count = 0;

  for(i = 0; i < MAX_NUM_AZ; i++)
  {
    Merged_ml_data[i].top = ml_data[i].top;
    Merged_ml_data[i].bottom = ml_data[i].bottom;
    if ( Merged_ml_data[i].top > top_max )
      top_max = Merged_ml_data[i].top;
    if ( Merged_ml_data[i].top < top_min )
      top_min = Merged_ml_data[i].top;

    if ( Merged_ml_data[i].bottom > bot_max )
      bot_max = Merged_ml_data[i].bottom;
    if ( Merged_ml_data[i].bottom < bot_min )
      bot_min = Merged_ml_data[i].bottom;

    if(Merged_ml_data[i].bottom > ML_NO_DATA)
    {
      top_sum += Merged_ml_data[i].top;
      bottom_sum += Merged_ml_data[i].bottom;
      count++; 
    }
  }

  if(count > 0)
  {
    top_avg = top_sum/count;
    bottom_avg = bottom_sum/count;
  }

    /* send intermediate product */
  if((int)(!Melting_Layer_Source) || ML_not_found)
  {
    char msg[100];
    if((int)(!Melting_Layer_Source))
      strcpy(msg, "Melting_Layer_Source is not set");
    else if(ML_not_found)
      strcpy(msg, "Insufficient data");
    else
      strcpy(msg, " ");

    RPGC_log_msg(GL_INFO, ">> Calculate_melting_layer(): %s - Sending out default product (Top: %6.3f Bottom: %6.3f km)",
                 msg, Default_ml_data[0].top, Default_ml_data[0].bottom);
    Send_ML_data(Default_ml_data, (void *)out_prod_ptr);
    RPGC_rel_outbuf((void *)out_prod_ptr, FORWARD);
    ML_prod_generated = 1;
  }
  else
  {
    RPGC_log_msg(GL_INFO, ">> Calculate_melting_layer(): Sending out product   Model valid: %d", ML_model_valid);
    RPGC_log_msg(GL_INFO, 
           ">> Radials (Merged) with ML detection= %d top_avg= %6.3f bottom_avg= %6.3f top_min=%6.3f max=%6.3f bot_min=%6.3f max=%6.3f",
           count, top_avg, bottom_avg, top_min, top_max, bot_min, bot_max); 
    Send_ML_data(Merged_ml_data, (void *)out_prod_ptr);
    RPGC_rel_outbuf((void *)out_prod_ptr, FORWARD);
    ML_prod_generated = 1;
  }

  if ( MLDA_DEBUG == 1 )
    Log_MLDA_Radials( date_time_str );

  /* Reset flag for next volume */
  ML_not_found = 0;

  return 0;
}  /* end of Calculate_melting_layer() */
	
/*********************** Log_MLDA_radials() *****************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:
**	Globals:	
**	Notes:
**
*/
void Log_MLDA_Radials( char *date_time_str )
{
  int i;

  fprintf( stderr, "Melting Layer: Volume %s (# %d)\n", date_time_str, IJK++ );
  for ( i = 0; i < MAX_NUM_AZ; i++ )
  {
    float FinalTop = 0.0, FinalBottom = 0.0;

    if ( ML_not_found ) 
    {
      FinalTop    = Default_ml_data[i].top;
      FinalBottom = Default_ml_data[i].bottom;
      Radial_flag[i] = RF_None;
    }
    else 
    {
      FinalTop    = Merged_ml_data[i].top;
      FinalBottom = Merged_ml_data[i].bottom;
    }

    char nameType[20];
    if ( Radial_flag[i] == RF_Radar ) 
      sprintf(nameType,"[Rad]");
    else if ( Radial_flag[i] == RF_AvgRadar )
      sprintf(nameType,"[AvgRad]");
    else if ( Radial_flag[i] == RF_IntRadar )
      sprintf(nameType,"[IntRad]");
    else if ( Radial_flag[i] == RF_CoastRadar )
      sprintf(nameType,"[CstRad]");
    else if ( Radial_flag[i] == RF_Model )
      sprintf(nameType,"[Mdl]");
    else if ( Radial_flag[i] == RF_AvgModel )
      sprintf(nameType,"[AvgMdl]");
    else if ( Radial_flag[i] == RF_MdlIntRadAvg )
      sprintf(nameType,"[AvgMIR]");
    else if ( Radial_flag[i] == RF_Mixed )
      sprintf(nameType,"[RT=MB]");
    else if ( Radial_flag[i] == RF_None )
      sprintf(nameType,"[Default]");
    else 
      sprintf(nameType,"[UNK]");

    fprintf( stderr, "T %4.1f B %4.1f az %d %s %d MTB: %4.1f %4.1f %d X %4.1f RTB %4.1f %4.1f %d %d %4.1f\n", 
             FinalTop, FinalBottom, i, nameType, Radial_flag[i], Model_ml_data[i].top, 
             Model_ml_data[i].bottom, sum_cnt[i], mdl_cross[i], Radar_ml_data[i].top,
             Radar_ml_data[i].bottom, radar_thr[i], Radial_flag[i], Raw_Radar_ml_data[i].top);
  }
}

/*********************** Get_last_top() *****************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:
**	Globals:	
**	Notes:
**
*/

float Get_last_top(ML_data_t * ml_data)
{
  float avg_top = 0, top_sum = 0;
  int i, count = 0;

  for(i = 0; i < MAX_NUM_AZ; i++)
  {
    if(ml_data[i].top > ML_NO_DATA)
    {
      top_sum += ml_data[i].top;
      count++;
    }
  } 
   
  if(count > 0)
    avg_top = top_sum/count; 
  else
    avg_top = (float)Default_top;

  return avg_top;  
}  /* end of Get_last_top() */

/*************************** Get_default_ML_value() *************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:
**	Globals:	
**	Notes:
*/

/* (CPT&E Label F) */

int Get_default_ML_value(ML_data_t * ml_data)
{
  int i;
  float bottom;

  /* Ensure top height is above ground */
  if (Default_top < 0.0) Default_top = 0.0;

  bottom = (float)Default_top - (float)ML_depth;
  
  /* Ensure bottom height is above ground */
  if (bottom < 0.0) bottom = 0.0;

  for(i = 0; i < MAX_NUM_AZ; i++)
  {
    ml_data[i].top = (float)Default_top;
    ml_data[i].bottom = bottom;
  }
 
  return 0;
}  /* end of Get_default_ML_value() */


/************************** Send_ML_data() **************************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:
**	Globals:	
**	Notes:
**
*/

int Send_ML_data(ML_data_t * ml_data, void * buf_ptr)
{
  int i; 
  ML_data_t * output = (ML_data_t *)buf_ptr;
  
  for(i = 0; i < MAX_NUM_AZ; i++)
    output[i] = ml_data[i];
 
  return 0; 
}  /* end of Send_ML_data() */

/***************************** Calculate_model_melting_layer() *************
**
**	Description:
**	Inputs:		
**	Outputs:	
**	Return:
**	Globals:	
**	Notes:
**
*/

int Calculate_model_melting_layer( void )
{
  int i, active, model_idx;
  char *frz_buf = NULL;
  RPGCS_model_grid_data_t *field_Ncross = NULL;
  RPGCS_model_grid_data_t *field_frzht = NULL;
  RPGCS_model_grid_data_t *field_range = NULL;
  RPGCS_model_grid_data_t *field_az = NULL;

  ML_data_t ml_data[MAX_NUM_AZ];
  ML_data_t temp_ml[MAX_NUM_AZ];
  float top_avg = 0, bottom_avg = 0;
  int valid_count = 0;
  int i_az, curr;
  double i_range;
  int ix, iy, ncross;
  double  top_frz_ht, bot_frz_ht;
  int tot_cnt = 0, cnt_radsT, cnt_radsB;
  float tot_sum = 0.0, tot_bot = 0.0;
  float top_max = -99.0, top_min = 99.0;
  float bot_max = -99.0, bot_min = 99.0;
  float sum_tops[MAX_NUM_AZ], sum_bots[MAX_NUM_AZ], sum_cross[MAX_NUM_AZ], sum_radsT, sum_radsB;
  float max_tops[MAX_NUM_AZ], min_tops[MAX_NUM_AZ];
  float max_bots[MAX_NUM_AZ], min_bots[MAX_NUM_AZ];
  int grid_units;
  int status = 0;

  /* Get necessary model fields, RANGE, AZ, NCROSS, HEIGHT_0 (all levels) */
  if( (ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE )) == ORPGSITE_FAA_REDUNDANT ) 
    active = ORPGRED_channel_state( ORPGRED_MY_CHANNEL ); 
  else
    active = ORPGRED_CHANNEL_ACTIVE;

  if( (!Frz_data_msg_updated) || (active != ORPGRED_CHANNEL_ACTIVE) )
  {
    RPGC_log_msg( GL_INFO, "FRZ_GRID LB data has not been updated" );
    return -1;
  }
  else
  {
    RPGC_log_msg( GL_INFO, "FRZ_GRID LB UPDATED---> Frz_data_msg_updated: %d   active: %d\n", Frz_data_msg_updated, active );
    /* Reset update flag */
    Frz_data_msg_updated = 0;
  }

  /* Get the model freezing grid data (utilizes updated generic rpgcs_ call) */
  if ( (model_idx = RPGCS_get_model_data(MODEL_FRZ_GRID, RUC_ANY_TYPE, &frz_buf)) < 0 )
  {
    if ( model_idx != LB_NOT_FOUND )
    {
      RPGC_log_msg( GL_ERROR, "FRZ GRID DATA: Processing Failure (get_model_data)" );
      return -1;
    }
  }

  /* Get the model attributes */
  Model_Attrs = (RPGCS_model_attr_t *)RPGCS_get_model_attrs( model_idx, frz_buf );
  if ( Model_Attrs == NULL )
  {
    RPGC_log_msg( GL_ERROR, "FRZ GRID DATA: Processing Failure (get_model_attrs)" );
    if( frz_buf != NULL )
      RPGP_product_free( frz_buf );
    return -1;
  }

  /* Initialize local and Model_  ml_data each time */
  for(i = 0; i < MAX_NUM_AZ; i++)
  {
    ml_data[i].top = ML_NO_DATA;
    ml_data[i].bottom = ML_NO_DATA;
    Model_ml_data[i].top = ML_NO_DATA;
    Model_ml_data[i].bottom = ML_NO_DATA;
  }

  /* Get number of zero-degree crossings */
  field_Ncross = RPGCS_get_model_field( model_idx, frz_buf, FRZ_GRID_NUM_ZERO_X);
  /* Get freezing height field */
  /* NOTE: This will return multiple layers of zero degree heights based on the Ncross field above */
  field_frzht = RPGCS_get_model_field( model_idx, frz_buf, FRZ_GRID_HEIGHT_ZERO);
  /* Get radar-range field */
  field_range = RPGCS_get_model_field( model_idx, frz_buf, FRZ_GRID_RANGE);
  /* Get radar-az field */
  field_az = RPGCS_get_model_field( model_idx, frz_buf, FRZ_GRID_AZIMUTH);
	
  /* All of the above fields are needed for successful processing, if any are missing then use what is already available */ 
  if ( field_Ncross == NULL || field_frzht == NULL || field_range == NULL || field_az == NULL )
  {
    RPGC_log_msg(GL_ERROR, "Model derived fields are unavailable, setting model data to unavailable and return to regualar processing\n");

    if( frz_buf != NULL )
      RPGP_product_free( frz_buf );
    if( Model_Attrs != NULL )
      free( Model_Attrs );
    if( field_Ncross != NULL )
      RPGCS_free_model_field( model_idx, (char *) field_Ncross );
    if( field_frzht != NULL )
      RPGCS_free_model_field( model_idx, (char *) field_frzht );
    if( field_range != NULL )
      RPGCS_free_model_field( model_idx, (char *) field_range );
    if( field_az != NULL )
      RPGCS_free_model_field( model_idx, (char *) field_az );	
    return -1;
  }

  /* 
  ** Loop through all the model grid points and calculate the 
  ** average freezing level for that azimuth.
  */
  for ( i = 0; i < 360; i++ )
  {
    sum_tops[i] = 0.0;
    sum_bots[i] = 0.0;
    sum_cross[i] = 0;
    sum_cnt[i] = 0;
    max_tops[i] = 0.0;
    min_tops[i] = 999.9;
    max_bots[i] = 0.0;
    min_bots[i] = 999.9;
  }
	
  for (ix = 0; ix < field_Ncross->dimensions[0]; ix++ )
  {
    for (iy = 0; iy < field_Ncross->dimensions[1]; iy++ )
    {
      i_range = RPGCS_get_data_value( field_range, 0, ix, iy, &grid_units );
      /* Send an error message if units are NOT in KM */
      if ( grid_units != RPGCS_KILOMETER_UNITS )
      {
        RPGC_log_msg(GL_ERROR, "Expected range units to be in KM(%d), but tagged as (%d) instead\n",
                     RPGCS_KILOMETER_UNITS, grid_units);
        return -1;
      }

      i_az    = RPGC_NINTD(RPGCS_get_data_value( field_az, 0, ix, iy, &grid_units ));
      /* Units are tagged unknown, but are in degrees */

      if ( i_az > 359 )
        i_az = 0;

      /* Get the freezing height, but adjust relative to feedhorn height */
      ncross = RPGCS_get_data_value( field_Ncross, 0, ix, iy, &grid_units );
      /* Units are tagged unknown but are unitless */

      if ( (int)ncross != 0 )
      {
        top_frz_ht = RPGCS_get_data_value( field_frzht, 0, ix, iy, &grid_units ) - Radar_height;
        /* Send an error message if units are NOT in KM */
        if ( grid_units != RPGCS_KILOMETER_UNITS ) {
          RPGC_log_msg(GL_ERROR, "Expected frz height units to be in KM(%d), but tagged as (%d) instead\n",
                       RPGCS_KILOMETER_UNITS,grid_units);
          return -1;
        }
        bot_frz_ht = top_frz_ht - Model_min_bot_delta;
      }
      else
      {
        top_frz_ht = 0.0;
        bot_frz_ht = 0.0;
      }

      if ( top_frz_ht < 0.0 )
        top_frz_ht = 0.0;

      if ( bot_frz_ht < 0.0 )
        bot_frz_ht = 0.0;

      if ( top_frz_ht >= 0.0 && i_range > (int)Model_min_range && i_range < (int)Model_max_range )
      { 
        /* Use this freezing height at multiple radials */
        for ( i = -1*(int)Model_half_window; i < (int)Model_half_window+1; i++ )
        {
          curr = i_az + i;
          if ( curr < 0 )
            curr = 360 + curr;
          else if ( curr > 359 )
            curr = curr - 360;
          sum_tops[curr] = sum_tops[curr] + top_frz_ht;
          sum_bots[curr] = sum_bots[curr] + bot_frz_ht;
          sum_cross[curr] = sum_cross[curr] + ncross;
          sum_cnt[curr]     = sum_cnt[curr] + 1;
        }
        tot_sum = tot_sum + top_frz_ht;
        tot_bot = tot_bot + bot_frz_ht;
        tot_cnt = tot_cnt + 1;
      }
    }
  }

  /* Load individual radials with respective freezing height */
  for ( i = 0; i < MAX_NUM_AZ; i++ )
  {
    if ( sum_cnt[i] > 0 )
    {
      Model_ml_data[i].top = sum_tops[i]/sum_cnt[i] + MDL_ADJUST_TOP; 
      Model_ml_data[i].bottom = sum_bots[i]/sum_cnt[i] + MDL_ADJUST_BOTTOM; 
      temp_ml[i].top    = ml_data[i].top;
      temp_ml[i].bottom = ml_data[i].bottom;
      mdl_cross[i] = sum_cross[i]/sum_cnt[i]; 
      valid_count++;
      Model_Radial_flag[i] = RF_Model;
      if ( Model_ml_data[i].top > top_max )
        top_max = Model_ml_data[i].top;
      if ( Model_ml_data[i].top < top_min )
        top_min = Model_ml_data[i].top;
      if ( Model_ml_data[i].bottom > bot_max )
        bot_max = Model_ml_data[i].bottom;
      if ( Model_ml_data[i].bottom < bot_min )
        bot_min = Model_ml_data[i].bottom;
    }
    else   /* fill the missing top and bottom with average value */
    {
      Model_ml_data[i].top = tot_sum/tot_cnt + MDL_ADJUST_TOP;
      Model_ml_data[i].bottom = tot_bot/tot_cnt + MDL_ADJUST_BOTTOM;
      Model_Radial_flag[i] = RF_AvgModel;
    }

    if ( Model_ml_data[i].top < MDL_MIN_HT )
    {
      Model_ml_data[i].top = MDL_MIN_HT;
      top_min = MDL_MIN_HT;
    }

    /* Limit the maximum range between top/bottom */
    if ( Model_ml_data[i].bottom < (Model_ml_data[i].top - Model_max_bot_delta) )
      Model_ml_data[i].bottom = Model_ml_data[i].top - Model_max_bot_delta;

    /* Make sure we have sufficient range between top/bottom */
    if ( Model_ml_data[i].bottom > (Model_ml_data[i].top - Model_min_bot_delta) )
      Model_ml_data[i].bottom = Model_ml_data[i].top - Model_min_bot_delta;

    if ( Model_ml_data[i].bottom < MDL_MIN_HT )
    {
      Model_ml_data[i].bottom = MDL_MIN_HT;
      bot_min = MDL_MIN_HT;
    }
		
  }  /* end of for(i = 0; i < MAX_NUM_AZ; i++) */

  /* Run N-radial average filter */
  for ( i_az = 0; i_az < MAX_NUM_AZ; i_az++ )
  {
    cnt_radsT = 0; sum_radsT = 0.0;
    cnt_radsB = 0; sum_radsB = 0.0;
    for ( i = -1*MDL_NFILTER/2; i<(MDL_NFILTER/2)+1; i++ )
    {
      curr = i_az + i;
      if ( curr < 0 ) 
        curr = 360 + curr;
      else if ( curr > 359 ) 
        curr = curr - 360;

      if ( Model_ml_data[curr].top > ML_NO_DATA )
      {
        sum_radsT = sum_radsT + Model_ml_data[curr].top;
        cnt_radsT++;
      }
      if ( Model_ml_data[curr].bottom > ML_NO_DATA )
      {
        sum_radsB = sum_radsB + Model_ml_data[curr].bottom;
        cnt_radsB++;
      }
    }

    if ( cnt_radsT > (MDL_NFILTER/2-1) )
      temp_ml[i_az].top = sum_radsT/cnt_radsT;
    else
      temp_ml[i_az].top = tot_sum/tot_cnt;

    if ( cnt_radsB > (MDL_NFILTER/2-1) )
      temp_ml[i_az].bottom = sum_radsB/cnt_radsB;
    else
      temp_ml[i_az].bottom = tot_bot/tot_cnt;
  } /* for ( i_az = 0; i_az < MAX_NUM_AZ; i_az++ ) */
	
  top_avg    = tot_sum/tot_cnt;
  bottom_avg = tot_bot/tot_cnt;

  /* Did we exceed the minimum number of valid model radials? */
  if ( valid_count > MDL_MIN_VALID )
  {
    status = 1;
    for ( i = 0; i < MAX_NUM_AZ; i++ )
    {
      Model_ml_data[i].top = temp_ml[i].top; 
      Model_ml_data[i].bottom = temp_ml[i].bottom; 
    }

    /* save model valid unix time */
    tm_model_valid = Model_Attrs->valid_time;

    RPGC_log_msg(GL_INFO, 
        ">> Radials (Model) with ML detection= %d top_avg= %6.3f bottom_avg= %6.3f top_min=%6.3f max=%6.3f bot_min=%6.3f max=%6.3f",
        valid_count, top_avg, bottom_avg, top_min, top_max, bot_min, bot_max);
  }
  else {
    status = 0;
    RPGC_log_msg(GL_INFO, ">> Radials (Model) with ML detection= %d; not using model because not enough valid model radials",
        valid_count);
  }

  if( frz_buf != NULL )
    RPGP_product_free( frz_buf );
  if( Model_Attrs != NULL )
    free( Model_Attrs );
  if( field_Ncross != NULL )
    RPGCS_free_model_field( model_idx, (char *) field_Ncross );
  if( field_frzht != NULL )
    RPGCS_free_model_field( model_idx, (char *) field_frzht );
  if( field_range != NULL )
    RPGCS_free_model_field( model_idx, (char *) field_range );
  if( field_az != NULL )
    RPGCS_free_model_field( model_idx, (char *) field_az );
	
  return(status);
}  /* end of Calculate_model_melting_layer() */

/******************************************************************

   Description:
      LB update callback function.

   Notes:
      See lb man page for description of function arguments.

******************************************************************/
static void LB_cb_frz (int fd, LB_id_t msgid, int msg_info, void *arg) {

  /* test data store and message ids against parameter values */
  if( (MODEL_FRZ_GRID == fd) && (msgid == MODEL_FRZ_ID) )
    Frz_data_msg_updated = 1;

  RPGC_log_msg( GL_INFO, "(LB_cb_frz)---> fd: %d   msgid: %d   MODEL_FRZ_GRID updated: %d\n",
                fd, msgid, Frz_data_msg_updated );

}  /* end of Lb_cb_frz() */

/****************************** End of melting_layer.c ******************/

