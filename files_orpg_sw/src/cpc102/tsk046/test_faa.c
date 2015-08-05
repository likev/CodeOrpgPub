/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/11 18:03:24 $
 * $Id: test_faa.c,v 1.11 2005/08/11 18:03:24 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
*/


/************************************************************************
 * The purpose of this tool is to create the redundant manager data	*
 * stores (if they do not exist) and allow one to set the various	*
 * status elements.  The primary user for this tool is the HCI so that	*
 * various redundant configurations can be tested.			*
 ************************************************************************/

/*
 * RCS Author
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/11 18:03:24 $
 * $Id: test_faa.c,v 1.11 2005/08/11 18:03:24 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <strings.h>

#include <infr.h>
#include <orpg.h>
#include <orpgdat.h>
#include <orpgred.h>
#include <orpgsite.h>
#include <mrpg.h>

Channel_status_t	Local;	/* local channel status buffer */
Channel_status_t	Red;	/* redundant channel status buffer */

/************************************************************************
 *  Description: This is the main module for the FAA redundant test	*
 *		 tool.							*
 *									*
 *  Input: argc - number of command line arguments			*
 *	   argv - pointer to command line argument data			*
 ************************************************************************/

int
main (
int	argc,
char	*argv[]
)
{
	char	name [NAME_SIZE];
	static LB_attr attr;
	int	status;
	int	selection;
	int	num;
	int	month, day, year;
	int	hour, minute, second;

/*	Read the local redundant status message.  If the LB or message	*
 *	does not exist, then create it.					*/

	status = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS,
			      (char *) &Local,
			      sizeof (Channel_status_t),
			      ORPGRED_CHANNEL_STATUS_MSG);

	if (status == LB_OPEN_FAILED) {

	    status = CS_entry ((char *)ORPGDAT_REDMGR_CHAN_MSGS,
			CS_INT_KEY | 1, NAME_SIZE, name);

	    if (status < 0) {

		fprintf (stderr,"CS_entry failed: %d\n", status);
		exit (1);

	    }

	    strncpy (attr.remark, "ORPGDAT_REDMGR_CHAN_MSGS", LB_REMARK_LENGTH);
	    attr.remark [LB_REMARK_LENGTH - 1] = '\0';
	    attr.mode = 0666;
	    attr.msg_size = 0;
	    attr.maxn_msgs = 3;
	    attr.types = LB_FILE | LB_REPLACE;
	    attr.tag_size = 30 << NRA_SIZE_SHIFT;

/*	    Create the LB.						*/

	    status = LB_open (name, LB_WRITE | LB_CREATE, &attr);

/*	    If the creation failed, then exit.				*/

	    if (status < 0) {

		fprintf (stderr,"LB_open failed: %d\n", status);
		exit (1);

	    }

/*	    Close the LB.  We will use ORPGDA to reopen it.		*/

	    LB_close (status);

/*	    Set the redundant status data elements to some defaults.	*/

	    Local.rpg_configuration     = FAA_REDUNDANT;
	    Local.rpg_channel_number    = 2;
	    Local.rpg_channel_state     = ORPGRED_CHANNEL_ACTIVE;
	    Local.rda_rpg_wb_link_state = ORPGRED_CHANNEL_LINK_UP;
	    Local.rda_operability_status = OS_ONLINE;
	    Local.rda_control_state     = RDA_IS_CONTROLLING;
	    Local.rpg_state             = MRPG_ST_OPERATING;
	    Local.rpg_mode              = MRPG_TM_NONE;
	    Local.comms_relay_state     = ORPGRED_COMMS_RELAY_ASSIGNED;
	    Local.rpg_rpg_link_state    = ORPGRED_CHANNEL_LINK_UP;
	    Local.adapt_dat_update_time = time (NULL);
	    Local.stat_msg_seq_num      = 0;

	    Red.rpg_configuration     = FAA_REDUNDANT;
	    Red.rpg_channel_number    = 1;
	    Red.rpg_channel_state     = ORPGRED_CHANNEL_INACTIVE;
	    Red.rda_rpg_wb_link_state = ORPGRED_CHANNEL_LINK_UP;
	    Red.rda_operability_status = OS_ONLINE;
	    Red.rda_control_state     = RDA_IS_NON_CONTROLLING;
	    Red.rpg_state             = MRPG_ST_OPERATING;
	    Red.rpg_mode              = MRPG_TM_NONE;
	    Red.comms_relay_state     = ORPGRED_COMMS_RELAY_UNASSIGNED;
	    Red.rpg_rpg_link_state    = ORPGRED_CHANNEL_LINK_UP;
	    Red.adapt_dat_update_time = time (NULL);
	    Red.stat_msg_seq_num      = 0;

/*	    Write out the local channel status data.			*/

	    status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
			(char *) &Local,
			sizeof (Channel_status_t),
			ORPGRED_CHANNEL_STATUS_MSG);

	    if (status <= 0) {

		fprintf (stderr,"Error writing channel status msg: %d\n",
			status);

		exit (1);

	    }

