/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/04/06 18:17:57 $
 * $Id: hci_environmental_winds_functions.c,v 1.20 2009/04/06 18:17:57 ccalvert Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_environmental_winds_functions.c			*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used to interface with the environmental winds table.		*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_environmental_wind.h>

/*	Local global variables.						*/

char		*buf1 = NULL;  /* Common data buffer */
A3cd97		*Enw = (A3cd97 *) NULL; /* Pointer to winds data */

char		*buf2 = NULL;  /* Common data vuffer */
A3cd97		*Wind = (A3cd97 *) NULL; /* Pointer to winds data */
static	EWT_update_t ewt_update;

static	int	model_data_init = 0; /* Initialized? */
static	int	wind_data_init = 0; /* Initialized? */
static	int	model_data_update = 1; /* New data to read? */
time_t	tm;	/* time structure used when updating update time.	*/
static LB_id_t	ewt_lb_id = LBID_A3CD97; /* EWT lb id to read */

static void read_model_data();
static void model_data_callback( int, LB_id_t, int, void * );

/************************************************************************
 *	Description: This function returns a pointer to the local	*
 *		     environmental winds data buffer.  If it hasn't	*
 *		     been initialized (read from LB) first, it is	*
 *		     before retuning.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to local envirinmental winds data.		*
 ************************************************************************/

A3cd97
*hci_get_environmental_wind_data_ptr ()
{
	int	status;

/*	If environmental wind data buffer not initialized, then		*
 *	initialize it.							*/

	if (Enw == (A3cd97 *) NULL) {

	    status = hci_read_environmental_wind_data ();

	}

	return (A3cd97 *) Enw;
}

/************************************************************************
 *	Description: This function returns the value of the VAD update	*
 *		     flag in the local environmental winds buffer.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VAD update flag						*
 ************************************************************************/

int
hci_get_vad_update_flag (
)
{
	int	status;

/*	If environmental wind data buffer not initialized, then		*
 *	initialize it.							*/

	if (Enw == (A3cd97 *) NULL) {

	    status = hci_read_environmental_wind_data ();

	}

	return (int) Enw->envwndflg;
}

/************************************************************************
 *	Description: This function sets the value of the VAD update	*
 *		     flag in the environmental winds LB.		*
 *									*
 *	Input:  new VAD update flag value				*
 *	Output: NONE							*
 *	Return: negative - Error; 0 - No change; positive - Updated	*
 ************************************************************************/

int
hci_set_vad_update_flag (
int	state
)
{
	int	status;

/*	If the requested state is different from the current state,	*
 *	then get the current system copy and only update the VAD	*
 *	update flag and write it back.					*/


	if (Enw->envwndflg != state) 
	{
	    buf2 = (char *) calloc (sizeof (A3cd97), 1);

	    status = ORPGDA_read ((A3CD97 / ITC_IDRANGE) * ITC_IDRANGE,
			(char *) buf2,
		 	sizeof (A3cd97),
		 	A3CD97 % ITC_IDRANGE);

	    Wind = (A3cd97 *) buf2;

	    if (status < 0) {

/*		If an error occurred reading the system enw data then	*
 *		do not change VAD update flag.				*/

		HCI_LE_error("ORPGDA_read A3CD97 status: %d", status);

	    } else {

	        Wind->envwndflg = state;

		if( !wind_data_init )
		{
		  wind_data_init = 1;
		  ORPGDA_write_permission ((A3CD97/ITC_IDRANGE) * ITC_IDRANGE);
		}

		status = ORPGDA_write ((A3CD97/ITC_IDRANGE)*ITC_IDRANGE,
			      (char *) buf2,
			      sizeof (A3cd97),
		 	      A3CD97 % ITC_IDRANGE);

		if (status < 0) {

		    HCI_LE_error("ORPGDA_write A3CD97 status: %d", status);

		} else {

		    Enw->envwndflg  = state;

		}

		EN_post (ORPGEVT_ENVWND_UPDATE,
			  (void *) NULL,
			  0, 0);
	    }

	    free (buf2);

	} else {

	    status = 0;

	}

	return status;
}

/************************************************************************
 *	Description: This function returns the value of the model	*
 *		     update flag.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: model update flag					*
 ************************************************************************/

int
hci_get_model_update_flag()
{
  int status = -1;

  if( !model_data_init )
  {
    model_data_init = 1;

    /* Register for winds update events. */

    ORPGDA_write_permission( (EWT_UPT/ITC_IDRANGE)*ITC_IDRANGE );
    status = ORPGDA_UN_register ( (EWT_UPT/ITC_IDRANGE)*ITC_IDRANGE,
                                  LBID_EWT_UPT,
                                  (void *) model_data_callback);

    if(status != 0)
    {
      HCI_LE_log("UN_register EWT_UPT Updates: %d", status);
      HCI_task_exit( HCI_EXIT_FAIL );
    }
  }

  if( model_data_update )
  {
    model_data_update = 0;
    read_model_data();
  }

  return (int) ewt_update.flag;
}

