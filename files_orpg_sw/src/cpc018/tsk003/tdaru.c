/*
* RCS info.
* $Author: ccalvert $
* $Date: 2010/05/24 20:42:01 $
* $Locker:  $
* $Id: tdaru.c,v 1.21 2010/05/24 20:42:01 ccalvert Exp $
* $Revision: 1.21 $
* $State: Exp $
* $Log: tdaru.c,v $
* Revision 1.21  2010/05/24 20:42:01  ccalvert
* TDA SCIT 20km
*
* Revision 1.20  2008/02/04 20:08:34  steves
* Issue 3-411
*
* Revision 1.19  2007/05/24 16:43:16  ryans
* Fix problem with macro reference
*
* Revision 1.18  2005/12/27 17:47:35  steves
* issue 2-879
*
* Revision 1.17  2005/07/21 16:24:23  steves
* issue 2-534
*
* Revision 1.16  2005/07/06 00:02:52  steves
* Reverted from version 1.14
*
* Revision 1.14  2005-07-01 14:41:23-05  steves
* issue 2-534
*
* Revision 1.13  2005/04/12 14:45:26  steves
* issue 2-710
*
* Revision 1.12  2005-04-11 14:02:20-05  steves
* issue 2-534
*
* Revision 1.11  2005-04-11 12:54:47-05  steves
* issue 2-591
*
* Revision 1.10  2004/03/15 16:50:35  steves
* issue 2-282
*
* Revision 1.9  2004/02/10 22:14:09  ccalvert
* code cleanup
*
* Revision 1.8  2004/02/05 23:08:14  ccalvert
* new dea format
*
* Revision 1.7  2004/01/27 01:14:55  ccalvert
* make site adapt dea format
*
* Revision 1.6  2003/07/23 22:38:05  ccalvert
* GAB bug fix
*
* Revision 1.12  2003/07/16 17:03:25  nshen
* In Update_data_array(): added if(elev_index) to avoid
* "Elve Index <0 or >16 -rpgcs_v*_info.c: 58" message in
* log file.
* In Build_GAB(): change gab.blk_length =
* total_pages * BYTES_PER_PAGE + sizeof(GAB_hdr_t) - sizeof(short) +
* sizeof(GAB_page_hdr_t);
* to gab.blk_length =
* total_pages * (BYTES_PER_PAGE + sizeof(GAB_page_hdr_t)) +
* sizeof(GAB_hdr_t) - sizeof(short);
*
* Revision 1.11  2003/06/30 13:40:43  nshen
* delivery version
*
* Revision 1.10  2003/06/26 13:44:09  nshen
* In Build_PSB(), convert the range to km/4 for packet_20 I and J position.
*
* Revision 1.9  2003/06/23 19:47:38  nshen
* assign strom ID once per volume only. modified Update_feat().
*
* Revision 1.8  2003/06/20 20:15:00  nshen
* modified Sort_feat() and Drop_feat(). Moved Sort_feat to Process_tvs()
* from Match_update_feat().
*
* Revision 1.7  2003/06/18 19:12:40  nshen
* ready version
*
* Revision 1.6  2003/06/17 18:42:58  nshen
* working version
*
* Revision 1.5  2003/06/04 12:47:11  nshen
* fixed bug in Write_page_hdr for vol_time. The vol_time passed in should be
* in milliseconds.
*
* Revision 1.4  2003/06/03 17:00:02  nshen
* change if(num_feat >0) Match_update_feat() to Match_update_feat()
* since Match_update_feat() handles this already.
* change Remove_feat() name to Remove_ext_feat()
*
* Revision 1.3  2003/06/02 13:35:14  nshen
* MDA track has been added and tested.
*
* Revision 1.2  2003/05/30 17:30:28  nshen
* working version has been tested
* mda has not yet been tested.
*
* Revision 1.1  2003/05/14 13:50:34  nshen
* Initial revision
*
* Revision 1.1  2003/05/02 19:36:34  nings
* Initial revision
*
*
*/

/*****************************************************************
**
**	File:		tdaru.c
**	Author:		Ning Shen
**	Date:		May 30, 2003
**	Version:	1.0
**
**	Description:
**	============
**	
**	This TDA Rapid Update program produces an elevation based
**	final TVS product.  
**
**	Inputs:
**		Elevation based TDAATR_RU (290) from TDA2D3DRU task;
**	 	Volume based TRFRCATR (50) from TRFRCALG task;
**		Volume based MDATTNN (295) from MDA Track
**	Output:
**		Elevation based final ICD-format product -- TRU (143).
**	
**	Change History:
**	===============
**
******************************************************************/

#include<rpg_globals.h>
#include<gen_stat_msg.h>
#include<rpgc.h>
#define DOUBLE
#include <rpgcs_coordinates.h>
#include<rpgcs.h>
#include<math.h>
#include<product.h>
#include<storm_cell_track.h>
#include<tda.h>
#include<siteadp.h>
#include"tdaru.h"

#define DEBUG
#undef DEBUG 

  /* global variables */

Scan_Summary Scan_summary_tbl;  /* scan summary table */

tvs_feat_t   TVS_feat;    /* input from TDA2D3DRU */
tvs_feat_t   TVS_pre_vol; /* hold data of previous volume */
scit_track_t SCIT_track;  /* input from TRFRCALG */
mda_track_t  MDA_track;   /* input from MDA track */
Siteadp_adpt_t Site_adapt; /* site information struct */
tdaru_prod_t TDA_current; /* output data array */

storm_cell_track_t SCIT_adapt;  /* SCIT adaptation data */
tda_t TVS_adapt;                /* TVS adaptation data */

int Max_num_tvs;     /* max number of TVSs in adaptation data */
int Max_num_etvs;    /* max number of ETVSs in adaptation data */
int Max_num_feat;    /* max number of TVSs + ETVSs */
int Max_assoc_dist;  /* max SCIT cell association distance (km) */

int Max_num_tvs_exceeded = FALSE;
int Max_num_etvs_exceeded = FALSE;

int TVS_saved = FALSE;      /* flag to indicate TVS data saved */
int SCIT_saved = FALSE;     /* flag to indicate SCIT data saved */
int MDA_saved = FALSE;      /* flag to indicate MDA data saved */
int Feat_advanced = FALSE;  /* flag to indicate TVS features advanced */

int Last_elev_flag = FALSE;     
int Elapsed_time_checked = FALSE;  

float Correlation_speed;         /* from SCIT adaptation data (m/s) */
int   Max_vtime_interval = 720;  /* from SCIT adaptation data (sec) */

char NO_ID[3] = "??";

  /* local function prototypes */

static void Process_control();
static int  Process_tvs(tvs_feat_t *);
static void Save_tvs(tvs_feat_t *);
static void Save_mda(mda_track_t *);
static void Save_scit(scit_track_t *);
static void Initialize();
static void Update_data_array(tvs_feat_t *, int);
static void Advance_features(int);
static void Associate_storm_ID(tdaru_feat_t *, int);
static int  Elapsed_too_much_time(tvs_feat_t *, int *);
static void Match_update_features(tvs_feat_t *, tdaru_prod_t *, int);
static int  Match_found(tvs_attr_t, tdaru_prod_t *, float);
static float Get_distance(float, float, float, float, float, float);
static void Update_feat(tvs_attr_t, tdaru_feat_t *);
static void Add_new_feat(tvs_attr_t, tdaru_prod_t *);
static void Remove_ext_feat(tdaru_prod_t *);
static void Remove_false_feat(tdaru_prod_t *);
static void Sort_feat(tdaru_prod_t *);
static void Drop_feat(tdaru_prod_t *);
static int  Build_ICD_product(tdaru_prod_t *, int);
static int  Build_PSB(tdaru_prod_t *, unsigned int *, unsigned char *);
static int  Build_GAB(tdaru_prod_t *, unsigned int *, unsigned char *);
static int  Build_TAB(tdaru_prod_t *, unsigned int *, unsigned char *, int);
static int  Write_page_hdr(tdaru_prod_t *, unsigned char *, int, int, int,
                           int, int, int, int *, int);
static int  Write_feat_lines(tdaru_feat_t, unsigned char *, int *);

#ifdef DEBUG
  static void Print_tvs_info(tvs_feat_t);
  static void Print_tvsru_info(tdaru_prod_t);
#endif



/***************************** main() ****************************
**
**  Description:	Set up the registrations for input/output
**			and start the program.
**  Inputs:		Not used
**  Outputs:		No
**  Returns:		Not used
**  Globals:		Max_num_tvs, Max_num_etvs, Max_num_feat
**  Notes:
**
*/

int main(int argc, char * argv[])
{
 
  int rc = 0; /* return code */
 
    /* register for inputs */
  RPGC_in_data(TVSATR_RU, ELEVATION_DATA);
  RPGC_in_data(TRFRCATR, VOLUME_DATA);
  RPGC_in_data(MDATTNN, ELEVATION_DATA);  

    /* register for output */
  RPGC_out_data(TRU, ELEVATION_DATA, TRU);

    /* register for scan summary */
  RPGC_reg_scan_summary();

    /* register adaptation data for SCIT, TVS, and site information */
  RPGC_reg_ade_callback( storm_cell_track_callback_fx, (void *) &SCIT_adapt, 
                         STORM_CELL_TRACK_DEA_NAME, BEGIN_VOLUME ); 
  RPGC_reg_ade_callback( tda_callback_fx, (void *) &TVS_adapt, 
                         TDA_DEA_NAME, BEGIN_VOLUME );

  rc = RPGC_reg_site_info( &Site_adapt );
  if ( rc < 0 )
  {
    RPGC_log_msg( GL_ERROR, "SITE INFO: cannot register adaptation data callback function\n");
  }

    /* initialize the task */
  RPGC_task_init(VOLUME_BASED, argc, argv); 

    /* Algorithm control loop */
  while(1)
  {
      /* initialize */
    Initialize();
 
      /*
      ** Blocked here until one of the three inputs 
      ** becomes available
      */
    RPGC_wait_for_any_data(WAIT_ANY_INPUT);

      /* max number of features allowed in adaptation data */
    Max_num_tvs = TVS_adapt.max_tvs_features;
    Max_num_etvs = TVS_adapt.max_etvs_features;
    Max_num_feat = Max_num_tvs + Max_num_etvs;
    Max_assoc_dist = TVS_adapt.max_storm_dist;

      /* enter main logic part */
    Process_control();
  }  /* end of while(1) */

  return 0;
} /* end of main() */

/********************** Initialize() **************************
**
**  Description:	It initializes the output data array.
**  Inputs:		No
**  Outputs:		No
**  Returns:		No
**  Globals:		TDA_current
**  Notes:
**
*/

void Initialize()
{
  int i;

    /* initialize data structure */
  TDA_current.hdr.vol_time = 0;
  TDA_current.hdr.vol_date = 0;
  TDA_current.hdr.current_elev = 0.0;
  TDA_current.hdr.num_features = 0;
  TDA_current.hdr.num_tvs = 0;
  TDA_current.hdr.num_etvs = 0;

  for(i = 0; i < TVFEAT_MAX; i++)
  {
    if(!Feat_advanced)
      strcpy(TDA_current.tda_feat[i].storm_id, NO_ID);

    TDA_current.tda_feat[i].feat_status = EMPTY;
    TDA_current.tda_feat[i].matched_flag = FALSE;

    TDA_current.tda_feat[i].feature.feature_type = 0;
    TDA_current.tda_feat[i].feature.base_az = 0;
    TDA_current.tda_feat[i].feature.base_rng = 0;
    TDA_current.tda_feat[i].feature.low_level_DV = 0;
    TDA_current.tda_feat[i].feature.average_DV = 0;
    TDA_current.tda_feat[i].feature.max_DV = 0;
    TDA_current.tda_feat[i].feature.max_DV_height = 0;
    TDA_current.tda_feat[i].feature.depth = 0;
    TDA_current.tda_feat[i].feature.base_height = 0;
    TDA_current.tda_feat[i].feature.base_elev = 0;
    TDA_current.tda_feat[i].feature.top_height = 0;
    TDA_current.tda_feat[i].feature.max_shear = 0;
    TDA_current.tda_feat[i].feature.max_shear_height = 0;

    TDA_current.tda_feat[i].attr_flag.type_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.az_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.rng_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.ll_DV_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.avg_DV_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.max_DV_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.max_DV_hgt_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.depth_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.base_hgt_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.base_elev_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.top_hgt_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.max_shear_flg = SPACE;
    TDA_current.tda_feat[i].attr_flag.max_shear_hgt_flg = SPACE; 
  }
}  /* end of Initialize() */


