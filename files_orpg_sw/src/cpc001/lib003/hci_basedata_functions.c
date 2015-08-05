/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:49 $
 * $Id: hci_basedata_functions.c,v 1.28 2009/02/27 22:25:49 ccalvert Exp $
 * $Revision: 1.28 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_basedata_functions.c				*
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate the internal basedata message.	*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_basedata.h>

/*	Static variables						*/

static	char   Data [SIZEOF_BASEDATA];	/* Radial data storage (1 beam) */
static	short  *Reflectivity_data;	/* Pointer to start of dBZ data */
static	short  *Velocity_data;		/* Pointer to start of vel data */
static	short  *Spectrum_width_data;	/* Pointer to start of spw data */

static	int    Init_flag  = 0;		/* Initialization flag */
static	int    Lock_state = 0;
static	int    Data_feed = SR_BASEDATA;	/* LB to use for basedata */

static	float  Reflectivity_LUT   [256]; /* Reflectivity lookup table */
static	float  Doppler_low_LUT    [256]; /* Dop low resolution lookup table */
static	float  Doppler_high_LUT   [256]; /* Dop high resolution lookup table */
static  float  Spectrum_Width_LUT [256]; /* Spectrum Width lookup table */
static	float  Reflectivity_range [MAX_BINS_ALONG_RADIAL];
static	float  Doppler_range      [MAX_BINS_ALONG_RADIAL];

/*	Function prototypes.						*/

static	void	hci_basedata_init();

/************************************************************************
 *	Description: Initialize the base data data source.		*
 *		     Assumes HCI_read_options has been called		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 *									*
 ************************************************************************/

static void hci_basedata_init()
{
	int	i;

	if (!Init_flag) {

/*	Initialize the data Lookup tables.				*/

	    for (i=2;i<256;i++) {

		Reflectivity_LUT [i] = i/2.0 - 33;
		Doppler_low_LUT  [i] = (i-129)*HCI_MPS_TO_KTS;
		Doppler_high_LUT [i] = ((i/2.0)-64.5)*HCI_MPS_TO_KTS;
		Spectrum_Width_LUT [i] = ((i/2.0)-64.5)*HCI_MPS_TO_KTS;
		if( Spectrum_Width_LUT[i] < 0.0 )
		    Spectrum_Width_LUT[i] = 0.0;

	    }

/*	    Set the below threshold value (0) to -999.9 and the	*
 *	    above threshold value (1) to 999.9.			*/

	    Reflectivity_LUT [0] = -999.9;
	    Doppler_low_LUT  [0] = -999.9;
	    Doppler_high_LUT [0] = -999.9;
	    Spectrum_Width_LUT [0] = -999.9;
	    Reflectivity_LUT [1] = 999.9;
	    Doppler_low_LUT  [1] = 999.9;
	    Doppler_high_LUT [1] = 999.9;
	    Spectrum_Width_LUT [1] = 999.9;

/*	    Initialize the range lookup tables.				*/

	    for (i=0;i<MAX_BINS_ALONG_RADIAL;i++) {

		Reflectivity_range [i] = i;
		Doppler_range      [i] = i/4.0;

	    }

/*	    Initialize the pointers to the moments in the message (real	*
 *	    time data stream only).					*/

	    Reflectivity_data   = (short *) &Data [START_OF_REFLECTIVITY_DATA];
	    Velocity_data       = (short *) &Data [START_OF_VELOCITY_DATA];
	    Spectrum_width_data = (short *) &Data [START_OF_SPECTRUM_WIDTH_DATA];
		
/*	    Initialize the entire data area to zeroes (might need to be	*
 *	    sophisticated).						*/

	    memset((void*)(&Data[0]), 0, SIZEOF_BASEDATA);

	    Init_flag = 1;

	}
}


/************************************************************************

    Description: The following routine gets the current basedata file
                 descriptor.

    Return:      Basedata file descriptor
 
 ************************************************************************/

int
hci_basedata_id ()
{
	hci_basedata_init();
	return ORPGDA_lbfd (Data_feed);
}


