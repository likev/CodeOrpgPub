
/******************************************************************

	file: manage_ldm.c

	This is the main module for managing LDM-related tasks.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/05/11 15:17:32 $
 * $Id: manage_ldm.c,v 1.16 2011/05/11 15:17:32 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <archive_II.h> 
#include <infr.h> 

#define STATUS_PUBLISH_PERIOD	10
#define ARCHIVE_II_DEA_NAME	"alg.Archive_II"

static int Start_ldm_only = 0;
static int Stop_ldm_only = 0;
static int In_termination = 0;

static int Terminate (int signal, int sigtype);
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Start_ldm_server ();
static int Stop_ldm_server ();
static void Publish_status ();

/******************************************************************

    Description: The main function of manage_ldm.

******************************************************************/

int main (int argc, char **argv) {
    int ret, failed;
    time_t publish_time;

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    /* Initialize the LE and CS services */
    ret = ORPGMISC_init (argc, argv, 1000, LB_SINGLE_WRITER, -1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMISC_init failed (ret %d)", ret);
	exit (1) ;
    }

    if (Stop_ldm_only) {
	if (Stop_ldm_server () < 0)
	    exit (1);
	exit (0);
    }

    failed = 0;
    if (Start_ldm_server () < 0)
	failed = 1;
    if (Start_ldm_only)
	exit (failed);

    ret = ORPGTASK_reg_term_handler (Terminate);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGTASK_reg_term_hdlr failed: %d", ret) ;
	exit (1) ;
    }

    /* report ready-for-operation and wait until RPG is in operational mode */
    ORPGMGR_report_ready_for_operation ();
    if (failed)
	exit (1);

    LE_send_msg (GL_INFO, "Starting operation");
    Publish_status ();
    publish_time = MISC_systime (NULL);
    while (1) {
	time_t cr_t;

	if (In_termination) {
	    LE_send_msg (GL_INFO, "Stopping LDM before termination...");
	    if (Stop_ldm_server () < 0)
		exit (1);
	    exit (0);
	}
	cr_t = MISC_systime (NULL);
	if (cr_t >= publish_time + STATUS_PUBLISH_PERIOD) {
	    Publish_status ();
	    publish_time = cr_t;
	}
	sleep (1);
    }
    exit (0);
}

/**************************************************************************

    Publishes LDM status.

**************************************************************************/

static void Publish_status () {
    static char *get_status = "sh -c (netstat -an | grep ESTABLISHED | awk '{if($4 ~ /:388$/){print $5}}' | awk -F: '{print $1}' | sort -u)";
    static int initialized = 0;
    ArchII_status_t st;
    int ret, len;
    char buf[1024];

    if (!initialized) {
	if ((ret = ORPGDA_open (ORPGDAT_BDDS_STATUS, LB_WRITE)) < 0) {
            LE_send_msg (GL_ERROR, 
			"ORPGDA_open ORPGDAT_BDDS_STATUS failed (%d)", ret);
	    return;
	}
	initialized = 1;
    }

    st.status = ARCHIVE_II_RUNNING;
    st.ctime = time (NULL);

    st.num_clients = 0;
    ret = MISC_system_to_buffer (get_status, buf, 1024, &len);
    if (ret != 0)
	LE_send_msg (GL_ERROR, 
			"MISC_system_to_buffer get_status failed (%d)", ret);
    else if (len > 0) {
	int off, total;
	char tok[128];
	char *p = buf;
	total = 0;
	while ((off = MISC_get_token (p, "S\n", 0, tok, 128)) > 0) {
	    int l = strlen (tok);
	    if (l > 0 &&
		total + l + 1 <= MAX_LDM_CLIENT_INFO_SIZE) {
		strcpy (st.buf + total, tok);
		total += l + 1;
		st.num_clients++;
	    }
	    p += off;
	}
    }

    ret = ORPGDA_write (ORPGDAT_BDDS_STATUS, (char *) &st, 
                          sizeof (ArchII_status_t), BDDS_LDM_STATUS_ID);
    if (ret < 0)
    {
        LE_send_msg (GL_ERROR, 
          "Publish_status ORPGDA_write BDDS_LDM_STATUS_ID failed (%d)", ret);
    }

    return;
}

/**************************************************************************

    Starts the LDM server - Cleaning up existing processes, checking/creating
    LDM queue and starting LDM processes. Returns 0 on success of -1 on
    failure.

**************************************************************************/