/************************ Process_control() ********************
**
**  Description:	This is a logic control function. It switches 
**			to the different function based on the type 
**			of input data.
**  Inputs:		No
**  Outputs:		No
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

void Process_control()
{
  char * input;
  int datatype;     /* returned product ID of intermediate data */
  int opstat;       /* return status */


    /* get inputs */
  input = (char *)RPGC_get_inbuf_any(&datatype, &opstat);

    /* check the status */
  if(opstat != NORMAL)
  {
    LE_send_msg(GL_INFO, "\n>Buffer ID: %d, opstatus: %d", 
                datatype, opstat);
    LE_send_msg(GL_INFO, 
                "\n>RPGC_get_inbuf_any() error, aborting ...\n");
    RPGC_abort();
    return;
  }

    /* reset the opstat */
  opstat = CONTINUE;

    /* process input data and create output product */
  if(datatype == TVSATR_RU)   /* process TDA attrs data (elev based) */
  {
    LE_send_msg(GL_INFO, "\n>TVS processing ....");

      /* process tvs data */
    opstat = Process_tvs((tvs_feat_t *)input);
  }
  else if(datatype == TRFRCATR)  /* process SCIT data (volume based) */
  {
    LE_send_msg(GL_INFO, "\n>SCIT processing ....");

      /* save the information */
    Save_scit((scit_track_t *)input);
  }
  else if(datatype == MDATTNN)  /* process MDA data (volume based) */
  {
    LE_send_msg(GL_INFO, "\n>MDA track processing ....");

      /* save the information */
    Save_mda((mda_track_t *)input);
  }
  else  /* should never be here */
  {
    LE_send_msg(GL_INFO, "\n>Input data type error. Return to ACL.");
  }   /* end of if(datatype == TVSATR_RU) else */

    /* check return status from Process_tvs() */
  if(opstat == ABORT)
  {
    LE_send_msg(GL_INFO, "\nProcess_tvs() error. Abort ...\n"); 
    RPGC_rel_inbuf(input);
    RPGC_abort();
  }
  else
    RPGC_rel_inbuf(input);

} /* end of Process_control() */

/************************* Process_tvs() ************************
**
**  Description:	It processes the TVS input data and generates
**			output product.
**  Inputs:		tvs_input
**  Outputs:		No
**  Returns:		ABORT/CONTINUE	
**  Globals:		Scan_summery_tbl, Max_vtime_interval,
**			Correlation_speed, TVS_saved, Elapsed_time_checked,
**			TDA_current, Last_elev_flag, Feat_advanced, SCIT_saved
**  Notes:
**
*/

int Process_tvs(tvs_feat_t * tvs_input)
{
  int vcp, vol_num;
  int last_elev_index = 0, current_elev_index = 0;
  static int elapsed_time = 0;
  Scan_Summary * scan_tbl;

    /* get current volume number */
  vol_num = RPGC_get_buffer_vol_num(tvs_input);

    /* get the last elevation in the volume */
  if((vcp = RPGC_get_buffer_vcp_num((void *)tvs_input)) > 0)
    RPGC_is_buffer_from_last_elev( tvs_input, &current_elev_index,
                                   &last_elev_index );
  else
  {
    LE_send_msg(GL_INFO, "\n>Process_tvs(): RPGC_get_buffer_vcp_num() error\n");
    return ABORT;
  }

    /* get scan summary table */
  if((scan_tbl = RPGC_get_scan_summary(vol_num)) != NULL)
    Scan_summary_tbl = *scan_tbl;
  else
  {
    LE_send_msg(GL_INFO, "\n>Process_tvs(): RPGC_get_scan_summary() error\n");
    return ABORT;
  }


    /* get max elapsed time allowed from adaptationdata */
  if(!SCIT_saved)
  {
    Max_vtime_interval = SCIT_adapt.max_time * SECONDS_PER_MINUTE;
    Correlation_speed = SCIT_adapt.correlation_spd;
  }
 
    /* check if too musch time elapsed between volumes */
  if(TVS_saved && !Elapsed_time_checked)
  {
      /* check the elapsed time between volumes */
    if(Elapsed_too_much_time(tvs_input, &elapsed_time))
    {
      #ifdef DEBUG
        LE_send_msg(GL_INFO, "\n>Too much time elapsed between volumes.\n");
      #endif

      TVS_saved = FALSE;
    }
    else
      Elapsed_time_checked = TRUE;

  }  /* end of if(TVS_saved && !Elapsed_time_checked) */  

    /* update output data array with either current input or previous saved data */
  if(!TVS_saved) /* only for the start over and the first volume */
  {
    Update_data_array(tvs_input, vcp);
  }
  else    /* previous volume saved already */
  {
    Update_data_array(&TVS_pre_vol, vcp);

      /* update elevation with current input */
    TDA_current.hdr.current_elev = 
             (float)RPGCS_get_target_elev_ang(vcp, current_elev_index) / 10.0; 
  }

    /* set Last_elev_flag */
  if(current_elev_index == last_elev_index)
    Last_elev_flag = TRUE;
  else
    Last_elev_flag = FALSE;

    /* assign storm ID to each feature */
/*  if(!Feat_advanced)*/
  Associate_storm_ID(TDA_current.tda_feat, 
                     TDA_current.hdr.num_features);

    /*
    ** advance/match features if previous volume 
    ** TVS data is available
    */
  if(TVS_saved) 
  {
      /* advance features */
    if(!Feat_advanced)
      Advance_features(elapsed_time);

      /* match and update features */
    Match_update_features(tvs_input, &TDA_current, elapsed_time); 

  } /* end of if(TVS_saved) */

    /* Remove any false alarms */
  Remove_false_feat(&TDA_current);

    /* sorting the features in output data array */
  if(TDA_current.hdr.num_features > 1)
    Sort_feat(&TDA_current);
   
  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Process_tvs(): Begin to build product \n");
    Print_tvsru_info(TDA_current);
  #endif

    /* build the ICD final product & send it to LB */
  if(Build_ICD_product(&TDA_current, vol_num) == ABORT)
  {
    LE_send_msg(GL_INFO, "\n>Process_tvs(): Build_ICD_product() abort\n");
    return ABORT;
  } 

    /* save the TVS data if this is the last elevation */
  if(current_elev_index == last_elev_index)
  {
    Save_tvs(tvs_input); 
    Feat_advanced = FALSE;  /* set to false for every new volume */ 
    Elapsed_time_checked = FALSE;
  }

  return CONTINUE;

}  /* end of Process_tvs() */


/********************* Elapsed_too_much_time() ******************
**
**  Description:	Get the elapsed time between volumes. It 
**			return TRUE if too much time elapsed, FALSE
**			otherwise.
**  Inputs:		tvs_input
**  Outputs:		elapsed_time
**  Returns:		TRUE/FALSE
**  Globals:		Scan_summary_tbl, TVS_pre_vol, Max_vtime_interval
**  Notes:
**
*/

int  Elapsed_too_much_time(tvs_feat_t * tvs_input, int * elapsed_time)
{
  int time_sec;

  time_sec = Scan_summary_tbl.volume_start_time - TVS_pre_vol.hdr.vol_time; 

  if(time_sec < 0)
    time_sec += SEC_PER_DAY;

  *elapsed_time = time_sec;

    
  if((*elapsed_time > Max_vtime_interval) || (*elapsed_time < 0))
    return TRUE;
  else
    return FALSE;
} /* end of Elapsed_too_much_time() */


/************************ Update_data_array() *******************
**
**  Description:	It updates the TDA_current with either 
**			current input data or previous saved tvs
**			data.
**  Inputs:		tvs_feat
**  Outputs:		No
**  Returns:		No
**  Globals:		TVS_saved, TDA_current, Scan_summary_tbl	
**  Notes:
**
*/

void Update_data_array(tvs_feat_t * tvs_feat, int vcp)
{
  int i, elev_index;
  float elev = -99;
  char value;

  if(TVS_saved)
    value = SPACE;
  else
    value = CARET;

    /* get the current elevation angle */
  elev_index = RPGC_get_buffer_elev_index((void *)tvs_feat);

    /* 
    ** elev_index may be -1 if tvs_feat is &TVS_pre_vol.
    ** The if(elev_index) was added to avoid error message 
    ** "Elev Index <0 or >16 -rpgcs_v*_info.c: 58" 
    ** in log file. In this case, the elevation angle is updated
    ** in Process_tvs() after this function returned.
    */
  if(elev_index > 0)
  {
    elev = (float)RPGCS_get_target_elev_ang(vcp, elev_index) / 10.0;

    TDA_current.hdr.current_elev = elev;
  }

  TDA_current.hdr.vol_time = Scan_summary_tbl.volume_start_time;
  TDA_current.hdr.vol_date = Scan_summary_tbl.volume_start_date;

  TDA_current.hdr.num_tvs = abs(tvs_feat->hdr.num_tvs);
  TDA_current.hdr.num_etvs = abs(tvs_feat->hdr.num_etvs);
  TDA_current.hdr.num_features = TDA_current.hdr.num_tvs + 
                                 TDA_current.hdr.num_etvs;  


  #ifdef DEBUG
    fprintf(stderr, "\n>Update_data_array(): vol_time = %d, vol_date = %d, elev = %3.1f\n",
            TDA_current.hdr.vol_time, TDA_current.hdr.vol_date, TDA_current.hdr.current_elev);
  #endif

  for(i = 0; i < TDA_current.hdr.num_features; i++)
  {
    if(!TVS_saved)
      TDA_current.tda_feat[i].feat_status = NEW;
    else
      TDA_current.tda_feat[i].feat_status = EXT;

    TDA_current.tda_feat[i].feature = tvs_feat->features[i];
    TDA_current.tda_feat[i].matched_flag = FALSE;

      /* update attr_flag */ 
    TDA_current.tda_feat[i].attr_flag.type_flg = value;
    TDA_current.tda_feat[i].attr_flag.az_flg = value;
    TDA_current.tda_feat[i].attr_flag.rng_flg = value;
    TDA_current.tda_feat[i].attr_flag.ll_DV_flg = value;
    TDA_current.tda_feat[i].attr_flag.avg_DV_flg = value;
    TDA_current.tda_feat[i].attr_flag.max_DV_flg = value;
    TDA_current.tda_feat[i].attr_flag.max_DV_hgt_flg = value;
    TDA_current.tda_feat[i].attr_flag.depth_flg = value;
    TDA_current.tda_feat[i].attr_flag.base_hgt_flg = value;
    TDA_current.tda_feat[i].attr_flag.base_elev_flg = value;
    TDA_current.tda_feat[i].attr_flag.top_hgt_flg = value;
    TDA_current.tda_feat[i].attr_flag.max_shear_flg = value;
    TDA_current.tda_feat[i].attr_flag.max_shear_hgt_flg = value;
  } 
}  /* end of Update_data_array() */

/************************** Associate_storm_ID() ****************
**
**  Description:	It assigns a storm ID to each (E)TVS feature
**			based on the information from SCIT. If SCIT
**			data is not available yet, it assigns ?? to 
**			the features.
**  Inputs:		tda_feat_ptr, num_feat
**  Outputs:		tda_feat_ptr
**  Returns:		No
**  Globals:		SCIT_saved, SCIT_track
**  Notes:
**
*/

void Associate_storm_ID(tdaru_feat_t * tda_feat_ptr, int num_feat)
{
  int i, k, n_index, c_index;
  float x, y;
  float alpha;    /* feature's azimuth angle in radian */
  float delta;    /* feature's elevation angle in radian */
  float min_dist; /* minimum distance found between feat and storm */
  float dist_x;   /* x distance between feature and storm */
  float dist_y;   /* y distance between feature and storm */
  float dist;     /* distance between feature and storm */
  int stm_num = 0;

  char numbers[] = "0123456789";
  char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int  NUM_ALPHA = 26;

  if(SCIT_saved)  /* SCIT track data is available */
  {
    if(num_feat > 1)  /* array of features */
    {
      for(i = 0; i < num_feat; i++)
      {
        alpha = tda_feat_ptr[i].feature.base_az * DEG_TO_RAD;
        delta = tda_feat_ptr[i].feature.base_elev * DEG_TO_RAD;

          /* convert to Cartesian */
        x = tda_feat_ptr[i].feature.base_rng * sin(alpha) * cos(delta);
        y = tda_feat_ptr[i].feature.base_rng * cos(alpha) * cos(delta);

          /* initialize the min_dist to a big value */
        min_dist = 999.0;

          /* go through all the storms in SCIT track */
        for(k = 0; k < SCIT_track.hdr.bnt; k++)
        {
            /* calculate the distances between feature and storms */
          dist_x = SCIT_track.bsm[k].x0 - x;
          dist_y = SCIT_track.bsm[k].y0 - y;

          dist = sqrt(dist_x * dist_x + dist_y * dist_y);
    
            /* get the closest storm's id */
          if(dist < min_dist)
          {
            min_dist = dist;
            stm_num = SCIT_track.bsi[k].num_id;
          }
        }  /* end of for(k = 0; k < SCIT_track.hdr.bnt; k++) */ 

          /*
          ** The storm IDs are arranged as A0 for the first one,
          ** B0 for the sencond, C0 for the third, ..., and Z0
          ** for the 26th, then A1 for the 27th, ....
          ** So the maximum number for the num_id is 260 (26 * 10).
          ** We use integer arithmetic for truncation to get the 
          ** proper index into the numbers[] and letters[].
          */
        if (min_dist < Max_assoc_dist)
        {
           n_index = (stm_num - 1) / NUM_ALPHA;
           c_index = (stm_num - (n_index * NUM_ALPHA)) - 1;

             /* assign the storm ID to the feature */
           tda_feat_ptr[i].storm_id[0] = letters[c_index];
           tda_feat_ptr[i].storm_id[1] = numbers[n_index];
           tda_feat_ptr[i].storm_id[2] = '\0';
        }
        else
        {
           strcpy(tda_feat_ptr[i].storm_id, NO_ID);
        }
 
      }  /* end of for(i = 0; i < num_feat; i++) */
    }
    else if(num_feat == 1)  /* single feature */
    {
      alpha = tda_feat_ptr->feature.base_az * DEG_TO_RAD; 
      delta = tda_feat_ptr->feature.base_elev * DEG_TO_RAD;

        /* convert to Cartesian */
      x = tda_feat_ptr->feature.base_rng * sin(alpha) * cos(delta);
      y = tda_feat_ptr->feature.base_rng * cos(alpha) * cos(delta);

        /* initialize the min_dist to a big value */
      min_dist = 999.0;

        /* go through all the storms in SCIT track */
      for(k = 0; k < SCIT_track.hdr.bnt; k++)
      {
          /* calculate the distances between feature and storms */
        dist_x = SCIT_track.bsm[k].x0 - x;
        dist_y = SCIT_track.bsm[k].y0 - y;

        dist = sqrt(dist_x * dist_x + dist_y * dist_y);
    
          /* get the closest storm's id */
        if(dist < min_dist)
        {
          min_dist = dist;
          stm_num = SCIT_track.bsi[k].num_id;
        }
      }  /* end of for(k = 0; k < SCIT_track.hdr.bnt; k++) */ 

        /*
        ** The storm IDs are arranged as A0 for the first one,
        ** B0 for the sencond, C0 for the third, ..., and Z0
        ** for the 26th, then A1 for the 27th, ....
        ** So the maximum number for the num_id is 260 (26 * 10).
        ** We use integer arithmetic for truncation to get the 
        ** proper index into the numbers[] and letters[].
        */
      if (min_dist < Max_assoc_dist)
      {
         n_index = (stm_num - 1) / NUM_ALPHA;
         c_index = (stm_num - (n_index * NUM_ALPHA)) - 1;

           /* assign the storm ID to the feature */
         tda_feat_ptr->storm_id[0] = letters[c_index];
         tda_feat_ptr->storm_id[1] = numbers[n_index];
         tda_feat_ptr->storm_id[2] = '\0';
      }
      else
      {
         strcpy(tda_feat_ptr->storm_id, NO_ID);
      }
 
    }   /* end of if(num_feat > 1) else */ 
  }  
  else  /* SCIT is not available */
  {
    if(num_feat > 1)  /* array of features */
    {
      for(i = 0; i < num_feat; i++)
        strcpy(tda_feat_ptr[i].storm_id, NO_ID);
    }
    else if(num_feat == 1)
    {
      strcpy(tda_feat_ptr->storm_id, NO_ID);
    }
  }  /* end of if(SCIT_saved) else */
}  /* end of Associate_storm_ID() */


