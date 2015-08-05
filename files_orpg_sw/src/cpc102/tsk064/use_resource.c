
/******************************************************************

    This is a tool that puts system resources, memory or cpu, load
    for testing the RPG resource utilization level.
	
******************************************************************/

/* 
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/10/08 21:13:26 $
 * $Id: use_resource.c,v 1.5 2008/10/08 21:13:26 cmn Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <infr.h>
#include <orpgdat.h>
#include <prod_gen_msg.h>
#include <orpgevt.h>
#include <math.h>
#include <a309.h>

#define MAX_NAME_SIZE 128
#define N_INPUT_MSGS 20
#define ACTIVATE_STRIDE	4000
#define MAX_RADIAL_SIZE 5000
#define REPORT_PERIOD 20

static int Cpu_load = 0;
static unsigned int Memory_load = 0;
static int Input_load = 0;
static int Output_load = 0;
static int Test_cpu = 0;
static int Cpu_capability = 0;
static int Max_init_delay = 0;
static int Verbose = 0;
static int N_instances = 0;
static int Radial_input = 0;
static int Full_cpu_load = 0;

static int Start_elevation = 0;
static int End_elevation = 0;
static int Timer_cnt = 0;

static char Input_lb_name[MAX_NAME_SIZE] = "";
static char Output_lb_name[MAX_NAME_SIZE];
static int Out_fd = -1;
static int In_fd = -1;
static int Radial_fd = -1;
static int Lost_radial_cnt = 0;

static char *Cmd_str = NULL;
static int N_pids = 0;
static char *Pids = NULL;


static void Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Test_and_print_cpu_capability ();
static void Initialize ();
static void Load_CPU ();
static void Keep_memory_active ();
static void Input_data ();
static void Output_data ();
static void Cleanup_exit (int sig);
static void Timer_cb (int sig);
static void Open_LBs ();
static void Write_msg (int fd, int size);
static void Elev_cb (EN_id_t event, char *msg, int msg_len, void *arg);
static void Use_cpu ();
static char *Get_CPU_capacity_file_name ();
static void Read_radial_data ();


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    time_t st_time, t, last_report_tm;
    int max_delay, i;

    Read_options (argc, argv);

    if (Full_cpu_load) {
	while (1)
	    Use_cpu ();
    }

    if (Test_cpu)
	Test_and_print_cpu_capability ();

    /* register for termination signals */
    MISC_sig_sigset (SIGTERM, Cleanup_exit);
    MISC_sig_sigset (SIGINT, Cleanup_exit);
    MISC_sig_sigset (SIGHUP, Cleanup_exit);

    last_report_tm = 0;
    N_pids = 0;
    Cmd_str = STR_cat (Cmd_str, " &");
    for (i = 1; i < N_instances; i++) {
	int pid;
	MISC_log ("Start new instance ...\n");
	pid = MISC_system_to_buffer (Cmd_str, NULL, 0, NULL);
	if (pid < 0) {
	    MISC_log ("MISC_system_to_buffer failed (%d)\n", pid);
	    Cleanup_exit (-1);
	}
	Pids = STR_append (Pids, (char *)&pid, sizeof (int));
	N_pids++;
    }

    Initialize ();
    if (Verbose)
	MISC_log ("Initialization done\n");

    max_delay = 0;
    st_time = time (NULL);
    while (1) {
	if (Start_elevation)
	    Input_data ();
	if (End_elevation)
	    Output_data ();
	Read_radial_data ();
	t = time (NULL);
	if (t > st_time) {
	    if (t > last_report_tm + REPORT_PERIOD) {
		if (Verbose && (t - st_time > 1 || Lost_radial_cnt > 0))
		    MISC_log (
			"processing delay: %d seconds. %d radials lost\n", 
			t - st_time, Lost_radial_cnt);
		Lost_radial_cnt = 0;
		last_report_tm = t;
	    }
	    if (t > st_time + max_delay) {
		max_delay = t - st_time;
		if (max_delay > 2)
		    MISC_log ("$$ New MAX processing delay: %d seconds\n", max_delay);
	    }
	    Load_CPU ();
	    Keep_memory_active ();
	    st_time++;
	}
	else
	    msleep (300);
    }
}

