
/******************************************************************

    This tool configures all RPG hardware devices.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/10 14:02:00 $
 * $Id: config_all.c,v 1.22 2014/04/10 14:02:00 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>

#include <infr.h> 

#define STR_SIZE 256

enum {CA_GET_PSWD, CA_CONSOLE, CA_LAN, CA_MSP, CA_RTR, CA_HUB, CA_UPS, CA_PTI_A, CA_PTI_B, CA_DIO, CA_CONSOLE_UPDATE};			/* device index */
enum {CA_NOT_DONE, CA_NOT_NEEDED, CA_SPAWNED, CA_FAILED, CA_FINISHED, CA_DONE, CP_CANNOT_CFG};			/* values for Device_t.status */

typedef struct {
    char *name;			/* name of the device */
    void *cp;			/* coprocessor identifier */
    int status;			/* processing status */
    int wait_secs;		/* waiting seconds after finish before done. */
    char *cmd;			/* configuration command */
} Device_t;			/* structure for device */

/* When a device configuration is finished, its service may not be ready. We 
   have to wait until the required service is available before we can proceed 
   to configure a device that depends on the service. */

static Device_t Devices[] = {
    {"- Get passwords",	NULL, CA_NOT_DONE,  0, "config_device -w -n -N lan"},
    {"Console Server",	NULL, CP_CANNOT_CFG,  15, "config_device -n console"},
    {"LAN",		NULL, CP_CANNOT_CFG,15,	"config_device -n -N -f lan"},
    {"Power Administrator",
			NULL, CP_CANNOT_CFG,15,	"config_device -n -s msp"},
    {"RPG Router",	NULL, CP_CANNOT_CFG, 0,	
					"config_device -n -N -s -f rtr"},
    {"Frame Relay Router",
			NULL, CP_CANNOT_CFG, 0,	
					"config_device -n -N -s -f hub"},
    {"UPS",		NULL, CP_CANNOT_CFG, 0,	"config_device -n -s ups"},
    {"PTI-A",		NULL, CP_CANNOT_CFG, 0,	"config_device -n -s pti-a"},
    {"PTI-B",		NULL, CP_CANNOT_CFG, 0,	"config_device -n -s pti-b"},
    {"DIO Module",	NULL, CP_CANNOT_CFG, 0,	"config_device -n -s dio"},
    {"- Console update",	NULL, CP_CANNOT_CFG,15, "config_device console"},
};				/* array of all devices to configure */

static int Connecting_dev = -1;	/* device index that is corrently connecting to
				   the console server. Only one device can do 
				   this at any given time. */

static int N_devices = sizeof (Devices) / sizeof (Device_t);
				/* array size of Devices */

static int Need_console_server_firmware_update = 0;
static int Passwd_ok = 1;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Read_cps ();
static void Perform_the_work ();
static void Spawn_cp (int dev_ind);
static int Read_password (char *prompt, char *buf, int size, int dev_ind);
static void Sig_handle (int sig);
static void Set_password_options (char *text);
static void Print_and_exit (char *msg, int status);
static void Set_unset_echo (struct termios *attr, int set, int has_term);
void Wait_for_any_input ();
int Get_terminal (struct termios *attr);


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    struct stat st;
    int ret;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if ((ret = LE_init (argc, argv)) != 0) {
	LE_send_msg (0, "LE_init failed (%d)", ret);
	exit (1);
    }

    if (stat ("/tftpboot/lan-cfg", &st) < 0 ||
	!S_ISREG (st.st_mode))
	Print_and_exit ("Site files not found in /tftpboot - Not installed?\n", 1);

    ret = MISC_system_to_buffer ("find_adapt -V HLRPG@/tftpboot/hub-cfg", NULL, 0, NULL);
    if (ret < 0) {
	char msg[256];
	sprintf (msg, "MISC_system_to_buffer find_adapt failed (%d)\n", ret);
	Print_and_exit (msg, ret);
    }
    else if (ret == 0) 
	LE_send_msg (0, "This is an RPG HUB loading site");
    else {
	LE_send_msg (0, "Not an RPG HUB router loading site (%d)", 
							WEXITSTATUS (ret));
	Devices[CA_HUB].status = CA_NOT_NEEDED;
    }

    if (MISC_system_to_buffer ("find_adapt -V FAA_CH1@/tftpboot/lan-cfg", NULL, 0, NULL) != 0 &&
	MISC_system_to_buffer ("find_adapt -V FAA_CH2@/tftpboot/lan-cfg", NULL, 0, NULL) != 0) {
	LE_send_msg (0, "Not a FAA radar");
	Devices[CA_DIO].status = CA_NOT_NEEDED;
	Devices[CA_PTI_A].status = CA_NOT_NEEDED;
	Devices[CA_PTI_B].status = CA_NOT_NEEDED;
    }

    MISC_sig_sigset (SIGINT, Sig_handle);
    MISC_sig_sigset (SIGTERM, Sig_handle);
    MISC_sig_sigset (SIGHUP, Sig_handle);

    printf ("Note: Please standby - You will be asked to enter passwords - hit return to continue\n");
    fflush (stdout);
    Wait_for_any_input ();

    while (1) {
	Read_cps ();
	Perform_the_work ();
	sleep (1);
    }

    exit (0);
}