/********************** Advance_features() **********************
**
**  Description:	Based on the storm average motion of previous
**			volume scan from MDA track or SCIT if MDA 
** 			track information is not available yet
**			to predict the positions of the features
**			in current volume scan.
**  Inputs:		elapsed_time (in seconds) between volumes
**  Outputs:		No
**  Returns:		No
**  Globals:		TVS_pre_vol, TDA_current, MDA_saved, SCIT_saved,
**			SCIT_adapt
**  Notes:
**
*/

void Advance_features(int elapsed_time)
{
  float  xfcst, yfcst;   /* forecast X and Y position */
  double range, az;
  double x, y;
  double x_spd, y_spd;   /* average speed in x and y direction */
  double direction;      /* average direction */ 

  int i, num_feats;

  num_feats = TVS_pre_vol.hdr.num_tvs + TVS_pre_vol.hdr.num_etvs;

  if(MDA_saved)  /* use MDA motion */
  {
    #ifdef DEBUG
      fprintf(stderr, "\n>Advance_features(): Using MDA to advance\n");
    #endif
    
    for(i = 0; i < num_feats; i++)
    {
        /* convert the feature's position to cartesian x & y */
      RPGCS_azranelev_to_xy((double)TVS_pre_vol.features[i].base_rng,
                            (double)TVS_pre_vol.features[i].base_az,
                            (double)TVS_pre_vol.features[i].base_elev,
                            &x,&y);

      x_spd = MDA_track.x_spd;
      y_spd = MDA_track.y_spd;

      xfcst = (float)(x + x_spd * elapsed_time / METERS_PER_KM); 
      yfcst = (float)(y + y_spd * elapsed_time / METERS_PER_KM);

        /* convert back to polar coordinates */
      RPGCS_xy_to_azran((double)xfcst, (double)yfcst, &range, &az); 

        /* update advanced position */
      TVS_pre_vol.features[i].base_rng = (float)range;
      TVS_pre_vol.features[i].base_az = (float)az;

        /* copy the TVS_pre_vol to TDA_current */
      TDA_current.tda_feat[i].feature = TVS_pre_vol.features[i];

        /* set feature status to EXT */
      TDA_current.tda_feat[i].feat_status = EXT;
      TDA_current.tda_feat[i].matched_flag = FALSE;

        /* set attribute update flags to SPACE */
      TDA_current.tda_feat[i].attr_flag.type_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.az_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.rng_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.ll_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.avg_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_DV_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.depth_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.base_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.base_elev_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.top_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_shear_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_shear_hgt_flg = SPACE;
    }  /* end of for(i = 0; i < num_feats; i++) */ 
  }
  else if(SCIT_saved)  /* use SCIT motion */
  {
    #ifdef DEBUG
      fprintf(stderr, "\n>Advance_features(): Using SCIT to advance\n");
    #endif

      /*
      ** Use the average storm motion to advance the features.
      ** The average motion is received in polar coordinates.
      ** Reverse the direction so the sign is correct. 
      */    
    direction = SCIT_track.hdr.bvd - 180.0;

    if(direction < 0.0)
      direction += 360.0;

    for(i = 0; i < num_feats; i++)
    {
        /* convert the feature's position to cartesian x & y */
      RPGCS_azranelev_to_xy((double)TVS_pre_vol.features[i].base_rng,
                            (double)TVS_pre_vol.features[i].base_az,
                            (double)TVS_pre_vol.features[i].base_elev,
                            &x,&y);

        /* get the average speed in both x and y direction */
      RPGCS_azranelev_to_xy((double)SCIT_track.hdr.bvs, direction,
                            (double)TVS_pre_vol.features[i].base_elev,
                            &x_spd, &y_spd);

      xfcst = (float)(x + x_spd * elapsed_time / METERS_PER_KM); 
      yfcst = (float)(y + y_spd * elapsed_time / METERS_PER_KM);

        /* convert back to polar coordinates */
      RPGCS_xy_to_azran((double)xfcst, (double)yfcst, &range, &az); 

        /* update advanced position */
      TVS_pre_vol.features[i].base_rng = (float)range;
      TVS_pre_vol.features[i].base_az = (float)az;

        /* copy the TVS_pre_vol to TDA_current */
      TDA_current.tda_feat[i].feature = TVS_pre_vol.features[i];

        /* set feature status to EXT */
      TDA_current.tda_feat[i].feat_status = EXT;
      TDA_current.tda_feat[i].matched_flag = FALSE;

        /* set attribute update flags to SPACE */
      TDA_current.tda_feat[i].attr_flag.type_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.az_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.rng_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.ll_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.avg_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_DV_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.depth_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.base_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.base_elev_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.top_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_shear_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_shear_hgt_flg = SPACE;
    } /* end of for(i = 0; i < num_feats; i++) */  
  } 
  else   /* use default SCIT adaptation data */
  {
    float spd;

    #ifdef DEBUG
      fprintf(stderr, "\n>Advance_features(): Using SCIT adapt data to advance\n");
    #endif

      /* get default derection */
    direction = SCIT_adapt.default_dir;

    if(direction < 0.0)
      direction += 360.0;

      /* get default speed and convert it to m/s */
    spd = SCIT_adapt.default_spd * KNOT_TO_M_PER_SEC;
     
    for(i = 0; i < num_feats; i++)
    {
        /* convert the feature's position to cartesian x & y */
      RPGCS_azranelev_to_xy((double)TVS_pre_vol.features[i].base_rng,
                            (double)TVS_pre_vol.features[i].base_az,
                            (double)TVS_pre_vol.features[i].base_elev,
                            &x,&y);

        /* get the average speed in both x and y direction */
      RPGCS_azranelev_to_xy((double)SCIT_track.hdr.bvs, direction,
                            (double)TVS_pre_vol.features[i].base_elev,
                            &x_spd, &y_spd);

      xfcst = (float)(x + x_spd * elapsed_time / METERS_PER_KM); 
      yfcst = (float)(y + y_spd * elapsed_time / METERS_PER_KM);

        /* convert back to polar coordinates */
      RPGCS_xy_to_azran((double)xfcst, (double)yfcst, &range, &az); 

        /* update advanced position */
      TVS_pre_vol.features[i].base_rng = (float)range;
      TVS_pre_vol.features[i].base_az = (float)az;

        /* copy the TVS_pre_vol to TDA_current */
      TDA_current.tda_feat[i].feature = TVS_pre_vol.features[i];

        /* set feature status to EXT */
      TDA_current.tda_feat[i].feat_status = EXT;
      TDA_current.tda_feat[i].matched_flag = FALSE;

        /* set attribute update flags to SPACE */
      TDA_current.tda_feat[i].attr_flag.type_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.az_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.rng_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.ll_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.avg_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_DV_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_DV_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.depth_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.base_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.base_elev_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.top_hgt_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_shear_flg = SPACE;
      TDA_current.tda_feat[i].attr_flag.max_shear_hgt_flg = SPACE;
    }  /* for(i = 0; i < num_feats; i++) */ 
  } /* end of if(MDA_saved) else */

    /* set Feat_advanced to TRUE */
  Feat_advanced = TRUE;

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Advance_features(): Feature advanced: \n");
    Print_tvsru_info(TDA_current);
  #endif

}  /* end of Advance_features() */

/************************* Save_tvs() ***************************
**
**  Description:	It saves TVS data at the end of each volume.
**  Inputs:		tvs_input
**  Outputs:		No
**  Returns:		No
**  Globals:		TVS_saved, TVS_pre_vol, Scan_summary_tbl 
**  Notes:
**
*/

void Save_tvs(tvs_feat_t * tvs_input)
{
  int num_feats;

  num_feats = abs(tvs_input->hdr.num_tvs) + 
              abs(tvs_input->hdr.num_etvs);

  if(num_feats <= 0)
  {
    #ifdef DEBUG
      fprintf(stderr, "\n>Save_tvs(): No feature to save \n");
    #endif

    TVS_saved = FALSE;
    return;
  }

    /* save the TVS data */
  TVS_pre_vol = *tvs_input; 
  TVS_pre_vol.hdr.num_tvs = abs(tvs_input->hdr.num_tvs);
  TVS_pre_vol.hdr.num_etvs = abs(tvs_input->hdr.num_etvs); 
  TVS_pre_vol.hdr.vol_time = Scan_summary_tbl.volume_start_time;
  TVS_pre_vol.hdr.vol_date = Scan_summary_tbl.volume_start_date;

  #ifdef DEBUG
    fprintf(stderr, "\n>Save_tvs(): Saved TVS Input:\n");
    Print_tvs_info(TVS_pre_vol);
  #endif

    /* set the TVS_saved flag */
  TVS_saved = TRUE;
 
}  /* end of Save_tvs() */ 

/************************* Match_update_features() *******************
**
**  Description:	It matches the previous saved features to 
**			to currently detected features. If match found
**			it updates the attributes to the current feature
**			based on certain rules. It removes all the features
**			with status of EXT if this is the last elevation in
**			the volume. It then sorts the features based on rules, 
**			such as, TVS is followed by ETVS and the features with 
**			greater LLDV is followed the one with lower LLDV.
**  Inputs:		tvs_input, elapsed_time
**  Outputs:		output
**  Returns:		No
**  Globals:		Last_elev_flag
**  Notes:
**
*/

void Match_update_features(tvs_feat_t * tvs_input, 
                           tdaru_prod_t * output,
                           int elapsed_time)
{
  int i;
  int found_match;      /* index returned from Match_found() */
  int num_detected_tvs; /* number of TVSs detected in this volume so far */
  float max_dist;       /* max search radius in km */

    /* get search radius */
  max_dist = Correlation_speed * elapsed_time / METERS_PER_KM;


    /* get total number of TVSs and ETVSs so far in this volume */
  num_detected_tvs = abs(tvs_input->hdr.num_tvs) + 
                     abs(tvs_input->hdr.num_etvs);

    /* process all features detected so far in current volume */
  for(i = 0; i < num_detected_tvs; i++)
  {
      /* get matched index */
    found_match = Match_found(tvs_input->features[i], output,
                              max_dist);

    if(found_match >= 0)  /* match found */
    {
      #ifdef DEBUG
        fprintf(stderr, "\n>>>> Match_update_features(): before Update_feat()\n");
        Print_tvsru_info(*output);
      #endif

        /* update attributes */
      Update_feat(tvs_input->features[i], &output->tda_feat[found_match]);

      #ifdef DEBUG
        fprintf(stderr, "\n>>>> Match_update_features(): after Update_feat()\n");
        Print_tvsru_info(*output);
      #endif
    }
    else   /* no match */
    {
        /* add new feature */
      Add_new_feat(tvs_input->features[i], output);
    }   
  }  /* end of for(i = 0; i < num_detectd_tvs; i++) */

    /* remove all features with EXT status at the last elevation */
  if(Last_elev_flag)  
    Remove_ext_feat(output);
  
} /* end of Match_update_features() */

