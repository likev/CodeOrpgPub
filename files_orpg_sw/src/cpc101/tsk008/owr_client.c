/****************************************************************
		
    This module implements the client part of the one-way-replicator
    (owr).

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2013/02/19 20:35:15 $
 * $Id: owr_client.c,v 1.11 2013/02/19 20:35:15 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/* System include files */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <orpg.h>
#include <infr.h>
#include "owr_def.h"

enum {MRPGDBM_STOP, MRPGDBM_START, MRPGDBM_KILL};

#define INIT_TIME 50			/* max seconds required for init LBs */
#define CONN_TIME 30	/* max seconds required for connecting to server */

static int Chan_number = 1;
static int Port_number = 0;
static char *Conf_file = NULL;
int Verbose = 0;
static char *Sever_name = NULL;
static int Cid = -1;
static EN_id_t Owr_cmd_event = 23;
static int Use_le = 0;
int Terminating = 0;
static time_t Receive_tm = 0;		/* time receiving the latest msg */
static time_t Send_tm = 0;		/* time sending the latest msg */
static int Rpg_cfg_changed = 0;
static int Auto_start = 0;
static time_t Init_active_time = 0;	/* estimated end time init is active */
static time_t Conn_tm = 0;		/* time connection established */
static time_t Start_time = 0;		/* time starting processing */