/*	    Write out the redundant channel status data.		*/

	    status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
			(char *) &Red,
			sizeof (Channel_status_t),
			ORPGRED_REDUN_CHANL_STATUS_MSG);

	    if (status <= 0) {

		fprintf (stderr,"Error writing redundant channel status msg: %d\n",
			status);

		exit (1);

	    }

/*	The FAA redundant LB exists so try to read its contents.	*/

	} else {

/*	    Read the local channel status message.			*/

	    status = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS,
		(char *) &Local,
		sizeof (Channel_status_t),
		ORPGRED_CHANNEL_STATUS_MSG);

/*	    If an error occurred because the message doesn't exist, set	*
 *	    the status elements to default values and save them.	*/

	    if (status <= 0) {

		if (status == LB_NOT_FOUND) {

		    Local.rpg_configuration     = FAA_REDUNDANT;
		    Local.rpg_channel_number    = 2;
		    Local.rpg_channel_state     = ORPGRED_CHANNEL_ACTIVE;
		    Local.rda_rpg_wb_link_state = ORPGRED_CHANNEL_LINK_UP;
		    Local.rda_operability_status = OS_ONLINE;
		    Local.rda_control_state     = RDA_IS_CONTROLLING;
		    Local.rpg_state             = MRPG_ST_OPERATING;
		    Local.rpg_mode              = MRPG_TM_NONE;
		    Local.comms_relay_state     = ORPGRED_COMMS_RELAY_ASSIGNED;
		    Local.rpg_rpg_link_state    = ORPGRED_CHANNEL_LINK_UP;
		    Local.adapt_dat_update_time = time (NULL);
		    Local.stat_msg_seq_num      = 0;

		    Red.rpg_configuration     = FAA_REDUNDANT;
		    Red.rpg_channel_number    = 1;
		    Red.rpg_channel_state     = ORPGRED_CHANNEL_INACTIVE;
		    Red.rda_rpg_wb_link_state = ORPGRED_CHANNEL_LINK_UP;
		    Red.rda_operability_status = OS_ONLINE;
		    Red.rda_control_state     = RDA_IS_NON_CONTROLLING;
		    Red.rpg_state             = MRPG_ST_OPERATING;
		    Red.rpg_mode              = MRPG_TM_NONE;
		    Red.comms_relay_state     = ORPGRED_COMMS_RELAY_UNASSIGNED;
		    Red.rpg_rpg_link_state    = ORPGRED_CHANNEL_LINK_UP;
		    Red.adapt_dat_update_time = time (NULL);
		    Red.stat_msg_seq_num      = 0;

		    status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
			(char *) &Local,
			sizeof (Channel_status_t),
			ORPGRED_CHANNEL_STATUS_MSG);

		    if (status <= 0) {

			fprintf (stderr,"Error writing channel status msg: %d\n",
				status);

			exit (1);

		    }

/*		If some other kind of read error uccurred, exit.	*/

		} else {

		    fprintf (stderr,"Error reading channel status msg: %d\n",
			status);

		    exit (1);

		}
	    }