/********************** Match_found() **************************
**
**  Description:	It returns the index of the matched entry 
**			if a match is found.
**  Inputs:		tvs_feat_input, max_dist
**  Outputs:		feat_output
**  Returns:		matched index
**  Globals:		No
**  Notes:
**
*/

int Match_found(tvs_attr_t tvs_feat_input, 
                tdaru_prod_t * feat_output, 
                float max_dist)
{
  float closest_dist = 999.0, dist;
  int i, index = -1;

    /* go through all the features in output array (previous volume) */
  for(i = 0; i < feat_output->hdr.num_features; i++)
  {
      /* skip matched features */
    if(feat_output->tda_feat[i].matched_flag || 
       feat_output->tda_feat[i].feat_status == NEW)
      continue;

      /* calculate the distance between two features */
    dist = Get_distance(tvs_feat_input.base_rng,
                        tvs_feat_input.base_az,
                        tvs_feat_input.base_elev,
                        feat_output->tda_feat[i].feature.base_rng,
                        feat_output->tda_feat[i].feature.base_az,
                        feat_output->tda_feat[i].feature.base_elev);

    #ifdef DEBUG 
      fprintf(stderr, "\n>Match_found(): closest_dist -> %3.1f, dist -> %3.1f, max_dist -> %3.1f\n",
               closest_dist, dist, max_dist);
    #endif 

    if((dist < max_dist) && (dist < closest_dist))
    {
      closest_dist = dist;
      index = i;
    }
  }  /* end of for(i = 0; i < feat_output->hdr.num_features; i++) */

  return index; 
} /* end of Match_found() */

/************************ Get_distance() *************************
**
**  Description:	It gets the distance between two features.
**  Inputs:		rng1, az1, elev1, rng2, az2, elev2
**  Outputs:		No
**  Returns:		distance between two features
**  Globals:		No
**  Notes:
**
*/

float Get_distance(float rng1, float az1, float elev1,
                 float rng2, float az2, float elev2)
{
  float x1, x2, y1, y2, x_dist, y_dist;
  float distance;
  float alpha, theta;

    /* convert to Cartesian */
  alpha = az1 * DEG_TO_RAD;
  theta = elev1 * DEG_TO_RAD;

  x1 = rng1 * sin(alpha) * cos(theta);
  y1 = rng1 * cos(alpha) * cos(theta);

  alpha = az2 * DEG_TO_RAD;
  theta = elev2 * DEG_TO_RAD;

  x2 = rng2 * sin(alpha) * cos(theta);
  y2 = rng2 * cos(alpha) * cos(theta);

  x_dist = x1 - x2;
  y_dist = y1 - y2;

    /* get distance */
  distance = sqrt(x_dist * x_dist + y_dist * y_dist);

  return distance;

}  /* end of Get_distance() */

/*********************** Update_feat() **************************
**
**  Description:	It updates the attributes of the matched 
**			feature to the current feature found based 
**			on certain rules.
**  Inputs:		attr
**  Outputs:		outptr
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

void Update_feat(tvs_attr_t attr, tdaru_feat_t * outptr)
{
    /* set matched_flag to TRUE */
  outptr->matched_flag = TRUE;

    /*
    ** For position attributes, always update base azimuth, 
    ** range, and base height to current. 
    */
  outptr->feature.base_az = attr.base_az;
  outptr->feature.base_rng = attr.base_rng;
  outptr->feature.base_height = attr.base_height;
  outptr->attr_flag.az_flg = CARET;
  outptr->attr_flag.rng_flg = CARET;
  outptr->attr_flag.base_hgt_flg = CARET;


    /*
    ** If the max delta velocity or shear is updated, the height 
    ** of that attribute is also updated.
    */
  if(attr.max_DV > outptr->feature.max_DV)
  {
    outptr->feature.max_DV = attr.max_DV;
    outptr->feature.max_DV_height = attr.max_DV_height;
    outptr->attr_flag.max_DV_flg = CARET;
    outptr->attr_flag.max_DV_hgt_flg = CARET;
  }

  if(attr.max_shear > outptr->feature.max_shear)
  {
    outptr->feature.max_shear = attr.max_shear;
    outptr->feature.max_shear_height = attr.max_shear_height;
    outptr->attr_flag.max_shear_flg = CARET;
    outptr->attr_flag.max_shear_hgt_flg = CARET;
  }

  if(attr.average_DV > outptr->feature.average_DV)
  {
    outptr->feature.average_DV = attr.average_DV;
    outptr->attr_flag.avg_DV_flg = CARET;
  }


    /*
    ** For strength attributes, such as feature type and low-level
    ** delta velocity, are updated to the current values if increasing 
    ** in magnitude.
    ** Top height and depth are updated if the top of the
    ** current detection is taller.
    ** The feature status is also updated accordingly. If increasing 
    ** in magnitude, the status should be updated to INC. If no change,
    ** the status should be PER. 
    */

  if((attr.low_level_DV > outptr->feature.low_level_DV) ||
     ((outptr->feature.feature_type == ETVS) && (attr.feature_type == TVS)) ||
     (fabs((double)attr.top_height) > fabs((double)outptr->feature.top_height))) 
  {
      /* update status to INC */
    outptr->feat_status = INC;

      /* update feature type */
    if((outptr->feature.feature_type == ETVS) &&
       (attr.feature_type == TVS))
    {
      outptr->feature.feature_type = attr.feature_type;
      outptr->attr_flag.type_flg = CARET;
      TDA_current.hdr.num_tvs++;
      TDA_current.hdr.num_etvs--;
    }

      /* update attributes as needed */
    if(attr.low_level_DV > outptr->feature.low_level_DV)
    {
      outptr->feature.low_level_DV = attr.low_level_DV;
      outptr->attr_flag.ll_DV_flg = CARET;
    }

    if(fabs((double)attr.top_height) > 
       fabs((double)outptr->feature.top_height))
    {
      outptr->feature.top_height = attr.top_height;
      outptr->feature.depth = attr.depth;
      outptr->attr_flag.top_hgt_flg = CARET;
      outptr->attr_flag.depth_flg = CARET;
    }

  }
  else  /* no change in strength */
  {
      /* update status to PER */
    outptr->feat_status = PER;
  }
 
} /* end of Update_feat() */

/************************ Add_new_feat() ***********************
**
**  Description:	Add new features found in current volume
**			to the output data array.
**  Inputs:		attr
**  Outputs:		outptr
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

void Add_new_feat(tvs_attr_t attr, tdaru_prod_t * outptr)
{
  int last_index;

    /* get the index where the new feat is to be added */
  last_index = outptr->hdr.num_features;

    /* increase feature number by 1 */
  outptr->hdr.num_features++;

  if(attr.feature_type == TVS)
    outptr->hdr.num_tvs++;
  else
    outptr->hdr.num_etvs++;

    /* set feature status to NEW */
  outptr->tda_feat[last_index].feat_status = NEW;
 
    /* put in attributes of the feature */
  outptr->tda_feat[last_index].feature.feature_type = attr.feature_type;
  outptr->tda_feat[last_index].feature.base_az = attr.base_az;
  outptr->tda_feat[last_index].feature.base_rng = attr.base_rng;
  outptr->tda_feat[last_index].feature.low_level_DV = attr.low_level_DV;
  outptr->tda_feat[last_index].feature.average_DV = attr.average_DV;
  outptr->tda_feat[last_index].feature.max_DV = attr.max_DV;
  outptr->tda_feat[last_index].feature.max_DV_height = attr.max_DV_height;
  outptr->tda_feat[last_index].feature.depth = attr.depth;
  outptr->tda_feat[last_index].feature.base_height = attr.base_height;
  outptr->tda_feat[last_index].feature.base_elev = attr.base_elev;
  outptr->tda_feat[last_index].feature.top_height = attr.top_height;
  outptr->tda_feat[last_index].feature.max_shear = attr.max_shear;
  outptr->tda_feat[last_index].feature.max_shear_height = attr.max_shear_height;

    /* set the attribute update flags to current */
  outptr->tda_feat[last_index].attr_flag.type_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.az_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.rng_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.ll_DV_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.avg_DV_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.max_DV_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.max_DV_hgt_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.depth_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.base_hgt_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.base_elev_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.top_hgt_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.max_shear_flg = CARET;
  outptr->tda_feat[last_index].attr_flag.max_shear_hgt_flg = CARET; 
  
 
    /* assign a storm ID to the new feature */
  Associate_storm_ID(&outptr->tda_feat[last_index], 1);

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Add_new_feat(): a new feature added\n");
    Print_tvsru_info(*outptr);
  #endif

}  /* end of Add_new_feat() */

/************************** Remove_ext_feat() **************************
**
**  Description:	It removes all features with EXT status at the
**			end of volume scan.
**  Inputs:		outptr
**  Outputs:		outptr
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

void Remove_ext_feat(tdaru_prod_t * outptr)
{
  int i, n, k = 0;
  tdaru_prod_t temp;
  int feat_count = 0;

  if(outptr->hdr.num_features <= 0)
    return;

    /* get the loop limit */
  n = outptr->hdr.num_features;

    /* get a copy of the contents of the *outptr structure */
  temp = *outptr;

    /* remove the features with EXT status */ 
  for(i = 0; i < n; i++)
  {
    if(temp.tda_feat[i].feat_status != EXT)
    {
        /* increase feat_count by 1 */
      feat_count++;

      outptr->tda_feat[k++] = temp.tda_feat[i]; 
    }
    else
    {
      if(temp.tda_feat[i].feature.feature_type == TVS)
        outptr->hdr.num_tvs--;
      else
        outptr->hdr.num_etvs--;
    }
  }

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Remove_ext_feat(): number before remove = %d ",
            outptr->hdr.num_features);
  #endif

    /* update feature number */
  outptr->hdr.num_features = feat_count;

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Remove_ext_feat(): number after remove = %d ",
            outptr->hdr.num_features);
  #endif
}  /* end of Remove_ext_feat() */


/************************** Remove_false_feat() **************************
**
**  Description:	It removes all features with ?? storm_id.
**  Inputs:		outptr
**  Outputs:		outptr
**  Returns:		No
**  Globals:		No
**  Notes:              Assumption is that the feature is a false alarm 
**                      if no SCIT cell is nearby.
**
*/

void Remove_false_feat(tdaru_prod_t * outptr)
{
  int i, n, k = 0;
  tdaru_prod_t temp;
  int feat_count = 0;

  if(outptr->hdr.num_features <= 0)
    return;

    /* get the loop limit */
  n = outptr->hdr.num_features;

    /* get a copy of the contents of the *outptr structure */
  temp = *outptr;

    /* remove the features with ?? storm_id */ 
  for(i = 0; i < n; i++)
  {
    if(strncmp(NO_ID, temp.tda_feat[i].storm_id, ID_LEN) != 0)
    {
        /* increase feat_count by 1 */
      feat_count++;

      outptr->tda_feat[k++] = temp.tda_feat[i]; 
    }
    else
    {
      if(temp.tda_feat[i].feature.feature_type == TVS)
        outptr->hdr.num_tvs--;
      else
        outptr->hdr.num_etvs--;
    }
  }

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Remove_false_feat(): number before remove = %d ",
            outptr->hdr.num_features);
  #endif

    /* update feature number */
  outptr->hdr.num_features = feat_count;

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Remove_false_feat(): number after remove = %d ",
            outptr->hdr.num_features);
  #endif
}  /* end of Remove_false_feat() */

/************************** Sort_feat() *************************
**
**  Description:	It sorts all the features in output data array
**			in the order of TVS followed by ETVS with the 
**			strongest LLDV in the first. 
**			After sorting, if number of features exceed the 
**			max number allowed in adaptation data, it calls
**			Drop_feat() to drop features until it meets the
**			limits.
**  Inputs:		outptr
**  Outputs:		outptr
**  Returns:		No
**  Globals:		Max_num_tvs, Max_num_etvs
**  Notes:
**
*/

