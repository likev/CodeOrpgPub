/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2006/06/19 19:54:25 $
 * $Id: orpgload.c,v 1.39 2006/06/19 19:54:25 ccalvert Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpglsc.c						*
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate load shed category info in the 	*
 *		      load_shed_msg linear buffer.			*
 *									*
 ************************************************************************/


#include <orpg.h>
#include <orpginfo.h>
#include <orpgload.h>
#include <orpgevt.h>

/*	Static variables						*/

static	load_shed_threshold_t	Threshold = {0};
static	load_shed_current_t	Current   = {0};
static	int		Init_threshold_flag = 0;
static	int		Init_current_flag   = 0;
static	int		Threshold_io_status = 0;
static	int		Current_io_status   = 0;
static	int		ORPGLOAD_registered = 0;

/************************************************************************
 *									*
 *	Description: This function returns the status of the last	*
 *		     Load Shed Category msg I/O operation.		*
 *									*
 *	Input:       The message ID to be read from:			*
 *				LOAD_SHED_THRESHOLD_MSG_ID		*
 *				LOAD_SHED_CURRENT_MSG_ID		*
 *									*
 *	Return: I/O status or ORPGLOAD_DATA_NOT_FOUND if invalid	*
 *		message ID found.					*
 ************************************************************************/

int
ORPGLOAD_io_status (
int	msg_id
)
{
	switch (msg_id) {

	    case LOAD_SHED_THRESHOLD_MSG_ID :

		return (Threshold_io_status);

	    case LOAD_SHED_CURRENT_MSG_ID :

		return (Current_io_status);

	}

	return ORPGLOAD_DATA_NOT_FOUND;
}

/************************************************************************
 *									*
 *	Description: This function returns 0 if the Load Shed msg	*
 *		     needs to be updated, otherwise 1 if it does not	*
 *		     need to be updated and -1 if there has been a	*
 *		     read error.					*
 *									*
 *	Input:       The message ID to be read from:			*
 *				LOAD_SHED_THRESHOLD_MSG_ID		*
 *				LOAD_SHED_CURRENT_MSG_ID		*
 *									*
 ************************************************************************/

int
ORPGLOAD_update_flag (
int	msg_id
)
{
	switch (msg_id) {

	    case LOAD_SHED_THRESHOLD_MSG_ID :

		return (Init_threshold_flag);

	    case LOAD_SHED_CURRENT_MSG_ID :

		return (Init_current_flag);

	}

	return ORPGLOAD_DATA_NOT_FOUND;
}

/************************************************************************
 *	Description: This function sets the appropriate load shed	*
 *		     msg init flag whenever a Load Shed message		*
 *		     update event is received.				*
 *									*
 *	Input: lbfd    - LB file descriptor				*
 *	       msg_id  - ID of message that was updated			*
 *	       msg_len - length of updated message			*
 *	       arg     - pointer to user argument data (unused)		*
 *	Return:	     NONE						*
 *									*
 ************************************************************************/

void
ORPGLOAD_en_status_callback (
int	lbfd,
LB_id_t	msg_id,
int	msg_len,
void	*arg
)
{

	switch (msg_id) {

	    case LOAD_SHED_THRESHOLD_MSG_ID :

		Init_threshold_flag = 0;
		break;

	    case LOAD_SHED_CURRENT_MSG_ID :

		Init_current_flag = 0;
		break;

	}
}

/************************************************************************
 *									*
 *	Description: The following routine reads the Load Shed msg	*
 *		     (LOAD_SHED_x_MSG_ID) from the load shed category	*
 *		     lb (ORPGDAT_LOAD_SHED_CAT).			*
 *									*
 *	Input:       The message ID to be read from:			*
 *				LOAD_SHED_THRESHOLD_MSG_ID		*
 *				LOAD_SHED_CURRENT_MSG_ID		*
 *									*
 *	Return:      On success, the size of the message (in bytes)	*
 *		     is returned.  On failure, a value <= 0 is returned.*
 *									*
 ************************************************************************/