/*******************************************************************

    Signal handler.

*******************************************************************/

void Wait_for_any_input () {
    struct termios attr;
    char buf[128];

    if (Get_terminal (&attr)) {
	printf ("Type Enter to continue: ");
	fflush (stdout);
    }
    while (read (STDIN_FILENO, buf, 127) < 0 && errno == EINTR);
}

/*******************************************************************

    Signal handler.

*******************************************************************/

static void Sig_handle (int sig) {
    int i;
    char msg[256];

    for (i = 0; i < N_devices; i++) {
	Device_t *dev = Devices + i;
	if (dev->status == CA_SPAWNED)
	    MISC_cp_close (dev->cp);
    }
    sprintf (msg, "Signal %d received\n", sig);
    Print_and_exit (msg, 1);
}

/**************************************************************************

    Reads output from all co-processors.

**************************************************************************/

#define BUF_SIZE 512

static void Read_cps () {
    static int pswd_done = 0;
    int i;

    for (i = 0; i < N_devices; i++) {
	char buf[BUF_SIZE];
	Device_t *dev;
	int ret;

	dev = Devices + i;
	if (dev->status != CA_SPAWNED)
	    continue;
	ret = MISC_cp_read_from_cp (dev->cp, buf, BUF_SIZE);
	if (ret < 0) {
	    unsigned int status;
	    status = MISC_cp_get_status (dev->cp);
	    if ((i == CA_CONSOLE || i == CA_CONSOLE_UPDATE) && 
					WEXITSTATUS (status) == 2) {
		Need_console_server_firmware_update = 1;
		if (i == CA_CONSOLE)
		    LE_send_msg (0, "Console firmware update failed");
		else
		    LE_send_msg (0, "Console firmware update failed again");
		status = 0;
	    }
	    if (i == CA_GET_PSWD) {
		if (WEXITSTATUS (status) == 3)
		    Print_and_exit ("Timeout in getting password\n", 1);
		if (WEXITSTATUS (status) == 4)
		    Print_and_exit ("Password check failed\n", 1);
		dev->status = CA_FINISHED;
	    }
	    else {
		if (status != 0) {
		    dev->status = CA_FAILED;
		    LE_send_msg (0, "Configuring %s failed", dev->name);
		    printf ("Configuring %s failed\n", dev->name);
		    fflush (stdout);
		}
		else {
		    LE_send_msg (0, "Configuring %s succeeded", dev->name);
		    printf ("Configuring %s succeeded\n", dev->name);
		    fflush (stdout);
		    dev->status = CA_FINISHED;
		}
	    }
	    if (i == Connecting_dev)
		Connecting_dev = -1;
	    if ((i == CA_GET_PSWD && status == 0 && Passwd_ok) ||
		(i == CA_RTR && Devices[CA_HUB].status == CA_NOT_NEEDED) ||
		i == CA_HUB) {
		if (!pswd_done) {
		    LE_send_msg (0, "Passwords obtained and verified");
		    printf ("Note: Passwords obtained and verified - No more questions will be asked\n");
		    fflush (stdout);
		    pswd_done = 1;
		}
	    }
	}
	else if (ret > 0) {
	    int off;
	    char b[STR_SIZE], *p, *text, *pp;
	    p = buf;
	    while ((off = MISC_get_token (p, "S\n", 0, b, STR_SIZE)) > 0) {
		char *pt;
		text = b;
		if ((pp = strstr (b, ": ")) != NULL)
		    text = pp + 2;
		if (i == CA_GET_PSWD && 
				(pt = strstr (text, "Passwords for ")) != NULL)
		    Set_password_options (pt + strlen ("Passwords for "));
		else
		    LE_send_msg (0, "%s: %s", dev->name, text);
		p += off;
		if (i == Connecting_dev && 
				strstr (text, "Connected to port ") != NULL)
		    Connecting_dev = -1;
		if (strstr (b, "Password:") != NULL) {
		    char pswd[STR_SIZE];
		    if (Read_password (b, pswd, STR_SIZE, i) <= 1)
			Read_password (b, pswd, STR_SIZE, i);
		    MISC_cp_write_to_cp (dev->cp, pswd);
		}
	    }
	}
    }
}