/************************************************************************

    Description: The following routine returns the message id of the 
                 previously	read message.

    Return:      ID of previously read message.

 ************************************************************************/

int
hci_basedata_msgid ()
{
	hci_basedata_init();
	return LB_previous_msgid (hci_basedata_id());
}


/************************************************************************

    Description: The following routine moves the LB message pointer to 
                 the specified message.

     Input:      msgid - The message to move the pointer to.

    Return:      status - the results of LB_seek(either LB_SUCCESS or an
                          error code. Refer lb.h for error codes.)

 ************************************************************************/

int
hci_basedata_seek (
int	msgid
)
{
	int	status;
	

	hci_basedata_init();	

	status = LB_seek (hci_basedata_id(),
			  0,
			  msgid,
			  NULL);

	return status;
}

/************************************************************************
 *	Description: This function reads the next radial in the 	*
 *		     basedata LB.					*
 *									*
 *	Input:  msgid - id of message to read				*
 *		partial_read - flag to control whether entire message	*
 *			       is read or just the header.		*
 *	Output: NONE							*
 *	Return: returns the message length on success(status > 0),	*
 *		or error code (refer to lb.h) for status <= 0.		*
 ************************************************************************/

int
hci_basedata_read_radial (
int	msg_id, 
int     partial_read
)
{
	int	status;
	int	i;
	char	data [SIZEOF_BASEDATA];
	int start_bytes_to_read = 0;
	int no_of_bytes_to_read = sizeof(Base_data_header);  

/*	Initialize internal arrays, etc if not done already.		*/

	hci_basedata_init();	

/*	If the input message ID contains the macro for the latest	*
 *	message, then move the file pointer to the latest message.	*/

	if (msg_id == LB_LATEST) {

	    status = hci_basedata_seek(LB_LATEST);
	    msg_id = LB_ANY;

	} else {

	    status = 0;

	}
	
	if (status >= 0) {

/*	    If a partial read is specified, read the radial header only	*/

  	    if (partial_read == HCI_BASEDATA_PARTIAL_READ) {

	      	status = LB_read_window (hci_basedata_id(),
					 start_bytes_to_read,
					 no_of_bytes_to_read);

		if (status >= 0) {

		    status = ORPGDA_read (Data_feed,
			    		  data, 
					  no_of_bytes_to_read,
					  msg_id);
		}

/*	    Else, read the entire radial message.			*/

	    } else {

		status = ORPGDA_read (Data_feed,
		    		      (char *) data,
				      SIZEOF_BASEDATA,
				      msg_id);
	    }	
	}
	
	
/*	If good data read, make sure to initialize the data lookup	*
 *	tables and range lookup tables.					*/

	if (status > 0) {

/*	    Since we read some data, lets put it into static storage	*
 *	    for other functions to access.				*/

	    for (i=0;i<status;i++)  {

	        Data [i] = data [i];

	    }
	}

	return status;

}

/************************************************************************
 *	Description: This function returns a pointer to the first data 	*
 *		     element (1st gate) for the specified moment.	*
 *									*
 *	Input:  moment - moment identifier: REFLECTIVITY,		*
 *					   VELOCITY			*
 *					   SPECTRUM_WIDTH		*
 *	Output: NONE							*
 *	Return: pointer to the first element of the specified moment	*
 *		in the radial (defaults to reflectivity).		*
 ************************************************************************/

short
*hci_basedata_data (
int	moment
)
{
	switch (moment) {

	    case REFLECTIVITY :

		return (short *) &Data [START_OF_REFLECTIVITY_DATA];

	    case VELOCITY :

		return (short *) &Data [START_OF_VELOCITY_DATA];

	    case SPECTRUM_WIDTH :

		return (short *) &Data [START_OF_SPECTRUM_WIDTH_DATA];

	    default :

		return (short *) &Data [START_OF_REFLECTIVITY_DATA];

	}
}

/************************************************************************
 *	Description: This function returns the range (in meters) to 	*
 *		     the first gate of the selected moment.		*
 *									*
 *	Input:  moment - moment identifier: REFLECTIVITY,		*
 *					   VELOCITY			*
 *					   SPECTRUM_WIDTH		*
 *	Output: NONE							*
 *	Return: range (meters) to the first gate			*
 ************************************************************************/