/************************************************************************
 *      Description: This is the LB notification callback for model     *
 *                   updates.                                           *
 *                                                                      *
 *      Input:  evtcd - event code                                      *
 *              info - event data                                       *
 *              msglen - size (bytes) of event data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void
model_data_callback (
int     fd,
LB_id_t msgid,
int     msg_info,
void   *arg
)
{
  model_data_update = 1;
}

/************************************************************************
 *      Description: This is the LB notification callback for model     *
 *                   updates.                                           *
 *                                                                      *
 *      Input:  evtcd - event code                                      *
 *              info - event data                                       *
 *              msglen - size (bytes) of event data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void
read_model_data()
{
  EWT_update_t ewt_update_temp;
  int status = -1;

  /* Read into temporary variable. */

  status = ORPGDA_read( (EWT_UPT/ITC_IDRANGE)*ITC_IDRANGE,
                        (char *) &ewt_update_temp, sizeof( EWT_update_t ),
                        LBID_EWT_UPT );

  if( status <= 0 )
  {
    HCI_LE_error("ORPGDA_read(LBID_EWT_UPT) Failed (%d)", status );
  }
  else
  {
    /* Everything is okay, copy from temporary variable. */
    ewt_update = ewt_update_temp;
  }
}

/************************************************************************
 *	Description: This function sets the value of the model update	*
 *		     flag.						*
 *									*
 *	Input:  new model update flag value				*
 *	Output: NONE							*
 *	Return: < 0 - error, 0 - successl				*
 ************************************************************************/

int
hci_set_model_update_flag( int state )
{
  EWT_update_t ewt_update;
  int status = -1;

  ewt_update.flag = state;

  status = ORPGDA_write( (EWT_UPT/ITC_IDRANGE)*ITC_IDRANGE,
                         (char *) &ewt_update,
                         sizeof (EWT_update_t),
                         LBID_EWT_UPT % ITC_IDRANGE);

  if( status <= 0 )
  {
    HCI_LE_error("ORPGDA_write(LBID_EWT_UPT) Failed (%d)", status);
    return status;
  }

  return status;
}

/************************************************************************
 *	Description: This function writes the contents of the local	*
 *		     environmental winds data buffer to file.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: negative - Error; positive - number of bytes written	*
 ************************************************************************/

int
hci_write_environmental_wind_data (
)
{
	int	status;

	tm = time (NULL);

	Enw->sound_time = tm/HCI_SECONDS_PER_MINUTE;

/*	Write the new environmental wind table to the ITC.		*/

	if( !wind_data_init )
	{
	  wind_data_init = 1;
	  ORPGDA_write_permission ((A3CD97/ITC_IDRANGE) * ITC_IDRANGE);
	}

	status = ORPGDA_write ((A3CD97/ITC_IDRANGE) * ITC_IDRANGE,
			      buf1,
			      sizeof (A3cd97),
		 	      A3CD97 % ITC_IDRANGE);

	if (status < 0) {

	    HCI_LE_error("ORPGDA_write A3CD97 status: %d", status);

	} else {

	    EN_post (ORPGEVT_ENVWND_UPDATE,
			  (void *) NULL,
			  0, 0);

	}

	return status;
}

/************************************************************************
 *	Description: This function converts an input direction and	*
 *		     speed in U and V components.			*
 *									*
 *	Input:  direction - Wind direction (0 - 360 degrees)		*
 *		speed     - Wind speed (any speed units)		*
 *	Output: u         - U component	(same units as speed)		*
 *		v         - V component (same units as speed)		*
 *	Return: NONE							*
 ************************************************************************/

void
hci_extract_wind_components (
float	direction,
float	speed,
float	*u,
float	*v
)
{
	*u = -speed * sin ((double) direction*HCI_DEG_TO_RAD);
	*v = -speed * cos ((double) direction*HCI_DEG_TO_RAD);
}

/************************************************************************
 *	Description: This function reads the contents of the local	*
 *		     environmental winds data buffer from file.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: negative - Error; positive - number of bytes written	*
 ************************************************************************/

int
hci_read_environmental_wind_data (
)
{
	int	status;

	if (buf1 == (char *) NULL) {

	    buf1 = (char *) calloc (sizeof (A3cd97), 1);

	}

	if( ewt_lb_id == LBID_A3CD97 )
	{	
	  status = ORPGDA_read ((A3CD97 / ITC_IDRANGE) * ITC_IDRANGE,
			  (char *) buf1,
			  sizeof (A3cd97),
			  LBID_A3CD97);
	}
	else /* Assume only other choice is LBID_MODEL_EWT */
	{
	  status = ORPGDA_read ((MODEL_EWT / ITC_IDRANGE) * ITC_IDRANGE,
			  (char *) buf1,
			  sizeof (A3cd97),
			  LBID_MODEL_EWT);
	}

/*	If an error occured accessing the ITC generate an error		*
 *	message and fill the table with NULL data.			*/

	Enw = (A3cd97 *) buf1;

	return status;
}

/************************************************************************
 *	Description: This function sets the LB ID to use for reading	*
 *		     the EWT.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_ewt_display_flag( int display_flag )
{
  if( display_flag == DISPLAY_CURRENT )
  {
    ewt_lb_id = LBID_A3CD97;
    HCI_LE_log("Set display EWT LB ID to LBID_A3CD97");
  }
  else
  {
    ewt_lb_id = LBID_MODEL_EWT;
    HCI_LE_log("Set display EWT LB ID to LBID_MODEL_EWT");
  }

  hci_read_environmental_wind_data();
}
/************************************************************************
 *	Description: This function returns the flag indicating the LB	*
 *		     used for reading the EWT.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
hci_get_ewt_display_flag()
{
  if( ewt_lb_id == LBID_A3CD97 )
  {
    return DISPLAY_CURRENT;
  }

  /* Otherwise, it's the model. */

  return DISPLAY_MODEL;
}