/**************************************************************************

    Performs the necessary configuration steps.

**************************************************************************/

static void Perform_the_work () {
    static time_t prev_tm = 0;
    time_t cr_tm;
    int i, done, failed;

    cr_tm = time (NULL);
    if (prev_tm == 0)
	prev_tm = cr_tm;
    for (i = 0; i < N_devices; i++) {

	Device_t *dev = Devices + i;
	if (dev->status == CA_FINISHED) {
	    dev->wait_secs -= cr_tm - prev_tm;
	    if (dev->wait_secs <= 0) {
		dev->status = CA_DONE;
	    }
	}
	switch (i) {

	    case CA_GET_PSWD:
		Spawn_cp (CA_GET_PSWD);
		if (dev->status == CA_DONE) {
		    Spawn_cp (CA_CONSOLE);
		}
	    break;

	    case CA_CONSOLE:
		if (dev->status == CA_DONE) {
		    Spawn_cp (CA_LAN);
		}
	    break;

	    case CA_LAN:
		if (dev->status == CA_DONE) {
		    Spawn_cp (CA_CONSOLE_UPDATE);
		}
	    break;

	    case CA_CONSOLE_UPDATE:
		if (dev->status == CA_DONE) {
		    Spawn_cp (CA_MSP);
		    Spawn_cp (CA_RTR);
		    Spawn_cp (CA_UPS);
		}
	    break;

	    case CA_RTR:
		if (dev->status == CA_DONE) {
		    Spawn_cp (CA_HUB);
		}
	    break;

	    case CA_MSP:
		if (dev->status == CA_DONE) {
		    Spawn_cp (CA_PTI_A);
		    Spawn_cp (CA_PTI_B);
		    Spawn_cp (CA_DIO);
		}
	    break;

	    case CA_UPS:
	    case CA_PTI_A:
	    case CA_PTI_B:
	    case CA_DIO:
	    break;
	}
    }
    prev_tm = cr_tm;

    done = 1;
    failed = 0;
    for (i = 0; i < N_devices; i++) {
	Device_t *dev = Devices + i;
	if (dev->status == CA_NOT_DONE || dev->status == CA_SPAWNED || 
					dev->status == CA_FINISHED) {
	    done = 0;
	    break;
	}
	if (dev->status == CA_FAILED || dev->status == CP_CANNOT_CFG)
	    failed = 1;
    }
    if (done) {
	if (!failed)
	    Print_and_exit ("Configuration of all devices completed\n", 0);
	else {
	    char buf[STR_SIZE];
	    buf[0] = '\0';
	    for (i = 0; i < N_devices; i++) {
		if (Devices[i].status == CA_FAILED)
		    sprintf (buf + strlen (buf), "%s, ", Devices[i].name);
	    }
	    if (strlen (buf) > 0) {
		buf[strlen (buf) - 2] = '\0';
		LE_send_msg (0, "Failed devices: %s", buf);
		fprintf (stderr, "Failed devices: %s\n", buf);
	    }
	    buf[0] = '\0';
	    for (i = 0; i < N_devices; i++) {
		if (Devices[i].status == CP_CANNOT_CFG)
		    sprintf (buf + strlen (buf), "%s, ", Devices[i].name);
	    }
	    if (strlen (buf) > 0) {
		buf[strlen (buf) - 2] = '\0';
		LE_send_msg (0, "Not configured: %s", buf);
		fprintf (stderr, "Not configured: %s\n", buf);
	    }
	    Print_and_exit ("Error detected in configuration all devices\n", 1);
	}
    }
}