int
hci_basedata_range_adjust (
int	moment
)
{
	int	range;
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

/*	Get the range adjust data from the header for the specified	*
 *	moment.								*/

	switch (moment) {

	    case REFLECTIVITY :

		range = hdr->surv_range;
		break;

	    case VELOCITY :

		range = hdr->dop_range;
		break;

	    case SPECTRUM_WIDTH :

		range = hdr->dop_range;
		break;

	    default :

		range = 0;
		break;

	}

	return range;
}

/************************************************************************
 *	Description: This function returns the gate size (in meters) of	*
 *		     the selected moment.				*
 *									*
 *	Input:  moment - moment identifier: REFLECTIVITY,		*
 *					   VELOCITY			*
 *					   SPECTRUM_WIDTH		*
 *	Output: NONE							*
 *	Return: Gate size (meters)					*
 ************************************************************************/

int
hci_basedata_bin_size (
int	moment
)
{
	int	size;
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

/*	Get the gate size data from the header for the specified	*
 *	moment.								*/

	switch (moment) {

	    case REFLECTIVITY :

		size = hdr->surv_bin_size;
		break;

	    case VELOCITY :

		size = hdr->dop_bin_size;
		break;

	    case SPECTRUM_WIDTH :

		size = hdr->dop_bin_size;
		break;

	    default :

		size = 0;
		break;

	}

	return size;

}

/************************************************************************
 *	Description: This function returns the number of gates defined	*
 *		     in the radial for the selected moment.		*
 *									*
 *	Input:  moment - moment identifier: REFLECTIVITY,		*
 *					   VELOCITY			*
 *					   SPECTRUM_WIDTH		*
 *	Output: NONE							*
 *	Return: Number of gates in radial				*
 ************************************************************************/

int
hci_basedata_number_bins (
int	moment
)
{
	int	num_bins;
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

/*	Get the number of gates from the header for the specified	*
 *	moment.								*/

	switch (moment) {

	    case REFLECTIVITY :

		if (hdr->msg_type & REF_ENABLED_BIT) {

		    num_bins = hdr->n_surv_bins;

		} else {

		    num_bins = 0;

		}

		break;

	    case VELOCITY :

		if (hdr->msg_type & VEL_ENABLED_BIT) {

		    num_bins = hdr->n_dop_bins;

		} else {

		    num_bins = 0;

		}

		break;

	    case SPECTRUM_WIDTH :

		if (hdr->msg_type & WID_ENABLED_BIT) {

		    num_bins = hdr->n_dop_bins;

		} else {

		    num_bins = 0;

		}

		break;

	    default :

		num_bins = 0;
		break;

	}

	return num_bins;
}

/************************************************************************
 *	Description: This function returns the time (in milliseconds	*
 *		     past midnight) of the radial.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: time radial was generated				*
 ************************************************************************/

int
hci_basedata_time (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->time;
}

/************************************************************************
 *	Description: This function returns the date (Modified Julian	*
 *		     of the radial.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: date radial was generated				*
 ************************************************************************/

int
hci_basedata_date (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->date;
}

/************************************************************************
 *	Description: This function returns the unambiguous range (km).	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: the unambiguous range					*
 ************************************************************************/

float
hci_basedata_unambiguous_range (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return (float) (hdr->unamb_range/10.0);
}

/************************************************************************
 *	Description: This function returns the Nyquist velocity (m/s).	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: the Nyquist velocity					*
 ************************************************************************/

float
hci_basedata_nyquist_velocity (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return (float) (hdr->nyquist_vel/100.0);
}

/************************************************************************
 *	Description: This function returns the elevation angle (deg)	*
 *		     of the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: the antenna elevation angle.				*
 ************************************************************************/

float
hci_basedata_elevation (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->elevation;
}

/************************************************************************
 *	Description: This function returns the target elevation angle	*
 *		     (in degrees) of the current radial.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: the target antenna elevation angle.				*
 ************************************************************************/

