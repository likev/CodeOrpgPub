/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/04/06 18:17:59 $
 * $Id: hci_rda_performance_data_functions.c,v 1.22 2009/04/06 18:17:59 ccalvert Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_rda_performance_data_functions.c			*
 *									*
 *	This file contains a collection of modules used to manipulete	*
 *	rda performance data.						*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_rda_performance.h>

/*	Global variables						*/

static	rda_performance_t	*Perf_data; /* Performance data buffer */
static	char			Pdata [sizeof (rda_performance_t)];
					    /* Performance data buffer */
static	int			Init_flag = 0; /* initialization flag */
static	int			Update_flag = 0; /* data update flag
						    !0 = needs update; */

/************************************************************************
 *	Description: This function initializes RDA performance data.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: status of read operation (error if not positive)	*
 ************************************************************************/

int
hci_initialize_rda_performance_data ()
{
	int	len;

/*	This initialization routine simply calls the read function	*/

	len = hci_read_rda_performance_data ();

	return len;
}

/************************************************************************
 *	Description: This function reads RDA performance data from the	*
 *		     RDA Performance linear buffer and saves it in	*
 *		     memory so other functions can access it.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: status of read operation (error if not positive)	*
 ************************************************************************/

int
hci_read_rda_performance_data ()
{
	int	len;

/*	Read the next message in the RDA performance linear buffer.  A 	*
 *	good read is one where the returned length is equal to the	*
 *	length requested.  If no new status messages exist, return	*
 *	the contents of the last message (which is saved until a new	*
 *	message is read).  Otherwise, return an error code.		*/

	len = ORPGDA_read (ORPGDAT_RDA_PERF_MAIN,
			   Pdata,
			   sizeof (rda_performance_t),
			   LB_NEXT);

	Perf_data = (rda_performance_t *) Pdata;

	if (len <= 0) {

/*	Check to see if any messages exist in the RDA Performance	*
 *	LB.  If they do, then read the contents of the last message	*
 *	in the LB.							*/

	    len = ORPGDA_seek (ORPGDAT_RDA_PERF_MAIN,
			      0,
			      LB_LATEST,
			      NULL);

	    if (len == LB_SUCCESS) {

		len = ORPGDA_read (ORPGDAT_RDA_PERF_MAIN,
			   Pdata,
			   sizeof (rda_performance_t),
			   LB_NEXT);

		if (len <= 0) {

		    return len;

		}

	    } else {

		return len;
	    }
	}

	Init_flag = 1;
	return len;

}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     RDA performance data buffer.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to start of RDA performance data buffer		*
 ************************************************************************/

rda_performance_t
*hci_get_rda_performance_data_ptr ()
{
	int	len;

	if (!Init_flag) {

	    len = hci_read_rda_performance_data ();

	}

	return (rda_performance_t *) Perf_data;
}

/*	The following command is used to send a request to the RDA	*
 *	for RDA performance data.					*/

void
hci_request_new_rda_performance_data ()
{
	int	status;
	char	buf [80];

	sprintf (buf,"Requesting new RDA Performance Data");

	HCI_display_feedback( buf );

	status = ORPGRDA_send_cmd (COM4_REQRDADATA,
			      (int) HCI_INITIATED_RDA_CTRL_CMD,
			      DREQ_PERFMAINT,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      NULL);

/*	Right now nothing is done if the request to send the command	*
 *	is unsuccessfull (status < 0).					*/

}

/************************************************************************
 *	Description: This function returns the value of the specified	*
 *		     RDA performance data item.				*
 *		     NOTE: At this time only generator fuel level data	*
 *		     are supported by this function.			*
 *									*
 *	Input:  item - ID of data item to extract from buffer		*
 *	Output: NONE							*
 *	Return: data value						*
 ************************************************************************/

int
hci_rda_performance_data (
int	item
)
{
	int	num;
	int	len;

	if (!Init_flag || Update_flag) {

            if( Update_flag )
               Update_flag = 0;

	    len = hci_read_rda_performance_data ();

	}

	switch (item) {

	    case GEN_FUEL_LEVEL :

		num = Perf_data->data.gen_fuel_level;
		break;

	    default :

		num = -1;
		break;

	}

	return num;

}

/************************************************************************
 *	Description: This function sets the value of the RDA		*
 *		     performance data update flag to the specified	*
 *		     value.						*
 *									*
 *	Input:  state - new update state (should be 0 for no update and	*
 *			non-zero for need update).			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_rda_performance_update_flag (
int	state
)
{
	Update_flag = state;
}

/************************************************************************
 *	Description: This function gets the value of the RDA		*
 *		     performance data update flag.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: update flag (!0 - needs update; 0 - no update)		*
 ************************************************************************/

int
hci_get_rda_performance_update_flag (
)
{
	return	Update_flag;
}

/************************************************************************
 *	Description: This function returns the value of the RDA		*
 *		     performance data initialization flag.  A 0 means	*
 *		     that the LB contains no messages.  1 means data	*
 *		     has been successfully read and the structure	*
 *		     properly initialized.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: initialization flag					*
 ************************************************************************/

int
hci_rda_performance_data_initialized (
)
{
	return Init_flag;
}

/************************************************************************
 *	Description: This function returns a pointer to a string with	*
 *		     the date/time of the latest RDA Performance data	*
 *		     message.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to date/time string.				*
 ************************************************************************/

char
*hci_rda_performance_time (
)
{
static	char	string [32];
	int	month, day, year;
	int	hour, minute, second;
	long int	seconds;

	if (Init_flag) {

	    seconds = (Perf_data->msg_hdr.julian_date-1)*HCI_SECONDS_PER_DAY +
		      Perf_data->msg_hdr.milliseconds/HCI_MILLISECONDS_PER_SECOND;

	    unix_time (&seconds,
		&year,
		&month,
		&day,
		&hour,
		&minute,
		&second);

	    sprintf (string,"%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
			HCI_get_month(month), day, year,
			hour, minute, second);

	    return (char *) string;

	} else {

	    return (char *) NULL;

	}
}