/**************************************************************************

    Write "msg" to both LE and the stdout/stderr port, sleep 3 seconds and
    exit with status "status".

**************************************************************************/

static void Print_and_exit (char *msg, int status) {

    LE_send_msg (0, msg);
    if (status == 0) {
	printf (msg);
	fflush (stdout);
    }
    else
	fprintf (stderr, msg);
    sleep (3);
    exit (status);
}

/**************************************************************************

    Spawns the device of index "dev_ind".

**************************************************************************/

static void Spawn_cp (int dev_ind) {
    int ret;

    if (dev_ind == CA_CONSOLE_UPDATE && !Need_console_server_firmware_update) {
	Devices[dev_ind].status = CA_DONE;
	return;		/* no need to run console update */
    }

    if (Devices[dev_ind].status != CA_NOT_DONE &&
			Devices[dev_ind].status != CP_CANNOT_CFG)
	return;

    if (Connecting_dev >= 0)
	return;

    LE_send_msg (0, "Start to configure %s", Devices[dev_ind].name);
    printf ("Start to configure %s\n", Devices[dev_ind].name);
    fflush (stdout);
    ret = MISC_cp_open (Devices[dev_ind].cmd, 
				MISC_CP_MANAGE, &(Devices[dev_ind].cp));
    if (ret != 0) {
	LE_send_msg (0, "Failed (%d) of MISC_cp_open \"%s\"", 
				ret, Devices[dev_ind].cmd);
	Devices[dev_ind].status = CA_FAILED;
    }
    else {
	Devices[dev_ind].status = CA_SPAWNED;
	Connecting_dev = dev_ind;
    }
}

/**************************************************************************

    Reads password from stdin.

**************************************************************************/

static int Read_password (char *prompt, char *buf, int size, int dev_ind) {

    struct termios attr;
    int n, has_term;
    char prmpt[STR_SIZE], *p;

    strncpy (prmpt, prompt, STR_SIZE - 1);
    prmpt[STR_SIZE - 1] = '\0';
    p = prmpt + (strlen (prmpt) - 1);
    while (p >= prmpt && *p == '\n') {
	*p = '\0';
	p--;
    }

    lseek (STDIN_FILENO, 0, SEEK_END);
    has_term = Get_terminal (&attr);
    if (has_term) {
	printf ("%s", prmpt);
	fflush (stdout);
	Set_unset_echo (&attr, 1, has_term);
    }
    else {		/* runs as the coprocessor */
	printf ("%s\n", prmpt);
	fflush (stdout);
    }

    while (1) {
	int status;
	struct pollfd pfd;
	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN | POLLPRI;
	if (poll (&pfd, 1, 1000) > 0) {
	    while ((n = read (STDIN_FILENO, buf, size - 1)) < 0 && 
							errno == EINTR);
	    break;
	}
	status = MISC_cp_get_status (Devices[dev_ind].cp);
	if (status != 0) {
	    Set_unset_echo (&attr, 0, has_term);
	    if (WEXITSTATUS (status) == 3)
		Print_and_exit ("Timeout in waiting for password\n", 1);
	    else
		Print_and_exit ("config_device killed\n", 1);
	}
    }

    if (n < 0) {
	char msg[256];
	sprintf (msg, "read stdin failed (ret %d, errno %d)\n", n, errno);
	Set_unset_echo (&attr, 0, has_term);
	Print_and_exit (msg, 1);
    }

    buf[n] = '\0';
    Set_unset_echo (&attr, 0, has_term);

    return (n);
}

