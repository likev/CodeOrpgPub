/************************************************************************
 *									*
 *	Module:	hci_register_callbacks.c				*
 *									*
 *	Description:	This function initializes/registers data stores	*
 *			and events needed by the RPG Control/Status	*
 *			task that are not initialized/registered 	*
 *			through API's.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:32:51 $
 * $Id: hci_register_callbacks.c,v 1.27 2014/10/03 16:32:51 steves Exp $
 * $Revision: 1.27 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Macros. */

#define	MAX_RADIAL_INFO_EVTS	10

/* Static/global variables. */

static	int	Hci_syslog_update_flag = 0;
static	int	Hci_prod_info_update_flag = 0;
static	int	Hci_env_wind_update_flag = 0;
static	float	Azimuth = HCI_INITIAL_AZIMUTH;
static	float	First_azimuth = HCI_INITIAL_AZIMUTH;
static	float	Elevation = 0.0;
static	int	Azimuth_num = 0;
static	int	Elevation_num = 1;
static	int	Radial_status = 0;
static  int     Super_res = 0;
static  int     Moments = 0;
static  int     Last_ele_flag = 0;
static  int     N_sails_cuts = 0;
static  int     Sails_cut_seq = 0;
static	int	radial_info_push_ptr = 0;
static	int	radial_info_pop_ptr = 0;
static	int	Previous_radial_status = -1;
static	Orpgevt_radial_acct_t radial_info_queue[ MAX_RADIAL_INFO_EVTS ];

/* Function prototypes. */

void	hci_update_environmental_wind_data( EN_id_t, void *, size_t, void* );
void	hci_rda_performance_data_update_event( EN_id_t, void *, size_t, void*);
void 	hci_update_rms( EN_id_t, void *, size_t, void*);
void	hci_RDA_message_update( int, LB_id_t, int, void * );
void	hci_system_log_updated( int, LB_id_t, int, void * );
void	hci_prod_info_updated( int, LB_id_t, int, void * );
void 	hci_radial_info_update( EN_id_t, void *, size_t, void* );

/************************************************************************
 *	Description: This function is used to initialize/register data	*
 *	stores and events needed by the RPG Control/Status task that	*
 *	are not initialized/registered through API's.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_register_callbacks()
{
  int status;

  status = EN_register( ORPGEVT_PERF_MAIN_RECEIVED,
			( void * ) hci_rda_performance_data_update_event );

  HCI_LE_log("EN_register ORPGEVT_PERF_MAIN_RECEIVED: %d", status);

  /* Register for environmental winds update events. */

  status = EN_register( ORPGEVT_ENVWND_UPDATE,
                        ( void * ) hci_update_environmental_wind_data );

  HCI_LE_log("EN_register ORPGEVT_ENVWND_UPDATE: %d", status);

  /* Register for RMS change events. */

  status = EN_register( ORPGEVT_RMS_CHANGE, ( void * ) hci_update_rms );

  HCI_LE_log("EN_register ORPGEVT_RMS_CHANGE: %d", status);

  /* Register for radial info events. */

  status = EN_register( ORPGEVT_RADIAL_ACCT,
			( void * ) hci_radial_info_update );

  HCI_LE_log("EN_register ORPGEVT_RADIAL_ACCT: %d", status);

  /* Register for updates to the system log file. We don't actually
     look at the main system log file but at the latest message
     which is extracted from the main system log file by the HCI
     agent task.  This is done to minimize data input for low
     bandwidth configurations. */

  ORPGDA_write_permission( ORPGDAT_SYSLOG_LATEST );

  status = ORPGDA_UN_register( ORPGDAT_SYSLOG_LATEST, LB_ANY,
                               hci_system_log_updated );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error("Unable to register for system log updates [%d]", status);
  }

  /* Register for updates to the product generation/distribution
     and product user line status LBs. These are used to determine
     the status of narrowband lines. */

  status = ORPGDA_UN_register( ORPGDAT_HCI_DATA, HCI_PROD_INFO_STATUS_MSG_ID,
                               hci_prod_info_updated );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error("Unable to register for PROD_INFO_STATUS_MSG [%d]", status);
  }

  /* Register for updates to the RDA console message LB. */

  ORPGDA_write_permission( ORPGDAT_RDA_CONSOLE_MSG );

  status = ORPGDA_UN_register( ORPGDAT_RDA_CONSOLE_MSG, LB_ANY, hci_RDA_message_update );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error( "Unable to register for RDA Console Msg [%d]", status );
  }
}

