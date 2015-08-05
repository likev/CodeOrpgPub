/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 18:56:13 $
 * $Id: rpg_scan_sum.c,v 1.19 2013/07/22 18:56:13 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */ 

/********************************************************************

	This module contains scan Summary update functions for the 
	old RPG product generaton tasks.

********************************************************************/

 

#include <rpg.h>
#include <math.h>

/* Scan Summary Definitions. */
static time_t Last_ss_update_time = -1;
static Summary_Data *Summary = NULL; /* pointer to the scan Summary */
static int Input_stream = PGM_REALTIME_STREAM;

/* Volume Status Definitions. */
static Vol_stat_gsm_t *Vol_stat = NULL;
static time_t Last_vs_update_time = -1;

/* Public Functions. */

/*********************************************************************

    Description:
        This public interface function sets the scan Summary array by
        reading the scan summary LB.

    Return:
        There is no return value defined for this function.

*********************************************************************/
void RPG_read_scan_summary (){

   SS_read_scan_summary();
   return;

/* End of RPG_read_scan_summary() */
}

/***********************************************************************

   Description:
      This functions returns the last update time for the scan summary
      data.  Time is UNIX time.

***********************************************************************/
time_t RPG_scan_summary_last_updated(){

   return( Last_ss_update_time );

/* End of RPG_scan_summary_last_updated() */
}

/*********************************************************************

    Description:
        This public interface function sets the Volume Status data by
        reading the Volume Status LB.

    Return:
        There is no return value defined for this function.

*********************************************************************/
void RPG_read_volume_status (){

   VS_read_volume_status();
   return;

/* End of RPG_read_volume_status() */
}

/***********************************************************************

   Description:
      This functions returns the last update time for the volume status
      data.  Time is UNIX time.

***********************************************************************/
time_t RPG_volume_status_last_updated(){

   return( Last_vs_update_time );

/* End of RPG_volume_status_last_updated() */
}

/* Private Functions. */

/*********************************************************************

   Description:
      Initialization routine for the SS module.

   Inputs:

   Outputs:

   Returns:

   Notes:

*********************************************************************/
void SS_initialize(){

   /* Set the tasks input stream. */
   Input_stream = INIT_get_task_input_stream();

   /* Read the Scan Summary Data. */
   SS_read_scan_summary();

/* End of SS_initialize() */
}

/*********************************************************************

   Description:
      Initialization routine for the VS module.

   Inputs:

   Outputs:

   Returns:

   Notes:

*********************************************************************/
void VS_initialize(){

   /* Set the tasks input stream. */
   Input_stream = INIT_get_task_input_stream();

   /* Read the Volume Status Data. */
   VS_read_volume_status();

/* End of VS_initialize() */
}

/*********************************************************************
	
    Description: 
        This function sets the scan Summary array by reading the 
        scan summary LB.  It reads the LB every start of volume,
        start of elevation and the first time the function is called.

        This is the Base Data Header version.

    Input:
 	bd_hd - the basedata header

    Return:
        There is no return value defined for this function.

    Note:	
        This is called by a C function

*********************************************************************/
void SS_update_summary ( Base_data_header *bd_hd ){

    int ret;

    static int initialized = 0;

    /* Retrieve the volume Summary information.  Note:  If Scan Summary 
       array has not been registered, then Summary should always be NULL. */
    if( (Summary != NULL) && (bd_hd != NULL) ){

	/* Read the scan summary table from LB. */
	if( (bd_hd->status == GOODBVOL)
                           ||
            (bd_hd->status == GOODBEL)
                           ||
	              !initialized){

	    ret = ORPGSUM_read_summary_data( Summary );
            if( ret >= 0 ){

               /* For non-realtime stream, want to update scan summary
                  data everytime called. */
               if( Input_stream == PGM_REALTIME_STREAM ) 
	          initialized = 1;

               Last_ss_update_time = time( NULL );

            }
            else
               LE_send_msg( GL_INFO, "ORPGSUM_read_summary_data Failed (%d)\n", ret );

	}
        else
           LE_send_msg( GL_ERROR, "SS_update_summary(): Status !GOODBVOL and !GOODBEL\n" );

    }

    return;

/* End of SS_update_summary() */
}