/*************************************************************************

    Gets terminal attributes. Returns 1 if this process has a terminal or
    0 otherwise.

*************************************************************************/

int Get_terminal (struct termios *attr) {
    int has_term = 1;
    if (tcgetattr (STDIN_FILENO, attr) != 0) {
	LE_send_msg (0, "tcgetattr failed - Not running from a terminal");
	has_term = 0;
    }
    return (has_term);
}

/*************************************************************************

    Sets/unsets ECHO for the terminal of "attr".

*************************************************************************/

static void Set_unset_echo (struct termios *attr, int set, int has_term) {

    if (!has_term)
	return;
    if (set) {
	attr->c_lflag &= ~(ECHO);
	sigset (SIGINT, SIG_HOLD);
	if (tcsetattr (STDIN_FILENO, TCSAFLUSH, attr) != 0)
	    LE_send_msg (0, "tcsetattr TCSAFLUSH failed\n");
    }
    else {
	attr->c_lflag |= ECHO;
	if (tcsetattr (STDIN_FILENO, TCSANOW, attr) != 0)
	    LE_send_msg (0, "tcsetattr TCSANOW failed\n");
	sigset (SIGINT, SIG_DFL);
	printf ("\n");
	fflush (stdout);
    }
}

/*************************************************************************

    Parses the "config_device -w" output, "text", to get the password and
    modifies the command for the appropriate device by adding the password
    options.

*************************************************************************/

static void Set_password_options (char *text) {
    char tok[STR_SIZE], *opts;
    int i;

    if (MISC_get_token (text, "S-", 4, tok, STR_SIZE) <= 0)
	return;
    MISC_get_token (text, "S-", 0, tok, STR_SIZE);
    if (strcmp (tok, "lan") == 0)
	i = CA_LAN;
    else if (strcmp (tok, "rtr") == 0)
	i = CA_RTR;
    else if (strcmp (tok, "hub") == 0)
	i = CA_HUB;
    else
	return;

    opts = STR_copy (NULL, "");
    MISC_get_token (text, "S-", 1, tok, STR_SIZE);
    if (strlen (tok) > 0) {
	opts = STR_cat (opts, " -P ");
	opts = STR_cat (opts, tok);
    }
    else
	Passwd_ok = 0;
    MISC_get_token (text, "S-", 3, tok, STR_SIZE);
    if (strlen (tok) > 0) {
	opts = STR_cat (opts, " -P ");
	opts = STR_cat (opts, tok);
    }
    MISC_get_token (text, "S-", 2, tok, STR_SIZE);
    if (strlen (tok) > 0) {
	opts = STR_cat (opts, " -S ");
	opts = STR_cat (opts, tok);
    }
    else
	Passwd_ok = 0;
    MISC_get_token (text, "S-", 4, tok, STR_SIZE);
    if (strlen (tok) > 0) {
	opts = STR_cat (opts, " -S ");
	opts = STR_cat (opts, tok);
    }
    else
	Passwd_ok = 0;
    if (strlen (opts) > 0) {
	Devices[i].cmd = STR_copy (NULL, Devices[i].cmd);
	if (MISC_get_token (Devices[i].cmd, "", 0, tok, STR_SIZE) > 0)
	    Devices[i].cmd = STR_replace (Devices[i].cmd, strlen (tok), 0, opts, strlen (opts));
    }
    STR_free (opts);
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
    while ((c = getopt (argc, argv, "h?")) != EOF) {
	switch (c) {

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
    char *usage = "\
        Configures the following RPG hardware devices: The console server, \n\
        the LAN switch, the RPG router, the frame Raley HUB router (if \n\
        applicable), the UPS, the power administrator, the three PTI comms \n\
        servers and the DIO module.\n\
        Options:\n\
            -h (show usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