int
ORPGLOAD_read (
int	msg_id
)
{
	int	status;

	status = -1;

/*	Check to see if Load Shed message update events have been	*
 *	registered.  If not, register them.  This code should get	*
 *	executed only once; the first time an ORPG operation is called.	*/

	if (!ORPGLOAD_registered) {

	    ORPGDA_write_permission (ORPGDAT_LOAD_SHED_CAT);

	    status = ORPGDA_UN_register (ORPGDAT_LOAD_SHED_CAT, LB_ANY,
			ORPGLOAD_en_status_callback);

	    if (status < 0) {

	        LE_send_msg (GL_INFO,
		"ORPGLOAD: ORPGDA_UN_register failed (ret %d)\n", status);

	    }

	    ORPGLOAD_registered = 1;

/*	    Since this is the first time through, lets initialize	*
 *	    both the Current and Threshold structures from their	*
 *	    respective LB msgs.						*/

	    Init_threshold_flag = 1;

	    status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT,
		      (char *) &Threshold,
		      sizeof (load_shed_threshold_t),
		      LOAD_SHED_THRESHOLD_MSG_ID);

	    Threshold_io_status = status;

	    if (status < 0) {

		LE_send_msg (GL_INFO,
		    "ORPGDA_read (LOAD_SHED_THRESHOLD_MSG_ID): %d\n", status);

		Init_threshold_flag = -1;

	    }

	    Init_current_flag = 1;

	    status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT,
		      (char *) &Current,
		      sizeof (load_shed_current_t),
		      LOAD_SHED_CURRENT_MSG_ID);

	    Current_io_status = status;

	    if (status < 0) {

		LE_send_msg (GL_INFO,
		    "ORPGDA_read (LOAD_SHED_CURRENT_MSG_ID): %d\n", status);
		Init_current_flag = -1;

	    }

	    return Current_io_status;

	}

/*	This portion of code should get executed after the first	*
 *	call to this function.						*/

	switch (msg_id) {

	    case LOAD_SHED_THRESHOLD_MSG_ID :

		Init_threshold_flag = 1;

		status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT,
			      (char *) &Threshold,
			      sizeof (load_shed_threshold_t),
			      LOAD_SHED_THRESHOLD_MSG_ID);

		Threshold_io_status = status;

		if (status < 0) {

		    LE_send_msg (GL_INFO,
			    "ORPGDA_read (LOAD_SHED_THRESHOLD_MSG_ID): %d\n", status);

		    Init_threshold_flag = -1;

		}

		break;

	    case LOAD_SHED_CURRENT_MSG_ID :

		Init_current_flag = 1;

		status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT,
			      (char *) &Current,
			      sizeof (load_shed_current_t),
			      LOAD_SHED_CURRENT_MSG_ID);

		Current_io_status = status;

		if (status < 0) {

		    LE_send_msg (GL_INFO,
			    "ORPGDA_read (LOAD_SHED_CURRENT_MSG_ID): %d\n", status);
		    Init_current_flag = -1;

		}

		break;

	}

	return status;

}

/************************************************************************
 *									*
 *	Description: The following routine writes the Load Shed Cat msg	*
 *		     (LOAD_SHED_MSG_ID) to the load shed category	*
 *		     lb (ORPGDAT_LOAD_SHED_CAT).			*
 *									*
 *	Input:       The message ID to be read from:			*
 *				LOAD_SHED_THRESHOLD_MSG_ID		*
 *				LOAD_SHED_CURRENT_MSG_ID		*
 *									*
 *	Return:      On success, the size of the message (in bytes)	*
 *		     is returned.  On failure, a value <= 0 is returned.*
 *									*
 ************************************************************************/