/*********************************************************************

    Description:
        This function sets the Volume Status data by reading the
        Volume Status LB.  It reads the LB every start of volume,
        start of elevation and the first time the function is called.

        This is the Base Data Header version.

    Input:
        bd_hd - the basedata header

    Return:
        There is no return value defined for this function.

    Note:
        This is called by a C function

*********************************************************************/
void VS_update_volume_status ( Base_data_header *bd_hd ){

    static int initialized = 0;

    /* Retrieve the volume status information. */
    if (Vol_stat != NULL) {

        /* Read the volume status data from LB. */
        if( (bd_hd->status == GOODBVOL)
                           ||
            (bd_hd->status == GOODBEL)
                           ||
                      !initialized){

            if( ORPGVST_read( (char *) Vol_stat ) != NULL ){

               /* For non-realtime stream, want to update volume status
                  data everytime called. */
               if( Input_stream == PGM_REALTIME_STREAM )
                  initialized = 1;

               Last_vs_update_time = time( NULL );

            }

        }

    }

    return;

/* End of VS_update_volume_status() */
}

/*********************************************************************
        
    Description: 
        This function sets the scan Summary array by reading the scan 
        summary LB.

    Return:
        There is no return value defined for this function.

*********************************************************************/
void SS_read_scan_summary (){

   int ret;

   /* Retrieve the Scan Summary information. */
   if (Summary != NULL) {

      /* Read the scan summary data. */
      ret = ORPGSUM_read_summary_data( Summary );

      if( ret >= 0 )
         Last_ss_update_time = time( NULL );

      else
         LE_send_msg( GL_INFO, "ORPGSUM_read_summary_data Failed (%d)\n", ret );

   }

   return;

/* End of SS_read_scan_summary() */
}

/*********************************************************************

    Description:
        This function sets the Volume Status data by reading the Volume
        Status LB.

    Return:
        There is no return value defined for this function.

*********************************************************************/
void VS_read_volume_status (){

   /* Retrieve the Volume Status information. */
   if (Vol_stat != NULL) {

      /* Read the Volume Status data. */
      if( ORPGVST_read( (char *) Vol_stat ) != NULL )
         Last_vs_update_time = time( NULL );

   }

   return;

/* End of VS_read_volume_status() */
}

/***********************************************************************

    Description: 
        This function is called to pass the scan Summary array,
	which is stored for later scan Summary updating.

    Input:	
        Summary - pointer to the scan Summary array

***********************************************************************/
int SS_send_summary_array (int *summary){

    if( Summary == NULL )
       Summary = (Summary_Data *)summary;

    return (0);

/* End of SS_send_summary_array() */
}

/***********************************************************************

    Description:
        This function is called to pass the Volume Status common block
        address which is stored for later Volume Status updating.

    Input:
        vol_status - pointer to the first element of Volume Status 
                     data.

***********************************************************************/
int VS_send_volume_status ( char *vol_status){

    if( Vol_stat == NULL )
       Vol_stat = (Vol_stat_gsm_t *) vol_status;

    return (0);

/* End of VS_send_volume_status() */
}

/***********************************************************************

    Description: 
        This function return pointer to Scan Summary data.

***********************************************************************/
void* SS_get_summary_data(){

   return ( (void*) Summary );

/* End of SS_get_summary_array() */
}

/***********************************************************************

    Description:
        This function returns pointer to Volume Status data.

***********************************************************************/
void* VS_get_volume_status(){

   return ( (void*) Vol_stat );

/* End of VS_get_volume_status() */
}

