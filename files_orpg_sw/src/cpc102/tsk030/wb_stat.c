/************************************************************************
 *	Module: wbstat.c						*
 *	Description: The purpose of this tool is to allow one to	*
 *		     modify, either by commandline or interactively,	*
 *		     the contents of the RDA wideband component of the	*
 *		     RDA status message buffer.				*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/07 19:09:23 $
 * $Id: wb_stat.c,v 1.3 2007/03/07 19:09:23 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	System and local include file definitions.			*/

#include <stdio.h>
#include <stdlib.h>
#include <infr.h>
#include <orpg.h>
#include <rda_control.h>
#include <gen_stat_msg.h>


/*	Global variables						*/

RDA_status_t	Rda_status;	/* Local RDA Status message buffer */
RDA_RPG_comms_status_t	*Wideband_status; /* pointer to wideband component */

/************************************************************************
 *	Description: This is the main function for the RDA-RPG wideband	*
 *		     link status tool.  Its purpose is to allow one to	*
 *		     change the contents of the wideband component of	*
 *		     the RDA status message.  The intended purpose was	*
 *		     for testing GUI response.				*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int main (int argc, char **argv)
{
static	int	rda_status_t_size = sizeof (RDA_status_t);
	int	len;
	int	selection;
	int	interactive_flag;
	int	display_blanking;
	int	wb_failed;
	int	state;
	int	display_state;

/*	Initialize local data/flags					*/

	state            = -1;
	display_blanking = -1;
	display_state    = 0;
	wb_failed        = 0;
	interactive_flag = 1;

/*	Parse commandline argument data					*/

	while (argc > 1) {

	    if (argv [1][0] == '-') {

		switch (argv [1][1]) {

		    case 'i':	/* Interactive mode */

			interactive_flag = 1;
			break;

		    case 'h':	/* Print command format */

			fprintf (stderr,"Description: Modifies the contents of the Wideband Status Message buffer\n");
			fprintf (stderr,"Usage: wbstat (options)\n");
			fprintf (stderr,"\n");
			fprintf (stderr,"    options:\n");
			fprintf (stderr,"	-b <num> display blanking\n");
			fprintf (stderr,"	-d display current state\n");
			fprintf (stderr,"	-h this help message\n");
			fprintf (stderr,"	-i interactive mode (default)\n");
			fprintf (stderr,"	-s <num> connection status\n");
			fprintf (stderr,"	-w <num> wideband failed flag\n");
			fprintf (stderr,"\n");

			exit (0);

		    case 'b':	/* Spot blanking */

			interactive_flag = 0;
			sscanf (&argv [2][0],"%d",&display_blanking);
			argc--;
			argv++;
			break;

		    case 'w':	/* Wideband Failed flag */

			interactive_flag = 0;
			sscanf (&argv [2][0],"%d",&wb_failed);
			argc--;
			argv++;
			break;

		    case 's':	/* Connection State */

			interactive_flag = 0;
			sscanf (&argv [2][0],"%d",&state);
			argc--;
			argv++;
			break;

		    case 'd':	/* Display state */

			interactive_flag = 0;
			display_state    = 1;
			break;

		}

		argv++;
		argc--;

	    } else {

		argv++;
		argc--;

	    }
	}

/*	Read the next message in the wideband status message.  A 	*
 *	good read is one where the returned length is equal to the	*
 *	length requested.  If no new status messages exist, return	*
 *	the contents of the last message (which is saved until a new	*
 *	message is read).  Otherwise, return an error code.		*/

	len = ORPGDA_read (ORPGDAT_GSM_DATA,
		 	   (char *) &Rda_status,
		           rda_status_t_size,
		           RDA_STATUS_ID);

	Wideband_status = (RDA_RPG_comms_status_t *) &Rda_status.wb_comms;

