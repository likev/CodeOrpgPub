/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2009/05/07 14:01:18 $
 * $Id: orpgred.c,v 1.21 2009/05/07 14:01:18 garyg Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgred.c

 Description: liborpg.a RED(redundant orpg command/data access) module 
 	      c file.

       Notes: 
 **************************************************************************/

#include <orpgerr.h>
#include <orpgda.h>
#include <orpgred.h>
#include <orpgdat.h>
#include <orpgsite.h>

/*	Static variable definitions.		*/

int	Init_local_status_flag 	= 0;	/* 0 = local channel status msg
					       needs to be read.
					   1 = local channel status msg
					       doesn't need to be read. */
int	Init_red_status_flag	= 0;	/* 0 = redundant channel status msg
					       needs to be read.
					   1 = redundant channel status msg
					       doesn't need to be read. */
static	int	Orpgred_local_status_reg; /* 0 = local channel status msg
					       event not registered.
					   1 = local channel status msg
					       event registered. */
static	int	Orpgred_red_status_reg; /* 0 = redundant channel status msg
					       event not registered.
					   1 = redundant channel status msg
					       event registered. */

static	Channel_status_t	Local_status;
static	Channel_status_t	Red_status;

static	Orpgred_io_status_t	Local_io_status = 0;
static	Orpgred_io_status_t	Red_io_status   = 0;

/*
* Static Function Prototypes
*/


/**************************************************************************
 *	Description: This routine sets the local channel status update	  *
 *		     flag whenever the local channel status message has	  *
 *		     been updated (via LB_notify).			  *
 **************************************************************************/

void ORPGRED_en_local_status_callback (int fd, LB_id_t msgid, int msg_info, 
                                       void *arg)
{
	Init_local_status_flag = 0;
}


/****************************************************************************
 *	Description: This routine sets the redundant channel status update  *
 *		     flag whenever the redundant channel status message has *
 *		     been updated (via LB_notify).			    *
 ****************************************************************************/

void ORPGRED_en_red_status_callback (int fd, LB_id_t msgid, int	msg_info,
                                     void *arg)
{
	Init_red_status_flag = 0;
}

 
/**************************************************************************
 *	Description: This function returns the status of the last 	  *
 *		     local/redundant channel status msg I/O operation.	  *
 *									  * 
 *	Input:	id - channel identifier:				  *
 *				ORPGRED_MY_CHANNEL or			  *
 *				ORPGRED_OTHER_CHANNEL			  *
 **************************************************************************/

Orpgred_io_status_t ORPGRED_io_status (Orpgred_chan_t id)
{
	switch (id) {

	    case ORPGRED_MY_CHANNEL :

		return Local_io_status;

	    case ORPGRED_OTHER_CHANNEL :

		return Red_io_status;

	}

	return Local_io_status;

}


/***************************************************************************
 *	Description: The following routine reads the indicated channel     *
 *		     status message (ORPGRED_CHANNEL_STATUS_MSG) to the    *
 *		     LB (ORPGDAT_REDMGR_CHAN_MSGS).			   *
 *									   *
 *	Input:	     which_channel - the channel status message to be	   *
 *		     written (ORPGRED_MY_CHANNEL or ORPGRED_OTHER_CHANNEL) *
 *									   *
 *	Return:      On success, ORPGRED_IO_NORMAL is returned.  On 	   *
 *		     failure, one of the following is returned.		   *
 *				ORPGRED_IO_ERROR			   *
 *				ORPGRED_IO_INVALID_MSG_ID		   *
 ***************************************************************************/

