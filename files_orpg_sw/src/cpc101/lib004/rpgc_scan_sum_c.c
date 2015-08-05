/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 18:56:13 $
 * $Id: rpgc_scan_sum_c.c,v 1.15 2013/07/22 18:56:13 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <rpgcs.h>

/* Static Global. */
static Summary_Data *Summary = NULL;
static int Summary_registered = 0;
static time_t Last_ss_update_time = -1;
static int Input_stream = PGM_REALTIME_STREAM;
static In_data_type *Inp_list = NULL;
static int N_inps = 0;

/* Volume Status Definitions. */
static Vol_stat_gsm_t *Vol_stat = NULL;
static time_t Last_vs_update_time = -1;


/* Public Functions */

/**********************************************************************

   Description:
      This subroutine passes the scan summary array to the C
      processing routines.

      The scan summary table is initially read.

   Returns:
      Returns -1 on error, or 0 otherwise.

***********************************************************************/
int RPGC_reg_scan_summary(){

   SS_register();

   return( 0 );

/* End of RPGC_reg_scan_summary() */
}

/**********************************************************************

   Description:
      This module returns, via a pointer to a Scan_Summary structure, 
      scan summary data corresponding to volume scan number "vol_num".
      processing routines.

   Inputs:
      vol_num - volume scan number in range 0 - MAX_SCAN_SUM_VOLS.

   Returns:
      Returns NULL on error, or pointer to Scan_Summary structure for
      "vol_num" on success.

***********************************************************************/
Scan_Summary* RPGC_get_scan_summary( int vol_num ){

   /* Initially check if the scan summary data has been registered.
      If not, register here and do an initial read. */
   if( !Summary_registered )
      SS_register();

   if( !Summary_registered ){

      LE_send_msg( GL_INFO, "Registration for Scan Summary Failed\n" );
      return(NULL);

   }

   /* Verify "vol_num" is within legal bounds */
   if( (vol_num < 0) || (vol_num > MAX_SCAN_SUM_VOLS) )
      return( NULL );

   /* Check if any inputs registered.   If no inputs registered, read
      the summary data. */
   if( N_inps == 0 )
      SS_read_scan_summary();

   /* Return the address of the summary data for "vol_num" */
   return( (Scan_Summary *) &Summary->scan_summary[vol_num] );

/* End of RPGC_get_scan_summary() */
}

/**********************************************************************

   Description:
      This subroutine passes the Volume Status structure to the C
      processing routines.

      The Volume Status is initially read.

   Returns:
      Always returns 0. 

***********************************************************************/
int RPGC_reg_volume_status( Vol_stat_gsm_t *vol_stat ){

   /* Validate the passed argument. */
   if( vol_stat == NULL )
      PS_task_abort( "Invalid Argument to RPGC_reg_volume_status()\n" );

   /* Send the volume status. */
   VS_send_volume_status ((char *) vol_stat );

   /* Do initial read of the Volume Status. */
   VS_read_volume_status();

   /* Return to caller. */
   return( 0 );

/* End of RPGC_reg_volume_status () */
}

/**********************************************************************

   Description:
      Reads the Volume Status.

   Returns:
      Always returns 0. 

**********************************************************************/
int RPGC_read_volume_status(){

   /* Read the volume status. */
   VS_read_volume_status();

   return 0;

/* End of RPGC_read_volume_status() */
}

/**********************************************************************

   Description:
      This subroutine passes the scan summary array to the C
      processing routines.

      The scan summary table is initially read.

   Returns:
      Returns -1 on error, or 0 otherwise.

***********************************************************************/
int SS_register(){

   /* Check if already registered. */
   if( Summary_registered )
      return( 0 );

   /* Begin registration process .....

      Allocate memory for Summary array. */
   Summary = (Summary_Data *) calloc( 1, ALIGNED_SIZE(sizeof( Summary_Data )) );
   if( Summary == NULL )
      PS_task_abort( "calloc Failed in RPGC_reg_scan_summary" );

   /* Send the scan summary array to the SS module. */
   SS_send_summary_array ((int *) Summary);

   /* Do initial read of the summary data. */
   SS_read_scan_summary();

   /* Set flag that summary data is registered. */
   Summary_registered = 1;

   /* For now, always return 0. */
   return( 0 );

/* End of SS_register() */
}

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

   /* Get the number of inputs registered.  Used to decide
      if a forced read of scan summary data is needed (if no
      inputs, data needs to be read.) */
   N_inps = IB_inp_list( &Inp_list );

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
   SS_read_scan_summary();

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
   SS_read_scan_summary();

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

   Returns:
      If AVSET is enabled, returns elevation angle * 10.
      Otherwise, returns 0.  