/******************************************************************

    Reads radial data input.

******************************************************************/

static void Read_radial_data () {

    if (Radial_fd < 0)
	return;
    while (1) {
	char buf[MAX_RADIAL_SIZE];
	int ret;
	ret = LB_read (Radial_fd, buf, MAX_RADIAL_SIZE, LB_NEXT);
	if (ret == LB_TO_COME)
	    return;
	else if (ret == LB_EXPIRED) {
	    Lost_radial_cnt++;
	    if (Lost_radial_cnt == 1)
		MISC_log ("$$ Cannot catch up with radial input\n");
	    continue;
	}
	else if (ret < 0) {
	    MISC_log ("LB_read radial input failed (%d)\n", ret);
	    Cleanup_exit (-1);
	}
    }
}

/******************************************************************

    Adds specified CPU load to the system.

******************************************************************/

static void Load_CPU () {
    int cnt, i, st;

    if (Cpu_load <= 0)
	return;
    cnt = Cpu_load * Cpu_capability / 100 + 1;
    st = Timer_cnt;
    for (i = 0; i < cnt; i++) {
	Use_cpu ();
	if (Timer_cnt - st >= 2) {
	    MISC_log ("$$ CPU overloaded\n");
	    break;
	}
    }
}

/******************************************************************

    Use some CPU resource.

******************************************************************/

#define ARRAY_SIZE 1000

static void Use_cpu () {
    double a[ARRAY_SIZE], b[ARRAY_SIZE], c[ARRAY_SIZE], d[ARRAY_SIZE];
    int i;

    for (i = 0; i < 1000; i++) {
	a[i] = sin (1.0 + i * .00001);
	b[i] = cos (1.0 + i * .00002);
	c[i] = a[i] / b[i];
	d[i] = tan (c[i] / a[i] / b[i]);
    }
}

/******************************************************************

    Activates the memory.

******************************************************************/

static void Keep_memory_active () {
    static char *buf = NULL;
    char *p, *e;
    int st, cnt;

    if (Memory_load == 0)
	return;
    if (buf == NULL) {
	buf = malloc (Memory_load);
	if (buf == NULL) {
	    MISC_log ("malloc (%d) failed\n", Memory_load);
	    Cleanup_exit (-1);
	}
	if (Verbose)
	    MISC_log ("Memory load is %d MB\n", Memory_load >> 20);
    }
    st = Timer_cnt;
    p = buf;
    e = buf + Memory_load;
    cnt = 0;
    while (p < e) {
	*p = 'a';
	p += ACTIVATE_STRIDE;
	cnt++;
	if ((cnt % 100) == 0) {
	    if (Timer_cnt - st >= 2) {
		MISC_log ("$$ Memory overloaded\n");
		break;
	    }
	}
    }
}

/******************************************************************

    Reads input data.

******************************************************************/

static void Input_data () {
    LB_id_t id;
    char *msg;
    int ret;

    if (In_fd < 0)
	return;
    Start_elevation = 0;
    id = (random () % N_INPUT_MSGS) + 1;
    ret = LB_read (In_fd, (char *)&msg, LB_ALLOC_BUF, id);
    if (ret < 0)
	MISC_log ("LB_read input data failed (%d)\n", ret);
    else if (ret > 0)
	free (msg);
    if (Verbose)
	MISC_log ("Input %d bytes\n", ret);
}

/******************************************************************

    Outputs data to the RPG data base.

******************************************************************/

static void Output_data () {
    if (Out_fd < 0)
	return;
    End_elevation = 0;
    if (Verbose)
	MISC_log ("Output %d bytes\n", Output_load * 1024);
    Write_msg (Out_fd, Output_load * 1024);
}

/******************************************************************

    Tests CPU capability, prints the result and terminates.

******************************************************************/