Orpgred_io_status_t ORPGRED_read_status (Orpgred_chan_t channel)
{
	int	status;

	switch (channel) {

	    case ORPGRED_MY_CHANNEL :

		if (!Orpgred_local_status_reg) {

		    status = ORPGDA_UN_register (ORPGDAT_REDMGR_CHAN_MSGS, 
				ORPGRED_CHANNEL_STATUS_MSG,
				ORPGRED_en_local_status_callback);

		    if (status != LB_SUCCESS) {

			LE_send_msg (GL_INFO,
				"ORPGDA_UN_register (ORPGRED_CHANNEL_STATUS_MSG): %d\n",
				status);

			status = ORPGRED_IO_ERROR;
			Local_io_status = (Orpgred_io_status_t) status;
			break;

		    }

		    Orpgred_local_status_reg = 1;

		}

		Init_local_status_flag = 1;
		status = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS,
				(char *) &Local_status,
				sizeof (Channel_status_t),
				ORPGRED_CHANNEL_STATUS_MSG);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
			"ORPGDA_read (ORPGRED_CHANNEL_STATUS_MSG): %d\n",
			status);
		    status = ORPGRED_IO_ERROR;
		    Init_local_status_flag = 0;

		} else {

		    status = ORPGRED_IO_NORMAL;

		}

		Local_io_status = (Orpgred_io_status_t) status;
		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Orpgred_red_status_reg) {

		    status = ORPGDA_UN_register (ORPGDAT_REDMGR_CHAN_MSGS, 
				ORPGRED_REDUN_CHANL_STATUS_MSG,
				ORPGRED_en_red_status_callback);

		    if (status != LB_SUCCESS) {

			LE_send_msg (GL_INFO,
				"ORPGDA_UN_register (ORPGRED_REDUN_CHANL_STATUS_MSG): %d\n",
				status);

			status = ORPGRED_IO_ERROR;
			Red_io_status = (Orpgred_io_status_t) status;
			break;

		    }

		    Orpgred_red_status_reg = 1;

		}

		Init_red_status_flag = 1;
		status = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS,
				(char *) &Red_status,
				sizeof (Channel_status_t),
				ORPGRED_REDUN_CHANL_STATUS_MSG);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
			"ORPGDA_read (ORPGRED_REDUN_CHANL_STATUS_MSG): %d\n",
			status);
		    status = ORPGRED_IO_ERROR;
		    Init_red_status_flag = 0;

		} else {

		    status = ORPGRED_IO_NORMAL;

		}

		Red_io_status = (Orpgred_io_status_t) status;
		break;

	    default :

		status = ORPGRED_IO_INVALID_MSG_ID;
		break;

	}

	return (Orpgred_io_status_t) status;
}

 
/**************************************************************************
 *	Description: This function returns a pointer to the specified 	  *
 *		     status message.					  *
 *	Input:	id - channel identifier:				  *
 *				ORPGRED_MY_CHANNEL or			  *
 *				ORPGRED_OTHER_CHANNEL			  *
 *	Return:      A pointer to the message is returned on success.	  *
 *                   On failure, NULL is returned.                        *
 **************************************************************************/

char *ORPGRED_status_msg (Orpgred_chan_t id)
{
	switch (id) {

	    case ORPGRED_MY_CHANNEL :

		return (char *) &Local_status;

	    case ORPGRED_OTHER_CHANNEL :

		return (char *) &Red_status;

	}

	return (char *) NULL;

}


/***************************************************************************
 *	Description: The following routine writes the indicated channel    *
 *		     status message (ORPGRED_CHANNEL_STATUS_MSG) to the    *
 *		     LB (ORPGDAT_REDMGR_CHAN_MSGS).                        *
 *									   *
 *	Input:	     which_channel - the channel status message to be	   *
 *		     written (ORPGRED_MY_CHANNEL or ORPGRED_OTHER_CHANNEL) *
 *									   *
 *	Return:      On success, ORPGRED_IO_NORMAL is returned.  On 	   *
 *		     failure, one of the following is returned.		   *
 *				ORPGRED_IO_ERROR			   *
 *				ORPGRED_IO_INVALID_MSG_ID		   *
 ***************************************************************************/

Orpgred_io_status_t ORPGRED_write_status (Orpgred_chan_t channel)
{
	int	status;

	switch (channel) {

	    case ORPGRED_MY_CHANNEL :

		status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
				(char *) &Local_status,
				sizeof (Channel_status_t),
				ORPGRED_CHANNEL_STATUS_MSG);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
			"ORPGDA_write (ORPGRED_CHANNEL_STATUS_MSG): %d\n",
			status);
		    status = ORPGRED_IO_ERROR;

		} else {

		    status = ORPGRED_IO_NORMAL;

		}

		break;

	    case ORPGRED_OTHER_CHANNEL :

		status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
				(char *) &Red_status,
				sizeof (Channel_status_t),
				ORPGRED_REDUN_CHANL_STATUS_MSG);

		if (status < 0) {

		    LE_send_msg (GL_INFO,
			"ORPGDA_write (ORPGRED_REDUN_CHANL_STATUS_MSG): %d\n",
			status);
		    status = ORPGRED_IO_ERROR;

		} else {

		    status = ORPGRED_IO_NORMAL;

		}

		break;

	    default :

		status = ORPGRED_IO_INVALID_MSG_ID;
		break;

	}

	return (Orpgred_io_status_t) status;
}