static void Print_usage (char **argv);
static int Read_options (int argc, char **argv);
static int Process_response (int len, char *resp);
static void En_ready_callback (int fd, int ready_flag);
static void En_callback (EN_id_t event, char *msg, int len, void *arg);
static void Gen_sys_cfg (char *cfg, int ch, int pid);
static char *Add_cfg_dir (char *cfg);
static void Signal_handler (int sig);
static void Restart (int ws);
static void Send_keep_alive ();
static void Manage_rpgdbm (int func, char *sys_cfg);
static void Launch_owr_client ();


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    char buf[MAX_STR_SIZE], *b;
    int en_fd, ret;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Auto_start)
	Launch_owr_client ();

    sigset (SIGTERM, Signal_handler); 
    sigset (SIGHUP, Signal_handler);
    sigignore (SIGPIPE);
    OC_prepare_for_restart (argc, argv);
    EN_control (EN_SET_SIGNAL, EN_NTF_NO_SIGNAL);
    Init_active_time = MISC_systime (NULL) + 8;
	/* 8 seconds for init this program and register events */

    if (Use_le) {
	char name[MAX_STR_SIZE];
	sprintf (name, "%s.%d", MISC_basename (argv[0]), Chan_number);
	LE_set_option ("LE name", name);
	LE_set_option ("LB type", LB_SINGLE_WRITER);
	if ((ret = LE_init (argc, argv)) < 0) {
	    if (ret == LE_DUPLI_INSTANCE) {
		if (getppid () != 1)
		    MISC_log ("Another owr_client for chan %d is running\n",
					Chan_number);
		exit (2);
	    }
	    else if (getppid () != 1)
		MISC_log ("LE_init failed (%d)\n", ret);
	    exit (1);
	}
    }

    MISC_log ("owr client for channel %d starts\n", Chan_number);
    Manage_rpgdbm (MRPGDBM_KILL, NULL);

    if (ORPGMISC_get_site_value ("FAA", buf, MAX_STR_SIZE) == 0 &&
	strcmp (buf, "YES") == 0) {
	MISC_log ("EN_SET_AN_GROUP %d for FAA site\n", Chan_number);
	EN_control (EN_SET_AN_GROUP, Chan_number);
    }
    ret = EN_register (ORPGEVT_DATA_STORE_CREATED, En_callback);
    if (ret < 0) {
	MISC_log ("EN_register ORPGEVT_DATA_STORE_CREATED failed (%d)\n", ret);
	exit (1);
    }

    b = Add_cfg_dir ("a");	/* make sure CFG_DIR exists */
    if (b[0] == '.') {
	MISC_log ("CFG_DIR is not found\n");
	exit (1);
    }

    CF_read_config (Conf_file);

    if (Port_number == 0)
	Port_number = OC_get_env_port ();
    if (Port_number == 0) {
	MISC_log ("Port number is not found\n");
	exit (1);
    }

    if (Sever_name != NULL)
	sprintf (buf, "%s:%d", Sever_name, Port_number);
    else
	sprintf (buf, "%d", Port_number);

    Cid = CSS_set_server_address ("service_1", buf, CSS_ANY_LINK);
    if (Cid < 0) {
	MISC_log (
		"CSS_set_server_address (%s) failed (ret %d)\n", buf, Cid);
	exit (1);
    }
    MISC_log ("CSS_set_server_address: %s\n", buf);

    en_fd = EN_control (EN_GET_NTF_FD);
    if (en_fd < 0) {
	MISC_log ("Connecting to event server failed (%d)\n", en_fd);
	exit (1);
    }
    ret = CSS_add_poll_fd (en_fd, POLL_IN_RFLAGS, En_ready_callback);
    if (ret < 0) {
	MISC_log ("CSS_add_poll_fd failed (%d)\n", ret);
	exit (1);
    }
    EN_control (EN_SET_SENDER_ID, getpid ());

    ret = EN_register (Owr_cmd_event, En_callback);
    if (ret < 0) {
	MISC_log ("Failed in EN_register (%d)\n", ret);
	exit (1);
    }
    MISC_log ("Event %d registered\n", Owr_cmd_event);
    if (CF_prereg_ans () < 0)
	exit (1);

    if (CF_send_lb_reqs () < 0) {
	MISC_log ("Failed in requesting LB rep service\n");
	exit (1);
    }
    Start_time = MISC_systime (NULL);
    Init_active_time = Start_time + 5;
	/* 5 seconds for getting the firs LB's, a small one, rep info */

    while (1) {
	int rid;
	unsigned int resp_seq;
	char *rresp;
	extern int All_lb_created;
	static int started_reported = 0;

	rid = CSS_wait (1000, NULL);
	if (rid < 0) {
	    MISC_log ("CSS_wait failed (ret %d)\n", rid);
	    Restart (10);
	}

	if (rid > 0) {
	    int len;
	    if (rid != Cid) {
		MISC_log ("Unexpected cid returned (%d != %d)\n", rid, Cid);
		continue;
	    }
	    len = CSS_get_response (Cid, &rresp, &resp_seq);
	    if (len == CSS_LOST_CONN) {
		MISC_log ("Lost connection %d (CSS_get_response)\n", Cid);
		Restart (30);
	    }
	    if (len < 0) {
		MISC_log ("CSS_get_response failed (ret %d)\n", len);
		Restart (30);
	    }
	    Receive_tm = MISC_systime (NULL);
	    if (Conn_tm == 0)
		Conn_tm = Receive_tm;
	    Process_response (len, rresp);
	    if (rresp != NULL)
		free (rresp);
	}

	CF_report_statistics ();

	if (Receive_tm > 0) {
	    time_t crt = MISC_systime (NULL);
	    if (crt > Receive_tm + 2 * KEEP_ALIVE_TIME) {
		MISC_log ("Connection to server not kept\n");
		Restart (30);
	    }
	    if (crt >= Send_tm + KEEP_ALIVE_TIME)
		Send_keep_alive ();
	    if (crt > Conn_tm + INIT_TIME && !All_lb_created) {
		MISC_log ("Server does not respond to init request\n");
		Restart (10);
	    }
	}
	else if (MISC_systime (NULL) > Start_time + CONN_TIME) {
	    MISC_log ("Server is not responding\n");
	    Restart (5);
	}

	if (Rpg_cfg_changed) {
	    MISC_log ("RPG configuration changed\n");
	    Restart (30);
	}
	if (!started_reported && All_lb_created) {
	    char buf[MAX_STR_SIZE];
	    sprintf (buf, "owr_client for channel %d restarted", Chan_number);
	    MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
	    EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
	    started_reported = 1;
	}
    }

    Restart (10);
}