static void Test_and_print_cpu_capability () {
    struct timeval t1, t2;
    time_t st;
    int cnt, ms, cap, i;
    FILE *fl;

    for (i = 0; i < 100; i++)
	Use_cpu ();
    gettimeofday (&t1, NULL);
    st = time (NULL);
    cnt = 0;
    while (time (NULL) <= st + 5) {
	for (i = 0; i < 100; i++)
	    Use_cpu ();
	cnt += 100;
    }
    gettimeofday (&t2, NULL);
    ms = (t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec) * .001;
    cap = (double) cnt * 1000 / ms;
    fl = fopen (Get_CPU_capacity_file_name (), "w");
    if (fl == NULL) {
	MISC_log ("fopen (%s) failed\n", Get_CPU_capacity_file_name ());
	Cleanup_exit (-1);
    }
    fprintf (fl, "%d", cap);
    printf ("CPU capacity is %d\n", cap);
    exit (0);
}

/******************************************************************

    Initializes the task before operation.

******************************************************************/

static char *Get_CPU_capacity_file_name () {
    static char fname[MAX_NAME_SIZE];
    int ret;

    ret = MISC_get_cfg_dir (fname, MAX_NAME_SIZE - 32);
    if (ret < 0) {
	MISC_log ("MISC_get_cfg_dir failed (%d)\n", ret);
	Cleanup_exit (-1);
    }
    strcat (fname, "/.CPU_capability");
    return (fname);
}

/******************************************************************

    Initializes the task before operation.

******************************************************************/

static void Initialize () {
    int ret;

    /* wait for a random time */
    if (Max_init_delay) {
	struct timeval t0, t;
	int delay;
	gettimeofday (&t0, NULL);
	srandom (t0.tv_usec);
	delay = random () % (Max_init_delay + 1);
	t.tv_sec = t0.tv_sec;
	while (t.tv_sec <= t0.tv_sec + delay) {
	    sleep (1);
	    gettimeofday (&t, NULL);
	}
	MISC_log ("use_resource starts\n");
    }

    if (Cpu_load > 0) {
	FILE *fl;
	char *fname, buf[128];

	fname = Get_CPU_capacity_file_name ();
	fl = fopen (fname, "r");
	if (fl == NULL) {
	    MISC_log ("cannot find file %s - run this with -t option first\n", fname);
	    Cleanup_exit (-1);
	}
	if (fgets (buf, 128, fl) == NULL ||
	    sscanf (buf, "%d", &Cpu_capability) != 1 ||
	    Cpu_capability <= 0) {
	    MISC_log ("Bad content in file %s - run this with -t option first\n", fname);
	    Cleanup_exit (-1);
	}
	fclose (fl);
	if (Verbose)
	    MISC_log ("CPU load is %d percent of capacity %d\n", 
					Cpu_load, Cpu_capability);
    }

    Open_LBs ();

    {					/* sets one second timer */
	struct itimerval itv;
	itv.it_interval.tv_sec = 1;
	itv.it_value.tv_sec = 1;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_usec = 0;
	ret = setitimer (ITIMER_REAL, &itv, NULL);
	if (ret < 0) {
	    MISC_log ("setitimer failed (errno %d)\n", errno);
	    Cleanup_exit (-1);
	}
	MISC_sig_sigset (SIGALRM, Timer_cb);
    }

    /* register for the elevation event */
    ret = EN_register (ORPGEVT_SCAN_INFO, Elev_cb);
    if (ret < 0) {
	MISC_log ("EN_register (ORPGEVT_SCAN_INFO) failed (%d)\n", ret);
	Cleanup_exit (-1);
    }
}

/******************************************************************

    Elevation event callback funciton.

******************************************************************/

