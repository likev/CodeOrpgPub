/************************************************************************
 *									*
 *	Module:  rda_alarm.c						*
 *									*
 *	Description:  This module simulates rda alarms			*
 *		      It was written to test the hci_popup_rda_alarms.c	*
 *		      module						*
 *									*
 ************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 19:49:49 $
 * $Id: rda_alarm.c,v 1.10 2005/12/27 19:49:49 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

/*	System include file definitions.				*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>  /* setpgid,getpid,getopt,gethostname,sleep       */

/*	Local include file definitions.					*/

#include <misc.h>
#include <rda_alarm_table.h>
#include <infr.h>
#include <orpg.h>
#include <orpgdat.h>
#include <en.h>
#include <orpgevt.h>

/**************************************************************************
 Description: Read the command-line options
       Input: argc,argv
       	      -t types
       	      -f seconds
       	      -a starting alarm no
	      -n number of alarms
	      -l write msg to RPG log
	      -s set (1)/clear (0) flag (default 1)
       	      -v verbose
       	      -h explain flags
      Output: none
     Returns: 1 if successful; 0 if successful, but do not run program, and
     		-1 if there is an error
       Notes:
 **************************************************************************/
static int Read_options(int argc,
                        char **argv,
                        int *frequency,
                        int *start_alarm_no,
			int *no_of_alarms,
			int *channel_number,
			int *log_flag,
			int *set_clear_flag,
                        int *verbose_flag)
{
   int input;
   int retval = 1;

   /*
    * Establish some defaults that may be overridden ...
    */
   *frequency      = 30;
   *start_alarm_no = 1;
   *verbose_flag   = 0;
   *no_of_alarms   = 1;
   *log_flag       = 0;
   *set_clear_flag = 1;
   *channel_number = 0;

   while ((input = getopt(argc,argv,"hlva:c:f:n:s:")) != -1) {
      switch(input) 
      {
         case 'a':	
         	*start_alarm_no = atoi(optarg);

		if ((*start_alarm_no > 800) || (*start_alarm_no < 1)) {

		    fprintf (stdout,"Invalid starting alarm no %d; using default\n", *start_alarm_no);
		    *start_alarm_no = 1;

		}
		fprintf (stderr,"start_alarm_no = %d\n", *start_alarm_no);
		break ;
         case 'f':	
         	*frequency = atoi(optarg);
		fprintf (stderr,"frequency = %d\n", *frequency);
		break ;
         case 'h':
		retval = 0;
		fprintf(stdout,"Version 1.0\n");
		fprintf(stdout,"options:\n");
		fprintf(stdout,"-a starting alarm number (default = 1)\n");
		fprintf(stdout,"-c channel number (default 0)\n");
		fprintf(stdout,"-f frequency of generated alarms in seconds (default = 30)\n");
		fprintf(stdout,"-h this help message\n");
		fprintf(stdout,"-l write to RPG system log\n");
		fprintf(stdout,"-n number of alarms\n");
		fprintf(stdout,"-s alarm set (1)/cleared (0)\n");
		fprintf(stdout,"-v verbose mode\n");
		break ;
	 case 'n':
		*no_of_alarms = atoi(optarg);
		fprintf (stderr,"no_of_alarms = %d\n", *no_of_alarms);
		break;
	 case 'c':
		*channel_number = atoi(optarg);
		fprintf (stderr,"channel_number = %d\n", *channel_number);
		break;
	 case 's':
		*set_clear_flag = atoi(optarg);
		fprintf (stderr,"set_clear_flag = %d\n", *set_clear_flag);
		break;
         case 'l':
		*log_flag = 1;
		fprintf (stderr,"log_flag = %d\n", *log_flag);
		break;
         case 'v':
		*verbose_flag = 1;
		fprintf (stderr,"verbose_flag = %d\n", *verbose_flag);
		break;
      }
   }

   return(retval);
/*END of Read_options()*/
}

/************************************************************************
 *	Description: Program to generate rda alarms every n seconds.	*
 *		     (Written to test hci_pop_rda_alarms.c alarm	*
 *		     selection mechanisms.				*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int main(int argc, char *argv[])
{
	int i;
	int hour,min,sec, year, month, day;
	int lbfd;		/*  ORPGDAT_GSM_DATA linear buffer id */
	time_t todays_time;	
	int status;			
	RDA_alarm_t	alarm_desc;
	RDA_alarm_entry_t *rda_alarm_entry;
	int frequency;   	/*  Frequency to generate alarms in seconds */
	int start_alarm_no;	/*  Starting alarm number to generate */
	int no_of_alarms;
	int verbose_flag;	/*  Verbose or not */
	int set_clear_flag;
	int log_flag;
	int code;
	int channel_number;
	