int
ORPGLOAD_write (
int	msg_id
)
{
	int	status;
	Orpgevt_load_shed_msg_t	msg;

	msg.msg_id = msg_id;

	switch (msg_id) {

	    case LOAD_SHED_THRESHOLD_MSG_ID :

		status = ORPGDA_write (ORPGDAT_LOAD_SHED_CAT,
					(char *) &Threshold,
					sizeof (load_shed_threshold_t),
					LOAD_SHED_THRESHOLD_MSG_ID);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
		    "ORPGDA_write (LOAD_SHED_THRESHOLD_MSG_ID): %d\n", status);

		}

		break;

	    case LOAD_SHED_CURRENT_MSG_ID :

		status = ORPGDA_write (ORPGDAT_LOAD_SHED_CAT,
					(char *) &Current,
					sizeof (load_shed_current_t),
					LOAD_SHED_CURRENT_MSG_ID);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
		    "ORPGDA_write (LOAD_SHED_CURRENT_MSG_ID): %d\n", status);

		}

		break;
	
	   case LOAD_SHED_THRESHOLD_BASELINE_MSG_ID :

		status = ORPGDA_write (ORPGDAT_LOAD_SHED_CAT,
					(char *) &Threshold,
					sizeof (load_shed_threshold_t),
					LOAD_SHED_THRESHOLD_BASELINE_MSG_ID);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
		    "ORPGDA_write (LOAD_SHED_THRESHOLD_BASELINE_MSG_ID): %d\n", status);

		}

		break;
	
	    default :

		status = -1;
		break;

	}

	if (status >= 0) {

	    status = EN_post (ORPGEVT_LOAD_SHED_CAT,
		     (char *) &msg,
		     sizeof (msg),
		     0);

	    if (status != 0) {

		LE_send_msg (GL_INFO,
			"Unable to post ORPGEVT_LOAD_SHED_CAT event: (ret %d)\n",
			status);

	    }
	}

	return status;

}


/************************************************************************
 *									*
 *	Description: The following function returns the data requested	*
 *		     by the user parameters.				*
 *									*
 *	Input:	     category - load shed category specifier from:	*
 *					LOAD_SHED_CATEGORY_PROD_DIST	*
 *					LOAD_SHED_CATEGORY_PROD_STORAGE	*
 *					LOAD_SHED_CATEGORY_INPUT_BUF	*
 *					LOAD_SHED_CATEGORY_RDA_RADIAL	*
 *					LOAD_SHED_CATEGORY_RPG_RADIAL	*
 *					LOAD_SHED_CATEGORY_WB_USER	*
 *					NOTE: the last 2 are to support	*
 *					RMS only.			*
 *		     type     - data type from:				*
 *					LOAD_SHED_WARNING_THRESHOLD	*
 *					LOAD_SHED_ALARM_THRESHOLD	*
 *					LOAD_SHED_CURRENT_VALUE		*
 *									*
 *	Output:	     value - pointer to variable which the requested	*
 *			     data will be written to (int).		*
 *									*
 *	Return:      On success, 0 is returned.  On failure, the macro	*
 *		     ORPGLOAD_DATA_NOT_FOUND is returned.		*
 *									*
 ************************************************************************/