/************************************************************************
 *	Description:  The following function is the callback invoked	*
 *	when an RDA Performance Data Update event is detected.		*
 *									*
 *	Input:  evtcd  - event code;					*
 *		*ptr   - pointer to data associated with event		*
 *		msglen - length of data (bytes).			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_rda_performance_data_update_event( EN_id_t evtcd, void *ptr,
                                            size_t msglen, void *arg)
{
  int config = ORPGRDA_get_rda_config( NULL );

  if( config == ORPGRDA_LEGACY_CONFIG )
  {
    hci_set_rda_performance_update_flag( 1 );
  }
  else if( config == ORPGRDA_ORDA_CONFIG )
  {
    hci_set_orda_pmd_update_flag( 1 );
  }
}

/************************************************************************
 *	Description:  The following function is the callback invoked	*
 *	when an RDA Console Message Update event is detected.		*
 *									*
 *	Input:  evtcd  - event code;					*
 *		*ptr   - pointer to data associated with event		*
 *		msglen - length of data (bytes).			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_RDA_message_update( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  hci_set_RDA_message_flag( 1 );
}

/************************************************************************
 *	Description:  The following function is the callback invoked	*
 *	when an RMS task status update event is detected.		*
 *									*
 *	Input:  evtcd  - event code;					*
 *		*ptr   - pointer to data associated with event		*
 *		msglen - length of data (bytes).			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_update_rms( EN_id_t evtcd, void *ptr, size_t msglen, void *arg )
{
  int rms_state = *( int* ) ptr;

  /* Set the RMS task status flag. */
	
  hci_set_rms_down_flag( rms_state );
}

/************************************************************************
 *	Description: This is the callback for radial info events.  The	*
 *		     radial info event is generated by the process base	*
 *		     data (pbd) task.  Its purpose is to provide	*
 *		     antenna position information for the HCI for the	*
 *		     radome graphic (shows antenna azran).		*
 *									*
 *	Input:  evtcd  - Event code (see orpgevt.h)			*
 *	*ptr   - Pointer to event data					*
 *		 msglen - Size (bytes) of event data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_radial_info_update( EN_id_t evtcd, void *ptr, size_t msglen, void *arg )
{
  Orpgevt_radial_acct_t *info = ( Orpgevt_radial_acct_t * )ptr;

  /* Sanity check. Ignore event if check fails. */

  if( msglen != sizeof( Orpgevt_radial_acct_t) )
  {
    HCI_LE_error("Bad radial info event size (%d vs %d)",
                 msglen, sizeof( Orpgevt_radial_acct_t));
    return;
  }

  /* Copy event into queue. */

  memcpy( &radial_info_queue[ radial_info_push_ptr ], ptr, msglen );

  /* To prevent a large number of entries with the same radial
     status, only increment pointer flag if radial status changes
     or the queue is empty. */

  if( info->radial_status != Previous_radial_status ||
      radial_info_push_ptr == radial_info_pop_ptr )
  {
    Previous_radial_status = info->radial_status;
    radial_info_push_ptr = (radial_info_push_ptr + 1) % MAX_RADIAL_INFO_EVTS;
  }
}

/************************************************************************
 *	Description: This function consumes radial info events. Various	*
 *		     values are set according to the latest radial info	*
 *		     event in the queue.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_set_latest_radial_info()
{
  Orpgevt_radial_acct_t info;

  /* Check if queue is empty. */

  if( radial_info_pop_ptr == radial_info_push_ptr )
  {
    return;
  }

  /* Queue is not empty. Set values according to next event in
     the queue and increment pointer flag. */

  info = radial_info_queue[ radial_info_pop_ptr ];
  radial_info_pop_ptr = (radial_info_pop_ptr + 1) % MAX_RADIAL_INFO_EVTS;

  Azimuth       = info.azimuth/10.0;
  Elevation     = info.elevation/10.0;
  Azimuth_num   = info.azi_num;
  Elevation_num = info.elev_num;
  Radial_status = info.radial_status;
  Super_res 	= info.super_res & HCI_ELEV_SR_BITMASK;
  Moments 	= info.moments;
  First_azimuth = info.start_elev_azm/10.0;
  Last_ele_flag = (int) info.last_ele_flag;
  N_sails_cuts  = (int) info.n_sails_cuts;
  Sails_cut_seq = (int) info.sails_cut_seq;

  /* Check if HCI should be cleared. */

  if( Azimuth_num == 1 || Radial_status == BEG_ELEV || Radial_status == BEG_VOL )
  {
    hci_control_panel_clear_HCI();
  }

  /* Needed to update the radome. */

  hci_control_panel_radome_data( Elevation_num, Super_res,
                                 Elevation, N_sails_cuts,
                                 Sails_cut_seq, Last_ele_flag );
}

/************************************************************************
 *	Description:  The following function is the callback invoked	*
 *	when a new message is written to the latest system log message	*
 *	LB.					.			*
 *									*
 *	Input:  fd       - File descriptor of LB with new message	*
 *		msg_id   - The ID of the new message in the LB		*
 *		msg_info - The length (bytes) of the new message	*
 *		*arg     - user registered argument (unused).		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_system_log_updated( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  Hci_syslog_update_flag = 1;
}

/************************************************************************
 *	Description:  The following function returns the value of the	*
 *	system log file update flag.					*
 *									*
 *	Input:	NONE							*
 *	Output: NONE							*
 *	Return: value of system log update flag				*
 ************************************************************************/

