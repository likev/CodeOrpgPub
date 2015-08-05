
/******************************************************************

	file: rpg_ps.c

	This is the main module for the rpg_ps (RPG process status) 
	program.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/07/14 20:23:16 $
 * $Id: rpg_ps.c,v 1.23 2011/07/14 20:23:16 jing Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

#include <mrpg.h>

static int Performance_monitoring;	/* performance monitoring mode */
static int Print_infr_processes;	/* printing option - see usage */
static int Print_all_processes;		/* printing option - see usage */
static int Print_full_command_line;	/* printing option - see usage */

static int Ps_received = 0;		/* RPG process status received */
static int End_elevation = 0;		/* RPG elevation scan ended */
static int New_volume = 0;		/* RPG new volume started */

static Orpgevt_scan_info_t Elev_info;
static Orpgevt_scan_info_t Vol_info;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Print_rpg_ps ();
static int Get_rpg_ps ();
static void Callback (int fd, LB_id_t msgid, int msg_info, void *arg);
static int Print_rpg_state ();
static void Perf_monitor ();
static void An_callback (EN_id_t evtcd, char *msg, int msglen, void *arg);
static void Print_volume_info ();
static void Print_elevation_info ();
static void Print_ps_info ();
static void Truncate_string (char *name, int size);
static int Is_infr_process (char *name);

#ifdef LITTLE_ENDIAN_MACHINE
char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len );
#endif


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (0);

    /* This is added to avoid annoying warning message in output. */
    LE_set_foreground();
    LE_init( argc, argv );

    if (Performance_monitoring)
	Perf_monitor ();

    if (Get_rpg_ps () != 0 ||
	Print_rpg_ps () < 0 ||
	Print_rpg_state () != 0)
	exit (1);
    exit (0);
}

/******************************************************************

    Sends RPG process status request and waits until the result to
	come.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Get_rpg_ps () {
    int ret;
    time_t st;

    Ps_received = 0;
    ret = ORPGDA_UN_register (ORPGDAT_TASK_STATUS, MRPG_PS_MSGID, Callback);
    if (ret < 0) {
	fprintf (stderr, 
	    "ORPGDA_UN_register process status failed (ret %d)\n", ret);
	return (-1);
    }
    if (ORPGMGR_send_command (MRPG_STATUS) < 0 &&
	MISC_system ("mrpg -b status") != 0) {
	fprintf (stderr, "Sending process status request failed\n");
	return (-1);
    }

    st = time (NULL);
    while (!Ps_received) {
	time_t t;

	sleep (2);
	t = time (NULL);
	if (t >= st + 4) {
	    printf ("Waiting (mrpg may be busy)\n");
	    st = t;
	}
    }
    return (0);
}

/******************************************************************

    The RPG status message UN callback function.

    Input:	See LB man page.
	
******************************************************************/

static void Callback (int fd, LB_id_t msgid, int msg_info, void *arg) {
    Ps_received = 1;
}