void Sort_feat(tdaru_prod_t * outptr)
{
  int i, j, n = 0, k = 0;
  tdaru_prod_t temp;
  int tvs_count = 0, etvs_count = 0;
  tdaru_feat_t strongest, swap;
  int swap_index = -1;

    /* if no feature, return. */
  if(outptr->hdr.num_features <= 0)
    return;

    /* make a local copy of the output data array */
  temp = *outptr;

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Sort_feat(): TVSs = %d, ETVSs = %d\n",
            outptr->hdr.num_tvs, outptr->hdr.num_etvs);
    fprintf(stderr, "\n>>>> Sort_feat(): copy of output data array\n");
    Print_tvsru_info(temp);
    fprintf(stderr, "\n>>>> Sort_feat(): original of output data array\n");
    Print_tvsru_info(*outptr);
  #endif   

  n = outptr->hdr.num_features;

    /* put TVS type features in front */
  if(outptr->hdr.num_etvs > 0)
    for(i = 0; i < n; i++)
    {
      if(temp.tda_feat[i].feature.feature_type == TVS)
      {
        tvs_count++;
        outptr->tda_feat[k++] = temp.tda_feat[i];
      }
    }
  else
    tvs_count = n;
  
    /* sort the TVS features based on LLDV */ 
  if(tvs_count > 1)
  {
    for(i = 0; i < tvs_count; i++)
    {
      strongest = outptr->tda_feat[i];
      swap = outptr->tda_feat[i];

      for(j = i+1; j < tvs_count; j++)
      {
        if(outptr->tda_feat[j].feature.low_level_DV > 
                     strongest.feature.low_level_DV)
        {
          strongest = outptr->tda_feat[j];
          swap_index = j;
        }
      }   

      if(swap_index > 0)
      {
        outptr->tda_feat[i] = strongest;
        outptr->tda_feat[swap_index] = swap; 
        swap_index = -1;
      }

    }
  }  /* end of if(tvs_count > 1) */

    /* put ETVS type features after TVS features */ 
  if(n > tvs_count)
  {
    for(i = 0; i < n; i++)
    {
      if(temp.tda_feat[i].feature.feature_type == ETVS)
      {
        etvs_count++;
        outptr->tda_feat[k++] = temp.tda_feat[i];
      }
    }

      /* reset the swap_index */
    swap_index = -1;
 
      /* sort the ETVS features based on LLDV */ 
    if(etvs_count > 1)
    {
      for(i = tvs_count; i < (etvs_count + tvs_count); i++)
      {
        strongest = outptr->tda_feat[i];
        swap = outptr->tda_feat[i];

        for(j = i+1; j < (tvs_count + etvs_count); j++)
        {
          if(outptr->tda_feat[j].feature.low_level_DV > 
                     strongest.feature.low_level_DV)
          {
            strongest = outptr->tda_feat[j];
            swap_index = j;
          }
        }   

        if(swap_index > 0)
        {
          outptr->tda_feat[i] = strongest;
          outptr->tda_feat[swap_index] = swap;
          swap_index = -1;
        } 
      }
    }  /* end of if(etvs_count > 1) */
  }  /* end of if(n > tvs_count) */

    /* check the number of TVSs and ETVSs */
  if(outptr->hdr.num_tvs > Max_num_tvs)
    Max_num_tvs_exceeded = TRUE;
  else
    Max_num_tvs_exceeded = FALSE;

  if(outptr->hdr.num_etvs > Max_num_etvs)
    Max_num_etvs_exceeded = TRUE;
  else
    Max_num_etvs_exceeded = FALSE;

    /* drop the features */
  if((outptr->hdr.num_tvs > Max_num_tvs) || 
     (outptr->hdr.num_etvs > Max_num_etvs))
    Drop_feat(outptr);

  #ifdef DEBUG 
    fprintf(stderr, "\n>>>> Sort_feat(): max number of feat -> %d\n", Max_num_feat);
    Print_tvsru_info(*outptr);
  #endif
}  /* end of Sort_feat() */

/************************** Drop_feat() *************************
**
**  Description:	It begins to drop features from the end of 
**			TVS/ETVS list in output data array. 
**  Inputs:		outptr
**  Outputs:		outptr
**  Returns:		No
**  Globals:		Max_num_tvs, Max_num_etvs
**  Notes:
**
*/

void Drop_feat(tdaru_prod_t * outptr)
{
  int old_num_tvs = outptr->hdr.num_tvs;
  int old_num_etvs = outptr->hdr.num_etvs;
  int old_num_feat = outptr->hdr.num_features;

    /* number of TVS exceeds the limit */ 
  if(old_num_tvs > Max_num_tvs)
  {
      /* set number of TVS to the limit */
    outptr->hdr.num_tvs = Max_num_tvs;

      /* put ETVS features into proper positions */
    if((Max_num_etvs > 0) && (old_num_etvs > 0))
    {
      int new_etvs_start_index = Max_num_tvs;
      int old_etvs_start_index; 

      for(old_etvs_start_index = old_num_tvs; 
          old_etvs_start_index < old_num_feat; 
          old_etvs_start_index++)
      {
        outptr->tda_feat[new_etvs_start_index++] =
                        outptr->tda_feat[old_etvs_start_index]; 
      }
    }
  }  /* end of if(old_num_tvs > Max_num_tvs) */

    /* number of ETVS exccedds the limit */
  if(old_num_etvs > Max_num_etvs)
    outptr->hdr.num_etvs = Max_num_etvs;

    /* update the total number of features */
  outptr->hdr.num_features = outptr->hdr.num_tvs +
                             outptr->hdr.num_etvs;

}  /* end of Drop_feat() */

/************************** Build_ICD_product() ******************
**
**  Description:	It builds the final ICD product from output
**			data array.
**  Inputs:		data, vol_num
**  Outputs:		No
**  Returns:		ABORT/CONTINUE	
**  Globals:		No
**  Notes:
**
*/

int Build_ICD_product(tdaru_prod_t * data, int vol_num)
{
  unsigned int temp, length = SYM_BLK_OFFSET;
  unsigned char * out_buf = NULL;
  int opstatus;
  Graphic_product * prod_hdr;

    /* get output buffer */
  out_buf = (unsigned char *)RPGC_get_outbuf(TRU, OUTBUF_SIZE, &opstatus);

  if(opstatus != NORMAL)
  {
    LE_send_msg(GL_INFO, "\n>RPGC_get_outbuf() error. Aborting ...\n");
    return ABORT;
  }

     /* get product header pointer */ 
  prod_hdr = (Graphic_product *)out_buf;

    /* system call to build PDB */
  RPGC_prod_desc_block((void *)out_buf, TRU, vol_num);

    /* build Product Symbology Block (PSB) */
  temp  = Build_PSB(data, &length, out_buf);
  RPGC_set_product_int( (void *) &prod_hdr->sym_off, temp );

    /* build GAB */
  temp = Build_GAB(data, &length, out_buf);
  RPGC_set_product_int( (void *) &prod_hdr->gra_off, temp );

    /* build TAB */
  temp = Build_TAB(data, &length, out_buf, vol_num);
  RPGC_set_product_int( (void *) &prod_hdr->tab_off, temp );

    /* finish Product Description Block (PDB) */
  prod_hdr->param_3 = (short)(RPGC_NINT( data->hdr.current_elev * 10.0 ));  
  prod_hdr->param_4 = (short)(data->hdr.num_tvs);  /* number of TVSs */
  prod_hdr->param_5 = (short)(data->hdr.num_etvs); /* number of ETVSs */ 

    /* update length */
  length -= SYM_BLK_OFFSET;

    /* system call to build Message header block */ 
  if(RPGC_prod_hdr((void *)out_buf, TRU, &length) != 0)
  {
    LE_send_msg(GL_INFO, "\n>RPGC_prod_hdr() error, aborting ...\n");
    RPGC_rel_outbuf((void *)out_buf, DESTROY); 
    return ABORT;
  }
  else  /* forward product */ 
  {
    RPGC_rel_outbuf((void *)out_buf, FORWARD);
  }


  return CONTINUE;
}  /* end of Build_ICD_product() */


/************************* Build_PSB() **************************
**
**  Description:	It builds the symbology block.
**  Inputs:		data, length
**  Outputs:		length, out_buf
**  Returns:		offset of symbology bliock
**  Globals:		No
**  Notes:
**
*/

int Build_PSB(tdaru_prod_t * data, unsigned int * length,
              unsigned char * out_buf)
{
  pkt_15_t packet_15;
  pkt_20_t packet_20;
  sym_hdr_t sym_hdr;
  double x, y;
  int i;
  unsigned int temp;

    /* check if there is any feature */
  if(data->hdr.num_features <= 0)
    return 0;

    /* Set up symbology header. */
  sym_hdr.blk_divider = -1;
  sym_hdr.blk_id = 1;
  sym_hdr.num_layers = 1;
  sym_hdr.lyr_divider = -1;

    /* set length to the offset for the first packet */
  *length = SYM_PKT_OFFSET; 
 
    /* loop for features */
  for(i = 0; i < data->hdr.num_features; i++)
  {
      /* set packet hdr */
    packet_20.hdr.pkt_code = 20;
    packet_20.hdr.num_bytes = sizeof(pkt_20_point_t);

      /* get the x, y positions and convert range to km/4 */
    RPGCS_azranelev_to_xy((double)data->tda_feat[i].feature.base_rng * 4.0,
                          (double)data->tda_feat[i].feature.base_az,
                          (double)data->tda_feat[i].feature.base_elev,
                          &x, &y);

    packet_20.point.pos_i = (short)x;
    packet_20.point.pos_j = (short)y;
    packet_20.point.attr = 0; 
    
    if(data->tda_feat[i].feature.feature_type == TVS)
    {
      if(data->tda_feat[i].feat_status == EXT)
        packet_20.point.type = TVS_EXT; 
      else 
        packet_20.point.type = TVS_UPDATED; 
    }
    else    /* ETVS */
    {
      if(data->tda_feat[i].feat_status == EXT)
        packet_20.point.type = ETVS_EXT;
      else 
        packet_20.point.type = ETVS_UPDATED;
    } 

      /* set packet 15 hdr */
    packet_15.hdr.pkt_code = 15;
    packet_15.hdr.num_bytes = sizeof(pkt_15_data_t);

    packet_15.data.pos_i = (short)x;
    packet_15.data.pos_j = (short)y;
    packet_15.data.letter = data->tda_feat[i].storm_id[0];
    packet_15.data.number = data->tda_feat[i].storm_id[1];
   
      /* copy the packets into output buffer */
    memcpy((out_buf + (*length)), &packet_20, sizeof(pkt_20_t));
    *length += sizeof(pkt_20_t);

    memcpy((out_buf + (*length)), &packet_15, sizeof(pkt_15_t));
    *length += sizeof(pkt_15_t);
 
  }  /* end of for(i = 0; i < data->hdr.num_features; i++) */

  #ifdef DEBUG 
    fprintf(stderr, "\n>Build_PSB(): ");
    fprintf(stderr, "\n>pkt_15_t size = %d, pkt_15_data_t size = %d, pkt_20_t size = %d, pkt_20_point_t = %d\n",
            sizeof(pkt_15_t), sizeof(pkt_15_data_t), sizeof(pkt_20_t), sizeof(pkt_20_point_t));
  #endif

  temp = *length - SYM_PKT_OFFSET; 
  RPGC_set_product_int( (void *) &sym_hdr.lyr_length, temp );
  temp = *length - SYM_BLK_OFFSET; 
  RPGC_set_product_int( (void *) &sym_hdr.blk_length, temp );

    /* copy the sym_hdr to output buffer */
  memcpy((out_buf + SYM_BLK_OFFSET), &sym_hdr, sizeof(sym_hdr_t));

  return (SYM_BLK_OFFSET/2);

}  /* end of Build_PSB() */


/************************* Build_GAB() **************************
**
**  Description:	It builds GAB.
**  Inputs:		data, length
**  Outputs:		length, out_buf
**  Returns:		offset of the GAB
**  Globals:		No
**  Notes:
**
*/