/*	    Read the redundant channel status message.			*/

	    status = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS,
		(char *) &Red,
		sizeof (Channel_status_t),
		ORPGRED_REDUN_CHANL_STATUS_MSG);

/*	    If an error occurred because the message doesn't exist, set	*
 *	    the status elements to default values and save them.	*/

	    if (status <= 0) {

		if (status == LB_NOT_FOUND) {

		    Red.rpg_configuration     = FAA_REDUNDANT;
		    Red.rpg_channel_number    = 1;
		    Red.rpg_channel_state     = ORPGRED_CHANNEL_INACTIVE;
		    Red.rda_rpg_wb_link_state = ORPGRED_CHANNEL_LINK_UP;
		    Red.rda_operability_status = OS_ONLINE;
		    Red.rda_control_state     = RDA_IS_NON_CONTROLLING;

		    status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
			(char *) &Red,
			sizeof (Channel_status_t),
			ORPGRED_REDUN_CHANL_STATUS_MSG);

		    if (status <= 0) {

			fprintf (stderr,"Error writing redundant channel status msg: %d\n",
			status);

			exit (1);

		    }

/*		If some other kind of read error uccurred, exit.	*/

		} else {

		    fprintf (stderr,"Error reading redundant channel status msg: %d\n",
			status);

		    exit (1);

		}
	    }
	}