/***********************************************************************
   Description:
      Given the volume scan number (1-80), returns the UNIX volume scan 
      start time in seconds.

      This functions also accepts volume sequence number as input.

   Inputs:
      vol_num - volume scan number (1-80) or volume scan sequence
                number.

   Outputs:

   Returns:
      The UNIX volume scan start time, or 0 if cannot determine the
      time.

   Notes:

***********************************************************************/
time_t SS_get_volume_time( unsigned int vol_num ){

   int vol_date, vol_time;

   /* If vol_num is not valid, return 0. */
   if( vol_num == 0xffffffff )
      return 0;

   /* If volume scan sequence number, make volume scan number. */
   vol_num = ORPGMISC_vol_scan_num( (unsigned int) vol_num );

   /* If vol_num not in range [0, MAX_SCAN_SUM_VOLS], return 0. */
   if( vol_num > MAX_SCAN_SUM_VOLS )
      return 0;

   /* Read the scan summary data. */
   RPG_read_scan_summary();
 
   /* Get the modified Julian date and seconds since midnight for
      the input volume number. */
   vol_date = Summary->scan_summary[ vol_num ].volume_start_date;
   vol_time = Summary->scan_summary[ vol_num ].volume_start_time;

   /* Validate the volume scan date and time. */
   if( vol_date <= 0 || vol_time <= 0 )
      return 0;

   /* Return the modified Julian time. */
   return( (vol_date-1)*86400 + vol_time );

/* End of SS_get_volume_time() */
}

/***********************************************************************
   Description:
      Given the volume scan number (1-80), returns the weather mode
      and vcp number for this volume scan.

      This functions also accepts volume sequence number as input.

   Inputs:
      vol_num - volume scan number (1-80) or volume scan sequence
                number.

   Outputs:
      vcp - volume coverage pattern number.

   Returns:
      The weather mode for this input volume scan number.  Defaults to
      weather mode 2 and vcp number 21. 

   Notes:

***********************************************************************/
int SS_get_wx_mode( unsigned int vol_num, int *vcp ){

   int vcp_num, wx_mode;

   /* If vol_num is not valid, return 0. */
   if( vol_num == 0xffffffff )
      return 0;

   /* If volume scan sequence number, make volume scan number. */
   vol_num = ORPGMISC_vol_scan_num( (unsigned int) vol_num );

   /* If vol_num not in range [0, MAX_SCAN_SUM_VOLS], return 0. */
   if( vol_num > MAX_SCAN_SUM_VOLS )
      return 0;

   /* Read the scan summary data. */
   RPG_read_scan_summary();
 
   /* Get the volume coverage pattern and weather mode. */
   wx_mode = Summary->scan_summary[ vol_num ].weather_mode;
   vcp_num = Summary->scan_summary[ vol_num ].vcp_number;

   /* Validate the weather mode.  Assumes the vcp number is correct if
      the weather mode is valid. */
   if( (wx_mode != MAINTENANCE_MODE) 
                &&
       (wx_mode != PRECIPITATION_MODE)
                &&
       (wx_mode != CLEAR_AIR_MODE) ){

      wx_mode = PRECIPITATION_MODE;
      vcp_num = 21;

   }

   /* Return the modified Julian time. */
   *vcp = vcp_num;
   return( wx_mode );

/* End of SS_get_volume_time() */
}

/********************************************************************
   Description:
      Returns the elevation angle*10 of the last elevation in the 
      VCP if AVSET is enabled.   Otherwise returns 0.

   Inputs:
      vol_num - current volume scan number, modulo 80.

********************************************************************/
void RPG_avset_last_elev( void *summary, short *last_elev ){

   Scan_Summary *summary_data = (Scan_Summary *) summary;

   /* Initialize the last elevation. */
   *last_elev = 0;

   /* If for some reason scan summary data can not be read or is otherwise
      unavailable, return 0. */
   if( summary_data == NULL ){

      LE_send_msg( GL_ERROR, "Unable To Read Scan Summary Data\n" );
      return;

   }

   /* Check if AVSET is enabled.  If enabled, return the elevation
      angle * 10.  Otherwise, return 0. */
   if( summary_data->avset_status == AVSET_ENABLED ){

      int last_rda_cut, vcp;
      float elev;

      last_rda_cut  = summary_data->last_rda_cut - 1;
      vcp = summary_data->vcp_number;

      elev = VI_get_elevation_angle( vcp, last_rda_cut );
      *last_elev = (int) roundf( elev*10.0f );

   }

} /* End of RPG_avset_last_elev(). */