/******************************************************************

    Sends a keep-alive message to the server.

******************************************************************/

static void Send_keep_alive () {
    Message_t msg;

    memset (&msg, 0, sizeof (Message_t));
    msg.type = KEEP_ALIVE;
    msg.size = sizeof (Message_t);
    CL_send_to_server ((char *)&msg, msg.size);
}

/******************************************************************

    Signal handler.

******************************************************************/

static void Signal_handler (int sig) {

    MISC_log ("Signal %d received and ignored\n", sig);
}

/******************************************************************

    Sends an event to the user, stops normal processing, waits "ws"
    seconds, restarts this program and terminates.

******************************************************************/

static void Restart (int ws) {
    char buf[MAX_STR_SIZE];
    time_t st_t;

    sprintf (buf, "owr_client for channel %d restarting", Chan_number);
    MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
    EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);

    MISC_log ("owr_client stops working\n");
    Terminating = 1;
    CSS_close (Cid);

    Manage_rpgdbm (MRPGDBM_STOP, NULL);

    st_t = MISC_systime (NULL);
    while (1) {
	EN_control (EN_WAIT, 200);
	msleep (50);
	if (MISC_systime (NULL) >= st_t + ws)
	    break;
    }

    MISC_log ("owr_client restarting itself...\n");
    OC_restart ();
}

/************************************************************************

    Processes response "req" of "req_len" bytes. Generates the response
    and sends it to the client.

************************************************************************/

static int Process_response (int len, char *resp) {
    Message_t *msg;

    msg = (Message_t *)resp;
    if (len < sizeof (Message_t) || len > msg->size) {
	MISC_log ("Bad RESP_LB_REP msg - Wrong size %d\n", len);
	return (-1);
    }
    switch (msg->type) {

	case RESP_LB_REP:
	    Init_active_time = MISC_systime (NULL) + 20;
		/* 20 seconds are sufficient for 250K LB on 128 bps line */
	    CF_process_resp_lb_rep (len, msg);
	    break;

	case LB_UPDATE:
	    CF_lb_update (len, msg);
	    break;

	case KEEP_ALIVE:
	    break;

	default:
	    MISC_log ("Unexpected message type %d from server\n", msg->type);
	    break;
    }

    return (0);
}

/**************************************************************************

    Starts additional processes to support the applications. This can be
    in the conf file in the future.

**************************************************************************/

static void Manage_rpgdbm (int func, char *sys_cfg) {
    int ret;

    if (func == MRPGDBM_START) {
	char cmd[MAX_STR_SIZE];
	if (Chan_number == 2)
	    sprintf (cmd, "rpgdbm -v -m -p 2 -g %s &", sys_cfg);
	else
	    sprintf (cmd, "rpgdbm -v -m -g %s &", sys_cfg);
	ret = MISC_system_to_buffer (cmd, NULL, 0, NULL);
	if (ret < 0)
	    MISC_log ("Failed in lauching rpgdbm (%d, %s)\n", ret, cmd);
    }
    else {
	char cmd[MAX_STR_SIZE];
	if (func == MRPGDBM_KILL)
	    strcpy (cmd, "prm -9 \"-pat-");
	else
	    strcpy (cmd, "prm \"-pat-");
	if (Chan_number == 2)
	    sprintf (cmd + strlen (cmd), "-v -m -p 2 -g\" rpgdbm");
	else
	    sprintf (cmd + strlen (cmd), "-v -m -g\" rpgdbm");
	ret = MISC_system_to_buffer (cmd, NULL, 0, NULL);
	if (ret < 0)
	    MISC_log ("Failed in removing rpgdbm (%d, %s)\n", ret, cmd);
    }
}