static int Start_ldm_server () {
    static char *start_ldm_cmd = "sh -c (				\n\
									\n\
	echo \"Starting convert_ldm\"					\n\
	convert_ldm &							\n\
	echo \"Starting levelII_stats_ICAO_ldmping\"			\n\
	levelII_stats_ICAO_ldmping &					\n\
	exit 0								\n\
    )";
    char buf[1024];
    int ldmret, ret, len;
    ArchII_status_t st;

    /* Stop ldm server first. */
    Stop_ldm_server();

    LE_send_msg (GL_INFO, "In Start_ldm_server, starting LDM-related tasks.");
    ldmret = MISC_system_to_buffer (start_ldm_cmd, buf, 1024, &len);
    if (len > 0)
	LE_send_msg (GL_INFO, "%ms", buf);
    if (ldmret != 0) {
	LE_send_msg (GL_ERROR, 
		"MISC_system_to_buffer start_ldm_cmd failed (%d)", ldmret);
        st.status = ARCHIVE_II_FAIL;
    }
    else {
        st.status = ARCHIVE_II_RUNNING;
    }

    st.ctime = time (NULL);
    st.num_clients = 0;
    ret = ORPGDA_write (ORPGDAT_BDDS_STATUS, (char *) &st, 
                          sizeof (ArchII_status_t), BDDS_LDM_STATUS_ID);
    if (ret < 0)
    {
      LE_send_msg (GL_ERROR, 
        "Start_ldm_server ORPGDA_write BDDS_LDM_STATUS_ID failed (%d)", ret);
    }

    if( ldmret != 0 ){ return( -1 ); }

    return (0);
}

/**************************************************************************

    Stops the LDM server - Cleaning up existing processes. Returns 0 on 
    success of -1 on failure.

**************************************************************************/

static int Stop_ldm_server () {
    static char *stop_ldm_cmd = "sh -c (					\n\
									\n\
	echo \"Terminating convert_ldm\"				\n\
	prm -9 convert_ldm						\n\
	echo \"Terminating levelII_stats_ICAO_ldmping\"			\n\
	prm -9 levelII_stats_ICAO_ldmping				\n\
	echo \"Terminating LDM server\"					\n\
	exit 0								\n\
    )";
    char buf[1024];
    int ldmret, ret, len;
    ArchII_status_t st;

    LE_send_msg (GL_INFO, "In Stop_ldm_server, Stopping LDM-related tasks.");
    ldmret = MISC_system_to_buffer (stop_ldm_cmd, buf, 1024, &len);
    if (len > 0)
	LE_send_msg (GL_INFO, "%ms", buf);
    if (ldmret != 0) {
	LE_send_msg (GL_ERROR, 
		"MISC_system_to_buffer stop_ldm_cmd failed (%d)", ldmret);
    }

    st.status = ARCHIVE_II_SHUT_DOWN;
    st.ctime = time (NULL);
    st.num_clients = 0;
    ret = ORPGDA_write (ORPGDAT_BDDS_STATUS, (char *) &st, 
                          sizeof (ArchII_status_t), BDDS_LDM_STATUS_ID);
    if (ret < 0)
    {
      LE_send_msg (GL_ERROR, 
        "Stop_ldm_server ORPGDA_write BDDS_LDM_STATUS_ID failed (%d)", ret);
    }

    if( ldmret != 0 ){ return( -1 ); }

    return (0);
}

/**************************************************************************

    Description: This function terminates this process.

**************************************************************************/

static int Terminate (int signal, int sigtype) {

    if (signal == SIGTERM) {
	LE_send_msg (GL_INFO, "SIGTERM received");
	In_termination = 1;
	return (1);
    }

    LE_send_msg (GL_INFO,  "manage_ldm terminates");
    return (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "sth?")) != EOF) {
	switch (c) {

	    case 's':
		Start_ldm_only = 1;
		break;

	    case 't':
		Stop_ldm_only = 1;
		break;

	    case 'h':
	    case '?':
	    default:
		Print_usage (argv);
		break;
	}
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {

    printf ("Usage: %s [options]\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -s (Start LDM server)\n");
    printf ("       -t (Stop LDM server)\n");
    printf ("       -h (Print usage info and terminate)\n");
    exit (0);
}