float
hci_basedata_target_elevation (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return ( hdr->target_elev / 10.0 ); 
}

/************************************************************************
 *	Description: This function returns the azimuth number of the	*
 *		     current radial.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: the azimuth number.					*
 ************************************************************************/

int
hci_basedata_azimuth_number (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->azi_num;
}

/************************************************************************
 *	Description: This function returns the azimuth angle (deg)	*
 *		     of the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: the antenna azimuth angle.				*
 ************************************************************************/

float
hci_basedata_azimuth (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->azimuth;
}

/************************************************************************
 *	Description: This function returns the VCP associated with the	*
 *		     current radial.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VCP number.						*
 ************************************************************************/

int
hci_basedata_vcp_number (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->vcp_num;
}

/************************************************************************
 *	Description: This function returns the message type field from	*
 *		     the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: message type.						*
 ************************************************************************/

int
hci_basedata_msg_type (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->msg_type;
}

/************************************************************************
 *	Description: This function returns the elevation number from	*
 *		     the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: elevation number.					*
 ************************************************************************/

int
hci_basedata_elevation_number (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	return hdr->elev_num;
}

/************************************************************************
 *	Description: This function returns the velocity resolution of	*
 *		     the Doppler data in the current radial.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: resolution code						*
 ************************************************************************/

int
hci_basedata_velocity_resolution (
)
{
	Base_data_header	*hdr;

/*	Cast radial data mesage to radial message header	*/

	hdr = (Base_data_header *) Data;

	switch (hdr->dop_resolution) {

	    case 1 :

		return DOPPLER_RESOLUTION_HIGH;

	    case 2 :
	    default :

		return DOPPLER_RESOLUTION_LOW;

	}
}

/************************************************************************
 *	Description: This function returns the unscaled reflectivity	*
 *		     for a given scaled reflectivity value. 		*
 *									*
 *	Input:  num - scaled reflectivity value (-256 to 256)		*
 *	Output: NONE							*
 *	Return: unscaled reflectivity					*
 ************************************************************************/

float
hci_basedata_refl_value (
int	num
)
{

	return Reflectivity_LUT [num];

}

/************************************************************************
 *	Description: This function returns the unscaled value for a	*
 *		     given scaled spectrum width value. 		*
 *									*
 *	Input:  num - scaled value 					*
 *	Output: NONE							*
 *	Return: unscaled value						*
 ************************************************************************/

float
hci_basedata_width_value (
int	num
)
{
	float value = Spectrum_Width_LUT [num];

	return value;
}

/************************************************************************
 *	Description: This function returns the unscaled value for a	*
 *		     given scaled velocity value. 			*
 *									*
 *	Input:  num - scaled value (-256 to 256)			*
 *	Output: NONE							*
 *	Return: unscaled value						*
 ************************************************************************/

float
hci_basedata_dopl_value (
int	num
)
{
	float	value;

	switch (hci_basedata_velocity_resolution ()) {

	    case DOPPLER_RESOLUTION_LOW :

		value = Doppler_low_LUT [num];
		break;

	    default :

		value = Doppler_high_LUT [num];
		break;

	}

	return value;
}

/************************************************************************
 *	Description: This function returns the unscaled value for a	*
 *		     specific gate and moment.			 	*
 *									*
 *	Input:  indx   - range gate index				*
 *		moment - Moment ID					*
 *	Output: NONE							*
 *	Return: unscaled value						*
 ************************************************************************/

float
hci_basedata_value (
int	indx,
int	moment
)
{
	float	value;

	switch (moment) {

	    case SPECTRUM_WIDTH :

		value = Spectrum_Width_LUT [Spectrum_width_data [indx]];
		break;

	    case VELOCITY :

		switch (hci_basedata_velocity_resolution ()) {

		    case DOPPLER_RESOLUTION_LOW :

			value = Doppler_low_LUT [Velocity_data [indx]];
			break;

		    default :

			value = Doppler_high_LUT [Velocity_data [indx]];
			break;

		}

		break;

	    case REFLECTIVITY:
	    default :

		value = Reflectivity_LUT [Reflectivity_data [indx]];
		break;

	}

	return value;

}

