/************************************************************************
 *									*
 *	Module:  hci_basedata_interrogate.c				*
 *									*
 *	Description: This module is used by the RPG Base Data Display	*
 *		     task to interrogate a pixel in the base data	*
 *	             display window and return the raw data value.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:49 $
 * $Id: hci_basedata_interrogate.c,v 1.13 2009/02/27 22:25:49 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_basedata.h>

/*	Macros.								*/

#define	BEAM_WIDTH	1.1

/************************************************************************
 *	Description: This function returns the value at a specified	*
 *		     polar coordinate.					*
 *									*
 *	Input:  data		- real-time (NULL) or replay database	*
 *				  data pointer.				*
 *		target_azimuth	- azimuth of the location from radar	*
 *		target_range	- range of the location from radar	*
 *		moment		- Data field to interrogate		*
 *		ptr		- message ID to begin search.		*
 *	Output: NONE							*
 *	Return: data value						*
 ************************************************************************/

float
hci_basedata_interrogate (
char		data[],		/* real-time (NULL) or replay database	*
				 * data pointer.			*/
float		target_azimuth,	/* azimuth of the location from radar	*/
float		target_range,	/* range of the location from radar	*/
int		moment,		/* Data field to interrogate		*/
int		ptr		/* message ID to begin search.		*/
)
{
	float	value = 0.0;
	LB_id_t	lb_id;
	int	indx;
	int	status;
	float	diff;

/*	If the data pointer is NULL we read from BASEDATA through	*
 *	libhci.								*/

	if (data == NULL) {

/*	    Make sure the basedata LB isn't currently being read by	*
 *	    another task.  If so, wait until it is unlocked.		*/

	    if (hci_basedata_get_lock_state ()) {
	
	        return -999.9;

	    }

/*	    Lock the basedata LB so another task cannot use it while	*
 *	    the file pointer is changed.				*/

	    hci_basedata_set_lock_state (1);

/*	    Save the message ID of the current radial so we can restore	*
 *	    it later.							*/

	    lb_id = hci_basedata_msgid ();

/*	    Move the message pointer to the first radial in the 	*
 *	    elevation cut.						*/

	    status = hci_basedata_seek (ptr);

	    if (status != LB_SUCCESS) {

		HCI_LE_error("ERROR LB_seek: %d", status);
	        value = -999.9;

	    }

	    status = hci_basedata_read_radial (LB_NEXT, HCI_BASEDATA_COMPLETE_READ);

/*	    If the read operation successful then get the azimuth angle	*
 *	    of the beam and see if it matches the target azimuth		*/

	    if (status > 0) {

	        diff = fabs ((double) (target_azimuth - hci_basedata_azimuth ()));

/*	        If the azimuth and target azimuths match, get the data	*
 *	        value and get out of the loop.				*/

	        if (diff < BEAM_WIDTH) {
	    
		    indx = (int) ((target_range*1000 -
			hci_basedata_range_adjust (moment))/
			hci_basedata_bin_size (moment) +
			hci_basedata_bin_size (moment)/2000);

		    if (indx >hci_basedata_number_bins (moment)) {

		        value = -999.0;
		        indx = -1;

		    }

		    if (indx >= 0) {

		        value = hci_basedata_value (indx, moment);

		    }

	        } else {

		    value = -888.8;

	        }
	    }

/*	    Restore the LB pointer to the state prior to entering this	*
 *	    routine.							*/

	    status = hci_basedata_seek (lb_id);

/*	    Unlock the basedata LB so another task can use it.		*/

	    hci_basedata_set_lock_state (0);

/*	Else, we read information from the replay database.		*/

	} else {

	    LB_id_t		start_id;
	    LB_id_t		stop_id;
	    unsigned int	vol_num;
	    int 		sub_type;
	    Base_data_header	*hdr;
	    short		*bin;
	    int			gates;
	    float		adjust;
	    int			i;

	    hdr = (Base_data_header *) &data[0];

	    vol_num = ORPGVST_get_volume_number();

            if( moment == REFLECTIVITY )
	       sub_type = (BASEDATA_TYPE | REFLDATA_TYPE);

            else
	       sub_type = (BASEDATA_TYPE | COMBBASE_TYPE);

	    start_id = ORPGBDR_get_start_of_elevation_msgid (BASEDATA,
			sub_type,
			vol_num,
			ptr);

	    stop_id = ORPGBDR_get_end_of_elevation_msgid (BASEDATA,
			sub_type,
			vol_num,
			ptr);

	    if ((start_id <= 0) || (stop_id <= 0)) {

		vol_num--;

		start_id = ORPGBDR_get_start_of_elevation_msgid (BASEDATA,
			sub_type,
			vol_num,
			ptr);

		stop_id = ORPGBDR_get_end_of_elevation_msgid (BASEDATA,
			sub_type,
			vol_num,
			ptr);

		if ((start_id <= 0) || (stop_id <= 0)) {

		    value = -999.9;

		}
	    }

	    if ((start_id > 0) && (stop_id > 0)) {

		for (i=start_id;i<stop_id;i++) {

		    status = ORPGBDR_read_radial (BASEDATA,
				&data[0], SIZEOF_BASEDATA, (LB_id_t) i);

		    if (status <= 0) {

			HCI_LE_error("ORPGBDR_read_radial failed (%d)", status);
			value = -999.9;
			break;

		    }

		    diff = fabs ((double) (target_azimuth - hdr->azimuth));

/*		    If the azimuth and target azimuths match, get	*
 *		    the data value and get out of the loop.		*/

		    if (diff < BEAM_WIDTH) {

			switch (moment) {

			    case REFLECTIVITY :
			    default :

				gates  = hdr->n_surv_bins;
				adjust = hdr->surv_range/HCI_METERS_PER_KM;
                                if( hdr->surv_bin_size == REFL_QUARTERKM )
				   indx   = (int) (4*(target_range - adjust)); 

				else
				   indx   = (int) (target_range - adjust); 

				if (indx <= gates) {

				    bin = (short *) &(data [START_OF_REFLECTIVITY_DATA]);
				    value = hci_basedata_refl_value (bin[indx]);
				    break;

				} else {

				    value = -999.9;

				}

				break;

			    case VELOCITY :

				gates  = hdr->n_dop_bins;
				adjust = hdr->dop_range/HCI_METERS_PER_KM;
				indx   = (int) (4*(target_range - adjust)); 

				if (indx <= gates) {

				    bin = (short *) &(data [START_OF_VELOCITY_DATA]);
				    value = hci_basedata_dopl_value (bin[indx]);
				    break;

				} else {

				    value = -999.9;

				}
				break;

			    case SPECTRUM_WIDTH :

				gates  = hdr->n_dop_bins;
				adjust = hdr->dop_range/HCI_METERS_PER_KM;
				indx   = (int) (4*(target_range - adjust)); 

				if (indx <= gates) {

				    bin = (short *) &(data [START_OF_SPECTRUM_WIDTH_DATA]);
				    value = hci_basedata_width_value (bin[indx]);
				    break;

				} else {

				    value = -999.9;

				}
				break;

			}

			break;

		    }
	        }
	    }
	}

	return value;
}