/******************************************************************

    Reads and prints RPG process status.

    Return:	0 on success or -1 on failure.
	
******************************************************************/
static int Print_rpg_ps () {
    int len, title_printed;
    char *buf, *cpt;

    len = ORPGDA_read (ORPGDAT_TASK_STATUS, 
			(char *)&buf, LB_ALLOC_BUF, MRPG_PS_MSGID);
    if (len < 0) {
	fprintf (stderr, "ORPGDA_read process status failed (ret %d)\n", len);
	return (-1);
    }
    if (len == 0) {
	printf ("No operational process is running\n");
	return (0);
    }

    title_printed = 0;
    cpt = buf;
    while (1) {
	Mrpg_process_status_t *ps;
	char pid[16], cpu[16], mem[16], life[16];

	ps = (Mrpg_process_status_t *)cpt;
	if (cpt - buf + sizeof (Mrpg_process_status_t) > len ||
	    cpt - buf + ps->size > len ||
	    ps->name_off == 0)
	    break;

	if (ps->name_off == 0) {
	    printf ("Process info not found - try later\n");
	    break;
	}

	if (Is_infr_process (cpt + ps->name_off)) {
	    if (!Print_infr_processes && !Print_all_processes) {
		cpt += ps->size;
		continue;
	    }
	}
	else {
	    if (Print_infr_processes) {
		cpt += ps->size;
		continue;
	    }
	}

	if (ps->pid < 0)
	    strcpy (pid, "  FAIL");
	else
	    sprintf (pid, "%6d", ps->pid);

	if (ps->mem >= 10 * 1024)
	    sprintf (mem, "%6dM", (ps->mem + 512) / 1024);
	else if (ps->mem > 0)
	    sprintf (mem, "%6dK", ps->mem);
	else
	    sprintf (mem, "    n/a");

	if (ps->cpu >= 10 * 1000000)
	    sprintf (cpu, "%5dk", (ps->cpu + 500000) / 1000000);
	else if (ps->cpu >= 10 * 1000)
	    sprintf (cpu, "%5ds", (ps->cpu + 500) / 1000);
	else if (ps->cpu >= 0)
	    sprintf (cpu, "%5dm", ps->cpu);
	else
	    sprintf (cpu, "    n/a");

	if (ps->life >= 10 * 86400)
	    sprintf (life, "%5dd", (ps->life + 43200) / 86400);
	else if (ps->life >= 10 * 3600)
	    sprintf (life, "%5dh", (ps->life + 1800) / 3600);
	else if (ps->life >= 10 * 60)
	    sprintf (life, "%5dm", (ps->life + 30) / 60);
	else
	    sprintf (life, "%5ds", ps->life);

	if (!Print_full_command_line)
	    Truncate_string (cpt + ps->cmd_off, 24);

	if (*(cpt + ps->node_off) != '\0') {	/* node info available */
	    if (!title_printed)
		printf (
	    "                name    pid   cpu    mem  life  node command\n");
	    printf ("%20s %s%s%s%s %5s %s\n", 
		cpt + ps->name_off, 
		pid, cpu, mem, life, cpt + ps->node_off, cpt + ps->cmd_off);
	}
	else {
	    if (!title_printed)
		printf (
	    "                name    pid   cpu    mem  life command\n");
	    printf ("%20s %s%s%s%s %s\n", 
		cpt + ps->name_off,
		pid, cpu, mem, life, cpt + ps->cmd_off);
	}
	title_printed = 1;
	cpt += ps->size;

    }
    return (0);
}

/******************************************************************

    Checks if process "name" is an infrustructure process. Returns 
    1 if it is or 0 if it is not.
	
******************************************************************/

static int Is_infr_process (char *name) {
    if (strcmp (name, "rssd") == 0 ||
	strcmp (name, "nds") == 0 ||
	strcmp (name, "bcast") == 0 ||
	strcmp (name, "brecv") == 0 ||
	strcmp (name, "lb_rep") == 0)
	return (1);
    return (0);
}

/******************************************************************

    Trancates string "name" to size of "size".
	
******************************************************************/

static void Truncate_string (char *name, int size) {
    int len;

    len = strlen (name);
    if (len > size) {
	name[size] = '\0';
	name[size - 1] = '.';
    }
}

/******************************************************************

    Reads and prints RPG state info.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Print_rpg_state () {
    Mrpg_state_t state;
    int ret;

    if ((ret = ORPGDA_read (ORPGDAT_TASK_STATUS, (char *)&state, 
		sizeof (Mrpg_state_t), MRPG_RPG_STATE_MSGID)) != 
						sizeof (Mrpg_state_t)) {
	fprintf (stderr, 
		"ORPGDA_read MRPG_RPG_STATE_MSGID failed (ret %d)\n", ret);
	return (-1);
    }
    printf ("RPG: ");

    switch (state.state) {
	case MRPG_ST_SHUTDOWN:
	printf ("Shutdown state - ");
	break;
	case MRPG_ST_STANDBY:
	printf ("Standby state - ");
	break;
	case MRPG_ST_OPERATING:
	printf ("Operating state - ");
	break;
	case MRPG_ST_FAILED:
	printf ("Failed state - ");
	break;
	case MRPG_ST_POWERFAIL:
	printf ("Powerfailure state - ");
	break;
	case MRPG_ST_TRANSITION:
	printf ("Transition state - ");
	break;
    }

    if (state.active)
	printf ("Active - ");
    else
	printf ("Inactive - ");

    if (state.test_mode)
	printf ("In Test Mode");
    else
	printf ("In Operational Mode");
    printf ("\n");
    return (0);
}

/******************************************************************

    Reporting process status every volume (trigged by the volume 
    start event).
	
******************************************************************/