********************************************************************/
int RPGC_avset_last_elev( int vol_num ){

   Scan_Summary *summary = NULL;
   int nearest_int;

   /* If for some reason scan summary data can not be read or is otherwise
      unavailable, return 0. */
   if( (summary = RPGC_get_scan_summary( vol_num )) == NULL ){

      LE_send_msg( GL_ERROR, "Unable To Read Scan Summary Data\n" );
      return 0;

   }

   /* Check if AVSET is enabled.  If enabled, return the elevation
      angle * 10.  Otherwise, return 0. */
   if( summary->avset_status == AVSET_ENABLED ){

      int last_rda_cut, vcp;
      float elev;

      last_rda_cut  = summary->last_rda_cut - 1;
      vcp = summary->vcp_number;

      elev = RPGCS_get_elevation_angle( vcp, last_rda_cut );
      nearest_int = (int) roundf( elev*10.0f );
      return( nearest_int );

   }

   return 0;

} /* End of RPGC_avset_last_elev(). */


/**********************************************************************

   Description:
      Reads radial accounting data for elevation start/end date/time.

   Inputs:
      flag - either RPGC_START_TIME, RPGC_END_TIME
      vol_num - volume scan number in range 1 - 80
      rda_elev_num - RDA elevation number

   Outputs:
      the_time - time of elevation start/end, in secs past midnight
      the_date - modified Julian date of elevation start/end.

   Returns:
      0 on success, -1 on failure.

   Notes:
      If rda_elev_num < 0, assume last elevation in volume.

***********************************************************************/
int RPGC_get_elev_time( int flag, int vol_num, int rda_elev_num,
                        int *the_time, int *the_date ){

   int bytes_read;
   Radial_accounting_data *accdata = NULL; 

   *the_time = *the_date = 0;

   /* Flag can be either RPGC_START_TIME or RPGC_END_TIME. */
   if( (flag != RPGC_START_TIME) && (flag != RPGC_END_TIME) ){

      LE_send_msg( GL_INFO, 
                   "Invalid flag value: %d to function RPGC_get_elev_time\n",
                   flag );
      return -1;

   }

   /* Volume number should be in the range of 1 - 80. */
   if( (vol_num <= 0) || (vol_num > MAX_SCAN_SUM_VOLS) ){

      LE_send_msg( GL_INFO, 
                   "Invalid volume number: %d to function RPGC_get_elev_time\n",
                   vol_num );
      return -1;

   }

   /* Read the radial acccounting data. */
   bytes_read = ORPGDA_read( ORPGDAT_ACCDATA, (char *) &accdata, LB_ALLOC_BUF, 
                             vol_num - 1 );

   if( bytes_read <= 0 ){

      LE_send_msg( GL_INFO, 
                   "Error reading ORPGDAT_ACCDATA: %d\n", bytes_read );
      return -1;

   }

   /* Verify the rda_elev_num is within bounds of the accounting data. */
   if( (rda_elev_num > accdata->accnumel) 
                    ||
             (rda_elev_num < 0) )
      rda_elev_num = accdata->accnumel;

   /* Gather the requested date/time. */
   if( flag == RPGC_START_TIME ){

      *the_date = accdata->acceldts[rda_elev_num-1];
      *the_time = (int) round( (double) accdata->acceltms[rda_elev_num-1]/1000.0 );

   }
   else {

      *the_date = accdata->acceldte[rda_elev_num-1];
      *the_time = (int) round( (double) accdata->acceltme[rda_elev_num-1]/1000.0 );

   }

   /* Check for "the_time" being rounded to 86400 ... if this happens, 
      set "the_time" to 0 and increment "the_date". */
   if( *the_time == SECS_IN_DAY ){

      LE_send_msg( GL_INFO, "Time has been rounded to %d.  Increment \"the_date\": %d\n",
                   *the_time, *the_date );
      
      *the_time = 0;
      (*the_date)++;

      LE_send_msg( GL_INFO, "-->New Date: %d, Time: %d\n", *the_date, *the_time );

   }

   /* Free accounting data. */
   free(accdata);

   /* Return normal. */
   return 0;

/* End of RPGC_get_elev_time() */
}