/*	Initialize log error services.					*/

	if ((status = ORPGMISC_init (argc, argv, 1000, 0, 0, 0)) < 0) {

	    fprintf (stderr,"ORPGMISC_init failed: (ret: %d)\n", status);

	}

/*	Parse commandline arguments.					*/

	if (Read_options(argc, argv, &frequency, &start_alarm_no, &no_of_alarms, &channel_number, &log_flag, &set_clear_flag, &verbose_flag) <= 0)
	    exit(0);

/*	If the verbose flag (-v) set, print info to standard output.	*/

	if (verbose_flag)
	{
		fprintf(stdout,"Generate alarms every %d seconds\n", frequency);
		fprintf(stdout,"Starting alarm number = %d\n", start_alarm_no);
	}
	
/*	Get the data ID for the RDA alarms LB if it exists.  If not	*
 *	return.								*/

	lbfd = ORPGDA_lbfd (ORPGDAT_RDA_ALARMS);

/*	If the LB doesn't exist or if there is some other problem,	*
 *	return.								*/

	if (lbfd <= 0) 
	{
	    fprintf (stderr, "ERROR: unable to open RDA status LB: %d\n",
		     lbfd);
	    exit(0);
	}
	
	code = start_alarm_no;

/*	Repeat for the number of times specified in commandline.	*/

	for (i = 0; i < no_of_alarms; i++)
	{

/*		Get the current system time.				*/

	    	todays_time = time(NULL);
	    	
		unix_time (&todays_time,
			   &year,
			   &month,
			   &day,
			   &hour,
			   &min,
			   &sec);

		
/*		Set the alarm data elements.				*/

		alarm_desc.month   = (short) month;
		alarm_desc.day     = (short) day;
		alarm_desc.year    = (short) year;
		alarm_desc.hour    = (short) hour;
		alarm_desc.minute  = (short) min;
		alarm_desc.second  = (short) sec;
		alarm_desc.code    = (short) set_clear_flag;
		alarm_desc.alarm   = (short) code;
		alarm_desc.channel = (short) channel_number;

		rda_alarm_entry = (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data (code);

                if( rda_alarm_entry != NULL ){

   		    if (log_flag) {

			unsigned int alarm_type = (GL_STATUS | GL_ERROR);

			/* Set the RDA alarm level for this alarm. */
			switch( rda_alarm_entry->state ){

			    case ORPGRDA_STATE_NOT_APPLICABLE:
				alarm_type |= LE_RDA_AL_NOT_APP;
				break;

			    case ORPGRDA_STATE_MAINTENANCE_MANDATORY:
				alarm_type |= LE_RDA_AL_MAM;
				break;

			    case ORPGRDA_STATE_MAINTENANCE_REQUIRED:
				alarm_type |= LE_RDA_AL_MAR;
				break;

			    case ORPGRDA_STATE_INOPERATIVE:
				alarm_type |= LE_RDA_AL_INOP;
				break;

			    case ORPGRDA_STATE_SECONDARY:
				alarm_type |= LE_RDA_AL_SEC;
				break;

			    default:
				alarm_type |= LE_RDA_AL_SEC;
				break;

			}

		    	if (set_clear_flag) {

				LE_send_msg (alarm_type, "%s %s\n",
					ORPGINFO_RDA_ALARM_ACTIVATED, rda_alarm_entry->alarm_text);
		    	} else {

				LE_send_msg (alarm_type, "%s %s\n",
					ORPGINFO_RDA_ALARM_CLEARED, rda_alarm_entry->alarm_text);

		    	}
		    }

		    if (verbose_flag)					       
			fprintf(stderr, "Write alarm %d:%s\n", code, rda_alarm_entry->alarm_text);				

/*		    Write out the new alarm message.			*/

		    status = ORPGDA_write (ORPGDAT_RDA_ALARMS, 
		  		           (char *) &alarm_desc,
				           sizeof (RDA_alarm_t),
				           LB_ANY);

		    if (status < 0)
			fprintf(stderr, "ORPGDA_write: %d\n", status);

		    if ((status = EN_post(ORPGEVT_RDA_ALARMS_UPDATE,NULL,0, EN_POST_FLAG_LOCAL)) < 0) 
		        fprintf(stderr, "EN_post error = %d \n", status);

                }
		    		
		/*  Wait specified number of seconds */
		sleep(frequency);
		    		
/*		Increment the alarm code if we are repeating block.	*/

	        code++;

		if (code > 800)
			code = 1;
	}
	
	LB_close (lbfd);	

        return 0;
}