/*	If the interactive or display flags set, then we want to	*
 *	display the contents of the wideband data.			*/

	if (interactive_flag || display_state) {

	    if (len > 0) {

	        switch (Wideband_status->wblnstat) {
	    
		    case RS_NOT_IMPLEMENTED :

		        fprintf (stderr,"Current State: RS_NOT_IMPLEMENTED\n");
		        break;
	    
		    case RS_CONNECT_PENDING :

		        fprintf (stderr,"Current State: RS_CONNECT_PENDING\n");
		        break;
	    
		    case RS_DISCONNECT_PENDING :

		        fprintf (stderr,"Current State: RS_DISCONNECT_PENDING\n");
		        break;
	    
		    case RS_DISCONNECTED_HCI :

		        fprintf (stderr,"Current State: RS_DISCONNECTED_HCI\n");
		        break;
	    
		    case RS_DISCONNECTED_CM :

		        fprintf (stderr,"Current State: RS_DISCONNECTED_CM\n");
		        break;
	    
		    case RS_DISCONNECTED_SHUTDOWN :

		        fprintf (stderr,"Current State: RS_DISCONNECTED_SHUTDOWN\n");
		        break;
	    
		    case RS_DISCONNECTED_RMS :

		        fprintf (stderr,"Current State: RS_DISCONNECTED_RMS\n");
		        break;
	    
		    case RS_CONNECTED :

		        fprintf (stderr,"Current State: RS_CONNECTED\n");
		        break;
	    
		    case RS_DOWN :

		        fprintf (stderr,"Current State: RS_DOWN\n");
		        break;
	    
		    case RS_WBFAILURE :

		        fprintf (stderr,"Current State: RS_WBFAILURE\n");
		        break;

		    default :

		        fprintf (stderr,"Current State: Unknown\n");
		        break;

		}

	        fprintf (stderr,"Display Blanking: %d\n",
			Wideband_status->rda_display_blanking);

		fprintf (stderr,"WB Failed flag: %d\n",
			Wideband_status->wb_failed);

  	    } else {

	        fprintf (stderr,"ERROR reading RDA/RPG wideband status : %d\n", len);
	        exit (1);

	    }

	}