/**************************************************************************

    Sends "msg" of "msg_size" bytes to the server.

**************************************************************************/

int CL_send_to_server (char *msg, int msg_size) {
    int ret;

    ret = CSS_send_request (Cid, msg, msg_size);
    if (ret == CSS_LOST_CONN) {
	MISC_log ("Lost connection cid = %d\n", Cid);
	Restart (30);
    }
    if (ret < 0) {
	MISC_log ("CSS_send_request failed (ret %d)\n", ret);
	Restart (30);
    }
    Send_tm = MISC_systime (NULL);
    return (0);
}

/**************************************************************************

    Returns the local path for LB path "path".

**************************************************************************/

char *CL_get_local_path (char *path) {
    static char *pb = NULL;
    char *dir, *p, *pt;

    if ((dir = getenv ("ORPGDIR")) == NULL) {
	MISC_log ("RPG data directory not defined - Use current dir\n");
	dir = ".";
    }
    p = pt = path;
    while (*pt != '\0') {	/* remove host names */
	if (*pt == ':')
	    p = pt + 1;
	pt++;
    }
    if (strncmp (p, dir, strlen (dir)) == 0)
	p += strlen (dir);
    pb = STR_copy (pb, dir);
    if (Chan_number == 2)
	pb = STR_cat (pb, "/ch2");
    else
	pb = STR_cat (pb, "/ch1");
    pb = STR_cat (pb, p);
    return (pb);
}

/************************************************************************

    Callback function when event fd has input data.

************************************************************************/

static void En_ready_callback (int fd, int ready_flag) {

    EN_control (EN_WAIT, 0);
}

/************************************************************************

    Callback function for processng events - owr commands and RPG events.

************************************************************************/

static void En_callback (EN_id_t event, char *msg, int len, void *arg) {
    extern int All_lb_created;
    char *is_ready, *gen_cfg;
    char tk[MAX_STR_SIZE];
    int ch, pid;

    if (Terminating)
	return;

    if (len > 0) {
	msg[len - 1] = '\0';
	MISC_log ("Event %d Rcvd: %s\n", event, msg);
    }
    else
	MISC_log ("Event %d Rcvd\n", event);
    if (event == ORPGEVT_DATA_STORE_CREATED) {
	Rpg_cfg_changed = 1;
	return;
    }

    if (len <= 0) {
	MISC_log ("Unexpected Empty command - Not processed\n");
	return;
    }
    is_ready = "Is owr ready for channel ";
    gen_cfg = "Generate sys_cfg for channel ";
    if (strncmp (msg, is_ready, strlen (is_ready)) == 0) {
	if (MISC_get_token (msg, "Ci", 5, &ch, 0) > 0 &&
	    ch == Chan_number &&
	    MISC_get_token (msg, "", 6, tk, MAX_STR_SIZE) > 0 &&
	    strcmp (tk, "-") == 0 &&
	    MISC_get_token (msg, "Ci", 7, &pid, 0) > 0) {
	    char buf[MAX_STR_SIZE];
	    if (All_lb_created)
		sprintf (buf, "%d - owr is ready for channel %d", pid, ch);
	    else if (MISC_systime (NULL) < Init_active_time)
		sprintf (buf, "%d - owr is initializing for channel %d", pid, ch);
	    else
		sprintf (buf, "%d - owr is not ready for channel %d", pid, ch);
	    MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
	    EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
	    return;
	}
    }
    else if (strncmp (msg, gen_cfg, strlen (gen_cfg)) == 0) {
	if (MISC_get_token (msg, "Ci", 4, &ch, 0) > 0 &&
	    ch == Chan_number &&
	    MISC_get_token (msg, "", 5, tk, MAX_STR_SIZE) > 0 &&
	    strcmp (tk, "from") == 0 &&
	    MISC_get_token (msg, "", 7, tk, MAX_STR_SIZE) > 0 &&
	    strcmp (tk, "-") == 0 &&
	    MISC_get_token (msg, "Ci", 8, &pid, 0) > 0 &&
	    MISC_get_token (msg, "", 6, tk, MAX_STR_SIZE) > 0) {

	    if (!All_lb_created) {
		char buf[MAX_STR_SIZE];
		sprintf (buf, "%d - sys_cfg for channel %d is not ready",
							pid, ch);
		MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
		EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
		return;
	    }
	    Gen_sys_cfg (tk, ch, pid);
	    return;
	}
    }
    MISC_log ("Unexpected command (%s) - Not processed\n", msg);
}