static void Perf_monitor () {
    int ret;

    New_volume = 0;
    ret = ORPGDA_UN_register (ORPGDAT_TASK_STATUS, MRPG_PS_MSGID, Callback);
    if (ret < 0) {
	fprintf (stderr, 
	    "ORPGDA_UN_register process status failed (ret %d)\n", ret);
	exit (1);
    }
    ret = EN_register (ORPGEVT_SCAN_INFO, An_callback);
    if (ret < 0) {
	fprintf (stderr, 
	    "EN_register scan info event failed (ret %d)\n", ret);
	exit (1);
    }

#ifdef LITTLE_ENDIAN_MACHINE
    /* Set byte swapping function. */
    EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_msg);
#endif

    printf ("    name instance_num pid cpu(ms) mem(bytes) life(seconds) node\n");

    while (1) {
	time_t st;

	if (!End_elevation && !New_volume) {
	    msleep (1000);
	    continue;
	}

        if( End_elevation ){

	   Print_elevation_info ();
	   Ps_received = 0;
	   if (ORPGMGR_send_command (MRPG_STATUS) < 0) {
	       fprintf (stderr, 
	   	   "ORPGMGR_send_command MRPG_STATUS failed\n");
	       exit (1);
	   }

	   /* wait for response */
	   st = time (NULL);
	   while (!Ps_received) {
	       msleep (1000);
	       if (time (NULL) >= st + 10) {
	   	   fprintf (stderr, "RPG process info not found\n");
		   exit (1);
	       }
	   }
	   Print_ps_info ();

	   End_elevation = 0;

        }
        else if( New_volume ){

	   Print_volume_info ();
           New_volume = 0;

        }
    }
}

/******************************************************************

    The RPG event callback function.

    Input:	See LB man page.
	
******************************************************************/
static void An_callback (EN_id_t evtcd, char *msg, int msglen, void *arg) {

    Orpgevt_scan_info_t *scan_info;

    if (evtcd == ORPGEVT_SCAN_INFO){

        scan_info = (orpgevt_scan_info_t *) msg;
        if( (scan_info->key == ORPGEVT_END_ELEV) 
                            || 
            (scan_info->key == ORPGEVT_END_VOL) ){

	   End_elevation = 1;
           memcpy( &Elev_info, msg, sizeof(Orpgevt_scan_info_t) );

        }
        else if (scan_info->key == ORPGEVT_BEGIN_VOL){

           New_volume = 1;
           memcpy( &Vol_info, msg, sizeof(orpgevt_scan_info_t) );

        }

    }
        
}

/******************************************************************

    Prints volume info.
	
******************************************************************/
static void Print_volume_info () {

    time_t vol_time;
    int yy, mon, dd, hh, min, ss;

    vol_time = UNIX_SECONDS_FROM_RPG_DATE_TIME 
				(Vol_info.data.date, Vol_info.data.time);
    unix_time (&vol_time, &yy, &mon, &dd, &hh, &min, &ss);

    printf ("Volume: # %d  %.2d/%.2d/%.2d %.2d:%.2d:%.2d  vcp %d\n", 
	(int)Vol_info.data.vol_scan_number, mon, dd, (yy - 1900) % 100, hh, min, ss, 
	Vol_info.data.vcp_number);

    return;
}