int
ORPGLOAD_get_data (
int	category,
int	type,
int	*value
)
{

	int	status;

	if (type == LOAD_SHED_CURRENT_VALUE) {

	    if (Init_current_flag != 1) {

		status = ORPGLOAD_read (LOAD_SHED_CURRENT_MSG_ID);

		if (status <= 0) {

		    return (int) ORPGLOAD_DATA_NOT_FOUND;

		}
	    }

	} else {

	    if (Init_threshold_flag != 1) {

		status = ORPGLOAD_read (LOAD_SHED_THRESHOLD_MSG_ID);

		if (status <= 0) {

		    return (int) ORPGLOAD_DATA_NOT_FOUND;

		}
	    }
	}

	switch (category) {

	    case LOAD_SHED_CATEGORY_PROD_DIST :

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			*value = (int) Threshold.prod_dist_warn;
			status = 0;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			*value = (int) Threshold.prod_dist_alarm;
			status = 0;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			*value = (int) Current.prod_dist;
			status = 0;
			break;

		    default :

			*value = 0;
			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_PROD_STORAGE :

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			*value = (int) Threshold.prod_storage_warn;
			status = 0;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			*value = (int) Threshold.prod_storage_alarm;
			status = 0;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			*value = (int) Current.prod_storage;
			status = 0;
			break;

		    default :

			*value = 0;
			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_WB_USER :

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			*value = (int) Threshold.wb_user_warn;
			status = 0;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			*value = (int) Threshold.wb_user_alarm;
			status = 0;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			*value = (int) Current.wb_user;
			status = 0;
			break;

		    default :

			*value = 0;
			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_INPUT_BUF :

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			*value = (int) Threshold.input_buf_warn;
			status = 0;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			*value = (int) Threshold.input_buf_alarm;
			status = 0;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			*value = (int) Current.input_buf;
			status = 0;
			break;

		    default :

			*value = 0;
			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_RDA_RADIAL :

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			*value = (int) Threshold.rda_radial_warn;
			status = 0;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			*value = (int) Threshold.rda_radial_alarm;
			status = 0;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			*value = (int) Current.rda_radial;
			status = 0;
			break;

		    default :

			*value = 0;
			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_RPG_RADIAL :

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			*value = (int) Threshold.rpg_radial_warn;
			status = 0;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			*value = (int) Threshold.rpg_radial_alarm;
			status = 0;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			*value = (int) Current.rpg_radial;
			status = 0;
			break;

		    default :

			*value = 0;
			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    default :

		status = ORPGLOAD_DATA_NOT_FOUND;
		break;

	}

	return status;
}

/************************************************************************
 *									*
 *	Description: The following function sets the data requested	*
 *		     by the user parameters.				*
 *									*
 *	Input:	     category - load shed category specifier from:	*
 *					LOAD_SHED_CATEGORY_PROD_DIST	*
 *					LOAD_SHED_CATEGORY_PROD_STORAGE	*
 *					LOAD_SHED_CATEGORY_INPUT_BUF	*
 *					LOAD_SHED_CATEGORY_RDA_RADIAL	*
 *					LOAD_SHED_CATEGORY_RPG_RADIAL	*
 *					LOAD_SHED_CATEGORY_WB_USER	*
 *					NOTE: the last 2 are to support	*
 *					RMS only.			*
 *		     type     - data type from:				*
 *					LOAD_SHED_WARNING_THRESHOLD	*
 *					LOAD_SHED_ALARM_THRESHOLD	*
 *					LOAD_SHED_CURRENT_VALUE		*
 *		     value    - new data value (0-100).			*
 *									*
 *	Return:      On success, >= 0 is returned.  On failure, a	*
 *		     negative value is returned.			*
 *									*
 ************************************************************************/