/*	Create a top level text menu and display it in the terminal	*
 *	window.								*/

	selection = 0;

	while (selection >= 0) {

	    fprintf (stderr,"\n-----Redundant Msgs Tool-----\n");
	    fprintf (stderr,"-1 = Exit\n");
	    fprintf (stderr," 0 = Display Local/Redundant Channel Status\n");
	    fprintf (stderr," 1 = Modify Local Channel Status\n");
	    fprintf (stderr," 2 = Update Local Channel Status\n");
	    fprintf (stderr," 3 = Modify Redundant Channel Status\n");
	    fprintf (stderr," 4 = Update Redundant Channel Status\n");
	    fprintf (stderr,"\n");
	    fprintf (stderr,"Enter your selection: ");

/*	    Wait for some input.					*/

	    scanf ("%d", &selection);

/*	    Process the input.						*/

	    switch (selection) {

		case -1 :	/* Exit */

		    break;

	 	case 0 :	/* Display Status */

		    fprintf (stderr,"\n--------Local Channel Status--------\n");
		    fprintf (stderr,"	Configuration:	%d\n", Local.rpg_configuration);
		    fprintf (stderr,"	Channel Number:	%d\n", Local.rpg_channel_number);
		    fprintf (stderr,"	Channel State:	%d\n", Local.rpg_channel_state);
		    fprintf (stderr,"	RPG State:	%d\n", Local.rpg_state);
		    fprintf (stderr,"	RPG Mode:	%d\n", Local.rpg_mode);
		    fprintf (stderr,"	RDA Ctl State	%d\n", Local.rda_control_state);
		    fprintf (stderr,"	RDA Status:	%d\n", Local.rda_status);
		    fprintf (stderr,"	RDA Op Status:	%d\n", Local.rda_operability_status);
		    fprintf (stderr,"	WB Link State:	%d\n", Local.rda_rpg_wb_link_state);
		    fprintf (stderr,"	Comms Relay:	%d\n", Local.comms_relay_state);
		    fprintf (stderr,"	RPG/RPG Link:	%d\n", Local.rpg_rpg_link_state);
		    fprintf (stderr,"	RDA msg seq #:	%d\n", Local.stat_msg_seq_num);
		    if (Local.adapt_dat_update_time > 0) {

			unix_time (&Local.adapt_dat_update_time,
				&year,
				&month,
				&day,
				&hour,
				&minute,
				&second);

		    } else {

			month  = 0;
			day    = 0;
			year   = 0;
			hour   = 0;
			minute = 0;
			second = 0;

		    }

		    fprintf (stderr,"	Adapt time:	%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d\n", month, day, year%100, hour, minute, second);

		    fprintf (stderr,"\n------Redundant Channel Status--------\n");
		    fprintf (stderr,"	Configuration:	%d\n", Red.rpg_configuration);
		    fprintf (stderr,"	Channel Number:	%d\n", Red.rpg_channel_number);
		    fprintf (stderr,"	Channel State:	%d\n", Red.rpg_channel_state);
		    fprintf (stderr,"	RPG State:	%d\n", Red.rpg_state);
		    fprintf (stderr,"	RPG Mode:	%d\n", Red.rpg_mode);
		    fprintf (stderr,"	RDA Ctl State	%d\n", Red.rda_control_state);
		    fprintf (stderr,"	RDA Status:	%d\n", Red.rda_status);
		    fprintf (stderr,"	RDA Op Status:	%d\n", Red.rda_operability_status);
		    fprintf (stderr,"	WB Link State:	%d\n", Red.rda_rpg_wb_link_state);
		    fprintf (stderr,"	Comms Relay:	%d\n", Red.comms_relay_state);
		    fprintf (stderr,"	RPG/RPG Link:	%d\n", Red.rpg_rpg_link_state);
		    fprintf (stderr,"	RDA msg seq #:	%d\n", Red.stat_msg_seq_num);
		    if (Red.adapt_dat_update_time > 0) {

			unix_time (&Red.adapt_dat_update_time,
				&year,
				&month,
				&day,
				&hour,
				&minute,
				&second);

		    } else {

			month  = 0;
			day    = 0;
			year   = 0;
			hour   = 0;
			minute = 0;
			second = 0;

		    }

		    fprintf (stderr,"	Adapt time:	%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d\n", month, day, year%100, hour, minute, second);
		    fprintf (stderr,"\n");
		    break;

		case 1 : /* Modify Local Channel Status */

		    fprintf (stderr,"\n\n-----Modify Local Channel Status-----\n");
		    fprintf (stderr,"1 = RRG Configuration [%d]\n",
					Local.rpg_configuration);
		    fprintf (stderr,"2 = Channel Number [%d]\n",
					Local.rpg_channel_number);
		    fprintf (stderr,"3 = Channel State [%d]\n",
					Local.rpg_channel_state);
		    fprintf (stderr,"4 = RPG State [%d]\n",
					Local.rpg_state);
		    fprintf (stderr,"5 = RPG Mode [%d]\n",
					Local.rpg_mode);
		    fprintf (stderr,"6 = RDA Control State [%d]\n",
					Local.rda_control_state);
		    fprintf (stderr,"7 = RDA Status [%d]\n",
					Local.rda_status);
		    fprintf (stderr,"8 = RDA Operability Status [%d]\n",
					Local.rda_operability_status);
		    fprintf (stderr,"9 = RDA/RPG Wideband Link State [%d]\n",
					Local.rda_rpg_wb_link_state);
		    fprintf (stderr,"10 = Comms Relay State [%d]\n",
					Local.comms_relay_state);
		    fprintf (stderr,"11 = RPG/RPG Link State [%d]\n",
					Local.rpg_rpg_link_state);

		    unix_time (&Local.adapt_dat_update_time,
				&year,
				&month,
				&day,
				&hour,
				&minute,
				&second);

		    fprintf (stderr,"12 = Adaptation Data Update Time [%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d]\n",
					month, day, year, hour, minute, second);
		    fprintf (stderr,"13 = RDA Status Msg Sequence # [%d]\n",
					Local.stat_msg_seq_num);

		    fprintf (stderr,"Enter your selection: ");
		    scanf ("%d", &selection);

		    switch (selection) {

			case 1 : /* RPG Configuration */

			    fprintf (stderr,"-------RPG Configuration-------\n");
			    fprintf (stderr,"	0 = Non-redundant\n");
			    fprintf (stderr,"	1 = FAA redundant\n");
			    fprintf (stderr,"	2 = NWS redundant\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rpg_configuration = num;
			    break;

			case 2 : /* Channel Number */

			    fprintf (stderr,"------Channel Number--------\n");
			    fprintf (stderr,"	0 = Not Redundant\n");
			    fprintf (stderr,"	1 = Channel 1\n");
			    fprintf (stderr,"	2 = Channel 2\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rpg_channel_number = num;
			    break;

			case 3 : /* Channel State */

			    fprintf (stderr,"------Channel State--------\n");
			    fprintf (stderr,"	0 = Inactive\n");
			    fprintf (stderr,"	1 = Active\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rpg_channel_state = num;
			    break;

			case 4 : /* RPG State */

			    fprintf (stderr,"------RPG State--------\n");
			    fprintf (stderr,"	0 = Shutdown\n");
			    fprintf (stderr,"	1 = Standby\n");
			    fprintf (stderr,"	2 = Operating\n");
			    fprintf (stderr,"	3 = Transition\n");
			    fprintf (stderr,"	4 = Failed\n");
			    fprintf (stderr,"	5 = Powerfail\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rpg_state = num;
			    break;

			case 5 : /* RPG Mode */

			    fprintf (stderr,"------RPG Mode--------\n");
			    fprintf (stderr,"	0 = Operate\n");
			    fprintf (stderr,"	1 = Test (RPG)\n");
			    fprintf (stderr,"	2 = Test (RDA)\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rpg_mode = num;
			    break;

			case 6 : /* RDA Control State */

			    fprintf (stderr,"------RDA Control State--------\n");
			    fprintf (stderr,"	0 = Controlling\n");
			    fprintf (stderr,"	1 = Non-controlling\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rda_control_state = num;
			    break;

			case 7 : /* RDA Status */

			    fprintf (stderr,"------RDA Status--------\n");
			    fprintf (stderr,"	 0 = Unknown\n");
			    fprintf (stderr,"	 2 = Startup\n");
			    fprintf (stderr,"	 4 = Standby\n");
			    fprintf (stderr,"	 8 = Restart\n");
			    fprintf (stderr,"	16 = Operate\n");
			    fprintf (stderr,"	32 = Playback\n");
			    fprintf (stderr,"	64 = Offline Operate\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rda_status = num;
			    break;

			case 8 : /* RDA Operability Status */

			    fprintf (stderr,"------RDA Operability Status--------\n");
			    fprintf (stderr,"	  0 = Indeterminate\n");
			    fprintf (stderr,"	  2 = Online\n");
			    fprintf (stderr,"	  4 = Maintenance Required\n");
			    fprintf (stderr,"	  8 = Maintenance Mandatory\n");
			    fprintf (stderr,"	 16 = Commanded Shutdown\n");
			    fprintf (stderr,"	 32 = Inoperable\n");
			    fprintf (stderr,"	128 = Wideband Disconnect\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rda_operability_status = num;
			    break;

			case 9 : /* RDA/RPG WB Link State */

			    fprintf (stderr,"------RDA/RPG WB Link State--------\n");
			    fprintf (stderr,"	1 = Link Up\n");
			    fprintf (stderr,"	2 = Link Down\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rda_rpg_wb_link_state = num;
			    break;

			case 10 : /* Comms Relay State */

			    fprintf (stderr,"------Comms Relay State--------\n");
			    fprintf (stderr,"	0 = Unknown\n");
			    fprintf (stderr,"	1 = Assigned\n");
			    fprintf (stderr,"	2 = Unassigned\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.comms_relay_state = num;
			    break;

			case 11 : /* RPG/RPG Link State */

			    fprintf (stderr,"------RPG/RPG Link State--------\n");
			    fprintf (stderr,"	1 = Link Up\n");
			    fprintf (stderr,"	2 = Link Down\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.rpg_rpg_link_state = num;
			    break;

			case 12 : /* Adaptation Data Update Time */

			    fprintf (stderr,"------RDA/RPG WB Link State--------\n");
			    Local.adapt_dat_update_time = time (NULL);

			    unix_time (&Local.adapt_dat_update_time,
				&year,
				&month,
				&day,
				&hour,
				&minute,
				&second);

			    break;

			case 13 : /* RDA Status Msg Sequence # */

			    fprintf (stderr,"------RDA Status Msg Sequence #--------\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Local.stat_msg_seq_num = num;
			    break;
	
		    }

		    break;

		case 2 : /* Update Local Channel Status */

		    status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
			(char *) &Local,
			sizeof (Channel_status_t),
			ORPGRED_CHANNEL_STATUS_MSG);

		    if (status <= 0) {

			fprintf (stderr,"Error writing channel status msg: %d\n",
				status);

			exit (1);

		    }

		    break;

		case 3 : /* Modify Redundant Channel Status */

		    fprintf (stderr,"\n\n-----Modify Redundant Channel Status-----\n");
		    fprintf (stderr,"1 = RRG Configuration [%d]\n",
					Red.rpg_configuration);
		    fprintf (stderr,"2 = Channel Number [%d]\n",
					Red.rpg_channel_number);
		    fprintf (stderr,"3 = Channel State [%d]\n",
					Red.rpg_channel_state);
		    fprintf (stderr,"4 = RPG State [%d]\n",
					Red.rpg_state);
		    fprintf (stderr,"5 = RPG Mode [%d]\n",
					Red.rpg_mode);
		    fprintf (stderr,"6 = RDA Control State [%d]\n",
					Red.rda_control_state);
		    fprintf (stderr,"7 = RDA Status [%d]\n",
					Red.rda_status);
		    fprintf (stderr,"8 = RDA Operability Status [%d]\n",
					Red.rda_operability_status);
		    fprintf (stderr,"9 = RDA/RPG Wideband Link State [%d]\n",
					Red.rda_rpg_wb_link_state);
		    fprintf (stderr,"10 = Comms Relay State [%d]\n",
					Red.comms_relay_state);
		    fprintf (stderr,"11 = RPG/RPG Link State [%d]\n",
					Red.rpg_rpg_link_state);

		    unix_time (&Red.adapt_dat_update_time,
				&year,
				&month,
				&day,
				&hour,
				&minute,
				&second);

		    fprintf (stderr,"12 = Adaptation Data Update Time [%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d]\n",
					month, day, year, hour, minute, second);
		    fprintf (stderr,"13 = RDA Status Msg Sequence # [%d]\n",
					Red.stat_msg_seq_num);

		    fprintf (stderr,"Enter your selection: ");
		    scanf ("%d", &selection);

		    switch (selection) {

			case 1 : /* RPG Configuration */

			    fprintf (stderr,"-------RPG Configuration-------\n");
			    fprintf (stderr,"	0 = Non-redundant\n");
			    fprintf (stderr,"	1 = FAA redundant\n");
			    fprintf (stderr,"	2 = NWS redundant\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rpg_configuration = num;
			    break;

			case 2 : /* Channel Number */

			    fprintf (stderr,"------Channel Number--------\n");
			    fprintf (stderr,"	0 = Not Redundant\n");
			    fprintf (stderr,"	1 = Channel 1\n");
			    fprintf (stderr,"	2 = Channel 2\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rpg_channel_number = num;
			    break;

			case 3 : /* Channel State */

			    fprintf (stderr,"------Channel State--------\n");
			    fprintf (stderr,"	0 = Inactive\n");
			    fprintf (stderr,"	1 = Active\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rpg_channel_state = num;
			    break;

			case 4 : /* RPG State */

			    fprintf (stderr,"------RPG State--------\n");
			    fprintf (stderr,"	0 = Shutdown\n");
			    fprintf (stderr,"	1 = Standby\n");
			    fprintf (stderr,"	2 = Operating\n");
			    fprintf (stderr,"	3 = Transition\n");
			    fprintf (stderr,"	4 = Failed\n");
			    fprintf (stderr,"	5 = Powerfail\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rpg_state = num;
			    break;

			case 5 : /* RPG Mode */

			    fprintf (stderr,"------RPG Mode--------\n");
			    fprintf (stderr,"	0 = Operate\n");
			    fprintf (stderr,"	1 = Test (RPG)\n");
			    fprintf (stderr,"	2 = Test (RDA)\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rpg_mode = num;
			    break;

			case 6 : /* RDA Control State */

			    fprintf (stderr,"------RDA Control State--------\n");
			    fprintf (stderr,"	0 = Controlling\n");
			    fprintf (stderr,"	1 = Non-controlling\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rda_control_state = num;
			    break;

			case 7 : /* RDA Status */

			    fprintf (stderr,"------RDA Status--------\n");
			    fprintf (stderr,"	 0 = Unknown\n");
			    fprintf (stderr,"	 2 = Startup\n");
			    fprintf (stderr,"	 4 = Standby\n");
			    fprintf (stderr,"	 8 = Restart\n");
			    fprintf (stderr,"	16 = Operate\n");
			    fprintf (stderr,"	32 = Playback\n");
			    fprintf (stderr,"	64 = Offline Operate\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rda_status = num;
			    break;

			case 8 : /* RDA Operability Status */

			    fprintf (stderr,"------RDA Operability Status--------\n");
			    fprintf (stderr,"	  0 = Indeterminate\n");
			    fprintf (stderr,"	  2 = Online\n");
			    fprintf (stderr,"	  4 = Maintenance Required\n");
			    fprintf (stderr,"	  8 = Maintenance Mandatory\n");
			    fprintf (stderr,"	 16 = Commanded Shutdown\n");
			    fprintf (stderr,"	 32 = Inoperable\n");
			    fprintf (stderr,"	128 = Wideband Disconnect\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rda_operability_status = num;
			    break;

			case 9 : /* RDA/RPG WB Link State */

			    fprintf (stderr,"------RDA/RPG WB Link State--------\n");
			    fprintf (stderr,"	1 = Link Up\n");
			    fprintf (stderr,"	2 = Link Down\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rda_rpg_wb_link_state = num;
			    break;

			case 10 : /* Comms Relay State */

			    fprintf (stderr,"------Comms Relay State--------\n");
			    fprintf (stderr,"	0 = Unknown\n");
			    fprintf (stderr,"	1 = Assigned\n");
			    fprintf (stderr,"	2 = Unassigned\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.comms_relay_state = num;
			    break;

			case 11 : /* RPG/RPG Link State */

			    fprintf (stderr,"------RPG/RPG Link State--------\n");
			    fprintf (stderr,"	1 = Link Up\n");
			    fprintf (stderr,"	2 = Link Down\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.rpg_rpg_link_state = num;
			    break;

			case 12 : /* Adaptation Data Update Time */

			    fprintf (stderr,"------RDA/RPG WB Link State--------\n");
			    Red.adapt_dat_update_time = Local.adapt_dat_update_time;
			    break;

			case 13 : /* RDA Status Msg Sequence # */

			    fprintf (stderr,"------RDA Status Msg Sequence #--------\n");
			    fprintf (stderr,"Enter a selection: ");
			    scanf ("%d", &num);

			    Red.stat_msg_seq_num = num;
			    break;
	
		    }

		    break;

		case 4 : /* Update Redundant Channel Status */

		    status = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS,
			(char *) &Red,
			sizeof (Channel_status_t),
			ORPGRED_REDUN_CHANL_STATUS_MSG);

		    if (status <= 0) {

			fprintf (stderr,"Error writing redundant channel status msg: %d\n",
				status);

			exit (1);

		    }

		    break;

	    }
	}

	exit (0);
}