int Build_GAB(tdaru_prod_t * data, unsigned int * length, 
               unsigned char * out_buf)
{
  #define NUM_HORIZ_VEC 6  /* number of horizontal vectors/lines(packet 10) */
  #define NUM_VERT_VEC 8   /* number of vertical vectors/lines(packet 10) */
  #define FEATS_PER_PAGE 6 /* max number of features per page */
  #define FIELD_WIDTH 70   /* I direction data field width in pixels */
  #define FIELD_HEIGHT 10  /* J direction data field height in pixels */
  #define GRID_COLOR 3     /* yellow color value for grid */
  #define I_INIT 4         /* I direction initial pixel value */
  #define J_INIT 0         /* J direction initial pixel value */
  #define J_INC 10         /* J direction increment */
  #define TEXT_COLOR 0     /* black */
  #define ROWS 5         /* text data rows in GAB */ 
  #define COLS 6         /* number of columns in GAB */
  #define CHS_FIELD 10   /* chars per field */
  #define CARET_INX_6 6  /* position of ^ */
  #define CARET_INX_1 1  /* position of ^ */
  #define PKT8_LENGTH 86 /* 80 per line + 3 short (6 bytes) */

    /* 
    ** There are total 5 rows of text packets (packet 8) and 6 rows
    ** + 8 column grid packets (packet 10). 
    ** For packet 8: 
    **              90 * 5 rows = 450 bytes
    ** For packet 10:
    **              8 bytes * 6 rows + 6 bytes = 54 bytes
    **              8 bytes * 8 col + 6 bytes  = 70 bytes
    ** Total: 450 + 54 + 70 = 574 bytes 
    */
  #define BYTES_PER_PAGE 574

  #define I_LENGTH (7 * FIELD_WIDTH)  /* total length in I direction */ 
  #define J_LENGTH (5 * FIELD_HEIGHT) /* total length in J direction */

  GAB_hdr_t gab;           /* one for all pages */
  GAB_page_hdr_t page_hdr; /* repeat for each page */
  
  unsigned int gab_start_pos, grid_start_pos, temp;
  short i, page, total_pages;
  int field_height, field_width;

  pkt_8_t text_pkt;
  pkt_10_hdr_t grid_hdr;
  pkt_10_data_t grid_point;
  char data_row[81];

  char depth[6], base_hgt[6];

  char feat_type[2][5] = {" TVS",
                          "ETVS"};
  char feat_status[4][4] = {"INC",
                            "NEW",
                            "PER",
                            "EXT"}; 
  char gab_hdr[ROWS][CHS_FIELD+1] = {" TYPE STID",
                                     " AZ    RAN",
                                     " LLDV  MDV",
                                     " STA AVGDV",
                                     " BASE DPTH"};
  char blanks[CHS_FIELD+1]        =  "          "; 
  char data_field[COLS][CHS_FIELD+1];

    /* If there is not any feature in data array, return 0 offset. */
  if(data->hdr.num_features <= 0)
    return 0;
 
    /* get the start position in output buffer */
  gab_start_pos = *length;

    /* get total pages in GAB */
  total_pages = (data->hdr.num_features - 1) / FEATS_PER_PAGE + 1;

    /* 
    ** Get GAB header information. 
    */
  gab.blk_divider = -1;
  gab.blk_id = 2;
  gab.num_pages = total_pages;

    /*
    ** GAB_hdr_t structure contains 10 bytes. However, the
    ** sizeof(GAB_hdr_t) is 12 bytes for alignment reason. So we need
    ** to take off 2 bytes off.
    */

  temp = total_pages * (BYTES_PER_PAGE + sizeof(GAB_page_hdr_t)) + 
                   sizeof(GAB_hdr_t) - sizeof(short); 
  RPGC_set_product_int( (void *) &gab.blk_length, temp );

    /* put gab in output buffer */
  memcpy((out_buf + gab_start_pos), &gab, (sizeof(GAB_hdr_t) - sizeof(short))); 

  #ifdef DEBUG
    fprintf(stderr, "\n>Build_GAB(): ");
    fprintf(stderr, "\n>GAB_hdr_t size = %d, gab size = %d\n", 
            sizeof(GAB_hdr_t), sizeof(gab));
    fprintf(stderr, "\n>pkt_8_t size = %d, pkt_10_hdr_t size = %d\n", 
            sizeof(pkt_8_t), sizeof(pkt_10_hdr_t));
  #endif
 
    /*
    ** Skip the bytes of GAB's header. 
    ** Account for alignment bytes (2 bytes) in the structure.
    */
  *length = *length + sizeof(GAB_hdr_t) - sizeof(short);

    /* loop for each page of data in GAB */
  for(page = 1; page <= total_pages; page++)
  {
    short fld = 0;
    int kk, jj;

      /* initialize data_field[][] */
    for(kk = 0; kk < COLS; kk++)
      for(jj = 0; jj < CHS_FIELD+1; jj++)
        data_field[kk][jj] = SPACE;

      /* get page hdr and put it in output buffer */
    page_hdr.page_num = page;
    page_hdr.page_length = BYTES_PER_PAGE;

    memcpy((out_buf + (*length)), &page_hdr, sizeof(page_hdr));
    *length += sizeof(page_hdr);

      /* get the initial values for text packet 8 */         
    text_pkt.pkt_code = 8;
    text_pkt.num_bytes = PKT8_LENGTH; 
    text_pkt.color_val = TEXT_COLOR;
    text_pkt.pos_i = 0;
    text_pkt.pos_j = 1;

    fld = 0;

      /*
      ** ROW 0: TYPE STID 
      */
    for(i = (FEATS_PER_PAGE * (page - 1)); 
       i < (FEATS_PER_PAGE * page); i++)
    {
      if(i < data->hdr.num_features)
      {
        if(data->tda_feat[i].feature.feature_type == TVS)
          sprintf(data_field[fld], " %s   %s", feat_type[0], 
                                            data->tda_feat[i].storm_id);
        else
          sprintf(data_field[fld], " %s   %s", feat_type[1], 
                                            data->tda_feat[i].storm_id);
    
          /* put update flag */
        data_field[fld][CARET_INX_6] = data->tda_feat[i].attr_flag.type_flg;
      }
      else
        sprintf(data_field[fld], "%s", blanks); 

      fld++;  /* next column field */
    }    

    sprintf(data_row, "%s%s%s%s%s%s%s%s",
            gab_hdr[0], data_field[0], data_field[1], data_field[2],
            data_field[3], data_field[4], data_field[5], blanks);


      /* put packet 8 into output buffer */
    memcpy(out_buf + (*length), &text_pkt, sizeof(pkt_8_t));
    *length += sizeof(pkt_8_t);

    memcpy((out_buf + (*length)), data_row, (sizeof(data_row) - 1));
    *length += (sizeof(data_row) - 1); 
   
      /*
      ** ROW 1:  AZ    RAN 
      */
    text_pkt.pos_j += J_INC;
    fld = 0;
 
    for(i = (FEATS_PER_PAGE * (page - 1));
        i < (FEATS_PER_PAGE * page); i++)
    {
      if(i < data->hdr.num_features)
      {
        sprintf(data_field[fld], "  %3.0f  %3.0f", 
                                 data->tda_feat[i].feature.base_az,
                                 data->tda_feat[i].feature.base_rng * NM_PER_KM);

          /* put update flag */
        data_field[fld][CARET_INX_6] = data->tda_feat[i].attr_flag.az_flg;
      }
      else
        sprintf(data_field[fld], "%s", blanks); 

      fld++;    /* next column field */
    }

    sprintf(data_row, "%s%s%s%s%s%s%s%s",
            gab_hdr[1], data_field[0], data_field[1], data_field[2],
            data_field[3], data_field[4], data_field[5], blanks);


      /* put packet 8 into output buffer */
    memcpy(out_buf + (*length), &text_pkt, sizeof(pkt_8_t));
    *length += sizeof(pkt_8_t);

    memcpy((out_buf + (*length)), data_row, (sizeof(data_row) - 1));
    *length += (sizeof(data_row) - 1);

      /*
      ** ROW 2:  LLDV  MDV
      */
    text_pkt.pos_j += J_INC;
    fld = 0;

    for(i = (FEATS_PER_PAGE * (page - 1));
        i < (FEATS_PER_PAGE * page); i++)
    {
      if(i < data->hdr.num_features)
      {
        sprintf(data_field[fld], "  %3.0f  %3.0f",
                (data->tda_feat[i].feature.low_level_DV * M_PER_SEC_TO_KNOT),
                (data->tda_feat[i].feature.max_DV * M_PER_SEC_TO_KNOT));

          /* put update flag */
        data_field[fld][CARET_INX_1] = data->tda_feat[i].attr_flag.ll_DV_flg;
        data_field[fld][CARET_INX_6] = data->tda_feat[i].attr_flag.max_DV_flg;
      }
      else
        sprintf(data_field[fld], "%s", blanks); 

      fld++;    /* next column field */
    }

    sprintf(data_row, "%s%s%s%s%s%s%s%s",
            gab_hdr[2], data_field[0], data_field[1], data_field[2],
            data_field[3], data_field[4], data_field[5], blanks);


      /* put packet 8 into output buffer */
    memcpy(out_buf + (*length), &text_pkt, sizeof(pkt_8_t));
    *length += sizeof(pkt_8_t);

    memcpy((out_buf + (*length)), data_row, (sizeof(data_row) - 1));
    *length += (sizeof(data_row) - 1);

      /*
      ** ROW 3:  STA AVGDV
      */
    text_pkt.pos_j += J_INC;
    fld = 0;

    for(i = (FEATS_PER_PAGE * (page - 1));
        i < (FEATS_PER_PAGE * page); i++)
    {
      if(i < data->hdr.num_features)
      {
        if(data->tda_feat[i].feat_status == INC)
          sprintf(data_field[fld], "  %s  %3.0f",
                                   feat_status[0], 
                  (data->tda_feat[i].feature.average_DV * M_PER_SEC_TO_KNOT));
        else if(data->tda_feat[i].feat_status == NEW)
          sprintf(data_field[fld], "  %s  %3.0f",
                                   feat_status[1], 
                  (data->tda_feat[i].feature.average_DV * M_PER_SEC_TO_KNOT));
        else if(data->tda_feat[i].feat_status == PER)
          sprintf(data_field[fld], "  %s  %3.0f",
                                   feat_status[2], 
                  (data->tda_feat[i].feature.average_DV * M_PER_SEC_TO_KNOT));
        else if(data->tda_feat[i].feat_status == EXT)
          sprintf(data_field[fld], "  %s  %3.0f",
                                   feat_status[3], 
                  (data->tda_feat[i].feature.average_DV * M_PER_SEC_TO_KNOT));

          /* put update flag */
        data_field[fld][CARET_INX_6] = data->tda_feat[i].attr_flag.avg_DV_flg;
      }
      else
        sprintf(data_field[fld], "%s", blanks); 

      fld++;    /* next column field */
    }

    sprintf(data_row, "%s%s%s%s%s%s%s%s",
            gab_hdr[3], data_field[0], data_field[1], data_field[2],
            data_field[3], data_field[4], data_field[5], blanks);


      /* put packet 8 into output buffer */
    memcpy(out_buf + (*length), &text_pkt, sizeof(pkt_8_t));
    *length += sizeof(pkt_8_t);

    memcpy((out_buf + (*length)), data_row, (sizeof(data_row) - 1));
    *length += (sizeof(data_row) - 1);

      /* 
      ** ROW 4:  BASE DPTH
      */
    text_pkt.pos_j += J_INC;
    fld = 0;

    for(i = (FEATS_PER_PAGE * (page - 1));
        i < (FEATS_PER_PAGE * page); i++)
    {
      if(i < data->hdr.num_features)
      {
        if(data->tda_feat[i].feature.base_height < 0)
          sprintf(base_hgt, "<%4.1f", 
                  ((-1) * data->tda_feat[i].feature.base_height * KFT_PER_KM));
        else
          sprintf(base_hgt, " %4.1f", 
                  (data->tda_feat[i].feature.base_height * KFT_PER_KM));

        if(data->tda_feat[i].feature.depth < 0)
          sprintf(depth, ">%2.0f", 
                  ((-1) * data->tda_feat[i].feature.depth * KFT_PER_KM));
        else
          sprintf(depth, " %2.0f", 
                  (data->tda_feat[i].feature.depth * KFT_PER_KM));
   
          sprintf(data_field[fld], " %s %s",
                                   base_hgt, depth);

          /* put update flag */
        data_field[fld][CARET_INX_6] = data->tda_feat[i].attr_flag.base_hgt_flg;
      }
      else
        sprintf(data_field[fld], "%s", blanks); 

      fld++;    /* next column field */
    }

    sprintf(data_row, "%s%s%s%s%s%s%s%s",
            gab_hdr[4], data_field[0], data_field[1], data_field[2],
            data_field[3], data_field[4], data_field[5], blanks);


      /* put packet 8 into output buffer */
    memcpy(out_buf + (*length), &text_pkt, sizeof(pkt_8_t));
    *length += sizeof(pkt_8_t);

    memcpy((out_buf + (*length)), data_row, (sizeof(data_row) - 1));
    *length += (sizeof(data_row) - 1);


      /* 
      ** put grid vectors (packet 10)
      */
 
      /* save horizontal grid position */
    grid_start_pos = *length;   

      /* move to the beginning of horizontal vector */
    *length += sizeof(pkt_10_hdr_t);

    grid_point.begin_i = I_INIT;
    grid_point.begin_j = J_INIT;
    field_height = 0;

    for(i = 0; i < NUM_HORIZ_VEC; i++)
    {
      grid_point.begin_j += field_height;
      grid_point.end_i = grid_point.begin_i + I_LENGTH;
      grid_point.end_j = grid_point.begin_j;
      field_height = FIELD_HEIGHT;

      memcpy((out_buf + (*length)), &grid_point, sizeof(pkt_10_data_t));
      *length += sizeof(pkt_10_data_t);
    }  /* end of for(i = 0; i < NUM_HORIZ_VEC; i++) */
 
      /* complete the packet hdr */
    grid_hdr.pkt_code = 10;
    grid_hdr.num_bytes = NUM_HORIZ_VEC * sizeof(pkt_10_data_t) +
                         sizeof(grid_hdr.color);
    grid_hdr.color = GRID_COLOR;

      /* put this header into output buffer */
    memcpy((out_buf + grid_start_pos), &grid_hdr, sizeof(pkt_10_hdr_t));

      /* save vertical grid position */
    grid_start_pos = *length;

      /* move to the beginning of vertical vector */
    *length += sizeof(pkt_10_hdr_t);

    grid_point.begin_i = I_INIT;
    grid_point.begin_j = J_INIT;
    field_width = 0;

    for(i = 0; i < NUM_VERT_VEC; i++)
    {
      grid_point.begin_i += field_width;
      grid_point.end_i = grid_point.begin_i;
      grid_point.end_j = grid_point.begin_j + J_LENGTH;
      field_width = FIELD_WIDTH;

      memcpy((out_buf + (*length)), &grid_point, sizeof(pkt_10_data_t));
      *length += sizeof(pkt_10_data_t);
    }  /* end of for(i = 0; i < NUM_VERT_VEC; i++) */

      /* complete the packet hdr */
    grid_hdr.pkt_code = 10;
    grid_hdr.num_bytes = NUM_VERT_VEC * sizeof(pkt_10_data_t) +
                         sizeof(grid_hdr.color);
    grid_hdr.color = GRID_COLOR;

      /* put this header into output buffer */
    memcpy((out_buf + grid_start_pos), &grid_hdr, sizeof(pkt_10_hdr_t));
    

  }  /* end of for(page = 1; page <= total_pages; page++) */

    /* return GAB offset in short */
  return (gab_start_pos / 2); 
}  /* end of Build_GAB() */

/************************* Build_TAB() *************************
**
**  Description:	It builds TAB.
**  Inputs:		data, length, vol_num
**  Outputs:		length, out_buf
**  Returns:		offset of the TAB
**  Globals:		No
**  Notes:
**
*/