int
ORPGLOAD_set_data (
int	category,
int	type,
int	value
)
{
	int	lbd;
	int	status = 0;
	int	alarm_status;
	int	lock_status;
	int	update_flag = 0;
	unsigned char	state;

/*	Get the file handle for the load shed LB.  If the file is 	*
 *	already open for "read-only", the open will fail so we will	*
 *	want to close the LB and reopen it for "write".	  If the file	*
 *	is already open with the correct access permsission, this 	*
 *	call will not cause the LB to be opened.			*/

	if( (lbd = ORPGDA_open(ORPGDAT_LOAD_SHED_CAT, LB_WRITE)) < 0 ){

	    LE_send_msg( GL_ERROR, 
		"ORPGDA_open( ORPGDAT_LOAD_SHED_CAT ) Failed (%d)\n", lbd );
	    ORPGDA_close( ORPGDAT_LOAD_SHED_CAT );
	    lbd = ORPGDA_open(ORPGDAT_LOAD_SHED_CAT, LB_WRITE);
	    if( lbd < 0 )
		return( lbd );

	}

/*	We should block all other consumers access to the current load	*
 *	shed message since we may need to set warning/alarm flags.	*/

	lock_status = LB_lock (lbd,
		      LB_EXCLUSIVE_LOCK | LB_BLOCK,
		      LOAD_SHED_CURRENT_MSG_ID);

/*	Our attempt to lock the message failed so do nothing and	*
 *	return.								*/

	if (lock_status != LB_SUCCESS) {

	    return lock_status;

	}

/*	If the warning/alarm threshold data has changed, update		*
 *	it first before proceding.					*/

	if (Init_threshold_flag != 1) {

/*	    We should block all other consumers access to the threshold	*
 *	    load shed message since we may need to set warning/alarm	*
 *	    flags.							*/

	    lock_status = LB_lock (lbd,
		      LB_EXCLUSIVE_LOCK | LB_BLOCK,
		      LOAD_SHED_THRESHOLD_MSG_ID);

	    status = ORPGLOAD_read (LOAD_SHED_THRESHOLD_MSG_ID);

	    lock_status = LB_lock (lbd,
			      LB_UNLOCK,
			      LOAD_SHED_THRESHOLD_MSG_ID);

	}

/*	If the current load shed utilization message has changed, read	*
 *	it before proceding.						*/

	if (Init_current_flag != 1) {

	    status = ORPGLOAD_read (LOAD_SHED_CURRENT_MSG_ID);

	    if (status < 0) {

		lock_status = LB_lock (lbd,
			      LB_UNLOCK,
			      LOAD_SHED_CURRENT_MSG_ID);

		return (int) status;

	    }
	}

/*	For the input category, set the new value.		*/

	switch (category) {

	    case LOAD_SHED_CATEGORY_PROD_DIST :

		status = 0;

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			Threshold.prod_dist_warn = (char) value;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			Threshold.prod_dist_alarm = (char) value;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			Current.prod_dist = (char) value;
			break;

		    default :

			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_PROD_STORAGE :

		status = 0;

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			Threshold.prod_storage_warn = (char) value;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			Threshold.prod_storage_alarm = (char) value;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			Current.prod_storage = (char) value;
			break;

		    default :

			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_INPUT_BUF :

		status = 0;

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			Threshold.input_buf_warn = (char) value;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			Threshold.input_buf_alarm = (char) value;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			Current.input_buf = (char) value;
			break;

		    default :

			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_RDA_RADIAL :

		status = 0;

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			Threshold.rda_radial_warn = (char) value;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			Threshold.rda_radial_alarm = (char) value;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			Current.rda_radial = (char) value;
			break;

		    default :

			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_RPG_RADIAL :

		status = 0;

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			Threshold.rpg_radial_warn = (char) value;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			Threshold.rpg_radial_alarm = (char) value;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			Current.rpg_radial = (char) value;
			break;

		    default :

			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	    case LOAD_SHED_CATEGORY_WB_USER :

		status = 0;

		switch (type) {

		    case LOAD_SHED_WARNING_THRESHOLD :

			Threshold.wb_user_warn = (char) value;
			break;

		    case LOAD_SHED_ALARM_THRESHOLD :

			Threshold.wb_user_alarm = (char) value;
			break;

		    case LOAD_SHED_CURRENT_VALUE :

			Current.wb_user = (char) value;
			break;

		    default :

			status = ORPGLOAD_DATA_NOT_FOUND;
			break;

		}

		break;

	}

	update_flag = 0;

/*	Check each of the load shed categories to see if a warning or	*
 *	alarm threshold crossed.  If so, we need to generate a syslog	*
 *	message and possibly set an RPG alarm bit.			*/

/*	LOAD_SHED_CATEGORY_PROD_DIST	*/

	if (Current.prod_dist >= Threshold.prod_dist_warn) {

/*	    If the current level reached the warning	*
 *	    threshold and it wasn't above it before	*
 *	    set the warning flag and generate a system	*
 *	    message.					*/

	    if (!(Current.prod_dist_state & LOAD_SHED_WARNING_ACTIVE)) {

		Current.prod_dist_state = Current.prod_dist_state |
					  LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING ACTIVATED: PRODUCT DISTRIBUTION LOAD SHED");

	    }

	} else {

/*	    If the current level is below the warning	*
 *	    threshold and is was previously at the	*
 *	    warning threshold, clear the warning flag	*
 *	    and generate a system message.		*/

	    if (Current.prod_dist_state & LOAD_SHED_WARNING_ACTIVE) {

		Current.prod_dist_state = Current.prod_dist_state &
					  ~LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING CLEARED: PRODUCT DISTRIBUTION LOAD SHED");

	    }
	}

	if (Current.prod_dist >= Threshold.prod_dist_alarm) {

/*	    If the current level reached the alarm	*
 *	    threshold and it wasn't above it before	*
 *	    set the alarm flag and generate a system	*
 *	    message.					*/

	    if (!(Current.prod_dist_state & LOAD_SHED_ALARM_ACTIVE)) {

		Current.prod_dist_state = Current.prod_dist_state |
					  LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS | LE_RPG_AL_LS,
		"RPG ALARM ACTIVATED: PRODUCT DISTRIBUTION LOAD SHED");

	    }

	} else {

/*	    If the current level is below the alarm	*
 *	    threshold and is was previously at the	*
 *	    alarm threshold, clear the alarm flag	*
 *	    and generate a system message.		*/

	    if (Current.prod_dist_state & LOAD_SHED_ALARM_ACTIVE) {

		Current.prod_dist_state = Current.prod_dist_state &
					  ~LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS | LE_RPG_AL_LS | LE_RPG_AL_CLEARED,
		"RPG ALARM CLEARED: PRODUCT DISTRIBUTION LOAD SHED");

	    }
	}