static void Elev_cb (EN_id_t event, char *msg, int msg_len, void *arg) {
    orpgevt_scan_info_t *scan_info;

    if (msg_len < sizeof (orpgevt_scan_info_t)) {
	MISC_log ("Unexpected scan_info event message size (%d)\n", msg_len);
	return;
    }
    scan_info = (orpgevt_scan_info_t *) msg;
    if (scan_info->key == ORPGEVT_BEGIN_ELEV || 
				scan_info->key == ORPGEVT_BEGIN_VOL)
	Start_elevation = 1;
    else if (scan_info->key == ORPGEVT_END_ELEV || 
				scan_info->key == ORPGEVT_END_VOL)
	End_elevation = 1;
}

/******************************************************************

    Creates and initializes the input LB and opens the output LB.

******************************************************************/

static void Open_LBs () {
    int ret;

    ret = MISC_get_work_dir (Input_lb_name, MAX_NAME_SIZE - 32);
    if (ret < 0) {
	MISC_log ("MISC_get_work_dir failed (%d)\n", ret);
	Cleanup_exit (-1);
    }
    sprintf (Input_lb_name + strlen (Input_lb_name), 
		"/use_rsc_inp.%d.lb", (int)getpid ());

    ret = CS_entry_int_key (ORPGDAT_PRODUCTS, 
				1, MAX_NAME_SIZE, Output_lb_name);
    if (ret <= 0) {
	MISC_log ("RPG product database not found (ORPGDAT_PRODUCTS)\n");
	Cleanup_exit (-1);
    }

    if (Input_load > 0) {
	LB_attr attr;
	int i;
	attr.mode = 0770;
	attr.msg_size = Input_load * 1024;
	attr.maxn_msgs = N_INPUT_MSGS;
	attr.types = 0;
	attr.tag_size = 0;
	In_fd = LB_open (Input_lb_name, LB_CREATE, &attr);
	if (In_fd < 0) {
	    MISC_log ("LB_open %s failed (%d)\n", Input_lb_name, In_fd);
	    Cleanup_exit (-1);
	}
	for (i = 0; i < N_INPUT_MSGS; i++)
	    Write_msg (In_fd, Input_load * 1024);
    }
    if (Output_load > 0) {
	Out_fd = LB_open (Output_lb_name, LB_WRITE, NULL);
	if (Out_fd < 0) {
	    MISC_log ("LB_open %s failed (%d)\n", Output_lb_name, Out_fd);
	    Cleanup_exit (-1);
	}
    }
    if (Radial_input) {
	char name[MAX_NAME_SIZE];
	ret = CS_entry_int_key (BASEDATA, 1, MAX_NAME_SIZE, name);
	if (ret <= 0) {
	    MISC_log ("RPG base radial data not found (BASEDATA)\n");
	    Cleanup_exit (-1);
	}
	Radial_fd = LB_open (name, LB_READ, NULL);
	if (Radial_fd < 0) {
	    MISC_log ("LB_open %s failed (%d)\n", name, Radial_fd);
	    Cleanup_exit (-1);
	}
    }
}

/******************************************************************

    Writes a message of "size" to LB "fd".

******************************************************************/

static void Write_msg (int fd, int size) {
    static char *buf = NULL;
    static int buf_size = 0;
    Prod_header *hd;
    int ret;

    if (size < sizeof (Prod_header))
	size = sizeof (Prod_header);
    if (size > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = malloc (size);
	if (buf == NULL) {
	    MISC_log ("malloc (%d) failed\n", size);
	    Cleanup_exit (-1);
	}
	buf_size = size;
    }

    hd = (Prod_header *) buf;
    memset (hd, 0, sizeof (Prod_header));
    hd->g.prod_id = 2900;
    hd->g.gen_t = time (NULL);
    hd->g.vol_t = hd->g.gen_t;
    hd->g.len = size;
    hd->elev_t = hd->g.gen_t;
    ret = LB_write (fd, buf, size, LB_ANY);
    if (ret < 0)
	MISC_log ("LB_write failed (%d)\n", ret);
}

/******************************************************************

    The termination signal callback. Preforms cleanup and terminates
    this process.

******************************************************************/