/************************************************************************

    Modifies "cfg" to generates a new system config file to use the 
    replicated LBs for channel "ch". "pid" is the command sender's pid. The
    responding event is generated if everything is fine or an error is
    detected.

************************************************************************/

static void Gen_sys_cfg (char *cfg, int ch, int pid) {
    static int n_owrcfgs = 0;
    static char *fname = NULL, *owrcfgs = NULL;
    char buf[MAX_STR_SIZE], *path, *cfgs;
    int i;
    FILE *fd, *ofd;

    fname = STR_copy (fname, cfg);
    fname = STR_cat (fname, ".owr");

    cfgs = owrcfgs;
    for (i = 0; i < n_owrcfgs; i++) {	/* already done before */
	if (strcmp (cfgs, fname) == 0) {
	    sprintf (buf, "%d - sys_cfg for channel %d generated: %s", 
					pid, ch, fname);
	    MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
	    EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
	    return;
	}
	cfgs += strlen (cfgs) + 1;
    }

    path = Add_cfg_dir (cfg);
    fd = fopen (path, "r");
    if (fd == NULL) {
	sprintf (buf, "%d - open %s failed", pid, path);
	MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
	EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
	return;
    }
    path = Add_cfg_dir (fname);
    ofd = fopen (path, "w");
    if (ofd == NULL) {
	sprintf (buf, "%d - Create %s failed", pid, path);
	MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
	EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
	return;
    }

    while (fgets (buf, MAX_STR_SIZE, fd) != NULL) {
	int did;
	char tk[MAX_STR_SIZE];

	if (MISC_get_token (buf, "Ci", 0, &did, 0) >= 0 &&
	    MISC_get_token (buf, "", 2, tk, MAX_STR_SIZE) <= 0 &&
	    MISC_get_token (buf, "", 1, tk, MAX_STR_SIZE) > 0) {
	    char *p = CF_get_local_path (did);
	    if (p != NULL)
		sprintf (buf, "%d\t:%s\n", did, p);
	}
	fprintf (ofd, "%s", buf);
    }
    fclose (fd);
    fclose (ofd);

    sprintf (buf, "%d - sys_cfg for channel %d generated: %s", 
					pid, ch, fname);
    MISC_log ("Post ev (%d): %s\n", Owr_cmd_event + 1, buf);
    EN_post_msgevent (Owr_cmd_event + 1, buf, strlen (buf) + 1);
    owrcfgs = STR_append (owrcfgs, fname, strlen (fname) + 1);
    n_owrcfgs++;
    if (n_owrcfgs == 1)
	Manage_rpgdbm (MRPGDBM_START, fname);
}

/************************************************************************

    Returns the full path of the system config file "cfg".

************************************************************************/

static char *Add_cfg_dir (char *cfg) {
    static char *p = NULL;
    char dir[MAX_STR_SIZE];

    if (cfg[0] == '/')		/* full path */
	return (cfg);

    if (MISC_get_cfg_dir (dir, MAX_STR_SIZE) < 0)
	strcpy (dir, ".");
    p = STR_copy (p, dir);
    p = STR_cat (p, "/");
    p = STR_cat (p, cfg);
    return (p);
}

/******************************************************************

    Launches owr_client for both channel when the corresponding RPG
    channel is up and running.

******************************************************************/