/**************************************************************************

    Description: Returns the name of the RPG host specified

    Inputs:  channel_id - The channel identifier:
                          ORPGRED_MY_CHANNEL
                          ORPGRED_OTHER_CHANNEL

    Outputs:
     Return: The RPG hostname of the channel specified, or NULL is returned
             on error or if the LB msg has not been updated with the 
             redundant channel host name.

**************************************************************************/

char *ORPGRED_get_hostname (Orpgred_chan_t channel_id)
{
   char *ret;
   int   status = ORPGRED_IO_NORMAL;

   switch (channel_id) {

      case ORPGRED_MY_CHANNEL :

         if (!Init_local_status_flag) {
             status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
         }

         ret = Local_status.rpg_hostname;

         if (strlen(ret) == 0)
            ret = NULL;

      break;

      case ORPGRED_OTHER_CHANNEL :

         if (!Init_red_status_flag) {
             status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
         }

         ret = Red_status.rpg_hostname;

         if (strlen(ret) == 0)
            ret = NULL;

      break;

      default :

         ret = NULL;
   }

   if (status != ORPGRED_IO_NORMAL)
      ret = NULL;

   return ret;
}


/**************************************************************************

    Description: Reads the redundant command in the redundant buffer.

    Inputs:  cmd - What command to execute (UPDATE or DOWNLOAD).

    Outputs: msg id is returned on success; ORPGDA_read return code
             is returned on failure.
    
**************************************************************************/
LB_id_t ORPGRED_get_msg (Redundant_cmd_t *cmd)
{
	int ret;
	LB_id_t  redun_msg_id;
		
	ret = ORPGDA_read(ORPGDAT_REDMGR_CMDS, (char*)&cmd, sizeof(Redundant_cmd_t), LB_NEXT);

	if (ret != sizeof(Redundant_cmd_t)){
		if( ret != -38){
			LE_send_msg( GL_LB(ret), "Failed read to redundant LB (%d).\n", ret );
		}
		return (ret);
	}
		
	redun_msg_id = ORPGDA_get_msg_id();
		
	return (redun_msg_id);
}


/**************************************************************************

    Description: Seeks to the specified message in the command LB.

    Inputs: msg_id - What message in the lb to extract.	

    Outputs:	
    
**************************************************************************/
LB_id_t ORPGRED_seek( LB_id_t seek_msg_id)
{
	LB_info info;
	int ret;
	
	ret = ORPGDA_seek (ORPGDAT_REDMGR_CMDS, 0, seek_msg_id, &info);
	
	if(ret < 0){
		LE_send_msg( GL_LB(ret), "Failed to find redundant message (%d).\n", ret );
		return (ret);
	}
		
	return (info.id);
}


/**************************************************************************

    Description: Places the redundant command in the redundant buffer.

    Inputs:  cmd - What command to execute (UPDATE or DOWNLOAD).

    Outputs:	
    
**************************************************************************/
int ORPGRED_send_msg (Redundant_cmd_t cmd)
{
   int ret;
	
   ret = ORPGDA_write(ORPGDAT_REDMGR_CMDS, (char*)&cmd, sizeof(Redundant_cmd_t), LB_ANY);

   if(ret != sizeof(Redundant_cmd_t)){
      LE_send_msg( GL_LB(ret), "Failed write to redundant LB (%d).\n", ret );
                   return (ret);
   }
		
   return (0);

} 