static void Cleanup_exit (int sig) {
    int i, *pids;

    pids = (int *)Pids;
    for (i = 0; i < N_pids; i++)
	kill (pids[i], SIGTERM);
    if (Input_lb_name[0] != '\0')
	LB_remove (Input_lb_name);
    if (sig < 0)
	exit (1);
    exit (0);
}

/******************************************************************

    The timer signal callback. A timer count is incremented.

******************************************************************/

static void Timer_cb (int sig) {
    Timer_cnt++;
}

/**************************************************************************

    Reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static void Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    Cmd_str = STR_copy (Cmd_str, argv[0]);
    while ((c = getopt (argc, argv, "c:m:i:o:d:n:tfrvh?")) != EOF) {
	switch (c) {

            case 'c':
		if (sscanf (optarg, "%d", &Cpu_load) != 1 ||
		    Cpu_load < 0 || Cpu_load > 100) {
		    fprintf (stderr, "unexpected -c option\n");
		    err = -1;
		}
		Cmd_str = STR_cat (Cmd_str, " -c ");
		Cmd_str = STR_cat (Cmd_str, optarg);
                break;

            case 'm':
		if (sscanf (optarg, "%d", &Memory_load) != 1 ||
		    Memory_load < 0) {
		    fprintf (stderr, "unexpected -m option\n");
		    err = -1;
		}
		Memory_load *= 1024 * 1024;
		Cmd_str = STR_cat (Cmd_str, " -m ");
		Cmd_str = STR_cat (Cmd_str, optarg);
                break;

            case 'i':
		if (sscanf (optarg, "%d", &Input_load) != 1 ||
		    Input_load < 0) {
		    fprintf (stderr, "unexpected -i option\n");
		    err = -1;
		}
		Cmd_str = STR_cat (Cmd_str, " -i ");
		Cmd_str = STR_cat (Cmd_str, optarg);
                break;

            case 'o':
		if (sscanf (optarg, "%d", &Output_load) != 1 ||
		    Output_load < 0) {
		    fprintf (stderr, "unexpected -o option\n");
		    err = -1;
		}
		Cmd_str = STR_cat (Cmd_str, " -o ");
		Cmd_str = STR_cat (Cmd_str, optarg);
                break;

            case 'd':
		if (sscanf (optarg, "%d", &Max_init_delay) != 1 ||
		    Max_init_delay < 0) {
		    fprintf (stderr, "unexpected -d option\n");
		    err = -1;
		}
		Cmd_str = STR_cat (Cmd_str, " -d ");
		Cmd_str = STR_cat (Cmd_str, optarg);
                break;

            case 'n':
		if (sscanf (optarg, "%d", &N_instances) != 1 ||
		    N_instances < 0) {
		    fprintf (stderr, "unexpected -n option\n");
		    err = -1;
		}
                break;

            case 't':
		Cmd_str = STR_cat (Cmd_str, " -t ");
		Test_cpu = 1;
                break;

            case 'r':
		Cmd_str = STR_cat (Cmd_str, " -r ");
		Radial_input = 1;
                break;

            case 'f':
		Full_cpu_load = 1;
                break;

            case 'v':
		Cmd_str = STR_cat (Cmd_str, " -v ");
		Verbose = 1;
                break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		exit (0);
	}
    }
    if (err < 0)
	Cleanup_exit (-1);

    return;
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
	Adds CPU, memory and IO load for testing purpose\n\
\n\
	Options:\n\
	  -c cpu_load (CPU load in percent)\n\
	  -m memory_load (memory load in MB)\n\
	  -f (apply full CPU load)\n\
	  -i input_load (input load in KB per elevation)\n\
	  -r (input base radial data)\n\
	  -o output_load (output load in KB per elevation)\n\
	     The default for the above options is 0.\n\
	  -d max_init_delay (The maximum delay in seconds of initialization.\n\
	     A random delay between 0 and this is applied before operation. \n\
	     The default is 0 - no delay)\n\
	  -n (number of instances started. The deafult is 1.)\n\
	  -t (Test CPU capability and then terminate)\n\
	  -v (verbose mode)\n\
	  -h (prints usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);
}