int Build_TAB(tdaru_prod_t * data, unsigned int * length, 
              unsigned char * out_buf, int vol_num)
{
  short END_OF_PAGE = -1;
  int n_lines = LINES_PER_PAGE;
  int n_pages = 0;
  int tab_hdr1_pos, tab_hdr2_pos;
  int total_pages;
  int i;

  int year, month, day, hour, minute, sec, milsec;
  unsigned int tab_len, temp;
  TAB_hdr1_t tab_hdr1;
  TAB_hdr2_t tab_hdr2;
  Graphic_product gp;
 
    /* if no feature, return */
  if(data->hdr.num_features <= 0)
    return 0;

    /* get the total pages */
  total_pages = (data->hdr.num_features - 1) / NUM_FEAT_LINES + 1;

    /* convert julian date and time to display format */
  RPGCS_julian_to_date(data->hdr.vol_date,
                       &year, &month, &day);
  RPGCS_convert_radial_time((data->hdr.vol_time * 1000),   /* convert to millisec */
                            &hour, &minute, &sec, &milsec);

  #ifdef DEBUG
    fprintf(stderr, "\n>Build_TAB(): ");
    fprintf(stderr, "\n>hour = %d, minute = %d, second = %d\n",
            hour, minute, sec);
  #endif

    /* save the start position for tab_hdr1 */
  tab_hdr1_pos = *length;

    /* Get tab_hdr1 info. Update block length later */
  tab_hdr1.blk_divider = -1;
  tab_hdr1.blk_id = 3;

    /* make room for tab_hdr1 and msg hdr as well as pdb */
  *length = *length + sizeof(TAB_hdr1_t) + sizeof(Graphic_product);

    /* start position for tab_hdr2 */
  tab_hdr2_pos = *length;

    /* Get tab_hdr2 info. */
  tab_hdr2.blk_divider = -1;
  tab_hdr2.num_pages = total_pages;

  memcpy((out_buf + tab_hdr2_pos), &tab_hdr2, sizeof(tab_hdr2));
  *length += sizeof(tab_hdr2);

    /* loop for all features */
  for(i = 0; i < data->hdr.num_features; i++)
  {
      /* write page header */
    if(n_lines == LINES_PER_PAGE)
    {
      n_lines = Write_page_hdr(data, out_buf, year, month, day,
                               hour, minute, sec, length, n_pages);

      n_pages++;
    }

    n_lines += Write_feat_lines(data->tda_feat[i], out_buf, length);
 
  } /* end of for(i = 0; i < data->hdr.num_features; i++) */ 
  
    /* Put END_OF_PAGE for this last page */
  memcpy((out_buf + (*length)), &END_OF_PAGE, sizeof(short));
  *length += sizeof(short);  

    /* update the tab block length */
  temp = (int)((*length) - tab_hdr1_pos);
  RPGC_set_product_int( (void *) &tab_hdr1.blk_length, temp );

  #ifdef DEBUG
    fprintf(stderr, "\n>>>>> tab_hdr1.blk_length -> %d\n", tab_hdr1.blk_length);
  #endif

    /* put tab_hdr1 into out_buf */
  memcpy((out_buf + tab_hdr1_pos), &tab_hdr1, sizeof(tab_hdr1));

    /* copy message hdr block and PDB into local version */
  memcpy(&gp, out_buf, sizeof(Graphic_product));

    /* fill in product description block */
  RPGC_prod_desc_block((void *)&gp, TRU, vol_num);
  RPGC_get_product_int( (void *) &tab_hdr1.blk_length, &temp );
  temp -= sizeof(TAB_hdr1_t);
  RPGC_set_product_int( (void *) &gp.msg_len, temp );
  gp.src_id = Site_adapt.rpg_id;
  gp.param_4 = data->hdr.num_tvs;
  gp.param_5 = data->hdr.num_etvs;
  gp.param_3 = (short)(data->hdr.current_elev * 10);
 
    /* put the halfword offset for this block in PDB */
  RPGC_set_product_int( (void *) &gp.sym_off, sizeof(Graphic_product)/2 );
  RPGC_set_product_int( (void *) &gp.gra_off, 0 );
  RPGC_set_product_int( (void *) &gp.tab_off, 0 );

    /*
    **  set message hdr info. Use local length
    */
  RPGC_get_product_int( (void *) &tab_hdr1.blk_length, &temp );
  tab_len = temp - sizeof(tab_hdr1) - sizeof(Graphic_product);
  RPGC_prod_hdr((void *)&gp, TRU, &tab_len);

    /* copy local version of product header into output buffer */
  memcpy((out_buf + tab_hdr1_pos + sizeof(TAB_hdr1_t)), &gp,
         sizeof(Graphic_product)); 

    /* return the TAB offset in half-word (2 bytes) */
  return (tab_hdr1_pos/2);
 
}  /* end of Build_TAB() */

/********************** Write_page_hdr() ************************
**
**  Description:	Writes TAB page header.
**  Inputs:		data, year, month, day, hour, minute, second,
**			length, n_pages
**  Outputs:		length, out_buf
**  Returns:		constant number of header lines
**  Globals:		No
**  Notes:
**
*/

int Write_page_hdr(tdaru_prod_t * data, unsigned char * out_buf,
                   int year, int month, int day, int hour, int minute,
                   int second, int * length, int n_pages)
{
  char title[] = "TVS Rapid Update";
  char prefix[33] = "                                ";  /* 32 spaces */
  char postfix[33] = "                                "; /* 32 spaces */
  char mid_pad[17] = "                ";   /* 16 spaces */
  short END_OF_PAGE = -1;
  int i; 
  TAB_line_t line;
  char num_tvs[4], num_etvs[4];
 
  line.num_chars = LINE_WIDTH;   /* fixed line width 80 chars */

    /* put end of page mark for previous page if needed */
  if(n_pages != 0)
  {
    memcpy((out_buf + (*length)), &END_OF_PAGE, sizeof(short));
    *length += sizeof(short);
  }

    /* header line 1: title line */
  sprintf(line.data, "%s%s%s", prefix, title, postfix);

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t));
  *length += sizeof(TAB_line_t);

  for(i = 0; i < LINE_WIDTH; i++)
    line.data[i] = SPACE;

  #ifdef DEBUG
    fprintf(stderr, "\n>Write_page_hdr(): ");
    fprintf(stderr, "\n>hour = %d, minute = %d, second = %d\n",
            hour, minute, second);
  #endif

    /* header line 2: */

  if(Max_num_tvs_exceeded)
    sprintf(num_tvs, ">%2d", Max_num_tvs);
  else
    sprintf(num_tvs, " %2d", data->hdr.num_tvs);

  if(Max_num_etvs_exceeded)
    sprintf(num_etvs, ">%2d", Max_num_etvs);
  else
    sprintf(num_etvs, " %2d", data->hdr.num_etvs);

  sprintf(line.data, 
  "  RADAR ID: %03d  DATE: %02d/%02d/%04d  TIME: %02d:%02d:%02d  TVS/ETVS: %s/%s  ELEV: %4.1f",
      Site_adapt.rpg_id, month, day, year, hour, minute, second, num_tvs,
      num_etvs, data->hdr.current_elev);

  #ifdef DEBUG
    fprintf(stderr, "\n line = %s, len = %d\n", line.data, strlen(line.data));
  #endif 

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t)); 
  *length += sizeof(TAB_line_t);

    /* header line 3: empty line */
  sprintf(line.data, "%s%s%s", prefix, mid_pad, postfix);

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t));
  *length += sizeof(TAB_line_t); 
 
    /* header line 4: */
  sprintf(line.data, 
  "  FEATURE  STORM  AZ/RAN  AVGDV LLDV  MXDV/Hgt   Depth    Base/Top    MXSHR/Hgt ");

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t));
  *length += sizeof(TAB_line_t); 

    /* header line 5: */
  sprintf(line.data,
  " STA TYPE   ID   (deg,nm)  (kt) (kt)  (kt,kft)   (kft)     (kft)     (E-3/s,kft)");

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t));
  *length += sizeof(TAB_line_t); 

    /* header line 6: empty line */
  sprintf(line.data, "%s%s%s", prefix, mid_pad, postfix);

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t));
  *length += sizeof(TAB_line_t); 
   
    /* return the number of header lines */
  return NUM_HDR_LINES;

}  /* end of Write_page_hdr() */

/********************** Write_feat_lines() ***********************
**
**  Description:	Writes the data line in each page.
**  Inputs:		tda_feat, length
**  Outputs:		length, out_buf
**  Returns:		always return 1 (line)
**  Globals:		No
**  Notes:
**
*/

int Write_feat_lines(tdaru_feat_t tda_feat, unsigned char * out_buf,
                     int * length) 
{
  TAB_line_t line;
  char feat_status[4];
  char type[5], base_hgt[6], top_hgt[6], depth[6];

  int az, rng, avgdv, lldv, mxdv, mxshr, i; 
  float mxdv_hgt, dph, b_hgt, t_hgt, mxshr_hgt;
 
  line.num_chars = LINE_WIDTH;   /* fixed line width 80 chars */

  az = (int)(tda_feat.feature.base_az + 0.5);
  rng = (int)(tda_feat.feature.base_rng * NM_PER_KM + 0.5);
  avgdv = (int)(tda_feat.feature.average_DV * M_PER_SEC_TO_KNOT + 0.5);
  lldv = (int)(tda_feat.feature.low_level_DV * M_PER_SEC_TO_KNOT + 0.5);
  mxdv = (int)(tda_feat.feature.max_DV * M_PER_SEC_TO_KNOT + 0.5);
  mxshr = (int)(tda_feat.feature.max_shear + 0.5);

  mxdv_hgt = tda_feat.feature.max_DV_height * KFT_PER_KM;
  dph = tda_feat.feature.depth * KFT_PER_KM;
  b_hgt = tda_feat.feature.base_height * KFT_PER_KM;
  t_hgt = tda_feat.feature.top_height * KFT_PER_KM;
  mxshr_hgt = tda_feat.feature.max_shear_height * KFT_PER_KM; 
   

  if(dph < 0)
    sprintf(depth, ">%4.1f", (dph * (-1)));
  else
    sprintf(depth, " %4.1f", dph);
  
  if(b_hgt < 0)
    sprintf(base_hgt, "<%4.1f", ((-1) * b_hgt));
  else 
    sprintf(base_hgt, " %4.1f", b_hgt);

  if(t_hgt < 0)
    sprintf(top_hgt, ">%4.1f", ((-1) * t_hgt));
  else
    sprintf(top_hgt, " %4.1f", t_hgt);
 
  if(tda_feat.feat_status == INC)
    sprintf(feat_status, "INC");
  else if(tda_feat.feat_status == NEW)
    sprintf(feat_status, "NEW");
  else if(tda_feat.feat_status == PER)
    sprintf(feat_status, "PER");
  else if(tda_feat.feat_status == EXT)
    sprintf(feat_status, "EXT");

  if(tda_feat.feature.feature_type == TVS)
    sprintf(type, " TVS");
  else
    sprintf(type, "ETVS");

  #ifdef DEBUG
    fprintf(stderr, ">\nlen_depth = %d, len_base_hgt = %d, len_top_hgt = %d, len_type = %d\n", 
            strlen(depth), strlen(base_hgt), strlen(top_hgt), strlen(type));
  #endif
  
    /* initialize the line buffer */
  for(i = 0; i < LINE_WIDTH; i++)
    line.data[i] = SPACE; 
  
  sprintf(line.data,  
  " %s %s%c  %s   %3d/%3d%c  %3d%c %3d%c %3d/%4.1f%c  %s%c  %s/%s%c  %3d/%4.1f%c ",
  feat_status, type, tda_feat.attr_flag.type_flg, tda_feat.storm_id, 
  az, rng, tda_feat.attr_flag.az_flg, avgdv, tda_feat.attr_flag.avg_DV_flg,
  lldv, tda_feat.attr_flag.ll_DV_flg, mxdv, mxdv_hgt, tda_feat.attr_flag.max_DV_flg, 
  depth, tda_feat.attr_flag.depth_flg, base_hgt, top_hgt, tda_feat.attr_flag.base_hgt_flg,
  mxshr, mxshr_hgt, tda_feat.attr_flag.max_shear_flg);

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> %s, length = %d\n", line.data, strlen(line.data));
  #endif 

  memcpy((out_buf + (*length)), &line, sizeof(TAB_line_t));
  *length += sizeof(TAB_line_t);
/*
  memcpy((out_buf + (*length)), &line.num_chars, sizeof(short));
  *length += sizeof(short);

  memcpy((out_buf + (*length)), &line.data, sizeof(line.data));
  *length += sizeof(line.data);
*/  
  return 1;
}  /* end of Write_feat_lines() */

/************************* Save_scit() **************************
**
**  Description:	It saves previous volume SCIT info.
**  Inputs:		scit_input
**  Outputs:		No
**  Returns:		No
**  Globals:		SCIT_track, Correlation_speed, Max_vtime_interval,
**			SCIT_saved 
**  Notes:
**
*/