/**************************************************************************

    Description: Returns the time the Adaptation Data was last updated

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns the time in time_t of the last update time (a time
	     of 0 means that the Adaptation Data has not been updated
	     since the software was installed on the system).
	     On failure, -1 is returned.

**************************************************************************/
time_t ORPGRED_adapt_dat_time (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.adapt_dat_update_time;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.adapt_dat_update_time;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the RPG channel number.

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns an integer corresponding to the RPG channel number
	     on success.  On failure, -1 is returned.

**************************************************************************/
int ORPGRED_channel_num (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rpg_channel_number;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rpg_channel_number;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;
 
	return ret;
}


/**************************************************************************

    Description: Returns the rpg channel state.

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the following:
		ORPGRED_CHANNEL_ACTIVE
		ORPGRED_CHANNEL_INACTIVE
	     On failure, -1 is returned.

**************************************************************************/
int ORPGRED_channel_state (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rpg_channel_state;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rpg_channel_state;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the NB comms relay state

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the comms relay states defined in orpgred.h
	     On failure, -1 is returned.

**************************************************************************/
int ORPGRED_comms_relay_state (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.comms_relay_state;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.comms_relay_state;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the rda control state.

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the following:
        ORPGRED_RDA_CONTROLLING
        ORPGRED_RDA_NON_CONTROLLING
        ORPGRED_RDA_CONTROL_STATE_UNKNOWN
        On failure, -1 is returned.

**************************************************************************/
int ORPGRED_rda_control_status (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rda_control_state;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rda_control_state;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the rda operability status.

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the RDA states defined in rda_status.h
	     On failure, -1 is returned.

**************************************************************************/
int ORPGRED_rda_op_state (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rda_operability_status;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rda_operability_status;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the rda status (ie. start-up, standby, etc.)

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the RDA states defined in rda_status.h
	     On failure, -1 is returned.

**************************************************************************/
int ORPGRED_rda_status (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rda_status;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rda_status;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the rpg operability mode (test, maintenance, etc.)

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the RPG states defined in mrpg.h
	     On failure, -1 is returned.

**************************************************************************/
int ORPGRED_rpg_mode (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rpg_mode;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rpg_mode;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the rpg state (operate, standby, etc.)

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the RPG states defined in mrpg.h
	     On failure, -1 is returned.

**************************************************************************/
int ORPGRED_rpg_state (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rpg_state;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rpg_state;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}


/**************************************************************************

    Description: Returns the rpg/rpg link state.

    Outputs: Returns one of the following:
		ORPGRED_CHANNEL_LINK_UP
		ORPGRED_CHANNEL_LINK_DOWN
                -1 on failure

**************************************************************************/
int ORPGRED_rpg_rpg_link_state (void)
{
        int     ret;
	int	status = ORPGRED_IO_NORMAL;

	if (!Init_local_status_flag) {

	    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);

	}

        if (status == ORPGRED_IO_NORMAL)
            ret = Local_status.rpg_rpg_link_state;
        else
            ret = -1;

	return ret;
}

/**************************************************************************

    Description: Returns a 0 or 1 state depending on whether the RPG
                 application software and the adaptation data version 
                 numbers match between the channels

         Inputs:  

        Outputs: Returns 1 if both the RPG apps version number and the 
                 adaptation data version number match; otherwise, 0 is
                 returned

**************************************************************************/

int ORPGRED_version_numbers_match (void) {
   int ret = 0;
   int status = ORPGRED_IO_NORMAL;

   if (!Init_local_status_flag)
      status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);

   if ((status == ORPGRED_IO_NORMAL) && (!Init_red_status_flag))
      status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);

   if (status == ORPGRED_IO_NORMAL) {
     if ((Local_status.rpg_sw_version_num == Red_status.rpg_sw_version_num) &&
         (Local_status.adapt_data_version_num == Red_status.adapt_data_version_num))
         ret = 1;
     else
         ret = 0;
   }
   else
      ret = 0;

   return ret;
}


/**************************************************************************

    Description: This routine updates the Adaptation Data Update Time stamp
                 in the local channel status message

         Inputs: update_time - the new time stamp to update the "Adaptation
                               Data Time" field in the status msg with

        Outputs: 0 on success; -1 on failure

**************************************************************************/
int ORPGRED_update_adapt_dat_time (time_t update_time)
{
   int ret    = 0;
   int status = ORPGRED_IO_NORMAL;


   if (!Init_local_status_flag) 
      status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);

   if (status != ORPGRED_IO_NORMAL)
      ret = -1;
   else
   {
      Local_status.adapt_dat_update_time = update_time;

      status = ORPGRED_write_status (ORPGRED_MY_CHANNEL);

      if (status != ORPGRED_IO_NORMAL)
         ret = -1;
   }

   return ret;
}


/**************************************************************************

    Description: Returns the wideband link state.

    Inputs:  channel_id - The channel identifier:
		ORPGRED_MY_CHANNEL
		ORPGRED_OTHER_CHANNEL

    Outputs: Returns one of the following:
		ORPGRED_CHANNEL_LINK_UP
		ORPGRED_CHANNEL_LINK_DOWN
                -1 on failure

**************************************************************************/
int ORPGRED_wb_link_state (Orpgred_chan_t channel_id)
{
	int	ret;
	int	status = ORPGRED_IO_NORMAL;

	switch (channel_id) {

	    case ORPGRED_MY_CHANNEL :

		if (!Init_local_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_MY_CHANNEL);
		}

		ret = Local_status.rda_rpg_wb_link_state;

		break;

	    case ORPGRED_OTHER_CHANNEL :

		if (!Init_red_status_flag) {

		    status = ORPGRED_read_status (ORPGRED_OTHER_CHANNEL);
		}

		ret = Red_status.rda_rpg_wb_link_state;

		break;

	    default :

		ret = -1;

	}

        if (status != ORPGRED_IO_NORMAL)
            ret = -1;

	return ret;
}
