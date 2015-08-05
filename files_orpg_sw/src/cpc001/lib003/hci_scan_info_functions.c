/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:25 $
 * $Id: hci_scan_info_functions.c,v 1.13 2009/02/27 22:26:25 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_scan_info_functions.c				*
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate the Volume scan info message in 	*
 *		      the gen_stat_msg linear buffer.			*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_scan_info.h>

#define	ELEVATION_SCALE		10.0

/*	Static variables						*/

static	Vol_stat_gsm_t	Scan_info; /* local scan info buffer. */
static	short		Scan_active_flag = 0; /* flag = 1 means radial data
				are being ingested by RPG. */
static	short		Scan_num   = 1; /* current scan number */
static	int		Scan_flag  = 1; /* used to indicate scan info msg
				has been updated and needs to be read.	*/
static	int		size       = sizeof (Vol_stat_gsm_t);
					/* size of scan info message */
static  int 		Scan_io_status = 0;  /* Last scan info I/O status */
int	len;

/************************************************************************
 *	Description: This function returns the status from the last I/O	*
 *		     operation						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Last scan info message I/O status.			*
 ************************************************************************/

int hci_scan_info_io_status()
{
	return(Scan_io_status);
}

/************************************************************************
 *	Description: This function returns the current scan number.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Current key value.					*
 ************************************************************************/

short
hci_get_scan_info_key ()
{
	return	Scan_num;
}

/************************************************************************
 *	Description: This function sets the current scan number.	*
 *									*
 *	Input:  key - new key value					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_scan_info_key (short key)
{
	if (key == Scan_num) {

	    Scan_num = -Scan_num;

	} else {

	    Scan_num = key;

	}
}

/************************************************************************
 *	Description: This function reads the current scan info message.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_read_scan_info_data ()
{

	/*  Set compression for GSM data */
	len = ORPGDA_read (ORPGDAT_GSM_DATA,
		 	   (char *) &Scan_info,
		           size,
		           VOL_STAT_GSM_ID);

	Scan_io_status = len;
	Scan_flag = 0;

	if (len < 0) {

	    HCI_LE_error("Error reading VOL_STAT_GSM_ID: %d", len);

	}
}

/************************************************************************
 *	Description: This function gets the current scan volume number.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: current scan volume number				*
 ************************************************************************/

unsigned long
hci_get_scan_volume_number ()
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}

	return Scan_info.volume_number;
}

/************************************************************************
 *	Description: This function gets the current scan mode.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: current scan mode					*
 ************************************************************************/

int
hci_get_scan_mode_operation ()
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}

	return Scan_info.mode_operation;
}

/************************************************************************
 *	Description: This function gets the current scan date.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: current scan julian date				*
 ************************************************************************/

int
hci_get_scan_julian_date ()
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return Scan_info.cv_julian_date;
}

/************************************************************************
 *	Description: This function gets the current scan time.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: current scan time (milliseconds past midnight)		*
 ************************************************************************/

int
hci_get_scan_time ()
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return Scan_info.cv_time;
}

/************************************************************************
 *	Description: This function gets the current scan VCP number.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: current scan VCP number					*
 ************************************************************************/

int
hci_get_scan_vcp_number ()
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return Scan_info.vol_cov_patt;
}

/************************************************************************
 *	Description: This function returns the number of elevation cuts	*
 *		     in the current VCP.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of elevation cuts in VCP				*
 ************************************************************************/

int
hci_get_scan_number_elevation_cuts ()
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return Scan_info.num_elev_cuts;
}

/************************************************************************
 *	Description: This function returns the RPG elevation index	*
 *		     associated with the specified cut.			*
 *									*
 *	Input:  cut - cut number					*
 *	Output: NONE							*
 *	Return: elevation index						*
 ************************************************************************/

int
hci_get_scan_elevation_number (
int	cut
)
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return (int) Scan_info.elev_index [cut];
}

/************************************************************************
 *	Description: This function returns the status of the previous	*
 *		     volume scan.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: 1 = completed OK; 0 = aborted				*
 ************************************************************************/

int
hci_get_scan_status (
)
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return (int) Scan_info.pv_status;
}

/************************************************************************
 *	Description: This function returns the unscaled elevation	*
 *		     angle associated with the pointed to elevation	*
 *		     index.						*
 *									*
 *	Input:  cut - elevation cut					*
 *	Output: NONE							*
 *	Return: unscaled elevation angle (degrees)			*
 ************************************************************************/

float
hci_get_scan_elevation_angle (
int	cut
)
{
	if (Scan_flag) {

	    hci_read_scan_info_data ();

	}
	return (float) Scan_info.elevations [cut]/ELEVATION_SCALE;
}

/************************************************************************
 *	Description: This function returns the flag indicating whether	*
 *		     the scan info message needs to be read.  Normally,	*
 *		     an event handler should use the correcponding set	*
 *		     function when an update event occurs.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: update flag (0 = no update; 1 = update)			*
 ************************************************************************/

int
hci_get_scan_info_flag ()
{
	return Scan_flag;
}

/************************************************************************
 *	Description: This function sets the flag indicating whether	*
 *		     the scan info message needs to be read.		*
 *									*
 *	Input:  flag - new update flag value (should be 0 or 1)		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_scan_info_flag (
int	flag
)
{
	Scan_flag = flag;
}

/************************************************************************
 *	Description: This function returns a flag indicating if data	*
 *		     are being processed by pbd.  A non 0 value		*
 *		     indicates that new data are being processed.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: 0 - no data begin processed; !0 - data being processed	*
 ************************************************************************/

int
hci_get_scan_active_flag ()
{
	return Scan_active_flag;
}

/************************************************************************
 *	Description: This function sets a flag indicating if data	*
 *		     are being processed by pbd.  A non 0 value		*
 *		     indicates that new data are being processed.	*
 *									*
 *	Input:  0 - no data begin processed; !0 - data being processed	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_scan_active_flag (
int	flag
)
{
	Scan_active_flag = flag;
}