int hci_get_system_log_update_flag()
{
  return( Hci_syslog_update_flag );
}

/************************************************************************
 *	Description:  The following function sets the value of the	*
 *	system log file update flag.					*
 *									*
 *	Input:	value							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_set_system_log_update_flag( int flag )
{
  Hci_syslog_update_flag = flag;
}

/************************************************************************
 *	Description:  The following function is the callback invoked	*
 *	when a new message is written to the PD_LINE_INFO_MSG_ID msg	*
 *	of the ORPGDAT_PROD_INFO LB.					*
 *									*
 *	Input:  fd       - File descriptor of LB with new message	*
 *		msg_id   - The ID of the new message in the LB		*
 *		msg_info - The length (bytes) of the new message	*
 *		*arg     - user registered argument (unused).		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_prod_info_updated( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  Hci_prod_info_update_flag = 1;
}

/************************************************************************
 *	Description:  The following function returns the value of the	*
 *	pd line info update flag.					*
 *									*
 *	Input:	NONE							*
 *	Output: NONE							*
 *	Return: value of pd line info update flag			*
 ************************************************************************/

int hci_get_prod_info_update_flag()
{
  return( Hci_prod_info_update_flag );
}

/************************************************************************
 *	Description:  The following function sets the value of the	*
 *	pd line info update flag.					*
 *									*
 *	Input:	value							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_set_prod_info_update_flag( int flag )
{
  Hci_prod_info_update_flag = flag;
}

/************************************************************************
 *	Description: This function returns the latest azimuth angle	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Azimuth angle (deg)					*
 ************************************************************************/

float hci_control_panel_azimuth()
{
  return( Azimuth );
}

/************************************************************************
 *      Description: This function returns the first azimuth angle	*
 *		of this current elevation as published by the process	*
 *		base data (pbd) task.					*
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: Azimuth angle (deg)                                     *
 ************************************************************************/

float hci_control_panel_first_azimuth()
{
  return( First_azimuth );
}

/************************************************************************
 *	Description: This function returns the latest elevation angle	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Elevation angle (deg)					*
 ************************************************************************/

float hci_control_panel_elevation()
{
  return( Elevation );
}

/************************************************************************
 *	Description: This function returns the latest azimuth number	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Azimuth number						*
 ************************************************************************/

int hci_control_panel_azimuth_num()
{
  return( Azimuth_num );
}

/************************************************************************
 *	Description: This function returns the latest elevation number	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Elevation number					*
 ************************************************************************/

int hci_control_panel_elevation_num()
{
  return( Elevation_num );
}

/************************************************************************
 *	Description: This function returns the Super Resolution flag	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Super Resolution flag					*
 ************************************************************************/

int hci_control_panel_super_res()
{
  return( Super_res );
}

/************************************************************************
 *	Description: This function returns the Last Elevation flag	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Last Elevation flag					*
 ************************************************************************/

int hci_control_panel_last_ele_flag()
{
  return( Last_ele_flag );
}

/************************************************************************
 *	Description: This function returns the Number of SAILS cuts	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Number SAILS cuts this VCP				*
 ************************************************************************/

int hci_control_panel_n_sails_cuts()
{
  return( N_sails_cuts );
}

/************************************************************************
 *	Description: This function returns the SAILS cut sequence #	*
 *		published by the process base data (pbd) task.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: SAILS cut sequence number				*
 ************************************************************************/

int hci_control_panel_sails_cut_seq()
{
  return( Sails_cut_seq );
}

/************************************************************************
 *      Description: This function returns the Moments flag published   *
 *              by the process base data (pbd) task.                    *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: Moments flag                                            *
 ************************************************************************/

int hci_control_panel_moments()
{
  return( Moments );
}

/************************************************************************
 *	Description: This function is the event notification callback	*
 *		     for ORPGEVT_ENVWND_UPDATE events.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_update_environmental_wind_data( EN_id_t evtcd, void *ptr, 
                                         size_t msglen, void *arg )
{
  Hci_env_wind_update_flag = 1;
}

/************************************************************************
 *	Description:  The following function returns the value of the	*
 *	environmental wind update flag.					*
 ************************************************************************/

int hci_get_env_wind_update_flag()
{
  return( Hci_env_wind_update_flag );
}

/************************************************************************
 *	Description:  The following function sets the value of the	*
 *	environmental wind update flag.					*
 ************************************************************************/

void hci_set_env_wind_update_flag( int flag )
{
  Hci_env_wind_update_flag = flag;
}