/*	LOAD_SHED_CATEGORY_PROD_STORAGE	*/

	if (Current.prod_storage >= Threshold.prod_storage_warn) {

/*	    If the current level reached the warning	*
 *	    threshold and it wasn't above it before	*
 *	    set the warning flag and generate a system	*
 *	    message.					*/

	    if (!(Current.prod_storage_state & LOAD_SHED_WARNING_ACTIVE)) {

		Current.prod_storage_state = Current.prod_storage_state |
					  LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING ACTIVATED: PRODUCT STORAGE LOAD SHED");

	    }

	} else {

/*	    If the current level is below the warning	*
 *	    threshold and is was previously at the	*
 *	    warning threshold, clear the warning flag	*
 *	    and generate a system message.		*/

	    if (Current.prod_storage_state & LOAD_SHED_WARNING_ACTIVE) {

		Current.prod_storage_state = Current.prod_storage_state &
					  ~LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING CLEARED: PRODUCT STORAGE LOAD SHED");

	    }
	}

	if (Current.prod_storage >= Threshold.prod_storage_alarm) {

/*	    If the current level reached the alarm	*
 *	    threshold and it wasn't above it before	*
 *	    set the alarm flag and generate a system	*
 *	    message.					*/

	    if (!(Current.prod_storage_state & LOAD_SHED_ALARM_ACTIVE)) {

		Current.prod_storage_state = Current.prod_storage_state |
					  LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_PRDSTGLS,
				ORPGINFO_STATEFL_SET,
				&state);

	    }

	} else {

/*	    If the current level is below the alarm	*
 *	    threshold and is was previously at the	*
 *	    alarm threshold, clear the alarm flag	*
 *	    and generate a system message.		*/

	    if (Current.prod_storage_state & LOAD_SHED_ALARM_ACTIVE) {

		Current.prod_storage_state = Current.prod_storage_state &
					  ~LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_PRDSTGLS,
				ORPGINFO_STATEFL_CLR,
				&state);

	    }
	}