/************************************************************************
 *	Description: This function returns the range (in km) of a	*
 *		     specified gate index  and moment.			*
 *									*
 *	Input:  indx   - range gate index				*
 *		moment - Moment ID					*
 *	Output: NONE							*
 *	Return: range to center of gate					*
 ************************************************************************/

float
hci_basedata_range (
int	indx,
int	moment
)
{
	float	range;

	switch (moment) {

	    case REFLECTIVITY :

                if( hci_basedata_bin_size (moment) == REFL_QUARTERKM )
		   range = Doppler_range [indx] +
			hci_basedata_range_adjust (moment)/HCI_METERS_PER_KM;
		else
		   range = Reflectivity_range [indx] +
			hci_basedata_range_adjust (moment)/HCI_METERS_PER_KM;
		break;

	    case VELOCITY :
	    case SPECTRUM_WIDTH :
	    default :

		range = Doppler_range [indx] +
			hci_basedata_range_adjust (moment)/HCI_METERS_PER_KM;
		break;

	}

	return range;

}

/************************************************************************
 *	Description: This function returns the minimum value for a	*
 *		     given moment.					*
 *									*
 *	Input:	moment - Moment ID					*
 *	Output: NONE							*
 *	Return: minimum unscaled value					*
 ************************************************************************/

float
hci_basedata_value_min (
int	moment
)
{
	float	value;

	switch (moment) {

	    case REFLECTIVITY :

		value = Reflectivity_LUT [2];
		break;

	    case SPECTRUM_WIDTH :

		value = Spectrum_Width_LUT [129];
		break;

	    case VELOCITY :
	    default :

		switch (hci_basedata_velocity_resolution ()) {

		    case DOPPLER_RESOLUTION_LOW :

			value = Doppler_low_LUT [2];
			break;

		    default:

			value = Doppler_high_LUT [2];
			break;

		}

	    break;

	}

	return value;

}

/************************************************************************
 *	Description: This function returns the maximum value for a	*
 *		     given moment.					*
 *									*
 *	Input:	moment - Moment ID					*
 *	Output: NONE							*
 *	Return: maximum unscaled value					*
 ************************************************************************/

float
hci_basedata_value_max (
int	moment
)
{
	float	value;

	switch (moment) {

	    case REFLECTIVITY :

		value = Reflectivity_LUT [255];
		break;

	    case SPECTRUM_WIDTH :

		value = Spectrum_Width_LUT [255];
		break;
		
	    case VELOCITY :
	    default :

		switch (hci_basedata_velocity_resolution ()) {

		    case DOPPLER_RESOLUTION_LOW :

			value = Doppler_low_LUT [255];
			break;

		    default:

			value = Doppler_high_LUT [255];
			break;

		}

	    break;

	}

	return value;

}

/************************************************************************
 *	Description: This function returns a flag indicating whether 	*
 *		     another function has locked the basedata LB.  This	*
 *		     must be done in instances where the file pointer	*
 *		     is changed to look at a previous message and no	*
 *		     timer updates are allowed. It is assumed that	*
 *		     whenever a function is done, it will restore the	*
 *		     file pointer to its state before the lock and 	*
 *		     reset the flag.					*
 *									*
 *	Input:	NONE							*
 *	Output: NONE							*
 *	Return: The lock state						*
 ************************************************************************/

int
hci_basedata_get_lock_state (
)
{
	return	Lock_state;
}

/************************************************************************
 *	Description: This function stes the basedata lock flag. Note: 	*
 *		     See the description for hci_basedata_get_lock_state*
 *		     for further information.				*
 *									*
 *	Input:	state - the new lock state				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_set_lock_state (
int	state
)
{
	Lock_state = state;
}

/************************************************************************
 *	Description: This function changes the LB to use for basedata.	*
 *									*
 *	Input:	feed_type - New feed type to use for basedata		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_set_data_feed (
int	feed_type
)
{
	Data_feed = feed_type;
}