/******************************************************************

    Prints elevation info.
	
******************************************************************/
static void Print_elevation_info () {

    time_t elev_time;
    int yy, mon, dd, hh, min, ss;

    elev_time = UNIX_SECONDS_FROM_RPG_DATE_TIME 
				(Elev_info.data.date, Elev_info.data.time);
    unix_time (&elev_time, &yy, &mon, &dd, &hh, &min, &ss);

    printf ("Elevation: # %d  %.2d/%.2d/%.2d %.2d:%.2d:%.2d  vcp %d\n", 
	(int)Elev_info.data.elev_cut_number, mon, dd, (yy - 1900) % 100, hh, min, ss, 
	Elev_info.data.vcp_number);

    return;
}

/******************************************************************

    Reads and prints RPG process status. Version for performance
    monitoring.
	
******************************************************************/
static void Print_ps_info () {
    int len;
    char *buf, *cpt;
    static char node_name[6];

    len = ORPGDA_read (ORPGDAT_TASK_STATUS, 
			(char *)&buf, LB_ALLOC_BUF, MRPG_PS_MSGID);
    if (len < 0) {
	fprintf (stderr, "ORPGDA_read process status failed (ret %d)\n", len);
	exit (1);
    }
    if (len == 0) {
	free (buf);
	return;
    }

    cpt = buf;
    while (1) {
	Mrpg_process_status_t *ps;

	ps = (Mrpg_process_status_t *)cpt;
	if (cpt - buf + sizeof (Mrpg_process_status_t) > len ||
	    cpt - buf + ps->size > len ||
	    ps->name_off == 0)
	    break;

	if (ps->name_off == 0)
	    break;

        if( *(cpt + ps->node_off) == '\0' )
          sprintf( node_name, "rpga1" );

        else
           sprintf( node_name, "%s", cpt + ps->node_off );

	printf ("%s %d %d %d %d %d %5s\n", 
		cpt + ps->name_off, ps->instance, 
		ps->pid, ps->cpu, ps->mem * 1024, ps->life, node_name);
	cpt += ps->size;
    }
    free (buf);
}

 
/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
/*    extern char *optarg; */	/* used by getopt */
/*    extern int optind; */
    int c;			/* used by getopt */
    int err;			/* error flag */

    Performance_monitoring = 0;
    Print_infr_processes = 0;
    Print_all_processes = 0;
    Print_full_command_line = 0;
    err = 0;
    while ((c = getopt (argc, argv, "psafh?")) != EOF) {
	switch (c) {

	    case 'p':
		Performance_monitoring = 1;
		break;

	    case 's':
		Print_infr_processes = 1;
		break;

	    case 'a':
		Print_all_processes = 1;
		break;

	    case 'f':
		Print_full_command_line = 1;
		break;

	    case 'h':
	    case '?':
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
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("Prints RPG process info. The units for cpu are k (kilo-seconds),\n");
    printf ("s (seconds) or m (milli-seconds). The units for memory are \n");
    printf ("M (mega-bytes), K (kilo-bytes) or B (bytes). The units for life\n");
    printf ("time are d (days), h (hours), m (minutes) or s (seconds).\n");
    printf ("Options:\n");
    printf ("     -s (print infr processes only)\n");
    printf ("     -a (print all processes including infr processes)\n");
    printf ("     -f (print lines without truncation)\n");
    printf ("     -p (volume event trigged reporting for performance monitoring)\n");
    exit (0);
}

#ifdef LITTLE_ENDIAN_MACHINE
/**************************************************************************

   Description:
      Byte-swapping function for scan info event.

   Note:
      This is required on LITTLE_ENDIAN_MACHINE since the message is
      passed Big Endian format.

**************************************************************************/
char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len ){

   static char swapped_msg[ ORPGEVT_SCAN_INFO_DATA_LEN ];
   int num_ints = msg_len / sizeof( int );

   if( (event == ORPGEVT_SCAN_INFO) && (msg != NULL) ){

      MISC_bswap( sizeof(int), msg, num_ints, swapped_msg ); 
      memcpy( msg, swapped_msg, msg_len );
      return( msg );

   }

   return( msg );
   
}

#endif