void Save_scit(scit_track_t * scit_input)
{
  int i;

    /* save the header */
  SCIT_track.hdr = scit_input->hdr;

    /* save the IDs, motion, etc. */
  for(i = 0; i < NSTF_MAX; i++)
  {
    SCIT_track.bsi[i] = scit_input->bsi[i];
    SCIT_track.bsm[i] = scit_input->bsm[i];
    SCIT_track.bsf[i] = scit_input->bsf[i];
    SCIT_track.bsb[i] = scit_input->bsb[i];
  }

    /* save adaptation data */
  SCIT_track.bfa = scit_input->bfa;

    /*
    ** Extract the correlation speed for determining
    ** the search radius. (The array comes from fortran
    ** code. So the index needs to be converted to c-style
    ** index)
    */
  Correlation_speed = SCIT_track.bfa.adp_f[STA_COSP];

    /*
    ** Extract the max time between volume for later use. 
    ** (convert to seconds) (The array comes from fortran
    ** code. So the index needs to be converted to c-style
    ** index)
    */
  Max_vtime_interval = SCIT_track.bfa.adp_i[STA_MAXT] * 
                       SECONDS_PER_MINUTE;  

    /* set the flag */
  SCIT_saved = TRUE;
 
}  /* end of Save_scit() */

/************************* Save_mda() **************************
**
**  Description:	It saves previous volume MDA trend info. 
**  Inputs:		mda_input
**  Outputs:		No
**  Returns:		No
**  Globals:		MDA_track, MDA_saved
**  Notes:
**
*/

void Save_mda(mda_track_t * mda_input)
{
  MDA_track.x_spd = mda_input->x_spd;
  MDA_track.y_spd = mda_input->y_spd;

    /* set the flag */
  MDA_saved = TRUE;

  #ifdef DEBUG
    fprintf(stderr, "\n>>>> Save_mda(): x speed -> %3.1f m/s, y speed -> %3.1f m/s\n",
            MDA_track.x_spd, MDA_track.y_spd);
  #endif

}  /* end of Save_mda() */

/************************* Print_tvs_info() **********************
**
**  Description:	It prints out TVS input info.
**  Inputs:		tvs_input
**  Outputs:		No
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

#ifdef DEBUG

void Print_tvs_info(tvs_feat_t tvs_input)
{
  int i;

  fprintf(stderr, "\n************ TVS Input *************\n");
  fprintf(stderr, "\n\tVolume Time ---------> %d", tvs_input.hdr.vol_time);
  fprintf(stderr, "\n\tVolume Date ---------> %d", tvs_input.hdr.vol_date);
  fprintf(stderr, "\n\tNumber of TVSs ------> %d", tvs_input.hdr.num_tvs);
  fprintf(stderr, "\n\tNumber of ETVSs -----> %d\n", tvs_input.hdr.num_etvs);

  for(i = 0; i < (tvs_input.hdr.num_tvs + tvs_input.hdr.num_etvs); i++)
  {
    fprintf(stderr, "\n\tFeat Type -----------> %3.0f", tvs_input.features[i].feature_type);
    fprintf(stderr, "\n\tBase AZ -------------> %3.1f deg", tvs_input.features[i].base_az);
    fprintf(stderr, "\n\tBase Range ----------> %3.1f km", tvs_input.features[i].base_rng);
    fprintf(stderr, "\n\tLow Level DV --------> %3.1f m/s", tvs_input.features[i].low_level_DV);
    fprintf(stderr, "\n\tAverage DV ----------> %3.1f m/s", tvs_input.features[i].average_DV);
    fprintf(stderr, "\n\tMax DV --------------> %3.1f m/s", tvs_input.features[i].max_DV);
    fprintf(stderr, "\n\tMax DV Hgt ----------> %3.1f km", tvs_input.features[i].max_DV_height);
    fprintf(stderr, "\n\tDepth ---------------> %3.1f km", tvs_input.features[i].depth);
    fprintf(stderr, "\n\tBase Hgt ------------> %3.1f km", tvs_input.features[i].base_height);
    fprintf(stderr, "\n\tBase Elev -----------> %3.1f deg", tvs_input.features[i].base_elev);
    fprintf(stderr, "\n\tTop Hgt -------------> %3.1f km", tvs_input.features[i].top_height);
    fprintf(stderr, "\n\tMax Shear -----------> %3.1f 0.001/s", tvs_input.features[i].max_shear);
    fprintf(stderr, "\n\tMax Shr Hgt ---------> %3.1f km\n", tvs_input.features[i].max_shear_height);
  }

  fprintf(stderr, "\n\tDV1 -----------------> %3.1f m/s", tvs_input.tda_adapt.DV1);
  fprintf(stderr, "\n\tDV2 -----------------> %3.1f m/s", tvs_input.tda_adapt.DV2);
  fprintf(stderr, "\n\tDV3 -----------------> %3.1f m/s", tvs_input.tda_adapt.DV3);
  fprintf(stderr, "\n\tDV4 -----------------> %3.1f m/s", tvs_input.tda_adapt.DV4);
  fprintf(stderr, "\n\tDV5 -----------------> %3.1f m/s", tvs_input.tda_adapt.DV5);
  fprintf(stderr, "\n\tDV6 -----------------> %3.1f m/s", tvs_input.tda_adapt.DV6);
  fprintf(stderr, "\n\tVector Vel Diff -----> %3.1f m/s", tvs_input.tda_adapt.vector_vel_diff);
  fprintf(stderr, "\n\tFeat 2D Ratio -------> %3.1f km/km", tvs_input.tda_adapt.feat2d_ratio);
  fprintf(stderr, "\n\tMax Vector Rng ------> %3.1f km", tvs_input.tda_adapt.max_vector_rng);
  fprintf(stderr, "\n\tCircle Radius1 ------> %3.1f km", tvs_input.tda_adapt.circ_radius1);
  fprintf(stderr, "\n\tCircle Radius2 ------> %3.1f km", tvs_input.tda_adapt.circ_radius2);
  fprintf(stderr, "\n\tCircle Radius Rng ---> %3.1f km", tvs_input.tda_adapt.circ_radius_rng);
  fprintf(stderr, "\n\tMin TVS Base Hgt ----> %3.1f km", tvs_input.tda_adapt.min_tvs_base_hgt);
  fprintf(stderr, "\n\tMin TVS Base Elev ---> %3.1f deg", tvs_input.tda_adapt.min_tvs_base_elev);
  fprintf(stderr, "\n\tMax Vector Hgt ------> %3.1f km", tvs_input.tda_adapt.max_vector_height);
  fprintf(stderr, "\n\tMin Number Vectors --> %3.1f", tvs_input.tda_adapt.min_num_vectors);
  fprintf(stderr, "\n\tMin Depth -----------> %3.1f km", tvs_input.tda_adapt.min_depth);
  fprintf(stderr, "\n\tMin 3D DV -----------> %3.1f m/s", tvs_input.tda_adapt.min_3d_DV);
  fprintf(stderr, "\n\tMin TVS DV ----------> %3.1f m/s", tvs_input.tda_adapt.min_tvs_DV);
  fprintf(stderr, "\n\t2D Vec Radial Dist --> %3.1f km", tvs_input.tda_adapt.vec_rad_dist_2d);
  fprintf(stderr, "\n\t2D Vec AZ Dist ------> %3.1f deg", tvs_input.tda_adapt.vec_az_dist_2d);
  fprintf(stderr, "\n\tMin Refl ------------> %3.1f dBZ", tvs_input.tda_adapt.min_refl);
  fprintf(stderr, "\n\tMax SAD -------------> %3.1f km", tvs_input.tda_adapt.max_SAD);
  fprintf(stderr, "\n\tMin Num 2D Feat -----> %3.1f", tvs_input.tda_adapt.min_num_2dfeat);
  fprintf(stderr, "\n\tMax Num 2D Feat -----> %3.1f", tvs_input.tda_adapt.max_num_2dfeat);
  fprintf(stderr, "\n\tMax Num 3D Feat -----> %3.1f", tvs_input.tda_adapt.max_num_3dfeat);
  fprintf(stderr, "\n\tMax Num Vectors -----> %3.1f", tvs_input.tda_adapt.max_num_vectors);
  fprintf(stderr, "\n\tMax Num ETVSs -------> %3.1f", tvs_input.tda_adapt.max_num_etvs);
  fprintf(stderr, "\n\tMax Num TVSs --------> %3.1f", tvs_input.tda_adapt.max_num_tvs);
  fprintf(stderr, "\n\tAvg DV Hgt ----------> %3.1f km", tvs_input.tda_adapt.average_DV_height);
  
}  /* end of Print_tvs_info() */

#endif

/********************** Print_tvsru_info() ***********************
**
**  Description:	It prints out output data array info.
**  Inputs:		data
**  Outputs:		No
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

#ifdef DEBUG

void Print_tvsru_info(tdaru_prod_t data)
{
  int i;

  fprintf(stderr, "\n*************** Output Data Array *************\n");
  fprintf(stderr, "\n\tVolume Time --------------> %d sec", data.hdr.vol_time);
  fprintf(stderr, "\n\tVolume Date --------------> %d day", data.hdr.vol_date);
  fprintf(stderr, "\n\tElevation ----------------> %2.1f deg", data.hdr.current_elev);
  fprintf(stderr, "\n\tNumber of TVSs -----------> %d", data.hdr.num_tvs);
  fprintf(stderr, "\n\tNumber of ETVSs ----------> %d", data.hdr.num_etvs);
  fprintf(stderr, "\n\tNumber of Feats ----------> %d\n", data.hdr.num_features);

  for(i = 0; i < data.hdr.num_features; i++)
  {
    fprintf(stderr, "\n\tStorm ID -----------------> %s", data.tda_feat[i].storm_id);

    if(data.tda_feat[i].feat_status == NEW)
      fprintf(stderr, "\n\tFeat Status --------------> NEW");
    else if(data.tda_feat[i].feat_status == INC)
      fprintf(stderr, "\n\tFeat Status --------------> INC");
    else if(data.tda_feat[i].feat_status == PER)
      fprintf(stderr, "\n\tFeat Status --------------> PER");
    else if(data.tda_feat[i].feat_status == EXT)
      fprintf(stderr, "\n\tFeat Status --------------> EXT");
    else
      fprintf(stderr, "\n\tFeat Status --------------> EMPTY");

    if(data.tda_feat[i].matched_flag)
      fprintf(stderr, "\n\tMatch Status -------------> MATCHED");
    else
      fprintf(stderr, "\n\tMatch Status -------------> NO MATCH");

    if(((int)data.tda_feat[i].feature.feature_type) == TVS)
    fprintf(stderr, "\n\tFeat Type ----------------> TVS %c", 
                                      data.tda_feat[i].attr_flag.type_flg);
    else
    fprintf(stderr, "\n\tFeat Type ----------------> ETVS %c", 
                                      data.tda_feat[i].attr_flag.type_flg);

    fprintf(stderr, "\n\tBase AZ ------------------> %3.1f deg %c", 
                                      data.tda_feat[i].feature.base_az,
                                      data.tda_feat[i].attr_flag.az_flg);
    fprintf(stderr, "\n\tBase Range ---------------> %3.1f km %c", 
                                      data.tda_feat[i].feature.base_rng,
                                      data.tda_feat[i].attr_flag.rng_flg);
    fprintf(stderr, "\n\tLow Level DV -------------> %3.1f m/s %c", 
                                      data.tda_feat[i].feature.low_level_DV,
                                      data.tda_feat[i].attr_flag.ll_DV_flg);
    fprintf(stderr, "\n\tAverage DV ---------------> %3.1f m/s %c", 
                                      data.tda_feat[i].feature.average_DV,
                                      data.tda_feat[i].attr_flag.avg_DV_flg);
    fprintf(stderr, "\n\tMax DV -------------------> %3.1f m/s %c", 
                                      data.tda_feat[i].feature.max_DV,
                                      data.tda_feat[i].attr_flag.max_DV_flg);
    fprintf(stderr, "\n\tMax DV Hgt ---------------> %3.1f km %c", 
                                      data.tda_feat[i].feature.max_DV_height,
                                      data.tda_feat[i].attr_flag.max_DV_hgt_flg);
    fprintf(stderr, "\n\tDepth --------------------> %3.1f km %c", 
                                      data.tda_feat[i].feature.depth,
                                      data.tda_feat[i].attr_flag.depth_flg);
    fprintf(stderr, "\n\tBase Hgt -----------------> %3.1f km %c", 
                                      data.tda_feat[i].feature.base_height,
                                      data.tda_feat[i].attr_flag.base_hgt_flg);
    fprintf(stderr, "\n\tBase Elev ----------------> %3.1f deg %c", 
                                      data.tda_feat[i].feature.base_elev,
                                      data.tda_feat[i].attr_flag.base_elev_flg);
    fprintf(stderr, "\n\tTop Hgt ------------------> %3.1f km %c", 
                                      data.tda_feat[i].feature.top_height,
                                      data.tda_feat[i].attr_flag.top_hgt_flg);
    fprintf(stderr, "\n\tMax Shear ----------------> %3.1f 0.001/s %c", 
                                      data.tda_feat[i].feature.max_shear,
                                      data.tda_feat[i].attr_flag.max_shear_flg);
    fprintf(stderr, "\n\tMax Shr Hgt --------------> %3.1f km %c\n", 
                                      data.tda_feat[i].feature.max_shear_height,
                                      data.tda_feat[i].attr_flag.max_shear_hgt_flg);
  }

}  /* end of Print_tvsru_info() */

#endif

/************************* End of tdaru.c *****************************/