/*	LOAD_SHED_CATEGORY_WB_USER	*/

	if (Current.wb_user >= Threshold.wb_user_warn) {

/*	    If the current level reached the warning	*
 *	    threshold and it wasn't above it before	*
 *	    set the warning flag and generate a system	*
 *	    message.					*/

	    if (!(Current.wb_user_state & LOAD_SHED_WARNING_ACTIVE)) {

		Current.wb_user_state = Current.wb_user_state |
					  LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING ACTIVATED: WIDEBAND USER LOAD SHED");

	    }

	} else {

/*	    If the current level is below the warning	*
 *	    threshold and is was previously at the	*
 *	    warning threshold, clear the warning flag	*
 *	    and generate a system message.		*/

	    if (Current.wb_user_state & LOAD_SHED_WARNING_ACTIVE) {

		Current.wb_user_state = Current.wb_user_state &
					  ~LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING CLEARED: WIDEBAND USER LOAD SHED");

	    }
	}

	if (Current.wb_user >= Threshold.wb_user_alarm) {

/*	    If the current level reached the alarm	*
 *	    threshold and it wasn't above it before	*
 *	    set the alarm flag and generate a system	*
 *	    message.					*/

	    if (!(Current.wb_user_state & LOAD_SHED_ALARM_ACTIVE)) {

		Current.wb_user_state = Current.wb_user_state |
					  LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS | LE_RPG_AL_LS,
		"RPG ALARM ACTIVATED: WIDEBAND USER LOAD SHED");

	    }

	} else {

/*	    If the current level is below the alarm	*
 *	    threshold and is was previously at the	*
 *	    alarm threshold, clear the alarm flag	*
 *	    and generate a system message.		*/

	    if (Current.wb_user_state & LOAD_SHED_ALARM_ACTIVE) {

		Current.wb_user_state = Current.wb_user_state &
					  ~LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS | LE_RPG_AL_LS | LE_RPG_AL_CLEARED,
		"RPG ALARM CLEARED: WIDEBAND USER LOAD SHED");

	    }
	}

/*	LOAD_SHED_CATEGORY_RDA_RADIAL	*/

	if (Current.rda_radial >= Threshold.rda_radial_warn) {

/*	    If the current level reached the warning	*
 *	    threshold and it wasn't above it before	*
 *	    set the warning flag and generate a system	*
 *	    message.					*/

	     if (!(Current.rda_radial_state & LOAD_SHED_WARNING_ACTIVE)) {

		Current.rda_radial_state = Current.rda_radial_state |
					  LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING ACTIVATED: RDA RADIAL LOAD SHED");

	    }

	} else {

/*	    If the current level is below the warning	*
 *	    threshold and is was previously at the	*
 *	    warning threshold, clear the warning flag	*
 *	    and generate a system message.		*/

	    if (Current.rda_radial_state & LOAD_SHED_WARNING_ACTIVE) {

		Current.rda_radial_state = Current.rda_radial_state &
					  ~LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING CLEARED: RDA RADIAL LOAD SHED");

	    }
	}

	if (Current.rda_radial >= Threshold.rda_radial_alarm) {

/*	    If the current level reached the alarm	*
 *	    threshold and it wasn't above it before	*
 *	    set the alarm flag and generate a system	*
 *	    message.					*/

	    if (!(Current.rda_radial_state & LOAD_SHED_ALARM_ACTIVE)) {

		Current.rda_radial_state = Current.rda_radial_state |
					  LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_RDAINLS,
				ORPGINFO_STATEFL_SET,
				&state);

	    }

	} else {

/*	    If the current level is below the alarm	*
 *	    threshold and is was previously at the	*
 *	    alarm threshold, clear the alarm flag	*
 *	    and generate a system message.		*/

	    if (Current.rda_radial_state & LOAD_SHED_ALARM_ACTIVE) {

		Current.rda_radial_state = Current.rda_radial_state &
					  ~LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_RDAINLS,
				ORPGINFO_STATEFL_CLR,
				&state);

	    }
	}