/**********************************************************************

   Description:
      Reads radial accounting data for elevation start/end date/time.

   Inputs:
      flag - either RPGC_START_TIME, RPGC_END_TIME
      vol_num - volume scan number in range 1 - 80
      rda_elev_num - RDA elevation number

   Outputs:
      the_time - time of elevation start/end, in secs past midnight
      the_time - modified Julian date of elevation start/end.

   Returns:
      0 on success, -1 on failure.

   Notes:  If rda_elev_number is -1, then assume the elevation number
           for the last cut in a VCP.

***********************************************************************/
void RPGC_get_elev_time( int *flag, int *vol_num, int *rda_elev_num,
                         int *the_time, int *the_date ){

   int bytes_read;
   Radial_accounting_data *accdata = NULL;

   int flg = *flag, elev = *rda_elev_num, vol = *vol_num;

   *the_time = *the_date = 0;

   /* Flag can be either RPGC_START_TIME or RPGC_END_TIME. */
   if( (flg != RPG_START_TIME) && (flg != RPG_END_TIME) ){

      LE_send_msg( GL_INFO,
                   "Invalid flag value: %d to function RPG_get_elev_time\n",
                   flg );
      return;

   }

   /* Volume number should be in the range of 1 - 80. */
   if( (vol <= 0) || (vol > MAX_SCAN_SUM_VOLS) ){

      LE_send_msg( GL_INFO,
                   "Invalid volume number: %d to function RPG_get_elev_time\n",
                   vol );
      return;

   }

   /* Read the radial acccounting data. */
   bytes_read = ORPGDA_read( ORPGDAT_ACCDATA, (char *) &accdata, LB_ALLOC_BUF,
                             vol - 1 );

   if( bytes_read <= 0 ){

      LE_send_msg( GL_INFO,
                   "Error reading ORPGDAT_ACCDATA: %d\n", bytes_read );
      return;

   }

   /* Verify the rda_elev_num is within bounds of the accounting data. */
   if( elev > accdata->accnumel ){

      LE_send_msg( GL_INFO, "Invalid RDA Elev # Passed to RPG_get_elev_time: %d\n",
                   elev );

      free(accdata);
      return;

   }

   /* If the elev is -1, set elev to accdata->accnumel. */
   if( elev < 0 )
      elev = accdata->accnumel;

   /* Gather the requested date/time. */
   if( flg == RPG_START_TIME ){

      *the_date = accdata->acceldts[elev-1];
      *the_time = (int) round( accdata->acceltms[elev-1]/1000.0 );

   }
   else {

      *the_date = accdata->acceldte[elev-1];
      *the_time = (int) round( accdata->acceltme[elev-1]/1000.0 );

   }

   /* Check for "the_time" being rounded to 86400 ... if this happens, 
      set "the_time" to 0 and increment "the_date". */
   if( *the_time == 86400 ){

      LE_send_msg( GL_INFO, "Time has been rounded to %d.  Increment \"the_date\": %d\n",
                   *the_time, *the_date );

      *the_time = 0;
      (*the_date)++;

      LE_send_msg( GL_INFO, "-->New Date: %d, Time: %d\n", *the_date, *the_time );

   }

   /* Free accounting data. */
   free(accdata);

   /* Return normal. */
   return;

/* End of RPG_get_elev_time() */
}

