/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/25 19:08:50 $
 * $Id: rdastat.c,v 1.11 2012/09/25 19:08:50 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
*/


/************************************************************************
 *	Module: rdastat.c						*
 *	Description: This module provides the user with	a simple way to	*
 *		     set various components of an RDA status message	*
 *		     and write the result to the rda_status_lb linear	*
 *		     buffer so it can be read by the ORPG UCP status	*
 *		     monitoring function.  This allows one to test the	*
 *		     GUI for displaying	and reporting RDA status.	*
 ************************************************************************/

/*	System and local include file definitions.			*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <infr.h>
#include <orpgdat.h>
#include <orpg.h>
#include <orpgevt.h>
#include <gen_stat_msg.h>
#include <rda_status.h>
#include <lb.h>

#define	SECONDS_IN_DAY	86400

ORDA_status_t	Rda_status; /* Local RDA status message data */

/************************************************************************
 *	Description: This is the main function for the RDA Status	*
 *		     message tool.  The user can set specific RDA	*
 *		     status message elements from the commandline or	*
 *		     interactively.					*
 *									*
 *	Input:  argc - number of commandline arguments.			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: Exit code						*
 ************************************************************************/

int main( int	argc, char *argv [] ){

	int	i;
	int	j;
	int	num;
	int	selection;
	int	len;
	time_t	tm;
	char	*element_name [32];
	char	*element_value [32];
	int	num_elements;
	int	post_event;
	int	interactive_flag;
	int	echo_flag;

	ORDA_status_msg_t	*rda_status;

/*	Initialize local data	*/

	num_elements     = 0;
	post_event       = 0;
	interactive_flag = 1;
	echo_flag = 0;

/*	Parse commandline argument data		*/

	while (argc > 1) {

	    if (argv [1][0] == '-') {

		switch (argv [1][1]) {

		    case 'i':	/* Interactive mode */

			interactive_flag = 1;
			break;

		    case 'h':	/* Print command format */

			fprintf (stderr,"Description: Modifies the contents of the RDA Status Message buffer\n");
			fprintf (stderr,"Usage: rdastat (options)\n");
			fprintf (stderr,"\n");
			fprintf (stderr,"    options:\n");
			fprintf (stderr,"	-h this help message\n");
			fprintf (stderr,"	-i interactive mode (default)\n");
			fprintf (stderr,"	-<n> <data> element name to change\n");
			fprintf (stderr,"	   (see rda_status.h for names\n");
			fprintf (stderr,"	    of elements: ie., -RS_RDA_STATUS)\n");
			fprintf (stderr,"	-s Post RDA Status update event\n");
			fprintf (stderr,"	-v echo changes items to stderr\n");
			fprintf (stderr,"\n");

			exit (0);

		    case 'R':	/* Element Name */

			interactive_flag = 0;
			element_name [num_elements]  = &argv [1][1];
			element_value [num_elements] = &argv [2][0];
			num_elements++;
			argc--;
			argv++;
			break;

		    case 's':	/* Post update event */

			interactive_flag = 0;
			post_event = 1;
			break;

		    case 'v':	/* Echo changes to stderr */

			interactive_flag = 0;
			echo_flag = 1;
			break;

		}

		argv++;
		argc--;

	    } else {

		argv++;
		argc--;

	    }
	}

/*	Initialize the elements of the RDA Status Message	*/

	ORPGDA_write_permission( ORPGDAT_GSM_DATA );

	len = ORPGDA_read (ORPGDAT_GSM_DATA,
			(char *) &Rda_status,
			sizeof (ORDA_status_t),
			RDA_STATUS_ID);

	rda_status = (ORDA_status_msg_t *) &Rda_status.status_msg;

/*	If there was an error reading the RDA status message just set	*
 *	the local data elements to some values so we can write them	*
 *	if we want.							*/

	if (len <= 0) {

	    rda_status->rda_status           =   16;
	    rda_status->op_status            =    2;
	    rda_status->control_status       =    4;
	    rda_status->aux_pwr_state        =    2;
	    rda_status->ave_trans_pwr        = 1000;
	    rda_status->ref_calib_corr       =    0;
	    rda_status->vc_ref_calib_corr    =    0;
	    rda_status->data_trans_enbld     =   28;
	    rda_status->vcp_num              =   21;
	    rda_status->rda_control_auth     =    0;
	    rda_status->rda_build_num        =    0;
	    rda_status->op_mode              =    4;
	    rda_status->super_res            =    2;
	    rda_status->cmd                  =    4;
	    rda_status->avset                =    4;
	    rda_status->rda_alarm            =    0;
	    rda_status->command_status       =    0;
	    rda_status->channel_status       =    0;
	    rda_status->spot_blanking_status =    0;
	    rda_status->bypass_map_date      =    1;
	    rda_status->bypass_map_time      =    0;
	    rda_status->clutter_map_date     =    1;
	    rda_status->clutter_map_time     =    0;
	    rda_status->tps_status           =    0;
	    rda_status->rms_control_status   =    0;
            rda_status->perf_check_status    =    0;

	    for (i=0;i<14;i++) {

	 	rda_status->alarm_code [i] = 0;

	    }

/*	    Try writing the made up data out.				*/

	    len = ORPGDA_write (ORPGDAT_GSM_DATA,
		    (char *) &Rda_status,
		    sizeof (ORDA_status_t),
		    RDA_STATUS_ID);

/*	    If there was an error then the LB probably doesn't exist.	*/

	    if (len <=0) {

		fprintf (stderr,"ERROR: ORPGDAT_GSM_DATA does not exist\n");
		exit (1);

	    }
	}

/*	Get the current julian date				*/

	tm = time (NULL);
	rda_status->msg_hdr.julian_date  = tm / SECONDS_IN_DAY;
	rda_status->msg_hdr.milliseconds = (tm -
		rda_status->msg_hdr.julian_date * SECONDS_IN_DAY) * 1000;

/*	Add one to the date since January 1, 1970 is day 1 instead of 0	*/

	rda_status->msg_hdr.julian_date++;

/*	If the interactive mode flag was set then we want to display	*
 *	text menus so the user can set the varius message components.	*/

	if (interactive_flag) {

	    selection = 0;

	    while (selection >= 0) {

	        fprintf (stderr,"\n\n\n\n\n-----------RDA Status Message Tool----------\n");
	        fprintf (stderr,"-1 = Exit\n");
	        fprintf (stderr," 0 = Modify RDA Status Message\n");
	        fprintf (stderr," 1 = Get RDA Status Message\n");
	        fprintf (stderr," 2 = Send RDA Status Message\n");
	        fprintf (stderr,"Enter your selection: ");
	        scanf ("%d", &selection);

	        switch (selection) {

	            case -1 : /*  Exit from program  */

		        break;

	            case 1 : /*  Get RDA Status Message  */

		        len = ORPGDA_read (ORPGDAT_GSM_DATA,
			    (char *) &Rda_status,
			    sizeof (ORDA_status_t),
			    RDA_STATUS_ID);

		        if (len <=0) {

			    fprintf (stderr,"ERROR reading RDA Status Message LB: %d\n",
				len);

		        }

		        break;

	            case 2 : /*  Send RDA Status Message  */

/*	Open the RDA status linear buffer for single writer.		*/

		        len = ORPGDA_write (ORPGDAT_GSM_DATA,
			    (char *) &Rda_status,
			    sizeof (ORDA_status_t),
			    RDA_STATUS_ID);

		        EN_post (ORPGEVT_RDA_STATUS_CHANGE,
		 	    NULL, 0, 0);

		        break;

	            case 0 : /*  Modify RDA Status Message  */

		        fprintf (stderr,"\n\n\n\n\n 1 = RDA Status [%d]\n",
				rda_status->rda_status);
		        fprintf (stderr," 2 = Operability Status [%d]\n",
				rda_status->op_status);
		        fprintf (stderr," 3 = Control Status [%d]\n",
				rda_status->control_status);
		        fprintf (stderr," 4 = Auxiliary Power Generator State [%d]\n",
				rda_status->aux_pwr_state);
		        fprintf (stderr," 5 = Average Transmitter Power [%d]\n",
				rda_status->ave_trans_pwr);
		        fprintf (stderr," 6 = Reflectivity Calibration Correction (H) [%d]\n",
				rda_status->ref_calib_corr);
		        fprintf (stderr," 7 = Reflectivity Calibration Correction (V) [%d]\n",
				rda_status->vc_ref_calib_corr);
		        fprintf (stderr," 8 = Data Transmission Enabled [%d]\n",
				rda_status->data_trans_enbld);
		        fprintf (stderr," 9 = Volume Coverage Pattern [%d]\n",
				rda_status->vcp_num);
		        fprintf (stderr," 10 = RDA Control Authorization [%d]\n",
				rda_status->rda_control_auth);
		        fprintf (stderr,"11 = RDA Build Number [%d]\n",
				rda_status->rda_build_num);
		        fprintf (stderr,"12 = Operational Mode [%d]\n",
				rda_status->op_mode);
		        fprintf (stderr,"13 = Super Resolution [%d]\n",
				rda_status->super_res);
		        fprintf (stderr,"14 = Clutter Mitigation Decision [%d]\n",
				rda_status->cmd);
		        fprintf (stderr,"15 = Avset [%d]\n",
				rda_status->avset);
		        fprintf (stderr,"16 = RDA Alarm Summary [%d]\n",
				rda_status->rda_alarm);
		        fprintf (stderr,"17 = Command Acknowledgment [%d]\n",
				rda_status->command_status);
		        fprintf (stderr,"18 = Channel Control Status [%d]\n",
				rda_status->channel_status);
		        fprintf (stderr,"19 = Spot Blanking Status [%d]\n",
				rda_status->spot_blanking_status);
		        fprintf (stderr,"20 = Bypass Map Generation Date [%d]\n",
				rda_status->bypass_map_date);
		        fprintf (stderr,"21 = Bypass Map Generation Time [%d]\n",
				rda_status->bypass_map_time);
		        fprintf (stderr,"22 = Clutter Map Generation Date [%d]\n",
				rda_status->clutter_map_date);
		        fprintf (stderr,"23 = Clutter Map Generation Time [%d]\n",
				rda_status->clutter_map_time);
		        fprintf (stderr,"24 = TPS Status [%d]\n",
				rda_status->tps_status);
		        fprintf (stderr,"25 = RMS Control Status [%d]\n",
				rda_status->rms_control_status);
		        fprintf (stderr,"26 = Perf Check Status [%d]\n",
				rda_status->perf_check_status);
		        fprintf (stderr,"27 = Alarm Codes [0] = %d\n",
				rda_status->alarm_code [0]);
		        fprintf (stderr,"Enter your selection: ");
		        scanf ("%d", &selection);

	    	        switch (selection) {

			    case 1 : /*  RDA Status  */

			        fprintf (stderr,"*****RDA status code*****\n");
			        fprintf (stderr,"	  2 = Start-Up\n");
			        fprintf (stderr,"	  4 = Standby\n");
			        fprintf (stderr,"	  8 = Restart\n");
			        fprintf (stderr,"	 16 = Operate\n");
			        fprintf (stderr,"	 32 = Playback\n");
			        fprintf (stderr,"	 64 = Off-line Operate\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

	     		        rda_status->rda_status = num;
			        break;

			    case 2 : /*  Operability Status  */

			        fprintf (stderr,"*****RDA operability status code*****\n");
			        fprintf (stderr,"	  2 = Online\n");
			        fprintf (stderr,"	  4 = Maintenance Required\n");
			        fprintf (stderr,"	  8 = Maintenance Mandatory\n");
			        fprintf (stderr,"	 16 = Commanded Shut-down\n");
			        fprintf (stderr,"	 32 = Inoperable\n");
			        fprintf (stderr,"	 NOTE: Add 1 to above codes to disable auto calibration\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

	    		        rda_status->op_status = num;
			        break;

			    case 3 : /*  Control Status  */

			        fprintf (stderr,"*****RDA control status code*****\n");
			        fprintf (stderr,"	  2 = Local\n");
			        fprintf (stderr,"	  4 = Remote\n");
			        fprintf (stderr,"	  8 = Either\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->control_status = num;
			        break;

			    case 4 : /*  Auxiliary Power Generator State  */

			        fprintf (stderr,"*****Auxilliary Power Generator State*****\n");
			        fprintf (stderr,"	  2 = Utility Power Available\n");
			        fprintf (stderr,"	  4 = Generator On\n");
			        fprintf (stderr,"	  8 = Transfer Switch (Manual)\n");
			        fprintf (stderr,"	 16 = Commanded Switchover\n");
			        fprintf (stderr,"	Above codes +1 = Switched to AUX\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->aux_pwr_state = num;
			        break;

			    case 5 : /*  Average Transmitter Power  */

			        fprintf (stderr,"*****Average Transmitter Power*****\n");
			        fprintf (stderr,"	Integer value\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

	    		        rda_status->ave_trans_pwr = num;
			        break;

			    case 6 : /*  Reflectivity Calibration Correction - Horizontal  */

			        fprintf (stderr,"*****Reflectivity calib correction (H)*****\n");
			        fprintf (stderr,"	-10 to 10 scaled by 4\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->ref_calib_corr = num;
			        break;

			    case 7 : /*  Reflectivity Calibration Correction - Vertical */

			        fprintf (stderr,"*****Reflectivity calib correction (V)*****\n");
			        fprintf (stderr,"	-10 to 10 scaled by 4\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->vc_ref_calib_corr = num;
			        break;

			    case 8 : /*  Data Transmission Enabled  */

			        fprintf (stderr,"*****Data Transmission Fields Enabled*****\n");
			        fprintf (stderr,"	  2 = none\n");
			        fprintf (stderr,"	  4 = Reflectivity\n");
			        fprintf (stderr,"	  8 = Velocity\n");
			        fprintf (stderr,"	 12 = Reflectivity+Velocity\n");
			        fprintf (stderr,"	 16 = Spectrum Width\n");
			        fprintf (stderr,"	 20 = Reflectivity+Spectrum Width\n");
			        fprintf (stderr,"	 24 = Velocity+Spectrum Width\n");
			        fprintf (stderr,"	 28 = All Fields\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->data_trans_enbld = num;
			        break;

			    case 9 : /*  Volume Coverage Pattern  */

			        fprintf (stderr,"*****Volume Coverage Pattern*****\n");
			        fprintf (stderr,"	  0     = No pattern\n");
			        fprintf (stderr,"	1 to 99 = Constant Elevation Types\n");
			        fprintf (stderr,"	<=255   = Operational\n");
			        fprintf (stderr,"	>255    = Maintenance/Test\n");
			        fprintf (stderr,"	Positive: RDA Remote pattern (RPG)\n");
			        fprintf (stderr,"	Negative: RDA Local pattern\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->vcp_num = num;
			        break;

			    case 10 : /*  RDA Control Authorization  */

			        fprintf (stderr,"*****RDA Control Authorization*****\n");
			        fprintf (stderr,"	  0 = No action\n");
			        fprintf (stderr,"	  2 = Local control requested\n");
			        fprintf (stderr,"	  4 = Remote control enabled\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->rda_control_auth = num;
			        break;

			    case 11 : /*  RDA Build Number  */

			        fprintf (stderr,"*****RDA Build Number*****\n");
			        fprintf (stderr,"	0 to 999\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->rda_build_num = num;
			        break;

			    case 12 : /*  Operational Mode  */

			        fprintf (stderr,"*****Operational Mode*****\n");
			        fprintf (stderr,"	  2 = Maintenance\n");
			        fprintf (stderr,"	  4 = Operational\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->op_mode = num;
			        break;

			    case 13 : /*  Super Resolution  */

			        fprintf (stderr,"*****Super Resolution*****\n");
			        fprintf (stderr,"	  2 = Enabled\n");
			        fprintf (stderr,"	  4 = Disabled\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->super_res = num;
			        break;

			    case 14 : /*  Clutter Mitigation Decision  */

			        fprintf (stderr,"*****Clutter Mitigation Decision*****\n");
			        fprintf (stderr,"	  2 = Enabled\n");
			        fprintf (stderr,"	  4 = Disabled\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->cmd = num;
			        break;

			    case 15 : /*  Automatic Volume Scan Evaluation and Termination  */

			        fprintf (stderr,"*****AVSET*****\n");
			        fprintf (stderr,"	2 - Enabled\n");
			        fprintf (stderr,"	4 - Disabled\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->avset = num;
			        break;

			    case 16 : /*  RDA Alarm Summary  */

			        fprintf (stderr,"*****RDA Alarm Summary*****\n");
			        fprintf (stderr,"	  0 = No alarms\n");
			        fprintf (stderr,"	  2 = Tower/Utilities\n");
			        fprintf (stderr,"	  4 = Pedestal\n");
			        fprintf (stderr,"	  8 = Transmitter\n");
			        fprintf (stderr,"	 16 = Receiver/Signal Processor\n");
			        fprintf (stderr,"	 32 = RDA Control\n");
			        fprintf (stderr,"	 64 = RPG Communication\n");
			        fprintf (stderr,"	128 = User Communication\n");
			        fprintf (stderr,"	256 = Archive II\n");
			        fprintf (stderr,"      NOTE:  Use combinations for multiple alarms\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->rda_alarm = num;
			        break;

			    case 17 : /*  Command Acknowledgement  */

			        fprintf (stderr,"*****Command Status*****\n");
			        fprintf (stderr,"	  0 = No acknowledgement\n");
			        fprintf (stderr,"	  1 = Remote VCP Received\n");
			        fprintf (stderr,"	  2 = Clutter bypass map received\n");
			        fprintf (stderr,"	  3 = Clutter sensor zones received\n");
			        fprintf (stderr,"	  4 = Redundant channel standby command accepted\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->command_status = num;
			        break;

		   	    case 18 : /*  Channel Control Status  */

			        fprintf (stderr,"*****Channel Status*****\n");
			        fprintf (stderr,"	  0 = Controlling\n");
			        fprintf (stderr,"	  1 = Non-controlling\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->channel_status = num;
			        break;

			    case 19 : /*  Spot Blanking Status  */

			        fprintf (stderr,"*****Spot Blanking Status*****\n");
			        fprintf (stderr,"	  0 = Not installed\n");
			        fprintf (stderr,"	  2 = Enabled\n");
			        fprintf (stderr,"	  4 = Disabled\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->spot_blanking_status = num;
			        break;

			    case 20 : /*  Bypass Map Gen Date  */

			        fprintf (stderr,"*****Bypass Map Date*****\n");
			        fprintf (stderr,"	Range 1 to 65535)\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->bypass_map_date = num;
			        break;

			    case 21 : /*  Bypass Map Gen Time  */

			        fprintf (stderr,"****Bypass Map Time****\n");
			        fprintf (stderr,"	Range 0 to 1440\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->bypass_map_time = num;
			        break;

			    case 22 : /*  Clutter Map Gen Date  */

			        fprintf (stderr,"*****Clutter Map Date*****\n");
			        fprintf (stderr,"	Range 1 to 65535\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->clutter_map_date = num;
			        break;

			    case 23 : /*  Notchwidth Map Gen Time  */

			        fprintf (stderr,"*****Clutter Map Time*****\n");
			        fprintf (stderr,"	Range 0 to 1440\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->clutter_map_time = num;
			        break;

			    case 24 : /*  TPS Status  */

			        fprintf (stderr,"*****TPS Status*****\n");
			        fprintf (stderr,"	  0 = Not installed\n");
			        fprintf (stderr,"	  1 = OFF\n");
			        fprintf (stderr,"	  3 = OK\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->tps_status = num;

				break;

			    case 25 : /*  RMS Control Status  */

			        fprintf (stderr,"*****RMS Control Status*****\n");
			        fprintf (stderr,"	  0 = Non-RMS System\n");
			        fprintf (stderr,"	  2 = RMS in Control\n");
			        fprintf (stderr,"	  4 = RDA MMI in Control\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->rms_control_status = num;

				break;

			    case 26 : /*  Perf Check Status  */

			        fprintf (stderr,"*****Perf Check Status*****\n");
			        fprintf (stderr,"	  0 = Auto\n");
			        fprintf (stderr,"	  1 = Pending\n");
			        fprintf (stderr,"Enter a value: ");
			        scanf ("%d", &num);

			        rda_status->perf_check_status = num;

				break;

			    case 27 : /*  Alarm Codes  */

			        fprintf (stderr,"Enter up to 14 alarm codes.  End list with code 0\n");
			        for (i=0;i<14;i++) {

				    fprintf (stderr,"Enter Alarm Code (1-800) [%d]: ", i);
		  	            scanf ("%d", &num);

	    			    if (num <= 0) {

				        for (j=i;j<14;j++) {

				            rda_status->alarm_code [j] = 0;

					}

				        break;

				    }

			            rda_status->alarm_code [i] = num;

				    fprintf (stderr,"0 = Clear Alarm Flag\n");
				    fprintf (stderr,"1 = Set Alarm Flag\n");
			            fprintf (stderr,"Enter a value: ");
			            scanf ("%d", &num);

				    if (num == 0) {

				        rda_status->alarm_code [i] = rda_status->alarm_code [i] | 0x8000;

				    }
			        }

			        break;

		        }
	        }
	    }

/*	Non-interactive mode so check to see which elements are to be	*
 *	modified via commandline arguments.				*/

	} else {

	    for (i=0;i<num_elements;i++) {

		if (echo_flag) {

		    fprintf (stderr,"%s -> %s\n", element_name [i], element_value [i]);

		}

		sscanf (element_value [i],"%d",&num);

		if (strcmp (element_name [i],"RS_RDA_STATUS") == 0) {

		    rda_status->rda_status = num;

		} else if (strcmp (element_name [i],"RS_OPERABILITY_STATUS") == 0) {

		    rda_status->op_status = num;

		} else if (strcmp (element_name [i],"RS_CONTROL_STATUS") == 0) {

		    rda_status->control_status = num;

		} else if (strcmp (element_name [i],"RS_AUX_POWER_GEN_STATE") == 0) {

		    rda_status->aux_pwr_state = num;

		} else if (strcmp (element_name [i],"RS_AVE_TRANS_POWER") == 0) {

		    rda_status->ave_trans_pwr = num;

		} else if (strcmp (element_name [i],"RS_REFL_CALIB_CORRECTION") == 0) {

		    rda_status->ref_calib_corr = num;

		} else if (strcmp (element_name [i],"RS_DATA_TRANS_ENABLED") == 0) {

		    rda_status->data_trans_enbld = num;

		} else if (strcmp (element_name [i],"RS_VCP_NUMBER") == 0) {

		    rda_status->vcp_num = num;

		} else if (strcmp (element_name [i],"RS_RDA_CONTROL_AUTH") == 0) {

		    rda_status->rda_control_auth = num;

		} else if (strcmp (element_name [i],"RS_RDA_BUILD_NUM") == 0) {

		    rda_status->rda_build_num = num;

		} else if (strcmp (element_name [i],"RS_OPERATIONAL_MODE") == 0) {

		    rda_status->op_mode = num;

		} else if (strcmp (element_name [i],"RS_SUPER_RES") == 0) {

		    rda_status->super_res = num;

		} else if (strcmp (element_name [i],"RS_CMD") == 0) {

		    rda_status->cmd = num;

		} else if (strcmp (element_name [i],"RS_AVSET") == 0) {

		    rda_status->avset = num;

		} else if (strcmp (element_name [i],"RS_RDA_ALARM_SUMMARY") == 0) {

		    rda_status->rda_alarm = num;

		} else if (strcmp (element_name [i],"RS_COMMAND_ACK") == 0) {

		    rda_status->command_status = num;

		} else if (strcmp (element_name [i],"RS_CHAN_CONTROL_STATUS") == 0) {

		    rda_status->channel_status = num;

		} else if (strcmp (element_name [i],"RS_SPOT_BLANKING_STATUS") == 0) {

		    rda_status->spot_blanking_status = num;

		} else if (strcmp (element_name [i],"RS_BPM_GEN_DATE") == 0) {

		    rda_status->bypass_map_date = num;

		} else if (strcmp (element_name [i],"RS_BPM_GEN_TIME") == 0) {

		    rda_status->bypass_map_time = num;

		} else if (strcmp (element_name [i],"RS_NWM_GEN_DATE") == 0) {

		    rda_status->clutter_map_date = num;

		} else if (strcmp (element_name [i],"RS_NWM_GEN_TIME") == 0) {

		    rda_status->clutter_map_time = num;

		} else if (strcmp (element_name [i],"RS_TPS_STATUS") == 0) {

		    rda_status->tps_status = num;

		} else if (strcmp (element_name [i],"RS_RMS_CONTROL_STATUS") == 0) {

		    rda_status->rms_control_status = num;

		} else if (strcmp (element_name [i],"RS_PERF_CHECK_STATUS") == 0) {

		    rda_status->perf_check_status = num;

		} else if (strcmp (element_name [i],"RS_ALARM_CODE1") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }
			
		} else if (strcmp (element_name [i],"RS_ALARM_CODE2") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE3") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE4") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE5") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE6") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE7") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE8") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE9") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE10") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE11") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE12") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE13") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		} else if (strcmp (element_name [i],"RS_ALARM_CODE14") == 0) {

		    if (num < 0) {

			rda_status->alarm_code [0] = ((short) -num) | 0x8000;

		    } else {

			rda_status->alarm_code [0] = num;

		    }

		}
	    }

/*	    Write the message back out.					*/

	    len = ORPGDA_write (ORPGDAT_GSM_DATA,
		    (char *) &Rda_status,
		    sizeof (ORDA_status_t),
		    RDA_STATUS_ID);

	    if (post_event) {

		EN_post (ORPGEVT_RDA_STATUS_CHANGE,
			    NULL, 0, 0);

	    }
	}

   return 0;
}