static void Launch_owr_client () {
    char buf[MAX_STR_SIZE];
    int done[3], n_chan;
    time_t crt;

    if (ORPGMISC_get_site_value ("SAT_CONN_MSCF", buf, MAX_STR_SIZE) != 0 ||
	strcmp (buf, "YES") != 0) {
	MISC_log ("This site is not SAT_CONN_MSCF\n");
	exit (0);
    }

    n_chan = 1;
    if (ORPGMISC_get_site_value ("FAA", buf, MAX_STR_SIZE) == 0 &&
	strcmp (buf, "YES") == 0)
	n_chan = 2;

    crt = MISC_systime (NULL);
    done[1] = 0;
    if (n_chan == 2)
	done[2] = 0;
    else
	done[2] = 1;
    while (1) {
	char mrpg_n[MAX_STR_SIZE];
	int chan;

	for (chan = 1; chan <= 2; chan++) {
	    if (done[chan])
		continue;
	    if (ORPGMGR_search_mrpg_host_name (chan, mrpg_n, MAX_STR_SIZE) ==
								chan) {
		char cmd[MAX_STR_SIZE];
		int ret;

		sprintf (cmd, "owr_client -c %d -l %s &", chan, mrpg_n);
		ret = MISC_system_to_buffer (cmd, NULL, 0, NULL);
		if (ret < 0)
		    MISC_log ("Failed in invoking owr_client (%d)", ret);
		else
		    done[chan] = 1;
	    }
	}
	if (done[1] && done[2])
	    exit (0);
	while (1) {	/* wait 10 seconds */
	    time_t t;
	    msleep (2000);
	    t = MISC_systime (NULL);
	    if (t >= crt + 10) {
		crt = t;
		break;
	    }
	}
    }
}

/**************************************************************************

    Reads command line arguments.

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
    Conf_file = STR_copy (Conf_file, "owr.conf");
    while ((c = getopt (argc, argv, "c:f:p:e:lavh?")) != EOF) {

	switch (c) {

            case 'c':
		if (sscanf (optarg, "%d", &Chan_number) != 1 ||
		    Chan_number < 0 || Chan_number > 2) {
		    fprintf (stderr, "unexpected -c option (%s)\n", optarg);
		    exit (1);
		}
                break;

            case 'f':
		Conf_file = STR_copy (Conf_file, optarg);
                break;

            case 'l':
		Use_le = 1;
                break;

            case 'a':
		Auto_start = 1;
                break;

            case 'p':
		if (sscanf (optarg, "%d", &Port_number) != 1) {
		    fprintf (stderr, "unexpected -p option (%s)\n", optarg);
		    exit (1);
		}
                break;

            case 'e':
		if (sscanf (optarg, "%d", &Owr_cmd_event) != 1) {
		    fprintf (stderr, "unexpected -e option (%s)\n", optarg);
		    exit (1);
		}
                break;

            case 'v':
		Verbose = 1;
                break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (!Auto_start) {
	if (optind != argc - 1) {	/* IP missing or more than one  */
	    fprintf (stderr, "Server name not well specified\n");
	    exit (1);
	}
	Sever_name = STR_copy (Sever_name, argv[optind]);
    }

    return (err);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        The client of the owr service. \"server\" is the server name or IP.\n\
        Options:\n\
          -p port_number (Server's TCP port number. The default is \n\
             RMTPORT + 3 or 40000)\n\
          -f config_file (Configuration file name. The default is \"owr.conf\")\n\
          -c channel_number (RPG channel number. The default is 1)\n\
          -e cmd_event (Event # for sending/receiving commands. The default is\n\
             23. The command response event number is cmd_event + 1.)\n\
          -a (Starts this on mscf for satellite connected sites)\n\
          -l (Logs to LE file)\n\
          -v (Verbose mode)\n\
          -h (Prints usage info)\n\
";

    printf ("Usage:  %s [options] server\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}