/*	LOAD_SHED_CATEGORY_RPG_RADIAL	*/

	if (Current.rpg_radial >= Threshold.rpg_radial_warn) {

/*	    If the current level reached the warning	*
 *	    threshold and it wasn't above it before	*
 *	    set the warning flag and generate a system	*
 *	    message.					*/

	    if (!(Current.rpg_radial_state & LOAD_SHED_WARNING_ACTIVE)) {

		Current.rpg_radial_state = Current.rpg_radial_state |
					  LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING ACTIVATED: RPG RADIAL LOAD SHED");

	    }

	} else {

/*	    If the current level is below the warning	*
 *	    threshold and is was previously at the	*
 *	    warning threshold, clear the warning flag	*
 *	    and generate a system message.		*/

	    if (Current.rpg_radial_state & LOAD_SHED_WARNING_ACTIVE) {

		Current.rpg_radial_state = Current.rpg_radial_state &
					  ~LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

		LE_send_msg (GL_STATUS,
		"RPG WARNING CLEARED: RPG RADIAL LOAD SHED");

	    }
	}

	if (Current.rpg_radial >= Threshold.rpg_radial_alarm) {

/*	    If the current level reached the alarm	*
 *	    threshold and it wasn't above it before	*
 *	    set the alarm flag and generate a system	*
 *	    message.					*/

	    if (!(Current.rpg_radial_state & LOAD_SHED_ALARM_ACTIVE)) {

		Current.rpg_radial_state = Current.rpg_radial_state |
					  LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_RPGINLS,
				ORPGINFO_STATEFL_SET,
				&state);

	    }

	} else {

/*	    If the current level is below the alarm	*
 *	    threshold and is was previously at the	*
 *	    alarm threshold, clear the alarm flag	*
 *	    and generate a system message.		*/

	    if (Current.rpg_radial_state & LOAD_SHED_ALARM_ACTIVE) {

		Current.rpg_radial_state = Current.rpg_radial_state &
					  ~LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_RPGINLS,
				ORPGINFO_STATEFL_CLR,
				&state);

	    }
	}

/*	LOAD_SHED_CATEGORY_INPUT_BUF	*/

/*	Since the input buffer category is a meld of the RDA radial	*
 *	and RPG radial categories, we need to check the states of	*
 *	those categories against the input buffer category.		*/

	if ((Current.rda_radial_state & LOAD_SHED_WARNING_ACTIVE) ||
	    (Current.rpg_radial_state & LOAD_SHED_WARNING_ACTIVE)) {

	    if (!(Current.input_buf_state & LOAD_SHED_WARNING_ACTIVE)) {

		Current.input_buf_state = Current.input_buf_state |
					  LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

	    }

	} else {

	    if (Current.input_buf_state & LOAD_SHED_WARNING_ACTIVE) {

		Current.input_buf_state = Current.input_buf_state &
					  ~LOAD_SHED_WARNING_ACTIVE;
		update_flag = 1;

	    }
	}

	if ((Current.rda_radial_state & LOAD_SHED_ALARM_ACTIVE) ||
	    (Current.rpg_radial_state & LOAD_SHED_ALARM_ACTIVE)) {

	    if (!(Current.input_buf_state & LOAD_SHED_ALARM_ACTIVE)) {

		Current.input_buf_state = Current.input_buf_state |
					  LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_WBDLS,
				ORPGINFO_STATEFL_SET,
				&state);

	    }

	} else {

	    if (Current.input_buf_state & LOAD_SHED_ALARM_ACTIVE) {

		Current.input_buf_state = Current.input_buf_state &
					  ~LOAD_SHED_ALARM_ACTIVE;
		update_flag = 1;

		alarm_status = ORPGINFO_statefl_rpg_alarm (
				ORPGINFO_STATEFL_RPGALRM_WBDLS,
				ORPGINFO_STATEFL_CLR,
				&state);

	    }
	}

/*	If any of the state flags were updated, update the current	*
 *	load shed utilization message so we can shere the states with	*
 *	other consumers.						*/

	if (update_flag || (type == LOAD_SHED_CURRENT_VALUE)) {

	    ORPGLOAD_write (LOAD_SHED_CURRENT_MSG_ID);

	}

	lock_status = LB_lock (lbd,
			      LB_UNLOCK,
			      LOAD_SHED_CURRENT_MSG_ID);

	return status;
}