/*	If the interactive flag is set, we want to present the user	*
 *	with menus from which they can change various wideband data	*
 *	components.							*/

	if (interactive_flag) {

	    selection = 0;

	    while (selection >= 0) {

	        fprintf (stderr,"\n\n------------Wideband LB Tool-------------\n");
	        fprintf (stderr,"-1 = Exit\n");
	        fprintf (stderr,"1 = Change Status\n");
	        fprintf (stderr,"2 = Change Display Blanking\n");
	        fprintf (stderr,"3 = Change WB Failed Flag\n");
	        fprintf (stderr,"\nEnter your selection: ");
	        scanf ("%d",&selection);

	        switch (selection) {

	            case -1 :

		        exit (0);
		        break;

	            case 1 :

		        fprintf (stderr,"\n\n------Change Wideband Status------\n");
		        fprintf (stderr,"-1 = Exit\n");
		        fprintf (stderr,"0 = Return to Previous Menu\n");
		        fprintf (stderr,"1 = Status: Not Implemented\n");
		        fprintf (stderr,"2 = Status: Connect Pending\n");
		        fprintf (stderr,"3 = Status: Disconnect Pending\n");
		        fprintf (stderr,"4 = Status: Disconnected HCI\n");
		        fprintf (stderr,"5 = Status: Disconnected CM\n");
		        fprintf (stderr,"6 = Status: Disconnected SHUTDOWN\n");
		        fprintf (stderr,"7 = Status: Connected\n");
		        fprintf (stderr,"8 = Status: Down\n");
		        fprintf (stderr,"9 = Status: Wideband Failure\n");
	    	        fprintf (stderr,"\nEnter your selection: ");
		        scanf ("%d",&selection);

		        switch (selection) {

			    case -1 :

			        break;

			    case 0 :

			        break;

		  	    case 1 :

			        Wideband_status->wblnstat = RS_NOT_IMPLEMENTED;
			        break;

			    case 2 :

			        Wideband_status->wblnstat = RS_CONNECT_PENDING;
			        break;

			    case 3 :

			        Wideband_status->wblnstat = RS_DISCONNECT_PENDING;
			        break;

			    case 4 :

			        Wideband_status->wblnstat = RS_DISCONNECTED_HCI;
			        break;

			    case 5 :

		 	        Wideband_status->wblnstat = RS_DISCONNECTED_CM;
			        break;

			    case 6 :

		 	        Wideband_status->wblnstat = RS_DISCONNECTED_SHUTDOWN;
			        break;

		  	    case 7 :

			        Wideband_status->wblnstat = RS_CONNECTED;
			        break;

			    case 8 :

			        Wideband_status->wblnstat = RS_DOWN;
			        break;

		  	    case 9 :

			        Wideband_status->wblnstat = RS_WBFAILURE;
			        break;

		        }

		        if ((selection > 0) &&
			    (selection <= 9)) {

			    len = ORPGDA_write (ORPGDAT_GSM_DATA,
		 		   (char *) &Rda_status,
		        	   rda_status_t_size,
		           	   RDA_STATUS_ID);

			    if (len < 0) {

			        fprintf (stderr,"ERROR writing RDA/RPG wideband status : %d\n", len);
	    		        exit (1);

			    }

			    fprintf (stderr,"Posting ORPGEVT_RDA_STATUS_CHANGE event\n");
			    EN_post (ORPGEVT_RDA_STATUS_CHANGE,
				 NULL, 0, 0);

		        }

		        break;

		    case 2 :

		        fprintf (stderr,"\n\n------Display Blanking------\n");
		        fprintf (stderr,"-1 = Return to Previous Menu\n");
		        fprintf (stderr,"0 = No Display Blanking\n");
		        fprintf (stderr,"1 = RDA status\n");
		        fprintf (stderr,"2 = Operability Status\n");
		        fprintf (stderr,"3 = Control Status\n");
		        fprintf (stderr,"4 = Aux Power Gen State\n");
		        fprintf (stderr,"5 = Ave Transmitted Power\n");
		        fprintf (stderr,"6 = Reflectivity Calib Corr\n");
		        fprintf (stderr,"7 = Data Transmission Enabled\n");
		        fprintf (stderr,"8 = VCP Number\n");
		        fprintf (stderr,"9 = RDA Control Authorization\n");
		        fprintf (stderr,"10 = Interference Detect Rate\n");
		        fprintf (stderr,"11 = Operational Mode\n");
		        fprintf (stderr,"12 = Interference Suppression Unit\n");
		        fprintf (stderr,"13 = Archive II Status\n");
		        fprintf (stderr,"14 = Archive II Capacity\n");
		        fprintf (stderr,"15 = RDA Alarm Summary\n");
		        fprintf (stderr,"16 = Command Acknowledge\n");
		        fprintf (stderr,"17 = Channel Control Status\n");
		        fprintf (stderr,"18 = Spot Blanking Status\n");
	    	        fprintf (stderr,"\nEnter your selection: ");
		        scanf ("%d",&selection);

			if (selection > 18) {

			    selection = -1;

			}

		        switch (selection) {

			    case -1 :

			        break;

		  	    default :

			        Wideband_status->rda_display_blanking =
				    selection;
			    break;

		        }

		        if ((selection >= 0) &&
			    (selection < 19)) {

			    len = ORPGDA_write (ORPGDAT_GSM_DATA,
		 		   (char *) &Rda_status,
		        	   rda_status_t_size,
		           	   RDA_STATUS_ID);

			    if (len < 0) {

			        fprintf (stderr,"ERROR writing RDA status : %d\n", len);
	    		        exit (1);

			    }

			    EN_post (ORPGEVT_RDA_STATUS_CHANGE,
				 NULL, 0, 0);

		        }

			break;

		    case 3 :

		        fprintf (stderr,"\n\n------WB Failed Flag------\n");
		        fprintf (stderr,"-1 = Return to Previous Menu\n");
		        fprintf (stderr,"0 = Clear FLag\n");
		        fprintf (stderr,"1 = Set Flag\n");
	    	        fprintf (stderr,"\nEnter your selection: ");
		        scanf ("%d",&selection);

			if (selection > 1) {

			    selection = 1;

			}

		        switch (selection) {

			    case -1 :

			        break;

		  	    default :

			        Wideband_status->wb_failed =
				    selection;
				break;

		        }

			len = ORPGDA_write (ORPGDAT_GSM_DATA,
		 		   (char *) &Rda_status,
		        	   rda_status_t_size,
		           	   RDA_STATUS_ID);

			if (len < 0) {

			    fprintf (stderr,"ERROR writing RDA status : %d\n", len);
	    		    exit (1);

			}

			EN_post (ORPGEVT_RDA_STATUS_CHANGE,
				 NULL, 0, 0);


			break;
	        }
	    }

/*	Non-interactive so we want to change the items directed from	*
 *	commandline arguments.						*/

	} else {

	    if ((state > 0) && (state < 9)) {

		 Wideband_status->wblnstat = state;

	    }

	    if ((display_blanking >= 0) && (display_blanking < 19)) {

		 Wideband_status->rda_display_blanking = display_blanking;

	    }

	    if ((wb_failed >= 0) && (display_blanking <= 1)) {

		 Wideband_status->wb_failed = wb_failed;

	    }

	    len = ORPGDA_write (ORPGDAT_GSM_DATA,
 		   (char *) &Rda_status,
        	   rda_status_t_size,
           	   RDA_STATUS_ID);

	    if (len < 0) {

	        fprintf (stderr,"ERROR writing RDA status : %d\n", len);
 	        exit (1);

	    }

	    if ((state > 0) || (display_blanking >= 0)) {

	        EN_post (ORPGEVT_RDA_STATUS_CHANGE,
			 NULL, 0, 0);

	    }
	}

   return 0;
}
